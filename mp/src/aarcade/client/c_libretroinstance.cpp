#include "cbase.h"
#include "aa_globals.h"
// ;..\..\portaudio\lib\portaudio_x86.lib

#include "c_libretroinstance.h"
#include "c_anarchymanager.h"
#include "../../../public/vgui_controls/Controls.h"
#include "vgui/IInput.h"
#include "c_canvasregen.h"
#include "c_embeddedinstance.h"

#include "../../public/bitmap/tgawriter.h"
#include "../../public/pixelwriter.h"

#include <algorithm>

#include "XUnzip.h"
#include "../../7z/7zFile.h"
#include "../../7z/7zAlloc.h"
#include "../../7z/7zCrc.h"
#include "../../7z/7z.h"

#include <mutex>

// OpenGL includes (only in cpp to avoid header pollution)
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "../../glew/include/GL/glew.h"
#include "../../glew/include/GL/wglew.h"

// OpenGL context structure (hidden from header to avoid type conflicts)
struct LibretroGLContext
{
	HWND hwnd;                           // Hidden window handle
	HDC hdc;                             // Device context
	HGLRC hglrc;                         // OpenGL rendering context
	GLuint framebuffer;
	GLuint color_texture;
	GLuint depth_stencil_renderbuffer;
	unsigned int hw_render_width;
	unsigned int hw_render_height;
};

// for generating timestamps to use in filenmaes of task screenshots (takeScreenshotNow)
#include <chrono>
#include <iomanip> // put_time
#include <fstream>
#include <sstream> // stringstream

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const size_t kInputBufSize = (1 << 18);  // 256 KB buffer

// Active hardware-rendering instance pointer.
// Only one HW core runs at a time (WGL context is exclusive),
// so a single static pointer is safe. Allows v3d_get_current_framebuffer()
// to work from any thread (PPSSPP spawns 32+ internal threads).
static C_LibretroInstance* s_pActiveHWInstance = null;

static bool InitCrc()
{
	CrcGenerateTable();
	return true;
}
static bool s_bCrcDummy = InitCrc();  // Runs once at program start

// Lock-free SPSC ring buffer helpers for non-blocking audio
static inline unsigned int RingBuf_Available(const AudioRingBuffer_t* pRing)
{
	return (unsigned int)((LONG)pRing->nWritePos - (LONG)pRing->nReadPos);
}

static inline unsigned int RingBuf_Free(const AudioRingBuffer_t* pRing)
{
	return pRing->nCapacity - RingBuf_Available(pRing);
}

static unsigned int RingBuf_Write(AudioRingBuffer_t* pRing, const int16_t* pData, unsigned int nSamples)
{
	unsigned int nFree = RingBuf_Free(pRing);
	if (nSamples > nFree)
		nSamples = nFree;

	if (nSamples == 0)
		return 0;

	unsigned int nWriteIdx = (unsigned int)pRing->nWritePos & pRing->nMask;
	unsigned int nFirstChunk = pRing->nCapacity - nWriteIdx;

	if (nFirstChunk >= nSamples)
	{
		Q_memcpy(pRing->pBuffer + nWriteIdx, pData, nSamples * sizeof(int16_t));
	}
	else
	{
		Q_memcpy(pRing->pBuffer + nWriteIdx, pData, nFirstChunk * sizeof(int16_t));
		Q_memcpy(pRing->pBuffer, pData + nFirstChunk, (nSamples - nFirstChunk) * sizeof(int16_t));
	}

	InterlockedExchangeAdd(&pRing->nWritePos, (LONG)nSamples);
	return nSamples;
}

static unsigned int RingBuf_Read(AudioRingBuffer_t* pRing, int16_t* pDest, unsigned int nSamples)
{
	unsigned int nAvail = RingBuf_Available(pRing);
	if (nSamples > nAvail)
		nSamples = nAvail;

	if (nSamples == 0)
		return 0;

	unsigned int nReadIdx = (unsigned int)pRing->nReadPos & pRing->nMask;
	unsigned int nFirstChunk = pRing->nCapacity - nReadIdx;

	if (nFirstChunk >= nSamples)
	{
		Q_memcpy(pDest, pRing->pBuffer + nReadIdx, nSamples * sizeof(int16_t));
	}
	else
	{
		Q_memcpy(pDest, pRing->pBuffer + nReadIdx, nFirstChunk * sizeof(int16_t));
		Q_memcpy(pDest + nFirstChunk, pRing->pBuffer, (nSamples - nFirstChunk) * sizeof(int16_t));
	}

	InterlockedExchangeAdd(&pRing->nReadPos, (LONG)nSamples);
	return nSamples;
}

// Worker-thread-safe debug output (Source Engine spew is not thread-safe)
static void WorkerDbgMsg(const char* fmt, ...)
{
	char buf[2048];
	va_list args;
	va_start(args, fmt);
	V_vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	OutputDebugStringA(buf);
}

// SEH wrapper for libretro core calls -- isolated from C++ destructors
// Returns true on success, false if the core crashed
static bool SafeRunCore(libretro_raw* raw)
{
	__try {
		raw->run();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DWORD code = GetExceptionCode();
		WorkerDbgMsg("libretro: SEH caught exception 0x%08X during run()\n", code);
		return false;
	}
}

// SEH wrapper for libretro core shutdown calls
static bool SafeCallCore(void(*fn)(void))
{
	__try {
		fn();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DWORD code = GetExceptionCode();
		WorkerDbgMsg("libretro: SEH caught exception 0x%08X during core call\n", code);
		return false;
	}
}

// SEH wrapper for retro_load_game -- different signature than SafeCallCore
static bool SafeLoadGame(bool(*fn)(const struct retro_game_info*), const struct retro_game_info* game)
{
	__try {
		return fn(game);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DWORD code = GetExceptionCode();
		WorkerDbgMsg("libretro: SEH caught exception 0x%08X during load_game()\n", code);
		return false;
	}
}

// Pre-register callbacks before init() for cores like bsnes that inspect
// callback pointers during init() to set up video/audio subsystems.
// Must be in a separate function from __try because MSVC prohibits SEH
// in functions with C++ automatic destructors.
static void PreRegisterCallbacksInner(libretro_raw* raw)
{
	raw->set_video_refresh(C_LibretroInstance::cbVideoRefresh);
	raw->set_audio_sample(C_LibretroInstance::cbAudioSample);
	raw->set_audio_sample_batch(C_LibretroInstance::cbAudioSampleBatch);
	raw->set_input_poll(C_LibretroInstance::cbInputPoll);
	if (raw->set_input_state)
		raw->set_input_state(C_LibretroInstance::cbInputState);
}

// SEH wrapper for pre-init callback registration.
// Some cores (e.g., mesen) crash if set_* is called before init() has
// created internal objects. The SEH wrapper makes this safe to attempt.
static bool SafePreRegisterCallbacks(libretro_raw* raw)
{
	__try {
		PreRegisterCallbacksInner(raw);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		WorkerDbgMsg("libretro: Pre-init callback registration failed (caught by SEH), will register after init\n");
		return false;
	}
}

// SEH wrapper for Sys_LoadModule -- catches DLLs with crashing DllMain.
static CSysModule* SafeLoadModule(const char* path)
{
	__try {
		return Sys_LoadModule(path);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		WorkerDbgMsg("libretro: SEH caught exception 0x%08X during DLL load of %s\n", GetExceptionCode(), path);
		return NULL;
	}
}

// SEH wrapper for OpenGL resource cleanup after a core crash.
// Must be a separate function because MyThread has CUtlBuffer objects with
// destructors, and MSVC prohibits __try in functions with C++ automatic destructors.
static bool SafeCleanupGL(LibretroGLContext* gl_ctx)
{
	__try {
		if (gl_ctx->framebuffer)
		{
			wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &gl_ctx->framebuffer);
			WorkerDbgMsg("libretro: Deleted framebuffer.\n");
		}

		if (gl_ctx->color_texture)
		{
			glDeleteTextures(1, &gl_ctx->color_texture);
			WorkerDbgMsg("libretro: Deleted color texture.\n");
		}

		if (gl_ctx->depth_stencil_renderbuffer)
		{
			glDeleteRenderbuffers(1, &gl_ctx->depth_stencil_renderbuffer);
			WorkerDbgMsg("libretro: Deleted depth/stencil renderbuffer.\n");
		}

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(gl_ctx->hglrc);
		ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
		DestroyWindow(gl_ctx->hwnd);
		WorkerDbgMsg("libretro: OpenGL cleanup complete.\n");
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		WorkerDbgMsg("libretro: WARNING - Exception 0x%08X during GL cleanup\n", GetExceptionCode());
		return false;
	}
}

// Fallback: try to at least release the GL context and destroy the window
static void SafeCleanupGLFallback(LibretroGLContext* gl_ctx)
{
	__try {
		wglMakeCurrent(NULL, NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}

	__try {
		if (gl_ctx->hwnd)
		{
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void ResizeFBO(LibretroGLContext* gl_ctx, unsigned int newWidth, unsigned int newHeight, bool depth, bool stencil)
{
	if (newWidth == 0 || newHeight == 0)
		return;
	if (newWidth == gl_ctx->hw_render_width && newHeight == gl_ctx->hw_render_height)
		return;

	WorkerDbgMsg("libretro: Resizing FBO from %ux%u to %ux%u\n",
		gl_ctx->hw_render_width, gl_ctx->hw_render_height, newWidth, newHeight);

	// Resize color texture
	glBindTexture(GL_TEXTURE_2D, gl_ctx->color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Resize depth/stencil renderbuffer if present
	if (gl_ctx->depth_stencil_renderbuffer)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, gl_ctx->depth_stencil_renderbuffer);
		if (depth && stencil)
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, newWidth, newHeight);
		else if (depth)
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, newWidth, newHeight);
		else
			glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, newWidth, newHeight);
	}

	// Verify FBO completeness
	glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		WorkerDbgMsg("libretro: WARNING - FBO incomplete after resize! Status: 0x%X\n", status);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gl_ctx->hw_render_width = newWidth;
	gl_ctx->hw_render_height = newHeight;
}

inline const char* WebStringToCharString3(WebString web_string)
{
	int len = web_string.ToUTF8(null, 0);
	char* buf = new char[len + 1];
	web_string.ToUTF8(buf, len);
	buf[len] = 0;	// null terminator

	std::string title = buf;
	delete[] buf;

	return VarArgs("%s", title.c_str());
}

C_LibretroInstance::C_LibretroInstance()
{
	DevMsg("LibretroInstance: Constructor\n");
	m_bTakeScreenshot = false;
	m_bIsDirty = false;
	m_iAdjustedStartTime = -1;
	m_iLastDelta = 0;
	m_pProjectorFixConVar = null;
	m_pTexture = null;
	m_iLastRenderedFrame = -1;
	m_iLastVisibleFrame = -1;
	m_iOriginalEntIndex = -1;
	m_bGotTime = false;
	m_iFastForwardSeconds = 0;
	m_fLastMouseX = 0.5;
	m_fLastMouseY = 0.5;
	m_pLocalVideoBehaviorConVar = cvar->FindVar("local_video_behavior");
	m_bShouldReopen = false;
	m_info = null;
	m_pOverlayKV = new KeyValues("overlay");
}

C_LibretroInstance::~C_LibretroInstance()
{
	DevMsg("LibretroInstance: Destructor\n");
	this->CleanUpTexture();

	// Delete the info struct (was previously deleted by worker thread, now owned by main thread)
	// Worker thread cleans up core resources, but leaves info struct for snapshot saving
	if (m_info)
	{
		delete m_info;
		m_info = null;
	}
}

void C_LibretroInstance::ClearOverlay(std::string type, std::string overlayId)
{
	std::string folder = "resource\\ui\\html\\overlays";
	g_pFullFileSystem->CreateDirHierarchy(folder.c_str(), "DEFAULT_WRITE_PATH");
	std::string file = VarArgs("%s\\%s.cfg", folder.c_str(), overlayId.c_str());

	std::string prettyCore = m_info->prettycore;
	std::string prettyGame = m_info->prettygame;
	KeyValues* pDefaultKV = m_pOverlayKV->FindKey("settings/default", true);
	KeyValues* pTargetKV = null;

	std::string testerCore;
	std::string testerGame;
	for (KeyValues *sub = m_pOverlayKV->FindKey("settings", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (!Q_stricmp(sub->GetName(), "default"))
			continue;

		testerCore = sub->GetString("core");
		testerGame = sub->GetString("game");

		if (type == "core" && testerCore == prettyCore && testerGame == "")
			pTargetKV = sub;
		else if (testerCore == prettyCore && testerGame == prettyGame && type == "game")
		{
			pTargetKV = sub;
			break;
		}
	}

	if (pTargetKV)
	{
		pTargetKV->Clear();
		pTargetKV->SetString(null, "");

		if (!m_pOverlayKV->SaveToFile(g_pFullFileSystem, file.c_str(), "DEFAULT_WRITE_PATH"))
			DevMsg("ERROR: Could not wite file %s\n", file.c_str());

		this->SetOverlay(overlayId);
	}
}

void C_LibretroInstance::SaveOverlay(std::string type, std::string overlayId, float x, float y, float width, float height)
{
	std::string folder = "resource\\ui\\html\\overlays";
	g_pFullFileSystem->CreateDirHierarchy(folder.c_str(), "DEFAULT_WRITE_PATH");
	std::string file = VarArgs("%s\\%s.cfg", folder.c_str(), overlayId.c_str());

	std::string prettyCore = m_info->prettycore;
	std::string prettyGame = m_info->prettygame;
	std::string preferredOverlayId = g_pAnarchyManager->GetLibretroManager()->DetermineOverlay(prettyCore, prettyGame);
	KeyValues* pDefaultKV = m_pOverlayKV->FindKey("settings/default", true);
	KeyValues* pCoreKV = null;
	KeyValues* pGameKV = null;
	std::string testerCore;
	std::string testerGame;
	for (KeyValues *sub = m_pOverlayKV->FindKey("settings", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (!Q_stricmp(sub->GetName(), "default"))
			continue;

		testerCore = sub->GetString("core");
		testerGame = sub->GetString("game");

		if (testerCore == prettyCore && testerGame == "")
			pCoreKV = sub;
		else if (testerCore == prettyCore && testerGame == prettyGame)
			pGameKV = sub;
	}

	if (type == "game")
	{
		if (!pGameKV)
		{
			pGameKV = m_pOverlayKV->FindKey("settings", true)->CreateNewKey();
			pGameKV->SetName("setting");
			pGameKV->SetString("core", prettyCore.c_str());
			pGameKV->SetString("game", prettyGame.c_str());
		}

		pGameKV->SetFloat("x", x);
		pGameKV->SetFloat("y", y);
		pGameKV->SetFloat("width", width);
		pGameKV->SetFloat("height", height);
	}
	else if (type == "core")
	{
		if (!pCoreKV)
		{
			pCoreKV = m_pOverlayKV->FindKey("settings", true)->CreateNewKey();
			pCoreKV->SetName("setting");
			pCoreKV->SetString("core", prettyCore.c_str());
		}

		pCoreKV->SetFloat("x", x);
		pCoreKV->SetFloat("y", y);
		pCoreKV->SetFloat("width", width);
		pCoreKV->SetFloat("height", height);
	}
	else if (type == "default")
	{
		pDefaultKV->SetFloat("x", x);
		pDefaultKV->SetFloat("y", y);
		pDefaultKV->SetFloat("width", width);
		pDefaultKV->SetFloat("height", height);
	}

	if (preferredOverlayId == overlayId)
	{
		m_pOverlayKV->SetFloat("current/x", x);
		m_pOverlayKV->SetFloat("current/y", y);
		m_pOverlayKV->SetFloat("current/width", width);
		m_pOverlayKV->SetFloat("current/height", height);
	}

	if (!m_pOverlayKV->SaveToFile(g_pFullFileSystem, file.c_str(), "DEFAULT_WRITE_PATH"))
		DevMsg("ERROR: Could not wite file %s\n", file.c_str());

	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
	{
		vgui::CInputSlate* pInputSlate = g_pAnarchyManager->GetInputManager()->GetInputSlate();
		if (pInputSlate)
			pInputSlate->AdjustOverlay(m_pOverlayKV->GetFloat("current/x", 0), m_pOverlayKV->GetFloat("current/y", 0), m_pOverlayKV->GetFloat("current/width", 1), m_pOverlayKV->GetFloat("current/height", 1), m_overlayId);
	}
}

void C_LibretroInstance::SetOverlay(std::string overlayId)
{
	if (!m_info)
		return;

	std::string goodOverlayId = overlayId;

	std::string prettyCore = m_info->prettycore;
	std::string prettyGame = m_info->prettygame;

	std::string preferredOverlayId = g_pAnarchyManager->GetLibretroManager()->DetermineOverlay(prettyCore, prettyGame);
	goodOverlayId = preferredOverlayId;

	if (m_pOverlayKV)
		m_pOverlayKV->Clear();

	if (goodOverlayId != "" && goodOverlayId != "none" && m_pOverlayKV->LoadFromFile(g_pFullFileSystem, VarArgs("resource\\ui\\html\\overlays\\%s.cfg", goodOverlayId.c_str()), "MOD"))
	{
		std::string testerCore;
		std::string testerGame;

		KeyValues* pDefaultOverlayKV = m_pOverlayKV->FindKey("settings/default");
		if (!pDefaultOverlayKV)
		{
			pDefaultOverlayKV = m_pOverlayKV->FindKey("settings/default", true);
			pDefaultOverlayKV->SetFloat("x", 0);
			pDefaultOverlayKV->SetFloat("y", 0);
			pDefaultOverlayKV->SetFloat("width", 1);
			pDefaultOverlayKV->SetFloat("height", 1);
		}

		KeyValues* pCoreOverlayKV = null;
		KeyValues* pGameOverlayKV = null;
		for (KeyValues *sub = m_pOverlayKV->FindKey("settings", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (sub == pDefaultOverlayKV)
				continue;

			testerCore = sub->GetString("core");
			testerGame = sub->GetString("game");

			if (testerCore == prettyCore && testerGame == "")
				pCoreOverlayKV = sub;
			else if (testerCore == prettyCore && testerGame == prettyGame)
				pGameOverlayKV = sub;
		}

		KeyValues* pBestOverlayKV = (pGameOverlayKV) ? pGameOverlayKV : pCoreOverlayKV;
		if (!pBestOverlayKV)
			pBestOverlayKV = pDefaultOverlayKV;

		if (preferredOverlayId == goodOverlayId)
		{
			m_pOverlayKV->SetFloat("current/x", pBestOverlayKV->GetFloat("x", 0));
			m_pOverlayKV->SetFloat("current/y", pBestOverlayKV->GetFloat("y", 0));
			m_pOverlayKV->SetFloat("current/width", pBestOverlayKV->GetFloat("width", 1));
			m_pOverlayKV->SetFloat("current/height", pBestOverlayKV->GetFloat("height", 1));
		}
	}
	else
	{
		m_pOverlayKV->SetFloat("current/x", 0);
		m_pOverlayKV->SetFloat("current/y", 0);
		m_pOverlayKV->SetFloat("current/width", 1);
		m_pOverlayKV->SetFloat("current/height", 1);
	}

	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
	{
		vgui::CInputSlate* pInputSlate = g_pAnarchyManager->GetInputManager()->GetInputSlate();
		if (pInputSlate)
			pInputSlate->AdjustOverlay(m_pOverlayKV->GetFloat("current/x", 0), m_pOverlayKV->GetFloat("current/y", 0), m_pOverlayKV->GetFloat("current/width", 1), m_pOverlayKV->GetFloat("current/height", 1), goodOverlayId);
	}

	m_overlayId = goodOverlayId;
}

void C_LibretroInstance::SelfDestruct()
{
	DevMsg("LibretroInstance: SelfDestruct\n");

	if (g_pAnarchyManager->ShouldAllowMultipleActive() && g_pAnarchyManager->IsLevelInitialized())
	{
		C_BaseEntity* pOriginalEntity = C_BaseEntity::Instance(m_iOriginalEntIndex);
		if (pOriginalEntity)
		{
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pOriginalEntity);
			pShortcut->PlaySequenceRegular("deactivated");
		}
	}

	if (g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() == this)
		g_pAnarchyManager->GetCanvasManager()->SetDifferentDisplayInstance(this);

	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->NotifyInstanceAboutToDie(this);
	g_pAnarchyManager->GetCanvasManager()->RenderUnseen(this);

	if (m_info)
	{
		m_info->libretroinstance = null;
		m_info->close = true;

		// Two-phase wait for worker thread to finish cleanup
		if (m_info->hThreadDoneEvent)
		{
			DWORD result = g_pVCR->Hook_WaitForSingleObject((HANDLE)m_info->hThreadDoneEvent, 5000);
			if (result == WAIT_TIMEOUT)
			{
				// Worker thread is stuck. Signal it to skip remaining core calls.
				DevMsg("libretro: WARNING - Worker thread did not exit within 5 seconds, forcing shutdown...\n");
				InterlockedExchange(&m_info->bForceShutdown, 1);

				// Give it 3 more seconds to finish non-core cleanup and signal
				result = g_pVCR->Hook_WaitForSingleObject((HANDLE)m_info->hThreadDoneEvent, 3000);
				if (result == WAIT_TIMEOUT)
				{
					// Thread is truly stuck (deadlocked in a core call).
					// Leak info and event handle to avoid use-after-free.
					DevMsg("libretro: WARNING - Worker thread still stuck after force shutdown! Leaking resources.\n");
					m_info = null;
					goto selfDestruct_finish;
				}
			}
			CloseHandle((HANDLE)m_info->hThreadDoneEvent);
			m_info->hThreadDoneEvent = NULL;
		}
	}

selfDestruct_finish:
	if (m_pOverlayKV)
		m_pOverlayKV->deleteThis();

	engine->ClientCmd("exec 360controller");
	delete this;
}

void C_LibretroInstance::CleanUpTexture()
{
	if (m_pTexture)
	{
		m_pTexture->SetTextureRegenerator(null);

		// save the last rendered image out as a TGA to use as a thumbnail
		if (m_info && m_info->lastframedata && !g_pAnarchyManager->GetCanvasManager()->GetItemTexture(m_originalItemId, "screen"))	// Note: This makes it so "always_refresh_snapshots" is useless.
		{
			std::string filePath = "cache/snapshots";
			std::string fileName = filePath + "/";
			fileName += g_pAnarchyManager->GenerateLegacyHash(m_originalGame.c_str());
			fileName += ".tga";

			if (cvar->FindVar("always_refresh_snapshots")->GetBool() || !g_pFullFileSystem->FileExists(fileName.c_str(), "DEFAULT_WRITE_PATH"))
			{
				unsigned int depth = 4;
				unsigned int width = AA_THUMBNAIL_SIZE;
				unsigned int height = AA_THUMBNAIL_SIZE;
				unsigned int bufferSize = width * height * depth;
				unsigned int pitch = width * depth;

				// Get the data from the render target and save to disk bitmap bits
				unsigned char *pImage = (unsigned char *)malloc(width * height * depth);

				if (AA_LIBRETRO_3D && m_info->context_type != RETRO_HW_CONTEXT_NONE)
					this->ResizeFrameFromXRGB8888(m_info->lastframedata, pImage, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 4, width, height, pitch, depth, m_info->bottom_left_origin);
				else if (m_info->videoformat == RETRO_PIXEL_FORMAT_RGB565)
					this->ResizeFrameFromRGB565(m_info->lastframedata, pImage, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);
				else if (m_info->videoformat == RETRO_PIXEL_FORMAT_XRGB8888)
					this->ResizeFrameFromXRGB8888(m_info->lastframedata, pImage, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 4, width, height, pitch, depth);
				else
					this->ResizeFrameFromRGB1555(m_info->lastframedata, pImage, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);

				// allocate a buffer to write the tga into
				int iMaxTGASize = (width * height * depth) + 1024;
				void *pTGA = malloc(iMaxTGASize);
				CUtlBuffer buffer(pTGA, iMaxTGASize);

				// pImage is always BGRA8888 after ResizeFrame* conversion, regardless of core's native format
				if (!TGAWriter::WriteToBuffer(pImage, buffer, width, height, IMAGE_FORMAT_BGRA8888, IMAGE_FORMAT_RGBA8888))
					DevMsg("Couldn't write bitmap data.\n");

				free(pImage);

				// save the TGA out
				g_pFullFileSystem->CreateDirHierarchy(filePath.c_str(), "DEFAULT_WRITE_PATH");

				FileHandle_t fileTGA = filesystem->OpenEx(fileName.c_str(), "wb", 0, "DEFAULT_WRITE_PATH");
				filesystem->Write(buffer.Base(), buffer.TellPut(), fileTGA);
				filesystem->Close(fileTGA);
				free(pTGA);

				std::string mode = "ALL";
				g_pAnarchyManager->GetCanvasManager()->PrepareRefreshItemTextures(m_originalItemId, mode);
				g_pAnarchyManager->GetCanvasManager()->RefreshItemTextures(m_originalItemId, mode);
			}
		}
		// now continue with regular stuff

		DevMsg("Unref texture from: C_LibretroInstance::CleanUpTexture\n");
		g_pAnarchyManager->GetCanvasManager()->UnreferenceEmbeddedInstance(this);
		g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(m_pTexture);
		g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(m_pTexture);
		m_pTexture = null;
	}

	// Free the frame buffer now that snapshot is saved (or skipped)
	// This was previously freed by worker thread, but we need it for snapshot saving
	if (m_info && m_info->lastframedata)
	{
		free(m_info->lastframedata);
		m_info->lastframedata = null;
	}
}

void C_LibretroInstance::GoPrevious()
{
	DevMsg("GoPrevious from C++\n");
	this->GoSomewhere(-1);
}

void C_LibretroInstance::GoSomewhere(int iDirection)
{
	LibretroInstanceInfo_t* info = this->GetInfo();
	std::string fileName = "";
	std::string fileFull = info->game;// "Beast Machines 1x05 - Forbidden Fruit.avi";
	std::string dir = "";// "V:\\TV\\Beast Wars";	// TODO: get the real dir for this LibretroInstance's item.

	std::string fileExtension = fileFull;
	std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
	size_t extensionFound = fileExtension.find_last_of(".");
	if (extensionFound != std::string::npos)
		fileExtension = fileExtension.substr(extensionFound + 1);
	else
	{
		DevMsg("libretro: ABORTED: The next file has no file extension.\n");
		return;
	}

	if (fileFull.find(':') == 1)
	{
		std::string testPath = fileFull;
		std::transform(testPath.begin(), testPath.end(), testPath.begin(), ::tolower);
		std::replace(testPath.begin(), testPath.end(), '\\', '/');

		size_t foundTestPathSlash = testPath.find_last_of("/");
		if (foundTestPathSlash != std::string::npos)
		{
			dir = fileFull.substr(0, foundTestPathSlash);
			fileName = fileFull.substr(foundTestPathSlash + 1);
		}
	}

	if (dir == "")
	{
		DevMsg("libretro: ABORTED: Failed to parse file path.\n");
		return;
	}

	std::vector<std::string> files;

	unsigned int uFoundIndex = -1;
	FileFindHandle_t findHandle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%s\\*.%s", dir.c_str(), fileExtension.c_str()), "", &findHandle);
	while (pFilename != NULL)
	{
		if (!Q_strcmp(pFilename, ".") || !Q_strcmp(pFilename, ".."))
		{
			pFilename = g_pFullFileSystem->FindNext(findHandle);
			continue;
		}

		if (!g_pFullFileSystem->FindIsDirectory(findHandle))
		{
			if (fileName == std::string(pFilename))
				uFoundIndex = files.size();

			files.push_back(pFilename);
		}
		pFilename = g_pFullFileSystem->FindNext(findHandle);
	}
	g_pFullFileSystem->FindClose(findHandle);

	if (files.size() <= 1)
	{
		// do something?
	}
	else
	{
		unsigned int uNextFileIndex = uFoundIndex + iDirection;
		if (uNextFileIndex >= files.size())
			uNextFileIndex = 0;
		else if (uNextFileIndex < 0)
			uNextFileIndex = files.size() - 1;

		std::string nextFileName = files[uNextFileIndex];
		std::string nextFileFull = dir + "\\" + nextFileName;

		DevMsg("Next File: %s\n", nextFileFull.c_str());

		// mp3-style flow
		this->SetShouldReopen(true);	// NOTE: The LibretroManager needs to know the file we want to override to somehow. (Because this libretro instance is about to be destroyed.)
		g_pAnarchyManager->GetLibretroManager()->SetNextLoadOverrideForInstance(this, nextFileFull);
		g_pAnarchyManager->GetLibretroManager()->DestroyLibretroInstance(this);
	}
}

void C_LibretroInstance::GoNext()
{
	DevMsg("GoNext from C++\n");
	this->GoSomewhere(1);
}

void C_LibretroInstance::OnMouseMove(float x, float y)
{
	m_fLastMouseX = x;
	m_fLastMouseY = y;
}

void C_LibretroInstance::Init(std::string id, std::string title, int iEntIndex)
{
	this->SetAdjustedStartTime();

	std::string goodTitle = (title != "") ? title : "Untitled Libretro Tab";
	m_title = goodTitle;
	m_id = id;
	if (m_id == "")
		m_id = g_pAnarchyManager->GenerateUniqueId();

	m_iOriginalEntIndex = iEntIndex;

	m_pProjectorFixConVar = cvar->FindVar("projector_fix");

	// create the texture (each instance has its own texture)
	std::string textureName = "canvas_";
	textureName += m_id;

	int iWidth = (id == "hud") ? AA_HUD_INSTANCE_WIDTH : AA_EMBEDDED_INSTANCE_WIDTH;
	int iHeight = (id == "hud") ? AA_HUD_INSTANCE_HEIGHT : AA_EMBEDDED_INSTANCE_HEIGHT;

	int flags = (0x0100 | 0x0200 | 0x0800 | 0x2000000);

	if (g_pAnarchyManager->ShouldTextureClamp())
		flags |= (0x0004 | 0x0008);

	int multiplyer = 1.0;// g_pAnarchyManager->GetDynamicMultiplyer();
	if (!g_pMaterialSystem->IsTextureLoaded(textureName.c_str()))
		m_pTexture = g_pMaterialSystem->CreateProceduralTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, iWidth * multiplyer, iHeight * multiplyer, IMAGE_FORMAT_BGR888, flags);
	else
	{
		m_pTexture = g_pMaterialSystem->FindTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, false, 1);
		g_pAnarchyManager->GetCanvasManager()->TextureNotDeferred(m_pTexture);
	}

	// get the regen and assign it
	CCanvasRegen* pRegen = g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen();
	m_pTexture->SetTextureRegenerator(pRegen);

	m_raw = new libretro_raw();

	engine->ClientCmd("exec strip_controller");

	if (m_iOriginalEntIndex >= 0 && g_pAnarchyManager->ShouldAllowMultipleActive() && g_pAnarchyManager->IsLevelInitialized())
	{
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(m_iOriginalEntIndex);
		if (pBaseEntity)
		{
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			if (pShortcut)
				pShortcut->PlaySequenceRegular("activated");
		}
	}
}

void C_LibretroInstance::Update()
{
	if (!m_info)
		return;

	if (g_pAnarchyManager->GetSuspendEmbedded() && m_id != "init")
		return;

	if (m_info->state == 1)
	{
		OnCoreLoaded();
	}
	else if (m_info->state == 5)// && m_info->audiostream)	// added m_info to try and detect failed video loads!! (FIXME: Should be removed after proper failed video load is added elsewhere.
	{
		unsigned int numPorts = m_info->numports;
		if (numPorts == 0)
		{
			// how the funnuck are we supposd to know how many input ports need to be held in the input back buffer if the core doesn't tell us??  just assume 1 for now.
			//DevMsg("WARNING: zero retro ports are active.\n");
			numPorts = 1;
		}

		for (unsigned int i = 0; i < numPorts; i++)
		{
			g_pAnarchyManager->GetLibretroManager()->ManageInputUpdate(m_info, i, RETRO_DEVICE_JOYPAD);
			g_pAnarchyManager->GetLibretroManager()->ManageInputUpdate(m_info, i, RETRO_DEVICE_ANALOG);
		}

		if (!m_bGotTime)
		{
			m_bGotTime = true;

			// only if local videos should resume are enabled
			if (m_pLocalVideoBehaviorConVar->GetInt() == 1 && m_info->core.find("ffmpeg") != std::string::npos && m_id != "init")
			{
				C_AwesomiumBrowserInstance* pNetwork = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");// static_cast<C_AwesomiumBrowserInstance*>(m_pHudVoid);
				if (pNetwork)
				{
					JSValue result = pNetwork->GetWebView()->ExecuteJavascriptWithResult(WSLit(VarArgs("localStorage.getItem(\"libtime%s\");", m_originalGameHash.c_str())), WSLit(""));
					if (!result.IsNull() && result.IsString())
					{
						int secs = Q_atoi(WebStringToCharString3(result.ToString()));
						if (secs > 0)
						{
							DevMsg("AArcade skipping to %s seconds...\n", WebStringToCharString3(result.ToString()));
							this->SetFastForwardSeconds(Q_atoi(WebStringToCharString3(result.ToString())));
						}
					}
				}
			}
		}

		this->OnProxyBind(null);

	}
	else if (m_info->state == 6)
	{
		// even though the core wants to close, don't do anything at all.  the core will keep waiting until the user closes this C_LibretroInstance like normal.
		if (m_id == "init")
			g_pAnarchyManager->GetLibretroManager()->DestroyLibretroInstance(this);
	}
}

void C_LibretroInstance::TakeScreenshot(std::string nextTaskScreenshotName)
{
	g_pAnarchyManager->SetNextTaskScreenshot(nextTaskScreenshotName);
	m_bTakeScreenshot = true;
}

void C_LibretroInstance::TakeScreenshotNow(ITexture* pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch, unsigned int depth)
{
	// This declares a lambda, which can be called just like a function
	std::string badAlphabet = "<>:\"/\\|?*";
	auto scrubBadAlphabet = [&](std::string str_in)
	{
		std::string str = str_in;
		unsigned int len = str.length();
		for (unsigned int i = 0; i < len; i++) {
			if (badAlphabet.find(str[i]) != std::string::npos) {
				str[i] = '_';
			}
		}
		return str;
	};

	// instead of using that name, let's make one based on a timestamp.
	auto now = std::chrono::system_clock::now();
	auto UTC = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream datetime;
	datetime << std::put_time(std::localtime(&in_time_t), "%Y.%m.%d - %X");
	std::string dateString = datetime.str();
	// change : to . in the hours:minutes:seconds of the %X timestamp.
	unsigned int len = dateString.length();
	for (unsigned int i = 0; i < len; i++) {
		if (dateString[i] == ':') {
			dateString[i] = '.';
		}
	}

	// get the item's title to use in the screenshot name.
	std::string itemTitle = ((C_EmbeddedInstance*)this)->GetTitle();
	unsigned int maxTitleLength = 60;
	if (itemTitle.length() > maxTitleLength) {
		itemTitle = itemTitle.substr(0, maxTitleLength);
	}

	// scrub the item title to be windows path friendly.
	std::string scrubbedItemTitle = scrubBadAlphabet(itemTitle);
	std::string screenshotFolder = "taskshots/" + scrubbedItemTitle;

	// if there's a "nextTaskScreenshot" that isn't empty, then use it
	std::string nextTaskScreenshot = g_pAnarchyManager->GetNextTaskScreenshot();
	if (nextTaskScreenshot != "") {
		std::string nextTaskScreenshotFolder = nextTaskScreenshot;
		std::string nextFolderPath = nextTaskScreenshotFolder.substr(0, nextTaskScreenshotFolder.find_last_of("/\\"));
		screenshotFolder = "screenshots/" + nextFolderPath;
	}

	g_pFullFileSystem->CreateDirHierarchy(screenshotFolder.c_str(), "DEFAULT_WRITE_PATH");
	std::string goodFile = screenshotFolder + "/" + scrubbedItemTitle + " " + dateString + ".tga";

	// if there's a "nextTaskScreenshot" that isn't empty, overwrite good file too
	if (nextTaskScreenshot != "") {
		std::string nextTaskScreenshotFile = nextTaskScreenshot.substr(nextTaskScreenshot.find_last_of("/\\") + 1);
		goodFile = screenshotFolder + "/" + nextTaskScreenshotFile + ".tga";
	}

	unsigned int bufferSize = width * height * depth;
	// allocate a buffer to write the tga into
	int iMaxTGASize = (width * height * depth);
	void *pTGA = malloc(iMaxTGASize);
	CUtlBuffer buffer(pTGA, iMaxTGASize);

	if (!TGAWriter::WriteToBuffer(dest, buffer, width, height, IMAGE_FORMAT_BGRA8888, IMAGE_FORMAT_RGBA8888))
	{
		DevMsg("ERROR: Could not write to TGA buffer.\n");
		g_pAnarchyManager->AddToastMessage("Failed to capture task screenshot.", true);
		return;
	}

	// save the TGA out
	FileHandle_t fileTGA = filesystem->OpenEx(goodFile.c_str(), "wb", 0, "DEFAULT_WRITE_PATH");
	filesystem->Write(buffer.Base(), buffer.TellPut(), fileTGA);
	filesystem->Close(fileTGA);
	free(pTGA);

	DevMsg("Saved screenshot %s\n", goodFile.c_str());
	g_pAnarchyManager->AddToastMessage(VarArgs("Saved screenshot %s", goodFile.c_str()), true);
}

bool C_LibretroInstance::LoadCore(std::string coreFile)
{
	if (coreFile != "")
	{
		// first check for cores in the user folder, then check for cores in the frontend folder.

		bool bReady = false;
		std::string core = g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_USER_BASE) + g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_CORE_PATH) + std::string("\\") + coreFile;
		if (g_pFullFileSystem->FileExists(core.c_str()))
			bReady = true;
		else
		{
			core = engine->GetGameDirectory() + g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_CORE_PATH) + std::string("\\") + coreFile;
			if (g_pFullFileSystem->FileExists(core.c_str()))
				bReady = true;
		}

		if (bReady)
		{
			CreateWorkerThread(core);
			return true;
		}
	}

	g_pAnarchyManager->AddToastMessage("Libretro Core Aborted");
	return false;
}

void C_LibretroInstance::OnGameLoaded()
{
	DevMsg("Game finished loading.\n");
}


std::string C_LibretroInstance::GetLibretroCore()
{
	if (m_info)
		return m_info->core;
	else
		return "";
}

std::string C_LibretroInstance::GetLibretroFile()
{
	if (m_info)
		return m_info->game;
	else
		return "";
}

void C_LibretroInstance::SetReset(bool bValue)
{
	// reset the seconds stats too
	m_iLastDelta = 0;
	m_iFastForwardSeconds = 0;
	m_iAdjustedStartTime = static_cast<int>(ceil(engine->Time()));
	C_AwesomiumBrowserInstance* pNetwork = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
	if (pNetwork)
		pNetwork->GetWebView()->ExecuteJavascript(WSLit(VarArgs("localStorage.setItem(\"libtime%s\", %i)", m_originalGameHash.c_str(), 0)), WSLit(""));

	if (m_info)
		m_info->reset = bValue;
}

void C_LibretroInstance::SetPause(bool bValue)
{
	if (m_info)
		m_info->paused = bValue;
}

void C_LibretroInstance::SetVolume(float fVolume)
{
	if (m_info)
		m_info->volume = fVolume;
}

bool C_LibretroInstance::GetPause()
{
	if (!m_info)
		return false;
	else
		return m_info->paused;
}

// Helper function: Parse path components (directory, name, extension)
static void ParsePathComponents(const std::string& fullPath, std::string& outDir, std::string& outName, std::string& outExt)
{
	// Find the last directory separator
	size_t dirSepPos = fullPath.find_last_of("/\\");
	if (dirSepPos != std::string::npos)
	{
		outDir = fullPath.substr(0, dirSepPos);
	}
	else
	{
		outDir = "";
	}

	// Get the filename (everything after the last separator)
	std::string filename;
	if (dirSepPos != std::string::npos)
		filename = fullPath.substr(dirSepPos + 1);
	else
		filename = fullPath;

	// Find the extension
	size_t extPos = filename.find_last_of(".");
	if (extPos != std::string::npos)
	{
		outName = filename.substr(0, extPos);
		outExt = filename.substr(extPos + 1);
		// Convert extension to lowercase
		std::transform(outExt.begin(), outExt.end(), outExt.begin(), ::tolower);
	}
	else
	{
		outName = filename;
		outExt = "";
	}
}

// Helper function: Apply content override for a given extension
static bool ApplyContentOverride(LibretroInstanceInfo_t* info, const std::string& extension, bool& outNeedFullpath, bool& outPersistentData)
{
	if (!info->has_content_overrides)
		return false;

	// Search through content overrides for matching extension
	for (const auto& override : info->content_overrides)
	{
		// Parse the pipe-delimited extension list
		std::string extensionsStr = override.extensions;
		std::transform(extensionsStr.begin(), extensionsStr.end(), extensionsStr.begin(), ::tolower);

		std::vector<std::string> extensionTokens;
		g_pAnarchyManager->Tokenize(extensionsStr, extensionTokens, "|");

		// Check if our extension matches
		if (std::find(extensionTokens.begin(), extensionTokens.end(), extension) != extensionTokens.end())
		{
			outNeedFullpath = override.need_fullpath;
			outPersistentData = override.persistent_data;
			return true;
		}
	}

	return false;
}

bool C_LibretroInstance::LoadGame()
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);

	if (!pLibretroInstance)
		return false;

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();

	std::string filename = info->game;

	// Parse path components for extended game info
	ParsePathComponents(filename, info->loaded_dir, info->loaded_name, info->loaded_ext);
	info->loaded_full_path = filename;
	info->loaded_archive_path = "";
	info->loaded_archive_file = "";
	info->loaded_file_in_archive = false;
	info->loaded_persistent_data = false;
	info->loaded_data = NULL;
	info->loaded_data_size = 0;

	// If this core *requires* a full file path, then we can check if the file extension is supported RIGHT NOW.
	// OTHERWISE, we might have to open up a ZIP file before we can check the real file extension.

	std::string fileExtension = filename;
	std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
	size_t extensionFound = fileExtension.find_last_of(".");
	if (extensionFound != std::string::npos)
		fileExtension = fileExtension.substr(extensionFound + 1);
	else
		fileExtension = "";

	if (fileExtension == "")
	{
		WorkerDbgMsg("libretro: ABORTED: The file has no file extension.\n");
		//info->close = true;
		return false;
	}

	// Format Example:
	//   valid_extensions: mkv|avi|f4v|f4f|3gp|ogm|flv|mp4|mp3|flac|ogg|m4a
	std::string testerExtensions = info->valid_extensions;
	std::transform(testerExtensions.begin(), testerExtensions.end(), testerExtensions.begin(), ::tolower);

	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(testerExtensions, tokens, "|");

	bool bIsZip = (fileExtension == "zip");
	bool bIs7z = (fileExtension == "7z");
	bool bIsValidExtension = true;

	// If this is NOT a ZIP/7z (or if ZIP/7z is a supported file extension for the core, OR(?) if the core requires fullpath) we can confirm validity RIGHT NOW.
	bool bIsArchive = bIsZip || bIs7z;
	bool bArchiveIsSupported = (bIsZip && std::find(tokens.begin(), tokens.end(), "zip") != tokens.end()) ||
		(bIs7z && std::find(tokens.begin(), tokens.end(), "7z") != tokens.end());

	// Only validate the archive extension if:
	// - Not an archive (regular file), OR
	// - Archive format is explicitly supported by the core (core wants the .zip/.7z directly), OR
	// - Core requires fullpath AND block_extract is true (core handles archives internally)
	if (!bIsArchive || bArchiveIsSupported || (info->need_fullpath && info->block_extract))
	{
		if (std::find(tokens.begin(), tokens.end(), fileExtension) != tokens.end())
			WorkerDbgMsg("Found extension %s within %s\n", fileExtension.c_str(), testerExtensions.c_str());
		else
			bIsValidExtension = false;
	}
	// Otherwise, if it's an archive that we'll extract, don't validate the archive extension yet
	// We'll validate the extracted file's extension later

	// Default pixel format is 0RGB1555 per libretro spec.
	// init() and load_game() may override this via SET_PIXEL_FORMAT.
	info->videoformat = RETRO_PIXEL_FORMAT_0RGB1555;

	//s_bSupportsNoGame

	void* fileData;
	bool bDataLoaded = false;
	bool bReadyToLoad = false;

	struct retro_game_info game;
	game.path = filename.c_str();
	std::string adjustedGamePath;	// Holds corrected path after archive extraction (must outlive load_game call)

	if (!bIsValidExtension)
	{
		WorkerDbgMsg("libretro: ABORTED: Invalid file extension %s for this core. Valid extensions for %s are: %s\n", fileExtension.c_str(), info->prettycore.c_str(), testerExtensions.c_str());
		//info->close = true;
	}
	else
	{
		// Determine whether to extract archives or pass them directly to the core
		std::string contentExtension = fileExtension;
		bool needFullpath;
		bool persistentData = false;

		if (bIsArchive)
		{
			// Check if core explicitly supports this archive format or handles archives internally
			if (bArchiveIsSupported || info->block_extract)
			{
				// Core wants the archive file directly (e.g., MAME with ZIP files)
				// Apply content override to the archive extension itself
				needFullpath = info->need_fullpath;
				if (ApplyContentOverride(info, contentExtension, needFullpath, persistentData))
				{
					WorkerDbgMsg("libretro: Using content override for archive extension '%s': need_fullpath=%d, persistent_data=%d\n",
						contentExtension.c_str(), needFullpath, persistentData);
					info->loaded_persistent_data = persistentData;
				}
				WorkerDbgMsg("libretro: Archive format supported by core, passing archive directly\n");
			}
			else
			{
				// Archive format not supported by core - extract it
				// Content override will be applied after extraction based on extracted file's extension
				needFullpath = false;
				WorkerDbgMsg("libretro: Archive not supported by core, will extract to determine content type\n");
			}
		}
		else
		{
			// For non-archives, apply content override to the file extension
			needFullpath = info->need_fullpath;
			if (ApplyContentOverride(info, contentExtension, needFullpath, persistentData))
			{
				WorkerDbgMsg("libretro: Using content override for extension '%s': need_fullpath=%d, persistent_data=%d\n",
					contentExtension.c_str(), needFullpath, persistentData);
				info->loaded_persistent_data = persistentData;
			}
		}

		if (needFullpath)
		{
			game.data = NULL;
			game.size = 0;
			game.meta = NULL;

			bReadyToLoad = true;
		}
		else
		{
			WorkerDbgMsg("libretro: File must be loaded by frontend!\n");

			// for easy char string access
			//char pFilename[AA_MAX_STRING];
			int iAAMaxString = filename.length() + 1;
			char* pFilename = new char[iAAMaxString];
			Q_strncpy(pFilename, filename.c_str(), iAAMaxString);

			if (bIsZip)
			{
				WorkerDbgMsg("libretro: ZIP file detected. Attempting to extract the 1st file..\n");

				bool bFailedUnzip = false;
				if (!g_pFullFileSystem->FileExists(filename.c_str()))
				{
					WorkerDbgMsg("libretro: ABORTED: ZIP file does not exist %s\n", pFilename);
					bFailedUnzip = true;
				}
				else
				{
					HZIP hz = OpenZip(pFilename, 0, ZIP_FILENAME);
					if (!hz)
					{
						WorkerDbgMsg("libretro: ABORTED: Failed to open ZIP file %s\n", pFilename);
						bFailedUnzip = true;
					}
					else
					{
						int zipIndex = 0;
						ZIPENTRY zipEntry;
						ZRESULT result = GetZipItem(hz, zipIndex, &zipEntry);

						std::string entryTesterExtension;
						size_t entryExtensionFound;
						bool bFoundFile = false;
						while (result == ZR_OK)
						{
							if (zipEntry.attr & FILE_ATTRIBUTE_DIRECTORY)
							{
								zipIndex++;
								result = GetZipItem(hz, zipIndex, &zipEntry);
								continue;
							}

							if (testerExtensions == "")
								bFoundFile = true;
							else
							{
								entryTesterExtension = zipEntry.name;
								std::transform(entryTesterExtension.begin(), entryTesterExtension.end(), entryTesterExtension.begin(), ::tolower);
								entryExtensionFound = entryTesterExtension.find_last_of(".");
								if (entryExtensionFound != std::string::npos)
									entryTesterExtension = entryTesterExtension.substr(entryExtensionFound + 1);
								else
									entryTesterExtension = "";

								if (entryTesterExtension != "" && std::find(tokens.begin(), tokens.end(), entryTesterExtension) != tokens.end())
									bFoundFile = true;
							}

							if (bFoundFile)
								break;
						}

						if (!bFoundFile || result != ZR_OK)
						{
							WorkerDbgMsg("libretro: ABORTED: Failed to locate a valid file in ZIP.");
							bFailedUnzip = true;
						}
						else
						{
							long fileSize = zipEntry.unc_size;
							fileData = malloc(fileSize);
							bDataLoaded = true;

							result = UnzipItem(hz, zipIndex, fileData, fileSize, ZIP_MEMORY);

							if (result != ZR_OK && result != ZR_MORE)
							{
								WorkerDbgMsg("libretro: ABORTED: Failed to unzip the file. ERROR CODE %i\n", result);
								bFailedUnzip = true;
							}
							else
							{
								game.data = fileData;
								game.size = fileSize;
								game.meta = NULL;

								// Track archive metadata for RETRO_ENVIRONMENT_GET_GAME_INFO_EXT
								info->loaded_archive_path = filename;
								info->loaded_archive_file = zipEntry.name;
								info->loaded_file_in_archive = true;

								// Parse the extracted file's path components
								std::string extractedFilename = zipEntry.name;
								ParsePathComponents(extractedFilename, info->loaded_dir, info->loaded_name, info->loaded_ext);
								// The directory is still the ZIP's directory
								info->loaded_dir = info->loaded_dir.empty() ?
									filename.substr(0, filename.find_last_of("/\\")) :
									filename.substr(0, filename.find_last_of("/\\"));

								// Now apply content override using the EXTRACTED file's extension
								bool extractedNeedFullpath = info->need_fullpath;
								bool extractedPersistentData = false;
								if (!info->loaded_ext.empty() &&
									ApplyContentOverride(info, info->loaded_ext, extractedNeedFullpath, extractedPersistentData))
								{
									WorkerDbgMsg("libretro: Using content override for extracted file extension '%s': need_fullpath=%d, persistent_data=%d\n",
										info->loaded_ext.c_str(), extractedNeedFullpath, extractedPersistentData);
									info->loaded_persistent_data = extractedPersistentData;

									// If the override requires fullpath, we need to abort and reload with fullpath
									if (extractedNeedFullpath && !needFullpath)
									{
										WorkerDbgMsg("libretro: WARNING: Extracted file requires fullpath but archive was loaded in memory. This may cause issues.\n");
									}
								}

								// Only adjust game.path for cores that DON'T need fullpath.
								// Cores with need_fullpath=1 (like bsnes) may open game.path from disk,
								// so it must point to a real file. Cores with need_fullpath=0 (like mesen)
								// use game.data but check game.path extension for file type identification.
								if (!info->need_fullpath && !info->loaded_ext.empty())
								{
									adjustedGamePath = info->loaded_dir + "\\" + info->loaded_name + "." + info->loaded_ext;
									game.path = adjustedGamePath.c_str();
									WorkerDbgMsg("libretro: Adjusted game.path for extracted file: %s\n", game.path);
								}

								bReadyToLoad = true;
							}
						}

						CloseZip(hz);
					}
				}

				// if bFailedUnzip is false, we have failed to unzip.
			}
			else if (bIs7z)
			{
				WorkerDbgMsg("libretro: 7z file detected. Attempting to extract the 1st file..\n");

				bool bFailed7z = false;
				if (!g_pFullFileSystem->FileExists(filename.c_str()))
				{
					WorkerDbgMsg("libretro: ABORTED: 7z file does not exist %s\n", pFilename);
					bFailed7z = true;
				}
				else
				{
					CFileInStream archiveStream;
					CLookToRead2 lookStream;
					CSzArEx db;
					ISzAlloc allocImp = { SzAlloc, SzFree };
					ISzAlloc allocTempImp = { SzAllocTemp, SzFreeTemp };

					if (InFile_Open(&archiveStream.file, pFilename) != 0)
					{
						WorkerDbgMsg("libretro: ABORTED: Failed to open 7z file %s\n", pFilename);
						bFailed7z = true;
					}
					else
					{
						FileInStream_CreateVTable(&archiveStream);
						LookToRead2_CreateVTable(&lookStream, False);
						lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, kInputBufSize);
						lookStream.bufSize = kInputBufSize;
						lookStream.realStream = &archiveStream.vt;
						LookToRead2_INIT(&lookStream);

						SzArEx_Init(&db);
						SRes res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);

						if (res != SZ_OK)
						{
							WorkerDbgMsg("libretro: ABORTED: Failed to parse 7z file %s. Error: %d\n", pFilename, res);
							bFailed7z = true;
						}
						else
						{
							// Find a valid file in the archive
							UInt32 foundIndex = (UInt32)-1;
							std::string foundEntryName = "";  // Store the found file name for metadata tracking
							for (UInt32 i = 0; i < db.NumFiles; i++)
							{
								if (SzArEx_IsDir(&db, i))
									continue;

								// Get filename
								size_t nameLen = SzArEx_GetFileNameUtf16(&db, i, NULL);
								std::vector<UInt16> nameBuf(nameLen);
								SzArEx_GetFileNameUtf16(&db, i, nameBuf.data());

								// Convert UTF-16 to std::string
								std::string entryName;
								for (size_t j = 0; j < nameLen - 1; j++)
									entryName += (char)nameBuf[j];

								// Check extension
								std::string entryExt;
								std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::tolower);
								size_t extPos = entryName.find_last_of(".");
								if (extPos != std::string::npos)
									entryExt = entryName.substr(extPos + 1);

								if (testerExtensions == "" ||
									(entryExt != "" && std::find(tokens.begin(), tokens.end(), entryExt) != tokens.end()))
								{
									foundIndex = i;
									foundEntryName = entryName;  // Save the entry name
									break;
								}
							}

							if (foundIndex == (UInt32)-1)
							{
								WorkerDbgMsg("libretro: ABORTED: Failed to locate a valid file in 7z.\n");
								bFailed7z = true;
							}
							else
							{
								// Extract the file
								UInt32 blockIndex = 0xFFFFFFFF;
								Byte* outBuffer = NULL;
								size_t outBufferSize = 0;
								size_t offset = 0;
								size_t outSizeProcessed = 0;

								res = SzArEx_Extract(&db, &lookStream.vt, foundIndex,
									&blockIndex, &outBuffer, &outBufferSize,
									&offset, &outSizeProcessed,
									&allocImp, &allocTempImp);

								if (res != SZ_OK)
								{
									WorkerDbgMsg("libretro: ABORTED: Failed to extract from 7z. Error: %d\n", res);
									bFailed7z = true;
								}
								else
								{
									// Copy to our own buffer (since outBuffer is managed by allocImp)
									fileData = malloc(outSizeProcessed);
									memcpy(fileData, outBuffer + offset, outSizeProcessed);
									bDataLoaded = true;

									game.data = fileData;
									game.size = outSizeProcessed;
									game.meta = NULL;

									// Track archive metadata for RETRO_ENVIRONMENT_GET_GAME_INFO_EXT
									info->loaded_archive_path = filename;
									info->loaded_archive_file = foundEntryName;
									info->loaded_file_in_archive = true;

									// Parse the extracted file's path components
									ParsePathComponents(foundEntryName, info->loaded_dir, info->loaded_name, info->loaded_ext);
									// The directory is still the 7z's directory
									info->loaded_dir = filename.substr(0, filename.find_last_of("/\\"));

									// Now apply content override using the EXTRACTED file's extension
									bool extractedNeedFullpath = info->need_fullpath;
									bool extractedPersistentData = false;
									if (!info->loaded_ext.empty() &&
										ApplyContentOverride(info, info->loaded_ext, extractedNeedFullpath, extractedPersistentData))
									{
										WorkerDbgMsg("libretro: Using content override for extracted file extension '%s': need_fullpath=%d, persistent_data=%d\n",
											info->loaded_ext.c_str(), extractedNeedFullpath, extractedPersistentData);
										info->loaded_persistent_data = extractedPersistentData;

										// If the override requires fullpath, we need to abort and reload with fullpath
										if (extractedNeedFullpath && !needFullpath)
										{
											WorkerDbgMsg("libretro: WARNING: Extracted file requires fullpath but archive was loaded in memory. This may cause issues.\n");
										}
									}

									// Only adjust game.path for cores that DON'T need fullpath.
									// Cores with need_fullpath=1 (like bsnes) may open game.path from disk,
									// so it must point to a real file. Cores with need_fullpath=0 (like mesen)
									// use game.data but check game.path extension for file type identification.
									if (!info->need_fullpath && !info->loaded_ext.empty())
									{
										adjustedGamePath = info->loaded_dir + "\\" + info->loaded_name + "." + info->loaded_ext;
										game.path = adjustedGamePath.c_str();
										WorkerDbgMsg("libretro: Adjusted game.path for extracted file: %s\n", game.path);
									}

									bReadyToLoad = true;
								}

								ISzAlloc_Free(&allocImp, outBuffer);
							}
						}

						SzArEx_Free(&db, &allocImp);
						ISzAlloc_Free(&allocImp, lookStream.buf);
						File_Close(&archiveStream.file);
					}
				}
			}
			else
			{
				game.data = NULL;
				game.size = 0;
				game.meta = NULL;

				FileHandle_t fileHandle = filesystem->Open(pFilename, "rb");
				if (!fileHandle)
				{
					WorkerDbgMsg("libretro: ABORTED: Failed to open file %s\n", pFilename);
					//info->close = true;
				}
				else
				{
					int bufferSize = filesystem->Size(fileHandle);
					fileData = malloc(bufferSize);
					bDataLoaded = true;

					filesystem->Read(fileData, bufferSize, fileHandle);
					filesystem->Close(fileHandle);

					game.data = fileData;
					game.size = bufferSize;

					bReadyToLoad = true;
				}
			}

			delete[] pFilename;
		}
	}

	bool bSuccess = false;
	if (bReadyToLoad)
	{
		// load any existing save state
		//if (g_pFullFileSystem->FileExists(VarArgs("%s\\%s\\%s.sav", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str())))
		//{
		if (info->settings && info->settings->GetBool("statesaves"))
		{
			FileHandle_t fileHandle = filesystem->Open(VarArgs("%s\\%s\\%s.sav", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str()), "rb", "");
			if (fileHandle)
			{
				info->statesize = filesystem->Size(fileHandle);	// statesize remains ZERO if game was loaded state saves disabled.  this prevents erroneous cleanup.
				info->statedata = malloc(info->statesize);
				filesystem->Read(info->statedata, info->statesize, fileHandle);
				filesystem->Close(fileHandle);
			}
		}
		//}

		// Store loaded data reference so GET_GAME_INFO_EXT can return it during load_game()
		info->loaded_data = game.data;
		info->loaded_data_size = game.size;

		if (!SafeCallCore(info->raw->init))
		{
			WorkerDbgMsg("libretro: CRITICAL - Core crashed during init!\n");
			info->runninglibretrocores->last_error = "Core Crashed";
			info->close = true;
		}
		else
		{
			info->bDidInit = true;
		}

		// Register callbacks after init for cores where pre-registration
		// failed in state 3 (e.g. mesen dereferences objects created in init)
		if (info->bDidInit && !info->bCallbacksRegistered)
		{
			info->raw->set_video_refresh(C_LibretroInstance::cbVideoRefresh);
			info->raw->set_audio_sample(C_LibretroInstance::cbAudioSample);
			info->raw->set_audio_sample_batch(C_LibretroInstance::cbAudioSampleBatch);
			info->raw->set_input_poll(C_LibretroInstance::cbInputPoll);
			if (info->raw->set_input_state)
				info->raw->set_input_state(C_LibretroInstance::cbInputState);
			info->bCallbacksRegistered = true;
		}

		if (!info->bDidInit)
		{
			WorkerDbgMsg("libretro: ABORTED: init() failed.\n");
		}
		else if (info->close)	// somebody else could have closed us from a different thread while we were doing that bottleneck above
		{
			WorkerDbgMsg("libretro: ABORTED: Canceled before loading game.\n");
			//info->close = true;
		}
		else
		{
			if (!SafeLoadGame(info->raw->load_game, &game))
			{
				WorkerDbgMsg("libretro: ABORTED: Core could not load game.\n");
				//info->close = true;
			}
			else
			{
				WorkerDbgMsg("libretro: Finished loading game %s\n", filename.c_str());

				if (!info->close)
				{
					info->state = 5;
					bSuccess = true;
				}
				else
				{
					WorkerDbgMsg("libretro: ABORTED: Canceled while loading game.\n");
					bSuccess = false;
					//info->close = true;
				}
			}
		}
	}

	// Clear non-persistent data reference (core should have copied it during load_game)
	if (!info->loaded_persistent_data)
	{
		info->loaded_data = NULL;
		info->loaded_data_size = 0;
	}

	if (bDataLoaded && !info->loaded_persistent_data)
		free(fileData);

	//pLibretroInstance->OnGameLoaded();
	return bSuccess;
}

void C_LibretroInstance::OnCoreLoaded()
{
	DevMsg("Core finished loading!\n");
	m_info->state = 2;

	// automatically load a game right away...
	//	m_info->game = "V:/Movies/Flash Gordon (1980).avi";//file
	//m_info->game = "V:/Movies/Jay and silent Bob Strike Back (2001).avi";
	//"V:\\Movies\\Judge Dredd (1995).mp4";
}

//bool C_LibretroManager::BuildInterface(void* pLib, struct libretro_raw * myInterface)
bool C_LibretroInstance::BuildInterface(libretro_raw* raw, void* pLib)
{
	HMODULE hModule = *static_cast<HMODULE*>(pLib);

	//void(*get_system_info)(struct retro_system_info * info);

	// check if this is a libretro core...
	if (!GetProcAddress(hModule, "retro_get_system_info"))
		return false;

	raw->set_environment = (void(*)(retro_environment_t))GetProcAddress(hModule, "retro_set_environment");
	raw->set_video_refresh = (void(*)(retro_video_refresh_t))GetProcAddress(hModule, "retro_set_video_refresh");
	raw->set_audio_sample = (void(*)(retro_audio_sample_t))GetProcAddress(hModule, "retro_set_audio_sample");
	raw->set_audio_sample_batch = (void(*)(retro_audio_sample_batch_t))GetProcAddress(hModule, "retro_set_audio_sample_batch");
	raw->set_input_poll = (void(*)(retro_input_poll_t))GetProcAddress(hModule, "retro_set_input_poll");
	raw->set_input_state = (void(*)(retro_input_state_t))GetProcAddress(hModule, "retro_set_input_state");
	raw->init = (void(*)(void))GetProcAddress(hModule, "retro_init");
	raw->deinit = (void(*)(void))GetProcAddress(hModule, "retro_deinit");
	raw->api_version = (unsigned(*)(void))GetProcAddress(hModule, "retro_api_version");
	raw->get_system_info = (void(*)(struct retro_system_info*))GetProcAddress(hModule, "retro_get_system_info");
	raw->get_system_av_info = (void(*)(struct retro_system_av_info*))GetProcAddress(hModule, "retro_get_system_av_info");
	raw->set_controller_port_device = (void(*)(unsigned, unsigned))GetProcAddress(hModule, "retro_set_controller_port_device");
	raw->reset = (void(*)(void))GetProcAddress(hModule, "retro_reset");
	raw->run = (void(*)(void))GetProcAddress(hModule, "retro_run");
	raw->serialize_size = (size_t(*)(void))GetProcAddress(hModule, "retro_serialize_size");
	raw->serialize = (bool(*)(void* data, size_t size))GetProcAddress(hModule, "retro_serialize");
	raw->unserialize = (bool(*)(const void* data, size_t size))GetProcAddress(hModule, "retro_unserialize");
	raw->cheat_reset = (void(*)(void))GetProcAddress(hModule, "retro_cheat_reset");
	raw->cheat_set = (void(*)(unsigned, bool, const char*))GetProcAddress(hModule, "retro_cheat_set");
	raw->load_game = (bool(*)(const struct retro_game_info *))GetProcAddress(hModule, "retro_load_game");
	raw->load_game_special = (bool(*)(unsigned, const struct retro_game_info*, size_t))GetProcAddress(hModule, "retro_load_game_special");
	raw->unload_game = (void(*)(void))GetProcAddress(hModule, "retro_unload_game");
	raw->get_region = (unsigned(*)(void))GetProcAddress(hModule, "retro_get_region");
	raw->get_memory_data = (void*(*)(unsigned))GetProcAddress(hModule, "retro_get_memory_data");
	raw->get_memory_size = (size_t(*)(unsigned))GetProcAddress(hModule, "retro_get_memory_size");
	return true;
}

// PortAudio callback for non-blocking audio output (runs on audio thread)
static int LibretroAudioCallback(
	const void* pInputBuffer,
	void* pOutputBuffer,
	unsigned long nFramesPerBuffer,
	const PaStreamCallbackTimeInfo* pTimeInfo,
	PaStreamCallbackFlags statusFlags,
	void* pUserData)
{
	(void)pInputBuffer;
	(void)pTimeInfo;
	(void)statusFlags;

	LibretroInstanceInfo_t* info = (LibretroInstanceInfo_t*)pUserData;
	int16_t* pOut = (int16_t*)pOutputBuffer;
	unsigned int nSamplesRequested = nFramesPerBuffer * 2; // stereo: 2 samples per frame

	if (!info || !info->pAudioRingBuffer)
	{
		memset(pOut, 0, nSamplesRequested * sizeof(int16_t));
		return paContinue;
	}

	unsigned int nSamplesRead = RingBuf_Read(info->pAudioRingBuffer, pOut, nSamplesRequested);

	// Fill remainder with silence on underrun
	if (nSamplesRead < nSamplesRequested)
	{
		memset(pOut + nSamplesRead, 0, (nSamplesRequested - nSamplesRead) * sizeof(int16_t));
	}

	return paContinue;
}

void C_LibretroInstance::CreateAudioStream()
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();

	if (!info->soundAllowed)
		return;

	WorkerDbgMsg("Sample rate is: %.0f\n", info->samplerate);

	// Determine actual output rate for PortAudio.
	// Standard sound cards support up to 96kHz. Anything above is a raw emulator
	// clock (e.g. SameBoy reports 2097152 Hz for Game Boy CPU/2) and needs resampling.
	float paRate = info->samplerate;
	if (paRate > 96000.0f)
	{
		paRate = 48000.0f;
		info->outputsamplerate = 48000.0f;
		info->resampleAccumulator = 0.0;
		WorkerDbgMsg("Core rate %.0f exceeds 96kHz, will resample to %.0f Hz\n",
			info->samplerate, paRate);
	}
	else
	{
		info->outputsamplerate = 0;
		info->resampleAccumulator = 0.0;
	}

	// Allocate ring buffer
	AudioRingBuffer_t* pRing = new AudioRingBuffer_t;
	pRing->nCapacity = AUDIO_RING_BUFFER_SAMPLES;
	pRing->nMask = pRing->nCapacity - 1;
	pRing->pBuffer = new int16_t[pRing->nCapacity];
	memset(pRing->pBuffer, 0, pRing->nCapacity * sizeof(int16_t));
	pRing->nWritePos = 0;
	pRing->nReadPos = 0;
	info->pAudioRingBuffer = pRing;

	PaStreamParameters outputParameters;
	outputParameters.device = Pa_GetDefaultOutputDevice();

	if (outputParameters.device == -1)
	{
		WorkerDbgMsg("FAILED TO GET PORT AUDIO DEVICE!!\n");
		delete[] pRing->pBuffer;
		delete pRing;
		info->pAudioRingBuffer = NULL;
		return;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaStream* stream;
	PaError err = Pa_OpenStream(
		&stream,
		NULL,
		&outputParameters,
		paRate,
		256,          // frames per buffer for predictable callback intervals
		paNoFlag,
		LibretroAudioCallback,
		info);

	if (err != paNoError)
	{
		WorkerDbgMsg("Failed to open stream: %s\n", Pa_GetErrorText(err));
		delete[] pRing->pBuffer;
		delete pRing;
		info->pAudioRingBuffer = NULL;
		return;
	}

	info->audiostream = stream;
	WorkerDbgMsg("Opened PA stream!\n");

	err = Pa_StartStream(stream);

	if (err != paNoError)
	{
		WorkerDbgMsg("Failed to start stream: %s\n", Pa_GetErrorText(err));
		Pa_CloseStream(stream);
		info->audiostream = NULL;
		delete[] pRing->pBuffer;
		delete pRing;
		info->pAudioRingBuffer = NULL;
	}
	else
	{
		WorkerDbgMsg("Started stream!\n");
	}
}

void C_LibretroInstance::DestroyAudioStream(LibretroInstanceInfo_t* info)
{
	if (!info || !info->soundAllowed || !info->audiostream)
		return;

	PaStream* stream = info->audiostream;

	PaError err = Pa_AbortStream(stream);
	if (err != paNoError)
		WorkerDbgMsg("Failed to abort stream: %s\n", Pa_GetErrorText(err));
	else
		WorkerDbgMsg("Aborted PA stream!\n");

	err = Pa_CloseStream(stream);
	if (err != paNoError)
		WorkerDbgMsg("Failed to close stream: %s\n", Pa_GetErrorText(err));
	else
		WorkerDbgMsg("Closed PA stream!\n");

	info->audiostream = NULL;

	// Free ring buffer after stream is closed (callback can no longer fire)
	if (info->pAudioRingBuffer)
	{
		if (info->pAudioRingBuffer->pBuffer)
			delete[] info->pAudioRingBuffer->pBuffer;
		delete info->pAudioRingBuffer;
		info->pAudioRingBuffer = NULL;
	}

	info->samplerate = 0;
	info->outputsamplerate = 0;
	info->resampleAccumulator = 0.0;
}

unsigned MyThread(void *params)
{
	bool bDidRun = false;
	bool bCoreCrashed = false;
	bool bDidTimeBegin = false;

	LibretroInstanceInfo_t* info = (LibretroInstanceInfo_t*)params; // always use a struct!
	void* hDoneEvent = info ? info->hThreadDoneEvent : NULL;	// Save before info gets deleted in cleanup

	CSysModule* pModule;
	bool bDidLoadDll = false;
	if (info->libretroinstance && !info->close)
	{
		bool bDidLoadModule = false;
		pModule = SafeLoadModule(info->core.c_str());
		if (!pModule)
		{
			WorkerDbgMsg("libretro: ERROR - Failed to load %s\n", info->core.c_str());
			info->runninglibretrocores->last_error = "Core Load Failed";
			info->close = true;
		}
		else
		{
			bDidLoadDll = true;
			HMODULE hModule = reinterpret_cast<HMODULE>(pModule);
			if (!hModule || !C_LibretroInstance::BuildInterface(info->raw, &hModule))
			{
				WorkerDbgMsg("libretro: ERROR - Failed to build interface!\n");
				info->runninglibretrocores->last_error = "Core Initialization Failed";
				info->close = true;
			}
			else
			{
				bDidLoadModule = true;

				info->module = pModule;
				info->threadid = ThreadGetCurrentId();
				info->coreloaded = true;

				struct retro_system_info system_info;
				info->raw->get_system_info(&system_info);

				info->library_name = system_info.library_name;
				info->library_version = system_info.library_version;
				info->valid_extensions = system_info.valid_extensions;
				info->need_fullpath = system_info.need_fullpath;
				info->block_extract = system_info.block_extract;

				WorkerDbgMsg("Loaded libretro core:\n");
				WorkerDbgMsg("\tlibrary_name: %s\n", info->library_name.c_str());
				WorkerDbgMsg("\tlibrary_version: %s\n", info->library_version.c_str());
				WorkerDbgMsg("\tvalid_extensions: %s\n", info->valid_extensions.c_str());
				WorkerDbgMsg("\tneed_fullpath: %i\n", info->need_fullpath);
				WorkerDbgMsg("\tblock_extract: %i\n", info->block_extract);

				g_pAnarchyManager->GetLibretroManager()->OnLibretroInstanceCreated(info);	// FIXME: If instance is closed by the time this line is reached, might cause the crash!

				WorkerDbgMsg("Thread: core loaded.\n");
			}
		}

		bool bIsInit = (info->id == "init");
		int state;
		libretro_raw* raw = info->raw;
		while (!info->close)
		{
			state = info->state;

			if (state == 2)
			{
				CSysModule* myModule = Sys_LoadModule(VarArgs("%s\\bin\\portaudio_x86.dll", engine->GetGameDirectory()));
				if (myModule)
				{
					WorkerDbgMsg("portaudio_x86.dll loaded successfully.\n");

					PaError err = Pa_Initialize();
					if (err != paNoError)
						WorkerDbgMsg("Failed to initialize PA.\n");
					else
						WorkerDbgMsg("Initialized PA successfuly!\n");
				}
				else
					WorkerDbgMsg("Failed to load portaudio_x86.dll.\n");

				// Improve Sleep() resolution from ~15.6ms to ~1ms for accurate frame pacing
				timeBeginPeriod(1);
				bDidTimeBegin = true;

				info->state = 3;
			}
			else if (state == 3)
			{
				raw->set_environment(C_LibretroInstance::cbEnvironment);

				// Try to register callbacks before init() (works for most cores).
				// Cores like mesen crash here because they dereference objects
				// created in init() — caught by SEH, deferred to after init() in LoadGame().
				info->bCallbacksRegistered = SafePreRegisterCallbacks(raw);

				info->state = 4;
			}
			else if (state == 4)
			{
				// load a game if we have one
				if (info->game != "")
				{
					WorkerDbgMsg("Load the game next!!\n");
					if (C_LibretroInstance::LoadGame())
					{
						if (info->state == 5)
						{
							// setup the memory map prior to the 1st call to run

							if (true)
							{
								info->memorymap->saveramsize = info->raw->get_memory_size(RETRO_MEMORY_SAVE_RAM);
								if (info->memorymap->saveramsize > 0)
								{
									info->memorymap->saveramdata = (uint8_t*)info->raw->get_memory_data(RETRO_MEMORY_SAVE_RAM);

									// data must be altered PRIOR to the 1st run
									if (info->settings && info->settings->GetBool("cartsaves") && g_pFullFileSystem->FileExists(VarArgs("%s\\%s\\%s.srm", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str())))
									{
										FileHandle_t fileHandle = filesystem->Open(VarArgs("%s\\%s\\%s.srm", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str()), "rb", "");
										if (fileHandle)
										{
											filesystem->Read((void*)info->memorymap->saveramdata, info->memorymap->saveramsize, fileHandle);
											filesystem->Close(fileHandle);
										}
									}
								}
							}

							// Make GL context current on worker thread if using hardware rendering
							if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
							{
								LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
								if (gl_ctx->hglrc)
									wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);

								// Pre-bind our FBO before context_reset so cores that cache
								// get_current_framebuffer during init get the correct FBO
								if (gl_ctx->framebuffer)
									glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);

								// Deferred context_reset: must be called AFTER load_game() returns,
								// not during SET_HW_RENDER, so the core can set its internal flags first.
								// This matches the libretro spec and RetroArch's behavior.
								if (info->raw->context_reset)
								{
									WorkerDbgMsg("libretro: Calling deferred context_reset...\n");
									if (!SafeCallCore(info->raw->context_reset))
									{
										WorkerDbgMsg("libretro: CRITICAL - Core crashed during context_reset!\n");
										info->runninglibretrocores->last_error = "Core Crashed";
										bCoreCrashed = true;
										info->state = 6;
										break;
									}
									WorkerDbgMsg("libretro: context_reset completed.\n");
								}
							}

							if (!SafeRunCore(info->raw))	// complete the game loading by executing 1 run
							{
								WorkerDbgMsg("libretro: CRITICAL - Core crashed during initial run! Exception code caught by SEH.\n");
								info->runninglibretrocores->last_error = "Core Crashed";
								bCoreCrashed = true;
								info->state = 6;
								break;
							}


							// remember old state size
							if (info->settings && info->settings->GetBool("statesaves"))
							{
								// get current state size
								size_t currentStateSize = raw->serialize_size();

								size_t oldStateSize = info->statesize;

								// handle auto-loaded save states
								if (oldStateSize != 0)
								{
									if (oldStateSize != currentStateSize)
									{
										free(info->statedata);
										info->statedata = malloc(currentStateSize);
										raw->serialize(info->statedata, currentStateSize);	// so it's never empty or garbage
									}
									else if (oldStateSize == currentStateSize)
									{
										// load the state already contained within statedata
										if (raw->unserialize(info->statedata, oldStateSize))
											info->runninglibretrocores->last_msg = "State Loaded";
									}
								}
								else
								{
									info->statedata = malloc(currentStateSize);
									raw->serialize(info->statedata, currentStateSize);	// so it's never empty or garbage
								}

								// *always* accept the statesize provided by the core... UNLESS we have state saves disabled, then statesize MUST remain zero to avoid erroneous cleanup
								info->statesize = currentStateSize;
							}

							bDidRun = true;

						}
					}
					else
					{
						info->runninglibretrocores->last_error = "Game Load Failed";
						info->close = true;
					}
				}
			}
			else if (state == 5)
			{
				if (info->reset)
				{
					info->reset = false;
					info->paused = false;

					// Make GL context current before reset (retro_reset may do GL ops)
					if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
					{
						LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
						if (gl_ctx->hglrc)
							wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);
						if (gl_ctx->framebuffer)
							glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);
					}

					if (!SafeCallCore(info->raw->reset))
					{
						WorkerDbgMsg("libretro: CRITICAL - Core crashed during reset!\n");
						info->runninglibretrocores->last_error = "Core Crashed";
						bCoreCrashed = true;
						info->state = 6;
						break;
					}
				}
				else if (!info->paused)
				{

					// Make GL context current on worker thread if using hardware rendering
					if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
					{
						LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
						if (gl_ctx->hglrc)
							wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);

						// Pre-bind our FBO so cores that don't call get_current_framebuffer()
						// (or cache a stale FBO 0) still render to the correct target.
						if (gl_ctx->framebuffer)
							glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);
					}

					if (!SafeRunCore(info->raw))
					{
						WorkerDbgMsg("libretro: CRITICAL - Core crashed during run! Exception code caught by SEH.\n");
						info->runninglibretrocores->last_error = "Core Crashed";
						bCoreCrashed = true;
						info->state = 6;
						break;
					}

					// Frame pacing: wait if ring buffer is filling up (audio-driven throttle)
					// This replaces the implicit pacing that the old blocking Pa_WriteStream provided.
					if (info->pAudioRingBuffer && info->audiostream)
					{
						unsigned int nThreshold = 2048; // Target ~21ms max latency at 48kHz stereo
						while (!info->close && !info->paused &&
							RingBuf_Available(info->pAudioRingBuffer) > nThreshold)
						{
							Sleep(1);
						}
					}
					else if (info->framerate > 0)
					{
						// No audio stream -- coarse timer-based pacing fallback
						Sleep((DWORD)(1000.0f / info->framerate));
					}

					if (bIsInit)
						info->state = 6;
				}
			}
			else if (state == 6) // waiting to die (requested by the libretro core)
			{
				Sleep(16);	// ~60Hz check, avoid burning CPU while waiting for close signal
			}
		}
	}

	// Restore default Sleep() resolution (only if we called timeBeginPeriod)
	if (bDidTimeBegin)
		timeEndPeriod(1);

	if (info)
	{
		// save any current state contained in the core to a file.
		// Skip if core crashed -- calling serialize() on corrupted state would hang or produce garbage.
		if (bDidRun && !bCoreCrashed && !info->bForceShutdown)
		{
			if (info->statesize > 0 && info->settings && info->settings->GetBool("statesaves"))
			{
				info->raw->serialize(info->statedata, info->statesize);

				CUtlBuffer buf;
				buf.Put(info->statedata, info->statesize);
				g_pFullFileSystem->CreateDirHierarchy(VarArgs("%s\\%s", info->savepath.c_str(), info->prettycore.c_str()));
				g_pFullFileSystem->WriteFile(VarArgs("%s\\%s\\%s.sav", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str()), "", buf);
				buf.Purge();

				info->runninglibretrocores->last_msg = "State Saved";
			}

			if (info->memorymap->saveramsize > 0 && info->settings && info->settings->GetBool("cartsaves"))
			{
				CUtlBuffer buf;
				buf.Put(info->memorymap->saveramdata, info->memorymap->saveramsize);
				g_pFullFileSystem->CreateDirHierarchy(VarArgs("%s\\%s", info->savepath.c_str(), info->prettycore.c_str()));
				g_pFullFileSystem->WriteFile(VarArgs("%s\\%s\\%s.srm", info->savepath.c_str(), info->prettycore.c_str(), info->prettygame.c_str()), "", buf);
				buf.Purge();
			}
		}

		if (info->statesize > 0 && info->statedata)
			free(info->statedata);

		RunningLibretroCores_t* pRunningLibretroCores = info->runninglibretrocores;

		info->statesize = 0;

		// Call core's context_destroy callback before unload (needs GL context active and raw valid)
		// Skip if core crashed -- calling into corrupted core state can deadlock.
		if (AA_LIBRETRO_3D && info->gl_context && info->raw && info->raw->context_destroy && !bCoreCrashed && !info->bForceShutdown)
		{
			LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
			if (gl_ctx->hglrc)
			{
				WorkerDbgMsg("libretro: Calling core's context_destroy callback...\n");
				wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);
				info->raw->context_destroy();
			}
		}

		// Properly shut down the libretro core before unloading DLL
		// Skip if core crashed -- unload_game/deinit on corrupted state can deadlock (SEH can't catch that).
		// Only call if init() actually succeeded (bDidInit) -- calling on uninitialized core is undefined.
		if (info->bDidInit && info->raw && !bCoreCrashed && !info->bForceShutdown)
		{
			if (info->raw->unload_game)
			{
				if (!SafeCallCore(info->raw->unload_game))
					WorkerDbgMsg("libretro: WARNING - unload_game crashed\n");
			}
			if (info->raw->deinit)
			{
				if (!SafeCallCore(info->raw->deinit))
					WorkerDbgMsg("libretro: WARNING - deinit crashed\n");
			}
		}

		// Destroy audio stream after core shutdown calls.
		// Must happen AFTER unload_game/deinit because cores may call cbAudioSampleBatch
		// during shutdown (e.g. to flush audio buffers). Destroying audio first causes the
		// callback to return 0, which some cores interpret as "blocked" and busy-wait.
		if (info->audiostream)
		{
			C_LibretroInstance::DestroyAudioStream(info);
		}

		// Unload the DLL.
		// Skip if core crashed -- FreeLibrary can deadlock on the loader lock
		// if the DLL has internal threads that haven't exited. Leaking the DLL
		// is acceptable; the address space cost is small and reclaimed on process exit.
		if (bDidLoadDll && !bCoreCrashed && !info->bForceShutdown)
		{
			Sys_UnloadModule(pModule);
			WorkerDbgMsg("Unloaded Libretro core.\n");
		}
		else if (bDidLoadDll && (bCoreCrashed || info->bForceShutdown))
		{
			WorkerDbgMsg("libretro: Skipping DLL unload -- %s (DLL leaked to avoid deadlock)\n",
				bCoreCrashed ? "core crashed" : "force shutdown");
		}

		// Free the raw function pointer struct (allocated in Init, function pointers are invalid after DLL unload)
		if (info->raw)
		{
			delete info->raw;
			info->raw = NULL;
		}

		// NOTE: lastframedata is NOT freed here - destructor needs it for snapshot saving
		// It will be freed in CleanUpTexture() after the snapshot is saved

		// Clear active HW instance pointer before GL cleanup
		if (s_pActiveHWInstance && s_pActiveHWInstance->GetInfo() == info)
			s_pActiveHWInstance = null;

		// Clean up OpenGL resources if hardware rendering was used.
		// Uses SEH wrapper since GL driver state may be corrupted after a core crash.
		if (AA_LIBRETRO_3D && info->gl_context)
		{
			LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;

			if (gl_ctx->hglrc)
			{
				WorkerDbgMsg("libretro: Cleaning up OpenGL resources...\n");

				if (!SafeCleanupGL(gl_ctx))
				{
					WorkerDbgMsg("libretro: GL cleanup failed, attempting fallback...\n");
					SafeCleanupGLFallback(gl_ctx);
				}
			}

			// Free the context struct (always -- this is our memory)
			delete gl_ctx;
			info->gl_context = NULL;
		}

		info->libretrokeybinds->deleteThis();
		info->corekeybinds->deleteThis();
		info->gamekeybinds->deleteThis();
		info->inputstate->deleteThis();
		info->coreCoreOptions->deleteThis();
		info->gameCoreOptions->deleteThis();

		// Clean up allocated variable strings to prevent memory leaks
		for (unsigned int i = 0; i < info->allocated_variable_strings.size(); i++)
		{
			delete[] info->allocated_variable_strings[i];
		}
		info->allocated_variable_strings.clear();

		// Clean up allocated game_info_ext structures
		for (unsigned int i = 0; i < info->allocated_game_info_ext.size(); i++)
		{
			delete info->allocated_game_info_ext[i];
		}
		info->allocated_game_info_ext.clear();

		delete info->memorymap;
		// NOTE: info struct is NOT deleted here - destructor needs it for snapshot saving
		// It will be deleted in the destructor after CleanUpTexture() completes

		InterlockedDecrement(&pRunningLibretroCores->count);
	}

	// Signal main thread that worker is completely done
	// Save handle before delete since info is freed above
	if (hDoneEvent)
		SetEvent((HANDLE)hDoneEvent);

	return 0;
}

bool C_LibretroInstance::IsSelected()
{
	return (this == g_pAnarchyManager->GetLibretroManager()->GetSelectedLibretroInstance());
}

bool C_LibretroInstance::HasFocus()
{
	return (this == g_pAnarchyManager->GetLibretroManager()->GetFocusedLibretroInstance());
}

bool C_LibretroInstance::Focus()
{
	return g_pAnarchyManager->GetLibretroManager()->FocusLibretroInstance(this);
}

bool C_LibretroInstance::Select()
{
	return g_pAnarchyManager->GetLibretroManager()->SelectLibretroInstance(this);
}

bool C_LibretroInstance::Blur()
{
	if (g_pAnarchyManager->GetLibretroManager()->GetFocusedLibretroInstance())
		g_pAnarchyManager->GetLibretroManager()->FocusLibretroInstance(null);

	return true;
}

bool C_LibretroInstance::Deselect()
{
	return g_pAnarchyManager->GetLibretroManager()->SelectLibretroInstance(null);
}

void C_LibretroInstance::Close()
{
	g_pAnarchyManager->GetLibretroManager()->DestroyLibretroInstance(this);
}

void C_LibretroInstance::GetFullscreenInfo(float& fPositionX, float& fPositionY, float& fSizeX, float& fSizeY, std::string& overlayId)
{
	fPositionX = m_pOverlayKV->GetFloat("current/x", 0);
	fPositionY = m_pOverlayKV->GetFloat("current/y", 0);
	fSizeX = m_pOverlayKV->GetFloat("current/width", 1);
	fSizeY = m_pOverlayKV->GetFloat("current/height", 1);
	overlayId = m_overlayId;
}

bool C_LibretroInstance::CreateWorkerThread(std::string core)
{
	std::string corePath = core.substr(0, core.find_last_of("/\\") + 1);

	m_info = new LibretroInstanceInfo_t;
	m_info->soundAllowed = g_pAnarchyManager->GetLibretroManager()->IsSoundAllowed();
	m_info->runninglibretrocores = g_pAnarchyManager->GetLibretroManager()->GetLibretroRunningCores();
	m_info->state = 0;
	m_info->paused = false;
	m_info->reset = false;
	m_info->close = false;
	m_info->hThreadDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	// manual-reset, initially unsignaled
	m_info->bForceShutdown = 0;
	m_info->bDidInit = false;
	m_info->bCallbacksRegistered = false;
	m_info->id = "";
	m_info->ready = false;

	float volume = cvar->FindVar("libretro_volume")->GetFloat();
	if (volume > 1.0)
		volume = 1.0;
	m_info->volume = volume;
	InterlockedExchange(&m_info->readyfornextframe, 1);
	InterlockedExchange(&m_info->copyingframe, 0);
	InterlockedExchange(&m_info->readytocopyframe, 0);
	m_info->coreloaded = false;
	m_info->gameloaded = false;
	m_info->raw = m_raw;
	m_info->corepath = corePath;// m_corePath;
	m_info->assetspath = g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_ASSETS_PATH);
	m_info->systempath = g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_SYSTEM_PATH);
	m_info->savepath = g_pAnarchyManager->GetLibretroManager()->GetLibretroPath(RETRO_SAVE_PATH);
	m_info->module = null;// pModule;
	m_info->threadid = 0;
	m_info->libretroinstance = this;
	m_info->core = core;
	m_info->game = m_originalGame;
	m_info->lastframedata = null;
	m_info->lastframebuffersize = 0;
	m_info->lastframewidth = 0;
	m_info->lastframeheight = 0;
	m_info->lastframepitch = 0;
	m_info->videoformat = RETRO_PIXEL_FORMAT_UNKNOWN;
	m_info->optionshavechanged = false;
	m_info->audiostream = null;
	m_info->pAudioRingBuffer = NULL;
	m_info->samplerate = 0;
	m_info->outputsamplerate = 0;
	m_info->resampleAccumulator = 0.0;
	m_info->framerate = 30;
	m_info->lastrendered = 0;

	// OpenGL hardware rendering (allocated on-demand by SET_HW_RENDER handler)
	m_info->gl_context = NULL;

	m_info->portdata = null;
	m_info->numports = 0;

	// hardware acceleration stuff
	m_info->context_type = RETRO_HW_CONTEXT_NONE;
	m_info->depth = false;
	m_info->stencil = false;
	m_info->bottom_left_origin = true;
	m_info->version_major = 0;
	m_info->version_minor = 0;
	m_info->cache_context = true;
	m_info->debug_context = false;

	// system info stuff
	m_info->library_name = "";
	m_info->library_version = "";
	m_info->valid_extensions = "";
	m_info->need_fullpath = true;
	m_info->block_extract = false;

	// content info override support
	m_info->has_content_overrides = false;

	// extended game info tracking
	m_info->loaded_full_path = "";
	m_info->loaded_archive_path = "";
	m_info->loaded_archive_file = "";
	m_info->loaded_dir = "";
	m_info->loaded_name = "";
	m_info->loaded_ext = "";
	m_info->loaded_file_in_archive = false;
	m_info->loaded_persistent_data = false;

	// state stuff
	m_info->statesize = 0;
	m_info->statedata = null;

	m_info->memorymap = new memory_map_t;
	m_info->memorymap->rtcsize = 0;
	m_info->memorymap->rtcdata = null;
	m_info->memorymap->saveramsize = 0;
	m_info->memorymap->saveramdata = null;
	m_info->memorymap->systemramsize = 0;
	m_info->memorymap->systemramdata = null;
	m_info->memorymap->videoramsize = 0;
	m_info->memorymap->videoramdata = null;

	// LIBRETRO-WIDE KEYBINDS
	KeyValues* kv = new KeyValues("keybinds");
	if (kv->LoadFromFile(g_pFullFileSystem, "libretro\\user\\keybinds.key", "MOD"))
		DevMsg("Loaded libretro keybinds!\n");
	m_info->libretrokeybinds = kv;

	// CORE-SPECIFIC KEYBINDS
	std::string prettyCore = m_info->core;
	size_t found = prettyCore.find_last_of("/\\");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(found + 1);

	found = prettyCore.find_last_of(".");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(0, found);
	prettyCore.erase(std::remove(prettyCore.begin(), prettyCore.end(), '.'), prettyCore.end());

	std::string kvPath = VarArgs("libretro\\user\\%s", prettyCore.c_str());

	kv = new KeyValues("keybinds");
	kv->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\keybinds.key", kvPath.c_str()), "MOD");
	m_info->corekeybinds = kv;

	// GAME-SPECIFIC KEYBINDS
	std::string prettyGame = m_info->game;
	found = prettyGame.find_last_of("/\\");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(found + 1);

	found = prettyGame.find_last_of(".");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(0, found);
	prettyGame.erase(std::remove(prettyGame.begin(), prettyGame.end(), '.'), prettyGame.end());

	m_info->prettygame = prettyGame;
	m_info->prettycore = prettyCore;

	kvPath = VarArgs("libretro\\user\\%s\\%s", prettyCore.c_str(), prettyGame.c_str());

	kv = new KeyValues("keybinds");
	kv->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\keybinds.key", kvPath.c_str()), "MOD");
	m_info->gamekeybinds = kv;

	// CURRENT KEYBINDS
	m_info->inputstate = new KeyValues("keybinds");	// just like the other structs, but holds backbuffer input values instead of source engine key enums.

	// CORE-SPECIFIC OPTIONS
	kvPath = "libretro\\user\\" + prettyCore + "\\options.key";

	kv = new KeyValues("options");
	kv->LoadFromFile(g_pFullFileSystem, kvPath.c_str(), "MOD");
	m_info->coreCoreOptions = kv;

	// GAME-SPECIFIC OPTIONS
	kvPath = "libretro\\user\\" + prettyCore + "\\" + prettyGame + "\\options.key";

	kv = new KeyValues("options");
	kv->LoadFromFile(g_pFullFileSystem, kvPath.c_str(), "MOD");
	m_info->gameCoreOptions = kv;

	// we are null settings by default
	m_info->settings = null;

	// but try to find us
	KeyValues* pCoreSettingsKV = g_pAnarchyManager->GetLibretroManager()->GetCoreSettingsKV();
	std::string compareCore = prettyCore + ".dll";
	std::string testerCore;
	for (KeyValues *sub = pCoreSettingsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		testerCore = sub->GetString("file");
		if (testerCore == compareCore)
		{
			m_info->settings = sub;
			break;
		}
	}

	std::string overlayId = g_pAnarchyManager->GetLibretroManager()->DetermineOverlay(prettyCore, prettyGame);
	this->SetOverlay(overlayId);
	g_pAnarchyManager->HudStateNotify();

	InterlockedIncrement(&m_info->runninglibretrocores->count);
	g_pAnarchyManager->AddToastMessage(VarArgs("Libretro Opened (%i running)", m_info->runninglibretrocores->count));
	CreateSimpleThread(MyThread, m_info);

	return true;
}
void C_LibretroInstance::cbMessage(enum retro_log_level level, const char * fmt, ...)
{
	va_list args;

	char msg[AA_MAX_STRING];

	va_start(args, fmt);
	int neededlen = Q_vsnprintf(msg, AA_MAX_STRING, fmt, args);
	va_end(args);

	std::string buf = msg;
	if (buf.at(buf.length() - 1) != '\n')
		buf += "\n";

	WorkerDbgMsg("libretro: %s", buf.c_str());
}

// http://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

const char* GetFormatName(int format)
{
	if (format == 0)
		return "0RGB1555";
	else if (format == 1)
		return "XRGB8888";
	else if (format == 2)
		return "RGB565";
	else
		return "UNKOWN";
}

static retro_proc_address_t v3d_get_proc_address(const char * sym)
{
	// First try wglGetProcAddress for OpenGL extensions
	PROC proc = wglGetProcAddress(sym);

	// If NULL, try GetProcAddress for core OpenGL 1.1 functions
	// (wglGetProcAddress only works for extensions on Windows)
	if (!proc)
	{
		HMODULE opengl32 = GetModuleHandleA("opengl32.dll");
		if (opengl32)
			proc = GetProcAddress(opengl32, sym);
	}

	return (retro_proc_address_t)proc;
}

static uintptr_t v3d_get_current_framebuffer()
{
	// Use the global active HW instance pointer instead of thread-ID lookup.
	// Cores like PPSSPP call this from internal rendering threads that don't
	// match our registered worker thread ID -- thread-ID lookup returns null
	// and we'd return 0 (FBO 0 = hidden window backbuffer), causing black screen.
	C_LibretroInstance* pLibretroInstance = s_pActiveHWInstance;

	if (!pLibretroInstance)
	{
		// Fallback: thread-ID lookup (works for well-behaved single-threaded cores)
		uint uId = ThreadGetCurrentId();
		pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	}

	if (!pLibretroInstance)
	{
		WorkerDbgMsg("libretro: WARNING - get_current_framebuffer called but no active HW instance (thread %u)\n", ThreadGetCurrentId());
		return 0;
	}

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();

	if (info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
	{
		LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;

		// One-time diagnostic: log FBO handle and thread info
		static bool s_bLoggedFBO = false;
		if (!s_bLoggedFBO)
		{
			WorkerDbgMsg("libretro: get_current_framebuffer() returning FBO %u (calling thread %u)\n",
				gl_ctx->framebuffer, ThreadGetCurrentId());
			s_bLoggedFBO = true;
		}

		return (uintptr_t)gl_ctx->framebuffer;
	}

	return 0;
}


bool set_rumble_state(unsigned port, enum retro_rumble_effect effect, uint16_t strength)
{
	return true;
}

bool C_LibretroInstance::cbEnvironment(unsigned cmd, void* data)
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);

	if (!pLibretroInstance)
		return false;

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();

	//1 SET_ROTATION, no known supported core uses that. Cores are expected to deal with failures, anyways.
	//2 GET_OVERSCAN, I have no opinion. Use the default.
	if (cmd == RETRO_ENVIRONMENT_GET_OVERSCAN) //2
	{
		WorkerDbgMsg("libretro: Asking frontend if overscan should be included or cropped.\n");
		*(bool*)data = false;
		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_CAN_DUPE) //3
	{
		WorkerDbgMsg("libretro: Asking frontend if CAN_DUPE.\n");
		*(bool*)data = true;
		return true;
	}

	//4 was removed and can safely be ignored.
	//5 was removed and can safely be ignored.
	//6 SET_MESSAGE, ignored because I don't know what to do with that.
	if (cmd == RETRO_ENVIRONMENT_SET_MESSAGE)
	{
		const struct retro_message* msg = (const struct retro_message*)data;
		std::string text = msg->msg;
		WorkerDbgMsg("libretro: Set Message (%u): %s\n", msg->frames, text.c_str());
		return true;
	}

	if (cmd == 60) // RETRO_ENVIRONMENT_SET_MESSAGE_EXT
	{
		const struct retro_message_ext* msg = (const struct retro_message_ext*)data;
		if (msg && msg->msg)
			WorkerDbgMsg("libretro: Set Message Ext: %s (duration=%ums, level=%d, target=%d)\n", msg->msg, msg->duration, msg->level, msg->target);
		return true;
	}

	//7 SHUTDOWN, ignored because no supported core has any reason to have Off buttons.
	//8 SET_PERFORMANCE_LEVEL, ignored because I don't support a wide range of powers.
	if (cmd == RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_LIBRETRO_PATH || cmd == RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) // note that libretro path might be wanting a full file location including extension, but ignore that for now and treat it like the others.
	{
		WorkerDbgMsg("String requested...\n");
		std::string folder;

		if (cmd == RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY)
			folder = info->assetspath + "\\" + info->prettycore;// +"\\assets";
		else if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY)
			folder = info->systempath + "\\" + info->prettycore;// +"\\system";
		else if (cmd == RETRO_ENVIRONMENT_GET_LIBRETRO_PATH)
			folder = info->corepath + "\\" + info->core;	// this is a full file location w/ extension
		else if (cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY)
			folder = info->savepath + "\\" + info->prettycore;// +"\\save";

		if (cmd != RETRO_ENVIRONMENT_GET_LIBRETRO_PATH)
			g_pFullFileSystem->CreateDirHierarchy(folder.c_str());

		char* buf = new char[AA_MAX_STRING];
		Q_strcpy(buf, folder.c_str());

		// Track this allocation for later cleanup
		info->allocated_variable_strings.push_back(buf);

		//V_FixSlashes(buf, '/');

		WorkerDbgMsg("libretro: Returning string for ");
		if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY)
			WorkerDbgMsg("system");
		else if (cmd == RETRO_ENVIRONMENT_GET_LIBRETRO_PATH)
			WorkerDbgMsg("libretro");
		else if (cmd == RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY)
			WorkerDbgMsg("core assets");
		else if (cmd == RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY)
			WorkerDbgMsg("content");

		WorkerDbgMsg(" directory %s\n", buf);

		(*(const char**)data) = buf;

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) //10
	{
		enum retro_pixel_format newfmt = *(enum retro_pixel_format *)data;
		if (newfmt == RETRO_PIXEL_FORMAT_0RGB1555 || newfmt == RETRO_PIXEL_FORMAT_XRGB8888 ||
			newfmt == RETRO_PIXEL_FORMAT_RGB565)
		{
			WorkerDbgMsg("libretro: Setting video format to %s\n", GetFormatName(newfmt));
			info->videoformat = newfmt;
			return true;
		}
		else
		{
			WorkerDbgMsg("libretro: Failed at setting video to format %s\n", GetFormatName(newfmt));
			return false;
		}
	}

	if (cmd == RETRO_ENVIRONMENT_SET_HW_RENDER) //14
	{
		// Check if hardware rendering is enabled (controlled by AA_LIBRETRO_3D define)
		if (!AA_LIBRETRO_3D)
		{
			WorkerDbgMsg("libretro: Hardware rendering disabled (AA_LIBRETRO_3D not defined), denying HW context request\n");
			return false;
		}

		// ffmpeg core doesn't need hardware rendering - deny it unless AA_LIBRETRO_FFMPEG_3D_ALLOWED is set
		if (!AA_LIBRETRO_FFMPEG_3D_ALLOWED && info->core.find("ffmpeg") != std::string::npos)
		{
			WorkerDbgMsg("libretro: Denying hardware context for ffmpeg core (AA_LIBRETRO_FFMPEG_3D_ALLOWED is false)\n");
			return false;
		}

		WorkerDbgMsg("libretro: Core requesting HW context: ");

		struct retro_hw_render_callback * render = (struct retro_hw_render_callback*)data;

		// Store core's lifecycle callbacks
		info->raw->context_reset = (retro_hw_context_reset_t)render->context_reset;
		info->raw->context_destroy = (retro_hw_context_reset_t)render->context_destroy;

		// Store rendering configuration
		info->context_type = render->context_type;
		info->depth = render->depth;
		info->stencil = render->stencil;
		info->bottom_left_origin = render->bottom_left_origin;
		info->cache_context = render->cache_context;
		info->debug_context = render->debug_context;

		// Determine GL API type and version
		switch (info->context_type)
		{
		case RETRO_HW_CONTEXT_NONE:
			WorkerDbgMsg("NONE (UNSUPPORTED)\n");
			info->version_major = 0;
			info->version_minor = 0;
			break;

		case RETRO_HW_CONTEXT_OPENGL:
			WorkerDbgMsg("OpenGL (2.x)\n");
			info->version_major = 2;
			info->version_minor = 0;
			break;

		case RETRO_HW_CONTEXT_OPENGLES2:
			WorkerDbgMsg("OpenGL ES (2.0)\n");
			info->version_major = 2;
			info->version_minor = 0;
			break;

		case RETRO_HW_CONTEXT_OPENGL_CORE:
			WorkerDbgMsg("OpenGL (%u.%u)\n", render->version_major, render->version_minor);
			info->version_major = render->version_major;
			info->version_minor = render->version_minor;
			break;

		case RETRO_HW_CONTEXT_OPENGLES3:
			WorkerDbgMsg("OpenGL ES (3.0)\n");
			info->version_major = 3;
			info->version_minor = 0;
			break;

		case RETRO_HW_CONTEXT_OPENGLES_VERSION:
			WorkerDbgMsg("OpenGL ES (%u.%u)\n", render->version_major, render->version_minor);
			info->version_major = render->version_major;
			info->version_minor = render->version_minor;
			break;

		case RETRO_HW_CONTEXT_VULKAN:
			WorkerDbgMsg("Vulkan (UNSUPPORTED)\n");
			info->version_major = 0;
			info->version_minor = 0;
			break;

		default:
			WorkerDbgMsg("UNKNOWN (UNSUPPORTED)\n");
			info->version_major = 0;
			info->version_minor = 0;
			break;
		}

		WorkerDbgMsg("\tdepth: %i\n", info->depth);
		WorkerDbgMsg("\tstencil: %i\n", info->stencil);
		WorkerDbgMsg("\tbottom_left_origin: %i\n", info->bottom_left_origin);
		WorkerDbgMsg("\tversion_major: %u\n", info->version_major);
		WorkerDbgMsg("\tversion_minor: %u\n", info->version_minor);
		WorkerDbgMsg("\tcache_context: %i\n", info->cache_context);
		WorkerDbgMsg("\tdebug_context: %i\n", info->debug_context);

		// Allocate GL context structure if needed
		if (!info->gl_context)
		{
			info->gl_context = new LibretroGLContext();
			memset(info->gl_context, 0, sizeof(LibretroGLContext));
		}

		// Initialize GLFW and create OpenGL context
		// Create a hidden dummy window for WGL context
		LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;

		WNDCLASSA wc = {};
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = DefWindowProcA;
		wc.lpszClassName = "AArcadeGLHelper";
		RegisterClassA(&wc);

		gl_ctx->hwnd = CreateWindowExA(
			0, "AArcadeGLHelper", "AArcade OpenGL Helper",
			WS_POPUP, 0, 0, 1, 1,
			NULL, NULL, NULL, NULL
			);

		if (!gl_ctx->hwnd)
		{
			WorkerDbgMsg("libretro: Failed to create dummy window.\n");
			return false;
		}

		gl_ctx->hdc = GetDC(gl_ctx->hwnd);
		if (!gl_ctx->hdc)
		{
			WorkerDbgMsg("libretro: Failed to get device context.\n");
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		// Set up pixel format
		PIXELFORMATDESCRIPTOR pfd = {};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = info->depth ? 24 : 0;
		pfd.cStencilBits = info->stencil ? 8 : 0;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pixelFormat = ChoosePixelFormat(gl_ctx->hdc, &pfd);
		if (!pixelFormat)
		{
			WorkerDbgMsg("libretro: Failed to choose pixel format.\n");
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		if (!SetPixelFormat(gl_ctx->hdc, pixelFormat, &pfd))
		{
			WorkerDbgMsg("libretro: Failed to set pixel format.\n");
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		// Create a temporary context for GLEW initialization
		HGLRC tempContext = wglCreateContext(gl_ctx->hdc);
		if (!tempContext)
		{
			WorkerDbgMsg("libretro: Failed to create temporary GL context.\n");
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}


		wglMakeCurrent(gl_ctx->hdc, tempContext);

		// Initialize GLEW
		glewExperimental = GL_TRUE;
		GLenum glewError = glewInit();
		if (glewError != GLEW_OK)
		{
			WorkerDbgMsg("libretro: Failed to initialize GLEW: %s\n", glewGetErrorString(glewError));
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(tempContext);
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		// Create the actual context with specific version if needed
		if (info->version_major > 2 && WGLEW_ARB_create_context)
		{
			// Use wglCreateContextAttribsARB for OpenGL 3.0+ contexts
			int attribs[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, (int)info->version_major,
				WGL_CONTEXT_MINOR_VERSION_ARB, (int)info->version_minor,
				WGL_CONTEXT_FLAGS_ARB, info->debug_context ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0
			};

			gl_ctx->hglrc = wglCreateContextAttribsARB(gl_ctx->hdc, NULL, attribs);
			if (!gl_ctx->hglrc)
			{
				WorkerDbgMsg("libretro: Failed to create versioned GL context, using compatibility context...\n");
				gl_ctx->hglrc = tempContext;
				tempContext = NULL;
			}
			else
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(tempContext);
				wglMakeCurrent(gl_ctx->hdc, gl_ctx->hglrc);
			}
		}
		else
		{
			// Use the temporary context for OpenGL 2.x
			gl_ctx->hglrc = tempContext;
		}

		if (!gl_ctx->hglrc)
		{
			WorkerDbgMsg("libretro: Failed to create GL context.\n");
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		WorkerDbgMsg("libretro: OpenGL context created successfully.\n");

		// Log OpenGL information
		const GLubyte* renderer = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);
		WorkerDbgMsg("\tRenderer: %s\n", renderer);
		WorkerDbgMsg("\tOpenGL version: %s\n", version);
		WorkerDbgMsg("=========================\n");

		// Create Framebuffer Object (FBO) for offscreen rendering
		// Note: FBO will be created with initial size, and resized when core specifies actual dimensions
		gl_ctx->hw_render_width = 1280;
		gl_ctx->hw_render_height = 720;

		// Generate and bind FBO
		glGenFramebuffers(1, &gl_ctx->framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);

		// Create color texture
		glGenTextures(1, &gl_ctx->color_texture);
		glBindTexture(GL_TEXTURE_2D, gl_ctx->color_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl_ctx->hw_render_width, gl_ctx->hw_render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_ctx->color_texture, 0);

		// Create depth/stencil renderbuffer if requested
		if (info->depth || info->stencil)
		{
			glGenRenderbuffers(1, &gl_ctx->depth_stencil_renderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, gl_ctx->depth_stencil_renderbuffer);

			if (info->depth && info->stencil)
			{
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gl_ctx->hw_render_width, gl_ctx->hw_render_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_ctx->depth_stencil_renderbuffer);
			}
			else if (info->depth)
			{
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, gl_ctx->hw_render_width, gl_ctx->hw_render_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_ctx->depth_stencil_renderbuffer);
			}
			else // stencil only
			{
				glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, gl_ctx->hw_render_width, gl_ctx->hw_render_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_ctx->depth_stencil_renderbuffer);
			}
		}

		// Check FBO completeness
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			WorkerDbgMsg("libretro: Framebuffer is incomplete! Status: 0x%X\n", status);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &gl_ctx->framebuffer);
			glDeleteTextures(1, &gl_ctx->color_texture);
			if (gl_ctx->depth_stencil_renderbuffer)
				glDeleteRenderbuffers(1, &gl_ctx->depth_stencil_renderbuffer);
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(gl_ctx->hglrc);
			ReleaseDC(gl_ctx->hwnd, gl_ctx->hdc);
			DestroyWindow(gl_ctx->hwnd);
			return false;
		}

		WorkerDbgMsg("libretro: Framebuffer created successfully (%u x %u)\n", gl_ctx->hw_render_width, gl_ctx->hw_render_height);

		// Unbind FBO (will be bound by core during rendering)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Wire up callbacks for core
		render->get_current_framebuffer = v3d_get_current_framebuffer;
		render->get_proc_address = v3d_get_proc_address;

		// NOTE: context_reset is NOT called here. Per the libretro spec, context_reset
		// must be called AFTER retro_load_game() returns, not during SET_HW_RENDER.
		// Cores like mupen64plus-next set internal flags after SET_HW_RENDER returns
		// that context_reset depends on. Calling it here would be too early.
		// The deferred call happens in the worker thread after LoadGame() completes.

		// Register as active HW instance so get_current_framebuffer works from any thread
		s_pActiveHWInstance = pLibretroInstance;

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE) //15
	{
		struct retro_variable * variable = (struct retro_variable*)data;

		// Force-disable flycast threaded rendering to prevent crashes on internal thread.
		// Flycast's internal rendering thread has no SEH protection from our frontend,
		// and crashes there kill the entire process. Disabling makes retro_run() synchronous.
		// NOTE: Commented out - flycast is incompatible with 32-bit frontend (nvmem pointer wrapping).
		// Keeping code for reference in case a future flycast build fixes the 32-bit issue.
		//if (std::string(variable->key) == "reicast_threaded_rendering")
		//{
		//	static const char* disabled_val = "disabled";
		//	variable->value = disabled_val;
		//	WorkerDbgMsg("Requesting variable: %s = %s (FORCED - threaded rendering disabled for stability)\n", variable->key, variable->value);
		//	return true;
		//}

		variable->value = NULL;

		bool bFoundVal = false;
		std::string val;
		KeyValues* kv;
		unsigned int index;
		unsigned int numOptions = info->options.size();
		for (index = 0; index < numOptions; index++)
		{
			if (std::string(variable->key) == info->options[index]->name_internal)
				break;
		}

		kv = info->gameCoreOptions;
		if (!Q_strcmp(kv->GetString(variable->key, "default"), "default"))
			kv = info->coreCoreOptions;

		if (Q_strcmp(kv->GetString(variable->key, "default"), "default"))
		{
			val = kv->GetString(variable->key);
			bFoundVal = true;
		}
		else if (index < numOptions)
		{
			if (!info->options[index]->default_value.empty())
				val = info->options[index]->default_value.c_str();
			else
				val = info->options[index]->values[0].c_str();
			bFoundVal = true;
		}
		else
		{
			WorkerDbgMsg("WARNING: Libretro core requested a variable that it did not tell us about before hand!: %s\n", variable->key);

			// try to reply with a default response (even tho we dont know what a default response is cuz this core never told us shit.)
			if (info->core.find("mame") != std::string::npos)
			{
				val = "disabled";
				bFoundVal = true;
			}
		}

		if (bFoundVal)
		{
			// Allocate buffer and track it for later cleanup
			char* buf = new char[AA_MAX_STRING];
			Q_strcpy(buf, val.c_str());
			variable->value = buf;

			// Track this allocation so we can clean it up later
			info->allocated_variable_strings.push_back(buf);

			// Output as single atomic DevMsg to prevent interleaving with other threads
			WorkerDbgMsg("Requesting variable: %s = %s\n", variable->key, buf);

		}
		else
			variable->value = null;

		info->optionshavechanged = false;
		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_VARIABLES)//16
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_VARIABLES\n");
		const struct retro_variable * variables = (const struct retro_variable*)data;

		// variables are stored as const chars and must be manually dealloc OBSOLTETE: i think they are strings now.
		while (!info->options.empty())
		{
			info->options.pop_back();
		}

		const struct retro_variable * variables_count = variables;

		while (variables_count->key) variables_count++;
		unsigned int numvars = variables_count - variables;

		bool bOptionListHasChanged = true;
		bool bOptionsHaveChanged = true;
		WorkerDbgMsg("Num vars is: %i\n", numvars);
		for (unsigned int i = 0; i<numvars; i++)
		{
			// Initialize to 0 index for this variable's value

			WorkerDbgMsg("libretro: Setting up environment variable %s with definition %s of %u\n", variables[i].key, variables[i].value, i);

			libretro_core_option* pOption = new libretro_core_option();
			pOption->name_internal = variables[i].key;

			const char * values = Q_strstr(variables[i].value, "; ");

			//if the value does not contain "; ", the core is broken, and broken cores can break shit in whatever way they want, anyways.
			//let's segfault.
			// In other words, values would be null and a crash would occur when we tried to use it.
			pOption->name_display = variables[i].value;
			size_t found = pOption->name_display.find("; ");
			if (found != std::string::npos)
				pOption->name_display = pOption->name_display.substr(0, found);

			unsigned int numvalues = 1;
			const char * valuescount = values;
			while (*valuescount)
			{
				if (*valuescount == '|') numvalues++;
				valuescount++;
			}

			std::string buf;
			const char * nextvalue = values;
			for (unsigned int j = 0; j<numvalues; j++)
			{
				nextvalue = values;
				while (*nextvalue && *nextvalue != '|') nextvalue++;
				unsigned int valuelen = nextvalue - values;

				buf = values;
				if (j == 0)
					buf = buf.substr(2, valuelen - 2);
				else
					buf = buf.substr(0, valuelen);

				pOption->values.push_back(buf);
				values = nextvalue + 1;
			}

			info->options.push_back(pOption);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS) //53
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CORE_OPTIONS\n");
		const struct retro_core_option_definition * def = (const struct retro_core_option_definition*)data;

		// Clear existing options
		while (!info->options.empty())
			info->options.pop_back();

		// Count definitions
		unsigned int numvars = 0;
		const struct retro_core_option_definition * def_count = def;
		while (def_count->key) { def_count++; numvars++; }

		WorkerDbgMsg("Num vars is: %i\n", numvars);

		for (unsigned int i = 0; i < numvars; i++)
		{
			WorkerDbgMsg("libretro: Setting up core option v1 %s with desc %s of %u\n", def[i].key, def[i].desc, i);

			libretro_core_option* pOption = new libretro_core_option();
			pOption->name_internal = def[i].key;
			pOption->name_display = def[i].desc;
			if (def[i].default_value)
				pOption->default_value = def[i].default_value;

			// Parse all possible values
			for (unsigned int j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; j++)
			{
				if (def[i].values[j].value == NULL)
					break;
				pOption->values.push_back(def[i].values[j].value);
			}

			info->options.push_back(pOption);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL) //54
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL\n");
		const struct retro_core_options_intl* options_intl = (const struct retro_core_options_intl*)data;

		// Use the US/English options (always non-NULL per spec)
		const struct retro_core_option_definition* def = options_intl->us;
		if (!def)
			return false;

		// Same logic as SET_CORE_OPTIONS handler above
		while (!info->options.empty())
			info->options.pop_back();

		unsigned int numvars = 0;
		const struct retro_core_option_definition* def_count = def;
		while (def_count->key) { def_count++; numvars++; }

		WorkerDbgMsg("Num vars is: %i\n", numvars);

		for (unsigned int i = 0; i < numvars; i++)
		{
			WorkerDbgMsg("libretro: Setting up core option v1 intl %s with desc %s of %u\n", def[i].key, def[i].desc, i);

			libretro_core_option* pOption = new libretro_core_option();
			pOption->name_internal = def[i].key;
			pOption->name_display = def[i].desc;
			if (def[i].default_value)
				pOption->default_value = def[i].default_value;

			for (unsigned int j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; j++)
			{
				if (def[i].values[j].value == NULL)
					break;
				pOption->values.push_back(def[i].values[j].value);
			}

			info->options.push_back(pOption);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) //67
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2\n");
		const struct retro_core_options_v2 * options_v2 = (const struct retro_core_options_v2*)data;

		// Clear existing options
		while (!info->options.empty())
			info->options.pop_back();

		// Parse all options from the definitions array
		const struct retro_core_option_v2_definition * def = options_v2->definitions;
		unsigned int numvars = 0;

		// Count definitions
		const struct retro_core_option_v2_definition * def_count = def;
		while (def_count->key) { def_count++; numvars++; }

		WorkerDbgMsg("Num vars is: %i\n", numvars);

		for (unsigned int i = 0; i < numvars; i++)
		{
			WorkerDbgMsg("libretro: Setting up core option v2 %s with desc %s of %u\n", def[i].key, def[i].desc, i);

			libretro_core_option* pOption = new libretro_core_option();
			pOption->name_internal = def[i].key;
			pOption->name_display = def[i].desc;
			if (def[i].default_value)
				pOption->default_value = def[i].default_value;

			// Parse all possible values
			for (unsigned int j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; j++)
			{
				if (def[i].values[j].value == NULL)
					break;
				pOption->values.push_back(def[i].values[j].value);
			}

			info->options.push_back(pOption);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL) // 68
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL\n");
		const struct retro_core_options_v2_intl* options_intl = (const struct retro_core_options_v2_intl*)data;

		// Use the US/English options (always non-NULL per spec)
		const struct retro_core_options_v2* options_v2 = options_intl->us;
		if (!options_v2)
			return false;

		// Same logic as SET_CORE_OPTIONS_V2 handler above
		while (!info->options.empty())
			info->options.pop_back();

		const struct retro_core_option_v2_definition* def = options_v2->definitions;
		unsigned int numvars = 0;

		const struct retro_core_option_v2_definition* def_count = def;
		while (def_count->key) { def_count++; numvars++; }

		WorkerDbgMsg("Num vars is: %i\n", numvars);

		for (unsigned int i = 0; i < numvars; i++)
		{
			WorkerDbgMsg("libretro: Setting up core option v2 intl %s with desc %s of %u\n", def[i].key, def[i].desc, i);

			libretro_core_option* pOption = new libretro_core_option();
			pOption->name_internal = def[i].key;
			pOption->name_display = def[i].desc;
			if (def[i].default_value)
				pOption->default_value = def[i].default_value;

			for (unsigned int j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; j++)
			{
				if (def[i].values[j].value == NULL)
					break;
				pOption->values.push_back(def[i].values[j].value);
			}

			info->options.push_back(pOption);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE) //17
	{
		*(bool*)data = info->optionshavechanged;
		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_PERF_INTERFACE) //18
	{
		WorkerDbgMsg("libretro: UNHANDLED RETRO_ENVIRONMENT_GET_PERF_INTERFACE\n");
		struct retro_perf_callback *cb = (struct retro_perf_callback*)data;

		cb->get_time_usec = null;
		cb->get_cpu_features = null;
		cb->get_perf_counter = null;

		cb->perf_register = null;
		cb->perf_start = null;
		cb->perf_stop = null;
		cb->perf_log = null;
		return false;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE) //23
	{
		struct retro_rumble_interface * iface = (struct retro_rumble_interface*)data;
		WorkerDbgMsg("libretro: Rumble interface requested.\n");
		iface->set_rumble_state = set_rumble_state;
		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_GET_LOG_INTERFACE) //27
	{
		struct retro_log_callback * logcb = (struct retro_log_callback*)data;
		logcb->log = &C_LibretroInstance::cbMessage;
		return true;
	}


	if (cmd == RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO) //32
	{
		const struct retro_system_av_info* avinfo = (const struct retro_system_av_info*)data;
		if (!avinfo)
		{
			WorkerDbgMsg("libretro: SET_SYSTEM_AV_INFO with null data\n");
			return false;
		}

		float oldSampleRate = info->samplerate;
		float oldFrameRate = info->framerate;

		if (avinfo->timing.fps > 0)
			info->framerate = float(avinfo->timing.fps);
		if (avinfo->timing.sample_rate > 0)
			info->samplerate = float(avinfo->timing.sample_rate);

		WorkerDbgMsg("libretro: SET_SYSTEM_AV_INFO fps=%.2f->%.2f samplerate=%.0f->%.0f base=%ux%u\n",
			oldFrameRate, info->framerate, oldSampleRate, info->samplerate,
			avinfo->geometry.base_width, avinfo->geometry.base_height);

		pLibretroInstance->SetFramerate(info->framerate);

		// Create or recreate audio stream if sample rate changed
		if (info->samplerate != oldSampleRate && info->samplerate > 0)
		{
			if (oldSampleRate > 0 && info->audiostream)
			{
				// Sample rate changed with existing stream -- recreate
				WorkerDbgMsg("libretro: Sample rate changed, recreating audio stream\n");
				float newSampleRate = info->samplerate;
				C_LibretroInstance::DestroyAudioStream(info);
				info->samplerate = newSampleRate;
				C_LibretroInstance::CreateAudioStream();
			}
			else if (!info->audiostream)
			{
				// First time we have a valid sample rate -- create stream
				WorkerDbgMsg("libretro: Creating audio stream (sample rate set to %.0f)\n", info->samplerate);
				C_LibretroInstance::CreateAudioStream();
			}
		}

		// Resize FBO to match core's requested geometry
		if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
		{
			LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
			if (avinfo->geometry.base_width > 0 && avinfo->geometry.base_height > 0)
				ResizeFBO(gl_ctx, avinfo->geometry.base_width, avinfo->geometry.base_height, info->depth, info->stencil);
		}

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_GEOMETRY) //37
	{
		const struct retro_game_geometry* geom = (const struct retro_game_geometry*)data;
		if (geom)
		{
			WorkerDbgMsg("libretro: SET_GEOMETRY base=%ux%u max=%ux%u aspect=%.2f\n",
				geom->base_width, geom->base_height, geom->max_width, geom->max_height, geom->aspect_ratio);

			// Resize FBO to match core's requested geometry
			if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE)
			{
				LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;
				if (geom->base_width > 0 && geom->base_height > 0)
					ResizeFBO(gl_ctx, geom->base_width, geom->base_height, info->depth, info->stencil);
			}
		}
		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS)
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS\n");

		unsigned controller, typeIndex;
		const struct retro_input_descriptor *controllerData = (const struct retro_input_descriptor*)data;

		for (controller = 0; controllerData[controller].description; controller++)
		{
			WorkerDbgMsg("Unhandled Controller info:\n");
			WorkerDbgMsg("\tPort: %u\n", controllerData[controller].port);
			WorkerDbgMsg("\tDevice: %u\n", controllerData[controller].device);
			WorkerDbgMsg("\tIndex: %u\n", controllerData[controller].index);
			WorkerDbgMsg("\tID: %u\n", controllerData[controller].id);
			WorkerDbgMsg("\tDescription: %s\n", controllerData[controller].description);

		}

		return false;
	}

	if (cmd == RETRO_ENVIRONMENT_SET_CONTROLLER_INFO) //35
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CONTROLLER_INFO\n");

		unsigned port, typeIndex;
		const struct retro_controller_info *portData = (const struct retro_controller_info*)data;

		info->currentPortTypes.clear();
		for (port = 0; portData[port].types; port++)
		{
			WorkerDbgMsg("Controller port: %u\n", port + 1);
			info->currentPortTypes.push_back(1);	// set every joystick to use the 1st entry (should always be RetroPad w/ id 1).
			// NOTE: 0 must ALWAYS gets inserted to the front when the current ports are gotten, which would mean Unplugged.
			// NOTE: These are vector indecies, NOT retro device IDs

			for (typeIndex = 0; typeIndex < portData[port].num_types; typeIndex++)
				WorkerDbgMsg("\t%s (ID: %u)\n", portData[port].types[typeIndex].desc, portData[port].types[typeIndex].id);
		}

		free((void*)info->portdata);
		info->portdata = (struct retro_controller_info*)
			calloc(port, sizeof(*info->portdata));
		memcpy((void*)info->portdata, portData,
			port * sizeof(*info->portdata));

		info->numports = port;

		return true;
	}

	if (cmd == RETRO_ENVIRONMENT_SHUTDOWN)
	{
		info->runninglibretrocores->last_error = "Core Shutdown";
		info->state = 6;
		// should probably show some kind of related items screen when a video ends.
		return true;
	}

	// RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE (47 | RETRO_ENVIRONMENT_EXPERIMENTAL)
	if (cmd == (47 | RETRO_ENVIRONMENT_EXPERIMENTAL))
	{
		// Tell the core to always render both audio and video
		if (data)
		{
			int* flags = (int*)data;
			*flags = (1 << 0) | (1 << 1);  // RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO
		}
		return true;
	}

	// RETRO_ENVIRONMENT_GET_FASTFORWARDING (49 | RETRO_ENVIRONMENT_EXPERIMENTAL)
	if (cmd == (49 | RETRO_ENVIRONMENT_EXPERIMENTAL))
	{
		if (data)
			*(bool*)data = false;  // We never fast-forward
		return true;
	}

	// RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK (62)
	if (cmd == 62)
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK (not implemented)\n");
		return false;  // Not implemented, core will handle gracefully
	}

	// RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY (63)
	if (cmd == 63)
	{
		if (data)
		{
			const unsigned* latency = (const unsigned*)data;
			WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY requested %u ms (ignored)\n", *latency);
		}
		return true;  // Acknowledge but don't change latency
	}

	// RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE (65)
	if (cmd == RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE)
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE\n");

		const struct retro_system_content_info_override* overrides =
			(const struct retro_system_content_info_override*)data;

		if (!overrides)
		{
			WorkerDbgMsg("libretro: Warning: SET_CONTENT_INFO_OVERRIDE called with NULL data\n");
			return false;
		}

		// Clear any existing overrides
		info->content_overrides.clear();

		// Copy the override array (it's null-terminated)
		int count = 0;
		while (overrides[count].extensions != NULL)
		{
			WorkerDbgMsg("libretro: Content override for extensions: %s (need_fullpath=%d, persistent_data=%d)\n",
				overrides[count].extensions,
				overrides[count].need_fullpath,
				overrides[count].persistent_data);

			info->content_overrides.push_back(overrides[count]);
			count++;
		}

		info->has_content_overrides = (count > 0);
		WorkerDbgMsg("libretro: Registered %d content info override(s)\n", count);

		return true;
	}

	// RETRO_ENVIRONMENT_GET_GAME_INFO_EXT (66)
	if (cmd == RETRO_ENVIRONMENT_GET_GAME_INFO_EXT)
	{
		WorkerDbgMsg("libretro: RETRO_ENVIRONMENT_GET_GAME_INFO_EXT\n");

		const struct retro_game_info_ext** game_info_ext_ptr =
			(const struct retro_game_info_ext**)data;

		if (!game_info_ext_ptr)
		{
			WorkerDbgMsg("libretro: GET_GAME_INFO_EXT querying support only\n");
			return true;  // Just querying support
		}

		// If called before load_game, we don't have valid data yet
		if (info->loaded_full_path.empty())
		{
			WorkerDbgMsg("libretro: GET_GAME_INFO_EXT called before game loaded, returning NULL\n");
			*game_info_ext_ptr = NULL;
			return false;
		}

		// Allocate a single retro_game_info_ext structure and track it for cleanup
		struct retro_game_info_ext* game_info_ext = new retro_game_info_ext;

		// Populate from stored metadata
		game_info_ext->full_path = info->loaded_full_path.empty() ? NULL : info->loaded_full_path.c_str();
		game_info_ext->archive_path = info->loaded_archive_path.empty() ? NULL : info->loaded_archive_path.c_str();
		game_info_ext->archive_file = info->loaded_archive_file.empty() ? NULL : info->loaded_archive_file.c_str();
		game_info_ext->dir = info->loaded_dir.empty() ? NULL : info->loaded_dir.c_str();
		game_info_ext->name = info->loaded_name.empty() ? NULL : info->loaded_name.c_str();
		game_info_ext->ext = info->loaded_ext.empty() ? NULL : info->loaded_ext.c_str();
		game_info_ext->meta = NULL;  // No implementation-specific metadata for now
		game_info_ext->data = info->loaded_data;
		game_info_ext->size = info->loaded_data_size;
		game_info_ext->file_in_archive = info->loaded_file_in_archive;
		game_info_ext->persistent_data = info->loaded_persistent_data;

		// Track this allocation for later cleanup
		info->allocated_game_info_ext.push_back(game_info_ext);

		WorkerDbgMsg("libretro: Returning game info ext:\n");
		WorkerDbgMsg("  full_path: %s\n", game_info_ext->full_path ? game_info_ext->full_path : "(null)");
		WorkerDbgMsg("  archive_path: %s\n", game_info_ext->archive_path ? game_info_ext->archive_path : "(null)");
		WorkerDbgMsg("  archive_file: %s\n", game_info_ext->archive_file ? game_info_ext->archive_file : "(null)");
		WorkerDbgMsg("  dir: %s\n", game_info_ext->dir ? game_info_ext->dir : "(null)");
		WorkerDbgMsg("  name: %s\n", game_info_ext->name ? game_info_ext->name : "(null)");
		WorkerDbgMsg("  ext: %s\n", game_info_ext->ext ? game_info_ext->ext : "(null)");
		WorkerDbgMsg("  file_in_archive: %d\n", game_info_ext->file_in_archive);

		*game_info_ext_ptr = game_info_ext;
		return true;
	}

	// RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION (52)
	if (cmd == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION)
	{
		unsigned* version = (unsigned*)data;
		*version = 2;
		return true;
	}

	// RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY (55)
	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY)
	{
		return true; // Acknowledge but don't change display
	}

	// RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER (56)
	if (cmd == RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER)
	{
		unsigned* preferred = (unsigned*)data;
		*preferred = RETRO_HW_CONTEXT_OPENGL;
		return true;
	}

	// RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION (59)
	if (cmd == 59)
	{
		unsigned* version = (unsigned*)data;
		*version = 1; // 1 = supports SET_MESSAGE_EXT
		return true;
	}

	// RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK (69)
	if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK)
	{
		return false; // Not implementing the display update callback
	}

	// RETRO_ENVIRONMENT_SET_VARIABLE (70) - Core forcibly sets an option value
	if (cmd == RETRO_ENVIRONMENT_SET_VARIABLE) //70
	{
		if (!data)
			return true; // Available but no data provided

		const struct retro_variable* variable = (const struct retro_variable*)data;
		if (!variable->key || !variable->value)
			return false;

		WorkerDbgMsg("libretro: SET_VARIABLE: %s = %s\n", variable->key, variable->value);

		// Find the matching option and validate the value
		unsigned int numOptions = info->options.size();
		for (unsigned int i = 0; i < numOptions; i++)
		{
			if (info->options[i]->name_internal == variable->key)
			{
				bool bValidValue = false;
				for (unsigned int v = 0; v < info->options[i]->values.size(); v++)
				{
					if (info->options[i]->values[v] == variable->value)
					{
						bValidValue = true;
						break;
					}
				}

				if (!bValidValue)
				{
					WorkerDbgMsg("libretro: SET_VARIABLE: invalid value '%s' for key '%s'\n",
						variable->value, variable->key);
					return false;
				}

				// Update core-level options (not persisted to disk -- runtime-only change)
				info->coreCoreOptions->SetString(variable->key, variable->value);
				info->optionshavechanged = true;
				return true;
			}
		}

		WorkerDbgMsg("libretro: SET_VARIABLE: unknown key '%s'\n", variable->key);
		return false;
	}

	if (cmd == (36 | RETRO_ENVIRONMENT_EXPERIMENTAL))  // SET_MEMORY_MAPS
	{
		const struct retro_memory_map* memmap = (const struct retro_memory_map*)data;
		if (memmap)
		{
			WorkerDbgMsg("libretro: SET_MEMORY_MAPS: %u descriptors\n", memmap->num_descriptors);
			for (unsigned i = 0; i < memmap->num_descriptors; i++)
			{
				const struct retro_memory_descriptor* desc = &memmap->descriptors[i];
				WorkerDbgMsg("  [%u] ptr=%p offset=0x%X start=0x%X len=0x%X\n",
					i, desc->ptr, (unsigned)desc->offset, (unsigned)desc->start, (unsigned)desc->len);
			}
		}
		return true;
	}

	const char * const names[] = {
		"(invalid)",
		"SET_ROTATION",
		"GET_OVERSCAN",
		"GET_CAN_DUPE",
		"(removed)",
		"(removed)",
		"SET_MESSAGE",
		"SHUTDOWN",
		"SET_PERFORMANCE_LEVEL",
		"GET_SYSTEM_DIRECTORY",
		"SET_PIXEL_FORMAT",
		"SET_INPUT_DESCRIPTORS",
		"SET_KEYBOARD_CALLBACK",
		"SET_DISK_CONTROL_INTERFACE",
		"SET_HW_RENDER",
		"GET_VARIABLE",
		"SET_VARIABLES",
		"GET_VARIABLE_UPDATE",
		"SET_SUPPORT_NO_GAME",
		"GET_LIBRETRO_PATH",
		"(removed)",
		"SET_FRAME_TIME_CALLBACK",
		"SET_AUDIO_CALLBACK",
		"GET_RUMBLE_INTERFACE",
		"GET_INPUT_DEVICE_CAPABILITIES",
		"GET_SENSOR_INTERFACE",
		"GET_CAMERA_INTERFACE",
		"GET_LOG_INTERFACE",
		"GET_PERF_INTERFACE",
		"GET_LOCATION_INTERFACE",
		"GET_CONTENT_DIRECTORY",
		"GET_SAVE_DIRECTORY",
		"SET_SYSTEM_AV_INFO",
		"SET_PROC_ADDRESS_CALLBACK",
		"SET_SUBSYSTEM_INFO",
		"SET_CONTROLLER_INFO",
		"SET_MEMORY_MAPS",
		"SET_GEOMETRY",
		"GET_USERNAME",
		"GET_LANGUAGE",
		"GET_CURRENT_SOFTWARE_FRAMEBUFFER",        // 40
		"GET_HW_RENDER_INTERFACE",                 // 41
		"SET_SUPPORT_ACHIEVEMENTS",                // 42
		"SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE", // 43
		"SET_SERIALIZATION_QUIRKS",                // 44
		"GET_VFS_INTERFACE",                       // 45
		"GET_LED_INTERFACE",                       // 46
		"GET_AUDIO_VIDEO_ENABLE",                  // 47
		"GET_MIDI_INTERFACE",                      // 48
		"GET_FASTFORWARDING",                      // 49
		"GET_TARGET_REFRESH_RATE",                 // 50
		"GET_INPUT_BITMASKS",                      // 51
		"GET_CORE_OPTIONS_VERSION",                // 52
		"SET_CORE_OPTIONS",                        // 53
		"SET_CORE_OPTIONS_INTL",                   // 54
		"SET_CORE_OPTIONS_DISPLAY",                // 55
		"GET_PREFERRED_HW_RENDER",                 // 56
		"GET_DISK_CONTROL_INTERFACE_VERSION",       // 57
		"SET_DISK_CONTROL_EXT_INTERFACE",           // 58
		"GET_MESSAGE_INTERFACE_VERSION",            // 59
		"SET_MESSAGE_EXT",                         // 60
		"SET_AUDIO_BUFFER_STATUS_CALLBACK",        // 61
		"SET_MINIMUM_AUDIO_LATENCY",               // 62
		"(reserved 63)",                           // 63
		"(reserved 64)",                           // 64
		"SET_CONTENT_INFO_OVERRIDE",               // 65
		"GET_GAME_INFO_EXT",                       // 66
		"SET_CORE_OPTIONS_V2",                     // 67
		"SET_CORE_OPTIONS_V2_INTL",                // 68
		"SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK", // 69
		"SET_VARIABLE",                            // 70
	};

	if ((cmd&~RETRO_ENVIRONMENT_EXPERIMENTAL) < sizeof(names) / sizeof(*names))
		C_LibretroInstance::cbMessage(RETRO_LOG_WARN, "Ignored unsupported environment command #%u %s.", cmd, names[cmd&~RETRO_ENVIRONMENT_EXPERIMENTAL]);
	else
		C_LibretroInstance::cbMessage(RETRO_LOG_WARN, "Ignored unsupported environment command #%u.", cmd);

	return false;
}

void C_LibretroInstance::cbVideoRefresh(const void * data, unsigned width, unsigned height, size_t pitch)
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	if (!pLibretroInstance)
		return;

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();
	if (info->close)
		return;

	if (!InterlockedCompareExchange(&info->readyfornextframe, 0, 0) || InterlockedCompareExchange(&info->copyingframe, 0, 0) || !data || pLibretroInstance->FastForwardSeconds() > pLibretroInstance->CurrentSeconds() + 10)
	{
		if (data)
		{
			pLibretroInstance->IncrementSkippedFrames();
		}
		return;
	}

	info->lastframewidth = width;
	info->lastframeheight = height;
	info->lastframepitch = pitch;

	info->lastrendered = gpGlobals->curtime;

	InterlockedExchange(&info->readyfornextframe, 0);
	InterlockedExchange(&info->readytocopyframe, 0);

	if (info->samplerate == 0)
	{
		WorkerDbgMsg("Get AV info\n");
		struct retro_system_av_info avinfo;
		info->raw->get_system_av_info(&avinfo);

		WorkerDbgMsg("Core reports: sample_rate=%.1f fps=%.2f\n", avinfo.timing.sample_rate, avinfo.timing.fps);

		if (avinfo.timing.sample_rate > 0 && avinfo.timing.fps > 0)
		{
			info->samplerate = float(avinfo.timing.sample_rate);
			info->framerate = float(avinfo.timing.fps);
			pLibretroInstance->SetFramerate(info->framerate);
			C_LibretroInstance::CreateAudioStream();

			if (info->audiostream)
				WorkerDbgMsg("Audio stream created at %.0f Hz (core rate: %.0f Hz)\n",
				info->outputsamplerate > 0 ? info->outputsamplerate : info->samplerate,
				info->samplerate);
			else
				WorkerDbgMsg("WARNING: Audio stream creation failed (core rate: %.0f Hz)\n",
				info->samplerate);
			// NOTE: Do NOT set readyfornextframe = true here!
			// The render synchronization flow will handle this in RegenerateTextureBits()
		}
		else
		{
			WorkerDbgMsg("WARNING: Core returned invalid AV info (sample_rate=%.1f fps=%.2f), will retry next frame\n",
				avinfo.timing.sample_rate, avinfo.timing.fps);
			// NOTE: Do NOT set readyfornextframe = true here!
			// Let the normal render flow handle synchronization
		}
	}

	size_t buffer_size = 0;
	if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE && data == RETRO_HW_FRAME_BUFFER_VALID)
	{
		// Hardware rendering - read from FBO on worker thread (GL context already current)
		WorkerDbgMsg("libretro: Hardware frame - reading from FBO on worker thread\n");
		LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;

		// Validate dimensions
		if (width == 0 || height == 0)
		{
			WorkerDbgMsg("libretro: ERROR - Invalid FBO dimensions (%u x %u), skipping frame\n", width, height);
			return;
		}

		// Allocate/reuse buffer for RGBA data (4 bytes per pixel)
		buffer_size = width * height * 4;
		pitch = width * 4;
		info->videoformat = RETRO_PIXEL_FORMAT_XRGB8888;
	}
	else
	{
		// Software rendering - copy from provided data pointer
		// Validate dimensions before allocating
		if (width == 0 || height == 0 || pitch == 0)
		{
			WorkerDbgMsg("libretro: ERROR - Invalid software buffer dimensions (w=%u h=%u pitch=%zu), skipping frame\n",
				width, height, pitch);
			return;
		}

		buffer_size = pitch * height;
	}

	// Reuse existing buffer if large enough, otherwise reallocate
	if (info->lastframebuffersize < buffer_size)
	{
		if (info->lastframedata)
			free(info->lastframedata);
		info->lastframedata = malloc(buffer_size);
		if (!info->lastframedata)
		{
			info->lastframebuffersize = 0;
			WorkerDbgMsg("libretro: ERROR - Failed to allocate %zu bytes for frame buffer\n", buffer_size);
			return;
		}
		info->lastframebuffersize = buffer_size;
	}

	if (AA_LIBRETRO_3D && info->gl_context && info->context_type != RETRO_HW_CONTEXT_NONE && data == RETRO_HW_FRAME_BUFFER_VALID)
	{
		LibretroGLContext* gl_ctx = (LibretroGLContext*)info->gl_context;

		// GL context is already current on worker thread from run()
		// Bind the FBO to read from it
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_ctx->framebuffer);

		// Read pixels from FBO (BGRA format to match XRGB8888 byte order on Windows)
		glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, info->lastframedata);

		// Check for GL errors
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
		{
			WorkerDbgMsg("libretro: glReadPixels error: 0x%X\n", err);
		}

		// Diagnostic: check first HW frame for all-zero data (black screen detection)
		static bool s_bLoggedFirstHWFrame = false;
		if (!s_bLoggedFirstHWFrame)
		{
			unsigned char* pixels = (unsigned char*)info->lastframedata;
			bool bAllZero = true;
			for (unsigned int i = 0; i < 256 && i < buffer_size; i++)
			{
				if (pixels[i] != 0) { bAllZero = false; break; }
			}
			WorkerDbgMsg("libretro: First HW frame: %ux%u, FBO=%u, glReadPixels %s\n",
				width, height, gl_ctx->framebuffer,
				bAllZero ? "ALL ZEROS (black)" : "has data (OK)");
			s_bLoggedFirstHWFrame = true;
		}

		// Unbind FBO (restore default framebuffer)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	else
	{
		Q_memcpy(info->lastframedata, data, buffer_size);
	}

	info->lastframewidth = width;
	info->lastframeheight = height;
	info->lastframepitch = pitch;

	InterlockedExchange(&info->readytocopyframe, 1);
	pLibretroInstance->MarkAsDirty();
	pLibretroInstance->IncrementRenderedFrames();
}

void C_LibretroInstance::cbAudioSample(int16_t left, int16_t right)
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	if (!pLibretroInstance)
		return;

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();
	if (!info->soundAllowed || !info->audiostream || !info->pAudioRingBuffer)
		return;

	// When resampling is active, decimate: only emit every Nth sample
	if (info->outputsamplerate > 0)
	{
		double step = (double)info->samplerate / (double)info->outputsamplerate;
		info->resampleAccumulator += 1.0;
		if (info->resampleAccumulator < step)
			return;
		info->resampleAccumulator -= step;
	}

	float volume = info->volume;
	double leftVal = (double)left * volume;
	double rightVal = (double)right * volume;

	if (leftVal > 32767.0) leftVal = 32767.0;
	else if (leftVal < -32768.0) leftVal = -32768.0;

	if (rightVal > 32767.0) rightVal = 32767.0;
	else if (rightVal < -32768.0) rightVal = -32768.0;

	int16_t sample[2];
	sample[0] = (int16_t)leftVal;
	sample[1] = (int16_t)rightVal;

	RingBuf_Write(info->pAudioRingBuffer, sample, 2);
}

size_t C_LibretroInstance::cbAudioSampleBatch(const int16_t * data, size_t frames)
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);

	if (!pLibretroInstance)
		return 0;

	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();
	if (!info->soundAllowed)
		return 0;

	if (pLibretroInstance->FastForwardSeconds() > pLibretroInstance->CurrentSeconds() + 10)
		return 0;

	// Lazy init: create audio stream on first audio data
	if (info->samplerate == 0)
	{
		struct retro_system_av_info avinfo;
		info->raw->get_system_av_info(&avinfo);

		if (avinfo.timing.sample_rate > 0)
		{
			info->samplerate = float(avinfo.timing.sample_rate);
			info->framerate = float(avinfo.timing.fps);
			pLibretroInstance->SetFramerate(info->framerate);
			C_LibretroInstance::CreateAudioStream();
		}
	}

	if (info->samplerate <= 0 || frames <= 0 || !info->audiostream || !info->pAudioRingBuffer)
		return 0;

	float volume = info->volume;

	if (info->outputsamplerate > 0)
	{
		// Resampling path: linear interpolation from core rate to output rate
		// e.g. SameBoy: 2097152 Hz -> 48000 Hz (ratio ~0.023, step ~43.69)
		double ratio = (double)info->outputsamplerate / (double)info->samplerate;
		double step = 1.0 / ratio;

		unsigned int nInputFrames = (unsigned int)frames;
		unsigned int nMaxOutputFrames = (unsigned int)((double)nInputFrames * ratio) + 2;
		unsigned int nMaxOutputSamples = nMaxOutputFrames * 2;

		int16_t stackBuf[4096];
		int16_t* pOutBuf;
		bool bHeapAlloc = false;

		if (nMaxOutputSamples <= 4096)
		{
			pOutBuf = stackBuf;
		}
		else
		{
			pOutBuf = new int16_t[nMaxOutputSamples];
			bHeapAlloc = true;
		}

		double srcPos = info->resampleAccumulator;
		unsigned int nOutIdx = 0;

		while (srcPos < (double)(nInputFrames - 1) && nOutIdx < nMaxOutputSamples)
		{
			unsigned int srcIdx = (unsigned int)srcPos;
			double frac = srcPos - (double)srcIdx;

			double leftSample = (double)data[srcIdx * 2] * (1.0 - frac)
				+ (double)data[(srcIdx + 1) * 2] * frac;
			double rightSample = (double)data[srcIdx * 2 + 1] * (1.0 - frac)
				+ (double)data[(srcIdx + 1) * 2 + 1] * frac;

			leftSample *= volume;
			rightSample *= volume;

			if (leftSample > 32767.0) leftSample = 32767.0;
			else if (leftSample < -32768.0) leftSample = -32768.0;
			if (rightSample > 32767.0) rightSample = 32767.0;
			else if (rightSample < -32768.0) rightSample = -32768.0;

			pOutBuf[nOutIdx++] = (int16_t)leftSample;
			pOutBuf[nOutIdx++] = (int16_t)rightSample;

			srcPos += step;
		}

		// Carry fractional remainder to next call for seamless batch boundaries
		info->resampleAccumulator = srcPos - (double)nInputFrames;
		if (info->resampleAccumulator < 0.0)
			info->resampleAccumulator = 0.0;

		RingBuf_Write(info->pAudioRingBuffer, pOutBuf, nOutIdx);

		if (bHeapAlloc)
			delete[] pOutBuf;
	}
	else
	{
		// Normal path: no resampling needed
		unsigned int nTotalSamples = (unsigned int)(frames * 2); // stereo

		int16_t stackBuf[4096];
		int16_t* pVolBuf;
		bool bHeapAlloc = false;

		if (nTotalSamples <= 4096)
		{
			pVolBuf = stackBuf;
		}
		else
		{
			pVolBuf = new int16_t[nTotalSamples];
			bHeapAlloc = true;
		}

		for (unsigned int i = 0; i < nTotalSamples; i++)
		{
			double val = (double)data[i] * volume;
			if (val > 32767.0)
				val = 32767.0;
			else if (val < -32768.0)
				val = -32768.0;
			pVolBuf[i] = (int16_t)val;
		}

		RingBuf_Write(info->pAudioRingBuffer, pVolBuf, nTotalSamples);

		if (bHeapAlloc)
			delete[] pVolBuf;
	}

	return frames;
}

void C_LibretroInstance::cbInputPoll(void)
{
	//DevMsg("cbInputPoll\n");
	// TODO: Implement this.  it might be related to timing of analog & mouse input devices because those expect offsets relative to the last time they were polled.
	// This is likely supposed to trigger the polling of input states on the FE for the core to later retrieve with InputState.
	//DevMsg("libretro: Input Poll called.\n");
}

int16_t C_LibretroInstance::cbInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
	uint uId = ThreadGetCurrentId();
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);

	if (!pLibretroInstance)
		return (int16_t)0;

	int iCurrentSeconds = pLibretroInstance->CurrentSeconds();
	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();

	if (info->close || !info->inputstate || port != 0)
		return (int16_t)0;

	// Handle RETRO_DEVICE_ANALOG: direct lookup from inputstate
	if (device == RETRO_DEVICE_ANALOG)
	{
		if (pLibretroInstance->FastForwardSeconds() > iCurrentSeconds + 10)
			return (int16_t)0;

		std::string keyPath = "port" + std::to_string(port) + "/device" + std::to_string(device) + "/index" + std::to_string(index) + "/key" + std::to_string(id);
		return (int16_t)info->inputstate->GetInt(keyPath.c_str());
	}

	// Only accept JOYPAD (device 1) index 0 from here on
	if (device != RETRO_DEVICE_JOYPAD || index != 0)
		return (int16_t)0;

	if (pLibretroInstance->FastForwardSeconds() > iCurrentSeconds + 10)
	{
		if (port == 0 && device == 1 && index == 0)
		{
			if (id == 4 && pLibretroInstance->FastForwardSeconds() > iCurrentSeconds + 60)	// 4 = 60 seconds
			{
				if (pLibretroInstance->GetLastDelta() == 0)
				{
					pLibretroInstance->FastForward(60, true);
					return (int16_t)1;
				}
				else
				{
					pLibretroInstance->SetLastDelta(0);
					return (int16_t)0;
				}
			}
			else if (id == 7 && pLibretroInstance->FastForwardSeconds() <= iCurrentSeconds + 60)	// 7 = 10 seconds
			{
				if (pLibretroInstance->GetLastDelta() == 0 || pLibretroInstance->GetLastDelta() == 60)
				{
					pLibretroInstance->FastForward(10, true);
					return (int16_t)1;
				}
				else
				{
					pLibretroInstance->SetLastDelta(0);
					return (int16_t)0;
				}
			}
		}
		return (int16_t)0;
	}

	std::string keyPath = "port" + std::to_string(port) + "/device" + std::to_string(device) + "/index" + std::to_string(index) + "/key" + std::to_string(id);
	int val = (int16_t)info->inputstate->GetInt(keyPath.c_str());

	if (port == 0 && device == 1 && index == 0)// && val == 1 && pLibretroInstance->GetLastDelta() == 0)
	{
		if (id == 4)	// 4 = 60 seconds
		{
			if (val != 0 && pLibretroInstance->GetLastDelta() == 0)
				pLibretroInstance->FastForward(60);
			else if (val == 0 && pLibretroInstance->GetLastDelta() == 60)
				pLibretroInstance->SetLastDelta(0);
		}
		else if (id == 7)	// 7 = 10 seconds
		{
			if (val != 0 && pLibretroInstance->GetLastDelta() == 0)
				pLibretroInstance->FastForward(10);
			else if (val == 0 && pLibretroInstance->GetLastDelta() == 10)
				pLibretroInstance->SetLastDelta(0);
		}
		else if (id == 5)	// 5 = -60 seconds
		{
			if (val != 0 && pLibretroInstance->GetLastDelta() == 0)
				pLibretroInstance->Rewind(-60);
			else if (val == 0 && pLibretroInstance->GetLastDelta() == -60)
				pLibretroInstance->SetLastDelta(0);
		}
		else if (id == 6)	// 6 = -10 seconds
		{
			if (val != 0 && pLibretroInstance->GetLastDelta() == 0)
				pLibretroInstance->Rewind(-10);
			else if (val == 0 && pLibretroInstance->GetLastDelta() == -10)
				pLibretroInstance->SetLastDelta(0);
		}
	}
	return (int16_t)val;
}

void C_LibretroInstance::ResizeFrameFromRGB565(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth)
{
	LibretroInstanceInfo_t* info = m_info;
	if (!info->lastframedata)
		return;

	WORD red_mask = 0xF800;
	WORD green_mask = 0x7E0;
	WORD blue_mask = 0x1F;

	uint16* pRealSrc = (uint16*)pSrc;

	unsigned char* pDstRow = (unsigned char*)pDst;
	for (int dstY = 0; dstY<destHeight; dstY++)
	{

		unsigned int srcY = dstY * sourceHeight / destHeight;
		uint16* pSrcRow = pRealSrc + (srcY * ((int)sourcePitch / 2));

		unsigned char* pDstCur = pDstRow;

		for (int dstX = 0; dstX<destWidth; dstX++)
		{
			int srcX = dstX * sourceWidth / destWidth;

			int red = (pSrcRow[srcX] & red_mask) >> 11;
			int green = (pSrcRow[srcX] & green_mask) >> 5;
			int blue = (pSrcRow[srcX] & blue_mask);

			pDstCur[0] = blue * (255 / 31);
			pDstCur[1] = green * (255 / 63);
			pDstCur[2] = red * (255 / 31);

			pDstCur[3] = 255;

			pDstCur += destDepth;
		}

		pDstRow += destPitch;
	}

}

void C_LibretroInstance::ResizeFrameFromRGB1555(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth)
{
	LibretroInstanceInfo_t* info = m_info;
	if (!info->lastframedata)
		return;

	WORD red_mask = 0x7C00;
	WORD green_mask = 0x03E0;
	WORD blue_mask = 0x001F;

	uint16* pRealSrc = (uint16*)pSrc;

	unsigned char* pDstRow = (unsigned char*)pDst;
	for (int dstY = 0; dstY<destHeight; dstY++)
	{

		unsigned int srcY = dstY * sourceHeight / destHeight;
		uint16* pSrcRow = pRealSrc + (srcY * ((int)sourcePitch / 2));

		unsigned char* pDstCur = pDstRow;

		for (int dstX = 0; dstX<destWidth; dstX++)
		{
			int srcX = dstX * sourceWidth / destWidth;

			int red = (pSrcRow[srcX] & red_mask) >> 10;
			int green = (pSrcRow[srcX] & green_mask) >> 5;
			int blue = (pSrcRow[srcX] & blue_mask);

			pDstCur[0] = blue * (255 / 31);
			pDstCur[1] = green * (255 / 31);
			pDstCur[2] = red * (255 / 31);

			pDstCur[3] = 255;

			pDstCur += destDepth;
		}

		pDstRow += destPitch;
	}

}

void C_LibretroInstance::ResizeFrameFromXRGB8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth, bool bFlip)
{
	if (!m_info->lastframedata)
		return;

	const unsigned char* pRealSrc = (const unsigned char*)pSrc;
	unsigned char* pDstRow = (unsigned char*)pDst;
	for (int dstY = 0; dstY<destHeight; dstY++)
	{
		unsigned int srcY = bFlip
			? (destHeight - 1 - dstY) * sourceHeight / destHeight
			: dstY * sourceHeight / destHeight;
		const unsigned char* pSrcRow = pRealSrc + srcY*(sourcePitch);

		unsigned char* pDstCur = pDstRow;

		for (int dstX = 0; dstX<destWidth; dstX++)
		{
			int srcX = dstX * sourceWidth / destWidth;
			pDstCur[0] = pSrcRow[srcX*sourceDepth + 0];
			pDstCur[1] = pSrcRow[srcX*sourceDepth + 1];
			pDstCur[2] = pSrcRow[srcX*sourceDepth + 2];

			pDstCur[3] = 255;

			pDstCur += destDepth;
		}

		pDstRow += destPitch;
	}
}

void C_LibretroInstance::ResizeFrameFromRGB888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth)
{
	if (!m_info->lastframedata)
		return;

	const unsigned char* pRealSrc = (const unsigned char*)pSrc;
	unsigned char* pDstRow = (unsigned char*)pDst;
	for (int dstY = 0; dstY<destHeight; dstY++)
	{
		unsigned int srcY = dstY * sourceHeight / destHeight;
		const unsigned char* pSrcRow = pRealSrc + srcY*(sourcePitch);

		unsigned char* pDstCur = pDstRow;

		for (int dstX = 0; dstX<destWidth; dstX++)
		{
			int srcX = dstX * sourceWidth / destWidth;
			pDstCur[0] = pSrcRow[srcX*sourceDepth + 2];
			pDstCur[1] = pSrcRow[srcX*sourceDepth + 1];
			pDstCur[2] = pSrcRow[srcX*sourceDepth + 0];

			pDstCur[3] = 255;

			pDstCur += destDepth;
		}

		pDstRow += destPitch;
	}
}

void C_LibretroInstance::CopyLastFrame(unsigned char* dest, unsigned int width, unsigned int height, size_t pitch, unsigned int depth)
{
	if (InterlockedCompareExchange(&m_info->copyingframe, 0, 0) || !InterlockedCompareExchange(&m_info->readytocopyframe, 0, 0) || g_pAnarchyManager->GetSuspendEmbedded())
		return;

	InterlockedExchange(&m_info->copyingframe, 1);
	InterlockedExchange(&m_info->readytocopyframe, 0);

	if (AA_LIBRETRO_3D && m_info->context_type != RETRO_HW_CONTEXT_NONE)
	{
		this->ResizeFrameFromXRGB8888(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 4, width, height, pitch, depth, m_info->bottom_left_origin);
	}
	else
	{
		if (m_info->videoformat == RETRO_PIXEL_FORMAT_RGB565)
			this->ResizeFrameFromRGB565(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);
		else if (m_info->videoformat == RETRO_PIXEL_FORMAT_XRGB8888)
			this->ResizeFrameFromXRGB8888(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 4, width, height, pitch, depth);
		else
			this->ResizeFrameFromRGB1555(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);
	}

	InterlockedExchange(&m_info->copyingframe, 0);
}

void C_LibretroInstance::OnProxyBind(C_BaseEntity* pBaseEntity)
{
	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;

	if (m_iLastVisibleFrame < gpGlobals->framecount)
	{
		m_iLastVisibleFrame = gpGlobals->framecount;

		if (m_bIsDirty && g_pAnarchyManager->GetCanvasManager()->RenderSeen(this) && g_pAnarchyManager->GetCanvasManager()->ShouldRender(this))
			Render();
	}
}

bool C_LibretroInstance::IsDirty()
{
	return m_bIsDirty && !InterlockedCompareExchange(&m_info->readyfornextframe, 0, 0) && InterlockedCompareExchange(&m_info->readytocopyframe, 0, 0);
}

void C_LibretroInstance::Render()
{
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(this);
	m_pTexture->Download();
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(null);

	m_iLastRenderedFrame = gpGlobals->framecount;

	g_pAnarchyManager->GetCanvasManager()->AllowRender(this);
}

void C_LibretroInstance::RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect)
{
	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;

	this->CopyLastFrame(pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);

	if (m_bTakeScreenshot) {
		this->TakeScreenshotNow(pTexture, pVTFTexture, pSubRect, pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
		m_bTakeScreenshot = false;
	}

	// fix the bleeding edges on projectors
	if (m_pProjectorFixConVar->GetBool())
	{
		CPixelWriter pixelWriter;
		pixelWriter.SetPixelMemory(pVTFTexture->Format(), pVTFTexture->ImageData(0, 0, 0), pVTFTexture->RowSizeInBytes(0));

		int xmax = pSubRect->x + pSubRect->width;
		int ymax = pSubRect->y + pSubRect->height;
		int x, y;
		for (y = pSubRect->y; y < ymax; ++y)
		{
			pixelWriter.Seek(pSubRect->x, y);
			x = pSubRect->x;
			while (x < xmax)
			{
				if (y == pSubRect->y || y == ymax - 1 || x == xmax - 1)
				{
					pixelWriter.WritePixel(0, 0, 0, 0);
					x++;
				}
				else if (x == pSubRect->x)
				{
					pixelWriter.WritePixel(0, 0, 0, 0);
					pixelWriter.SkipPixels(xmax - 2);
					x = xmax - 1;
				}
				else
				{
					pixelWriter.SkipPixels(1);
					x++;
				}
			}
		}
	}

	m_bIsDirty = false;
	InterlockedExchange(&m_info->readyfornextframe, 1);
}

C_InputListener* C_LibretroInstance::GetInputListener()
{
	return g_pAnarchyManager->GetLibretroManager()->GetInputListener();
}

bool C_LibretroInstance::SetGame(std::string file)
{
	if (!m_info || m_info->gameloaded || m_info->close)
		return false;

	m_info->game = file;
	return true;
}

void C_LibretroInstance::SetOriginalGame(std::string file)
{
	m_originalGame = file;
	m_originalGameHash = g_pAnarchyManager->GenerateLegacyHash(file.c_str());
}

C_EmbeddedInstance* C_LibretroInstance::GetParentSelectedEmbeddedInstance()
{
	return g_pAnarchyManager->GetLibretroManager()->GetSelectedLibretroInstance();
}

void C_LibretroInstance::SaveLibretroKeybind(std::string type, unsigned int retroport, unsigned int retrodevice, unsigned int retroindex, unsigned int retrokey, std::string steamkey)
{
	// pretty CORE
	std::string prettyCore = m_info->core;
	size_t found = prettyCore.find_last_of("/\\");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(found + 1);

	found = prettyCore.find_last_of(".");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(0, found);
	prettyCore.erase(std::remove(prettyCore.begin(), prettyCore.end(), '.'), prettyCore.end());

	// pretty GAME
	std::string prettyGame = m_info->game;
	found = prettyGame.find_last_of("/\\");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(found + 1);

	found = prettyGame.find_last_of(".");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(0, found);
	prettyGame.erase(std::remove(prettyGame.begin(), prettyGame.end(), '.'), prettyGame.end());

	// now do keybind stuff
	KeyValues* kv;
	std::string savePath;
	if (type == "libretro")
	{
		kv = m_info->libretrokeybinds;	// LIBRETRO-WIDE KEYBINDS
		savePath = "libretro\\user";
	}
	else if (type == "core")
	{
		kv = m_info->corekeybinds;	// CORE-SPECIFIC KEYBINDS
		savePath = "libretro\\user\\" + prettyCore;
	}
	else if (type == "game")
	{
		kv = m_info->gamekeybinds;	// GAME-SPECIFIC KEYBINDS
		savePath = "libretro\\user\\" + prettyCore + "\\" + prettyGame;
	}

	// add the info to the KV: PORT/INDEX/TYPE/RETORKEY = STEAMKEY
	kv->SetString(VarArgs("port%u/device%u/index%u/key%u", retroport, retrodevice, retroindex, retrokey), steamkey.c_str());

	// save the KV out
	// (load up a fresh version and write ONLY this value to it to avoid saving other shit that we don't really want to save at this time.)
	KeyValues* fresh;
	if (type != "libretro")
	{
		fresh = new KeyValues("keybinds");
		fresh->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\keybinds.key", savePath.c_str()), "DEFAULT_WRITE_PATH");
	}
	else
		fresh = kv;

	if (steamkey != "default")
		fresh->SetString(VarArgs("port%u/device%u/index%u/key%u", retroport, retrodevice, retroindex, retrokey), steamkey.c_str());
	else
		fresh->SetString(VarArgs("port%u/device%u/index%u/key%u", retroport, retrodevice, retroindex, retrokey), "");

	g_pFullFileSystem->CreateDirHierarchy(savePath.c_str(), "DEFAULT_WRITE_PATH");
	fresh->SaveToFile(g_pFullFileSystem, VarArgs("%s\\keybinds.key", savePath.c_str()), "DEFAULT_WRITE_PATH");
	if (type != "libretro")
		fresh->deleteThis();
}

void C_LibretroInstance::GetLastMouse(float &fMouseX, float &fMouseY)
{
	fMouseX = m_fLastMouseX;
	fMouseY = m_fLastMouseY;
}

void C_LibretroInstance::SaveLibretroOption(std::string type, std::string name_internal, std::string value)
{
	// pretty CORE
	std::string prettyCore = m_info->core;
	size_t found = prettyCore.find_last_of("/\\");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(found + 1);

	found = prettyCore.find_last_of(".");
	if (found != std::string::npos)
		prettyCore = prettyCore.substr(0, found);
	prettyCore.erase(std::remove(prettyCore.begin(), prettyCore.end(), '.'), prettyCore.end());

	// pretty GAME
	std::string prettyGame = m_info->game;
	found = prettyGame.find_last_of("/\\");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(found + 1);

	found = prettyGame.find_last_of(".");
	if (found != std::string::npos)
		prettyGame = prettyGame.substr(0, found);
	prettyGame.erase(std::remove(prettyGame.begin(), prettyGame.end(), '.'), prettyGame.end());

	// now do keybind stuff
	KeyValues* kv;
	std::string savePath;
	if (type == "core")
	{
		kv = m_info->coreCoreOptions;	// CORE-SPECIFIC OPTIONS
		savePath = "libretro\\user\\" + prettyCore;
	}
	else if (type == "game")
	{
		kv = m_info->gameCoreOptions;	// GAME-SPECIFIC OPTIONS
		savePath = "libretro\\user\\" + prettyCore + "\\" + prettyGame;
	}

	// add the info to the KV: NAME_INTERNAL = VALUE
	kv->SetString(name_internal.c_str(), value.c_str());
	m_info->optionshavechanged = true;

	// save the KV out
	// (load up a fresh version and write ONLY this value to it to avoid saving other shit that we don't really want to save at this time.)
	KeyValues* fresh = new KeyValues("options");
	fresh->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\options.key", savePath.c_str()), "DEFAULT_WRITE_PATH");
	if (value != "default")
		fresh->SetString(name_internal.c_str(), value.c_str());
	else
		fresh->SetString(name_internal.c_str(), "");

	g_pFullFileSystem->CreateDirHierarchy(savePath.c_str(), "DEFAULT_WRITE_PATH");
	fresh->SaveToFile(g_pFullFileSystem, VarArgs("%s\\options.key", savePath.c_str()), "DEFAULT_WRITE_PATH");
	fresh->deleteThis();
}

void C_LibretroInstance::ResetFastForwardSeconds()
{
	m_iFastForwardSeconds = 0;

	C_AwesomiumBrowserInstance* pNetwork = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
	if (pNetwork)
		pNetwork->GetWebView()->ExecuteJavascript(WSLit(VarArgs("localStorage.setItem(\"libtime%s\", %i)", m_originalGameHash.c_str(), m_iFastForwardSeconds)), WSLit(""));
}

void C_LibretroInstance::OnSecondsUpdated()
{
	if (m_info->core.find("ffmpeg") == std::string::npos)
		return;

	int iSeconds = this->CurrentSeconds();
	if (iSeconds >= m_iFastForwardSeconds)	//iSeconds >
	{
		m_iFastForwardSeconds = iSeconds;
		C_AwesomiumBrowserInstance* pNetwork = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
		if (pNetwork)
			pNetwork->GetWebView()->ExecuteJavascript(WSLit(VarArgs("localStorage.setItem(\"libtime%s\", %i)", m_originalGameHash.c_str(), iSeconds)), WSLit(""));
	}
}

int C_LibretroInstance::GetLibretroSeconds()
{
	return this->CurrentSeconds();
}

int C_LibretroInstance::GetLibretroStartSeconds()
{
	return this->m_iFastForwardSeconds;
}

void C_LibretroInstance::SetAdjustedStartTime()
{
	m_iAdjustedStartTime = static_cast<int>(ceil(engine->Time()));
}

void C_LibretroInstance::FastForward(int iAmount, bool bAutoSkip)
{
	if (m_info->core.find("ffmpeg") == std::string::npos)
		return;

	m_iAdjustedStartTime -= iAmount;

	if (bAutoSkip && m_iFastForwardSeconds <= this->CurrentSeconds() + 10)
		m_iLastDelta = 0;
	else
		m_iLastDelta = iAmount;
}

int C_LibretroInstance::CurrentSeconds()
{
	int t = ((static_cast<int>(ceil(engine->Time()))) - m_iAdjustedStartTime);
	if (t < 0)
		t = 0;

	return t;
}

void C_LibretroInstance::Rewind(int iAmount, bool bAutoSkip)
{
	if (m_info->core.find("ffmpeg") == std::string::npos)
		return;

	m_iAdjustedStartTime -= iAmount;

	m_iLastDelta = iAmount;

	if (!bAutoSkip)
	{
		m_iFastForwardSeconds += iAmount;
		if (m_iFastForwardSeconds < 0)
			m_iFastForwardSeconds = 0;
	}
}