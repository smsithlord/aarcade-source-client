#include "cbase.h"
#include "aa_globals.h"
#include "c_anarchymanager.h"
//#include "c_systemtime.h"
#include "WinBase.h"
#include <cctype>
#include <algorithm>
#include "c_browseslate.h"
#include <ctime>	// for UpdateSystemTimeState method

#include "vgui/IInput.h"
#include <vgui/ISurface.h>
//#include "input.h"
//#include "inputsystem/iinputsystem.h"
#include "ienginevgui.h"
#include "c_toast.h"
#include "c_arcadecrosshair.h"
#include "aarcade/client/c_earlyerror.h"

#include "materialsystem/materialsystem_config.h"
#include "dlight.h"
#include "iefx.h"
#include "../../../game/client/flashlighteffect.h"

#include "view.h"
#include "../../public/view_shared.h"

#include "../../public/bitmap/tgawriter.h"
#include "../../public/pixelwriter.h"

#include "../../public/ivrenderview.h"	// For ManageAlwaysLookObjects to get the view entity.

//#include <regex>
#include "../../sqlite/include/sqlite/sqlite3.h"
#include "../public/sourcevr//isourcevirtualreality.h"
//#include "mathlib/mathlib.h"
//#include <math.h>
#include <time.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//.\Release_mod_hl2mp\..\..\..\game\AArcade.exe

C_AnarchyManager g_AnarchyManager;
extern C_AnarchyManager* g_pAnarchyManager(&g_AnarchyManager);

C_AnarchyManager::C_AnarchyManager() : CAutoGameSystemPerFrame("C_AnarchyManager")
{
	DevMsg("AnarchyManager: Constructor\n");
	m_bInitialized = false;
	m_bPrecacheInstances = false;
	m_bBatchObjectSpawn = false;
	m_iVRSpazzFix = -1;//null;
	m_iVRSpazzFix2 = -1;// null;
	m_bVRSuspending = 0;
	m_iPlayerCount = 0;
	m_hwnd = null;
	m_bPreviousJoystickClickDown = false;
	m_fAudioPeak = 0.0f;
	m_fHueShifter = 0.0f;
	m_bIsInSourceGame = false;
	m_bLevelInitialized = false;
	m_bImagesReady = false;
	m_bImagesResetting = false;
	m_bSocialMode = false;
	m_pAutoSaveConVar = null;
	m_panoshotState = PANO_NONE;
	m_pToastMessagesKV = new KeyValues("toast");
	m_fNextToastExpiration = 0;
	m_pHoverLabel = null;
	m_iHoverEntityIndex = -1;
	m_bHoverEntityShortcut = false;
	m_hoverTitle = "";
	m_fHoverTitleExpiration = 0;
	m_pTextureClampConVar = null;
	m_pOriginalWndProc = null;
	m_pMeterInfo = null;
	m_pLastFOVConVar = null;
	m_bVRActive = false;
	m_bHandTrackingActive = false;
	pVRHandRight = null;
	pVRPointerRight = null;
	pVRHandLeft = null;
	pVRPointerLeft = null;
	pVRTeleport = null;
	m_iLastNextDirection = 0;
	m_bWasVRSnapTurn = false;
	m_bWasVRTeleport = false;
	m_bNeedVRTeleport = false;
	m_iVRSnapTurn = 0;
	m_flNextAttractCameraTime = -1;
	m_flComparisonRenderTime = -1;
	m_pInspectShortcut = null;
	m_pJoystickConVar = null;
	m_flInspectStartingScale = 1.0f;
	m_flInspectGoodScale = 1.0f;
	m_flInspectOffsetScale = 1.0f;
	m_flInspectPreviousScale = 0.0f;

	//m_pTalkerSteamBrowserInstance = null;

	m_flStartQuestsSoon = 0.0f;

	m_flSpawnObjectsButtonDownTime = 0.0f;

	m_systemTimeState.iDate = 1;
	m_systemTimeState.iDay = 0;
	m_systemTimeState.iHours = 0;
	m_systemTimeState.iMinutes = 0;
	m_systemTimeState.iMonths = 0;
	m_systemTimeState.iPostfix = 0;
	m_systemTimeState.iSeconds = 0;
	m_systemTimeState.iYears = 0;

	m_numberStatsState.iObjects = 0;
	m_numberStatsState.iGlobalPlayers = 0;
	m_numberStatsState.iGlobalTime = 0;
	m_numberStatsState.iGlobalTubes = 0;
	m_numberStatsState.iServerVisitors = 0;
	m_numberStatsState.iServerVisits = 0;
	m_numberStatsState.iLibraryItems = 0;
	m_numberStatsState.iLibraryMaps = 0;
	m_numberStatsState.iLibraryModels = 0;

	m_flNextGlobalPlayerCountUpdateTime = 0.0f;
	m_flNextGlobalStatRefreshTime = 0.0f;

	m_flNextSystemTimeUpdateTime = 0.0f;
	m_flNextNumberStatsUpdateTime = 0.0f;

	m_flLastGlobalStatFetchTime = 0.0f;

	m_pAlwaysAnimatingImagesConVar = null;
	m_iOldAlwaysAnimatingImagesValue = 0;

	//m_crashModelLogHandle = null;

	m_iSelectorTraceEntityIndex = -1;

	m_pModelThumbSizeConVar = null;
	pModelPreview = null;

	m_pPaintTextureConVar = null;

	m_pLocalAvatarObject = null;
	m_pVRSpectatorRenderTexture = null;

	m_pWaitForInitialImagesConVar = false;
	
	m_flPreviousJoystickForward = 0.0f;
	m_flPreviousJoystickSide = 0.0f;
	m_flPreviousJoystickPitch = 0.0f;
	m_flPreviousJoystickYaw = 0.0f;


	for (unsigned int i = 0; i < 10; i++)
		m_pActionBarSlotConVars[i] = null;

	m_bWaitingForInitialImagesToLoad = false;

	m_pAutoRebuildSoundCacheConVar = null;

	m_iOriginalSelect = -1;

	m_saveMode = SAVEMODE_NONE;

	//m_bVRSpectatorMode = false;
	m_bIsCreatingLocalAvatar = false;

	m_pVRSpectatorModeConVar = null;
	m_pVRSpectatorMirrorModeConVar = null;
	m_pVRHMDRenderConVar = null;
	m_pNoDrawShortcutsConVar = null;

	m_eye = ISourceVirtualReality::VREye_Left;

	m_iVRAPI = 0;

	m_pIPDConVar = null;
	m_iVRBufferWidth = 0;
	m_iVRBufferHeight = 0;
	m_pDebugInvertVRMatricesConVar = null;

	m_pPanoStuff = new panoStuff_t();

	m_fNextPanoCompleteManage = 0;
	m_fNextExtractOverviewCompleteManage = 0;
	m_fNextWindowManage = 0;

	m_flStartTime = 0;
	m_flCurrentTime = 0;
	m_flStatTime = 0;
	m_iTotalHours = 0;

	m_pConnectedUniverse = null;

	m_pLocalAutoPlaylistsConVar = null;

	//m_pAudioStripMaterial = null;

	//m_bIgnoreNextFire = false;
	m_tabMenuFile = "taskMenu.html";	// OBSOLETE!!
	m_bIsShuttingDown = false;
	m_bIsHoldingPrimaryFire = false;
	m_pHoverGlowEntity = null;
	m_pLastNearestObjectToPlayerLook = null;

	m_pVRTwoEyesHMDRenderTextures[0] = null;
	m_pVRTwoEyesHMDRenderTextures[1] = null;
	m_pVRTwoEyesHMDRenderTextures[2] = null;
	m_pModelPreviewRenderTexture = null;

	m_pAttractModeActiveConVar = null;
	m_pCabinetAttractModeActiveConVar = null;

	m_pNextBigScreenshotKV = null;

	m_bIsDisconnecting = false;

	m_pHoverTitlesConVar = null;
	m_pToastMsgsConVar = null;

	m_pShouldShowCrosshairConVar = null;

	m_pUseSBSRenderingConVar = null;

	m_iLastDynamicMultiplyer = -1;
	m_pPicMipConVar = null;
	m_pWeaponsEnabledConVar = null;

	m_pAAIManager = null;

	// FIXME: ONLY WORKS FOR ME
	//m_legacyFolder = "A:\\SteamLibrary\\steamapps\\common\\Anarchy Arcade\\aarcade\\";
	m_legacyFolder = "";

	m_state = AASTATE_NONE;
	m_iState = 0;
	m_bSuspendEmbedded = false;
	m_bIncrementState = false;
	m_bPaused = false;
	m_pCanvasManager = null;
	//m_pWebManager = null;
	//m_pLoadingManager = null;
	m_pLibretroManager = null;
	m_pSteamBrowserManager = null;
	m_pMetaverseManager = null;
	m_pBackpackManager = null;
	m_pWindowManager = null;
	m_pQuestManager = null;
	m_pInputManager = null;
	m_pSelectedEntity = null;
	m_pMountManager = null;
	m_pInstanceManager = null;
	m_dLastGenerateIdTime = 0;
	m_lastGeneratedChars = "000000000000";

	m_pFreeMouseModeConVar = null;
	m_pShouldShowWindowsTaskBarConVar = null;
	m_pRightFreeMouseToggleConVar = null;
	m_pAutoCloseTasksConVar = null;
	m_pShouldAllowMultipleActiveConVar = null;

	m_pBuildGhostConVar = null;
	m_bAutoRes = true;

	m_pNextLoadInfo = new nextLoadInfo_t();
	m_pNextLoadInfo->instanceId = "";
	m_pNextLoadInfo->position = "";
	m_pNextLoadInfo->rotation = "";

	m_pImportInfo = new importInfo_t();
	m_pImportInfo->count = 0;
	m_pImportInfo->status = AAIMPORTSTATUS_NONE;
	m_pImportInfo->type = AAIMPORT_NONE;
	m_uPreviousImportCount = 0;
	m_uLastProcessedModelIndex = 0;
	m_uValidProcessedModelCount = 0;
	m_uProcessBatchSize = 100;
	m_uProcessCurrentCycle = 0;
	
	//m_hCursor = LoadCursor(NULL, IDC_ARROW);
	m_cursor = vgui::dc_arrow;

	m_pRagdollShortcut = null;
}

C_AnarchyManager::~C_AnarchyManager()
{
	DevMsg("AnarchyManager: Destructor\n");
}

bool C_AnarchyManager::DragEnter()
{
	// only show the dropping UI if we are just in walk-around mode anyways...
	if (!this->GetInputManager()->GetInputMode())
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		//std::string encodedText = encodeURIComponent(text);
		std::string url = "asset://ui/dropping.html";//?text=" + encodedText;

		if (this->GetSelectedEntity())
			this->DeselectEntity(url);
		else
			pHudBrowserInstance->SetUrl(url);

		m_pInputManager->ActivateInputMode(true, false);
		return true;
	}
	
	return false;
}

bool C_AnarchyManager::ShouldAllowMultipleActive()
{
	if (!m_pShouldAllowMultipleActiveConVar)
		return false;

	return m_pShouldAllowMultipleActiveConVar->GetBool();
}

void C_AnarchyManager::DragLeave()
{
	// only show the dropping UI if we are just in walk-around mode anyways...
	if (this->GetInputManager()->GetInputMode())
	{
		// we're **probably** in drag mode, but do some random possible cleanup anyways??
		if (this->GetSelectedEntity())
			this->DeselectEntity();

		// Now really deactivate our dropping.html UI.
		m_pInputManager->DeactivateInputMode(true);
	}
}

void C_AnarchyManager::HandleTextDragDrop(std::string text)
{
	// try to extract a URL from it
	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	if (this->GetInputManager()->GetInputMode())
	{
		std::vector<std::string> params;
		params.push_back(text);

		pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "dropTextListener", params);
	}
	else
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		std::string encodedText = encodeURIComponent(text);
		std::string url = "asset://ui/drop.html?text=" + encodedText;

		if (this->GetSelectedEntity())
			this->DeselectEntity(url);
		else
			pHudBrowserInstance->SetUrl(url);

		m_pInputManager->ActivateInputMode(true, false);
	}
}

void C_AnarchyManager::HandleDragDrop(HDROP hDrop)
{
	bool bIsOLE = true;
	
	// get where mouse is.. if u care
	//POINT mousePt;
	//DragQueryPoint(hDrop, &mousePt);

	std::vector<std::string> files;
	// get file(s)
	int numFiles = DragQueryFile(hDrop, -1, NULL, 0);
	for (int x = 0; x < numFiles; x++)
	{
		char filename[MAX_PATH];
		DragQueryFile(hDrop, x, filename, MAX_PATH);

		files.push_back(std::string(filename));
	}

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	if (this->GetInputManager()->GetInputMode())
	{
		std::vector<std::string> params;
		for (unsigned int i = 0; i < files.size(); i++)
			params.push_back(files[i]);

		pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "dropListener", params);
	}
	else
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		std::string filenames = "";
		for (unsigned int i = 0; i < files.size(); i++)
		{
			if (i > 0)
				filenames += "::";

			filenames += files[i];
		}

		std::string url = "asset://ui/drop.html?files=" + filenames;

		if (this->GetSelectedEntity())
			this->DeselectEntity(url);
		else
			pHudBrowserInstance->SetUrl(url);

		m_pInputManager->ActivateInputMode(true, false);
	}



	// clean up
	if (!bIsOLE)
		DragFinish(hDrop);

	SetActiveWindow(m_hwnd);
	SetForegroundWindow(m_hwnd);
}

LRESULT CALLBACK HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//if (g_pAnarchyManager->IsPaused())
	//	return 0;

	WNDPROC proc = g_pAnarchyManager->GetOriginalWndProc();
	switch (uMsg)
	{
		case WM_DROPFILES:
			g_pAnarchyManager->HandleDragDrop((HDROP)wParam);
			break;

		/*case WM_MOUSEMOVE:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse move.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_NCMOUSEMOVE:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse ncmove.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_NCMOUSELEAVE:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse ncleave.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_NCMOUSEHOVER:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse nchover.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_NCHITTEST:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse nctest.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_MOUSELEAVE:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse leave.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;

		case WM_MOUSEHOVER:
			if (g_pAnarchyManager->IsPaused())
			{
				DevMsg("Ignored mouse hover.\n");
				return 0;
			}
			else
				return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
			break;*/

		/*case WM_COPYDATA:
			DevMsg("Handle copy data\n");
			break;*/

		/*case 0x0049:
			DevMsg("Handle 0x0049\n");
			break;*/

		/*case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT && g_pAnarchyManager->GetState() == AASTATE_RUN)
			{
				g_pAnarchyManager->SetSystemCursor();
				break;
			}*/

		default:
			return CallWindowProc(proc, hWnd, uMsg, wParam, lParam);	// invoke original
	}

	return 0;
}

#include "MyEdDropTarget.h"
TDropTarget m_dropTarget;
//#include "aarcade\client\MyEdDropTarget.h"
//CMyEdDropTarget gcMyEdDropTarget;  // make a global instance
void C_AnarchyManager::InitDragDrop(HWND hWnd)
{
	// add ACCEPTFILES style
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES | GetWindowLongPtr(hWnd, GWL_EXSTYLE));

	//TDropTarget* pDropTarget = new TDropTarget;
	RegisterDragDrop(hWnd, &m_dropTarget);

	// snag existing wndproc
	m_pOriginalWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWL_WNDPROC);

	// supplant with our own
	//SetWindowLongPtr(hWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(HookWndProc));	// disabled in favor ot eh OLE style drag & drop.

	/*ChangeWindowMessageFilterEx(hWnd, WM_DROPFILES, MSGFLT_ALLOW, 0);
	ChangeWindowMessageFilterEx(hWnd, WM_COPYDATA, MSGFLT_ALLOW, 0);
	ChangeWindowMessageFilterEx(hWnd, 0x0049, MSGFLT_ALLOW, 0);*/

	//ChangeWindowMessageFilterEx(hWnd, WM_COPYDATA, MSGFLT_ALLOW, 0);
		/*
		HWND                hwnd,
		UINT                message,
		DWORD               action,
		PCHANGEFILTERSTRUCT pChangeFilterStruct
		);*/

	//CWnd* pWnd = CWnd::FromHandle(hWnd);
	//gcMyEdDropTarget.Register(pWnd);
}

#include "aa_apikeys.h"
void C_AnarchyManager::Tester()
{
	HTTPRequestHandle requestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.thetvdb.com/login");
	steamapicontext->SteamHTTP()->SetHTTPRequestHeaderValue(requestHandle, "content-type", "application/json");
	std::string body = VarArgs("{\"apikey\": \"%s\"}", AA_TVDB_API_KEY);
	char* bodyData = new char(body.length()+1);
	Q_strncpy(bodyData, body.c_str(), body.length()+1);

	steamapicontext->SteamHTTP()->SetHTTPRequestRawPostBody(requestHandle, "application/json", (uint8*)bodyData, body.length() * sizeof(uint8));

	SteamAPICall_t hAPICall;
	steamapicontext->SteamHTTP()->SendHTTPRequest(requestHandle, &hAPICall);
	m_HTTPResponseCallback.Set(hAPICall, this, &C_AnarchyManager::HTTPResponse);
}

void C_AnarchyManager::CaptureWindowSnapshotsAll()
{
	// get all the Windows tasks
	std::vector<C_EmbeddedInstance*> windowsEmbeddedInstances;
	g_pAnarchyManager->GetWindowManager()->GetAllInstances(windowsEmbeddedInstances);

	// ignore special instances
	bool bHiddenTask;
	bool bPresetHiddenTask;

	bool bIsDisplayTask = false;
	bool bIsFirstTaskToDisplay = false;
	bool bIsWindowsTask = true;
	C_EmbeddedInstance* pEmbeddedInstance;
	C_WindowInstance* pWindowInstance;
	unsigned int size = windowsEmbeddedInstances.size();
	for (unsigned int i = 0; i < size; i++)
	{
		pEmbeddedInstance = windowsEmbeddedInstances[i];
		if (!pEmbeddedInstance)
			continue;

		pWindowInstance = dynamic_cast<C_WindowInstance*>(pEmbeddedInstance);
		if (pWindowInstance->IsHidden() || pWindowInstance->IsPresetHidden())
			continue;

		pWindowInstance->Render();

		//if (this->CaptureWindowSnapshot(pWindowInstance->GetHWND()))
		//	break;
	}
}

BYTE* C_AnarchyManager::CaptureWindowSnapshot(HWND hwnd, int& iWidth, int& iHeight)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	//create
	HDC hdcScreen = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen,
		rc.right - rc.left, rc.bottom - rc.top);
	SelectObject(hdc, hbmp);

	//Print to memory hdc
	PrintWindow(hwnd, hdc, PW_CLIENTONLY);

	/*
	//copy to clipboard
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP, hbmp);
	CloseClipboard();
	*/

	//HDC hdcSource = NULL; // the source device context
	//HBITMAP hSource = NULL; // the bitmap selected into the device context

	BITMAPINFO MyBMInfo = { 0 };
	MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader);

	// Get the BITMAPINFO structure from the bitmap
	if (0 == GetDIBits(hdcScreen, hbmp, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS))
	{
		// error handling
		DevMsg("ERROR: Failed to get DIBits.\n");
		return NULL;
	}

	//unsigned long
	//DevMsg("Image size: %lu\n", MyBMInfo.bmiHeader.biSizeImage);

	// create the pixel buffer
	BYTE* lpPixels = new BYTE[MyBMInfo.bmiHeader.biSizeImage];

	// We'll change the received BITMAPINFOHEADER to request the data in a
	// 32 bit RGB format (and not upside-down) so that we can iterate over
	// the pixels easily. 

	// requesting a 32 bit image means that no stride/padding will be necessary,
	// although it always contains an (possibly unused) alpha channel
	MyBMInfo.bmiHeader.biBitCount = 32;
	MyBMInfo.bmiHeader.biCompression = BI_RGB;  // no compression -> easier to use
	// correct the bottom-up ordering of lines (abs is in cstdblib and stdlib.h)
	MyBMInfo.bmiHeader.biHeight = abs(MyBMInfo.bmiHeader.biHeight);

	// Call GetDIBits a second time, this time to (format and) store the actual
	// bitmap data (the "pixels") in the buffer lpPixels
	int iLines = GetDIBits(hdcScreen, hbmp, 0, MyBMInfo.bmiHeader.biHeight, lpPixels, &MyBMInfo, DIB_RGB_COLORS);
	if ( iLines == 0 )
	{
		// error handling
		DevMsg("ERROR: Failed to copy DIBits.\n");
		return false;
	}
	// clean up: deselect bitmap from device context, close handles, delete buffer
	//DevMsg("Lines copieid: %i\n", iLines);

	iWidth = MyBMInfo.bmiHeader.biWidth;
	iHeight = MyBMInfo.bmiHeader.biHeight;

	//delete[] lpPixels;	// Don't forget to clean this up in the caller's code!



	//release
	DeleteDC(hdc);
	DeleteObject(hbmp);
	ReleaseDC(NULL, hdcScreen);
	return lpPixels;
}

void C_AnarchyManager::RequestGlobalStatsResponse(GlobalStatsReceived_t *pResult, bool bIOFailure)
{
	if (pResult->m_eResult != k_EResultOK)
	{
		DevMsg("Could not fetch global stats.\n");
	}

	if (!this->IsInitialized())
	{
		m_pAccountant->StoreStats();
		g_pAnarchyManager->IncrementState();
	}

	m_flLastGlobalStatFetchTime = engine->Time();

	//else
	//m_iPlayerCount = pResult->m_cPlayers;
}

void C_AnarchyManager::GetSteamPlayerCountResponse(NumberOfCurrentPlayers_t *pResult, bool bIOFailure)
{
	if (!pResult->m_bSuccess)
		return;

	m_iPlayerCount = pResult->m_cPlayers;
}

void C_AnarchyManager::GetSteamPlayerCount()
{
	SteamAPICall_t hAPICall = steamapicontext->SteamUserStats()->GetNumberOfCurrentPlayers();
	m_GetNumberOfPlayersHelper.Set(hAPICall, this, &C_AnarchyManager::GetSteamPlayerCountResponse);
}

void C_AnarchyManager::HTTPResponse(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	void* pBuf = malloc(pResult->m_unBodySize);
	steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize);

	std::string dataString = VarArgs("%s", (unsigned char*)pBuf);
	dataString[dataString.length() - 1] = '\0';
	free(pBuf);
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
	//DevMsg("%s\n", dataString.c_str());
}

bool C_AnarchyManager::Init()
{
	DevMsg("AnarchyManager: Init\n");
	steamapicontext->SteamFriends()->SetRichPresence("steam_display", "#Status_Loading");
	steamapicontext->SteamFriends()->SetRichPresence("objectcount", "0");
	steamapicontext->SteamFriends()->SetRichPresence("mapname", "None");
	//steamapicontext->SteamUserStats()->RequestCurrentStats();
	ToastSlate->Create(enginevgui->GetPanel(PANEL_ROOT));
	//ToastSlate->GetPanel()->MoveToFront();

	/*
	CSysModule* pModule = Sys_LoadModule(info->core.c_str());
	if (!pModule)
	{
		DevMsg("Failed to load %s\n", info->core.c_str());
		// FIXME FIX ME Probably need to clean up!
		return 0;
	}

	HMODULE	hModule = reinterpret_cast<HMODULE>(pModule);
	if (!C_LibretroInstance::BuildInterface(info->raw, &hModule))
	{
		DevMsg("libretro: Failed to build interface!\n");
		return 0;
	}
	*/

	m_pSourceEngineGamesKV = new KeyValues("sourceenginegames");
	m_pSourceEngineGamesKV->LoadFromFile(g_pFullFileSystem, "sourcegames.txt", "MOD");

	KeyValues* legacyLog = new KeyValues("legacy");
	if (legacyLog->LoadFromFile(g_pFullFileSystem, "legacy_log.key", "MOD"))
	{
		m_legacyFolder = legacyLog->GetString("path");

		size_t found = m_legacyFolder.find("\\\\");
		while (found != std::string::npos)
		{
			m_legacyFolder.replace(found, 2, "\\");
			found = m_legacyFolder.find("\\\\");
		}
	}
	legacyLog->deleteThis();

	std::string workshopDir = engine->GetGameDirectory();	// just use the game directory to find workshop content normally.
	if (workshopDir == "d:\\projects\\aarcade-source\\game\\frontend")
		workshopDir = m_legacyFolder;

	workshopDir = workshopDir.substr(0, workshopDir.find_last_of("\\"));
	workshopDir = workshopDir.substr(0, workshopDir.find_last_of("\\"));
	workshopDir = workshopDir.substr(0, workshopDir.find_last_of("\\"));
	workshopDir = workshopDir.substr(0, workshopDir.find_last_of("\\"));
	workshopDir += "\\workshop\\content\\266430\\";
	m_workshopFolder = workshopDir;

	std::string aarcadeUserFolder = engine->GetGameDirectory();
	size_t found = aarcadeUserFolder.find_last_of("/\\");
	aarcadeUserFolder = aarcadeUserFolder.substr(0, found + 1);

	std::string aarcadeToolsFolder = aarcadeUserFolder;
	aarcadeToolsFolder += std::string("bin");
	m_aarcadeToolsFolder = aarcadeToolsFolder;

	aarcadeUserFolder += std::string("aarcade_user");
	m_aarcadeUserFolder = aarcadeUserFolder;

	g_pFullFileSystem->CreateDirHierarchy("resource\\ui\\html", "DEFAULT_WRITE_PATH");


	FileHandle_t fh = filesystem->Open("modelload.log", "r", "DEFAULT_WRITE_PATH");
	if (fh)
	{
		int file_len = filesystem->Size(fh);
		char* modelName = new char[file_len + 1];

		filesystem->Read((void*)modelName, file_len, fh);
		modelName[file_len] = 0; // null terminator

		filesystem->Close(fh);

		// Use GameInfo here...
		failedModelName = modelName;

		delete[] modelName;
	}

	return true;
}

void C_AnarchyManager::PostInit()
{
	DevMsg("AnarchyManager: PostInit\n");

	///*
	// allow for 2 instances of the Source engine to run
	// NOTE: There are conflicts with older Source engine games that use the default VGUI main menu, but other than that they work fine as well.
	bool keepGoing;

	// Disable mutexes
	keepGoing = true;
	while (keepGoing)
	{
		HANDLE myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "hl2_singleton_mutex");

		if (myHandle)
		{
			if (!ReleaseMutex(myHandle))
				myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "hl2_singleton_mutex");
			else
			{
				CloseHandle(myHandle);
				keepGoing = false;
			}
		}
		else
			keepGoing = false;
	}

	keepGoing = true;
	while (keepGoing)
	{
		HANDLE myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "ValvePlatformUIMutex");

		if (myHandle)
		{
			if (!ReleaseMutex(myHandle))
				myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "ValvePlatformUIMutex");
			else
			{
				CloseHandle(myHandle);
				keepGoing = false;
			}
		}
		else
			keepGoing = false;
	}


	//	keepGoing = true;
	//	while (keepGoing)
	{
		HANDLE myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "ValvePlatformWaitMutex");

		//		if (!ReleaseMutex(myHandle))
		//			myHandle = OpenMutex(MUTEX_ALL_ACCESS, true, "ValvePlatformWaitMutex");
		//		else
		//		{

		if (myHandle)
		{
			ReleaseMutex(myHandle);
			CloseHandle(myHandle);
		}
		//			keepGoing = false;
		//		}
	}
	//*/
}

void C_AnarchyManager::FetchGlobalStats()
{
	float flTime = engine->Time();
	if (m_flLastGlobalStatFetchTime != 0.0f && flTime - m_flLastGlobalStatFetchTime < 120.0f)
		return;

	m_flLastGlobalStatFetchTime = INFINITY;

	// if our local stats have changed since the last time we fetched them

	SteamAPICall_t hAPICall = steamapicontext->SteamUserStats()->RequestGlobalStats(2);	// we ain't really ready yet, but global stats aren't **too** important
	m_RequestGlobalStatsHelper.Set(hAPICall, this, &C_AnarchyManager::RequestGlobalStatsResponse);
}

void C_AnarchyManager::ActivateInspectObject(C_PropShortcutEntity* pShortcut) {
	if (!pShortcut) {
		return;
	}

	if (m_pInspectShortcut) {
		return;
	}

	m_pInspectShortcut = pShortcut;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	m_inspectStartingAngles = m_pInspectShortcut->GetAbsAngles();
	m_inspectStartingOrigin = m_pInspectShortcut->GetAbsOrigin();

	//object_t* pObject = m_pInstanceManager->GetInstanceObject(m_pInspectShortcut->GetObjectId());
	m_flInspectStartingScale = m_pInspectShortcut->GetModelScale();//pObject->scale

	//float goodScale = pEntity->GetModelScale() + 0.1f * iDelta;

	DevMsg("starging scale: %f\n", m_flInspectStartingScale);
	
	float flOffsetMultiplyer = 80.0;
	//float flBoundingRadius = m_pInspectShortcut->BoundingRadius();
	Vector size = m_pInspectShortcut->CollisionProp()->OBBSize();
	float flMax = size.x;
	if (size.y > flMax) {
		flMax = size.y;
	}
	if (size.z > flMax) {
		flMax = size.z;
	}

	float flBoundingRadius = flMax / 2.0;
	DevMsg("Bounding racius: %02f\n", flBoundingRadius);

	Vector forward = MainViewForward();
	//Vector right = MainViewRight();
	//pPlayer->EyeVectors(&forward);
	//Vector origin = pPlayer->EyePosition();	// start at the eye position
	//Vector origin = MainViewOrigin();
	//origin += forward * (24.0f + flBoundingRadius); // add in the default offset, for this model.
	//m_inspectGoodOrigin = origin;
	m_flInspectGoodOriginDist = 24.0f;// + flBoundingRadius;
	//float flScale = m_flInspectStartingScale;

	float flDesiredRadius = 24.0f;
	m_flInspectGoodScale = (flDesiredRadius / (flBoundingRadius / m_flInspectStartingScale)) * m_flInspectStartingScale;// flBoundingRadius;// (flBoundingRadius / m_flInspectStartingScale);


	Vector objectToView;
	VectorSubtract(MainViewOrigin(), Vector(m_inspectStartingOrigin), objectToView);

	float flYaw = UTIL_VecToYaw(objectToView);
	float flPitch = UTIL_VecToPitch(objectToView);
	m_inspectGoodAngles = QAngle(flPitch, flYaw, 0);
	
	//m_inspectGoodAngles = MainViewAngles();
	//m_inspectGoodAngles += QAngle(0, 180, 0);

	m_flInspectPreviousScale = m_flInspectGoodScale;

	// set the initial good transform
	Vector goodOrigin = MainViewForward();
	goodOrigin *= m_flInspectGoodOriginDist;
	goodOrigin += MainViewOrigin();

	/*
	//Vector max = m_pInspectShortcut->WorldAlignMaxs();
	Vector centerSpot = m_pInspectShortcut->CollisionProp()->OBBCenter();// m_pInspectShortcut->WorldSpaceCenter();
	DevMsg("Center Spot: %f %f %f\n", centerSpot.x, centerSpot.y, centerSpot.z);
	Vector centerOffset = centerSpot;// centerSpot - m_pInspectShortcut->GetAbsOrigin();

	DevMsg("Offset Spot: %f %f %f\n", centerOffset.x, centerOffset.y, centerOffset.z);
	DevMsg("Good Scale: %f\n", m_flInspectGoodScale);

	goodOrigin -= centerOffset * m_flInspectGoodScale;
	*/

	//m_pInspectShortcut->VPhysicsGetObject()->GetMassCenterLocalSpace()

	if (m_flInspectGoodScale != m_flInspectStartingScale) {
		engine->ClientCmd(VarArgs("setscale %i %f;\n", m_pInspectShortcut->entindex(), m_flInspectGoodScale));
	}
	engine->ClientCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", m_pInspectShortcut->entindex(), goodOrigin.x, goodOrigin.y, goodOrigin.z, m_inspectGoodAngles.x, m_inspectGoodAngles.y, m_inspectGoodAngles.z));
	engine->ClientCmd(VarArgs("makeghost %i %i;\n", m_pInspectShortcut->entindex(), false));

	//origin += forward * m_flPreviousJoystickForward * flOffsetMultiplyer; // add in the forward offset

	//engine->ClientCmd(VarArgs("setparent %i %i;\n", pShortcut->entindex(), pPlayer->entindex()));

	// Show some type of 2D menu as well...
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/inspectObject.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}

void C_AnarchyManager::DeactivateInspectObject() {
	if (m_pInspectShortcut) {
		//engine->ClientCmd(VarArgs("setparent %i;\n", m_pInspectShortcut->entindex()));

		Vector origin = m_inspectStartingOrigin;
		QAngle angles = m_inspectStartingAngles;
		// NOTE: This didn't work from the UI - most likely because the cmd was coming BETWEEN engine updates?? But changing it to a server cmd fixed it.
		//engine->ClientCmd(VarArgs("snap_object_pos %i %02f %02f %02f %02f %02f %02f;\n", m_pInspectShortcut->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));

		engine->ServerCmd(VarArgs("setscale %i %f\n", m_pInspectShortcut->entindex(), m_flInspectStartingScale));
		engine->ServerCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", m_pInspectShortcut->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
		engine->ServerCmd(VarArgs("makenonghost %i %i;\n", m_pInspectShortcut->entindex(), false));
		//engine->ClientCmd(VarArgs("makenonghost %i %i;\n", m_pInspectShortcut->entindex(), false));
		//engine->ClientCmd_Unrestricted(VarArgs("setscale %i %f\n", m_pInspectShortcut->entindex(), m_flInspectStartingScale));
		//engine->ClientCmd_Unrestricted(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", m_pInspectShortcut->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
	}

	m_pInspectShortcut = null;
	
	/*m_inspectGoodAngles.x = 0.0f;
	m_inspectGoodAngles.y = 0.0f;
	m_inspectGoodAngles.z = 0.0f;
	m_inspectGoodOrigin.x = 0.0f;
	m_inspectGoodOrigin.y = 0.0f;
	m_inspectGoodOrigin.z = 0.0f;
	m_flInspectGoodScale = 1.0f;
	m_flInspectGoodOriginDist = 0.0f;*/

	m_inspectOffsetAngles.x = 0.0f;
	m_inspectOffsetAngles.y = 0.0f;
	m_inspectOffsetAngles.z = 0.0f;
	m_inspectOffsetOrigin.x = 0.0f;
	m_inspectOffsetOrigin.y = 0.0f;
	m_inspectOffsetOrigin.z = 0.0f;
	m_flInspectOffsetScale = 1.0f;
	m_flInspectPreviousScale = 1.0f;
}

void C_AnarchyManager::InspectModeTick(float flFrameTime) {
	//m_pInputManager->GetInputCapture()
	//if (!m_pInputManager->IsGamepadInputMode()) {
	//	return;
	//}

	if (!m_pInspectShortcut) {
		return;
	}

	if (!m_pJoystickConVar) {
		m_pJoystickConVar = cvar->FindVar("joystick");
	}

	int iJoystick = m_pJoystickConVar->GetInt();

	/*
	if (iJoystick > 0) {
		//vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER)
		//vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER)
		int iVal = inputsystem->GetAnalogValue((AnalogCode_t)JOYSTICK_AXIS(iJoystick, KEY_XSTICK1_RIGHT));
		int iVal = inputsystem->GetAnalogValue();
		DevMsg("Value: %i\n", iVal);
	}*/

	if (iJoystick > 0) {
		//g_pAnarchyManager->UpdateGamepadAxisInput(m_flPreviousJoystickForward, m_flPreviousJoystickSide, m_flPreviousJoystickPitch, m_flPreviousJoystickYaw);
		//DevMsg("Value: (%02f, %02f)\n", m_flPreviousJoystickYaw, m_flPreviousJoystickPitch);

		//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		Vector forward = MainViewForward();
		Vector right = MainViewRight();
		Vector up = MainViewUp();
		//Vector forward, right, up;
		//pPlayer->EyeVectors(&forward, &right, &up);
		//Vector eyePosition = MainViewOrigin();//pPlayer->EyePosition();

		float flOffsetMultiplyer = 150.0;
		//m_inspectOffsetOrigin += forward * (m_flPreviousJoystickForward * flOffsetMultiplyer * flFrameTime); // add in the forward offset
		//m_inspectOffsetOrigin += right * (m_flPreviousJoystickSide * flOffsetMultiplyer * flFrameTime); // add in the sideways offset
		m_inspectOffsetOrigin.y += m_flPreviousJoystickForward * flOffsetMultiplyer * flFrameTime; // add in the forward offset
		m_inspectOffsetOrigin.x += m_flPreviousJoystickSide * flOffsetMultiplyer * flFrameTime; // add in the sideways offset

		float flTriggerAxis = 0.0f;
		if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER)) {
			flTriggerAxis = -1.0f;
		}
		else if (vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER)) {
			flTriggerAxis = 1.0f;
		}

		m_inspectOffsetOrigin.z += flTriggerAxis * flOffsetMultiplyer * 0.4 * flFrameTime; // add in the sideways offset

		Vector origin = MainViewForward();
		origin *= m_flInspectGoodOriginDist;
		origin += MainViewOrigin();
		//Vector origin = m_inspectGoodOrigin;// m_pInspectShortcut->GetAbsOrigin();
		//origin += m_inspectOffsetOrigin;
		origin += forward * m_inspectOffsetOrigin.y;
		origin += right * m_inspectOffsetOrigin.x;
		origin += up * m_inspectOffsetOrigin.z;



		//Vector max = m_pInspectShortcut->WorldAlignMaxs();
		//Vector centerSpot = m_pInspectShortcut->CollisionProp()->OBBCenter();// m_pInspectShortcut->WorldSpaceCenter();
		//m_pInspectShortcut->VPhysicsGetObject()->GetMassCenterLocalSpace()
		//DevMsg("Center Spot: %f %f %f\n", centerSpot.x, centerSpot.y, centerSpot.z);
		//Vector centerOffset = centerSpot;// centerSpot - m_pInspectShortcut->GetAbsOrigin();

		//DevMsg("Offset Spot: %f %f %f\n", centerOffset.x, centerOffset.y, centerOffset.z);
		//DevMsg("Good Scale: %f\n", m_flInspectGoodScale);

		////origin -= centerSpot;
		////origin -= right * centerSpot.x;
		////origin += forward * centerSpot.y;
		//origin -= up * centerSpot.z;





		//QAngle angles = m_inspectStartingAngle;//pPlayer->GetAbsAngles();
		//angles += QAngle(180 * m_flPreviousJoystickPitch, 180 * m_flPreviousJoystickYaw, 0);

		float flAngleMultiplyer = 150.0f;
		m_inspectOffsetAngles += QAngle(m_flPreviousJoystickPitch * flAngleMultiplyer * flFrameTime, m_flPreviousJoystickYaw * flAngleMultiplyer * flFrameTime, 0);

		QAngle angles = m_inspectGoodAngles;//m_pInspectShortcut->GetAbsAngles();
		angles += m_inspectOffsetAngles;

		engine->ClientCmd(VarArgs("snap_object_pos %i %02f %02f %02f %02f %02f %02f;\n", m_pInspectShortcut->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
	}

	//snap_object_pos
	//m_pInspectShortcut-

	//if (fabs(m_flPreviousJoystickSide) > 0.1 || fabs(m_flPreviousJoystickForward) > 0.1)
}

void C_AnarchyManager::StartQuestsSoon()
{
	m_pQuestManager->SetQuestsInitializing(true);
	m_flStartQuestsSoon = engine->Time() + 3.0f;
}

void C_AnarchyManager::OnAccountantReady()
{
	this->FetchGlobalStats();	// The 1st time this hits its callback, AArcade's internal startup state is incremented.  So it is an important part of the startup process.
	//SteamAPICall_t hAPICall = steamapicontext->SteamUserStats()->RequestGlobalStats(2);	// we ain't really ready yet, but global stats aren't **too** important
	//m_RequestGlobalStatsHelper.Set(hAPICall, this, &C_AnarchyManager::RequestGlobalStatsResponse);

	// this is done in the callback now!
	//m_pAccountant->StoreStats();
	//g_pAnarchyManager->IncrementState();
}

void C_AnarchyManager::OnStartup()
{
	engine->ClientCmd("bind f4 task_menu; bind f8 players_menu;");	// TEMPORARY. You can delete this line on June 2019.

#ifdef VR_ALLOWED
	engine->ClientCmd("mat_queue_mode 0;");
#else
	engine->ClientCmd("mat_queue_mode 2;");
#endif

	this->GetSteamPlayerCount();	// get a player count ready for the 1st main menu.

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->Select();
	pHudBrowserInstance->Focus();
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);

	/*KeyValues* pMaterialKV = new KeyValues("VertexLitGeneric");
	pMaterialKV->SetString("$basetexture", "vgui/canvas");
	IMaterial* pMaterial = materials->CreateMaterial("projectorview", pMaterialKV);
	DevMsg("Projector Material Name: %s\n", pMaterial->GetName());*/
}

/*const char* C_AnarchyManager::GetProjectorMaterialName()
{
	return "projectorview";
}*/

void C_AnarchyManager::OnUpdateLibraryVersionCallback()
{
	int iLibraryVersion = m_pMetaverseManager->ExtractLibraryVersion();
	if (iLibraryVersion == -1)
		iLibraryVersion = 0;

	if (iLibraryVersion < AA_LIBRARY_VERSION)
	{
		DevMsg("Making a backup then converting your library from version %i to %i\n", iLibraryVersion, (iLibraryVersion + 1));
		//unsigned int uSafeLibraryVersion = (iLibraryVersion < 0) ? 0 : iLibraryVersion;
		if (m_pMetaverseManager->ConvertLibraryVersion(iLibraryVersion, iLibraryVersion + 1))
		{
			DevMsg("Done updating library.  Backup copy placed in the backups folder just in case.\n");
			//this->OnReadyToLoadUserLibrary();
		}
		else
		{
			DevMsg("CRITICAL ERROR: Could not convert library version! Aborting! You should make a copy of the newest backup in aarcade_user/backups and replace the aarcade_user/library.db file with the backup and try again. If it still fails, let me know on during a Twitch stream and I'll help figure things out with you! - SM Sith Lord\n");
			engine->ClientCmd("quit");
			return;
		}
	}

	this->OnStartupCallback(true);
}

void C_AnarchyManager::OnStartupCallbackB()
{
	bool bAddDefaultLibrary = false;
	KeyValues* pLastVersionKV = new KeyValues("version");

	if (!pLastVersionKV->LoadFromFile(g_pFullFileSystem, "version.txt", "DEFAULT_WRITE_PATH"))
	{
		// reset the KV, just in case.
		pLastVersionKV->deleteThis();
		pLastVersionKV = new KeyValues("version");
	}

	//{
		float flValA = pLastVersionKV->GetFloat("version");
		float flValB = AA_CLIENT_VERSION;
		flValA += 1;
		flValA -= 1;
		flValB += 1;
		flValB -= 1;

		DevMsg("Version comparison: %f vs %f\n", flValA, flValB);
		if (flValA < flValB)
		{
			bAddDefaultLibrary = true;
			pLastVersionKV->SetFloat("version", AA_CLIENT_VERSION);
			pLastVersionKV->SaveToFile(g_pFullFileSystem, "version.txt", "DEFAULT_WRITE_PATH");

			if ( flValB - flValA > 1 )	// Only exec the default config if the user has missed more than 1 update - in which case they might be missing a large amount of button binds.
				engine->ClientCmd("exec config_default_redux\n");
		}
	//}
	//else
	//	DevMsg("WARNING: Failed to load any version.txt for version info.\n");
	
	pLastVersionKV->deleteThis();
	this->OnStartupCallback(bAddDefaultLibrary);
}

void C_AnarchyManager::OnStartupCallback(bool bForceDefaultAdd)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	
	// Now start loading stuff in... (if this is our 1st time here...)
	m_pMetaverseManager->Init();

	// check if library version is obsolete
	int iLibraryVersion = m_pMetaverseManager->ExtractLibraryVersion();
	//if (iLibraryVersion == -1)
		//iLibraryVersion = 0;

	//DevMsg("Library version initially detected as: %i (default: %i)\n", iLibraryVersion, AA_LIBRARY_VERSION);
	//DevMsg("CRITICAL ERROR: Could not extract library version from library.db!\n");

	if (iLibraryVersion >= 0 && iLibraryVersion < AA_LIBRARY_VERSION)
	{
		DevMsg("Backing Up & Updating Your Library\n");
		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Backing up & updating your library...", "userlibrary", "0", "0", "5", "defaultLibraryReadyCallback");
		pHudBrowserInstance->AddHudLoadingMessage("", "", "Backing Up & Updating Your Library", "updatelibraryversion", "", "", "", "updateLibraryVersionCallback");
		return;
	}

	bool bNeedsDefault = m_pMetaverseManager->IsEmptyDb();
	if (bNeedsDefault)
	{
		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding Default Library", "defaultlibrary", "", "4", "+", "addNextDefaultLibraryCallback");

		// the current DB is already created, but empty.
		// we just need to add stuff to it.
		m_pMetaverseManager->AddDefaultTables();
	}

	m_pMetaverseManager->BeginTransaction();
	if (bNeedsDefault || iLibraryVersion < AA_LIBRARY_VERSION || bForceDefaultAdd)
	{	
		//this->OnAddNextDefaultLibraryCallback();
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Apps", "defaultapps", "", "", "+0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Cabinets", "defaultcabinets", "", "", "+0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Maps", "defaultmaps", "", "", "+0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Models", "defaultmodels", "", "", "+0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Types", "defaulttypes", "", "", "+0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding Default Library", "defaultlibrary", "", "5", "+0", "addNextDefaultLibraryCallback");
	}
	else
	{
		/*
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Types", "usertypes", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Cabinets", "usercabinets", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Models", "usermodels", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Apps", "userapps", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Items", "useritems", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Instances", "userinstances", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading User Library", "userlibrary", "0", "0", "5", "defaultLibraryReadyCallback");
		*/

		this->OnDefaultLibraryReady();
	}
}

void C_AnarchyManager::OnAddNextDefaultLibraryCallback()
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	addDefaultLibraryContext_t* context = m_pMetaverseManager->GetAddDefaultLibraryContext();

	if (!context)
	{
		context = new addDefaultLibraryContext_t();
		context->pDb = null;
		context->numApps = 0;
		context->numCabinets = 0;
		context->numMaps = 0;
		context->numModels = 0;
		context->numTypes = 0;
		context->kv = null;
		context->state = 0;

		m_pMetaverseManager->SetAddDefaultLibraryToDbIterativeContext(context);
	}

	// remember which value gets incremeneted
	unsigned int oldState = context->state;
	unsigned int oldNumApps = context->numApps;
	unsigned int oldNumCabinets = context->numCabinets;
	unsigned int oldNumMaps = context->numMaps;
	unsigned int oldNumModels = context->numModels;
	unsigned int oldNumTypes = context->numTypes;

	m_pMetaverseManager->AddDefaultLibraryToDbIterative(context);

	std::string callbackName = (context->state == 1) ? "" : "addNextDefaultLibraryCallback";

	if ( oldNumApps != context->numApps )
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Apps", "defaultapps", "", "", "+", callbackName);
	else if (oldNumCabinets != context->numCabinets)
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Cabinets", "defaultcabinets", "", "", "+", callbackName);
	else if (oldNumMaps != context->numMaps)
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Maps", "defaultmaps", "", "", "+", callbackName);
	else if (oldNumModels != context->numModels)
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Models", "defaultmodels", "", "", "+", callbackName);
	else if (oldNumTypes != context->numTypes)
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Default Types", "defaulttypes", "", "", "+", callbackName);

	// for now...
	std::string curState = "0";
	if (oldState != context->state)
	{
		if (context->state == 3)
			curState = "1";
		else if (context->state == 4)
			curState = "2";
		else if (context->state == 5)
			curState = "3";
		else if (context->state == 6)
			curState = "4";
		else if (context->state == 1)
			curState = "5";

		if (context->state != 1)
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding Default Library", "defaultlibrary", "", "5", curState);
	}

	if (context->state == 1)
	{
		m_pMetaverseManager->DeleteAddDefaultLibraryContext(context);
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding Default Library", "defaultlibrary", "", "5", "5", "defaultLibraryReadyCallback");
		//this->OnDefaultLibraryReady();
	}
}

void C_AnarchyManager::OnReadyToLoadUserLibrary()
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	unsigned int uCount;
	std::string num;


	// TODO: All of these should be incremental (per-group)
	// And continue starting up
	uCount = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalTypes();
	num = VarArgs("%u", uCount);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Types", "usertypes", "", "", num);

	unsigned int uNumDynamic = 0;
	uCount = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalModels(uNumDynamic);
	num = VarArgs("%u", uNumDynamic);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Cabinets", "usercabinets", "", "", num);
	num = VarArgs("%u", uCount - uNumDynamic);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Models", "usermodels", "", "", num);

	DevMsg("Done loading local models.\n");

	uCount = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalApps();
	num = VarArgs("%u", uCount);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Apps", "userapps", "", "", num);

	uCount = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalItems();
	num = VarArgs("%u", uCount);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Items", "useritems", "", "", num);

	uCount = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalInstances();
	num = VarArgs("%u", uCount);
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Instances", "userinstances", "", "", num);

	//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading User Library", "userlibrary", "0", "6", "6");

	DevMsg("Done loading instances.  Loading legacy stuff next probably: %s\n", m_legacyFolder.c_str());

	// time to add in some search paths
	if (m_legacyFolder != "")
	{
		DevMsg("Loading in Legacy content from: %s\n", m_legacyFolder.c_str());

		g_pFullFileSystem->AddSearchPath(m_legacyFolder.c_str(), "MOD", PATH_ADD_TO_TAIL);
		g_pFullFileSystem->AddSearchPath(m_legacyFolder.c_str(), "GAME", PATH_ADD_TO_TAIL);

		std::string legacyDownloadFolder = m_legacyFolder + std::string("download\\");
		g_pFullFileSystem->AddSearchPath(legacyDownloadFolder.c_str(), "GAME", PATH_ADD_TO_TAIL);

		DevMsg("Mounting legacy download path: %s\n", legacyDownloadFolder.c_str());

		std::string workshopFile;
		FileFindHandle_t findHandle;
		// No longer mount legacy workshop stuff directly from the legacy workshop folder, as those are mounted directly by Redux now.
		/*
		std::string workshopMapsPath = m_legacyFolder + std::string("workshop\\workshopmaps\\");
		g_pFullFileSystem->AddSearchPath(workshopMapsPath.c_str(), "GAME", PATH_ADD_TO_TAIL);

		DevMsg("Mounting legacy workshopmaps path: %s\n", workshopMapsPath.c_str());

		const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%sworkshop\\*", m_legacyFolder.c_str()), "", &findHandle);
		while (pFilename != NULL)
		{
			workshopFile = m_legacyFolder + std::string("workshop\\") + std::string(pFilename);

			if (workshopFile.find(".vpk") == workshopFile.length() - 4)
			{
				DevMsg("Adding %s to the search paths.\n", workshopFile.c_str());
				g_pFullFileSystem->AddSearchPath(workshopFile.c_str(), "GAME", PATH_ADD_TO_TAIL);
			}

			pFilename = g_pFullFileSystem->FindNext(findHandle);
		}
		g_pFullFileSystem->FindClose(findHandle);
		*/

		std::string customFolder;
		const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%scustom\\*", m_legacyFolder.c_str()), "", &findHandle);
		while (pFilename != NULL)
		{
			if (Q_strcmp(pFilename, ".") && Q_strcmp(pFilename, "..") && g_pFullFileSystem->FindIsDirectory(findHandle))
			{
				customFolder = m_legacyFolder + std::string("custom\\") + std::string(pFilename);
				DevMsg("Adding %s to the search paths.\n", customFolder.c_str());
				g_pFullFileSystem->AddSearchPath(customFolder.c_str(), "GAME", PATH_ADD_TO_TAIL);
			}

			pFilename = g_pFullFileSystem->FindNext(findHandle);
		}
		g_pFullFileSystem->FindClose(findHandle);
	}

	DevMsg("Done loading user library.\n");

	// Create the mount manager & initialize it
	m_pMountManager = new C_MountManager();
	m_pMountManager->Init();
	m_pMountManager->LoadMountsFromKeyValues("mounts.txt");

	// Initialize the backpack manager (it already exists)
	//m_pBackpackManager->Init();

	// Create the workshop manager & initialize it
	m_pWorkshopManager = new C_WorkshopManager();
	m_pWorkshopManager->Init();
}

void C_AnarchyManager::OnDefaultLibraryReadyCallback()
{
	this->OnDefaultLibraryReady();
}

void C_AnarchyManager::OnDefaultLibraryReady()
{
	m_pMetaverseManager->CommitTransaction();

#ifdef VR_ALLOWED
//	engine->ClientCmd("fps_max 90");
#endif	
	/*
	// check if library version is obsolete
	int iLibraryVersion = m_pMetaverseManager->ExtractLibraryVersion();
	if (iLibraryVersion == -1)
		iLibraryVersion = 0;
		//DevMsg("CRITICAL ERROR: Could not extract library version from library.db!\n");

	if (iLibraryVersion < AA_LIBRARY_VERSION)
	{
		DevMsg("Making a backup then converting your library from version %i to %i\n", iLibraryVersion, (iLibraryVersion + 1));
		if (m_pMetaverseManager->ConvertLibraryVersion(0, 1))
		{
			DevMsg("Done updating library.  Backup copy placed in the backups folder just in case.\n");
			this->OnReadyToLoadUserLibrary();
		}
		else
			DevMsg("CRITICAL ERROR: Could not convert library version! Aborting! You should make a copy of the newest backup in aarcade_user/backups and replace the aarcade_user/library.db file with the backup and try again. If it still fails, let me know on during a Twitch stream and I'll help figure things out with you! - SM Sith Lord\n");
	}
	else
	*/
		//this->OnReadyToLoadUserLibrary();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	/*
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Types", "usertypes", "", "", "0");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Cabinets", "usercabinets", "", "", "0");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Models", "usermodels", "", "", "0");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Apps", "userapps", "", "", "0");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Items", "useritems", "", "", "0");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "User Instances", "userinstances", "", "", "0");
	*/
	//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading User Library", "userlibrary", "0", "6", "0", "readyToLoadUserLibraryCallback");

	pHudBrowserInstance->AddHudLoadingMessage("", "", "Loading User Library...", "userlibrary", "", "", "", "readyToLoadUserLibraryCallback");
}

void C_AnarchyManager::Shutdown()
{
	DevMsg("AnarchyManager: Shutdown\n");
	m_bIsShuttingDown = true;

	m_pAccountant->StoreStats();

	if (m_pMeterInfo)
	{
		CoUninitialize();
		m_pMeterInfo = null;
	}

	/*
	if (m_pWebManager)
	{
		delete m_pWebManager;
		m_pWebManager = null;
	}
	*/

//	delete m_pLoadingManager;
	//m_pLoadingManager = null;

	delete m_pPanoStuff;

	if (m_pInputManager)
		m_pInputManager->DeactivateInputMode(true);

	if (m_pLibretroManager)
	{
		delete m_pLibretroManager;
		m_pLibretroManager = null;
	}

	if (m_pSteamBrowserManager)
	{
		delete m_pSteamBrowserManager;
		m_pSteamBrowserManager = null;
	}

	if (m_pMountManager)
	{
		delete m_pMountManager;
		m_pMountManager = null;
	}

	if (m_pWorkshopManager)
	{
		delete m_pWorkshopManager;
		m_pWorkshopManager = null;
	}

	if (m_pMetaverseManager)
	{
		delete m_pMetaverseManager;
		m_pMetaverseManager = null;
	}

	if (m_pAAIManager)
	{
		delete m_pAAIManager;
		m_pAAIManager = null;
	}

	if (m_pInstanceManager)
	{
		delete m_pInstanceManager;
		m_pInstanceManager = null;
	}

	if (m_pAwesomiumBrowserManager)
	{
		delete m_pAwesomiumBrowserManager;
		m_pAwesomiumBrowserManager = null;
	}

	if (m_pInputManager)
	{
		//m_pInputManager->DeactivateInputMode(true);
		delete m_pInputManager;
		m_pInputManager = null;
	}

	m_pToastMessagesKV->deleteThis();
	m_pToastMessagesKV = null;

	delete m_pNextLoadInfo;
	m_pNextLoadInfo = null;

	delete m_pImportInfo;
	m_pImportInfo = null;

	delete m_pConnectedUniverse;
	m_pConnectedUniverse = null;

	//delete[] m_pActionBarSlotConVars;

	for (unsigned int i = 0; i < 10; i++)
	{
		if (m_pActionBarSlotConVars[i])
		{
			//delete m_pActionBarSlotConVars[i];
			m_pActionBarSlotConVars[i] = null;
		}
	}

	ConVar* pOldAutoSaveConVar = cvar->FindVar("old_auto_save");
	int oldAutoSave = pOldAutoSaveConVar->GetInt();
	pOldAutoSaveConVar->SetValue(-1);
	if (oldAutoSave >= 0)
		cvar->FindVar("auto_save")->SetValue(oldAutoSave);

	DevMsg("AnarchyManager: Finished Shutdown\n");

	//g_pFullFileSystem->RemoveAllSearchPaths();	// doesn't make shutdown faster and causes warnings about failing to write cfg/server_blacklist.txt
}

void C_AnarchyManager::ResetImageLoader()
{
	C_AwesomiumBrowserInstance* pImagesBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("images");
	m_bImagesReady = false;
	m_bImagesResetting = true;
	pImagesBrowserInstance->ResetImagesSession();
	//m_pCanvasManager->RefreshItemTextures("", "ALL");
}

void C_AnarchyManager::LevelInitPreEntity()
{
	// do nothing.  moved to GamePrecache.
	this->GamePrecache();
}

void C_AnarchyManager::InstaLaunchItem(std::string itemId)
{
	std::string cmdLine = std::string(GetCommandLine());
	size_t found = cmdLine.find("-allow_insta_launch");	// FIXME: Improve this to tokenize the command line 1st.  Same with the -vr check.
	if (found != std::string::npos)
	{
		KeyValues* pItemKV = null;
		std::string goodItemId = "";
		if (itemId != "")
			goodItemId = itemId;
		else
		{
			int iEntityIndex = g_pAnarchyManager->GetSelectorTraceEntityIndex();
			if (iEntityIndex >= 0)
			{
				C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(iEntityIndex);
				if (pBaseEntity)
				{
					C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
					if (pShortcut)
						goodItemId = pShortcut->GetItemId();
				}
			}
		}

		if (goodItemId != "")
		{
			pItemKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->GetLibraryItem(goodItemId));
			if (pItemKV)
			{
				//g_pAnarchyManager->LaunchItem(goodItemId);
				if (!m_pInputManager->GetInputMode() && engine->IsInGame())
				{
					// handle escape if not in input mode & map is loaded (display the main menu)
					if (this->GetInputManager()->GetInputMode())
						this->GetInputManager()->DeactivateInputMode(true);

					C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

					std::string uri = VarArgs("asset://ui/launchItem.html?id=%s&insta=1", this->encodeURIComponent(goodItemId).c_str());
					if (m_pSelectedEntity)
					{
						DevMsg("Deselecting entity.\n");
						this->DeselectEntity(uri);
					}
					else
						pHudBrowserInstance->SetUrl(uri);

					m_pInputManager->ActivateInputMode(true, false);
				}
			}
		}
	}
}

void C_AnarchyManager::GamePrecache()
{
	if (!this->IsInitialized())
		return;

	m_bWasVRSnapTurn = false;
	m_bWasVRTeleport = false;
	m_bNeedVRTeleport = false;
	m_iVRSnapTurn = 0;
	m_bWaitingForInitialImagesToLoad = false;
	m_iLastNextDirection = 0;
	m_iOriginalSelect = -1;
	
	m_bPrecacheInstances = cvar->FindVar("precache_instances")->GetBool();
	m_bBatchObjectSpawn = cvar->FindVar("spawn_objects_in_batches")->GetBool();

	//if (m_bPrecacheInstances)	// Release the materials so that the dynamic textures don't screw up when the items are loaded over & over again.
	//	g_pMaterialSystem->UncacheAllMaterials();	// FIXME: It'd be best if we ONLY flushed the dynamic materials, not ALL materials.

	DevMsg("AnarchyManager: LevelInitPreEntity\n");
	//g_pHLVR->LevelInitPreEntity();

	cvar->FindVar("painted_skyname")->SetValue("");

	m_pAttractModeActiveConVar->SetValue(0);
	m_pCabinetAttractModeActiveConVar->SetValue(0);

	engine->ClientCmd(VarArgs("exec %s", this->MapName()));

	m_bIsDisconnecting = false;

	m_instanceId = m_pNextLoadInfo->instanceId;// m_nextInstanceId;
	m_currentLobby = m_nextLobby;
	m_currentLobbyTitle = m_nextLobbyTitle;
	m_nextLobby = "";
	m_nextLobbyTitle = "";

	// copy last_opened_project into current_project & clear last_opened_project
	cvar->FindVar("current_project")->SetValue(cvar->FindVar("last_opened_project")->GetString());
	cvar->FindVar("last_opened_project")->SetValue("");

	// copy next_player_spawn_override into player_spawn_override & clear next_player_spawn_override
	cvar->FindVar("player_spawn_override")->SetValue(cvar->FindVar("next_player_spawn_override")->GetString());
	cvar->FindVar("next_player_spawn_override")->SetValue("");


	C_AwesomiumBrowserInstance* pImagesBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("images");
	m_bImagesReady = false;
	m_bImagesResetting = true;
	pImagesBrowserInstance->ResetImagesSession();

	this->GetInputManager()->SetTempSelect(false);
	m_pFreeMouseModeConVar->SetValue(false);
	cvar->FindVar("tempfreelook")->SetValue(false);

	this->SetTabMenuFile("taskMenu.html");

	DevMsg("Finished resetting image session.\n");

	//g_pAnarchyManager->GetLibretroManager()->DetectAllOverlaysPNGs();

	// attempt to load the instance now, prior to post-entity
	/*
	if (m_pNextLoadInfo->instanceId != "")
	{
		g_pAnarchyManager->GetInstanceManager()->LoadInstance(null, m_pNextLoadInfo->instanceId, m_pNextLoadInfo->position, m_pNextLoadInfo->rotation);
		this->GetInstanceManager()->CacheAllInstanceModels();
	}
	else
		DevMsg("Map loaded with no instance chosen yet.  FIXME: Display a menu to choose an instance.\n");
		*/
}

void C_AnarchyManager::LevelInitPostEntity()
{
	if (!this->IsInitialized())
		return;

	DevMsg("AnarchyManager: LevelInitPostEntity\n");
	//g_pHLVR->LevelInitPostEntity();

	m_iSelectorTraceEntityIndex = -1;
	m_selectorTraceVector.x = 0;
	m_selectorTraceVector.y = 0;
	m_selectorTraceVector.z = 0;
	m_selectorTraceNormal.x = 0;
	m_selectorTraceNormal.y = 0;
	m_selectorTraceNormal.z = 0;

	m_pQuestManager->LoadEZQuests(m_pNextLoadInfo->instanceId);

	if (m_pNextLoadInfo->instanceId != "")
		g_pAnarchyManager->GetInstanceManager()->LoadInstance(null, m_pNextLoadInfo->instanceId, m_pNextLoadInfo->position, m_pNextLoadInfo->rotation);
	else
		DevMsg("Map loaded with no instance chosen yet.  FIXME: Display a menu to choose an instance.\n");

	m_pAccountant->Action("aa_maps_discovered", 1, this->MapName());

	g_pAnarchyManager->SetNextLoadInfo();

	m_bSuspendEmbedded = false;

	engine->ClientCmd("sv_infinite_aux_power 1; mp_timelimit 0; sv_timeout 300; cl_timeout 300;\n");

	if (cvar->FindVar("crosshair_color")->GetInt() != 0)
		engine->ClientCmd("hide_arcade_crosshair; show_arcade_crosshair;");

//	engine->ClientCmd("r_drawothermodels 1;");

	/* STILL TOO EARLY TO EXEC STUFF WITH SERVER CMDS!!
	// look for map-specific CFG files
	// ... actually, just try to exec them.  if they don't exist, it doesn't matter.
	DevMsg("Yadda: %s\n", this->MapName());
	engine->ClientCmd(VarArgs("exec %s\n", this->MapName()));
	*/

	/* REMOVED in favor of AudioPeak proxy
	m_pAudioStripMaterial = materials->FindMaterial("sm_arcade/audiostrip", TEXTURE_GROUP_WORLD);
	if (!m_pAudioStripMaterial || m_pAudioStripMaterial->IsErrorMaterial())
		m_pAudioStripMaterial = null;*/

	std::string actualModel = cvar->FindVar("playermodel")->GetString();
	if (actualModel != "")
		engine->ClientCmd(VarArgs("morphmodel \"%s\" 0\n", actualModel));

	m_bLevelInitialized = true;

	// set rich presence
	std::string mapName = (engine->IsInGame()) ? this->MapName() : "";
	steamapicontext->SteamFriends()->SetRichPresence("mapname", mapName.c_str());
	steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", m_pInstanceManager->GetInstanceObjectCount()));
	std::string status = (this->GetConnectedUniverse()) ? "#Status_Multiplayer" : "#Status_Singleplayer";
	if (this->GetConnectedUniverse())
		steamapicontext->SteamFriends()->SetRichPresence("connect", VarArgs("+join %s", this->GetConnectedUniverse()->lobby.c_str()));
	else
		steamapicontext->SteamFriends()->SetRichPresence("connect", null);

	steamapicontext->SteamFriends()->SetRichPresence("steam_display", status.c_str());
}

void C_AnarchyManager::LevelShutdownPreClearSteamAPIContext()
{
	if (!this->IsInitialized())
		return;

	DevMsg("AnarchyManager: LevelShutdownPreClearSteamAPIContext\n");
}

void C_AnarchyManager::LevelShutdownPreEntity()
{
	if (!this->IsInitialized())
		return;

	if (this->IsVRActive())
	{
		this->ToggleVR();
		m_bVRSuspending = 1;
	}

	this->DeactivateInspectObject();

	m_flStartQuestsSoon = 0.0f;

	m_pQuestManager->ShutdownWorldQuests();

	if (m_pLocalAvatarObject)
	{
		m_bIsCreatingLocalAvatar = false;
		m_pLocalAvatarObject = null;
	}
	
	m_flSpawnObjectsButtonDownTime = 0.0f;

	//m_pAudioStripMaterial = null;
	
	m_bPrecacheInstances = cvar->FindVar("precache_instances")->GetBool();
	m_bBatchObjectSpawn = cvar->FindVar("spawn_objects_in_batches")->GetBool();

	m_pInstanceManager->LevelShutdownPreEntity();

	g_pAnarchyManager->SetSelectOriginal(-1);
	m_alwaysLookObjects.clear();
	pVRHandLeft = null;
	pVRHandRight = null;
	pVRPointerLeft = null;
	pVRPointerRight = null;
	pVRTeleport = null;
	m_bWasVRSnapTurn = false;
	m_bWasVRTeleport = false;
	m_bNeedVRTeleport = false;
	m_iVRSnapTurn = 0;
	m_iVRSpazzFix = -1;//null;
	m_iVRSpazzFix2 = -1;// null;

	m_flNextAttractCameraTime = -1.0;

	//engine->ClientCmd("r_drawothermodels 0;");	// FIXME: obsolete??
	m_bLevelInitialized = false;

	DevMsg("AnarchyManager: LevelShutdownPreEntity\n");

	cvar->FindVar("current_project")->SetValue("");
	cvar->FindVar("player_spawn_override")->SetValue("");
	m_pAttractModeActiveConVar->SetValue(0);
	m_pCabinetAttractModeActiveConVar->SetValue(0);

	if (pModelPreview)
		pModelPreview->Remove();

	pModelPreview = null;

	m_fNextExtractOverviewCompleteManage = 0;
	/*
	bool bIsHost = false;
	aampConnection_t* pConnection = this->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		bIsHost = true;

	if (!bIsHost)
		m_pMetaverseManager->DisconnectSession();
		*/

	if (this->GetConnectedUniverse() && this->GetConnectedUniverse()->connected && this->GetConnectedUniverse()->isHost && this->GetConnectedUniverse()->isPersistent)
		m_pMetaverseManager->DisconnectSession();

	m_pMetaverseManager->RestartNetwork(false);

	m_pLibretroManager->LevelShutdownPreEntity();

	m_bSuspendEmbedded = true;
	m_bIsDisconnecting = true;
	
	C_BaseEntity* pEntity = this->GetSelectedEntity();
	if (pEntity)
		this->DeselectEntity();

	m_bImagesReady = false;
	// don't bother doing m_bImagesResetting cuz we don't really wanna load any new images until the NEXT reset, after the next map is loaded.
	C_AwesomiumBrowserInstance* pImagesBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("images");
	pImagesBrowserInstance->ResetImagesSession();

///////	// all instances other than HUD and IMAGES should be removed at this time (until cross-map background item play gets re-enabled.)
	m_pCanvasManager->LevelShutdownPreEntity();

	m_pAccountant->StoreStats();

	/*
	C_WebTab* pWebTab = m_pWebManager->GetSelectedWebTab();
	if (pWebTab)
		m_pWebManager->DeselectWebTab(pWebTab);

	m_pWebManager->LevelShutdownPreEntity();
	*/
}

void C_AnarchyManager::PerformAutoSave(saveModeType_t saveMode, bool bDoSaveInstance, bool bOnlyNew)
{
	m_saveMode = saveMode;
	if (m_saveMode == SAVEMODE_SAVE)	// assume we need to save, for now. some sort of switch needs to be flipped PRIOR to actually shutting down a level for changes to ACTUALLY be saved, eventually.
	{
		if (bDoSaveInstance)
			m_pInstanceManager->SaveActiveInstance();

		m_pMetaverseManager->SaveVolatileLocally();
	}
	else if (m_saveMode == SAVEMODE_DISCARD)
		m_pMetaverseManager->DiscardVolatileLocally();
	//else // else do nothing. FIXME: This means that instances changes will persist, but the logic will be used on active instead instead, which could result in an uneeded save to SQL of it is all.
	//m_pMetaverseManager->PurgeVolatileLocally();

	m_saveMode = SAVEMODE_NONE;
}

float C_AnarchyManager::GetVRGestureDist()
{
	Vector originA = m_VRGestureLeft.GetTranslation();
	Vector originB = m_VRGestureRight.GetTranslation();
	return originA.DistTo(originB);
}

float C_AnarchyManager::GetIPD()
{
	if (!m_pIPDConVar)
		m_pIPDConVar = cvar->FindVar("ipd");

	return m_pIPDConVar->GetFloat();
}

float C_AnarchyManager::GetLastFOV()
{
	if (!m_pLastFOVConVar)
		m_pLastFOVConVar = cvar->FindVar("lastfov");

	return m_pLastFOVConVar->GetFloat();
}

void C_AnarchyManager::SetLastFOV(float fValue)
{
	if (!m_pLastFOVConVar)
		m_pLastFOVConVar = cvar->FindVar("lastfov");

	m_pLastFOVConVar->SetValue(fValue);
}

void C_AnarchyManager::PrecacheModel(std::string file)
{
	if (modelinfo->GetModelIndex(file.c_str()) < 0)
		engine->ClientCmd(VarArgs("precachemodel \"%s\";", file.c_str()));	// servercmdfix
}

int C_AnarchyManager::GetModelThumbSize()
{
	return AA_MODEL_THUMB_SIZE;
	//if (!m_pModelThumbSizeConVar)
		//m_pModelThumbSizeConVar = cvar->FindVar("model_thumb_size");

	//return m_pModelThumbSizeConVar->GetInt();
}


int C_AnarchyManager::GetNoDrawShortcutsValue()
{
	return m_pNoDrawShortcutsConVar->GetInt();
}

///*
int C_AnarchyManager::GetVRSpazzFix(int iIndex)
{
	if (iIndex == 1)
		return m_iVRSpazzFix2;
	
	return m_iVRSpazzFix;
}
//*/

int C_AnarchyManager::GetSteamPlayerCountNumber()
{
	// only do this once-per-minute, max.
	if (m_flNextSystemTimeUpdateTime <= 0.0f || m_flNextNumberStatsUpdateTime <= m_flCurrentTime)
	{
		this->GetSteamPlayerCount();	// fetch the player count from the steamworks server.
		m_flNextGlobalPlayerCountUpdateTime = m_flCurrentTime + 60.0f;
	}

	return m_iPlayerCount;
}

void C_AnarchyManager::TravelByScreenshot(std::string screenshotId)
{
	//if (m_pInputManager->GetInputMode())
	//	m_pInputManager->DeactivateInputMode(true);

	KeyValues* pScreenshotKV = m_pMetaverseManager->GetScreenshot(screenshotId);

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->Select();
	pHudBrowserInstance->Focus();

	std::string mapId = pScreenshotKV->GetString("map/id");
	mapId = mapId.substr(2);
	std::string instanceId = pScreenshotKV->GetString("instance/id");
	instanceId = instanceId.substr(2);

	if (!m_pInstanceManager->FindInstance(instanceId))
	{
		std::vector<instance_t*> instances;
		m_pInstanceManager->FindAllInstances(mapId, instances);
		if (!instances.empty())
			instanceId = instances[0]->id;
		else
			instanceId = "";
	}

	std::string pos = pScreenshotKV->GetString("body/position");
	std::string rot = pScreenshotKV->GetString("body/rotation");

	pHudBrowserInstance->SetUrl(VarArgs("asset://ui/loading.html?map=%s&instance=%s&pos=%s&rot=%s&screenshot=%s", mapId.c_str(), instanceId.c_str(), pos.c_str(), rot.c_str(), screenshotId.c_str()));
	if (!m_pInputManager->GetInputMode())
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);
	/*else
	{

	}*/
}

ITexture* C_AnarchyManager::GetVRSpectatorTexture()
{
	/*if (!g_pAnarchyManager->IsVRActive() || !g_pAnarchyManager->VRSpectatorMode())
		return null;

	C_PropShortcutEntity* pBestCamera = g_pAnarchyManager->GetBestSpectatorCameraObject();
	if (!pBestCamera)
		return null;*/
	
	return m_pVRSpectatorRenderTexture;
	//ITexture* pVRFullFrame = materials->FindTexture("_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET);
	//if (pVRFullFrame)
//		m_pMaterialTextureVar->SetTextureValue(pVRFullFrame);
}

void C_AnarchyManager::Join(std::string lobby)
{
	//if (this->GetConnectedUniverse() && this->GetConnectedUniverse()->connected)//&& !g_pAnarchyManager->GetConnectedUniverse()->isHost)

	if (lobby == "")
		return;

	if (!this->IsInitialized())
		cvar->FindVar("firstserver")->SetValue(lobby.c_str());
	else
		this->JoinLobby(lobby);
}

std::string C_AnarchyManager::ModifyMaterial(std::string materialName, std::string materialVar, std::string value)
{
	// NOTE: We should really be giving this pointers, not strings.

	// find the material
	// we don't want the user to have to provide us with an internal texture group name, so check both the WORLD and MODEL texture groups.

	IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(materialName.c_str(), TEXTURE_GROUP_WORLD);
	if (!pMaterial || pMaterial->IsErrorMaterial())
		pMaterial = g_pMaterialSystem->FindMaterial(materialName.c_str(), TEXTURE_GROUP_MODEL);

	if ( !pMaterial || pMaterial->IsErrorMaterial() )
	{
		DevMsg("Material not found: %s\n", materialName.c_str());
		return "";
	}

	// find the materialvar
	bool bFoundVar;
	IMaterialVar* pMaterialVar = pMaterial->FindVar(materialVar.c_str(), &bFoundVar);

	if (pMaterialVar && bFoundVar)
	{
		// now we have to worry about types.
		// is the user trying to set a literal string, a number, or a texture value?

		// for now, lets just assume it's a texture value in $baseTexture, so let's find the texture we are gonna replace it with.
		if (pMaterialVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE)
		{
			// again, we don't want the user to have to specify an internal texture group, so let's check both.
			bool bIsModelMaterial = false;
			ITexture* pTexture = g_pMaterialSystem->FindTexture(value.c_str(), TEXTURE_GROUP_WORLD);
			if (!pTexture || pTexture->IsError())
			{
				pTexture = g_pMaterialSystem->FindTexture(value.c_str(), TEXTURE_GROUP_MODEL);
				bIsModelMaterial = true;
			}

			if (!pTexture || pTexture->IsError())
			{
				DevMsg("Texture not found (or is not loaded?): %s\n", value.c_str());
				return "";
			}

			// we will be returning the current value, as a string
			std::string oldValue = pMaterialVar->GetTextureValue()->GetName();

			// all systems go.  modify the material variable.
			pMaterialVar->SetTextureValue(pTexture);
			//pMaterial->Refresh();	<-- ACTUALLY WORKS for RESETTING ALL MATERIAL CHANGES!!!
			pMaterial->RefreshPreservingMaterialVars(); // This fixes the fullly lit issue that props have.

			// update the sv_skyname value
			if ((materialName.find("skybox/") == 0 || materialName.find("skybox\\") == 0))
			{
				if (value != "")
				{
					std::string skyboxName = value.substr(0, value.size() - 2);
					skyboxName = skyboxName.substr(7);
					engine->ClientCmd(VarArgs("painted_skyname \"%s\"", skyboxName.c_str()));
				}
				else
					engine->ClientCmd("painted_skyname \"\"");
			}

			// return old value, so book keeping can be done on the caller's end, if needed.
			return oldValue;
		}
		else
		{
			DevMsg("Unhandled materialvar type specified by the materialvar provided: %s - %s\n", materialName.c_str(), materialVar.c_str());
			return "";
		}
	}
}

bool C_AnarchyManager::ActionBarUse(int iSlot)
{
	bool bDidAction = false;
	if (iSlot > 9)
		return false;

	if (m_pWeaponsEnabledConVar->GetBool() && iSlot < 5)
		return false;

	ConVar* pSlotConVar = m_pActionBarSlotConVars[iSlot];
	if (iSlot < 0)
		return false;

	std::string val = pSlotConVar->GetString();
	engine->ClientCmd(val.c_str());
	return true;
}

void C_AnarchyManager::DoComparisonRender()
{
	if (!engine->IsInGame())
		return;

	m_pUseSBSRenderingConVar->SetValue(1);
	m_pNoDrawShortcutsConVar->SetValue(2);
	m_flComparisonRenderTime = gpGlobals->curtime + 2.0f;

	if (cvar->FindVar("reshade")->GetBool() && cvar->FindVar("reshadedepth")->GetBool())
		cvar->FindVar("r_drawvgui")->SetValue(1);
}

void C_AnarchyManager::OnComparisonRenderFinished()
{
	m_pUseSBSRenderingConVar->SetValue(0);
	m_pNoDrawShortcutsConVar->SetValue(0);
	m_flComparisonRenderTime = -1;

	if (cvar->FindVar("reshade")->GetBool() && cvar->FindVar("reshadedepth")->GetBool() && !enginevgui->IsGameUIVisible())
		cvar->FindVar("r_drawvgui")->SetValue(0);
}

float C_AnarchyManager::GetComparisonRenderAmount()
{
	if (!engine->IsInGame() || m_flComparisonRenderTime < 0)
		return 0;

	float flAmount = gpGlobals->curtime - m_flComparisonRenderTime;
	if (flAmount < 0)
		return 0;
	else if (flAmount > 1.0f)
		return 1.0f;

	return flAmount;
}

void C_AnarchyManager::MakeRagdoll(C_PropShortcutEntity* pShortcut)
{
	engine->ClientCmd(VarArgs("makeragdollnow %i", pShortcut->entindex()));
}

void C_AnarchyManager::RagdollInfo(C_PropShortcutEntity* pShortcut)
{
	m_pRagdollShortcut = pShortcut;
	//pShortcut->BecomeRagdollOnClient();

	int iHeadBone = pShortcut->LookupBone("ValveBiped.Bip01_Head1");

	//pShortcut->Bone
	DevMsg("Bone number: %i\n", iHeadBone);

	if (iHeadBone < 0)// || !pShortcut->IsRagdoll()
		return;

	Vector pos;
	matrix3x4_t headBoneMatrix;

	headBoneMatrix = pShortcut->GetBoneForWrite(iHeadBone);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 0 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);

	CStudioHdr* hdr = pShortcut->GetModelPtr();
	int boneControllerCount = hdr->numbonecontrollers();

	//m_iv_flEncodedController.SetMaxCount(boneControllerCount);

	mstudiobone_t* pBone = hdr->pBone(iHeadBone);
	for (int i = 0; i < boneControllerCount; i++)
	{
		bool loop = (hdr->pBonecontroller(i)->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) != 0;
		//m_iv_flEncodedController.SetLooping(loop, i);
		pShortcut->SetBoneController(i, 0.0);
	}


	/*
	int m_headYawMin;
	int m_headYawMax;
	int m_headYawPoseParam = pShortcut->LookupPoseParameter("head_yaw");
	pShortcut->GetPoseParameterRange(m_headYawPoseParam, m_headYawMin, m_headYawMax);

	m_headPitchPoseParam = LookupPoseParameter("head_pitch");
	GetPoseParameterRange(m_headPitchPoseParam, m_headPitchMin, m_headPitchMax);

	CStudioHdr *hdr = GetModelPtr();
	for (int i = 0; i < hdr->GetNumPoseParameters(); i++)
	{
		SetPoseParameter(hdr, i, 0.0);
	}
	*/
	
	
	/*
	Vector vDeltaToAdd;
	vDeltaToAdd.x = 0;
	vDeltaToAdd.y = 0;
	vDeltaToAdd.z = 100;

	matrix3x4_t& bone = pShortcut->GetBoneForWrite(iHeadBone);
	Vector vBonePos;
	MatrixGetTranslation(bone, vBonePos);
	vBonePos += vDeltaToAdd;
	MatrixSetTranslation(vBonePos, bone);
	pShortcut->VPhysicsUpdate(pShortcut->VPhysicsGetObject());
	*/
	

	/*
	Vector offset;
	offset.x = 0;
	offset.y = 0;
	offset.z = 100;

	matrix3x4_t &matrix0 = pShortcut->GetBoneForWrite(iHeadBone);
	MatrixGetColumn(matrix0, 3, pos);
	pos += offset;
	MatrixSetColumn(pos, 3, matrix0);
	*/

	/*
	Vector vDeltaToAdd;
	vDeltaToAdd.x = 0;
	vDeltaToAdd.y = 0;
	vDeltaToAdd.z = 100;

	matrix3x4_t& bone = pShortcut->GetBoneForWrite(iHeadBone);
	Vector vBonePos;
	MatrixGetTranslation(bone, vBonePos);
	vBonePos += vDeltaToAdd;
	MatrixSetTranslation(vBonePos, bone);
	*/

	//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	//matrix3x4_t headMatrix = pShortcut->GetBone(iHeadBone);
	//pShortcut->SetupSingleBoneMatrix()

	/*
	Vector pos;
	matrix3x4_t headBoneMatrix;

	headBoneMatrix = pShortcut->GetBoneForWrite(0);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 0 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(1);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 1 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(2);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 2 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(2);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 3 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);

	Vector offset;
	offset.x = 0;
	offset.y = 0;
	offset.z = 100;

	//Vector pos;
	matrix3x4_t &matrix0 = pShortcut->GetBoneForWrite(0);
	MatrixGetColumn(matrix0, 3, pos);
	pos += offset;
	MatrixSetColumn(pos, 3, matrix0);

	//Vector pos;
	matrix3x4_t &matrix1 = pShortcut->GetBoneForWrite(1);
	MatrixGetColumn(matrix1, 3, pos);
	pos += offset;
	MatrixSetColumn(pos, 3, matrix1);

	//Vector pos;
	matrix3x4_t &matrix2 = pShortcut->GetBoneForWrite(2);
	MatrixGetColumn(matrix2, 3, pos);
	pos += offset;
	MatrixSetColumn(pos, 3, matrix2);

	//Vector pos;
	matrix3x4_t &matrix3 = pShortcut->GetBoneForWrite(3);
	MatrixGetColumn(matrix3, 3, pos);
	pos += offset;
	MatrixSetColumn(pos, 3, matrix3);

	Vector vDeltaToAdd;
	vDeltaToAdd.x = 0;
	vDeltaToAdd.y = 0;
	vDeltaToAdd.z = 100;

	matrix3x4_t& bone = pShortcut->GetBoneForWrite(3);
	Vector vBonePos;
	MatrixGetTranslation(bone, vBonePos);
	vBonePos += vDeltaToAdd;
	MatrixSetTranslation(vBonePos, bone);
	*/

	/*
	// build the peseudo matrix
	Vector pseudoOrigin;
	pseudoOrigin.Init(0, 100, 0);
	QAngle pseudoAngles;
	pseudoAngles.Init();

	VMatrix pseudoEyeMatrix;
	pseudoEyeMatrix.SetupMatrixOrgAngles(pseudoOrigin, pseudoAngles);

	// apply
	headBoneMatrix = pseudoEyeMatrix.As3x4();//headBoneMatrix * 

	headBoneMatrix = pShortcut->GetBoneForWrite(1);
	pseudoEyeMatrix.SetupMatrixOrgAngles(pseudoOrigin, pseudoAngles);
	//MatrixBuildTranslation(headBoneMatrix, Vector(0, 0, 100));

	headBoneMatrix = pseudoEyeMatrix.As3x4();
	headBoneMatrix = pShortcut->GetBoneForWrite(2);
	pseudoEyeMatrix.SetupMatrixOrgAngles(pseudoOrigin, pseudoAngles);
	headBoneMatrix = pseudoEyeMatrix.As3x4();
	headBoneMatrix = pShortcut->GetBoneForWrite(3);
	pseudoEyeMatrix.SetupMatrixOrgAngles(pseudoOrigin, pseudoAngles);
	headBoneMatrix = pseudoEyeMatrix.As3x4();


	headBoneMatrix = pShortcut->GetBoneForWrite(0);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 0 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(1);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 1 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(2);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 2 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	headBoneMatrix = pShortcut->GetBoneForWrite(2);
	MatrixPosition(headBoneMatrix, pos);
	DevMsg("Bone 3 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);
	*/

	//pShortcut->ApplyBoneMatrixTransform(headMatrix);
	//VMatrix playerMatrix;
	//playerMatrix.SetupMatrixOrgAngles(pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles());

	//pShortcut->ApplyBoneMatrixTransform()
}

void C_AnarchyManager::RefreshImages(std::string itemId, std::string modelId)
{
	if (itemId != "")
	{
		// get the current cache image file
		ITexture* pTexture = m_pCanvasManager->GetItemTexture(itemId, "screen");
		if (pTexture)
		{
			std::string cacheFile = m_pCanvasManager->GetItemTextureCacheName(pTexture);
			//DevMsg("Screen Cache: %s\n", cacheFile.c_str());
			if (cacheFile != "")//cacheFile.find("asset://cache/") == 0)
			{
				//cacheFile = std::string("cache/urls/") + cacheFile.substr(14);

				if (g_pFullFileSystem->FileExists(cacheFile.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(cacheFile.c_str(), "DEFAULT_WRITE_PATH");
			}
		}
		
		pTexture = m_pCanvasManager->GetItemTexture(itemId, "marquee");
		if (pTexture)
		{
			std::string cacheFile = m_pCanvasManager->GetItemTextureCacheName(pTexture);
			//DevMsg("Marquee Cache: %s\n", cacheFile.c_str());
			if (cacheFile != "")//cacheFile.find("asset://cache/") == 0)
			{
				//cacheFile = std::string("cache/urls/") + cacheFile.substr(14);

				if (g_pFullFileSystem->FileExists(cacheFile.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(cacheFile.c_str(), "DEFAULT_WRITE_PATH");
			}
		}

		m_pCanvasManager->PrepareRefreshItemTextures(itemId, "ALL");
		m_pCanvasManager->RefreshItemTextures(itemId, "ALL");
	}

	if (modelId != "")
	{
		DevMsg("Model thumb cache should refresh too.\n");
	}
}

std::string C_AnarchyManager::CreateModelPreview(std::string givenModelName)
{
	//pModelPreview
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return "";

	std::string modelHash = this->GenerateLegacyHash(givenModelName.c_str());

	std::string filePath = "cache/models";
	std::string fileName = filePath + "/";
	fileName += modelHash;
	fileName += ".tga";

	if (g_pFullFileSystem->FileExists(fileName.c_str(), "DEFAULT_WRITE_PATH"))
		return fileName;
	
#ifdef VR_ALLOWED
	return "";
#endif

	std::string modelName = givenModelName;
	//std::transform(modelName.begin(), modelName.end(), modelName.begin(), ::tolower);
	std::replace(modelName.begin(), modelName.end(), '/', '\\');
	//std::replace(modelName.begin(), modelName.end(), '\\', '/');

	DevMsg("Generating thumbnail for model: %s\n", modelName.c_str());

	// if the pointer already exists, remove it as we create a new one.
	if (pModelPreview)
		pModelPreview->Remove();
	
	// create the model entity
	pModelPreview = new C_DynamicProp();

	//const model_t* TheModel = modelinfo->FindOrLoadModel(modelName.c_str());

	int iIndex = modelinfo->GetModelIndex(modelName.c_str());
	if (iIndex < 0)
	{
		iIndex = modelinfo->GetModelIndex(givenModelName.c_str());
		if (iIndex >= 0)
			modelName = givenModelName;
	}

	if (iIndex < 0)
	{
		//if (!m_crashModelLogHandle)
		//	m_crashModelLogHandle = filesystem->Open("modelload.log", "w", "DEFAULT_WRITE_PATH");

		//if (m_crashModelLogHandle)
		//{
		///	filesystem->FPrintf(m_crashModelLogHandle, "%s", modelName.c_str());
		//	filesystem->Close(m_crashModelLogHandle);
		//}

		FileHandle_t logfile = filesystem->Open("modelload.log", "w", "DEFAULT_WRITE_PATH");
		if (logfile)
		{
			failedModelName = modelName;
			filesystem->FPrintf(logfile, "%s", modelHash.c_str());// modelName.c_str());
			filesystem->Close(logfile);
		}

		this->PrecacheModel(modelName);
		return "-caching-";
	}

	if (pModelPreview->InitializeAsClientEntity(modelName.c_str(), RENDER_GROUP_OPAQUE_ENTITY))
	{
		FileHandle_t logfile = filesystem->Open("modelload.log", "w", "DEFAULT_WRITE_PATH");
		if (logfile)
		{
			failedModelName = "";
			filesystem->Close(logfile);
		}

		pModelPreview->AddEffects(EF_NODRAW); // don't let the renderer draw the model normally
		//pModelPreview->SetSequence(pModelPreview->LookupSequence("idle"));

		// create the light
		Vector origin = pPlayer->EyePosition();
		Vector lightOrigin = origin;

		// find a spot inside the world for the dlight's origin, or it won't illuminate the model
		Vector testPos(origin.x - 100, origin.y, origin.z + 100);
		trace_t tr;
		UTIL_TraceLine(origin, testPos, MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction == 1.0f)
			lightOrigin = tr.endpos;
		else
		{
			// Now move the model away so we get the correct illumination
			lightOrigin = tr.endpos + Vector(1, 0, -1);	// pull out from the solid
			Vector start = lightOrigin;
			Vector end = lightOrigin + Vector(100, 0, -100);
			UTIL_TraceLine(start, end, MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr);
			origin = tr.endpos;
		}

		float ambient = engine->GetLightForPoint(origin, true).Length();

		// Make a light so the model is well lit.
		// use a non-zero number so we cannibalize ourselves next frame
		dlight_t* dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + 1);

		dl->flags = DLIGHT_NO_WORLD_ILLUMINATION;
		dl->origin = lightOrigin;
		// Go away immediately so it doesn't light the world too.
		dl->die = gpGlobals->curtime + 0.1f;

		dl->color.r = dl->color.g = dl->color.b = 250;
		if (ambient < 1.0f)
			dl->color.exponent = 1 + (1 - ambient) * 2;
		dl->radius = 400;

		// move model in front of view
		pModelPreview->SetAbsOrigin(origin);
		pModelPreview->SetAbsAngles(QAngle(0, 180, 0));

		// set upper body animation
		/*pPlayer->m_SequenceTransitioner.Update(
			pPlayer->GetModelPtr(),
			pPlayer->LookupSequence(idle_lower),
			pPlayer->GetCycle(),
			pPlayer->GetPlaybackRate(),
			gpGlobals->realtime,
			false,
			true
			);*/

		// Now, blend the lower and upper (aim) anims together
		/*pPlayerModel->SetNumAnimOverlays(2);
		int numOverlays = pPlayerModel->GetNumAnimOverlays();
		for (int i = 0; i < numOverlays; ++i)
		{
		C_AnimationLayer* layer = pPlayerModel->GetAnimOverlay(i);
		layer->flCycle = pPlayerModel->GetCycle();
		if (i)
		layer->nSequence = pPlayerModel->LookupSequence(pWeaponSequence);
		else
		layer->nSequence = pPlayerModel->LookupSequence(walk_lower);
		layer->flPlaybackrate = 1.0;
		layer->flWeight = 1.0f;
		layer->SetOrder(i);
		}*/

		//pModelPreview->FrameAdvance(gpGlobals->frametime);

		// Now draw it.
		CViewSetup view;
		// setup the views location, size and fov (amongst others)
		view.x = 0;
		view.y = 0;
		view.width = this->GetModelThumbSize();
		view.height = this->GetModelThumbSize();

		view.m_bOrtho = false;
		view.fov = 70;

		view.origin = origin;// +Vector(-50, 0, -5);// +Vector(-110, -5, -5);
		view.m_flAspectRatio = 1.0f;
		view.m_bViewToProjectionOverride = false;


		// make sure that we see all of the model
		Vector vMins, vMaxs;
		pModelPreview->C_BaseAnimating::GetRenderBounds(vMins, vMaxs);
		Vector vRenderOrigin = pModelPreview->GetRenderOrigin();
		//DevMsg("RENDER X/Y/Z: %02f %02f %02f\n", vRenderOrigin.x, vRenderOrigin.y, vRenderOrigin.z);
		//DevMsg("MINS X/Y/Z: %02f %02f %02f\n", vMins.x, vMins.y, vMins.z);
		//DevMsg("MAXS X/Y/Z: %02f %02f %02f\n", vMaxs.x, vMaxs.y, vMaxs.z);

		//float flBiggestDist = -1.0f;
		float flDist = (vMins.z + vMaxs.z);
		//if (flBiggestDist < flDist)
		//	flBiggestDist = flDist;
		view.origin.z += flDist * 0.5f;

		flDist = (vMins.y + vMaxs.y);
		//if (flBiggestDist < flDist)
		//	flBiggestDist = flDist;
		view.origin.y -= flDist *  0.5f;

		flDist = (vMins.x + vMaxs.x);
		//if (flBiggestDist < flDist)
		//	flBiggestDist = flDist;
		view.origin.x -= (flDist * 0.5f);

		float flBiggestDist = -1.0f;
		flDist = (vMaxs.x - vMins.x);
		if (flBiggestDist < flDist)
			flBiggestDist = flDist;
		flDist = (vMaxs.y - vMins.y);
		if (flBiggestDist < flDist)
			flBiggestDist = flDist;
		flDist = (vMaxs.z - vMins.z);
		if (flBiggestDist < flDist)
			flBiggestDist = flDist;
		view.origin.x -= flBiggestDist * 1.1f;

		view.origin.y -= (flBiggestDist * 0.3f);
		view.origin.z += (flBiggestDist * 0.3f);
		view.angles = QAngle(16, 16, 0);

		//DevMsg("Min/Max: %f / %f\n", vMins.x, vMaxs.x);
		//view.angles.Init();
		view.zNear = VIEW_NEARZ * 0.3;
		view.zFar = 1000;

		// render it out to the new CViewSetup area

		//GetFullFrameFrameBufferTexture( 0 )
		Frustum dummyFrustum;
		ITexture* pRenderTarget = this->GetModelPreviewRenderTarget();
		render->Push3DView(view, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pRenderTarget, dummyFrustum);
		modelrender->SuppressEngineLighting(true);
		float color[3] = { 1.0f, 1.0f, 1.0f };
		render->SetColorModulation(color);
		render->SetBlend(1.0f);

		int iStudioRenderer = 0x00000001;	// (from frontend/public/model_types.h line 16)
		pModelPreview->DrawModel(iStudioRenderer);

		modelrender->SuppressEngineLighting(false);
		render->PopView(dummyFrustum);
		//pRenderContext->CopyTextureToRenderTargetEx(0, pVRFullFrame, &SourceRect, &DestinationRect);

		// grab the pixels & create a TGA
		//ITexture* pTexture = g_pAnarchyManager->GetModelPreviewRenderTarget();
		CMatRenderContextPtr pRenderContext(materials);
		ITexture* pOldTexture = pRenderContext->GetRenderTarget();
		pRenderContext->SetRenderTarget(pRenderTarget);

		unsigned int width = this->GetModelThumbSize();
		unsigned int height = this->GetModelThumbSize();
		unsigned int bufferSize = width * height * 4;

		// Get the data from the render target and save to disk bitmap bits
		unsigned char *pImage = (unsigned char *)malloc(width * 4 * height);

		// Get Bits from the material system
		pRenderContext->ReadPixels(0, 0, width, height, pImage, IMAGE_FORMAT_RGBA8888);

		CPixelWriter pixelWriter;
		pixelWriter.SetPixelMemory(IMAGE_FORMAT_RGBA8888, pImage, width * 4);

		// change the depth channel into an alpha channel, 1-bit
		int r, g, b, a;
		int xmax = width;
		int ymax = height;
		int x, y;
		for (y = 0; y < ymax; ++y)
		{
			pixelWriter.Seek(0, y);
			for (x = 0; x < xmax; ++x)
			{
				pixelWriter.ReadPixelNoAdvance(r, g, b, a);
				a = (a < 255 || r > 0 || g > 0 || b > 0) ? 255 : 0;
				pixelWriter.WritePixel(r, g, b, a);
			}
		}

		// allocate a buffer to write the tga into
		int iMaxTGASize = 1024 + (width * height * 4);
		void *pTGA = malloc(iMaxTGASize);
		CUtlBuffer buffer(pTGA, iMaxTGASize);
		//pRenderContext->CopyTextureToRenderTargetEx(0, pVRFullFrame, &SourceRect, &DestinationRect);
		if (!TGAWriter::WriteToBuffer(pImage, buffer, width, height, IMAGE_FORMAT_RGBA8888, IMAGE_FORMAT_RGBA8888))
			DevMsg("Couldn't write bitmap data snapshot.\n");

		free(pImage);

		// async write to disk (this will take ownership of the memory)
		//char szPathedFileName[_MAX_PATH];
		//Q_snprintf(szPathedFileName, sizeof(szPathedFileName), "//MOD/%d_%s_%s.tga", s_nRTIndex++, pFilename, IsOSX() ? "OSX" : "PC");

		// save the TGA out

		g_pFullFileSystem->CreateDirHierarchy(filePath.c_str(), "DEFAULT_WRITE_PATH");


		FileHandle_t fileTGA = filesystem->OpenEx(fileName.c_str(), "wb", 0, "DEFAULT_WRITE_PATH");
		filesystem->Write(buffer.Base(), buffer.TellPut(), fileTGA);
		filesystem->Close(fileTGA);

		//bufferSize = buffer.Size();
		//SendResponse(request_id, bufferSize + 1, (unsigned char*)buffer.Base(), WSLit("image"));

		free(pTGA);
		//buffer.Clear();
		pRenderContext->SetRenderTarget(pOldTexture);
		return fileName;
	}

	return "";
}

void C_AnarchyManager::IncrementHueShifter(float frametime)
{
	m_fHueShifter += frametime * 4.0f;
	if (m_fHueShifter > 360.0f)
		m_fHueShifter = 0.0f;

	engine->ClientCmd(VarArgs("hueshift %f", m_fHueShifter));
}

bool C_AnarchyManager::GetAutoSave()
{
	if (!m_pAutoSaveConVar)
		m_pAutoSaveConVar = cvar->FindVar("auto_save");

	return m_pAutoSaveConVar->GetBool();
}

void C_AnarchyManager::LevelShutdownPostEntity()
{
	if (!this->IsInitialized())
		return;

	DevMsg("AnarchyManager: LevelShutdownPostEntity\n");

	if (this->IsVRActive())
	{
		m_VRHeadMatrix.Identity();

		if (this->IsHandTrackingActive())
		{
			//memcpy(&m_VRLeftControllerMatrix, flLeftController, sizeof(m_VRLeftControllerMatrix));
			m_VRLeftControllerMatrix.Identity();// = g_pAnarchyManager->SMMatrixToVMatrix(m_VRLeftControllerMatrix.Base());

			//memcpy(&m_VRRightControllerMatrix, flRightController, sizeof(m_VRRightControllerMatrix));
			m_VRRightControllerMatrix.Identity();// = g_pAnarchyManager->SMMatrixToVMatrix(m_VRRightControllerMatrix.Base());
		}
	}

	if (m_pInstanceManager)
	{
		if (!this->GetAutoSave())
		{
			// perform auto-save logic
			this->PerformAutoSave(m_saveMode);
		}

		m_pInstanceManager->LevelShutdownPostEntity();
	}

	//if (m_crashModelLogHandle)
	//{
	//	filesystem->Close(m_crashModelLogHandle);
	//	m_crashModelLogHandle = null;
	//}

	m_bWasVRSnapTurn = false;
	m_bWasVRTeleport = false;
	m_bNeedVRTeleport = false;
	m_iVRSnapTurn = 0;

	m_instanceId = "";	// wtf is this??  (its the name of the currently loaded instance, which *should* actually be held inside of the isntance manager.

	// Clear out the simple images
	// all instances other than HUD and IMAGES should be removed at this time (until cross-map background item play gets re-enabled.)
	m_pCanvasManager->LevelShutdownPostEntity();

	m_pMetaverseManager->DestroyAllUploadBatches();
	m_pMetaverseManager->DestroyAllDownloadBatches();

	// set rich presence
	//steamapicontext->SteamFriends()->ClearRichPresence();
	steamapicontext->SteamFriends()->SetRichPresence("objectcount", "0");
	steamapicontext->SteamFriends()->SetRichPresence("mapname", "None");
	steamapicontext->SteamFriends()->SetRichPresence("steam_display", "#Status_Loading");
}

void C_AnarchyManager::PaintMaterial()
{
	if (!m_pInstanceManager->GetCurrentInstance())
		return;

	trace_t tr;
	Vector forward;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->EyeVectors(&forward);
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID

	if (tr.DidHit())
	{
		// check if we have a valid texture
		IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
		if (!pMaterial || pMaterial->IsErrorMaterial())
			pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

		if (pMaterial && !pMaterial->IsErrorMaterial())
			m_pInstanceManager->SetMaterialMod(tr.surface.name, "$baseTexture", m_pPaintTextureConVar->GetString());
		else if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			if (tr.DidHitWorld())
			{

				/* don't know what to do with displacement surfaces yet.
				if (tr.IsDispSurface())
				{
				DevMsg("Entity Index: %i\n", tr.GetEntityIndex());
				}*/

				/* potentially this could get the specific material hit, but may need to make it relative to the world origin instead when casting against static props?? not sure.
				Ray_t ray;
				ray.Init(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE);

				trace_t trProp;
				staticpropmgr->TraceRayAgainstStaticProp(ray, tr.hitbox, trProp);

				if (trProp.fraction != 1.0)
				{
				DevMsg("Surface: %s\n", trProp.surface.name);
				}*/

				ICollideable* pCollideable =  staticpropmgr->GetStaticPropByIndex(tr.hitbox);
				if (pCollideable)
				{
					bool bDidModify = false;
					const model_t* TheModel = pCollideable->GetCollisionModel();

					int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
					if (iNumMaterials < 1)
						iNumMaterials = 32;
					IMaterial** pMaterials = new IMaterial*[iNumMaterials];
					for (unsigned int i = 0; i < iNumMaterials; i++)
						pMaterials[i] = null;
					modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

					for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
					{
						if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
						{
							m_pInstanceManager->SetMaterialMod(pMaterials[x]->GetName(), "$baseTexture", m_pPaintTextureConVar->GetString(), false);
							bDidModify = true;
						}
					}

					delete[] pMaterials;

					if (bDidModify && (!this->GetConnectedUniverse() || !this->GetConnectedUniverse()->connected || this->GetConnectedUniverse()->isHost) )
						m_pInstanceManager->SaveActiveInstance();
				}

			}
			else if (tr.DidHitNonWorldEntity())
			{
				bool bDidModify = false;
				// we have no material yet, but do have an entity.  so apply to ALL materials used on the model.
				const model_t* TheModel = tr.m_pEnt->GetModel();

				int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
				if (iNumMaterials < 1)
					iNumMaterials = 32;
				IMaterial** pMaterials = new IMaterial*[iNumMaterials];
				for (unsigned int i = 0; i < iNumMaterials; i++)
					pMaterials[i] = null;
				modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

				for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
				{
					if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
					{
						m_pInstanceManager->SetMaterialMod(pMaterials[x]->GetName(), "$baseTexture", m_pPaintTextureConVar->GetString(), false);
						bDidModify = true;
					}
				}

				delete[] pMaterials;

				if (bDidModify && (!this->GetConnectedUniverse() || !this->GetConnectedUniverse()->connected || this->GetConnectedUniverse()->isHost))
					m_pInstanceManager->SaveActiveInstance();
			}
		}
	}
}

void C_AnarchyManager::UnpaintMaterial()
{
	if (!m_pInstanceManager->GetCurrentInstance())
		return;

	trace_t tr;
	Vector forward;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->EyeVectors(&forward);
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	if (tr.DidHit())
	{
		//m_pInstanceManager->ClearMaterialMod(tr.surface.name);

		// check if we have a valid texture
		IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
		if (!pMaterial || pMaterial->IsErrorMaterial())
			pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

		if (pMaterial && !pMaterial->IsErrorMaterial())
			m_pInstanceManager->ClearMaterialMod(tr.surface.name);
		else if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			if (tr.DidHitWorld())
			{
				/*Ray_t ray;
				ray.Init(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE);

				trace_t trProp;
				staticpropmgr->TraceRayAgainstStaticProp(ray, tr.hitbox, trProp);

				if (trProp.fraction != 1.0)
				{
				DevMsg("Surface: %s\n", trProp.surface.name);
				}*/

				ICollideable* pCollideable = staticpropmgr->GetStaticPropByIndex(tr.hitbox);
				if (pCollideable)
				{
					bool bDidModify = false;
					const model_t* TheModel = pCollideable->GetCollisionModel();

					int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
					if (iNumMaterials < 1)
						iNumMaterials = 32;
					IMaterial** pMaterials = new IMaterial*[iNumMaterials];
					for (unsigned int i = 0; i < iNumMaterials; i++)
						pMaterials[i] = null;
					modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

					for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
					{
						if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
						{
							m_pInstanceManager->ClearMaterialMod(pMaterials[x]->GetName(), false);
							bDidModify = true;
						}
					}

					delete[] pMaterials;

					if (bDidModify && (!this->GetConnectedUniverse() || !this->GetConnectedUniverse()->connected || this->GetConnectedUniverse()->isHost))
						m_pInstanceManager->SaveActiveInstance();
				}

			}
			else if (tr.DidHitNonWorldEntity())
			{
				bool bDidModify = false;
				// we have no material yet, but do have an entity.  so apply to ALL materials used on the model.
				const model_t* TheModel = tr.m_pEnt->GetModel();

				int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
				if (iNumMaterials < 1)
					iNumMaterials = 32;
				IMaterial** pMaterials = new IMaterial*[iNumMaterials];
				for (unsigned int i = 0; i < iNumMaterials; i++)
					pMaterials[i] = null;
				modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

				for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
				{
					if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
					{
						m_pInstanceManager->ClearMaterialMod(pMaterials[x]->GetName(), false);
						bDidModify = true;
					}
				}

				delete[] pMaterials;

				if (bDidModify && (!this->GetConnectedUniverse() || !this->GetConnectedUniverse()->connected || this->GetConnectedUniverse()->isHost))
					m_pInstanceManager->SaveActiveInstance();
			}
		}
	}
}

std::string C_AnarchyManager::GenerateTextureThumb(std::string textureName)
{
	// read the texture
	FileHandle_t fh = filesystem->Open(VarArgs("materials/%s.vtf", textureName.c_str()), "rb", "GAME");
	if (fh)
	{
		int file_len = filesystem->Size(fh);
		unsigned char* pImageData = new unsigned char[file_len + 1];

		filesystem->Read((void*)pImageData, file_len, fh);
		pImageData[file_len] = 0; // null terminator

		filesystem->Close(fh);

		g_pFullFileSystem->CreateDirHierarchy("cache/textures", "DEFAULT_WRITE_PATH");

		std::string textureId = this->GenerateLegacyHash(textureName.c_str());

		//std::string textureFile = VarArgs("screenshots/overviews/%s.vtf", mapName.c_str());
		FileHandle_t fh2 = filesystem->Open("cache/textures/temp.vtf", "wb", "DEFAULT_WRITE_PATH");
		if (fh2)
		{
			filesystem->Write(pImageData, file_len, fh2);
			filesystem->Close(fh2);

			// cleanup
			delete[] pImageData;

			std::string toolsFolder = g_pAnarchyManager->GetAArcadeToolsFolder();
			std::string userFolder = g_pAnarchyManager->GetAArcadeUserFolder();

			FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");
			if (launch_file)
			{
				std::string executable = VarArgs("%s\\vtf2tga.exe", toolsFolder.c_str());
				std::string goodExecutable = "\"" + executable + "\"";
				filesystem->FPrintf(launch_file, "%s:\n", goodExecutable.substr(1, 1).c_str());
				filesystem->FPrintf(launch_file, "cd \"%s\"\n", goodExecutable.substr(1, goodExecutable.find_last_of("/\\", goodExecutable.find("\"", 1)) - 1).c_str());
				filesystem->FPrintf(launch_file, "START \"Launching VTEX...\" %s -i \"%s\\cache\\textures\\temp.vtf\" -o \"%s\\cache\\textures\\%s.tga\"", goodExecutable.c_str(), userFolder.c_str(), userFolder.c_str(), textureId.c_str());
				filesystem->Close(launch_file);
				system("Arcade_Launcher.bat");
				return textureId;
			}
		}
	}

	return "";
}

void C_AnarchyManager::AdoptSky(std::string skyname)
{
	std::string name_in = std::string("skybox/") + skyname;

	//char input[AA_MAX_STRING];
	int iAAMaxString = name_in.length() + 1;
	char* input = new char[iAAMaxString];
	Q_strncpy(input, name_in.c_str(), iAAMaxString);

	// Convert it to lowercase & change all slashes to back-slashes
	V_FixSlashes(input, '/');
	std::string originalTextureName = input;
	delete[] input;

	size_t foundSlash = originalTextureName.find('/');
	if (foundSlash == 0)
		originalTextureName = originalTextureName.substr(1);

	std::vector<std::string> vars;
	vars.push_back("bk");
	vars.push_back("dn");
	vars.push_back("ft");
	vars.push_back("lf");
	vars.push_back("rt");
	vars.push_back("up");

	for (unsigned int i = 0; i < vars.size(); i++)
	{
		std::string textureName = originalTextureName + vars[i];
		std::string relativeName = textureName;
		std::string relativePath = "custom/adopted/materials";

		foundSlash = relativeName.find_last_of("/");
		if (foundSlash != std::string::npos)
		{
			relativePath += "/";
			relativePath += relativeName.substr(0, foundSlash);
			relativeName = relativeName.substr(foundSlash + 1);
		}

		// adopt the texture, if needed.
		char* fullPath = new char[AA_MAX_STRING];
		PathTypeQuery_t pathTypeQuery;
		g_pFullFileSystem->RelativePathToFullPath(VarArgs("materials/%s.vtf", textureName.c_str()), "GAME", fullPath, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);//FILTER_CULLPACK
		//DevMsg("Full Path: %s and %i\n", fullPath, (int)(pathTypeQuery == PATH_IS_NORMAL));

		if (pathTypeQuery != PATH_IS_NORMAL && !g_pFullFileSystem->FileExists(VarArgs("custom/adopted/materials/%s.vtf", textureName.c_str()), "DEFAULT_WRITE_PATH"))//Q_stricmp(textureName.c_str(), fullPath) && 
		{
			// first, read the texture
			FileHandle_t fh = filesystem->Open(VarArgs("materials/%s.vtf", textureName.c_str()), "rb", "GAME");
			if (fh)
			{
				int file_len = filesystem->Size(fh);
				unsigned char* pImageData = new unsigned char[file_len + 1];

				filesystem->Read((void*)pImageData, file_len, fh);
				pImageData[file_len] = 0; // null terminator

				filesystem->Close(fh);

				g_pFullFileSystem->CreateDirHierarchy(relativePath.c_str(), "DEFAULT_WRITE_PATH");

				FileHandle_t fh2 = filesystem->Open(VarArgs("custom/adopted/materials/%s.vtf", textureName.c_str()), "wb", "DEFAULT_WRITE_PATH");
				if (fh2)
				{
					filesystem->Write(pImageData, file_len, fh2);
					filesystem->Close(fh2);
				}

				// cleanup
				delete[] pImageData;
			}
		}
	}

	this->AddToastMessage(VarArgs("Adopted Skybox Textures: %s", originalTextureName.c_str()), true);
}

void C_AnarchyManager::PickPaintTexture()
{
	if (!m_pInstanceManager->GetCurrentInstance())
		return;

	trace_t tr;
	Vector forward;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->EyeVectors(&forward);
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	if (tr.DidHit())
	{
		IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
		if (!pMaterial || pMaterial->IsErrorMaterial())
			pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

		ITexture* pTexture = null;
		if (pMaterial && !pMaterial->IsErrorMaterial())
		{
			// find the materialvar
			bool bFoundVar;
			IMaterialVar* pMaterialVar = pMaterial->FindVar("$baseTexture", &bFoundVar);

			if (pMaterialVar && bFoundVar && pMaterialVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE)
				pTexture = pMaterialVar->GetTextureValue();
		}
		else if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			if (tr.DidHitWorld())
			{
				/*Ray_t ray;
				ray.Init(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE);

				trace_t trProp;
				staticpropmgr->TraceRayAgainstStaticProp(ray, tr.hitbox, trProp);

				if (trProp.fraction != 1.0)
				{
				DevMsg("Surface: %s\n", trProp.surface.name);
				}*/

				ICollideable* pCollideable = staticpropmgr->GetStaticPropByIndex(tr.hitbox);
				if (pCollideable)
				{
					const model_t* TheModel = pCollideable->GetCollisionModel();

					int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
					if (iNumMaterials < 1)
						iNumMaterials = 32;
					IMaterial** pMaterials = new IMaterial*[iNumMaterials];
					for (unsigned int i = 0; i < iNumMaterials; i++)
						pMaterials[i] = null;
					modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

					bool bFoundVar;
					IMaterialVar* pMaterialVar = null;
					for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
					{
						if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
						{
							// find the materialvar
							pMaterialVar = pMaterials[x]->FindVar("$baseTexture", &bFoundVar);
							if (pMaterialVar && bFoundVar && pMaterialVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE)
							{
								pTexture = pMaterialVar->GetTextureValue();
								if (pTexture && pTexture->IsError())
									pTexture = null;
								else
									break;
							}
						}
					}

					delete[] pMaterials;
				}
			}
			else if (tr.DidHitNonWorldEntity())
			{
				// we have no material yet, but do have an entity.  so apply to ALL materials used on the model.
				const model_t* TheModel = tr.m_pEnt->GetModel();

				int iNumMaterials = modelinfo->GetModelMaterialCount(TheModel);//32;//
				if (iNumMaterials < 1)
					iNumMaterials = 32;
				IMaterial** pMaterials = new IMaterial*[iNumMaterials];
				for (unsigned int i = 0; i < iNumMaterials; i++)
					pMaterials[i] = null;
				modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

				bool bFoundVar;
				IMaterialVar* pMaterialVar = null;
				for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
				{
					if (pMaterials[x] && !pMaterials[x]->IsErrorMaterial())
					{
						// find the materialvar
						pMaterialVar = pMaterials[x]->FindVar("$baseTexture", &bFoundVar);

						if (pMaterialVar && bFoundVar && pMaterialVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE)
						{
							pTexture = pMaterialVar->GetTextureValue();
							if (pTexture && pTexture->IsError())
								pTexture = null;
							else
								break;
						}
					}
				}

				delete[] pMaterials;
			}
		}


		if (pTexture && !pTexture->IsError())
		{
			//char input[AA_MAX_STRING];
			int iAAMaxString = Q_strlen(pTexture->GetName()) + 1;
			char* input = new char[iAAMaxString];
			Q_strncpy(input, pTexture->GetName(), iAAMaxString);

			// Convert it to lowercase & change all slashes to back-slashes
			V_FixSlashes(input, '/');
			std::string textureName = input;
			delete[] input;
			size_t foundSlash = textureName.find('/');
			if (foundSlash == 0)
				textureName = textureName.substr(1);

			std::string relativeName = textureName;
			std::string relativePath = "custom/adopted/materials";

			foundSlash = relativeName.find_last_of("/");
			if (foundSlash != std::string::npos)
			{
				relativePath += "/";
				relativePath += relativeName.substr(0, foundSlash);
				relativeName = relativeName.substr(foundSlash + 1);
			}

			m_pPaintTextureConVar->SetValue(pTexture->GetName());

			// adopt the texture, if needed.
			char* fullPath = new char[AA_MAX_STRING];
			PathTypeQuery_t pathTypeQuery;
			g_pFullFileSystem->RelativePathToFullPath(VarArgs("materials/%s.vtf", textureName.c_str()), "GAME", fullPath, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);//FILTER_CULLPACK
			//DevMsg("Full Path: %s and %i\n", fullPath, (int)(pathTypeQuery == PATH_IS_NORMAL));

			if (pathTypeQuery != PATH_IS_NORMAL && !g_pFullFileSystem->FileExists(VarArgs("custom/adopted/materials/%s.vtf", textureName.c_str()), "DEFAULT_WRITE_PATH"))//Q_stricmp(textureName.c_str(), fullPath) && 
			{
				// first, read the texture
				FileHandle_t fh = filesystem->Open(VarArgs("materials/%s.vtf", textureName.c_str()), "rb", "GAME");
				if (fh)
				{
					int file_len = filesystem->Size(fh);
					unsigned char* pImageData = new unsigned char[file_len + 1];

					filesystem->Read((void*)pImageData, file_len, fh);
					pImageData[file_len] = 0; // null terminator

					filesystem->Close(fh);

					g_pFullFileSystem->CreateDirHierarchy(relativePath.c_str(), "DEFAULT_WRITE_PATH");

					FileHandle_t fh2 = filesystem->Open(VarArgs("custom/adopted/materials/%s.vtf", textureName.c_str()), "wb", "DEFAULT_WRITE_PATH");
					if (fh2)
					{
						filesystem->Write(pImageData, file_len, fh2);
						filesystem->Close(fh2);
						this->AddToastMessage(VarArgs("Paint Texture Set (And Adopted): %s", textureName.c_str()), true);
					}
					else
						this->AddToastMessage(VarArgs("Paint Texture Set: %s", textureName.c_str()), true);

					// cleanup
					delete[] pImageData;
				}
				else
					this->AddToastMessage(VarArgs("Paint Texture Set: %s", textureName.c_str()), true);
			}
			else
				this->AddToastMessage(VarArgs("Paint Texture Set: %s", textureName.c_str()), true);
		}
	}
}

void C_AnarchyManager::OnSave()
{
	DevMsg("AnarchyManager: OnSave\n");
}

void C_AnarchyManager::OnRestore()
{
	DevMsg("AnarchyManager: OnRestore\n");
}
void C_AnarchyManager::SafeRemoveIfDesired()
{
	//DevMsg("AnarchyManager: SafeRemoveIfDesired\n");
}

bool C_AnarchyManager::IsPerFrame()
{
	DevMsg("AnarchyManager: IsPerFrame\n");
	return true;
}

void C_AnarchyManager::PreRender()
{
	if (!this->IsInitialized())
		return;

	//DevMsg("AnarchyManager: PreRender\n");
	if (m_state == AASTATE_RUN)
	{
		// This stuff should only get called if we are NOT paused
		this->CheckPicMip();
		//this->GetPeakAudio();

		//this->VRUpdate();
	}
}

void C_AnarchyManager::SetSelectOriginal(int iEntityIndex)
{
	m_iOriginalSelect = iEntityIndex;
	m_iLastNextDirection = 0;
}

void C_AnarchyManager::IncrementState()
{
	DevMsg("Increment state called.\n");

	if (m_bIncrementState)
		DevMsg("CRITICAL ERROR: State attempted to increment while it was still waiting for the previous increment!\n");

	m_bIncrementState = true;
}

void C_AnarchyManager::VRControllerVibrateStart(int iHandSide, int iPercent)
{
#ifdef VR_ALLOWED
	hmdControllerVibrateStart(iHandSide, iPercent);
#endif
}

void C_AnarchyManager::VRControllerVibrateStop(int iHandSide)
{
#ifdef VR_ALLOWED
	hmdControllerVibrateStop(iHandSide);
#endif
}

//Note that this wil be a matrix from the pose to the default mideye zero
VMatrix C_AnarchyManager::SMMatrixToVMatrix(float matrix[16], int iEyeFactor, bool bNoZOffset)
{
	/*
	float flMetersToGameUnits = 39.3701;
	float flHeightOffset = (bNoZOffset) ? 0.0f : 64.0f;

	VMatrix initialValue = VMatrix(
		matrix[0], matrix[4], matrix[8], matrix[14] - flMetersToGameUnits,
		matrix[1], matrix[5], matrix[9], matrix[12] - flMetersToGameUnits,
		matrix[2], matrix[6], matrix[10], matrix[13] * flMetersToGameUnits - flHeightOffset,
		matrix[3], matrix[7], matrix[11], matrix[15]);

	return initialValue;
	*/



	VMatrix returnValue;
	VMatrix initialValue = VMatrix(
		matrix[0], matrix[4], matrix[8], matrix[12],
		matrix[1], matrix[5], matrix[9], matrix[13],
		matrix[2], matrix[6], matrix[10], matrix[14],
		matrix[3], matrix[7], matrix[11], matrix[15]);

	Vector pos = initialValue.GetTranslation();
	Quaternion rot;

	rot.w = sqrt(1.f + matrix[0] + matrix[5] + matrix[10]) / 2.0;
	double w4 = (4.0 * rot.w);
	rot.x = (matrix[6] - matrix[9]) / w4;
	rot.y = (matrix[2] - matrix[8]) / w4;
	rot.z = (matrix[1] - matrix[4]) / -w4;

	QuaternionNormalize(rot);

	Vector naxis;
	float angle;

	naxis = Vector(rot.x, rot.y, rot.z).Normalized();
	angle = 2 * acosf(rot.w);

	if (angle > (3.1415f)) // Reduce the magnitude of the angle, if necessary
	{
		angle = (3.1415f * 2.f) - angle;
		naxis = naxis * (-1);
	}

	float c = cos(angle);
	float s = sin(angle);
	float t = 1.f - c;

	Vector axis;
	axis.x = naxis.z;
	axis.y = -naxis.x;
	axis.z = -naxis.y;

	//bNoZOffset = true;

	float flMetersToGameUnits = 39.3701;
	float flHeightOffset = (bNoZOffset) ? 0.0f : 64.0f;
	float ipd = (g_pAnarchyManager->GetIPD() / 1000.0) * flMetersToGameUnits;
	float flIPDOffset = iEyeFactor * ipd;
	returnValue = VMatrix(t*axis.x*axis.x + c, t*axis.x*axis.y - axis.z*s, t*axis.x*axis.z + axis.y*s, pos.z * -flMetersToGameUnits,
	t*axis.x*axis.y + axis.z*s, t*axis.y*axis.y + c, t*axis.y*axis.z - axis.x*s, (pos.x * -flMetersToGameUnits) + flIPDOffset,
		t*axis.x*axis.z - axis.y*s, t*axis.y*axis.z + axis.x*s, t*axis.z*axis.z + c, pos.y * flMetersToGameUnits - flHeightOffset,
		0.f, 0.f, 0.f, 1.f);

	return returnValue;

	/*
	VMatrix inver, delta;
	MatrixInverseTR(initialValue, inver);
	delta = returnValue * inver;
	delta.GetTranslation();
	//Do something with this ^, maybe?
	*/

	//if (isHand) {	//Skew the rotation if it's a weapon
		/*
		matrix3x4_t rot;
		SetIdentityMatrix(rot);
		if (!isLeftSpace) {
			MatrixSetTranslation(Vector(handOffset.x, -handOffset.y, handOffset.z), rot);
		}
		else {
			MatrixSetTranslation(handOffset, rot);
		}
		returnValue = returnValue * rot;

		MatrixBuildRotationAboutAxis(Vector(0.f, 1.f, 0.f), hlvr_hand_pitch.GetFloat(), rot);
		returnValue = returnValue * rot;
		*/
	//}
	/*
	if (gPlayer && !roomscale) {
		engine->ClientCmd_Unrestricted("hlvr_resetposition\n");
		if (false){//gPlayer->m_Local.m_vecHeadApplied != Vector(0.f, 0.f, 0.f) && gPlayer->m_Local.m_vecHeadApplied.x == gPlayer->m_eyeOffset.x && gPlayer->m_Local.m_vecHeadApplied.y == gPlayer->m_eyeOffset.y){
			roomscale = true;
			engine->ClientCmd_Unrestricted("hlvr_roomscale_on\n");
		}
	}*/

	//VMatrix addition;
	//MatrixBuildRotateZ(addition, rotationOffset + joyOffset + vehicleOffset);							//Game yaw offset

	//if (gPlayer && roomscale) addition.SetTranslation(-gPlayer->m_Local.m_vecHeadApplied);				//Recenter in game, with roomscale.
	//returnValue = addition * returnValue;

	//return returnValue;
}

VMatrix C_AnarchyManager::GetMidEyeFromEye(ISourceVirtualReality::VREye eEye)
{
	float flMetersToGameUnits = 39.3701;
	return VMatrix(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, ((m_pIPDConVar->GetFloat() / 1.0) / flMetersToGameUnits) * ((eEye == 0) ? 1.f : -1.f),
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f);
}

//#define VR_BACK_BUFFER "_rt_two_eyes_VR"
//#define FULL_MAIN_FB "_rt_FullFrameFB"
void C_AnarchyManager::RenderVREyeToView(const CViewSetup &view, ISourceVirtualReality::VREye eyeToCopy)// , bool isMenuUp)
{
	CMatRenderContextPtr pRenderContext(materials);

	//Tried to overlay UI on VR eye
	//for (bool menuPass = false; (int)menuPass <= (int)isMenuUp; menuPass = true) 
	//bool menuPass = isMenuUp;
	//bool menuPass = false;
	//{
	//ITexture* pVRFullFrame = materials->FindTexture(VR_BACK_BUFFER_0, TEXTURE_GROUP_RENDER_TARGET);

	/*int currentRenderBufferIndex = hmdGetCurrentTextureIndex() - 1;
	if (currentRenderBufferIndex < 0)
		currentRenderBufferIndex = 2;

	const char* currentRenderBufferTextureName = this->BackBufferNamePerIndex(currentRenderBufferIndex);
	ITexture* pVRFullFrame = materials->FindTexture(currentRenderBufferTextureName, TEXTURE_GROUP_RENDER_TARGET);*/
	ITexture* pVRFullFrame = materials->FindTexture("_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET);

	//ITexture* pVRFullFrame = m_pInputManager->GetEmbeddedInstance()->GetTexture();

	ITexture* pMainFullFrame = materials->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);
	ITexture* pOldTexture = pRenderContext->GetRenderTarget();
	pRenderContext->SetRenderTarget(null);// pMainFullFrame);

	Rect_t	DestinationRect, SourceRect;
	
	/* THIS CODE MAKES THE BACKGROUND MATCH THE TOP-LEFT PIXEL COLOR
	SourceRect.width = 1;
	SourceRect.height = 1;
	SourceRect.x = 0;
	SourceRect.y = 0;

	DestinationRect.x = 0;
	DestinationRect.y = 0;
	DestinationRect.height = pMainFullFrame->GetActualHeight();
	DestinationRect.width = pMainFullFrame->GetActualWidth();
	pRenderContext->CopyTextureToRenderTargetEx(0, pVRFullFrame, &SourceRect, &DestinationRect);
	*/


	SourceRect.width = pVRFullFrame->GetActualWidth() / 2;
	SourceRect.height = pVRFullFrame->GetActualHeight();
	SourceRect.x = (eyeToCopy == ISourceVirtualReality::VREye::VREye_Left) ? 0 : pVRFullFrame->GetActualWidth() / 2;
	SourceRect.y = 0;
		
		//DevMsg("%i\n", )

		DestinationRect.height = pMainFullFrame->GetActualHeight();
		DestinationRect.width = SourceRect.width * (DestinationRect.height / SourceRect.height) * 1.7;

		DestinationRect.x = (pMainFullFrame->GetActualWidth()/2) - (DestinationRect.width/2);
		DestinationRect.y = 0;

		/*
		SourceRect.width = 1920;
		SourceRect.height = 1080;
		SourceRect.x = 1920;
		SourceRect.y = 1080;
		*/

		///*
		//*/

		pRenderContext->CopyTextureToRenderTargetEx(0, pVRFullFrame, &SourceRect, &DestinationRect);
	//}
		pRenderContext->SetRenderTarget(pOldTexture);// pMainFullFrame);
}

void C_AnarchyManager::RenderMonoEyeToVRSpectatorCamera(const CViewSetup &view)
{
	CMatRenderContextPtr pRenderContext(materials);

	ITexture* pVRFullFrame = materials->FindTexture("_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET);
	ITexture* pVRSpectatorFrame = materials->FindTexture(VR_SPECTATOR_CAMERA, TEXTURE_GROUP_RENDER_TARGET);

	ITexture* pOldTexture = pRenderContext->GetRenderTarget();
	pRenderContext->SetRenderTarget(pVRSpectatorFrame);

	Rect_t	DestinationRect, SourceRect;
	SourceRect.width = pVRFullFrame->GetActualWidth();//ScreenWidth();
	SourceRect.height = pVRFullFrame->GetActualHeight();//ScreenHeight();
	SourceRect.x = 0;
	SourceRect.y = 0;

	DestinationRect.width = pVRSpectatorFrame->GetActualWidth();//ScreenWidth();
	DestinationRect.height = pVRSpectatorFrame->GetActualHeight();//ScreenHeight();
	DestinationRect.x = 0;
	DestinationRect.y = 0;

	pRenderContext->CopyTextureToRenderTargetEx(0, pVRFullFrame, &SourceRect, &DestinationRect);
	pRenderContext->SetRenderTarget(pOldTexture);
}

// --------------------------------------------------------------------
// Purpose: Computes the FOV from the projection matrix
// --------------------------------------------------------------------
void C_AnarchyManager::CalcFovFromProjection(float *pFov, const VMatrix &proj)
{
	// The projection matrices should be of the form:
	// p0  0   z1 p1 
	// 0   p2  z2 p3
	// 0   0   z3 1
	// (p0 = X fov, p1 = X offset, p2 = Y fov, p3 = Y offset )
	// TODO: cope with more complex projection matrices?
	float xscale = proj.m[0][0];
	Assert(proj.m[0][1] == 0.0f);
	float xoffset = proj.m[0][2];
	Assert(proj.m[0][3] == 0.0f);
	Assert(proj.m[1][0] == 0.0f);
	float yscale = proj.m[1][1];
	float yoffset = proj.m[1][2];
	Assert(proj.m[1][3] == 0.0f);
	// Row 2 determines Z-buffer values - don't care about those for now.
	Assert(proj.m[3][0] == 0.0f);
	Assert(proj.m[3][1] == 0.0f);
	Assert(proj.m[3][2] == -1.0f);
	Assert(proj.m[3][3] == 0.0f);

	/*
	// The math here:
	// A view-space vector (x,y,z,1) is transformed by the projection matrix
	// / xscale   0     xoffset  0 \
	// |    0   yscale  yoffset  0 |
	// |    ?     ?        ?     ? |
	// \    0     0       -1     0 /
	//
	// Then the result is normalized (i.e. divide by w) and the result clipped to the [-1,+1] unit cube.
	// (ignore Z for now, and the clipping is slightly different).
	// So, we want to know what vectors produce a clip value of -1 and +1 in each direction, e.g. in the X direction:
	//    +-1 = ( xscale*x + xoffset*z ) / (-1*z)
	//        = xscale*(x/z) + xoffset            (I flipped the signs of both sides)
	// => (+-1 - xoffset)/xscale = x/z
	// ...and x/z is tan(theta), and theta is the half-FOV.
	*/

	float fov_px = 2.0f * RAD2DEG(atanf(fabsf((1.0f - xoffset) / xscale)));
	float fov_nx = 2.0f * RAD2DEG(atanf(fabsf((-1.0f - xoffset) / xscale)));
	float fov_py = 2.0f * RAD2DEG(atanf(fabsf((1.0f - yoffset) / yscale)));
	float fov_ny = 2.0f * RAD2DEG(atanf(fabsf((-1.0f - yoffset) / yscale)));

	*pFov = Max(Max(fov_px, fov_nx), Max(fov_py, fov_ny));
	// FIXME: hey you know, I could do the Max() series before I call all those expensive atanf()s...
}

ConVar selector_ray_enabled("selector_ray_enabled", "1", FCVAR_NONE);
void C_AnarchyManager::Update(float frametime)
{
	if (m_bIncrementState)
	{
		m_bIncrementState = false;

		DevMsg("Incrementing internal state from %i...\n", (int)m_state);

		switch (m_state)
		{
			case AASTATE_NONE:
				DevMsg("Incrementing to state AASTATE_STATS\n");
				m_state = AASTATE_STATS;
				break;

			case AASTATE_STATS:
				DevMsg("Incrementing to state AASTATE_INPUTMANAGER\n");
				m_state = AASTATE_INPUTMANAGER;
				break;

			case AASTATE_INPUTMANAGER:
				DevMsg("Incrementing to state AASTATE_CANVASMANAGER\n");
				m_state = AASTATE_CANVASMANAGER;
				break;

			case AASTATE_CANVASMANAGER:
				DevMsg("Incrementing to state AASTATE_LIBRETROMANAGER\n");
				m_state = AASTATE_LIBRETROMANAGER;
				break;

			case AASTATE_LIBRETROMANAGER:
				DevMsg("Incrementing to state AASTATE_STEAMBROWSERMANAGER\n");
				m_state = AASTATE_STEAMBROWSERMANAGER;
				break;

			case AASTATE_STEAMBROWSERMANAGER:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGER\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGER;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGER:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERWAIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERWAIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERWAIT:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERHUD\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERHUD;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERHUD:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERHUDINIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERHUDINIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERHUDINIT:
				if (!cvar->FindVar("disable_multiplayer")->GetBool())
				{
					DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERNETWORK\n");
					m_state = AASTATE_AWESOMIUMBROWSERMANAGERNETWORK;
				}
				else
				{
					DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERIMAGES\n");
					m_state = AASTATE_AWESOMIUMBROWSERMANAGERIMAGES;
				}
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERNETWORK:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERNETWORKINIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERNETWORKINIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERNETWORKINIT:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERIMAGES\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERIMAGES;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERIMAGES:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT:
				DevMsg("Incrementing to state AASTATE_AWESOMIUMBROWSERMANAGERIMAGESINIT\n");
				m_state = AASTATE_AWESOMIUMBROWSERMANAGERIMAGESINIT;
				break;

			case AASTATE_AWESOMIUMBROWSERMANAGERIMAGESINIT:
				DevMsg("Incrementing to state AASTATE_RUN\n");
				m_state = AASTATE_RUN;
				break;
		}
	}

	m_flCurrentTime = engine->Time();

	switch (m_state)
	{
		case AASTATE_RUN:
			if (m_bPaused)	// FIXME: You might want to let the web manager do its core logic, but don't render anything. This is because Awesomimue queues API calls until the next update is called.
			{
				if (m_pAwesomiumBrowserManager)
					m_pAwesomiumBrowserManager->Update();

				// NOTE: NO logic is actually done in the metaverse manager's update method if the game is paused, but calling it here just for consistency sake.
				if (m_pMetaverseManager)
					m_pMetaverseManager->Update();

				if (m_pCanvasManager)// && this->IsPaused())
				{
					m_pCanvasManager->Update();
					//m_pCanvasManager->CleanupTextures();
				}

				return;	// TODO: Nothing else but texture proxy & callback-induced code needs to worry about paused mode now.
			}

			//DevMsg("Float: %f\n", frametime);	// deltatime
			//DevMsg("Float: %i\n", gpGlobals->framecount);	// numframes total

			this->GetPeakAudio();
			this->IncrementHueShifter(frametime);
			this->ManageAlwaysLookObjects();

			if (m_pCanvasManager)
			{
				if (m_pCanvasManager && !m_pCanvasManager->GetDisplayInstance())
					m_pCanvasManager->SetDifferentDisplayInstance(null);

				m_pCanvasManager->Update();
			}

			if (m_pLibretroManager)
				m_pLibretroManager->Update();

			if (m_pSteamBrowserManager)
				m_pSteamBrowserManager->Update();

			if (m_pAwesomiumBrowserManager)
				m_pAwesomiumBrowserManager->Update();

			if (m_pInstanceManager)
				m_pInstanceManager->Update();

			if (m_pMetaverseManager)
				m_pMetaverseManager->Update();

			if (m_pBackpackManager)
				m_pBackpackManager->Update();

			if (m_pInputManager)
				m_pInputManager->Update();

			if (m_pQuestManager && engine->IsInGame())
				m_pQuestManager->Update();

			//g_pAnarchyManager->CheckPicMip();
			/*
			if (m_pWebManager)
				m_pWebManager->Update();
			*/
			
			if (m_fNextToastExpiration > 0 && m_fNextToastExpiration <= m_flCurrentTime)
				this->PopToast();

			if ((m_fHoverTitleExpiration > 0 && m_fHoverTitleExpiration <= m_flCurrentTime) || (!m_pHoverTitlesConVar->GetBool() && m_hoverTitle != ""))
			{
				m_hoverTitle = "";
				m_fHoverTitleExpiration = 0;
				this->UpdateHoverLabel();
			}

			ManageHUDLabels();
			UpdateSystemTimeState();	// more like MANAGE than actually updating, as sometimes it doesn't always need to ACTUALLY update.
			UpdateNumberStatsState();


			if (engine->IsInGame() && m_flSpawnObjectsButtonDownTime > 0.0f && engine->Time() - m_flSpawnObjectsButtonDownTime >= 1.0f)
			{
				this->GetInstanceManager()->SpawnActionPressed(true);
				m_flSpawnObjectsButtonDownTime = 0.0f;
			}

			if (engine->IsInGame() && m_flComparisonRenderTime >= 0 && gpGlobals->curtime - m_flComparisonRenderTime >= 1.0)
			{
				this->OnComparisonRenderFinished();
			}

			if (engine->IsInGame() && m_flNextAttractCameraTime > 0.0f && m_flNextAttractCameraTime < engine->Time())
			{
				m_flNextAttractCameraTime = -1.0f;
				this->FindNextAttractCamera();
			}

			if (engine->IsInGame() && m_flStartQuestsSoon > 0.0f && m_flStartQuestsSoon < engine->Time())
			{
				m_flStartQuestsSoon = 0.0f;
				m_pQuestManager->InitializeAndBeginAllQuests();
			}

			if (engine->IsInGame())
			{
				m_pQuestManager->UpdateQueue();
			}

			if (engine->IsInGame() && m_pInspectShortcut) {
				this->InspectModeTick(frametime);
			}

			this->FetchGlobalStats();

			//ManageAudioStrips();

			if (selector_ray_enabled.GetBool())
			{
#ifdef VR_ALLOWED
				if (!this->IsVRActive() || !this->IsHandTrackingActive())
					engine->ClientCmd("selector_trace");
				else if (pVRHandRight)
					engine->ClientCmd(VarArgs("selector_trace %i", pVRHandRight->entindex()));
				else
					engine->ClientCmd("selector_trace");
#else
				engine->ClientCmd("selector_trace");
#endif
			}

			if (m_flCurrentTime >= m_flStatTime)
			{
				//if (m_iTotalHours < 0)
				//	m_iTotalHours = m_pAccountant->GetStat("aa_total_time")->iValue;

				int iCurrentHours = (m_flCurrentTime - m_flStartTime) / 60 / 60;
				
				if (iCurrentHours > m_iTotalHours)
				{
					aaStat* pStat = m_pAccountant->GetStat("aa_total_time");
					m_pAccountant->Action("aa_total_time", (iCurrentHours - m_iTotalHours));
					m_iTotalHours = iCurrentHours;

					pStat = m_pAccountant->GetStat("aa_marathon_time");
					if (iCurrentHours > pStat->iValue)
						m_pAccountant->Action("aa_marathon_time", iCurrentHours - pStat->iValue);
				}

				m_flStatTime = engine->Time() + (60 * 60);
			}


			ManageGamepadInput(frametime);
			ManageHoverLabel();
			ManageImportScans();
			ManagePanoshot();
			ManageExtractOverview();

			if (m_fNextWindowManage > 0 && m_fNextWindowManage <= m_flCurrentTime)
				ManageWindow();

			//DevMsg("AnarchyManager: Update\n");
			break;

		case AASTATE_STATS:
			if (!m_pAccountant)
			{
				m_pAccountant = new C_Accountant();
				m_pAccountant->Init();
			}
			break;

		case AASTATE_INPUTMANAGER:
			m_pInputManager = new C_InputManager();	// then wait for state change
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_CANVASMANAGER:
			m_pCanvasManager = new C_CanvasManager();	// then wait for state change
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_LIBRETROMANAGER:
			m_pLibretroManager = new C_LibretroManager();
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_STEAMBROWSERMANAGER:
			m_pSteamBrowserManager = new C_SteamBrowserManager();
			if (m_pSteamBrowserManager->IsSupported())
				g_pAnarchyManager->IncrementState();
			else
			{
				m_state = AASTATE_NONE;
				m_bIncrementState = false;
			}
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGER:
			m_pAwesomiumBrowserManager = new C_AwesomiumBrowserManager();
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERWAIT:
			m_pCanvasManager->Update();
			m_pAwesomiumBrowserManager->Update();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERHUD:
			m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance("hud", "asset://ui/hud.html", "AArcade HUD", true);
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT:
			m_pCanvasManager->Update();
			m_pAwesomiumBrowserManager->Update();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERHUDINIT:
			DevMsg("Finished initing HUD.\n");

			m_pInstanceManager = new C_InstanceManager();
			m_pMetaverseManager = new C_MetaverseManager();
			//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Meta Scrapers", "loadmetascrapers", "", "", "69", "loadNextMetaScraper");
			m_pMetaverseManager->UpdateScrapersJS();	// make sure everything that hud.js loads is ready right away, to avoid cache issues

			m_pAAIManager = new C_AAIManager();
			m_pAAIManager->Init();

			//m_pMetaverseManager->Init();	// try to delay this until after startup
			m_pBackpackManager = new C_BackpackManager();
			m_pInputManager = new C_InputManager();

			// insta-startup the Window Manager for now, because it's just a placeholder until actual in-game deskto rendering can be done.
			m_pWindowManager = new C_WindowManager();
			m_pWindowManager->Init();

			// let the quest manager get ready, if it needs to.
			m_pQuestManager = new C_QuestManager();
			m_pQuestManager->Init();

			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERNETWORK:
			m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance("network", "asset://ui/network.html", "AArcade Network", true);
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT:
			m_pAwesomiumBrowserManager->Update();
			//this->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERNETWORKINIT:
			DevMsg("Finished initing NETWORK.\n");
			this->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERIMAGES:
			m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance("images", "asset://ui/imageLoader.html", "AArcade Image Renderer", true);	// defaults to asset://ui/blank.html	// does this need to be created here????
			g_pAnarchyManager->IncrementState();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT:
			//if (m_pCanvasManager)
			m_pCanvasManager->Update();

			m_pAwesomiumBrowserManager->Update();
			break;

		case AASTATE_AWESOMIUMBROWSERMANAGERIMAGESINIT:
			DevMsg("Finished initing IMAGES.\n");

			// auto-load aarcade stuff
			g_pAnarchyManager->RunAArcade();

			this->IncrementState();
			break;
	}

	this->VRUpdate();
}

void C_AnarchyManager::ManageGamepadInput(float flFrametime)
{
	if ((!m_pInputManager->IsGamepadInputMode() && (!this->IsVRActive() || !this->IsHandTrackingActive())) || !m_pInputManager->GetInputCapture())//!vgui::surface()->IsCursorVisible() || 
		return;

	//->ExtraMouseSample();

	if (!this->IsVRActive() || !this->IsHandTrackingActive())
	{
		if (!m_bPreviousJoystickClickDown)
		{
			if (vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER))
			{
				m_bPreviousJoystickClickDown = true;
				m_pInputManager->MousePress(MOUSE_LEFT);
			}
		}
		else
		{
			if (!vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER))
			{
				m_bPreviousJoystickClickDown = false;
				m_pInputManager->MouseRelease(MOUSE_LEFT);
			}
		}
	}

	// Get the last mouse position
	if (fabs(m_flPreviousJoystickSide) > 0.1 || fabs(m_flPreviousJoystickForward) > 0.1)
	{
		//DevMsg("Vertical: (%f) Horizontal: (%f)\n", m_flPreviousJoystickForward, m_flPreviousJoystickSide);// , m_flPreviousJoystickPitch, m_flPreviousJoystickYaw);

		C_EmbeddedInstance* pEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
		//if (!pEmbeddedInstance)
			pEmbeddedInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		if (pEmbeddedInstance)
		{
			float flLastMouseX;
			float flLastMouseY;
			pEmbeddedInstance->GetLastMouse(flLastMouseX, flLastMouseY);
			//DevMsg("Frame time: %f\n", );
			float flAbsoluteFrametime = gpGlobals->absoluteframetime;
			flLastMouseX += m_flPreviousJoystickSide * flAbsoluteFrametime;
			flLastMouseY += (-m_flPreviousJoystickForward) * flAbsoluteFrametime;

			if (flLastMouseX > 1.0f)
				flLastMouseX = 1.0f;
			else if (flLastMouseX < 0)
				flLastMouseX = 0.0f;

			if (flLastMouseY > 1.0f)
				flLastMouseY = 1.0f;
			else if (flLastMouseY < 0)
				flLastMouseY = 0.0f;

			m_pInputManager->MouseMove(flLastMouseX, flLastMouseY);
			//int iVal = inputsystem->GetAnalogValue((AnalogCode_t)JOYSTICK_AXIS(0, KEY_XBUTTON_RTRIGGER));
			//DevMsg("IVal is: %i\n", iVal);
			//vgui::input()->>Key //>IsKeyDown(KEY_XBUTTON_START)
		}
	}
}

#include "../game/client/input.h"
ConVar vrsnapturn("vrsnapturn", "1", FCVAR_ARCHIVE, "VR comfort mode turning snaps you to 90 degree increments.");
ConVar vrfreemove("vrfreemove", "0", FCVAR_ARCHIVE, "VR non-comfort mode has free game-style locomotion.");
void C_AnarchyManager::VRMutateGamepadInput(float* flPreviousJoystickForward, float* flPreviousJoystickSide, float* flPreviousJoystickPitch, float* flPreviousJoystickYaw)
{
	if (!this->IsVRActive() || !this->IsHandTrackingActive() || this->GetMetaverseManager()->GetSpawningObjectEntity() || (m_pInputManager->GetInputMode() && m_pInputManager->GetInputCapture()))
		return;

	float flYaw, flForward, flSide;
	if (vrsnapturn.GetBool())
	{
		flYaw = 0;

		if (abs(m_flPreviousJoystickYaw) > 0.5)
		{
			if (!m_bWasVRSnapTurn)
			{
				m_iVRSnapTurn = (m_flPreviousJoystickYaw > 0) ? 1 : -1;
				m_bWasVRSnapTurn = true;
			}
		}
		else
			m_bWasVRSnapTurn = false;
	}
	else
		flYaw = -m_flPreviousJoystickYaw;

	if (vrfreemove.GetBool())
	{
		flForward = -m_flPreviousJoystickForward;
		flSide = m_flPreviousJoystickSide;
	}
	else
	{
		flForward = 0;
		flSide = 0;

		if (m_bWasVRTeleport && pVRTeleport && pVRPointerRight)
		{
			if (abs(m_flPreviousJoystickForward) + abs(m_flPreviousJoystickSide) < 0.5)
			{
				m_bWasVRTeleport = false;
				m_bNeedVRTeleport = true;

				//C_BaseEntity* pEntity = (m_iHoverEntityIndex >= 0) ? C_BaseEntity::Instance(m_iHoverEntityIndex) : null;
				//C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				//if (!pShortcut)
				if (!m_bHoverEntityShortcut)
					engine->ClientCmd(VarArgs("set_render_mode %i 1; set_render_mode %i 1;\n", pVRTeleport->entindex(), pVRPointerRight->entindex()));
				else
					engine->ClientCmd(VarArgs("set_render_mode %i 1\n", pVRTeleport->entindex()));
				//pVRTeleport->SetRenderMode(kRenderNone, true);
			}
		}
		else if (m_flPreviousJoystickForward > 0.7)
		{
			if (!m_bWasVRTeleport && pVRTeleport && pVRPointerRight)
				engine->ClientCmd(VarArgs("set_render_mode %i 0; set_render_mode %i 0;\n", pVRTeleport->entindex(), pVRPointerRight->entindex()));

			m_bWasVRTeleport = true;
		}
		else
			m_bWasVRTeleport = false;
	}

	*flPreviousJoystickForward = flForward;
	*flPreviousJoystickSide = flSide;
	*flPreviousJoystickPitch = 0;// m_flPreviousJoystickPitch;
	*flPreviousJoystickYaw = flYaw;

	VRGamepadInputPostProcess();
}

void C_AnarchyManager::UpdateSystemTimeState()
{
	// only update if we really need to (ie. if at least 1 second has passed since the last update - or if we haven't updated yet.)
	if (!engine->IsInGame() || (m_flNextSystemTimeUpdateTime > 0.0f && m_flNextSystemTimeUpdateTime > m_flCurrentTime))
		return;
	
	// system time
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);

	int iDay = now->tm_wday;
	int iDate = now->tm_mday;
	int iMonths = now->tm_mon;
	int iYears = 1900 + now->tm_year;
	int iPostfix = 0;
	int iHours = now->tm_hour;
	if (iHours > 11)
	{
		iPostfix = 1;
		iHours = iHours - 12;
	}

	int iMinutes = now->tm_min;
	int iSeconds = now->tm_sec;

	m_systemTimeState.iDate = iDate;
	m_systemTimeState.iDay = iDay;
	m_systemTimeState.iHours = iHours;
	m_systemTimeState.iMinutes = iMinutes;
	m_systemTimeState.iMonths = iMonths;
	m_systemTimeState.iPostfix = iPostfix;
	m_systemTimeState.iSeconds = iSeconds;
	m_systemTimeState.iYears = iYears;

	m_flNextSystemTimeUpdateTime = m_flCurrentTime + 1.0f;
}

void C_AnarchyManager::UpdateNumberStatsState(int iNumberStatType)
{
	// TODO: Figure out a good way to use iNumberStatType to reduce calls to stats in the future that cause a lot of overhead.

	// only do this if in a game.
	if (!engine->IsInGame())
		return;

	// only update once every 5 seconds
	if (m_flNextSystemTimeUpdateTime > 0.0f && m_flNextNumberStatsUpdateTime > m_flCurrentTime)
		return;

	m_numberStatsState.iObjects = this->GetInstanceManager()->GetInstanceObjectCount();
	m_numberStatsState.iGlobalPlayers = this->GetSteamPlayerCountNumber();

	m_numberStatsState.iLibraryItems = this->GetMetaverseManager()->GetLibraryItemsCount();
	m_numberStatsState.iLibraryMaps = this->GetMetaverseManager()->GetLibraryMapsCount();
	m_numberStatsState.iLibraryModels = this->GetMetaverseManager()->GetLibraryModelsCount();

	if (m_flNextGlobalStatRefreshTime <= 0.0f || m_flNextGlobalStatRefreshTime <= m_flCurrentTime)
	{
		std::vector<int64> stats;
		g_pAnarchyManager->GetAccountant()->GetGlobalStatHistory("aa_total_time", 2, stats);

		// tally up the days of stats, if needed.
		int64 iTotal = 0;
		if (stats.size() > 1)
			iTotal = stats[1];
		//for (unsigned int i = 0; i < stats.size(); i++)
		//	iTotal += stats[i];

		m_numberStatsState.iGlobalTime = iTotal;
		stats.clear();

		g_pAnarchyManager->GetAccountant()->GetGlobalStatHistory("aa_tubes_watched", 2, stats);

		iTotal = 0;
		if (stats.size() > 1)
			iTotal = stats[1];
		//for (unsigned int i = 0; i < stats.size(); i++)
		//	iTotal += stats[i];

		m_numberStatsState.iGlobalTubes = iTotal;
		stats.clear();

		m_flNextGlobalStatRefreshTime = m_flCurrentTime + 60.0f;
	}

	m_flNextNumberStatsUpdateTime = m_flCurrentTime + 1.0f;
}

void C_AnarchyManager::VRGamepadInputPostProcess()
{
	//if (m_bWasVRTeleport)//if (!vrfreemove.GetBool())
	//{
	//	DevMsg("F/S: %f / %f\n", m_flPreviousJoystickForward, m_flPreviousJoystickSide);
		//flForward = -m_flPreviousJoystickForward;
		//flSide = m_flPreviousJoystickSide;
	//}

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (m_bNeedVRTeleport)
	{
		m_bNeedVRTeleport = false;

		//DevMsg("Do teleport!!\n");

		Vector origin = pVRTeleport->GetAbsOrigin();
		QAngle angles = pVRTeleport->GetAbsAngles();
		float flYaw = angles.y;


		//origin = pPlayer->GetAbsOrigin();
		angles = pPlayer->EyeAngles();
		angles.y = flYaw;

		engine->ClientCmd(VarArgs("teleport_player %i %f %f %f %f %f %f\n", pPlayer->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
		//pPlayer->SetAbsOrigin(origin);
		//pPlayer->SetAbsAngles(angles);
		//render->SetMainView(origin, angles);
		//engine->ClientCmd(VarArgs("teleport_player %i %f %f %f 0 0 0\n", pPlayer->entindex(), origin.x, origin.y, origin.z));
		//engine->ClientCmd(VarArgs("teleport_player %i %f %f %f %f %f %f\n", pPlayer->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));

		//origin = pPlayer->GetAbsOrigin();
		//engine->ClientCmd(VarArgs("teleport_player %i %f %f %f %f %f %f\n", pPlayer->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
		//engine->ClientCmd("escape; escape;");
		//engine->ClientCmd("+left; -left;");
		//render->SetMainView(origin, angles);

		//KeyDown(in_left);
		//in_left->state |= 1 + 2;	// down + impulse down
		//in_left->state &= ~1;		// now up
		//in_left->state |= 4; 		// impulse up


		// Let the client mode at the mouse input before it's used
		//input->AccumulateMouse();
		//g_pClientMode->OverrideMouseInput(&mouse_x, &mouse_y);

		// Add mouse X/Y movement to cmd
		//ApplyMouse(viewangles, cmd, mouse_x, mouse_y);

		// Re-center the mouse.
		//ResetMouse();
	}
	
	if (vrsnapturn.GetBool() && m_iVRSnapTurn != 0) //(m_bWasVRSnapTurn)
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		//QAngle angles = pPlayer->GetAbsAngles();
		QAngle angles = pPlayer->EyeAngles();
		angles.y += 45 * (float)m_iVRSnapTurn;
		m_iVRSnapTurn = 0;

		Vector origin = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		//QAngle angles = C_BasePlayer::GetLocalPlayer()->GetAbsAngles();

		/*
		Vector origin = C_BasePlayer::GetLocalPlayer()->EyePosition();
		QAngle angles = C_BasePlayer::GetLocalPlayer()->EyeAngles();

		char buf[AA_MAX_STRING];
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
		pInfoKV->SetString("camera/position", buf);

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
		pInfoKV->SetString("camera/rotation", buf);

		Vector bodyOrigin = pPlayer->GetAbsOrigin();
		QAngle bodyAngles = pPlayer->GetAbsAngles();

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", bodyOrigin.x, bodyOrigin.y, bodyOrigin.z);
		pInfoKV->SetString("body/position", buf);

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", bodyAngles.x, bodyAngles.y, bodyAngles.z);
		pInfoKV->SetString("body/rotation", buf);
		*/

		//pPlayer->SetAbsAngles(angles);
		engine->ClientCmd(VarArgs("teleport_player %i %f %f %f %f %f %f\n", pPlayer->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
	}
}

void C_AnarchyManager::UpdateGamepadAxisInput(float flPreviousJoystickForward, float flPreviousJoystickSide, float flPreviousJoystickPitch, float flPreviousJoystickYaw)
{
	m_flPreviousJoystickForward = -flPreviousJoystickForward;
	m_flPreviousJoystickSide = flPreviousJoystickSide;
	m_flPreviousJoystickPitch = flPreviousJoystickPitch;
	m_flPreviousJoystickYaw = flPreviousJoystickYaw;

	if (this->IsVRActive() && this->IsHandTrackingActive() && this->GetMetaverseManager()->GetSpawningObjectEntity())
	{
		float flRotAcceleration = 1.3;
		float flMoveAcceleration = 0.5;
		//float flScaleAcceleration = 0.05;

		transform_t* transform = this->GetInstanceManager()->GetTransform();
		transform->rotP += -flPreviousJoystickForward * flRotAcceleration;
		transform->rotY += flPreviousJoystickSide * flRotAcceleration;
		transform->offX += -flPreviousJoystickPitch * flMoveAcceleration;
		//transform->scale += flPreviousJoystickYaw * flScaleAcceleration;
	}
	//vgui::input()->Key //>IsKeyDown(KEY_XBUTTON_START)
}

void C_AnarchyManager::ManageHoverLabel()
{
	if (!m_pHoverLabel)
		return;

	float fHoverTitleDuration = 4.0;
	bool bChanged = false;
	int iHoverEntityIndex = -1;
	bool bHoverEntityShortcut = false;
	std::string hoverTitle = "";
	bool bIsRemoteOnlyItem = false;
	bool bIsRemoteOnlyModel = false;

	if (!m_pHoverTitlesConVar->GetBool() && (!this->IsVRActive() || !this->IsHandTrackingActive()))
	{
		if (m_iHoverEntityIndex != -1)
			bChanged = true;
		else
			return;
	}
	else
	{
		// get the entity under the player's crosshair
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		if (pPlayer->GetHealth() <= 0)
			return;

		int iEntityIndex = -1;
		C_BaseEntity* pEntity = C_BaseEntity::Instance(m_iSelectorTraceEntityIndex);
		if (pEntity)
			iEntityIndex = m_iSelectorTraceEntityIndex;

		/*
		// fire a trace line
		trace_t tr;
		this->SelectorTraceLine(tr);

		int iEntityIndex;
		if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
		{
			pEntity = tr.m_pEnt;
			iEntityIndex = pEntity->entindex();
		}
		*/

		if (pEntity)
		{
			//if (m_iHoverEntityIndex == iEntityIndex)
			//hoverTitle = m_hoverTitle;
			if (m_iHoverEntityIndex != iEntityIndex)
			{
				bChanged = true;

				C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				if (pShortcut)
				{
					bHoverEntityShortcut = true;

					std::string itemId = pShortcut->GetItemId();

					C_EmbeddedInstance* pDisplayInstance = m_pCanvasManager->GetDisplayInstance();
					std::string displayItemId = (pDisplayInstance) ? pDisplayInstance->GetOriginalItemId() : "";

					object_t* pObject = m_pInstanceManager->GetInstanceObject(pShortcut->GetObjectId());
					if (itemId != "" && (displayItemId == "" || !pObject->slave || pEntity == m_pSelectedEntity || itemId == displayItemId))
					{
						// an item.
						KeyValues* pItemKV = this->GetMetaverseManager()->GetActiveKeyValues(this->GetMetaverseManager()->GetLibraryItem(itemId));
						if (pItemKV)
						{
							hoverTitle = pItemKV->GetString("title");
							bIsRemoteOnlyItem = this->GetMetaverseManager()->IsItemRemoteOnly(itemId);
						}
						else
						{
							KeyValues* pModelKV = this->GetMetaverseManager()->GetActiveKeyValues(this->GetMetaverseManager()->GetLibraryModel(itemId));
							if (pModelKV)
							{
								hoverTitle = pModelKV->GetString("title");
								bIsRemoteOnlyModel = this->GetMetaverseManager()->IsModelRemoteOnly(itemId);
							}
						}
					}
					else if (displayItemId != "")
					{
						KeyValues* pItemKV = this->GetMetaverseManager()->GetActiveKeyValues(this->GetMetaverseManager()->GetLibraryItem(displayItemId));
						if (pItemKV)
						{
							hoverTitle = pItemKV->GetString("title");// std::string(pItemKV->GetString("title")) + " (slave)";
							bIsRemoteOnlyItem = this->GetMetaverseManager()->IsItemRemoteOnly(displayItemId);
						}
					}
				 }
				 else if (!pShortcut && m_pConnectedUniverse && m_pConnectedUniverse->connected && m_pMetaverseManager->GetNumInstanceUsers() > 0)
				 {
					 C_DynamicProp* pDynamicProp = dynamic_cast<C_DynamicProp*>(pEntity);
					 if (pDynamicProp)
					 {
						 user_t* pUser = m_pMetaverseManager->FindInstanceUser(pDynamicProp);
						 if (pUser)
						 {
							 hoverTitle = pUser->displayName;

							 if (pUser->state == "1")
							 {
								 std::string gameTitle = "";
								 if (pUser->launched != "")
								 {
									 KeyValues* pItemKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->GetLibraryItem(pUser->launched));
									 if (pItemKV)
										 gameTitle = pItemKV->GetString("title");
								 }

								 if (gameTitle != "")
								 {
									 if (pUser->twitchChannel != "" && pUser->twitchLive == "1")
										 hoverTitle += std::string("\nSTREAMING: ") + gameTitle;
									 else
										 hoverTitle += std::string("\nPLAYING: ") + gameTitle;
								 }
								 else
									 hoverTitle += std::string(" (PAUSED)");
							 }
							 else
							 {
								 std::string gameTitle = "";
								 if (pUser->objectId != "")
								 {
									 object_t* pObject = m_pInstanceManager->GetInstanceObject(pUser->objectId);
									 if (pObject)
									 {
										 KeyValues* pItemKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->GetLibraryItem(pObject->itemId));
										 if (pItemKV)
											 gameTitle = pItemKV->GetString("title");
									 }
								 }

								 if (gameTitle != "")
									 hoverTitle += std::string("\nWATCHING: ") + gameTitle;
							 }
						 }
					 }
				 }

				m_fHoverTitleExpiration = engine->Time() + fHoverTitleDuration;
				iHoverEntityIndex = iEntityIndex;

				if (this->IsVRActive() && this->IsHandTrackingActive())
					this->VRControllerVibrateStart(1, 20);
			}
			else
			{

				C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				if (pShortcut)
					bHoverEntityShortcut = true;

				if (this->IsVRActive() && this->IsHandTrackingActive())
					this->VRControllerVibrateStop(1);
			}
		}
		else if (m_iHoverEntityIndex != -1)
		{
			bChanged = true;

			if (this->IsVRActive() && this->IsHandTrackingActive())
			{
				this->VRControllerVibrateStop(1);
				//if (!m_bWasVRTeleport)
				//	engine->ClientCmd(VarArgs("set_render_mode %i 1\n", pVRPointerRight->entindex()));
			}
		}
	}

	if (bChanged)
	{
		if (this->IsVRActive() && this->IsHandTrackingActive() && pVRPointerRight)
		{
			if (!bHoverEntityShortcut && !m_bWasVRTeleport && m_bHoverEntityShortcut)
				engine->ClientCmd(VarArgs("set_render_mode %i 1\n", pVRPointerRight->entindex()));
			else if (bHoverEntityShortcut && !m_bWasVRTeleport && !m_bHoverEntityShortcut)
				engine->ClientCmd(VarArgs("set_render_mode %i 0\n", pVRPointerRight->entindex()));
		}

		if (bHoverEntityShortcut && hoverTitle != "" && !bIsRemoteOnlyItem && !bIsRemoteOnlyModel && g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && !g_pAnarchyManager->GetConnectedUniverse()->isHost)
			hoverTitle += " (From Library)";

		m_hoverTitle = hoverTitle;
		m_iHoverEntityIndex = iHoverEntityIndex;
		m_bHoverEntityShortcut = bHoverEntityShortcut;
		this->UpdateHoverLabel();
	}
}

void C_AnarchyManager::UpdateHoverLabel()
{
	if (!m_pHoverLabel)
		return;

	const size_t cSize = m_hoverTitle.length() + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, m_hoverTitle.c_str(), cSize);

	//m_pHoverLabel->SetCenterWrap(false);
	//m_pHoverLabel->SetSize(1, 1);

	int contentWidth = -1;
	int contentHeight = -1;

	m_pHoverLabel->SetText(wc);

	if (contentWidth == -1 || contentHeight == -1)
		m_pHoverLabel->GetContentSize(contentWidth, contentHeight);

	m_pHoverLabel->SetSize(contentWidth + 20, contentHeight + 20);
	//m_pHoverLabel->SetCenterWrap(true);
	m_pHoverLabel->SetPos((ScreenWidth() / 2) - ((contentWidth + 20) / 2.0), ((ScreenHeight() / 2) - ((contentHeight + 20) / 2.0)) + (ScreenHeight() / 6));

	delete[] wc;
}

void C_AnarchyManager::EnableSBSRendering()
{
	if (!m_pUseSBSRenderingConVar)
		m_pUseSBSRenderingConVar = cvar->FindVar("usesbs");

	// can't enable twice.
	if (m_pUseSBSRenderingConVar->GetBool())
		return;

	m_pUseSBSRenderingConVar->SetValue(true);
}

void C_AnarchyManager::DisableSBSRendering()
{
	if (!m_pUseSBSRenderingConVar)
		m_pUseSBSRenderingConVar = cvar->FindVar("usesbs");

	m_pUseSBSRenderingConVar->SetValue(false);
}

bool C_AnarchyManager::UseSBSRendering()
{
	if (!m_pUseSBSRenderingConVar)
		m_pUseSBSRenderingConVar = cvar->FindVar("usesbs");

	return m_pUseSBSRenderingConVar->GetBool();
}

void C_AnarchyManager::OnSelectorTraceResponse(int iEntity, float flX, float flY, float flZ, float flNormalX, float flNormalY, float flNormalZ)
{
	m_iSelectorTraceEntityIndex = iEntity;
	m_selectorTraceVector.x = flX;
	m_selectorTraceVector.y = flY;
	m_selectorTraceVector.z = flZ;
	m_selectorTraceNormal.x = flNormalX;
	m_selectorTraceNormal.y = flNormalY;
	m_selectorTraceNormal.z = flNormalZ;

	//GetVRTeleport
	if (pVRTeleport && m_bWasVRTeleport && pVRHandRight)
	{
		// figure out the angles
		float flYaw = RAD2DEG(atan2(-m_flPreviousJoystickSide, -m_flPreviousJoystickForward));

		VMatrix entToWorld;
		Vector xaxis;
		Vector yaxis;

		Vector normal = Vector(0.0f, 0.0f, 1.0f);// = g_pAnarchyManager->GetSelectorTraceNormal();
		Vector endpos = g_pAnarchyManager->GetSelectorTraceVector();
		Vector ItemToPlayer;
		VectorSubtract(pVRHandRight->GetAbsOrigin(), Vector(endpos.x, endpos.y, endpos.z), ItemToPlayer);

		xaxis = Vector(ItemToPlayer.x, ItemToPlayer.y, ItemToPlayer.z);

		CrossProduct(normal, xaxis, yaxis);
		if (VectorNormalize(yaxis) < 1e-3)
		{
			xaxis.Init(0.0f, 0.0f, 1.0f);
			CrossProduct(normal, xaxis, yaxis);
			VectorNormalize(yaxis);
		}
		CrossProduct(yaxis, normal, xaxis);
		VectorNormalize(xaxis);

		entToWorld.SetBasisVectors(xaxis, yaxis, normal);

		QAngle angles;
		MatrixToAngles(entToWorld, angles);

		// sync the pos & angles
		engine->ClientCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", pVRTeleport->entindex(), flX, flY, flZ, angles.x, angles.y - flYaw, angles.z));	// servercmdfix, false);
	}
}

void C_AnarchyManager::InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig)
{
	m_modelPreviewRenderTarget.Init(this->CreateModelPreviewRenderTarget(pMaterialSystem));

#ifdef VR_ALLOWED
	DevMsg("Init AArcade VR render targets...\n");

	if (this->OpenVRHMD())
	{
		//m_VRTwoEyesHMDRenderTarget.Init(this->CreateVRTwoEyesHMDRenderTarget(pMaterialSystem));


		if (hmdIsConnected())
		{
			//m_ScopeTexture.Init(CreateScopeTexture(pMaterialSystem));
			//m_MenuTexture.Init(CreateMenuTexture(pMaterialSystem));
			//m_VGuiTexture.Init(CreateVGuiTexture(pMaterialSystem));

			//m_VRTwoEyesHMDRenderTargets[0] = new CTextureReference();
			//m_VRTwoEyesHMDRenderTargets[1] = new CTextureReference();
			//m_VRTwoEyesHMDRenderTargets[2] = new CTextureReference();

			hmdSetNextTextureType(hmdTextureTypes::tBothEyes, hmdTextureTypes::iIndex0);
			m_VRTwoEyesHMDRenderTargets[0].Init(CreateVRTwoEyesHMDRenderTarget(pMaterialSystem, 0));
			hmdSetNextTextureType(hmdTextureTypes::tBothEyes, hmdTextureTypes::iIndex1);
			m_VRTwoEyesHMDRenderTargets[1].Init(CreateVRTwoEyesHMDRenderTarget(pMaterialSystem, 1));
			hmdSetNextTextureType(hmdTextureTypes::tBothEyes, hmdTextureTypes::iIndex2);
			m_VRTwoEyesHMDRenderTargets[2].Init(CreateVRTwoEyesHMDRenderTarget(pMaterialSystem, 2));

			m_VRSpectatorRenderTarget.Init(CreateVRSpectatorRenderTarget(pMaterialSystem));

			//m_VROneEyeTextureQuarterSize.Init(CreateVROneEyeTextureQuarterSize(pMaterialSystem));
		}

		// Water effects & camera from the base class (standard HL2 targets) 
		//BaseClass::InitClientRenderTargets(pMaterialSystem, pHardwareConfig);
	}
#endif
}


void C_AnarchyManager::ShutdownClientRenderTargets()
{
	m_modelPreviewRenderTarget.Shutdown();
#ifdef VR_ALLOWED
	if (hmdIsConnected())
	{
		hmdClose();

		//m_VRTwoEyesHMDRenderTarget.Shutdown();
		m_VRTwoEyesHMDRenderTargets[0].Shutdown();
		m_VRTwoEyesHMDRenderTargets[1].Shutdown();
		m_VRTwoEyesHMDRenderTargets[2].Shutdown();
		m_VRSpectatorRenderTarget.Shutdown();
	}
#endif
}

ITexture* C_AnarchyManager::GetRenderTarget(ISourceVirtualReality::VREye eEye, ISourceVirtualReality::EWhichRenderTarget eWhich)
{
	///*&& eEye > 0 && eEye < 3
	// && eEye == 0 && eEye == 1 &&
	/*
	if (m_bVRActive && (eWhich != ISourceVirtualReality::EWhichRenderTarget::RT_Depth || eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Color))//((eEye > 0 && eEye < 3) || eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Depth)
		return m_pVRTwoEyesHMDRenderTexture;// materials->FindTexture("_rt_two_eyes_VR", TEXTURE_GROUP_RENDER_TARGET);
	else
		return null;
		*/
	//*/

	/*
	if (m_bVRActive && eEye > 0 && eEye < 3)
	{
		if (eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Color || eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Depth)
		{
			return materials->FindTexture("_rt_two_eyes_VR", TEXTURE_GROUP_RENDER_TARGET);
		}
	}
	return NULL;
	*/


#ifdef VR_ALLOWED
	if (m_bVRActive)
	{
		/*if (eEye > 0 && eEye < 3)
		{
			if (eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Color || (eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Depth))
			{
				return materials->FindTexture("_rt_two_eyes_VR", TEXTURE_GROUP_RENDER_TARGET);
			}
		}*/
		const int currentRenderBufferIndex = hmdGetCurrentTextureIndex();
		const char* currentRenderBufferTextureName = this->BackBufferNamePerIndex(currentRenderBufferIndex);
		return materials->FindTexture(currentRenderBufferTextureName, TEXTURE_GROUP_RENDER_TARGET);
	}
#endif
	return NULL;
}

ITexture* C_AnarchyManager::GetModelPreviewRenderTarget()
{
	return m_pModelPreviewRenderTexture;
}

std::string C_AnarchyManager::ExtractRelativeAssetPath(std::string fullPath)
{
	std::string path = fullPath;
	bool bUseBackslash = (path.find("\\") != std::string::npos);
	size_t foundFirstSlash = (bUseBackslash) ? path.find_first_of("\\") : path.find_first_of("/");
	std::string token;
	while (foundFirstSlash != std::string::npos)
	{
		token = path.substr(0, foundFirstSlash);
		std::transform(token.begin(), token.end(), token.begin(), ::tolower);
		if (token == "models" && g_pFullFileSystem->FileExists(path.c_str(), "GAME"))
			break;
		else
			path = path.substr(foundFirstSlash + 1);

		foundFirstSlash = (bUseBackslash) ? path.find_first_of("\\") : path.find_first_of("/");
	}

	if (foundFirstSlash != std::string::npos)
		return path;
	else
		return fullPath;
}

/*
bool C_AnarchyManager::OverrideView(CViewSetup *pViewMiddle, Vector *pViewModelOrigin, QAngle *pViewModelAngles, HeadtrackMovementMode_t hmmMovementOverride)
{
	// Everything in here is in Source coordinate space.
	if (!this->UseSBSRendering() && !UseVR())
	{
		return false;
	}

	if (hmmMovementOverride == HMM_NOOVERRIDE)
	{
		if (CurrentlyZoomed())
		{
			m_hmmMovementActual = static_cast<HeadtrackMovementMode_t>(vr_moveaim_mode_zoom.GetInt());
		}
		else
		{
			m_hmmMovementActual = static_cast<HeadtrackMovementMode_t>(vr_moveaim_mode.GetInt());
		}
	}
	else
	{
		m_hmmMovementActual = hmmMovementOverride;
	}


	// Incoming data may or may not be useful - it is the origin and aim of the "player", i.e. where bullets come from.
	// In some modes it is an independent thing, guided by the mouse & keyboard = useful.
	// In other modes it's just where the HMD was pointed last frame, modified slightly by kbd+mouse.
	// In those cases, we should use our internal reference (which keeps track thanks to OverridePlayerMotion)
	QAngle originalMiddleAngles = pViewMiddle->angles;
	Vector originalMiddleOrigin = pViewMiddle->origin;

	// Figure out the in-game "torso" concept, which corresponds to the player's physical torso.
	m_PlayerTorsoOrigin = pViewMiddle->origin;

	// Ignore what was passed in - it's just the direction the weapon is pointing, which was determined by last frame's HMD orientation!
	// Instead use our cached value.
	QAngle torsoAngles = m_PlayerTorsoAngle;

	VMatrix worldFromTorso;
	worldFromTorso.SetupMatrixOrgAngles(m_PlayerTorsoOrigin, torsoAngles);

	//// Scale translation e.g. to allow big in-game leans with only a small head movement.
	//// Clamp HMD movement to a reasonable amount to avoid wallhacks, vis problems, etc.
	float limit = vr_translation_limit.GetFloat();
	VMatrix matMideyeZeroFromMideyeCurrent = g_pSourceVR->GetMideyePose();
	Vector viewTranslation = matMideyeZeroFromMideyeCurrent.GetTranslation();
	if (viewTranslation.IsLengthGreaterThan(limit))
	{
		viewTranslation.NormalizeInPlace();
		viewTranslation *= limit;
		matMideyeZeroFromMideyeCurrent.SetTranslation(viewTranslation);
	}

	// Now figure out the three principal matrices: m_TorsoFromMideye, m_WorldFromMidEye, m_WorldFromWeapon
	// m_TorsoFromMideye is done so that OverridePlayerMotion knows what to do with WASD.

	switch (m_hmmMovementActual)
	{
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTFACE_MOVETORSO:
		// Aim point is down your nose, i.e. same as the view angles.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = m_WorldFromMidEye;
		break;

	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
		// Aim point is independent of view - leave it as it was, just copy it into m_WorldFromWeapon for our use.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon.SetupMatrixOrgAngles(originalMiddleOrigin, originalMiddleAngles);
		break;

	case HMM_SHOOTMOVELOOKMOUSE:
		// HMD is ignored completely, mouse does everything.
		m_PlayerTorsoAngle = originalMiddleAngles;

		worldFromTorso.SetupMatrixOrgAngles(m_PlayerTorsoOrigin, originalMiddleAngles);

		m_TorsoFromMideye.Identity();
		m_WorldFromMidEye = worldFromTorso;
		m_WorldFromWeapon = worldFromTorso;
		break;

	case HMM_SHOOTMOVELOOKMOUSEFACE:
		// mouse does everything, and then we add head tracking on top of that
		worldFromTorso = worldFromTorso * matMideyeZeroFromMideyeCurrent;

		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = worldFromTorso;
		m_WorldFromMidEye = worldFromTorso;
		break;

	default: Assert(false); break;
	}

	// Finally convert back to origin+angles that the game understands.
	pViewMiddle->origin = m_WorldFromMidEye.GetTranslation();
	VectorAngles(m_WorldFromMidEye.GetForward(), m_WorldFromMidEye.GetUp(), pViewMiddle->angles);

	*pViewModelAngles = pViewMiddle->angles;
	if (vr_viewmodel_translate_with_head.GetBool())
	{
		*pViewModelOrigin = pViewMiddle->origin;
	}
	else
	{
		*pViewModelOrigin = originalMiddleOrigin;
	}

	m_WorldFromMidEyeNoDebugCam = m_WorldFromMidEye;
	if (vr_debug_remote_cam.GetBool())
	{
		Vector vOffset(vr_debug_remote_cam_pos_x.GetFloat(), vr_debug_remote_cam_pos_y.GetFloat(), vr_debug_remote_cam_pos_z.GetFloat());
		Vector vLookat(vr_debug_remote_cam_target_x.GetFloat(), vr_debug_remote_cam_target_y.GetFloat(), vr_debug_remote_cam_target_z.GetFloat());
		pViewMiddle->origin += vOffset;
		Vector vView = vLookat - vOffset;
		VectorAngles(vView, m_WorldFromMidEye.GetUp(), pViewMiddle->angles);

		m_WorldFromMidEye.SetupMatrixOrgAngles(pViewMiddle->origin, pViewMiddle->angles);

		m_TorsoFromMideye.Identity();
	}

	// set the near clip plane so the local player clips less
	pViewMiddle->zNear *= vr_projection_znear_multiplier.GetFloat();

	return true;
}
*/

void C_AnarchyManager::GetViewportBounds(ISourceVirtualReality::VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight)
{
	//POINT vrres;// = hmdGetResolution();
	//vrres.x = this->GetVRBufferWidth();// 1920;// 2160;
	//vrres.y = this->GetVRBufferHeight();// 1080;// 1200;

	//unsigned int uScreenWidth = vrres.x;
	//unsigned int uScreenHeight = vrres.y;

	//float flVar = (eEye == 0) ? 1 : 0;

	//ISourceVirtualReality::VREye eGoodEye = (eEye == ISourceVirtualReality::VREye::VREye_Left) ? ISourceVirtualReality::VREye::VREye_Right : ISourceVirtualReality::VREye::VREye_Left;
	if (this->IsVRActive())
	{
		if (pnX != NULL)
		{
			*pnX = this->GetVRBufferWidth() * (eEye)* 0.5f;
			*pnY = 0;
		}

		*pnWidth = this->GetVRBufferWidth() * 0.5f;
		*pnHeight = this->GetVRBufferHeight();
	}
	else
	{
		if (g_pAnarchyManager->GetNoDrawShortcutsValue() != 2)
		{
			*pnHeight = ScreenHeight();
			*pnWidth = ScreenWidth() * 0.5f;
			*pnX = (eEye == ISourceVirtualReality::VREye::VREye_Left) ? 0 : *pnWidth;
			*pnY = 0;
		}
		else
		{
			*pnHeight = ScreenHeight();
			*pnWidth = ScreenWidth();// *0.5f;
			//*pnX = (eEye == ISourceVirtualReality::VREye::VREye_Left) ? -(*pnWidth) * 0.5 : (*pnWidth) * 0.5;
			//*pnX = 0;// (*pnWidth) * -0.5;
			//*pnX = (eEye == ISourceVirtualReality::VREye::VREye_Right) ? 0 : (*pnWidth) * 0.5;
			*pnX = 0;
			*pnY = 0;
		}
	}
	return;
}

//#include "view.h"
void C_AnarchyManager::ScreenToWorld(int mousex, int mousey, float fov,
	const Vector& vecRenderOrigin,
	const QAngle& vecRenderAngles,
	Vector& vecPickingRay)
{
	float dx, dy;
	float c_x, c_y;
	float dist;
	Vector vpn, vup, vright;

	float scaled_fov = ScaleFOVByWidthRatio(fov, engine->GetScreenAspectRatio() * 0.75f);

	c_x = ScreenWidth() / 2;
	c_y = ScreenHeight() / 2;

	dx = (float)mousex - c_x;
	// Invert Y
	dy = c_y - (float)mousey;

	// Convert view plane distance
	dist = c_x / tan(M_PI * scaled_fov / 360.0);

	// Decompose view angles
	AngleVectors(vecRenderAngles, &vpn, &vright, &vup);

	// Offset forward by view plane distance, and then by pixel offsets
	vecPickingRay = vpn * dist + vright * (dx)+vup * (dy);

	// Convert to unit vector
	VectorNormalize(vecPickingRay);
}

object_t* C_AnarchyManager::GetObjectUnderCursor(int iMouseX, int iMouseY)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	Vector vEyePosition, vForward, vUp, vRight;
	pPlayer->EyePositionAndVectors(&vEyePosition, &vForward, &vRight, &vUp);

	Vector vecPickingRay;
	ScreenToWorld(iMouseX, iMouseY, pPlayer->GetFOV(), vEyePosition, pPlayer->EyeAngles(), vecPickingRay);

	Vector vStartingPos = vEyePosition + vecPickingRay;
	Vector vEndingPos = vEyePosition + vecPickingRay * MAX_COORD_RANGE;

	Ray_t ray;
	ray.Init(vStartingPos, vEndingPos);

	trace_t tr;
	UTIL_TraceRay(ray, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	C_BaseEntity* pEntity = null;
	if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
		pEntity = tr.m_pEnt;

	// only allow prop shortcuts
	C_PropShortcutEntity* pShortcut = null;
	if (pEntity)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);

	if (pShortcut)
	{
		std::string itemId = pShortcut->GetItemId();
		if (itemId != "")
		{
			object_t* pObject = m_pInstanceManager->GetInstanceObject(pShortcut->GetObjectId());
			if (pObject)
			{
				return pObject;
			}
		}
	}

	return null;
}

bool C_AnarchyManager::GetTaskInfo(C_EmbeddedInstance* pEmbeddedInstance, taskinfo_t &taskinfo)
{
	/*
	std::string id;
	std::string title;
	std::string itemTitle;
	bool isHiddenTask;
	bool isPresetHiddenTask;
	bool isDisplayTask;
	bool isWindowsTask;
	std::string embeddedType;
	int entityIndex;
	std::string itemId;
	std::string objectId;
	std::string modelId;
	*/

	taskinfo.id = pEmbeddedInstance->GetId();
	taskinfo.title = pEmbeddedInstance->GetTitle();
	taskinfo.isHiddenTask = (pEmbeddedInstance->GetId() == "hud" || pEmbeddedInstance->GetId() == "images" || pEmbeddedInstance->GetId() == "network");
	taskinfo.isPresetHiddenTask = taskinfo.isHiddenTask;
	taskinfo.isDisplayTask = (pEmbeddedInstance == g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() || (!g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() && pEmbeddedInstance == g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay()));
	//bool bIsFirstTaskToDisplay = (!g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() && pEmbeddedInstance == g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay());
	taskinfo.isWindowsTask = false;

	taskinfo.embeddedType = "Unknown";
	C_LibretroInstance* pLibretroInstance = dynamic_cast<C_LibretroInstance*>(pEmbeddedInstance);
	if (pLibretroInstance)
		taskinfo.embeddedType = "Libretro";
	else
	{
		C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(pEmbeddedInstance);
		if (pSteamBrowserInstance)
			taskinfo.embeddedType = "SteamworksBrowser";
		else
		{
			C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pEmbeddedInstance);
			if (pAwesomiumBrowserInstance)
				taskinfo.embeddedType = "AwesomiumBrowser";
		}
	}

	taskinfo.itemTitle = taskinfo.title;
	taskinfo.entityIndex = pEmbeddedInstance->GetOriginalEntIndex();
	C_PropShortcutEntity* pEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(taskinfo.entityIndex));
	if (pEntity)
	{
		taskinfo.itemId = pEntity->GetItemId();
		taskinfo.objectId = pEntity->GetObjectId();
		taskinfo.modelId = pEntity->GetModelId();

		KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(taskinfo.itemId));
		if (pItemKV)
			taskinfo.itemTitle = pItemKV->GetString("title");
	}
	else
	{
		taskinfo.itemId = "";
		taskinfo.objectId = "";
		taskinfo.modelId = "";
	}

	return true;
}

void C_AnarchyManager::OnSpawnObjectsButtonDown()
{
	m_flSpawnObjectsButtonDownTime = engine->Time();
}

void C_AnarchyManager::OnSpawnObjectsButtonUp()
{
	if (engine->Time() - m_flSpawnObjectsButtonDownTime < 1.0f)
		this->GetInstanceManager()->SpawnActionPressed();

	m_flSpawnObjectsButtonDownTime = 0.0f;
}

void C_AnarchyManager::BringToTop()
{
	bool bIsMapLoaded = (g_pAnarchyManager->MapName());
	if (bIsMapLoaded && this->ShouldShowWindowsTaskBar())
		BringWindowToTop(m_hwnd);
}

void C_AnarchyManager::JoinLobbyWeb(std::string lobbyId)
{
	//std::string firstLetter = VarArgs("%c", lobbyId.at(0));
	//if (!this->AlphabetSafe(firstLetter, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") || !this->AlphabetSafe(lobbyId, "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"))
	//	return;

	// just pop-out for now, and do security checks at the last possible moment (for the least chance of haxing)
	this->Popout("lobby", lobbyId);
}

//void C_AnarchyManager::DoConnect(std::string lobbyId)
//{
//	this->Disconnect();
//	engine->ClientCmd(VarArgs("show_connect %s", lobbyId.c_str()));
//}

void C_AnarchyManager::JoinLobby(std::string lobbyId)
{
	this->Disconnect();
	engine->ClientCmd(VarArgs("show_connect %s", lobbyId.c_str()));

	/*
	aampConnection_t* pConnection = this->GetConnectedUniverse();
	if (!pConnection || g_pAnarchyManager->GetConnectedUniverse()->connected)// && !m_pMetaverseManager->GetHasDisconnected())
	{
		DevMsg("Invalid network state. Aborting.\n");
		return;
	}

	DevMsg("Now do real work!!\n");
	*/
	/*
		1. Remember the lobby ID we are trying to connect to.
		2. Unload the current map & activate deferred saving.
		3. Load the Connect To Server page.
		4. Upon successfully prepping, load the map.
		5. Consider the universe connected to while the web tab connects & starts to automatically load entries from the firebase.
		6. Smoke a mid-way bowl.
	*/
}

void C_AnarchyManager::PopToast()
{
	std::vector<KeyValues*> deadMessages;

	float fCurrentTime = engine->Time();
	float fTesterTime = 0;
	float fNextBestExpirationTime = 0;

	// A toast message is ready to pop.  Find it, pop it, update toat text.
	for (KeyValues *pMessageKV = m_pToastMessagesKV->GetFirstSubKey(); pMessageKV; pMessageKV = pMessageKV->GetNextKey())
	{
		fTesterTime = pMessageKV->GetFloat("end");
		if (fTesterTime <= fCurrentTime || !m_pHoverTitlesConVar->GetBool() )
			deadMessages.push_back(pMessageKV);
		else if (fNextBestExpirationTime == 0 || fTesterTime < fNextBestExpirationTime)
			fNextBestExpirationTime = fTesterTime;
	}

	unsigned int max = deadMessages.size();
	for (unsigned int i = 0; i < max; i++)
		m_pToastMessagesKV->RemoveSubKey(deadMessages[i]);

	m_fNextToastExpiration = fNextBestExpirationTime;

	this->UpdateToastText();
}

void C_AnarchyManager::AddToastMessage(std::string text, bool bForce)
{
	if (!m_pToastMsgsConVar || (!m_pToastMsgsConVar->GetBool() && !bForce) || g_pAnarchyManager->IsPaused())
		return;

	float fToastDuration = 7.0;

	float fEndTime = engine->Time() + fToastDuration;
	KeyValues* pMessageKV = m_pToastMessagesKV->CreateNewKey();
	pMessageKV->SetName("message");
	pMessageKV->SetString("text", text.c_str());
	pMessageKV->SetFloat("end", fEndTime);

	if (this->GetNextToastExpiration() == 0)
		this->SetNextToastExpiration(fEndTime);

	// update toast text
	this->UpdateToastText();
}

void C_AnarchyManager::UpdateToastText()
{
	std::string text;

	// Any msg that exists gets added. (Expired msgs get removed prior to this method being called.)
	for (KeyValues *pMessageKV = m_pToastMessagesKV->GetFirstSubKey(); pMessageKV; pMessageKV = pMessageKV->GetNextKey())
	{
		//if (text != "")
			//text = "\n" + text;
		//text = pMessageKV->GetString("text") + text;

		if (text != "")
			text += "\n";
		text += pMessageKV->GetString("text");
	}

	m_toastText = text;
	this->SetToastText(m_toastText);
}

void C_AnarchyManager::AddToastLabel(vgui::Label* pLabel)
{
	m_toastLabels.push_back(pLabel);
}

void C_AnarchyManager::RemoveToastLabel(vgui::Label* pLabel)
{
	unsigned int max = m_toastLabels.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_toastLabels[i] == pLabel)
		{
			m_toastLabels.erase(m_toastLabels.begin() + i);
			return;
		}
	}
}

void C_AnarchyManager::SetToastText(std::string text)
{
	const size_t cSize = text.length() + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, text.c_str(), cSize);

	int contentWidth = -1;
	int contentHeight = -1;

	unsigned int max = m_toastLabels.size();
	for (unsigned int i = 0; i < max; i++)
	{
		m_toastLabels[i]->SetText(wc);

		if (contentWidth == -1 || contentHeight == -1)
			m_toastLabels[i]->GetContentSize(contentWidth, contentHeight);

		m_toastLabels[i]->SetSize(contentWidth + 20, contentHeight + 20);
	}

	delete[] wc;
}

void C_AnarchyManager::DoTakeBigScreenshot()
{
	//std::string cmd = VarArgs("jpeg \"%s\" \"%s\";\n", m_pNextBigScreenshotKV->GetString("file"), m_pNextBigScreenshotKV->GetString("quality"));
	//DevMsg("CMD: %s\n", cmd.c_str());
	engine->ClientCmd(VarArgs("jpeg \"%s\" \"%s\"\n", m_pNextBigScreenshotKV->GetString("file"), m_pNextBigScreenshotKV->GetString("quality")));

	bool bIsInteractive = m_pNextBigScreenshotKV->GetBool("IsInteractive");
	bool bIsPanoShot = m_pNextBigScreenshotKV->GetBool("isPanoShot");
	m_pNextBigScreenshotKV->deleteThis();
	m_pNextBigScreenshotKV = null;

	if (bIsPanoShot)
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		std::vector<std::string> params;
		//params.push_back(m_pNextBigScreenshotKV->GetString("file"));
		pHudBrowserInstance->DispatchJavaScriptMethod("shotListener", "shotTaken", params);
	}
	/*
	else if (bIsInteractive)
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		std::vector<std::string> params;
		//params.push_back(m_pNextBigScreenshotKV->GetString("file"));
		pHudBrowserInstance->DispatchJavaScriptMethod("shotListener", "initialShotTaken", params);	// This might not ever be used. Possibly obsolete.
	}
	*/
}

void C_AnarchyManager::OpenScreenshot(std::string screenshotId)
{
	KeyValues* pScreenshotKV = g_pAnarchyManager->GetMetaverseManager()->GetScreenshot(screenshotId);
	if (pScreenshotKV)
	{
		std::string fileName = this->GetAArcadeUserFolder();
		fileName += "\\screenshots\\";
		fileName += screenshotId;
		fileName += ".html";

		this->Acquire(fileName, false, true, true);
	}
}

void C_AnarchyManager::InteractiveScreenshotReady(std::string screenshotId, std::string codeText)
{
	FileHandle_t fh = g_pFullFileSystem->Open(VarArgs("screenshots\\%s.js", screenshotId.c_str()), "w", "DEFAULT_WRITE_PATH");	//a+

	if (fh)
	{
		if (codeText != "")
			g_pFullFileSystem->Write(codeText.c_str(), codeText.length(), fh);
		else
			g_pFullFileSystem->Write("// empty", 8, fh);

		g_pFullFileSystem->Close(fh);
		this->AddToastMessage("Interactive Screenshot Saved");
	}
	else
		this->AddToastMessage("Interactive Screenshot Failed");
}

void C_AnarchyManager::CheckPicMip()
{
	int multiplyer = this->GetDynamicMultiplyer();
	if (m_iLastDynamicMultiplyer != multiplyer)
	{
		if (m_iLastDynamicMultiplyer != -1)
		{
			// this is NOT the first time the mip quality has been detected.
			DevMsg("Mip has changed!\n");	// DOES NOT WORK
		}

		m_iLastDynamicMultiplyer = multiplyer;
	}
}

int C_AnarchyManager::GetDynamicMultiplyer()
{
	return 1;
	/*if (!m_pPicMipConVar)
		m_pPicMipConVar = cvar->FindVar("mat_picmip");

	int multiplyer = 1;
	int mip = m_pPicMipConVar->GetInt();
	if (mip == 2)
		multiplyer = 4;
	else if (mip == 1)
		multiplyer = 2;

	return multiplyer;*/
}

//void C_AnarchyManager::CalculateDynamicMultiplyer()
//{
	/*
	if (!m_pPicMipConVar)
		m_pPicMipConVar = cvar->FindVar("mat_picmip");

	int multiplyer = 1;
	int mip = m_pPicMipConVar->GetInt();
	if (mip == 2)
		multiplyer = 4;
	else if (mip == 1)
		multiplyer = 2;

	m_iDynamicMultiplyer = multiplyer;
	*/
//}

void C_AnarchyManager::ShowWindowsTaskBar()
{
	if (!this->ShouldShowWindowsTaskBar())
		return;

	HWND hwndDesktop = GetDesktopWindow();
	HWND hwndTray = FindWindowEx(hwndDesktop,
		NULL,  // start looking from first child
		"Shell_TrayWnd",
		NULL); // don't care about window title

	if (hwndTray)
	{
		SetWindowPos(hwndTray, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		SetWindowPos(hwndTray, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
		//BringWindowToTop(hwndTray);
		//SetForegroundWindow(hwndTray);
	//SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void C_AnarchyManager::SpecialReady(C_AwesomiumBrowserInstance* pInstance)
{
	std::string id = pInstance->GetId();
	/*
	auto it = m_specialInstances.find(id);
	if (it != m_specialInstances.end())
		it->second = true;

	// check if ALL specials are ready
	bool bAllReady = true;

	it = m_specialInstances.begin();
	while (it != m_specialInstances.end())
	{
		if (!it->second)
		{
			bAllReady = false;
			break;
		}

		it++;
	}
	*/
	if (id == "hud" && AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT)	// is this too early??
		g_pAnarchyManager->IncrementState();
	else if (id == "network" && AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT)
	{
		m_pAwesomiumBrowserManager->SetNetworkAwesomiumBrowserInstance(pInstance);
		g_pAnarchyManager->IncrementState();
	}
	else if (id == "images" && AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT)
		g_pAnarchyManager->IncrementState();
}

// If mode is NOT empty (usually "autoinspect"), then Libretro will NOT auto-play.
bool C_AnarchyManager::QuickRemember(int iEntityIndex, std::string mode)
{
	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(iEntityIndex);
	if (!pBaseEntity)
		return false;

	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
	if (!pShortcut)
		return false;

	// only if shortcut is an item
	if (pShortcut && pShortcut->GetItemId() != pShortcut->GetModelId() && pShortcut->GetItemId() != "" && pShortcut->GetModelId() != "")
	{
		//g_pAnarchyManager->AttemptSelectEntity(pShortcut);

		// FIXME: This is redundant code.  It's also in the selectEntity method! (and possibly other places too)
		// TODO: Generalize this into a method of g_pAnarchymanager!!

		std::string taskId = "auto" + pShortcut->GetItemId(); //tabTitle = "auto" + pShortcut->GetItemId();
		C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(taskId);// this->GetWebManager()->FindWebTab(tabTitle);
		if (!pEmbeddedInstance)
		{
			std::string itemId = pShortcut->GetItemId();
			KeyValues* item = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId);
			if (item)
			{
				KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(item);

				bool bDoAutoInspect = true;

				std::string gameFile = "";
				std::string coreFile = "";
				bool bShouldLibretroLaunch = (g_pAnarchyManager->DetermineLibretroCompatible(item, gameFile, coreFile) && g_pAnarchyManager->GetLibretroManager()->GetInstanceCount() == 0);

				// auto-libretro
				if (mode == "" && cvar->FindVar("auto_libretro")->GetBool() && bShouldLibretroLaunch && g_pFullFileSystem->FileExists(gameFile.c_str()))
				{
					C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->CreateLibretroInstance();
					pLibretroInstance->Init(taskId, VarArgs("%s - Libretro", active->GetString("title", "Untitled")), pShortcut->entindex());
					DevMsg("Setting game to: %s\n", gameFile.c_str());
					pLibretroInstance->SetOriginalGame(gameFile);
					pLibretroInstance->SetOriginalItemId(itemId);
					pLibretroInstance->SetOriginalEntIndex(pShortcut->entindex());	// probably NOT needed?? (or maybe so, from here.)
					if (!pLibretroInstance->LoadCore(coreFile))	// FIXME: elegantly revert back to autoInspect if loading the core failed!
						DevMsg("ERROR: Failed to load core: %s\n", coreFile.c_str());
					else
						m_pAccountant->Action("aa_libretro_played", 1);
					pEmbeddedInstance = pLibretroInstance;
					bDoAutoInspect = false;
				}

				if (bDoAutoInspect)
				{
					std::string fileBuf = active->GetString("file");
					if (fileBuf.find("travel.html?") == 0)
					{
						std::string uri = "asset://ui/travel.html?item=" + itemId + "&entity=";
						uri += VarArgs("%i", pShortcut->entindex());
						uri += VarArgs("&object=%s&intro=1", pShortcut->GetObjectId().c_str());

						C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
						pHudBrowserInstance->SetUrl(uri);
						g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
						return false;

						/*
						std::string title = active->GetString("title", "Untitled");
						C_AwesomiumBrowserInstance* pBrowserInstance = m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance(taskId, uri, title, false, null, pShortcut->entindex());
						//pBrowserInstance->Init(taskId, uri, title, pShortcut->entindex());
						//pBrowserInstance->SetOriginalGame(gameFile);
						pBrowserInstance->SetOriginalItemId(itemId);
						pEmbeddedInstance = pBrowserInstance;

						bDoAutoInspect = false;
						*/
					}
					else
					{
						std::string previewBuf = active->GetString("preview");
						std::string matchedField;
						if (fileBuf.find("slideshow.html?") == 0)
							matchedField = "file";
						else if (previewBuf.find("slideshow.html?") == 0)
							matchedField = "preview";

						if (matchedField != "")
						{
							std::string uri = "asset://ui/slideshow.html?item=" + itemId + "&entity=";
							uri += VarArgs("%i", pShortcut->entindex());
							uri += "&field=" + matchedField;

							std::string title = active->GetString("title", "Untitled");
							C_AwesomiumBrowserInstance* pBrowserInstance = m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance(taskId, uri, title, false, null, pShortcut->entindex());
							//pBrowserInstance->Init(taskId, uri, title, pShortcut->entindex());
							//pBrowserInstance->SetOriginalGame(gameFile);
							pBrowserInstance->SetOriginalItemId(itemId);
							pEmbeddedInstance = pBrowserInstance;

							bDoAutoInspect = false;
						}
					}
				}

				if (bDoAutoInspect)
				{
					// If the currently selected input is ALSO a steamworks browser, we need to make sure it retains focus.
					C_EmbeddedInstance* pInputEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
					C_SteamBrowserInstance* pInputSteamInstance = dynamic_cast<C_SteamBrowserInstance*>(pInputEmbeddedInstance);

					pEmbeddedInstance = this->AutoInspect(item, "", pShortcut->entindex());

					if (pInputSteamInstance)
						g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(pInputSteamInstance);
					else
					{
						C_AwesomiumBrowserInstance* pInputAwesomiumInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pInputEmbeddedInstance);
						g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pInputAwesomiumInstance);
					}
				}

				if (pEmbeddedInstance)
				{
					g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(pEmbeddedInstance);
					return true;
				}
			}
		}
		else
		{
			g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(pEmbeddedInstance);
			return true;
		}
	}

	return false;
}

bool C_AnarchyManager::TempSelectEntity(int iEntityIndex)
{
	C_EmbeddedInstance* pLookingInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstanceByEntityIndex(iEntityIndex);
	if (!pLookingInstance)
	{
		if (!this->QuickRemember(iEntityIndex))
			return false;

		pLookingInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstanceByEntityIndex(iEntityIndex);

		if (!pLookingInstance)
			return false;
	}

	// 3. If it is, then temp-select THAT instance for input.
	//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	g_pAnarchyManager->SelectEntity(C_BaseEntity::Instance(iEntityIndex));
	g_pAnarchyManager->GetInputManager()->SetTempSelect(true);

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER))
		g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pLookingInstance);
	cvar->FindVar("glow_enabled")->SetValue(false);

	//g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pLookingInstance);
	g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pLookingInstance);
	g_pAnarchyManager->GetInputManager()->SetFullscreenMode(false);
	//pHudBrowserInstance->SetUrl("asset://ui/overlay.html");
	//pHudBrowserInstance->SetUrl("asset://ui/mouseEZ.html");
	return true;
}

bool C_AnarchyManager::HandleUiToggle(bool bIsFromToggleHandler)
{
	// ignore the start button on the gamepad if we are NOT the focused window
	HWND myHWnd = FindWindow(null, "AArcade: Source");
	HWND foregroundWindow = GetForegroundWindow();
	if (myHWnd != foregroundWindow)
		return true;

	if (m_bPaused)
	{
		DevMsg("Unpausing...\n");
		this->Unpause();

		if (!engine->IsInGame())
			engine->ClientCmd("main_menu\n");

		return true;
	}

	bool bStartButtonHeld = vgui::input()->IsKeyDown(KEY_XBUTTON_START);

	// handle escape if in pause mode (ignore it)
	if (!engine->IsInGame())
	{
		// GOOD MAIN MENU EMBEDDED APP ESCAPE BINDS AS OF 9/13/2016

		if (m_pSteamBrowserManager)
		{
			C_SteamBrowserInstance* pInstance = m_pSteamBrowserManager->GetSelectedSteamBrowserInstance();
			if (pInstance && pInstance->GetId().find("scrape") == 0)	// web tabs starting with scrape in their ID get auto-deleted when input mode closes.
				m_pInputManager->DeactivateInputMode(true);
			else
			{
				//if (m_pInputManager->GetMainMenuMode() && m_pInputManager->GetInputMode() && m_pInputManager->GetFullscreenMode() && pInstance && pInstance->GetTexture() && pInstance->GetTexture() == m_pInputManager->GetInputCanvasTexture())
				if (pInstance && g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
				{
					m_pSteamBrowserManager->DestroySteamBrowserInstance(pInstance);
					m_pInputManager->SetEmbeddedInstance(null);
					return true;
				}
			}
			return true;
		}

		if (m_pLibretroManager && !vgui::input()->IsKeyDown(KEY_XBUTTON_START))
		{
			C_LibretroInstance* pInstance = m_pLibretroManager->GetSelectedLibretroInstance();
			//if (m_pInputManager->GetMainMenuMode() && m_pInputManager->GetInputMode() && m_pInputManager->GetFullscreenMode() && pInstance && pInstance->GetTexture() && pInstance->GetTexture() == m_pInputManager->GetInputCanvasTexture())
			if (pInstance && g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
			{
				m_pLibretroManager->DestroyLibretroInstance(pInstance);
				m_pInputManager->SetEmbeddedInstance(null);
				return true;
			}
		}

		if (m_pAwesomiumBrowserManager)
		{
			C_AwesomiumBrowserInstance* pInstance = m_pAwesomiumBrowserManager->GetSelectedAwesomiumBrowserInstance();
			//if (m_pInputManager->GetMainMenuMode() && m_pInputManager->GetInputMode() && m_pInputManager->GetFullscreenMode() && pInstance && pInstance->GetTexture() && pInstance->GetTexture() == m_pInputManager->GetInputCanvasTexture())
			if (pInstance && g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance )
			{
				//if (pInstance->GetId() == "hud" && )// || pInstance->GetId() == "images" )
				if (engine->IsInGame() || (pInstance->GetId() != "hud" && pInstance->GetId() != "network"))
				{
					m_pAwesomiumBrowserManager->DestroyAwesomiumBrowserInstance(pInstance);
					m_pInputManager->SetEmbeddedInstance(null);
					return true;
				}
				else if (!engine->IsInGame() && pInstance->GetId() == "hud" || pInstance->GetId() == "network")
				{
					g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

						//C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
						//pHudBrowserInstance->SetUrl("asset://ui/welcome.html");
						//g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true);
				}
			}
		}

		return false;
	}

	// update this code block when joypad input gets restricted to the selected input slate instance!!
	// (currently it will assume that a selected cabinet with a libretro instances ALWAYS wants joypad input!!
	// ignore if its a libretro instance
	C_BaseEntity* pEntity = this->GetSelectedEntity();
	if (pEntity)
	{
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
		if (pShortcut)
		{
			std::vector<C_EmbeddedInstance*> embeddedInstances;
			pShortcut->GetEmbeddedInstances(embeddedInstances);

			C_LibretroInstance* pLibretroInstance;
			unsigned int max = embeddedInstances.size();
			if (bStartButtonHeld)
			{
				for (unsigned int i = 0; i < max; i++)
				{
					pLibretroInstance = dynamic_cast<C_LibretroInstance*>(m_pInputManager->GetEmbeddedInstance());
					if (pLibretroInstance)
						return true;
				}
			}
		}
	}

	if (m_pInputManager->GetInputMode())
	{
		if (this->GetMetaverseManager()->GetSpawningObjectEntity())
		{
			std::string mode = this->GetMetaverseManager()->GetLibraryBrowserContext("id");

			g_pAnarchyManager->DeactivateObjectPlacementMode(false);

			// undo changes AND cancel
			C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();
			g_pAnarchyManager->DeactivateObjectPlacementMode(false);	// FIXME: probably does absolutely NOTHING calling it twice, but make sure before removing this line.

			//std::string id = pShortcut->GetObjectId();
			//g_pAnarchyManager->GetInstanceManager()->ResetObjectChanges(pShortcut);

			// "save" cha
			//m_pInstanceManager->ApplyChanges(id, pShortcut);
			//DevMsg("CHANGES REVERTED\n");

			if (mode == "automove")
				this->AddToastMessage("Move Canceled");
			else
				this->AddToastMessage("Spawn Canceled");
		}
		else
		{
			// handle escape if in fullscreen input mode (drop out of fullscreen mode)
			if ((!m_pInputManager->GetFullscreenMode() || !this->GetSelectedEntity() || m_pInputManager->GetWasForceInputMode()) || (this->GetSelectedEntity() && m_pInputManager->GetFullscreenMode()))
			{
				if (!m_pInputManager->IsTempSelect())
				{
					m_pInputManager->DeactivateInputMode(true);
					g_pAnarchyManager->HudStateNotify();
					m_pFreeMouseModeConVar->SetValue(false);
				}
				else
				{
					g_pAnarchyManager->TaskRemember();
					g_pAnarchyManager->GetInputManager()->SetTempSelect(false);
					g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

					if (m_pFreeMouseModeConVar->GetBool())
					{
						C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

						std::string uri = "asset://ui/welcome.html";
						pHudBrowserInstance->SetUrl(uri);
						if (bStartButtonHeld)
							m_pInputManager->SetGamepadInputMode(true);
						m_pInputManager->ActivateInputMode(true, false);
					}
				}
			}
			else
			{
				m_pInputManager->SetFullscreenMode(false);	// never gets called?
			}
		}

		return true;
	}
	else if (!m_pInputManager->GetInputMode() && engine->IsInGame() )
	{
		// handle escape if not in input mode & map is loaded (display the main menu)
		//engine->IsInGame()
		//engine->IsPaused()
		if (!enginevgui->IsGameUIVisible())
		{
			if (this->GetAttractMode())
			{
				this->ToggleAttractMode();
				return true;
			}
			else if (this->GetCabinetAttractMode() && this->GetSelectedEntity())
			{
				this->DeselectEntity();
				return true;
			}

			C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

			std::string uri = "asset://ui/welcome.html";
			if (bIsFromToggleHandler)
			{
				m_pFreeMouseModeConVar->SetValue(true);
				uri += "?showmenu=1";
			}

			//DevMsg("DISPLAY MAIN MENU\n");
			if (m_pSelectedEntity)
			{
				DevMsg("Deselecting entity.\n");
				this->DeselectEntity(uri);
			}
			else
				pHudBrowserInstance->SetUrl(uri);

			//DevMsg("Trying to activate input mode.\n");
			//m_pInputManager->ActivateInputMode(true, true);	// NOTE: Disabled main-menu mode for the main menu (ironically) because now the main menu is like a task bar.
			//return false;

			if (bStartButtonHeld)
				m_pInputManager->SetGamepadInputMode(true);
			m_pInputManager->ActivateInputMode(true, false);
			return true;
		}
	}

	return false;
}

void C_AnarchyManager::ShowAdoptAssetMenu()
{

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

	std::string materialName = this->GetMaterialUnderCrosshair();
	std::string modelName = this->GetModelUnderCrosshair();

	std::string uri = std::string("asset://ui/adopt.html?modelFile=") + this->encodeURIComponent(modelName) + std::string("&materialFile=") + this->encodeURIComponent(materialName);

	if (m_pSelectedEntity)
		this->DeselectEntity(uri);
	else
		pHudBrowserInstance->SetUrl(uri);

	m_pInputManager->ActivateInputMode(true, false);
}

bool C_AnarchyManager::HandleCycleToNextWeapon()
{
	if (!engine->IsInGame())
		return false;

	if (this->GetMetaverseManager()->GetSpawningObject())
	{
		g_pAnarchyManager->GetInputManager()->OnMouseWheeled(1);
		return true;
	}
	else if (m_bIsHoldingPrimaryFire)
	{
		g_pAnarchyManager->GetInputManager()->OnMouseWheeled(1);
		return true;
	}
	else if (this->GetAttractMode())
	{
		this->FindNextAttractCamera();
		return true;
	}
	else if (this->GetCabinetAttractMode())
	{
		this->SelectNext();
		return true;
	}
	else if (!C_BasePlayer::LocalPlayerInFirstPersonView() && C_BasePlayer::GetLocalPlayer()->GetHealth() > 0 && !cvar->FindVar("allow_weapons")->GetBool())
	{
		ConVar* pConVar = cvar->FindVar("cam_idealdist");
		float flVal = pConVar->GetFloat();
		flVal -= 10.0f;
		if (flVal < 50.0f)
		{
			engine->ClientCmd("cam_idealdist 50; firstperson;");
			return true;
		}

		engine->ClientCmd(VarArgs("cam_idealdist %f; thirdperson;", flVal));
		return true;
	}

	return false;
}

bool C_AnarchyManager::HandleCycleToPrevWeapon()
{
	if (!engine->IsInGame())
		return false;

	if (this->GetMetaverseManager()->GetSpawningObject())
	{
		g_pAnarchyManager->GetInputManager()->OnMouseWheeled(-1);
		return true;
	}
	else if (this->m_bIsHoldingPrimaryFire)
	{
		g_pAnarchyManager->GetInputManager()->OnMouseWheeled(-1);
		return true;
	}
	else if (this->GetAttractMode())
	{
		this->FindPreviousAttractCamera();
		return true;
	}
	else if (this->GetCabinetAttractMode())
	{
		this->SelectPrev();
		return true;
	}
	else if (!C_BasePlayer::LocalPlayerInFirstPersonView() && C_BasePlayer::GetLocalPlayer()->GetHealth() > 0 && !cvar->FindVar("allow_weapons")->GetBool())
	{
		ConVar* pConVar = cvar->FindVar("cam_idealdist");
		float flVal = pConVar->GetFloat();
		flVal += 10.0f;
		if (flVal >= 1000.0f)
			flVal = 1000.0f;

		engine->ClientCmd(VarArgs("cam_idealdist %f; thirdperson;", flVal));
		return true;
	}
	else if (C_BasePlayer::LocalPlayerInFirstPersonView() && C_BasePlayer::GetLocalPlayer()->GetHealth() > 0 && !cvar->FindVar("allow_weapons")->GetBool())
	{
		engine->ClientCmd("cam_idealdist 50; thirdperson;");
		return true;
	}

	return false;
}

void C_AnarchyManager::DoPause(int iPauseModeOverride)
{
	//if (g_pAnarchyManager->GetInputManager()->GetInputMode())
	//	g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	//if (g_pAnarchyManager->GetSelectedEntity())
	//	g_pAnarchyManager->DeselectEntity("none");

	// Only makes sense if done from the main menu via ESCAPE (not tab)
	//if( !engine->IsPaused())
		//engine->ServerCmd("pause");
	//engine->ServerCmd("serverpause");

	engine->ClientCmd("setpause");	// servercmdfix
	
	// set rich presence
	std::string mapName = (engine->IsInGame()) ? this->MapName() : "";
	if (mapName != "")
	{
		std::string status = (this->GetConnectedUniverse()) ? "#Status_MultiplayerPaused" : "#Status_SingleplayerPaused";
		steamapicontext->SteamFriends()->SetRichPresence("steam_display", status.c_str());
	}
	else
		steamapicontext->SteamFriends()->SetRichPresence("steam_display", "#Status_AtMainMenuPaused");

	if (m_pInputManager->GetInputMode())
	{
		//C_EmbeddedInstance* pEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
		//m_pInputManager->SetEmbeddedInstance(null);
		//m_pInputManager->DeactivateInputMode(true);
	//	m_pInputManager->SetEmbeddedInstance(pEmbeddedInstance);

		//m_pInputManager->SetEmbeddedInstance(null);
		//g_pAnarchyManager->GetInputManager()->SetFullscreenMode(true);
		//g_pAnarchyManager->GetInputManager()->GetInputSlateCanvasTexture()
	//	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, null, false, false);
		g_pAnarchyManager->GetInputManager()->SetFullscreenMode(true);
	}
	else
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, null, true, false);
		//g_pAnarchyManager->GetInputManager()->SetFullscreenMode(true);
	g_pAnarchyManager->Pause(iPauseModeOverride);
}

void C_AnarchyManager::Pause(int iPauseModeOverride)
{
	ConVar* pOldPauseModeConVar = cvar->FindVar("old_pause_mode");
	ConVar* pPauseModeConVar = cvar->FindVar("pause_mode");
	if (iPauseModeOverride >= 0)
	{
		pOldPauseModeConVar->SetValue(pPauseModeConVar->GetInt());
		pPauseModeConVar->SetValue(iPauseModeOverride);
	}
	else
		pOldPauseModeConVar->SetValue(pPauseModeConVar->GetInt());

	ConVar* pVolumeConVar = cvar->FindVar("volume");
	cvar->FindVar("old_volume")->SetValue(pVolumeConVar->GetFloat());
	pVolumeConVar->SetValue(0);

	m_bPaused = true;

	if (this->GetConnectedUniverse() && this->GetConnectedUniverse()->connected)
		m_pMetaverseManager->PerformLocalPlayerUpdate();

	ConVar* pConVar = cvar->FindVar("engine_no_focus_sleep");
	m_oldEngineNoFocusSleep = pConVar->GetString();
	pConVar->SetValue(1000);

	m_iOldAlwaysAnimatingImagesValue = m_pAlwaysAnimatingImagesConVar->GetInt();
	m_pAlwaysAnimatingImagesConVar->SetValue(false);

	C_SteamBrowserInstance* pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("aai");
	if (pBrowserInstance)
		pBrowserInstance->Close();

	pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("SteamTalker");
	if (pBrowserInstance)
		pBrowserInstance->Close();

	if (pPauseModeConVar->GetInt() == 1)
		this->HardPause();
}

void C_AnarchyManager::Unpause()
{
	this->SetLastLaunchedItemId("");

	ConVar* pPauseModeConVar = cvar->FindVar("pause_mode");
	if (pPauseModeConVar->GetInt() == 1)
		this->WakeUp();

	if (m_bIsInSourceGame)
		m_bIsInSourceGame = false;

	pPauseModeConVar->SetValue(cvar->FindVar("old_pause_mode")->GetInt());

	cvar->FindVar("volume")->SetValue(cvar->FindVar("old_volume")->GetFloat());
	cvar->FindVar("engine_no_focus_sleep")->SetValue(Q_atoi(m_oldEngineNoFocusSleep.c_str()));
	m_bPaused = false;

	m_pAlwaysAnimatingImagesConVar->SetValue(m_iOldAlwaysAnimatingImagesValue);

	m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud")->SetUrl("asset://ui/welcome.html");
	m_pInputManager->DeactivateInputMode(true);
//	engine->ClientCmd("-input_mode");

	//if (m_pFreeMouseModeConVar->GetBool())
	//	this->ShowMouseMenu();

	if (cvar->FindVar("broadcast_mode")->GetBool())
	{
		// Write this live URL out to the save file.
		std::string XSPlitLiveFolder = cvar->FindVar("broadcast_folder")->GetString();
		if (XSPlitLiveFolder != "")
		{
			if (cvar->FindVar("broadcast_auto_game")->GetBool())
				this->WriteBroadcastGame("Anarchy Arcade");

			// Also update a JS file to force the page to re-load
			FileHandle_t hFile = g_pFullFileSystem->Open(VarArgs("%s\\vote.js", XSPlitLiveFolder.c_str()), "a+", "");
			if (hFile)
			{
				std::string code = "gAnarchyTV.OnAArcadeCommand(\"finishPlaying\");\n";
				g_pFullFileSystem->Write(code.c_str(), code.length(), hFile);
				g_pFullFileSystem->Close(hFile);
			}
		}
	}


	if (m_pMetaverseManager->IsTwitchBotEnabled() && m_pMetaverseManager->GetTwitchChannelLive() && cvar->FindVar("twitch_enabled")->GetBool() && m_pMetaverseManager->GetTwitchChannel() != "" && m_pMetaverseManager->GetTwitchUsername() != "" && m_pMetaverseManager->GetTwitchToken() != "" && m_pMetaverseManager->GetTwitchToken() != "********")
	{
		m_pMetaverseManager->OpenTwitchConnection();
		m_pMetaverseManager->SendTwitchChat("", "!game Anarchy Arcade");
	}

	// set rich presence
	if ( engine->IsInGame() )
	{
		//std::string status = (this->GetConnectedUniverse()) ? "#Status_Multiplayer" : "#Status_Singleplayer";
		//steamapicontext->SteamFriends()->SetRichPresence("steam_display", status.c_str());

		// set rich presence
		std::string mapName = (engine->IsInGame()) ? this->MapName() : "";
		steamapicontext->SteamFriends()->SetRichPresence("mapname", mapName.c_str());
		steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", m_pInstanceManager->GetInstanceObjectCount()));
		std::string status = (this->GetConnectedUniverse()) ? "#Status_Multiplayer" : "#Status_Singleplayer";
		if (this->GetConnectedUniverse())
			steamapicontext->SteamFriends()->SetRichPresence("connect", VarArgs("+join %s", this->GetConnectedUniverse()->lobby.c_str()));
		else
			steamapicontext->SteamFriends()->SetRichPresence("connect", null);

		steamapicontext->SteamFriends()->SetRichPresence("steam_display", status.c_str());
	}
	else
		steamapicontext->SteamFriends()->SetRichPresence("steam_display", "#Status_AtMainMenu");

	g_pAnarchyManager->GetAccountant()->StoreStats();
}

bool C_AnarchyManager::AlphabetSafe(std::string text, std::string in_alphabet)
{
	std::string alphabet = (in_alphabet != "") ? in_alphabet : " 1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`-=[]\\;',./~!@#$%^&*()_+{}|:<>?";
	
	//char textBuf[AA_MAX_STRING];
	int iAAMaxString = text.length() + 1;
	char* textBuf = new char[iAAMaxString];
	Q_strncpy(textBuf, text.c_str(), iAAMaxString);

	unsigned int i = 0;
	char tester = textBuf[i];
	while (tester)
	{
		if (alphabet.find(tester) == std::string::npos)
		{
			delete[] textBuf;
			return false;
		}

		i++;
		tester = textBuf[i];
	}

	delete[] textBuf;

	return true;
}

bool C_AnarchyManager::PrefixSafe(std::string text)
{
	// check for web
	if (text.find("http://") == 0 || text.find("https://") == 0 || text.find("steam://") == 0)
		return true;

	// check for integer-only value
	if (!Q_strcmp(VarArgs("%llu", Q_atoui64(text.c_str())), text.c_str()))
		return true;

	// check for single driveletter synatx
	std::string driveAlphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (driveAlphabet.find(text.at(0)) != std::string::npos && text.at(1) == ':' && (text.at(2) == '\\' || text.at(2) == '/'))
		return true;

	/*
	std::regex re("[a-zA-Z]:[\\\\/].*");
	std::smatch match;
	if (std::regex_search(text, match, re) && match.size() > 1)
		return true;
	*/

	// NOT web, NOT steam app ID, and INVALID driveletter syntax
	return false;
}

bool C_AnarchyManager::DirectorySafe(std::string text)
{
	// directories do not begin with a blank space or a % symbol
	if (text.at(0) == ' ' || text.at(0) == '%' )
		return false;

	// directories also need to start with a safe character (that is out of the scope of this method.  AlphabetSafe can always be called to test that.
	//if (!this->AlphabetSafe(text.substr(0, 1)))
	//	 return false;

	// - The following sequences should not be allowed: /.., \..
	if (text.find("/..") != std::string::npos || text.find("\\..") != std::string::npos)
		return false;

	// - The following folders should be blacklisted: driveletter:/windows, driveletter:\windows
	if (text.length() > 7)
	{
		if (!Q_stricmp(text.substr(3, 7).c_str(), "windows"))
			return false;
	}

	return true;
}

void C_AnarchyManager::ClearLookAtObjects()
{
	m_alwaysLookObjects.clear();
}

void C_AnarchyManager::ToggleAlwaysLookObject(C_PropShortcutEntity* pShortcut)
{
	bool bFound = false;
	signed int iIndex;
	for (unsigned int i = 0; i < m_alwaysLookObjects.size(); i++)
	{
		if (m_alwaysLookObjects[i] == pShortcut)
		{
			this->RemoveAlwaysLookObject(pShortcut);
			return;
		}
	}

	this->AddAlwaysLookObject(pShortcut);
	//this->QuickRemember(pShortcut->entindex());
}

void C_AnarchyManager::AddAlwaysLookObject(C_PropShortcutEntity* pShortcut)
{
	m_alwaysLookObjects.push_back(pShortcut);
}

void C_AnarchyManager::RemoveAlwaysLookObject(C_PropShortcutEntity* pShortcut)
{
	bool bFound = false;
	signed int iIndex;
	for (unsigned int i = 0; i < m_alwaysLookObjects.size(); i++)
	{
		if (m_alwaysLookObjects[i] == pShortcut)
		{
			bFound = true;
			iIndex = i;
			break;
		}
	}

	if (bFound)
	{
		m_alwaysLookObjects.erase(m_alwaysLookObjects.begin() + iIndex);
		object_t* pObject = m_pInstanceManager->GetInstanceObject(pShortcut->GetObjectId());
		if (pObject)
		{
			engine->ClientCmd(VarArgs("setcabangles %i %f %f %f;\n", pShortcut->entindex(), pObject->angles.x, pObject->angles.y, pObject->angles.z));
			//engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z));//	// servercmdfix , false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.
		}
	}
}

void C_AnarchyManager::ManageAlwaysLookObjects()
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	/*
	float fPlayerHeight = 60.0f;
	Vector playerOrigin = pPlayer->GetAbsOrigin();
	playerOrigin.z += fPlayerHeight;
	*/

	C_BaseEntity* pViewEntity = C_BaseEntity::Instance(render->GetViewEntity());
	if (!pViewEntity)
		return;

	Vector playerEyePosition = pViewEntity->EyePosition();

	C_PropShortcutEntity* pShortcut;
	Vector shortcutOrigin;
	Vector shortcutForward;
	Vector shortcutRight;
	Vector shortcutUp;
	QAngle shortcutAngles;
	QAngle currentAngles;
	for (unsigned int i = 0; i < m_alwaysLookObjects.size(); i++)
	{
		pShortcut = m_alwaysLookObjects[i];
		currentAngles = pShortcut->GetAbsAngles();
		shortcutOrigin = pShortcut->GetAbsOrigin();
		pShortcut->GetVectors(&shortcutForward, &shortcutRight, &shortcutUp);

		// TEMP BILLBOARD TEST
		//playerOrigin.z = shortcutOrigin.z;

		Vector vView = playerEyePosition - shortcutOrigin;
		VectorAngles(vView, shortcutUp, shortcutAngles);
		if (shortcutAngles.x != currentAngles.x || shortcutAngles.y != currentAngles.y)
			engine->ClientCmd(VarArgs("setcabangles %i %f %f %f;\n", pShortcut->entindex(), shortcutAngles.x, shortcutAngles.y, shortcutAngles.z));
			//engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), shortcutOrigin.x, shortcutOrigin.y, shortcutOrigin.z, shortcutAngles.x, shortcutAngles.y, shortcutAngles.z));//	// servercmdfix , false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.
	
		
		
		//engine->ClientCmd(VarArgs("setang %f %f %f; ", shortcutAngles.x, shortcutAngles.y, shortcutAngles.z));
		//pShortcut->SetAbsAngles(shortcutAngles);
		//DevMsg("%02f %02f %02f\n", shortcutAngles.x, shortcutAngles.y, shortcutAngles.z);
	}
}

C_PropShortcutEntity* C_AnarchyManager::GetBestSpectatorCameraObject()
{
	Vector playerOrigin = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();

	int iBestIndex = -1;
	float flBestDistance = -1;
	float flTestDistance;
	for (unsigned int i = 0; i < m_alwaysLookObjects.size(); i++)
	{
		if (i == 0)
		{
			flBestDistance = m_alwaysLookObjects[i]->GetAbsOrigin().DistTo(playerOrigin);
			iBestIndex = i;
		}
		else
		{
			flTestDistance = m_alwaysLookObjects[i]->GetAbsOrigin().DistTo(playerOrigin);
			if (flTestDistance < flBestDistance)
			{
				flBestDistance = m_alwaysLookObjects[i]->GetAbsOrigin().DistTo(playerOrigin);
				iBestIndex = i;
			}
		}
	}

	if (iBestIndex >= 0)
	{
		C_PropShortcutEntity* pBestEntity = m_alwaysLookObjects[iBestIndex];

		if (!m_pLocalAvatarObject)
		{
			if (!m_bIsCreatingLocalAvatar)
			{
				m_bIsCreatingLocalAvatar = true;
				/*
				1 - modelFile
				2 - origin X
				3 - origin Y
				4 - origin Z
				5 - angles P
				6 - angles Y
				7 - angles R
				8 - userId
				*/
				std::vector<std::string> modelNames;
				modelNames.push_back("models/players/tube0.mdl");
				/*
				modelNames.push_back("models/players/heads/cowboycarl.mdl");
				modelNames.push_back("models/players/heads/flipflopfred.mdl");
				modelNames.push_back("models/players/heads/hackerhaley.mdl");
				modelNames.push_back("models/players/heads/ninjanancy.mdl");
				modelNames.push_back("models/players/heads/zombiejoe.mdl");
				*/

				unsigned int index = rand() % modelNames.size();	// non-uniform, but who cares :S
				std::string modelName = modelNames[index];

				Vector cameraOrigin = pBestEntity->GetAbsOrigin();
				QAngle cameraAngles = pBestEntity->GetAbsAngles();

				/*float x = pUser->bodyOrigin.x;
				float y = pUser->bodyOrigin.y;
				float z;
				float fPlayerHeight = 60.0f;
				// FIXME: Simply checking if bodyorigin is equal to 0 is NOT enough to be sure it's a web client! (sometimes 0 z is correct, after all.)
				if (pUser->bodyOrigin.z == 0)
				{
				C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
				Vector localPlayerOrigin = pLocalPlayer->GetAbsOrigin();
				float fFudgeHeight = localPlayerOrigin.z + fPlayerHeight;
				z = fFudgeHeight;// localPlayerOrigin.z;//pUser->bodyOrigin.z
				}
				else
				z = pUser->bodyOrigin.z + fPlayerHeight;
				//userSessionUpdated

				// FIXME: Need to make this use same logic as choosing a player spawn point, in case the local player is inside of a wall when a new player joins.
				*/
				engine->ClientCmd(VarArgs("create_local_avatar_object \"%s\" %f %f %f %f %f %f;\n", modelName.c_str(), cameraOrigin.x, cameraOrigin.y, cameraOrigin.z, cameraAngles.x, cameraAngles.y, cameraAngles.z));// , false);
			}
		}

		return pBestEntity;
	}

	return null;
}

void C_AnarchyManager::LocalAvatarObjectCreated(int iEntIndex)
{
	m_pLocalAvatarObject = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iEntIndex));
}

void C_AnarchyManager::AddSubKeysToKeys(KeyValues* kv, KeyValues* targetKV)
{
	if (!kv)
		return;

	for (KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (sub->GetFirstSubKey())
		{
			KeyValues* key = targetKV->FindKey(sub->GetName(), true);
			AddSubKeysToKeys(sub, targetKV);
		}
		else
			targetKV->SetString(sub->GetName(), sub->GetString());
	}
}

bool C_AnarchyManager::LoadMapCommand(std::string mapId, std::string instanceId, std::string position, std::string rotation, std::string screenshotId)
{
	// reset fog overrides
	engine->ClientCmd("fog_override 0; fog_enable 1; fog_enableskybox 1;");

	m_bPrecacheInstances = cvar->FindVar("precache_instances")->GetBool();
	m_bBatchObjectSpawn = cvar->FindVar("spawn_objects_in_batches")->GetBool();

	if (instanceId != "" && !m_pInstanceManager->FindInstance(instanceId) && m_nextLobby == "")
	{
		DevMsg("ERROR: Could not load. Instance not found.\n");
		return false;
	}

	KeyValues* map = g_pAnarchyManager->GetMetaverseManager()->GetMap(mapId);
	KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(map);
	if (!active)
	{
		DevMsg("ERROR: Could not load. Map not found.\n");
		return false;
	}

	std::string mapName = active->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");

	if (!g_pFullFileSystem->FileExists(VarArgs("maps/%s", mapName.c_str()), "GAME"))
	{
		DevMsg("ERROR: Could not load. BSP not found.\n");
		return false;
	}

	mapName = mapName.substr(0, mapName.length() - 4);

	/*
	aampConnection_t* pConnectedUniverse = g_pAnarchyManager->GetConnectedUniverse();
	if (pConnectedUniverse)
	{
		DevMsg("%s vs %s\n", pConnectedUniverse->lobby.c_str(), m_nextLobby.c_str());
	}
	*/

	std::string title = (m_nextLobby != "" && m_nextLobbyTitle != "") ? m_nextLobbyTitle : "Unnamed (" + mapName + ")";

	//if (m_currentLobbyTitle != "")
	//	DevMsg("%s\n", m_currentLobbyTitle.c_str());

	m_pMetaverseManager->SetLoadingScreenshotId(screenshotId);

	if (instanceId == "")
		instanceId = g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(0, null, "", mapId, title);
	else
	{
		if (m_nextLobby != "" && !g_pAnarchyManager->GetInstanceManager()->FindInstance(instanceId))
			g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(0, null, instanceId, mapId, title);
	}

	g_pAnarchyManager->SetNextLoadInfo(instanceId, position, rotation);
	//g_pAnarchyManager->SetNextInstanceId(instanceId);

	//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
	//C_AwesomiumBrowserInstance* pHudInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	//std::string url = "asset://ui/loading.html?map=" + mapId + "&instance=" + instanceId;
	//pHudInstance->SetUrl(url);

	if (this->ShouldPrecacheInstance())
	{
		KeyValues* pPrecacheModelsKV = new KeyValues("models");
		KeyValues* pModelKV;
		std::string modelId;

		KeyValues* dummyKV = g_pAnarchyManager->GetInstanceManager()->LoadInstance(null, instanceId, "", "", true);
		KeyValues* dummyObjectsKV = dummyKV->FindKey("objects", true);
		for (KeyValues *sub = dummyObjectsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			modelId = sub->GetString("local/model");
			if (!pPrecacheModelsKV->FindKey(modelId.c_str()))
			{
				pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId));
				if (pModelKV)
					pPrecacheModelsKV->SetString(modelId.c_str(), pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
			}
		}
		dummyKV->deleteThis();

		pPrecacheModelsKV->SaveToFile(g_pFullFileSystem, "precache_models.txt", "DEFAULT_WRITE_PATH");
	}
	//else if (!this->ShouldPrecacheInstance() && g_pFullFileSystem->FileExists("precache_models.txt", "DEFAULT_WRITE_PATH"))
	//	g_pFullFileSystem->RemoveFile("precache_models.txt", "DEFAULT_WRITE_PATH");

	if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && !m_pMetaverseManager->GetHasDisconnected() && g_pAnarchyManager->GetConnectedUniverse()->isHost && !g_pAnarchyManager->GetConnectedUniverse()->isPersistent )
	{
		DevMsg("Sending map change notification...\n");

		if (instanceId != "")
			cvar->FindVar("host_next_map")->SetValue(true);

		m_pMetaverseManager->SendChangeInstanceNotification(instanceId, mapName);
	}
	else
		engine->ClientCmd(VarArgs("map \"%s\"\n", mapName.c_str()));

	return true;
}

bool C_AnarchyManager::WeaponsEnabled()
{
	if (!m_pWeaponsEnabledConVar)
		return false;
	else
		return m_pWeaponsEnabledConVar->GetBool();
}

bool C_AnarchyManager::ExecutableSafe(std::string text)
{
	// RULES:
	// 1. [text] must only consist of characters in the safe alphabet.
	// 2. [text] should be required to start with: http://, https://, steam://, driveletter:/, driveletter:\, or JUST an integer (to indicate a Steam App ID)
	// 3. The following sequences should not be allowed: /.., \..
	// 4. The following folders should be blacklisted: driveletter:/windows, driveletter:\windows
	// 5. Environment variable paths are not allowed: %APPDATA%, etc.
	// 6. Directories cannot begin with a blank space.

	return (this->PrefixSafe(text) && this->DirectorySafe(text));
}

uint64 C_AnarchyManager::GetTimeNumber()
{
	time_t currentrawtime = time(NULL);
	tm* timeinfo = localtime(&currentrawtime);
	return Q_atoui64(VarArgs("%llu", currentrawtime));

	/* whats this for?
	uint64 bigtime = currentrawtime;
	int lowerHalf = (int)((bigtime << 32) >> 32);
	int upperHalf = (int)((bigtime >> 32) << 32);

	time_t constructedTime = lowerHalf | (upperHalf << 32);

	tm* constructedtimeinfo = localtime(&constructedTime);
	DevMsg("Constructed time is: %s\n", asctime(constructedtimeinfo));
	DevMsg("As a uint64 it is: %llu\n", constructedTime);
	asctime(timeinfo)
	*/
}

std::string C_AnarchyManager::GetTimeString()
{
	time_t currentrawtime = time(NULL);
	tm* timeinfo = localtime(&currentrawtime);
	std::string timeString = VarArgs("%s", asctime(timeinfo));
	return timeString;
}

void C_AnarchyManager::SetLastLaunchedItemId(std::string value)
{
	m_lastLaunchedItemId = value;
	//if (this->GetConnectedUniverse() && this->GetConnectedUniverse()->connected)
	//	m_pMetaverseManager->SyncLastLaunchedItemId();
}

ConVar znear("znear", "7", FCVAR_ARCHIVE, "The nearest things can get to the camera before they get clipped.  Also impacts z-fighting of holograms that are far away against walls.");
ConVar znearvr("znearvr", "2", FCVAR_ARCHIVE, "(VR) The nearest things can get to the camera before they get clipped.  Also impacts z-fighting of holograms that are far away against walls.");
float C_AnarchyManager::GetZNear()
{
	float flNear;
	if (this->IsVRActive())
		flNear = znearvr.GetFloat();
	else
		flNear = znear.GetFloat();

	if (flNear < 2.0f)
		flNear = 2.0f;
	else if (flNear > 10.0f)
		flNear = 10.0f;

	return flNear;
}

void C_AnarchyManager::PrepPano()
{
	m_pPanoStuff->hud = cvar->FindVar("cl_drawhud")->GetString();
	m_pPanoStuff->weapons = cvar->FindVar("r_drawviewmodel")->GetString();
	m_pPanoStuff->titles = cvar->FindVar("cl_hovertitles")->GetString();
	m_pPanoStuff->toast = cvar->FindVar("cl_toastmsgs")->GetString();
	m_pPanoStuff->developer = cvar->FindVar("developer")->GetString();
	m_pPanoStuff->hdrlevel = cvar->FindVar("mat_hdr_level")->GetString();
	m_pPanoStuff->exposuremax = cvar->FindVar("mat_autoexposure_max")->GetString();
	m_pPanoStuff->exposuremin = cvar->FindVar("mat_autoexposure_min")->GetString();
	m_pPanoStuff->fov = VarArgs("%f", this->GetLastFOV());

	g_pAnarchyManager->SetLastFOV(106);
	engine->ExecuteClientCmd("firstperson; cl_drawhud 0; r_drawviewmodel 0; cl_hovertitles 0; cl_toastmsgs 0; developer 0; jpeg_quality 97; fov 106;");
	//g_pAnarchyManager->SetLastFOV(121);
	//engine->ExecuteClientCmd("firstperson; cl_drawhud 0; r_drawviewmodel 0; cl_hovertitles 0; cl_toastmsgs 0; developer 0; jpeg_quality 97; fov 121;");
}


void C_AnarchyManager::FinishPano()
{
	g_pAnarchyManager->SetLastFOV(Q_atof(m_pPanoStuff->fov.c_str()));
	engine->ClientCmd(VarArgs("cl_drawhud %s; r_drawviewmodel %s; cl_hovertitles %s; cl_toastmsgs %s; developer %s; mat_hdr_level %s; mat_autoexposure_max %s; mat_autoexposure_min %s; fov %s; lastfov %s;", m_pPanoStuff->hud.c_str(), m_pPanoStuff->weapons.c_str(), m_pPanoStuff->titles.c_str(), m_pPanoStuff->toast.c_str(), m_pPanoStuff->developer.c_str(), m_pPanoStuff->hdrlevel.c_str(), m_pPanoStuff->exposuremax.c_str(), m_pPanoStuff->exposuremin.c_str(), m_pPanoStuff->fov.c_str(), m_pPanoStuff->fov.c_str()));
}

C_SteamBrowserInstance* C_AnarchyManager::AutoInspect(KeyValues* pItemKV, std::string tabId, int iEntIndex, std::string imageFlagsOverride)
{
	std::string imageFlagsToUse = (imageFlagsOverride == "") ? g_pAnarchyManager->GetAutoInspectImageFlags() : imageFlagsOverride;
	KeyValues* pActiveKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItemKV);
	std::string itemId = pActiveKV->GetString("info/id");
	std::string tabTitle = (tabId == "") ? "auto" + itemId : tabId;

	// IF this is a LOCAL file that is also an MP3, then try to give all sibling files in the URL request.
	std::string otherFiles = "";

	if (m_pLocalAutoPlaylistsConVar->GetBool())
	{
		std::string file = pActiveKV->GetString("file");
		if (file.find(':') == 1)
		{
			std::string testPath = file;
			std::transform(testPath.begin(), testPath.end(), testPath.begin(), ::tolower);
			std::replace(testPath.begin(), testPath.end(), '\\', '/');

			size_t foundTestPathSlash = testPath.find_last_of("/");
			if (foundTestPathSlash != std::string::npos)
			{
				testPath = testPath.substr(0, foundTestPathSlash);

				std::string fileExtension = file;
				std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

				size_t extensionFound = fileExtension.find_last_of(".");
				if (extensionFound != std::string::npos)
					fileExtension = fileExtension.substr(extensionFound + 1);
				else
					fileExtension = "";

				if (fileExtension == "mp3" && g_pFullFileSystem->FileExists(file.c_str()))
				{
					int iNumFiles = 0;
					FileFindHandle_t findHandle;
					std::string otherFile;
					const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%s/*.%s", testPath.c_str(), fileExtension.c_str()), "", &findHandle);
					while (pFilename != NULL && iNumFiles < 20)
					{
						//otherFile = VarArgs("%s/%s", testPath.c_str(), pFilename);
						otherFiles += VarArgs("&f%i=", iNumFiles) + g_pAnarchyManager->encodeURIComponent(pFilename);// otherFile);
						iNumFiles++;
						pFilename = g_pFullFileSystem->FindNext(findHandle);
					}
					g_pFullFileSystem->FindClose(findHandle);
				}
			}
		}
	}

	// assume we want to show this in a steam browser instance
	std::string uri = "file://";
	uri += engine->GetGameDirectory();
	uri += "/resource/ui/html/autoInspectItem.html?imageflags=" + imageFlagsToUse + "&id=" + g_pAnarchyManager->encodeURIComponent(itemId) + "&title=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("title")) + "&screen=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("screen")) + "&marquee=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("marquee")) + "&preview=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("preview")) + "&reference=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("reference")) + "&stream=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("stream")) + "&file=" + g_pAnarchyManager->encodeURIComponent(pActiveKV->GetString("file")) + "&plbehavior=" + std::string(g_pAnarchyManager->GetYouTubePlaylistBehaviorConVar()->GetString()) + "&annotations=" + std::string(g_pAnarchyManager->GetYouTubeAnnotationsConVar()->GetString()) + "&mixes=" + std::string(g_pAnarchyManager->GetYouTubeMixesConVar()->GetString()) + "&related=" + std::string(g_pAnarchyManager->GetYouTubeRelatedConVar()->GetString()) + "&vbehavior=" + std::string(g_pAnarchyManager->GetYouTubeVideoBehaviorConVar()->GetString()) + "&endbehavior=" + std::string(g_pAnarchyManager->GetYouTubeEndBehaviorConVar()->GetString());
	if (otherFiles != "")
		uri += otherFiles;

	// FIXME: Need to allow HTTP redirection (302).

	// If the currently selected input is ALSO a steamworks browser, we need to make sure it retains focus.
	C_EmbeddedInstance* pInputEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
	C_SteamBrowserInstance* pInputSteamInstance = dynamic_cast<C_SteamBrowserInstance*>(pInputEmbeddedInstance);

	C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
	pSteamBrowserInstance->Init(tabTitle, uri, "Newly selected item...", null, iEntIndex);
	pSteamBrowserInstance->SetOriginalItemId(itemId);	// FIXME: do we need to do this for original entindex too???
	pSteamBrowserInstance->SetOriginalEntIndex(iEntIndex);	// probably NOT needed?? (or maybe so, from here.)

	return pSteamBrowserInstance;
}

bool C_AnarchyManager::DetermineStreamCompatible(KeyValues* pItemKV)
{
	KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItemKV);
	bool bStreamIsUri = (std::string(active->GetString("stream")).find("http") == 0) ? true : false;
	//bool bFileIsUri = (std::string(active->GetString("file")).find("http") == 0) ? true : false;
	//bool bPreviewIsUri = (std::string(active->GetString("preview")).find("http") == 0) ? true : false;
	return bStreamIsUri;
}

bool C_AnarchyManager::DeterminePreviewCompatible(KeyValues* pItemKV)
{
	KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItemKV);
	bool bPreviewIsUri = (std::string(active->GetString("preview")).find("http") == 0) ? true : false;
	//bool bFileIsUri = (std::string(active->GetString("file")).find("http") == 0) ? true : false;
	//bool bPreviewIsUri = (std::string(active->GetString("preview")).find("http") == 0) ? true : false;
	return bPreviewIsUri;
}

void C_AnarchyManager::SetSystemCursor(unsigned long cursor, std::string cursorName)
{
	if (!m_pInputManager->GetInputSlate())
		return;

	if ((!this->IsVRActive() || !this->IsHandTrackingActive()) && m_pInputManager->GetFullscreenMode())
		m_pInputManager->GetInputSlate()->SetCursor(cursor);

	if (cursorName != "" && m_cursor != cursor)
	{
		std::vector<std::string> params;
		params.push_back(cursorName);
		m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud")->DispatchJavaScriptMethod("arcadeHud", "setCursor", params);
	}

	m_cursor = cursor;
}

bool C_AnarchyManager::DetermineLibretroCompatible(KeyValues* pItemKV, std::string& gameFile, std::string& coreFile)
{
	bool bShouldLibretroLaunch = false;
	//if (m_pLibretroManager->GetInstanceCount() == 0)
	//{
		KeyValues* active = m_pMetaverseManager->GetActiveKeyValues(pItemKV);
		// 1. resolve the file
		// 2. check if the file is of correct path & extension of a core
		// 3. confirm file exists
		// 4. run in libretro (or bDoAutoInspect if fail)

		std::string testCoreFile = "";
		std::string file = active->GetString("file");

		bool bFileIsGood = false;
		// is the path relative to the app?
		size_t found = file.find(":");
		if (found == 1 && g_pFullFileSystem->FileExists(file.c_str()))
		{
			bFileIsGood = true;
		}
		else
		{
			// does it have an app?
			if (Q_strcmp(active->GetString("app"), ""))
			{
				bool bHasApp = true;
				KeyValues* app = g_pAnarchyManager->GetMetaverseManager()->GetLibraryApp(active->GetString("app"));
				KeyValues* appActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(app);
				if (appActive)
				{
					bool bHasAppFilepath = false;
					bool bAtLeastOneAppFilepathExists = false;

					// just grab the FIRST filepath for now.
					// FIXME: Need to keep searching through filepaths until the item's file is found inside of one.
					// Note: Apps are not required to have a filepath specified.
					std::string testFile;
					std::string testPath;
					KeyValues* filepaths = appActive->FindKey("filepaths", true);
					for (KeyValues *sub = filepaths->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						// true if even 1 filepath exists for the app, even if it is not found on the local PC.
						// (because in that case the local user probably needs to specify a correct location for it.)
						bHasAppFilepath = true;

						testPath = sub->GetString("path");

						// test if this path exists
						// FIXME: always assume it exists for now
						if (true)
						{
							bAtLeastOneAppFilepathExists = true;

							// test if the file exists inside of this filepath
							testFile = testPath + file;

							// FIXME: always assume the file exists in this path for now.
							if (true)
							{
								file = testFile;
								bFileIsGood = true;
								break;
							}
						}
					}
				}
			}
		}

		if (bFileIsGood && file[1] == ':')
		{
			std::string fileExtension = file;
			std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

			size_t extensionFound = fileExtension.find_last_of(".");
			if (extensionFound != std::string::npos)
				fileExtension = fileExtension.substr(extensionFound + 1);
			else
				fileExtension = "";

			if (fileExtension != "")
			{
				bool bExtensionMatch;
				std::string extensions;
				std::vector<std::string> extensionTokens;

				std::string testPath = file;
				std::transform(testPath.begin(), testPath.end(), testPath.begin(), ::tolower);
				std::replace(testPath.begin(), testPath.end(), '\\', '/');

				size_t foundTestPathSlash = testPath.find_last_of("/");
				if (foundTestPathSlash != std::string::npos)
				{
					testPath = testPath.substr(0, foundTestPathSlash);

					size_t foundContentPathSlash;
					std::string contentPath;
					KeyValues* pCoreSettingsKV = m_pLibretroManager->GetCoreSettingsKV();
					for (KeyValues* pCoreKV = pCoreSettingsKV->GetFirstSubKey(); pCoreKV; pCoreKV = pCoreKV->GetNextKey())
					{
						if (!pCoreKV->GetBool("enabled") || !pCoreKV->GetBool("exists"))
							continue;

						for (KeyValues* pPathKV = pCoreKV->FindKey("paths", true)->GetFirstSubKey(); pPathKV; pPathKV = pPathKV->GetNextKey())
						{
							bExtensionMatch = false;
							extensions = pPathKV->GetString("extensions");
							extensionTokens.clear();

							if (extensions != "")
							{
								this->Tokenize(extensions, extensionTokens, ", ");

								// Check if the extension is found
								for (unsigned int i = 0; i < extensionTokens.size(); i++)
								{
									//DevMsg("Testing %s vs %s\n", extensionTokens[i].c_str(), fileExtension.c_str());
									if (extensionTokens[i] == fileExtension)
									{
										bExtensionMatch = true;
										break;
									}
								}
							}

							if (bExtensionMatch || extensionTokens.empty())
							{
								// Check if the path matches
								contentPath = pPathKV->GetString("path");
								std::transform(contentPath.begin(), contentPath.end(), contentPath.begin(), ::tolower);
								std::replace(contentPath.begin(), contentPath.end(), '\\', '/');

								foundContentPathSlash = contentPath.find_last_of("/");
								if (foundContentPathSlash == contentPath.length() - 1)
									contentPath = contentPath.substr(0, foundContentPathSlash);

								if (contentPath == "" || testPath.find(contentPath) == 0)//testPath == contentPath)
								{
									bShouldLibretroLaunch = true;
									break;
								}
							}
						}

						if (bShouldLibretroLaunch)
						{
							testCoreFile = pCoreKV->GetString("file");
							break;
						}
					}

					if (bShouldLibretroLaunch)
					{
						gameFile = file;
						coreFile = testCoreFile;
					}
					//return true;
				}
			}
		}
	//}

	return bShouldLibretroLaunch;
}

void C_AnarchyManager::WriteBroadcastGame(std::string gameTitle)
{
	cvar->FindVar("broadcast_game")->SetValue(gameTitle.c_str());

	std::string broadcastFolder = cvar->FindVar("broadcast_folder")->GetString();
	FileHandle_t hFile = g_pFullFileSystem->Open(VarArgs("%s\\game.txt", broadcastFolder.c_str()), "w+", "");
	if (hFile)
	{
		std::string xml = "";
		xml += "<div class=\"response\">\n";
		xml += "\t<activetitle class=\"activetitle\">";

		std::string xmlBuf = gameTitle;

		// Make it XML safe
		size_t found = xmlBuf.find("&");
		while (found != std::string::npos)
		{
			xmlBuf.replace(found, 1, "&amp;");
			found = xmlBuf.find("&", found + 5);
		}

		found = xmlBuf.find("<");
		while (found != std::string::npos)
		{
			xmlBuf.replace(found, 1, "&lt;");
			found = xmlBuf.find("<", found + 4);
		}

		found = xmlBuf.find(">");
		while (found != std::string::npos)
		{
			xmlBuf.replace(found, 1, "&gt;");
			found = xmlBuf.find(">", found + 4);
		}

		xml += xmlBuf;

		xml += "</activetitle>\n";

		xml += "</div>";

		g_pFullFileSystem->Write(xml.c_str(), xml.length(), hFile);
		g_pFullFileSystem->Close(hFile);
	}
}

launchErrorType_t C_AnarchyManager::LaunchItem(std::string id)
{
	/*
	// these items need improvements on how they are launched (they didn't work):
	Launching Item:
	Executable: C:\Users\Owner\Desktop\launcher - Shortcut.lnk
	Directory:
	Master Commands:

	ALSO roms with full file locations as their file instead of just the short filename.
	*/
	//DevMsg("LAUNCH THE SHIT!\n");

	// genereate the executable, executableDirectory, and masterCommands values.
	// then give these to the actual launch method.

	bool bUnknownError = false;

	// user resolvable errors to catch
	bool bItemGood = false;
	bool bItemFileGood = false;
	bool bItemPathAllowed = false;
	bool bAppGood = false;
	bool bAppExecutableGood = false;
	bool bAppFilepathGood = false;
	bool bAtLeastOneAppFilepathExists = false;
	bool bReadyToActuallyLaunch = false;

	// required fields for ArcadeCreateProcess
	std::string executable;
	std::string executableDirectory;
	std::string masterCommands;

	// other fields used to generate the required fields
	bool bHasApp = false;
	bool bHasAppFilepath = false;
	KeyValues* item = null;	// the KV of the item being used.
	KeyValues* itemActive = null;	// the active node of the item KV.
	std::string file;
	std::string composedFile;
	KeyValues* app = null;
	KeyValues* appActive = null;
	std::string appExecutable;
	std::string appFilepath;
	std::string appCommands;

	bool bSourceQuickFix = false;

	launchErrorType_t errorType = NONE;
	/*
	if (!bItemGood)
	errorType = ITEM_NOT_FOUND;
	else if (!bItemFileGood)
	errorType = ITEM_FILE_NOT_FOUND;
	else if (!bItemPathAllowed)
	errorType = ITEM_FILE_PATH_RESTRICTED;
	else if (!bAppGood)
	errorType = APP_NOT_FOUND;
	else if (!bAppExecutableGood)
	errorType = APP_FILE_NOT_FOUND;
	else if (!bAppFilepathGood)
	errorType = APP_PATH_NOT_FOUND;
	*/

	// attempt to get the item
	item = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(id);
	if (item)
	{
		// if there is an item, attempt to get the active node kv
		itemActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(item);
		bItemGood = true;

		// if there is an active node kv for the item, attempt to get file
		file = itemActive->GetString("file");

		if (file != "")
		{
			// does it have an app?
			if (Q_strcmp(itemActive->GetString("app"), "") && Q_strcmp(itemActive->GetString("app"), "Windows (default)"))	// to catch erronous items created by the bugged create & edit item menus
			{
				bHasApp = true;

				app = g_pAnarchyManager->GetMetaverseManager()->GetLibraryApp(itemActive->GetString("app"));
				appActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(app);

				// if there is an app, attempt to get its executable
				if (appActive)
				{
					bAppGood = true;

					appExecutable = appActive->GetString("file");
					if (appExecutable != "" && g_pFullFileSystem->FileExists(appExecutable.c_str()))
					{
						bAppExecutableGood = true;
						appCommands = appActive->GetString("commandformat");


						std::string slash = "";
						std::string otherSlash = "";
						if (appCommands.find("\\") != std::string::npos)
						{
							slash = "\\";
							otherSlash = "/";
						}
						else if (appCommands.find("/") != std::string::npos)
						{
							slash = "/";
							otherSlash = "\\";
						}

						// just grab the FIRST filepath for now.
						// FIXME: Need to keep searching through filepaths until the item's file is found inside of one.
						// Note: Apps are not required to have a filepath specified.
						std::string shortFileOnly = file;
						bool bShortFileResolved = false;
						std::string testFile;
						std::string testPath;
						KeyValues* filepaths = appActive->FindKey("filepaths", true);
						for (KeyValues *sub = filepaths->GetFirstSubKey(); sub; sub = sub->GetNextKey())
						{
							// true if even 1 filepath exists for the app, even if it is not found on the local PC.
							// (because in that case the local user probably needs to specify a correct location for it.)
							bHasAppFilepath = true;

							testPath = sub->GetString("path");

							// remove any trailing slash
							size_t foundLastSlash = testPath.find_last_of("/\\");
							if (foundLastSlash == testPath.length() - 1)
								testPath = testPath.substr(0, foundLastSlash);

							// test if this path exists
							// FIXME: always assume it exists for now
							//if (true)
							if (g_pFullFileSystem->IsDirectory(testPath.c_str()))//g_pFullFileSystem->FileExists(testPath.c_str()))
							{
								bAtLeastOneAppFilepathExists = true;

								// test if the file exists inside of this filepath
								std::string testSlash = (testPath.find("\\") != std::string::npos) ? "\\" : "/";
								std::string testOtherSlash = (testSlash == "\\") ? "/" : "\\";

								std::replace(testPath.begin(), testPath.end(), testOtherSlash[0], testSlash[0]);	// slashes should all be the same direction.

								// lets create a short file
								std::string testShortFileOnly = shortFileOnly;
								std::replace(testShortFileOnly.begin(), testShortFileOnly.end(), testOtherSlash[0], testSlash[0]);	// slashes should all be the same direction.

								// if the testPath is found at the base of the short file, simply subtract it
								size_t found = testShortFileOnly.find(testPath);
								if (found == 0)
									testShortFileOnly = testShortFileOnly.substr(found + testPath.length() + 1);
								else if (testShortFileOnly.find(":") != std::string::npos)
								{
									size_t foundLastSlash = testShortFileOnly.find_last_of("/\\");
									if (foundLastSlash != std::string::npos)
										testShortFileOnly = testShortFileOnly.substr(foundLastSlash + 1);
								}
								testFile = testPath + testSlash + testShortFileOnly;	// NOTE: This might be the full path if the full path has no ":" in it.

								// FIXME: always assume the file exists in this path for now.
								FileFindHandle_t findHandle;
								const char* pFilename = g_pFullFileSystem->FindFirstEx(testFile.c_str(), "", &findHandle);

								if (pFilename != NULL)
								{
									shortFileOnly = testShortFileOnly;

									if (slash == "")
									{
										slash = testSlash;
										otherSlash = testOtherSlash;
									}

									bShortFileResolved = true;
									composedFile = testFile;
									appFilepath = testPath;
									bAppFilepathGood = true;
									bItemFileGood = true;
									g_pFullFileSystem->FindClose(findHandle);
									break;
								}
								g_pFullFileSystem->FindClose(findHandle);
							}
						}

						if (!bShortFileResolved)
						{
							if (shortFileOnly.find(":") != std::string::npos)
							{
								size_t foundLastSlash = shortFileOnly.find_last_of("/\\");
								if (foundLastSlash != std::string::npos)
									shortFileOnly = shortFileOnly.substr(foundLastSlash + 1);
							}
						}

						// resolve the composedFile now
						if (!bAppFilepathGood)
							composedFile = file;

						if (slash != "")
						{
							std::replace(shortFileOnly.begin(), shortFileOnly.end(), otherSlash[0], slash[0]);	// slashes should all be the same direction.
							std::replace(composedFile.begin(), composedFile.end(), otherSlash[0], slash[0]);	// slashes should all be the same direction.
						}

						// generate the commands
						// try to apply a command format
						if (appCommands != "")
						{
							// if the app has a command syntax, replace item variables with their values.

							// replace $FILE with active->GetString("file")
							// replace $QUOTE with a double quote
							// replace $SHORTFILE with active->GetString("file")'s filename only
							// replace etc.

							size_t found;

							found = appCommands.find("$FILE");
							if (found != std::string::npos)
							{
								if (this->AlphabetSafe(composedFile) && this->DirectorySafe(composedFile))
								{
									while (found != std::string::npos)
									{
										appCommands.replace(found, 5, composedFile);
										found = appCommands.find("$FILE");
									}
								}
								else
									errorType = UNKNOWN_ERROR;
							}

							found = appCommands.find("$SHORTFILE");
							if (found != std::string::npos)
							{
								while (found != std::string::npos)
								{
									appCommands.replace(found, 10, shortFileOnly);
									found = appCommands.find("$SHORTFILE");
								}
							}

							found = appCommands.find("$QUOTE");
							while (found != std::string::npos)
							{
								appCommands.replace(found, 6, "\"");
								found = appCommands.find("$QUOTE");
							}
						}
						else
						{
							// otherwise, apply the default Windows command syntax for "open with"
							appCommands = "\"" + composedFile + "\"";
						}
					}
				}
			}

			if (!bAppExecutableGood)
				composedFile = file;

			if (!bAppFilepathGood)
			{
				// check if the file exists.
				// (file could also be an HTTP or STEAM protocol address at this point.)

				// are we a web address missing an http?
				if (composedFile.find("www.") == 0)
					composedFile = "http://" + composedFile;

				// is it a steam appid?
				if (!Q_strcmp(VarArgs("%llu", Q_atoui64(composedFile.c_str())), composedFile.c_str()))
				{
					std::string steamAppId = composedFile;

					std::string testerId = "id" + steamAppId;
					// check if this is a Source engine game real quick...
					for (KeyValues *sub = m_pSourceEngineGamesKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						if (!Q_strcmp(sub->GetString(), testerId.c_str()))
						{
							bSourceQuickFix = true;
							break;
						}
					}

					composedFile = "steam://run/" + steamAppId;
					bItemFileGood = true;
				}
				else if (composedFile.find("http") == 0)
					bItemFileGood = true;
				else if (g_pFullFileSystem->FileExists(composedFile.c_str()))
					bItemFileGood = true;
				//else if (true)	// check if local file exists // FIXME: assume it always exists for now
					//bItemFileGood = true;
			}
		}
	}

	// if the security scrubs of the item's variables referenced by the app's launch syntax failed, we ALREADY have an error.
	if (errorType != NONE)
		return errorType;

	// all variables are resolved, now do some logic.
	if (bItemGood)
	{
		// check for a good app first, because that determines if the item file can be resolved.
		if (bHasApp)
		{
			if (bAppGood)
			{
				if (bAppExecutableGood)
				{
					if (!bHasAppFilepath || bAtLeastOneAppFilepathExists)
					{
						if (bItemFileGood)
						{
							// executable
							executable = appExecutable;

							// executableDirectory
							std::string dir = appExecutable;
							size_t found = dir.find_last_of("/\\");
							if (found != std::string::npos)
								executableDirectory = dir.substr(0, found + 1);

							// masterCommands
							std::string shortAppFile = appExecutable;
							found = shortAppFile.find_last_of("/\\");
							if (found != std::string::npos)
								shortAppFile = shortAppFile.substr(found + 1);

							masterCommands = "\"" + shortAppFile + "\" " + appCommands;
							bReadyToActuallyLaunch = true;
						}
						else
						{
							DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
							errorType = ITEM_FILE_NOT_FOUND;
							//return;
						}
					}
					else
					{
						DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
						errorType = APP_PATH_NOT_FOUND;
						//return;
					}
				}
				else
				{
					DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
					errorType = APP_FILE_NOT_FOUND;
					//return;
				}
			}
			else
			{
				DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
				errorType = APP_NOT_FOUND;
				//return;
			}
		}
		else
		{
			if (bItemFileGood)
			{
				// doesn't use an app
				executable = composedFile;
				executableDirectory = "";
				masterCommands = "";
				bReadyToActuallyLaunch = true;
			}
			else
			{
				DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
				errorType = ITEM_FILE_NOT_FOUND;
				//return;
			}
		}
	}
	else
	{
		DevMsg("USER-RESOLVABLE-LAUNCH-ERROR: Show it, bra.\n");
		errorType = ITEM_NOT_FOUND;
		//return;
	}

	// all regular user-correctable errors would have been detected by now...
	if (errorType != NONE)
		return errorType;

	// perform security checks now that all variables are resolved
	// NOTE: masterCommands is generated from locally defined launch commands in the user's APP
	// NOTE: executableDirectory is generated locally from the user's APP.  It is the same as the front of the executable provided by the user's APP.
	// NOTE: executable potentially comes from the ITEM
	// NOTE: malicious items might try to break out of the trusted app command syntax, so strict formatting in AcradeCreateProcess is required.
	// NOTE: Untrusted item values being referenced by trusted app command syntax should be scrubbed through the alphabet & checked for breakout payloads.
	if (!this->ExecutableSafe(executable))
		errorType = UNKNOWN_ERROR;

	if (errorType == NONE && bReadyToActuallyLaunch)
	{
		this->SetLastLaunchedItemId(id);	// TODO: This should use the network dictionaries in case our local ID differs from the server's ID.  Would be best to use an OBJECT instead of an ITEM.

		if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() != g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud"))
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

		// deslect any entity
		if (g_pAnarchyManager->GetSelectedEntity())
			g_pAnarchyManager->DeselectEntity("none");

		// clear the embedded instance (to stop YT videos from playing, for example)
		/*
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		C_EmbeddedInstance* pOldEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		if (pOldEmbeddedInstance && pOldEmbeddedInstance != pHudBrowserInstance)
		{
		//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		//g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, null);
		//g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pHudBrowserInstance);
		pOldEmbeddedInstance->Close();
		}
		else
		*/

		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, null, true, false);
		//C_AwesomiumBrowserInstance* pHudBrowserInstance = 
		//g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud")->
		if (!bSourceQuickFix)
		{
			// pause AArcade
			//engine->ServerCmd("setpause");
			g_pAnarchyManager->DoPause();
			//g_pAnarchyManager->DoPause();
		}
		else
		{
			g_pAnarchyManager->ShowRawMainMenu();
			//m_bIsInSourceGame = true;
			//g_pAnarchyManager->DoPause(1);
		}
			//engine->ClientCmd("disconnect");
			//engine->ExecuteClientCmd();

		// Write this live URL out to the save file.
		if (cvar->FindVar("broadcast_mode")->GetBool())
		{
			// Write this live URL out to the save file.
			std::string XSPlitLiveFolder = cvar->FindVar("broadcast_folder")->GetString();
			if (XSPlitLiveFolder != "")
			{
				if (cvar->FindVar("broadcast_auto_game")->GetBool())
					this->WriteBroadcastGame(std::string(itemActive->GetString("title")));

				// Also update a JS file
				FileHandle_t hFile = g_pFullFileSystem->Open(VarArgs("%s\\vote.js", XSPlitLiveFolder.c_str()), "a+", "");
				if (hFile)
				{
					std::string code = "gAnarchyTV.OnAArcadeCommand(\"startPlaying\", \"";
					code += itemActive->GetString("info/id");
					code += "\");\n";
					g_pFullFileSystem->Write(code.c_str(), code.length(), hFile);
					g_pFullFileSystem->Close(hFile);
				}
			}
		}


		// we may have launched from something on continous play, so lets check for that case...
		std::vector<C_EmbeddedInstance*> embeddedInstances;
		m_pCanvasManager->GetAllInstances(embeddedInstances);
		unsigned int max = embeddedInstances.size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (embeddedInstances[i]->GetOriginalItemId() == id)
			{
				///*
				C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(embeddedInstances[i]);
				if (pAwesomiumBrowserInstance)
					m_pAwesomiumBrowserManager->DestroyAwesomiumBrowserInstance(pAwesomiumBrowserInstance);
				else
				{
					C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(embeddedInstances[i]);
					if (pSteamBrowserInstance)
						m_pSteamBrowserManager->DestroySteamBrowserInstance(pSteamBrowserInstance);
					else
					{
						C_LibretroInstance* pLibretroInstance = dynamic_cast<C_LibretroInstance*>(embeddedInstances[i]);
						if (pLibretroInstance)
							m_pLibretroManager->DestroyLibretroInstance(pLibretroInstance);
					}
				}
				//*/

				//embeddedInstances[i]->Close();
			}
		}

		m_pAccountant->Action("aa_shortcuts_launched", 1);

		// launch the item
		g_pAnarchyManager->ArcadeCreateProcess(executable, executableDirectory, masterCommands);
	}
	//else
	//	DevMsg("ERROR: Could not launch item.\n");

	return errorType;
}

void C_AnarchyManager::Acquire(std::string query, bool bQuietRun, bool bShouldPause, bool bAllowLocal)
{
	if ((!bAllowLocal && query.find("http") != 0) || (bAllowLocal && !g_pFullFileSystem->FileExists(query.c_str())) )
	{
		DevMsg("ERROR: %s is not valid.\n", query.c_str());
		return;
	}

	//DevMsg("DISPLAY MAIN MENU\n");
	if (!bQuietRun)
	{
		std::string uiFile = (bShouldPause) ? "pause" : "welcome";
		if (m_pSelectedEntity)
			this->DeselectEntity("asset://ui/" + uiFile + ".html");
		else
		{
			C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
			pHudBrowserInstance->SetUrl("asset://ui/" + uiFile + ".html");
		}

		m_pInputManager->ActivateInputMode(true, true);
	}

	FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");
	if (!launch_file)
	{
		Msg("Error creating ArcadeLauncher.bat!\n");
		return;
	}

	if (query.find("\"") == std::string::npos)
	{
		std::string goodQuery = "\"" + query + "\"";
		filesystem->FPrintf(launch_file, "START \"Launching web browser...\" %s", goodQuery.c_str());
		filesystem->Close(launch_file);
		system("Arcade_Launcher.bat");
	}
	else
		DevMsg("ERROR: Invalid URL detected.  Cannot have quotes in it.\n");
}

void C_AnarchyManager::BeginImportSteamGames(std::string tabName)
{
	if (!this->IsInitialized())
	{
		DevMsg("Not initialized.  Aborting Steam games import.\n");
		return;
	}

	// Scan user profile.
	// 1. Activate input mode.
	// 2. Navigate to the user's games list on their Steam profile in the in-game Steamworks browser.
	// 3. Notify & instruct the user if their profile is set to private, otherwise have an "IMPORT" button appear.
	// 4. Import all games from their list into a KeyValues file ownedGames.key
	// 5. Load all entries from ownedGames.key as items, but do not automatically save them out until the user modifies them.

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	CSteamID sid = steamapicontext->SteamUser()->GetSteamID();
	std::string profileUrl = "http://www.steamcommunity.com/profiles/" + std::string(VarArgs("%llu/games/?tab=%s", sid.ConvertToUint64(), tabName.c_str()));// all";	// would "my / games / ..." work instead of the literal ID?

	C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
	pSteamBrowserInstance->SetActiveScraper("importSteamGames", "", "");

	//this->HudStateNotify();	// So the UI knows we have a scraper PRIOR to loading a overlay.html (so overlay.html can perform auto-hide or auto-close)

	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->DeselectEntity("asset://ui/overlay.html");
	else
		pHudBrowserInstance->SetUrl("asset://ui/overlay.html");

	bool bOldGamepadInputMode = m_pInputManager->IsGamepadInputMode();
	this->GetInputManager()->DeactivateInputMode(true);

	std::string id = "scrape" + std::string(g_pAnarchyManager->GenerateUniqueId());

	pSteamBrowserInstance->Init(id, profileUrl, "Steam Game Importer", null);
	pSteamBrowserInstance->Focus();
	pSteamBrowserInstance->Select();
	g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pSteamBrowserInstance);
	//this->HudStateNotify();	// So the UI knows we have a scraper PRIOR to loading a overlay.html (so overlay.html can perform auto-hide or auto-close)
	if (bOldGamepadInputMode)
		m_pInputManager->SetGamepadInputMode(true);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pSteamBrowserInstance);
}

size_t ExecuteProcess(std::string FullPathToExe, std::string Parameters)
{
	size_t iMyCounter = 0, iReturnVal = 0, iPos = 0;
	DWORD dwExitCode = 0;
	std::string sTempStr = "";

	/* - NOTE - You should check here to see if the exe even exists */

	/* Add a space to the beginning of the Parameters */
	if (Parameters.size() != 0)
	{
		if (Parameters[0] != L' ')
		{
			Parameters.insert(0, " ");
		}
	}

	/* The first parameter needs to be the exe itself */
	sTempStr = FullPathToExe;
	iPos = sTempStr.find_last_of("/\\");
	sTempStr.erase(0, iPos + 1);
	Parameters = sTempStr.append(Parameters);

	/* CreateProcessW can modify Parameters thus we allocate needed memory */
	char* pwszParam = new char[Parameters.size() + 1];
	if (pwszParam == 0)
	{
		return 1;
	}
	const char* pchrTemp = Parameters.c_str();
	Q_strcpy(pwszParam, pchrTemp);
	//wcscpy_s(pwszParam, Parameters.size() + 1, pchrTemp);

	/* CreateProcess API initialization */
	//STARTUPINFOW siStartupInfo;
	STARTUPINFO siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);

	if (CreateProcess(FullPathToExe.c_str(),
		pwszParam, 0, 0, false,
		CREATE_DEFAULT_ERROR_MODE, 0, 0,
		&siStartupInfo, &piProcessInfo) != false)
	{
		/* Watch the process. */
		//dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, (SecondsToWait * 1000));
	}
	else
	{
		/* CreateProcess failed */
		iReturnVal = GetLastError();
	}


	DevMsg("Done\n");
	/* Free memory */
	delete[]pwszParam;
	pwszParam = 0;

	/* Release handles */
	CloseHandle(piProcessInfo.hProcess);
	CloseHandle(piProcessInfo.hThread);

	return iReturnVal;
}

void C_AnarchyManager::ArcadeCreateProcess(std::string executable, std::string executableDirectory, std::string masterCommands)
{

	//DevMsg("Launching Item: \n\tExecutable: %s\n\tDirectory: %s\n\tMaster Commands: %s\n", executable.c_str(), executableDirectory.c_str(), masterCommands.c_str());
	//size_t result = ExecuteProcess(executable, masterCommands);
	//DevMsg("Finished launching item.\n");

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	DevMsg("Launching Item: \n\tExecutable: %s\n\tDirectory: %s\n\tMaster Commands: %s\n", executable.c_str(), executableDirectory.c_str(), masterCommands.c_str());

	bool bIsDirectlyExecutable = false;

	/*
	bool bIsDirectlyExecutable = false;

	std::string executableExtensions = "::exe::";
	std::string fileName = executable;
	std::string fileExtension = "";
	size_t found = fileName.find_last_of("/\\");
	if ( found != std::string::npos )
	{
		fileName = fileName.substr(found + 1);

		found = fileName.find_last_of(".");
		if (found != std::string::npos && found < fileName.length() - 1)
		{
			fileExtension = fileName.substr(found + 1);
			fileExtension = "::" + fileExtension + "::";
			//fileName = fileName.substr(0, found);
			if (executableExtensions.find(fileExtension) != std::string::npos)
			{
				if (g_pFullFileSystem->FileExists(executable.c_str()))
				{
					fileName = fileName + " ";
					bIsDirectlyExecutable = true;
					Q_strcpy(pCommands, fileName.c_str());
					DevMsg("Using %s as the filename\n", fileName.c_str());
				}
			}
		}
	}

	if ( !bIsDirectlyExecutable )
		Q_strcpy(pCommands, masterCommands.c_str());
	*/

	if (!bIsDirectlyExecutable && executableDirectory == "" && masterCommands == "")
	{
		bool bUseKodi = cvar->FindVar("kodi")->GetBool();
		bool bIsKodiFileExtension = false;
		if (bUseKodi)
		{
			// Check for Kodi files
			std::vector<std::string> kodiFileExtensions;
			kodiFileExtensions.push_back(".avi");
			kodiFileExtensions.push_back(".mpg");
			kodiFileExtensions.push_back(".mp4");
			kodiFileExtensions.push_back(".mpeg");
			kodiFileExtensions.push_back(".vob");
			kodiFileExtensions.push_back(".mkv");

			bIsKodiFileExtension = false;
			unsigned int length = executable.length();
			unsigned int max = kodiFileExtensions.size();
			for (unsigned int i = 0; i < max; i++)
			{
				if (executable.find(kodiFileExtensions[i]) == length - kodiFileExtensions[i].length())
				{
					bIsKodiFileExtension = true;
					break;
				}
			}
		}

		if (bUseKodi && bIsKodiFileExtension)
		{
			DevMsg("Launch option A\n");
			std::string bufLocationString = executable;

			size_t found = bufLocationString.find("\\");
			while (found != std::string::npos)
			{
				bufLocationString[found] = '/';
				found = bufLocationString.find("\\");
			}

			std::string kodiIP = cvar->FindVar("kodi_ip")->GetString();
			std::string kodiPort = cvar->FindVar("kodi_port")->GetString();
			std::string kodiUser = cvar->FindVar("kodi_user")->GetString();
			std::string kodiPassword = cvar->FindVar("kodi_password")->GetString();

			if (kodiIP == "")
				DevMsg("ERROR: You need to go into options and set your Kodi IP first.\n");
			else if (kodiPort == "")
				DevMsg("ERROR: You need to go into options and set your Kodi PORT first.\n");
			else if (kodiUser == "")
				DevMsg("ERROR: You need to go into options and set your Kodi USER first.\n");
			else if (kodiPassword == "")
				DevMsg("ERROR: You need to go into options and set your Kodi PASSWORD first.\n");
			else
			{
				std::string kodiGetRequest = kodiUser + std::string(":") + kodiPassword + std::string("@") + kodiIP + std::string(":") + kodiPort;
				DevMsg("Thing is: %s\n", kodiGetRequest.c_str());
				C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
				std::vector<std::string> params;
				params.push_back(kodiGetRequest);
				params.push_back(executable);
				pHudBrowserInstance->DispatchJavaScriptMethod("kodiListener", "play", params);
			}
		}
		else
		{
			DevMsg("Launch option B: %s\n", executable.c_str());
			//g_pVGuiSystem->ShellExecuteA("open", executable.c_str());

			// NOW DO THE ACTUAL LAUNCHING STUFF
			// old-style bat launching
			FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");

			if (!launch_file)
			{
				Msg("Error creating ArcadeLauncher.bat!\n");
				return;
			}

			bool bCommandIsURL = false;

			bool DoNotPause = false;
			std::string goodExecutable = "\"" + executable + "\"";
			filesystem->FPrintf(launch_file, "%s:\n", goodExecutable.substr(1, 1).c_str());
			filesystem->FPrintf(launch_file, "cd \"%s\"\n", goodExecutable.substr(1, goodExecutable.find_last_of("/\\", goodExecutable.find("\"", 1)) - 1).c_str());
			filesystem->FPrintf(launch_file, "START \"Launching item...\" %s", goodExecutable.c_str());
			//filesystem->FPrintf(launch_file, "START \"Launching item...\" %s", masterCommands.c_str());
			filesystem->Close(launch_file);
			system("Arcade_Launcher.bat");
		}
	}
	else
	{
		DevMsg("Launch option C\n");

		//char pCommands[AA_MAX_STRING];
		int iAAMaxString = masterCommands.length() + 1;
		char* pCommands = new char[iAAMaxString];
		Q_strcpy(pCommands, masterCommands.c_str());

		// start the program up
		CreateProcess(executable.c_str(),   // the path
			pCommands,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,//CREATE_DEFAULT_ERROR_MODE,              //0 // No creation flags
			NULL,           // Use parent's environment block
			executableDirectory.c_str(),           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi);           // Pointer to PROCESS_INFORMATION structure

		delete[] pCommands;
	}
	DevMsg("Finished launching item.\n");
}

void C_AnarchyManager::RunAArcade()
{
	if (!m_hwnd)
	{
		//engine->ClientCmd("anarchymanager");
		//if (engine->IsInGame())
		//	this->GetInputManager()->DeactivateInputMode(true);
		DevMsg("Cannot initialize AArcade after a map is already loaded.\n");
		return;
	}

	cvar->FindVar("throttle_embedded_render")->SetValue(0);	// force threaded rendering to be off.

	ConVar* pCvar = cvar->FindVar("avatar_url");
	if (!Q_strcmp(pCvar->GetString(), ""))
	{
		pCvar->SetValue("http://aarcade.tv/anarchybot/avatars/default0.jpg");	// to prevent us doing this in an infinite loop due to failure.
		m_pMetaverseManager->FetchUserInfo();
		return;
	}

	if (!this->IsInitialized())
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl("asset://ui/startup.html");
	}
	else
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl("asset://ui/welcome.html");

		if (vgui::input()->IsKeyDown(KEY_XBUTTON_START))
			g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);

		this->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);

		m_bSuspendEmbedded = false;
	}
}

void C_AnarchyManager::ShowConnect(std::string lobbyId)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl(VarArgs("asset://ui/connect.html?lobby=%s", lobbyId.c_str()));
	this->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);
	m_bSuspendEmbedded = false;
}

void C_AnarchyManager::HudStateNotify()
{
	//DevMsg("HudStateNotify\n");
	std::vector<std::string> params;

	// isFullscreen
	params.push_back(VarArgs("%i", (g_pAnarchyManager->GetInputManager()->GetFullscreenMode())));

	// isHudPinned
	params.push_back(VarArgs("%i", (g_pAnarchyManager->GetInputManager()->GetWasForceInputMode())));

	// isMapLoaded
	params.push_back(VarArgs("%i", engine->IsInGame()));

	// isObjectSelected (any object)
	params.push_back(VarArgs("%i", (g_pAnarchyManager->GetSelectedEntity() != null)));

	// isItemSelected (any item)
	std::string itemId;
	int entIndex = -1;
	int isItemSelected = 0;
	C_PropShortcutEntity* pShortcut = null;
	C_BaseEntity* pEntity = g_pAnarchyManager->GetSelectedEntity();
	if (pEntity)
	{
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
		if (pShortcut )// && pShortcut->GetItemId() != "")
		{
			itemId = pShortcut->GetItemId();

			if (itemId != "")
				isItemSelected = 1;
		}
	}
	params.push_back(VarArgs("%i", isItemSelected));

	// isMainMenu
	params.push_back(VarArgs("%i", (g_pAnarchyManager->GetInputManager()->GetMainMenuMode())));

	// url
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
	if (pEmbeddedInstance && pEmbeddedInstance != pHudBrowserInstance)
	{
		params.push_back(pEmbeddedInstance->GetURL());

		if (pShortcut && pEmbeddedInstance->GetOriginalEntIndex() == pShortcut->entindex())
			entIndex = pShortcut->entindex();
	}
	else
		params.push_back("");

	// isSelectedObject
	if (pShortcut && pEmbeddedInstance && pEmbeddedInstance->GetOriginalEntIndex() == pShortcut->entindex())
		params.push_back("1");
	else
		params.push_back("0");

	float fPositionX = 0;
	float fPositionY = 0;
	float fSizeX = 1;
	float fSizeY = 1;
	//std::string file = "";
	std::string overlayId = "";

	// embeddedInstanceType
	bool bCanGoForward = false;
	bool bCanGoBack = false;
	std::string activeScraperId;
	std::string libretroCore = "";
	std::string libretroFile = "";
	std::string embeddedType = "Unknown";

	C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(pEmbeddedInstance);
	if (pSteamBrowserInstance)
		activeScraperId = pSteamBrowserInstance->GetScraperId();

	if (pEmbeddedInstance)
	{
		pEmbeddedInstance->GetFullscreenInfo(fPositionX, fPositionY, fSizeX, fSizeY, overlayId);

		C_LibretroInstance* pLibretroInstance = dynamic_cast<C_LibretroInstance*>(pEmbeddedInstance);
		if (pLibretroInstance)
		{
			embeddedType = "Libretro";
			libretroCore = pLibretroInstance->GetLibretroCore();
			libretroFile = pLibretroInstance->GetLibretroFile();
		}

		//C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(pEmbeddedInstance);
		if (pSteamBrowserInstance)
		{
			embeddedType = "SteamworksBrowser";
			bCanGoForward = pSteamBrowserInstance->GetCanGoForward();
			bCanGoBack = pSteamBrowserInstance->GetCanGoBack();
		}

		C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pEmbeddedInstance);
		if (pAwesomiumBrowserInstance)
			embeddedType = "AwesomiumBrowser";
	}

	params.push_back(embeddedType);

	KeyValues* pItemKV = null;
	if (itemId != "")
		pItemKV = m_pMetaverseManager->GetLibraryItem(itemId);

	int iCanLibretroRun = 0;
	int iCanStream = 0;
	int iCanPreview = 0;
	if (pItemKV)
	{
		std::string gameFile = "";
		std::string coreFile = "";
		iCanLibretroRun = (this->DetermineLibretroCompatible(pItemKV, gameFile, coreFile)) ? 1 : 0;
		iCanStream = (this->DetermineStreamCompatible(pItemKV)) ? 1 : 0;
		iCanPreview = (this->DeterminePreviewCompatible(pItemKV)) ? 1 : 0;
	}

	// canGoForward
	params.push_back(VarArgs("%i", iCanStream));
	params.push_back(VarArgs("%i", iCanPreview));
	params.push_back((bCanGoForward) ? "1" : "0");
	params.push_back((bCanGoBack) ? "1" : "0");
	params.push_back(libretroCore);
	params.push_back(libretroFile);
	params.push_back(VarArgs("%i", iCanLibretroRun));	// just ASSUME that Libretro can open stuff always, for now.
	params.push_back(VarArgs("%f", fPositionX));
	params.push_back(VarArgs("%f", fPositionY));
	params.push_back(VarArgs("%f", fSizeX));
	params.push_back(VarArgs("%f", fSizeY));
	params.push_back(overlayId);
	params.push_back(activeScraperId);

	bool bIsConnectedToUniverse = (m_pConnectedUniverse) ? m_pConnectedUniverse->connected : false;
	params.push_back(VarArgs("%i", bIsConnectedToUniverse));

	bool bIsHost = (bIsConnectedToUniverse) ? m_pConnectedUniverse->isHost : true;
	params.push_back(VarArgs("%i", bIsHost));

	bool isGamepad = (g_pAnarchyManager->GetInputManager()->IsGamepadInputMode() || (g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->IsHandTrackingActive())) ? true : false;
	params.push_back(VarArgs("%i", isGamepad));
	
	pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onActivateInputMode", params);
}

void C_AnarchyManager::ShowMouseMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

	if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER) || vgui::input()->IsKeyDown(KEY_XBUTTON_START))
		g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
	pHudBrowserInstance->SetUrl("asset://ui/mouseEZ.html");
	m_pInputManager->ActivateInputMode(true, false);
}

void C_AnarchyManager::HideMouseMenu()
{
	m_pInputManager->DeactivateInputMode(true);
}

void C_AnarchyManager::ShowTaskMenu(bool bForceTaskTab)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();
	//g_pAnarchyManager->DeselectEntity();

	//if (!enginevgui->IsGameUIVisible())
	//{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		//pHudBrowserInstance->SetUrl(VarArgs("asset://ui/%s", this->GetTabMenuFile().c_str()));//taskMenu.html");
		if (bForceTaskTab)
			pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=tasks");
		else
			pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html");//?tab=tasks
		m_pInputManager->ActivateInputMode(true, false);	// changed this to false because pausing the game due to a menu is obsolete!
	//}
}

void C_AnarchyManager::TeleportToScreenshot(std::string id, bool bDeactivateInputMode)
{
	KeyValues* pScreenshotKV = g_pAnarchyManager->GetMetaverseManager()->GetScreenshot(id);
	if (pScreenshotKV)
	{
		if (bDeactivateInputMode)
			this->GetInputManager()->DeactivateInputMode(true);

		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		//QAngle angles;
		//UTIL_StringToVector(angles.Base(), pScreenshotKV->GetString("camera/rotation", "0 0 0"));
		//std::string angles = pScreenshotKV->GetString("camera/rotation", "0 0 0");
		//if (angles != "0 0 0")
		//	engine->ExecuteClientCmd(VarArgs("setang %s;", angles.c_str()));
		engine->ClientCmd(VarArgs("teleport_player %i %s %s %s\n", pPlayer->entindex(), pScreenshotKV->GetString("body/position", "0 0 0"), pScreenshotKV->GetString("body/rotation", "0 0 0"), pScreenshotKV->GetString("camera/rotation", "0 0 0")));// servercmdfix , true);
	}
}

bool C_AnarchyManager::TakeScreenshot(bool bCreateBig, std::string id, bool bCreateInteractiveViewer)
{
	//bCreateBig = true;

	//int thumbWidth = 512;
	//int thumbHeight = 512;
	int thumbWidth = 240;
	int thumbHeight = 135;
	//int bigWidth = 1920;
	//int bigHeight = 1080;

	std::string mapShotsFolder = "screenshots";
	g_pFullFileSystem->CreateDirHierarchy(mapShotsFolder.c_str(), "DEFAULT_WRITE_PATH");

	std::string mapName = g_pAnarchyManager->MapName();
	if (mapName != "")
	{
		std::string panoFolder;
		std::string goodId = id;
		std::string mapShotFile;
		std::string mapShotBigFile;
		std::string mapShotInfoFile;
		std::string mapShotJSFile;
		std::string mapShotHTMLFile;
		std::string mapShotIndexHTMLFile;
		std::string mapShotIndexJPGFile;
		std::string mapShotIndexJSFile;
		std::string mapShotReadMeFile;
		std::string relativeMapShotFolder;
		std::string relativeMapShotFile;
		std::string relativeMapShotJSFile;
		std::string relativeMapShotHTMLFile;
		std::string relativeMapShotBigFile;
		std::string relativeMapShotInfoFile;
		std::string relativeMapShotIndexHTMLFile;
		std::string relativeMapShotIndexJPGFile;
		std::string relativeMapShotIndexJSFile;
		std::string relativeMapShotReadMeFile;

		std::string panoId;
		bool bIsPanoShot = false;
		if (goodId == "")
		{
			// find a filename for this new screenshot
			unsigned int screenshotNumber = 0;

			goodId = mapName + std::string(VarArgs("%i", screenshotNumber));

			mapShotFile = goodId + ".tga";
			mapShotBigFile = goodId + ".jpg";
			mapShotInfoFile = goodId + ".txt";
			mapShotJSFile = goodId + ".js";
			mapShotHTMLFile = goodId + ".html";
			mapShotIndexHTMLFile = "index.html";
			mapShotIndexJPGFile = "index.jpg";
			mapShotIndexJSFile = "index.js";
			mapShotReadMeFile = "readme.txt";

			relativeMapShotFolder = mapShotsFolder + "\\" + goodId;
			relativeMapShotFile = mapShotsFolder + "\\" + mapShotFile;
			relativeMapShotBigFile = mapShotsFolder + "\\" + mapShotBigFile;
			relativeMapShotInfoFile = mapShotsFolder + "\\" + mapShotInfoFile;
			relativeMapShotJSFile = mapShotsFolder + "\\" + mapShotJSFile;
			relativeMapShotHTMLFile = mapShotsFolder + "\\" + mapShotHTMLFile;
			//relativeMapShotIndexHTMLFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexHTMLFile;
			//relativeMapShotIndexJPGFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexJPGFile;
			//relativeMapShotIndexJSFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexJSFile;
			//relativeMapShotReadMeFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotReadMeFile;

			while (g_pFullFileSystem->IsDirectory(relativeMapShotFolder.c_str(), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(relativeMapShotFile.c_str(), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(relativeMapShotBigFile.c_str(), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(relativeMapShotInfoFile.c_str(), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(relativeMapShotJSFile.c_str(), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(relativeMapShotHTMLFile.c_str(), "DEFAULT_WRITE_PATH") ||
				g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotFile.c_str()), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotBigFile.c_str()), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotInfoFile.c_str()), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotJSFile.c_str()), "DEFAULT_WRITE_PATH") || g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotHTMLFile.c_str()), "DEFAULT_WRITE_PATH")
				)
			{
				screenshotNumber++;

				goodId = mapName + std::string(VarArgs("%i", screenshotNumber));

				mapShotFile = goodId + ".tga";
				relativeMapShotFile = mapShotsFolder + "\\" + mapShotFile;

				mapShotBigFile = goodId + ".jpg";
				relativeMapShotBigFile = mapShotsFolder + "\\" + mapShotBigFile;

				mapShotInfoFile = goodId + ".txt";
				relativeMapShotInfoFile = mapShotsFolder + "\\" + mapShotInfoFile;

				mapShotJSFile = goodId + ".js";
				relativeMapShotJSFile = mapShotsFolder + "\\" + mapShotJSFile;

				mapShotHTMLFile = goodId + ".html";
				relativeMapShotHTMLFile = mapShotsFolder + "\\" + mapShotHTMLFile;

				relativeMapShotFolder = mapShotsFolder + "\\" + goodId;
			}
		}
		else
		{
			size_t foundIdStuff = goodId.find_last_of("\\");
			if (foundIdStuff != std::string::npos)
			{
				bIsPanoShot = true;
				panoId = goodId.substr(foundIdStuff + 1);
				panoFolder = goodId.substr(0, foundIdStuff);

				g_pFullFileSystem->CreateDirHierarchy(VarArgs("screenshots\\%s", panoFolder.c_str()));
			}

			mapShotFile = goodId + ".tga";
			mapShotBigFile = goodId + ".jpg";
			mapShotInfoFile = goodId + ".txt";
			mapShotJSFile = goodId + ".js";
			mapShotHTMLFile = goodId + ".html";
			mapShotIndexHTMLFile = "index.html";
			mapShotIndexJPGFile = "index.jpg";
			mapShotIndexJSFile = "index.js";
			mapShotReadMeFile = "readme.txt";

			relativeMapShotFolder = mapShotsFolder + "\\" + goodId;
			relativeMapShotFile = mapShotsFolder + "\\" + mapShotFile;
			relativeMapShotBigFile = mapShotsFolder + "\\" + mapShotBigFile;
			relativeMapShotInfoFile = mapShotsFolder + "\\" + mapShotInfoFile;
			relativeMapShotJSFile = mapShotsFolder + "\\" + mapShotJSFile;
			relativeMapShotHTMLFile = mapShotsFolder + "\\" + mapShotHTMLFile;
			relativeMapShotIndexHTMLFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexHTMLFile;
			relativeMapShotIndexJPGFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexJPGFile;
			relativeMapShotIndexJSFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotIndexJSFile;
			relativeMapShotReadMeFile = mapShotsFolder + "\\" + panoFolder + "\\" + mapShotReadMeFile;

			// if files already exist for this screenshot id, remove them
			if (g_pFullFileSystem->FileExists(relativeMapShotFile.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(relativeMapShotFile.c_str(), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(relativeMapShotBigFile.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(relativeMapShotBigFile.c_str(), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(relativeMapShotInfoFile.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(relativeMapShotInfoFile.c_str(), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(relativeMapShotJSFile.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(relativeMapShotJSFile.c_str(), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(relativeMapShotHTMLFile.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(relativeMapShotHTMLFile.c_str(), "DEFAULT_WRITE_PATH");

			// support for when the shots folder was used instead of screenshots
			if (g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotFile.c_str()), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(VarArgs("shots\\%s", mapShotFile.c_str()), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotBigFile.c_str()), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(VarArgs("shots\\%s", mapShotBigFile.c_str()), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotInfoFile.c_str()), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(VarArgs("shots\\%s", mapShotInfoFile.c_str()), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotJSFile.c_str()), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(VarArgs("shots\\%s", mapShotJSFile.c_str()), "DEFAULT_WRITE_PATH");

			if (g_pFullFileSystem->FileExists(VarArgs("shots\\%s", mapShotHTMLFile.c_str()), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(VarArgs("shots\\%s", mapShotHTMLFile.c_str()), "DEFAULT_WRITE_PATH");

			// is there a subfolder for this screenshot?
			/*
			std::string foldername = "screenshots\\" + panoFolder;
			if (g_pFullFileSystem->IsDirectory(foldername.c_str(), "DEFAULT_WRITE_PATH"))
			{
				std::string filename = "screenshots\\" + panoFolder + "\\index.html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				filename = "screenshots\\" + panoFolder + "\\readme.txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// front
				std::string side = "front";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// back
				side = "back";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// left
				side = "left";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// right
				side = "right";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// bottom
				side = "bottom";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// top
				side = "top";
				filename = "screenshots\\" + panoFolder + "\\" + side + ".jpg";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".html";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\" + side + ".txt";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				// pano folder
				filename = "screenshots\\" + panoFolder + "\\pano\\aalogo.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\close.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\eye.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\launchicon.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\OBJLoader.js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\roller.css";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\sceneManager.js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\skipbackicon.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\skipnexticon.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\stopicon.png";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
				filename = "screenshots\\" + panoFolder + "\\pano\\three.min.js";
				if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
					g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

				std::string fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
				fullFile += "\\screenshots\\" + panoFolder + "\\pano";
				if (!RemoveDirectory(fullFile.c_str()))
					Msg("WARNING: COULD NOT REMOVE FOLDER %s\n", fullFile.c_str());

				fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
				fullFile += "\\screenshots\\" + panoFolder;
				if (!RemoveDirectory(fullFile.c_str()))
					Msg("WARNING: COULD NOT REMOVE FOLDER %s\n", fullFile.c_str());
			}
					*/
		}

		// create the info file
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

		bool bNeedsAdd = false;
		KeyValues* pInfoKV = (bIsPanoShot) ? null : this->GetMetaverseManager()->GetScreenshot(goodId);
		if (!pInfoKV)
		{
			pInfoKV = new KeyValues("screenshot");
			bNeedsAdd = !bIsPanoShot;
		}

		pInfoKV->SetInt("hasBigFile", 1);
		pInfoKV->SetInt("hasJSFile", 1);
		pInfoKV->SetInt("hasThumbFile", 1);;

		uint64 timeNumber = g_pAnarchyManager->GetTimeNumber();
		pInfoKV->SetString("created", VarArgs("%llu", timeNumber));
		pInfoKV->SetString("id", ((bIsPanoShot) ? panoId.c_str() : goodId.c_str()));

		// "id" prefixed fields: instance/id, instance/mapId, instance/workshopIds, instance/mountIds, map/id, map/workshopIds, map/mountIds

		// FIXME: This helper function should be generalized because it is also used in awesomiumjshandlers.cpp in the "getWorldInfo" handler
		instance_t* instance = g_pAnarchyManager->GetInstanceManager()->GetInstance(g_pAnarchyManager->GetInstanceId());
		pInfoKV->SetString("instance/id", VarArgs("id%s", instance->id.c_str()));
		pInfoKV->SetString("instance/mapId", VarArgs("id%s", instance->mapId.c_str()));
		pInfoKV->SetString("instance/title", instance->title.c_str());
		pInfoKV->SetString("instance/file", instance->file.c_str());

		if (instance->workshopIds != "")
			pInfoKV->SetString("instance/workshopIds", VarArgs("id%s", instance->workshopIds.c_str()));

		if (instance->workshopIds != "")
			pInfoKV->SetString("instance/mountIds", VarArgs("id%s", instance->mountIds.c_str()));

		// Now fill out the universe info, if needed.
		aampConnection_t* pUniverse = this->GetConnectedUniverse();
		if (pUniverse && pUniverse->connected)
		{
			/*
				bool connected;
				bool isHost;
				std::string address;
				std::string universe;
				std::string instance;
				std::string user;
				std::string session;
				std::string lobby;
				std::string lobbyPassword;
				bool isPublic;
				bool isPersistent;
			*/

			KeyValues* pUniverseKV = pInfoKV->FindKey("multiplayer", true);
			pUniverseKV->SetString("address", pUniverse->address.c_str());
			pUniverseKV->SetString("universe", VarArgs("id%s", pUniverse->universe.c_str()));
			//pUniverseKV->SetString("instance", pUniverse->user.c_str());
			//pUniverseKV->SetString("user", pUniverse->user.c_str());	// is this the local user ID or the owner ID??
			pUniverseKV->SetString("lobby", pUniverse->lobby.c_str());
			pUniverseKV->SetBool("hasPassword", (pUniverse->lobbyPassword != ""));
			pUniverseKV->SetBool("isPublic", pUniverse->isPublic);
			pUniverseKV->SetBool("isPersistent", pUniverse->isPersistent);
			pUniverseKV->SetBool("isHost", pUniverse->isHost);



			/*
			std::string userId;
			std::string sessionId;

			std::string followingId;	// todo: implement this on the Firebase-side stuff.
			// todo: implement some generic fields that can be primarily used for Twitch channel ID, Twitch user Id, Steam ID, maybe Twitter ID, etc.
			//std::string instanceId;
			std::string displayName;
			std::string itemId;
			std::string objectId;
			std::string say;
			Vector bodyOrigin;//std::string bodyOrigin;
			QAngle bodyAngles;//std::string bodyAngles;
			Vector headOrigin;//std::string headOrigin;
			QAngle headAngles;//std::string headAngles;
			std::string mouseX;
			std::string mouseY;
			std::string webUrl;
			std::string avatarUrl;
			std::string state;
			std::string launched;
			std::string twitchChannel;
			std::string twitchLive;
			C_DynamicProp* entity;
			bool needsEntity;
			bool isWebUser;
			*/

			user_t* pLocalUser = m_pMetaverseManager->GetLocalUser();
			if (pLocalUser)
			{
				KeyValues* pPhotographer = pInfoKV->FindKey("photographer", true);
				pPhotographer->SetString("id", VarArgs("id%s", pLocalUser->userId.c_str()));
				pPhotographer->SetString("displayName", pLocalUser->displayName.c_str());
				pPhotographer->SetString("avatarUrl", pLocalUser->avatarUrl.c_str());
			}
		}
		

		KeyValues* pMapKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetMap(instance->mapId));
		KeyValues* dummyKV = pInfoKV->FindKey("map", true);
		this->AddSubKeysToKeys(pMapKV, dummyKV);
		pInfoKV->SetString("map/created", "");
		pInfoKV->SetString("map/owner", "");
		pInfoKV->SetString("map/removed", "");

		std::string idBuf = pInfoKV->GetString("map/id");
		if (idBuf != "")
			pInfoKV->SetString("map/id", VarArgs("id%s", idBuf.c_str()));

		idBuf = pInfoKV->GetString("map/workshopIds");
		if (idBuf != "")
			pInfoKV->SetString("map/workshopIds", VarArgs("id%s", idBuf.c_str()));

		idBuf = pInfoKV->GetString("map/mountIds");
		if (idBuf != "")
			pInfoKV->SetString("map/mountIds", VarArgs("id%s", idBuf.c_str()));

		Vector origin = C_BasePlayer::GetLocalPlayer()->EyePosition();
		QAngle angles = C_BasePlayer::GetLocalPlayer()->EyeAngles();

		char buf[AA_MAX_STRING];
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
		pInfoKV->SetString("camera/position", buf);

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
		pInfoKV->SetString("camera/rotation", buf);

		Vector bodyOrigin = pPlayer->GetAbsOrigin();
		QAngle bodyAngles = pPlayer->GetAbsAngles();

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", bodyOrigin.x, bodyOrigin.y, bodyOrigin.z);
		pInfoKV->SetString("body/position", buf);

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", bodyAngles.x, bodyAngles.y, bodyAngles.z);
		pInfoKV->SetString("body/rotation", buf);

		if (bIsPanoShot || pInfoKV->SaveToFile(g_pFullFileSystem, relativeMapShotInfoFile.c_str(), "DEFAULT_WRITE_PATH"))	// IMPORTANT: If this is a pano shot, nothing is saved on this line.
		{
			if ( !bIsPanoShot)
				clientdll->WriteSaveGameScreenshotOfSize(relativeMapShotFile.c_str(), thumbWidth, thumbHeight, false, false);

			if (bCreateBig)
			{
				/*
				if (panoId == "top")
				{
					DevMsg("HERE IS WHERE THE PANO VIEWER HTML FILE MUST BE COPIED OVER!\n");
				}
				else
				*/
				if (bCreateInteractiveViewer && !bIsPanoShot)
				{
					// copy frontend/resource/ui/html/snapviewer.html to aarcade_user/screenshots/[SCREENSHOT_ID].html
					// make a uniquely named config_redux.cfg to make sure we exec the right one

					FileHandle_t fh = filesystem->Open("resource/ui/html/snapviewer.html", "r", "MOD");
					if (fh)
					{
						int file_len = filesystem->Size(fh);
						char* GameInfo = new char[file_len + 1];

						filesystem->Read((void*)GameInfo, file_len, fh);
						GameInfo[file_len] = 0; // null terminator

						filesystem->Close(fh);

						std::string reallyGoodId = goodId;
						std::replace(reallyGoodId.begin(), reallyGoodId.end(), '\\', '/'); // replace all '/' to '\'

						// Use GameInfo here...
						std::string text = GameInfo;
						delete[] GameInfo;

						size_t found = text.find("<meta property=\"og:image\" content=\"\"/>");
						while (found != std::string::npos)
						{
							text.replace(found, 38, "<meta property=\"og:image\" content=\"" + std::string(cvar->FindVar("interactive_directory_url")->GetString()) + reallyGoodId + std::string(".jpg") + "\"/>");
							found = text.find("<meta property=\"og:image\" content=\"\"/>", found + 38);
						}

						found = text.find("<meta property=\"og:url\" content=\"\"/>");
						while (found != std::string::npos)
						{
							text.replace(found, 36, "<meta property=\"og:url\" content=\"" + std::string(cvar->FindVar("interactive_directory_url")->GetString()) + reallyGoodId + ".html\"/>");
							found = text.find("<meta property=\"og:url\" content=\"\"/>", found + 36);
						}

						std::string personaName = steamapicontext->SteamFriends()->GetPersonaName();
						if (personaName != "")
						{
							found = text.find("<title>Interactive Screenshot - Anarchy Arcade</title>");
							while (found != std::string::npos)
							{
								text.replace(found, 54, "<title>" + personaName + "'s Interactive Screenshot</title>");
								found = text.find("<title>Interactive Screenshot - Anarchy Arcade</title>", found + 54);
							}

							found = text.find("<meta property=\"og:title\" content=\"Interactive Screenshot\"/>");
							while (found != std::string::npos)
							{
								text.replace(found, 60, std::string("<meta property=\"og:title\" content=\"") + personaName + "'s Interactive Screenshot\"/>");
								found = text.find("<meta property=\"og:title\" content=\"Interactive Screenshot\"/>", found + 60);
							}
						}

						// TEMP FIX: end at the 1st </html>
						found = text.find("</html>");
						text = text.substr(0, found + 7);

						FileHandle_t fh2 = filesystem->Open(relativeMapShotHTMLFile.c_str(), "w", "MOD");
						if (fh2)
						{
							filesystem->Write(text.c_str(), text.length(), fh2);
							filesystem->Close(fh2);
						}
					}
				}
				
				if (bIsPanoShot && panoId == "top")
				{
					// make a copy of the primary screenshot into the subfolder
					std::string filename = mapShotsFolder + "\\" + panoFolder + ".jpg";
					CUtlBuffer buf;
					if (filesystem->ReadFile(filename.c_str(), "DEFAULT_WRITE_PATH", buf))
						filesystem->WriteFile(relativeMapShotIndexJPGFile.c_str(), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					filename = mapShotsFolder + "\\" + panoFolder + ".js";
					if (filesystem->ReadFile(filename.c_str(), "DEFAULT_WRITE_PATH", buf))
						filesystem->WriteFile(relativeMapShotIndexJSFile.c_str(), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					FileHandle_t fh = filesystem->Open("resource/ui/html/panoviewer.html", "r", "MOD");
					if (fh)
					{
						int file_len = filesystem->Size(fh);
						char* GameInfo = new char[file_len + 1];

						filesystem->Read((void*)GameInfo, file_len, fh);
						GameInfo[file_len] = 0; // null terminator

						filesystem->Close(fh);

						//std::string reallyGoodId = goodId;
						//std::replace(reallyGoodId.begin(), reallyGoodId.end(), '\\', '/'); // replace all '/' to '\'

						// Use GameInfo here...
						std::string text = GameInfo;
						delete[] GameInfo;

						size_t found = text.find("<meta property=\"og:image\" content=\"\"/>");
						while (found != std::string::npos)
						{
							text.replace(found, 38, "<meta property=\"og:image\" content=\"" + std::string(cvar->FindVar("interactive_directory_url")->GetString()) + panoFolder + std::string("/index.jpg") + "\"/>");
							found = text.find("<meta property=\"og:image\" content=\"\"/>", found + 38);
						}

						found = text.find("<meta property=\"og:url\" content=\"\"/>");
						while (found != std::string::npos)
						{
							text.replace(found, 36, "<meta property=\"og:url\" content=\"" + std::string(cvar->FindVar("interactive_directory_url")->GetString()) + panoFolder + "/\"/>");
							found = text.find("<meta property=\"og:url\" content=\"\"/>", found + 36);
						}

						std::string personaName = steamapicontext->SteamFriends()->GetPersonaName();
						if (personaName != "")
						{
							found = text.find("<title>Interactive 360 Screenshot - Anarchy Arcade</title>");
							while (found != std::string::npos)
							{
								text.replace(found, 58, "<title>" + personaName + "'s Interactive 360 Screenshot</title>");
								found = text.find("<title>Interactive 360 Screenshot - Anarchy Arcade</title>", found + 58);
							}

							found = text.find("<meta property=\"og:title\" content=\"Interactive 360 Screenshot\"/>");
							while (found != std::string::npos)
							{
								text.replace(found, 64, std::string("<meta property=\"og:title\" content=\"") + personaName + "'s Interactive 360 Screenshot\"/>");
								found = text.find("<meta property=\"og:title\" content=\"Interactive 360 Screenshot\"/>", found + 64);
							}
						}

						// TEMP FIX: end at the 1st </html>
						found = text.find("</html>");
						if (found != std::string::npos)
							text = text.substr(0, found + 7);

						FileHandle_t fh2 = filesystem->Open(relativeMapShotIndexHTMLFile.c_str(), "w", "DEFAULT_WRITE_PATH");
						if (fh2)
						{
							filesystem->Write(text.c_str(), text.length(), fh2);
							filesystem->Close(fh2);
						}
					}

					if (filesystem->ReadFile("resource/ui/html/panoviewerreadme.txt", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\readme.txt", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					g_pFullFileSystem->CreateDirHierarchy(VarArgs("screenshots\\%s\\pano", panoFolder.c_str()), "DEFAULT_WRITE_PATH");

					if (filesystem->ReadFile("resource/ui/html/pano/aalogo.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\aalogo.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/close.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\close.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/eye.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\eye.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/launchicon.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\launchicon.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/OBJLoader.js", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\OBJLoader.js", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/roller.css", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\roller.css", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/sceneManager.js", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\sceneManager.js", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/skipbackicon.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\skipbackicon.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/skipnexticon.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\skipnexticon.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/stopicon.png", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\stopicon.png", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();

					if (filesystem->ReadFile("resource/ui/html/pano/three.min.js", "MOD", buf))
						filesystem->WriteFile(VarArgs("screenshots\\%s\\pano\\three.min.js", panoFolder.c_str()), "DEFAULT_WRITE_PATH", buf);
					buf.Purge();
				}

				//clientdll->WriteSaveGameScreenshotOfSize(relativeMapShotBigFile.c_str(), bigWidth, bigHeight, false, false);
				std::string quality = "90";
				std::string goodMapShotBigFile = relativeMapShotBigFile.substr(0, relativeMapShotBigFile.size() - 4);
				goodMapShotBigFile = goodMapShotBigFile.substr(12);
				//std::transform(goodMapShotBigFile.begin(), goodMapShotBigFile.end(), goodMapShotBigFile.begin(), ::tolower);
				std::replace(goodMapShotBigFile.begin(), goodMapShotBigFile.end(), '\\', '/');

				KeyValues* pNextBigScreenshotKV = new KeyValues("screenshot");
				pNextBigScreenshotKV->SetString("file", goodMapShotBigFile.c_str());
				pNextBigScreenshotKV->SetString("quality", quality.c_str());
				pNextBigScreenshotKV->SetBool("isInteractive", (bCreateInteractiveViewer && !bIsPanoShot));
				pNextBigScreenshotKV->SetBool("isPanoShot", bIsPanoShot);
				m_pNextBigScreenshotKV = pNextBigScreenshotKV;

				//std::string cmd = VarArgs("jpeg \"%s\" \"%s\";\n", goodMapShotBigFile.c_str(), quality.c_str());
				//DevMsg("CMD: %s\n", cmd.c_str());
				//engine->ClientCmd(VarArgs("jpeg \"%s\" \"%s\"\n", goodMapShotBigFile.c_str(), quality.c_str()));
			}

			if (bNeedsAdd)
				this->GetMetaverseManager()->AddScreenshot(pInfoKV);

			//Msg("Shot saved.\n");
			m_pAccountant->Action("aa_screenshots_taken", 1);
			return true;
		}
		//pInfoKV->deleteThis();	// don't delete us cuz we are already auto-loaded by the metaverse manager now!
	}

	return false;
}

void C_AnarchyManager::HideScreenshotMenu()
{
	m_pInputManager->DeactivateInputMode(true);
}

void C_AnarchyManager::ManualPause()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->DeselectEntity();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/pause.html");
	m_pInputManager->ActivateInputMode(true, true);	// except this one.  it pauses it.
}

void C_AnarchyManager::ShowVehicleMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/options.html?tab=vehicle");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowScreenshotMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=screenshots");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowBulkImportList(std::string listId)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl(VarArgs("asset://ui/bulk.html?list=%s", listId.c_str()));
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowFavoritesMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=backpack");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowCommandsMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=commands");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowPaintMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=paint");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ShowPlayersMenu()
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html?tab=players");
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::HideTaskMenu()
{
	m_pWindowManager->DoPendingSwitch();	// in case the user clicked on SwitchToWindowInstance while looking at the task manager, we must wait until he releases the menu button.
	m_pInputManager->DeactivateInputMode(true);
}

void C_AnarchyManager::ObsoleteLegacyCommandReceived()
{
	DevMsg("Obsolete LEGACY command detected! Attempting to correct your keybinds for REDUX...\n");

	if (g_pFullFileSystem->FileExists("config/config.cfg", "DEFAULT_WRITE_PATH"))
	{
		// make a uniquely named config_redux.cfg to make sure we exec the right one
		CUtlBuffer buf;
		if (filesystem->ReadFile("config/config.cfg", "DEFAULT_WRITE_PATH", buf))
		{
			filesystem->WriteFile("config/config_redux.cfg", "DEFAULT_WRITE_PATH", buf);
			engine->ClientCmd("exec config_redux\n");
		}
		buf.Purge();
	}
	else
	{
		engine->ClientCmd("exec config_default_redux\n");
	}
}

bool C_AnarchyManager::ShouldTextureClamp()
{
	if (!m_pTextureClampConVar)
		m_pTextureClampConVar = cvar->FindVar("clamp_dynamic_textures");

	return m_pTextureClampConVar->GetBool();
}

void C_AnarchyManager::StartHoldingPrimaryFire()
{
	if (m_bIsHoldingPrimaryFire)
		return;

	m_bIsHoldingPrimaryFire = true;

	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	//pHudInstance->SetUrl("asset://ui/primaryMenu.html");
	pHudInstance->SetUrl("asset://ui/buildMode.html?mode=select");
	if (vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER))
		g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, null, false);
}

void C_AnarchyManager::StopHoldingPrimaryFire()
{
	if (!m_bIsHoldingPrimaryFire)
		return;
	
	m_bIsHoldingPrimaryFire = false;

	bool bOldGamepadInputMode = g_pAnarchyManager->GetInputManager()->IsGamepadInputMode();

	C_BaseEntity* pGlowEntity = m_pHoverGlowEntity;
	g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
	if (pGlowEntity)
	{
		//if (m_pSelectedEntity)	// NOTE: Nothing should be selected at this point, because a fullscreen HUD menu is what initiates this method.
//			g_pAnarchyManager->DeselectEntity();

		//g_pAnarchyManager->AttemptSelectEntity(pGlowEntity);	// Instead of selecting the entity, we're making the build mode context appear instead.
		//if (m_pSelectedEntity == pGlowEntity)
		//{
			/* have the player look at the entity, if possible.
			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			Vector lookPos = pGlowEntity->GetAbsOrigin();
			pPlayer->Eye
			//lookPos.
			*/
		//}

		//if (g_pAnarchyManager->GetSelectedEntity())
			//g_pAnarchyManager->DeselectEntity("asset://ui/buildModeContext.html");
		//else
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl(VarArgs("asset://ui/buildModeContext.html?entity=%i", pGlowEntity->entindex()));

		if (bOldGamepadInputMode)
			g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);

		g_pAnarchyManager->GetAccountant()->Action("aa_selected_collisionless", 1);
	}
}

void C_AnarchyManager::MinimizeAArcade()
{
	ShowWindow(m_hwnd, SW_MINIMIZE);
}

void C_AnarchyManager::EndTempFreeLook()
{
	this->ShowMouseMenu();
}

// IMPORTANT NOTE: Key values loaded with LoadFromFile forget the data types for their fields and if a string is just numbers, it gets turned into a number instead of a string after loading.
// This matters if the number started with a 0, because leading zeros get removed for numbers.
// So to catch this, additional checks must be performed on ID's read from KeyValues files.
bool C_AnarchyManager::CompareLoadedFromKeyValuesFileId(const char* testId, const char* baseId)
{
	/*if (Q_strlen(testId) < 8)
	{
		DevMsg("less than!\n");
	}
	else
		DevMsg("greater than\n");*/

	int intBaseId = Q_atoi(baseId);
	if (!Q_stricmp(testId, baseId) || (intBaseId != 0 && Q_atoi(testId) == intBaseId))//(Q_strcmp(VarArgs("%i", intBaseId), baseId) && intBaseId == intBaseId == Q_atoi(testId)))
		return true;
	else
		return false;
		
}

void C_AnarchyManager::Popout(std::string popoutId, std::string auxId, bool bQuietRun)
{
	std::string goodUrl = "";
	if (popoutId == "kodi")
		goodUrl = "https://kodi.tv/";
	else if (popoutId == "libretro")
		goodUrl = "https://www.libretro.com/";
	else if (popoutId == "steamworks")
		goodUrl = "https://partner.steamgames.com/doc/api/ISteamHTMLSurface";
	else if (popoutId == "awesomium")
		goodUrl = "http://www.awesomium.com/";
	else if (popoutId == "twitch")
		goodUrl = "http://www.twitch.tv/";
	else if (popoutId == "twitchtoken")
		goodUrl = "https://twitchapps.com/tmi/";
	else if (popoutId == "hammer")
		goodUrl = "https://www.youtube.com/watch?v=XrCm0A6TsC4";
	else if (popoutId == "lobby")
	{
		std::string firstLetter = VarArgs("%c", auxId.at(0));
		if (this->AlphabetSafe(firstLetter, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") && this->AlphabetSafe(auxId, "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"))
		{
			goodUrl = "http://aarcade.tv/live/";
			goodUrl += auxId;
		}
	}

	if (goodUrl != "")
		this->Acquire(goodUrl, bQuietRun);
		//steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(goodUrl.c_str());
}

void C_AnarchyManager::CreateHammerProject(std::string projectName)
{
	DevMsg("Create project: %s\n", projectName.c_str());

	std::string mapTemplateFilename = "resource\\mapTemplate.vmf";
	if (!g_pFullFileSystem->FileExists(mapTemplateFilename.c_str(), "MOD"))
	{
		DevMsg("ERROR: Map template cannot be found. Aborting.");
		return;
	}

	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));
	std::string projectFilename = VarArgs("%s_%s.vmf", projectName.c_str(), userHash.c_str());
	std::string shortProjectPath = VarArgs("mapsrc\\%s\\", projectName.c_str());
	std::string shortProjectFilename = VarArgs("%s%s", shortProjectPath.c_str(), projectFilename.c_str());

	//std::string fullProjectPath = VarArgs("%smapsrc\\%s\\", this->GetAArcadeUserFolder().c_str(), projectName.c_str());
	//std::string fullProjectFilename = VarArgs("%s%s", fullProjectPath.c_str(), projectFilename.c_str());

	// does the project filename already exist?
	if (g_pFullFileSystem->FileExists(shortProjectFilename.c_str(), "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Project file \"%s\" already exists.  Aborting.", shortProjectFilename.c_str());
		return;
	}

	g_pFullFileSystem->CreateDirHierarchy(shortProjectPath.c_str(), "DEFAULT_WRITE_PATH");// projectPath.c_str());

	// copy source to destination
	CUtlBuffer buf;
	if (g_pFullFileSystem->ReadFile(mapTemplateFilename.c_str(), "MOD", buf))
	{
		if (!g_pFullFileSystem->WriteFile(shortProjectFilename.c_str(), "DEFAULT_WRITE_PATH", buf))
		{
			DevMsg("ERROR: Could not write project file \"%s\".  Aborting.", shortProjectFilename.c_str());
			return;
		}
	}
}

void C_AnarchyManager::OpenHammerProject(std::string projectName)
{
	std::string toolsPath = this->GetAArcadeToolsFolder();
	std::string basePath = this->GetAArcadeToolsFolder();
	auto found = basePath.find_last_of("\\");
	basePath = basePath.substr(0, found);

	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));
	std::string projectFilename = VarArgs("%s_%s.vmf", projectName.c_str(), userHash.c_str());
	std::string shortProjectPath = VarArgs("mapsrc\\%s\\", projectName.c_str());
	std::string shortProjectFilename = VarArgs("%s%s", shortProjectPath.c_str(), projectFilename.c_str());
	std::string fullProjectFilename = VarArgs("%s\\%s", this->GetAArcadeUserFolder().c_str(), shortProjectFilename.c_str());

	if (projectName != "")
	{
		DevMsg("Open project: %s\n", projectName.c_str());

		// does the VMF exist?
		if (!g_pFullFileSystem->FileExists(fullProjectFilename.c_str()))
		{
			DevMsg("ERROR: Project VMF file \"%s\" cannot be found. Aborting.", fullProjectFilename.c_str());
			return;
		}
	}

	// auto-configure Hammer...
	std::string toolsFolder = this->GetAArcadeToolsFolder();
	std::string fullConfigFilename = VarArgs("%s\\GameConfig.txt", toolsFolder.c_str());

	DevMsg("Config file is: %s\n", fullConfigFilename.c_str());

	KeyValues* configKV = new KeyValues("Configs");
	KeyValues* aarcadeKV = null;
	if (g_pFullFileSystem->FileExists(fullConfigFilename.c_str()))
	{
		if (configKV->LoadFromFile(g_pFullFileSystem, fullConfigFilename.c_str()))
		{
			KeyValues* gamesKV = configKV->FindKey("Games");
			if (gamesKV)
			{
				for (KeyValues *sub = gamesKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					if (!Q_stricmp(sub->GetName(), "Anarchy Arcade"))
					{
						aarcadeKV = sub;
						break;
					}
				}

				if (!aarcadeKV)
				{
					configKV->deleteThis();
					configKV = new KeyValues("Configs");
				}
			}
			else
			{
				configKV->deleteThis();
				configKV = new KeyValues("Configs");
			}
		}
		else
		{
			configKV->deleteThis();
			configKV = new KeyValues("Configs");
		}
	}

	if (aarcadeKV)
	{
		aarcadeKV->Clear();
		aarcadeKV = null;
	}

	if ( !aarcadeKV )
	{
		std::string gameDir = basePath;
		gameDir += "\\frontend";

		std::string gameData = basePath;
		gameData += "\\bin\\hl2mp.fgd";

		std::string gameExe = basePath;
		gameExe += "\\aarcade.exe";

		std::string bsp = basePath;
		bsp += "\\bin\\vbsp.exe";

		std::string vis = basePath;
		vis += "\\bin\\vvis.exe";

		std::string rad = basePath;
		rad += "\\bin\\vrad.exe";

		std::string mapDir = basePath;
		mapDir += "\\aarcade_user\\mapsrc";

		std::string bspDir = basePath;
		bspDir += "\\aarcade_user\\maps";

		KeyValues* gamesKV = configKV->FindKey("Games");
		if (!gamesKV)
		{
			gamesKV = configKV->CreateNewKey();
			gamesKV->SetName("Games");
		}

		aarcadeKV = gamesKV->CreateNewKey();
		aarcadeKV->SetName("Anarchy Arcade");
		aarcadeKV->SetString("GameDir", gameDir.c_str());
		aarcadeKV->SetString("Hammer/GameData0", gameData.c_str());
		aarcadeKV->SetString("Hammer/TextureFormat", "5");
		aarcadeKV->SetString("Hammer/MapFormat", "4");
		aarcadeKV->SetString("Hammer/DefaultTextureScale", "0.250000");
		aarcadeKV->SetString("Hammer/DefaultLightmapScale", "16");
		aarcadeKV->SetString("Hammer/GameExe", gameExe.c_str());
		aarcadeKV->SetString("Hammer/DefaultSolidEntity", "func_detail");
		aarcadeKV->SetString("Hammer/DefaultPointEntity", "prop_static");
		aarcadeKV->SetString("Hammer/BSP", bsp.c_str());
		aarcadeKV->SetString("Hammer/Vis", vis.c_str());
		aarcadeKV->SetString("Hammer/Light", rad.c_str());
		aarcadeKV->SetString("Hammer/GameExeDir", basePath.c_str());
		aarcadeKV->SetString("Hammer/MapDir", mapDir.c_str());
		aarcadeKV->SetString("Hammer/BSPDir", bspDir.c_str());
		aarcadeKV->SetString("Hammer/CordonTexture", "tools\\toolsskybox");
		aarcadeKV->SetString("Hammer/MaterialExcludeCount", "0");

		configKV->SetInt("SDKVersion", 5);
		configKV->SaveToFile(g_pFullFileSystem, fullConfigFilename.c_str());
		configKV->deleteThis();
	}

	// launch Hammer
	char driveLetter = toolsPath.at(0);

	FileHandle_t launch_file = g_pFullFileSystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");

	if (!launch_file)
	{
		Msg("Error creating ArcadeLauncher.bat!\n");
		return;
	}

	g_pFullFileSystem->FPrintf(launch_file, "%c:\n", driveLetter);
	g_pFullFileSystem->FPrintf(launch_file, "cd \"%s\"\n", toolsPath.c_str());

	if ( projectName != "" && g_pFullFileSystem->FileExists(fullProjectFilename.c_str()))
	{
		g_pFullFileSystem->FPrintf(launch_file, "START \"Launching...\" \"hammer.exe\" \"%s\"", fullProjectFilename.c_str());
	}
	else
		g_pFullFileSystem->FPrintf(launch_file, "START \"Launching...\" \"hammer.exe\"");
	g_pFullFileSystem->Close(launch_file);
	system("Arcade_Launcher.bat");
}

void C_AnarchyManager::ShowRawMainMenu()
{
	bool bInputMode = m_pInputManager->GetInputMode();
	if (bInputMode)// || bForce)
	{
		this->Disconnect();

		//if ( bInputMode)
		m_pInputManager->DeactivateInputMode(true);
	}
}

void C_AnarchyManager::GetAllHammerProjects(std::vector<std::string> &projects)
{
	// VALID PROJECTS:
	//	aarcade_user/mapsrc/PROJECTNAME/PROJECTNAME_USERIDHASH.vmf

	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));

	// loop through all folders in aarcade_user/mapsrc, and ones that have a PROJECTNAME_USERIDHASH.vmf in it get pushed onto the vector!
	std::string fullMapSRCFolder = VarArgs("%s\\mapsrc", this->GetAArcadeUserFolder().c_str());
	std::string projectFilename;
	std::string projectFolder;
	FileFindHandle_t findHandle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%s\\*", fullMapSRCFolder.c_str()), "", &findHandle);
	while (pFilename != NULL)
	{
		projectFilename = VarArgs("%s_%s.vmf", pFilename, userHash.c_str());

		if (g_pFullFileSystem->FileExists(VarArgs("%s\\%s\\%s", fullMapSRCFolder.c_str(), pFilename, projectFilename.c_str())))
			projects.push_back(std::string(pFilename));

		pFilename = g_pFullFileSystem->FindNext(findHandle);
	}
	g_pFullFileSystem->FindClose(findHandle);
}

std::string C_AnarchyManager::GetHammerProjectMapID(std::string projectName)
{
	std::string mapId = "";
	// generate the project map name
	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));
	std::string projectMapFilename = VarArgs("%s_%s.bsp", projectName.c_str(), userHash.c_str());

	// search to see if there is already a map with this file
	KeyValues* pMapKV = null;// = m_pMetaverseManager->FindMap(projectMapFilename.c_str());
	if (g_pFullFileSystem->FileExists(VarArgs("maps\\%s", projectMapFilename.c_str()), "GAME"))
		pMapKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->DetectMap(projectMapFilename));	// this detects it OR just returns it, if its already detected.

	if (pMapKV)
		return std::string(pMapKV->GetString("info/id"));
	else
		return "";
}

void C_AnarchyManager::MapTransition(std::string mapfile, std::string spawnEntityName, std::string screenshot, std::string lobby, std::string lobbytitle, std::string instance, std::string pos, std::string rot)
{
	KeyValues* pMapKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->DetectMap(mapfile + ".bsp"));
	if (pMapKV)
	{
		std::string extras;
		//if (mapfile != "")
		//	extras += "&mapfile=" + mapfile;
		if (screenshot != "")
			extras += "&screenshot=" + screenshot;
		if (lobby != "")
			extras += "&lobby=" + lobby;
		if (lobbytitle != "")
			extras += "&lobbytitle=" + lobbytitle;
		if (instance != "")
			extras += "&instance=" + instance;
		if (pos != "")
			extras += "&pos=" + pos;
		if (rot != "")
			extras += "&rot=" + rot;

		// open the menu
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl(VarArgs("asset://ui/mapTransition.html?mapfile=%s&spawn=%s%s", mapfile.c_str(), spawnEntityName.c_str(), extras.c_str()));

		if (vgui::input()->IsKeyDown(KEY_XBUTTON_A))
			m_pInputManager->SetGamepadInputMode(true);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
	}
}
/*
void C_AnarchyManager::FindMap(std::string mapfile, std::string spawnEntityName)
{
	KeyValues* pMapKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->FindMap(mapfile.c_str()));
	if (pMapKV)
	{
		// open the menu
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl("asset://ui/mapTransition.html");
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
	}
}
*/

void C_AnarchyManager::Feedback(std::string type)
{
	std::string discussionsUrl = "http://steamcommunity.com/app/266430/discussions/5/";
	std::string suggestionsUrl = "http://steamcommunity.com/app/266430/discussions/6/";
	std::string bugsUrl = "http://steamcommunity.com/app/266430/discussions/4/";
	std::string trelloUrl = "https://trello.com/b/PLcyQaio";
	std::string discordUrl = "https://discord.gg/8cxtuKY";
	std::string twitchUrl = "http://www.twitch.tv/AnarchyArcade";

	std::string goodUrl = "";
	if (type == "discussions")
		goodUrl = discussionsUrl;
	else if (type == "suggestions")
		goodUrl = suggestionsUrl;
	else if (type == "bugs")
		goodUrl = bugsUrl;
	else if (type == "trello")
		goodUrl = trelloUrl;
	else if (type == "discord")
		goodUrl = discordUrl;
	else if (type == "twitch")
		goodUrl = twitchUrl;

	if (goodUrl != "")
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(goodUrl.c_str());
}

#include "baseviewport.h"
void C_AnarchyManager::PlaySound(std::string file)
{
	vgui::surface()->PlaySound(file.c_str());
}

void C_AnarchyManager::SetModelSequence(C_PropShortcutEntity* pShortcut, std::string sequenceName)
{
	//int iSequence = pShortcut->LookupSequence(sequenceName.c_str());
	//if (iSequence > 0)
	if (sequenceName != "")
		pShortcut->PlaySequenceRegular(sequenceName.c_str());
}

void C_AnarchyManager::SetModelSkin(C_PropShortcutEntity* pShortcut, int iSkin)
{
	CStudioHdr *hdr = pShortcut->GetModelPtr();
	if (hdr)
	{
		if (iSkin < hdr->numskinfamilies())
			engine->ClientCmd(VarArgs("setskin %i %i", pShortcut->entindex(), iSkin));
	}
}

KeyValues* C_AnarchyManager::GetAllStickersKV()
{
	if (!m_pAllStickersKV)
		this->DetectAllStickerPNGs();

	return m_pAllStickersKV;
}

void C_AnarchyManager::DetectAllStickerPNGs()
{
	if (m_pAllStickersKV)
		m_pAllStickersKV->deleteThis();

	m_pAllStickersKV = new KeyValues("stickers");

	std::string id;
	FileFindHandle_t pFileFindHandle;
	const char *pFile = g_pFullFileSystem->FindFirstEx("resource\\ui\\html\\stickers\\*.png", "MOD", &pFileFindHandle);
	while (pFile != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(pFileFindHandle))
		{
			pFile = g_pFullFileSystem->FindNext(pFileFindHandle);
			continue;
		}

		id = pFile;
		if (id.length() < 5)
		{
			pFile = g_pFullFileSystem->FindNext(pFileFindHandle);
			continue;
		}
		id = id.substr(0, id.length() - 4);

		m_pAllStickersKV->CreateNewKey()->SetString("", id.c_str());
		pFile = g_pFullFileSystem->FindNext(pFileFindHandle);
	}
	g_pFullFileSystem->FindClose(pFileFindHandle);
}

void C_AnarchyManager::NotifyGameSchemaFetched(std::string responseText)
{
	std::string name;
	std::string displayName;
	std::string icon;
	std::vector<std::string> params;
	size_t foundEntryEnd;
	size_t foundDisplayName;
	size_t foundIcon;

	// NAME FIELD
	size_t foundName = responseText.find("\"name\":\"");
	while (foundName != std::string::npos)
	{
		responseText = responseText.substr(foundName + 8);

		// NAME FIELD END
		foundEntryEnd = responseText.find("\"");
		if (foundEntryEnd != std::string::npos)
		{
			// NAME VALUE
			name = responseText.substr(0, foundEntryEnd);
			responseText = responseText.substr(foundEntryEnd + 1);

			// DISPLAYNAME FIELD
			foundDisplayName = responseText.find("\"displayName\":\"");
			if (foundDisplayName != std::string::npos)
			{
				responseText = responseText.substr(foundDisplayName + 15);

				// DISPLAYNAME FIELD END
				foundEntryEnd = responseText.find("\"");
				if (foundEntryEnd != std::string::npos)
				{
					// DISPLAYNAME VALUE
					displayName = responseText.substr(0, foundEntryEnd);
					responseText = responseText.substr(foundEntryEnd + 1);

					// ICON FIELD
					foundIcon = responseText.find("\"icon\":\"");
					if (foundIcon != std::string::npos)
					{
						responseText = responseText.substr(foundIcon + 8);

						// ICON FIELD END
						foundEntryEnd = responseText.find("\"");
						if (foundEntryEnd != std::string::npos)
						{
							// ICON VALUE
							icon = responseText.substr(0, foundEntryEnd);
							responseText = responseText.substr(foundEntryEnd + 1);

							// victory
							params.push_back(name);
							params.push_back(displayName);
							params.push_back(icon);
						}
					}
				}
			}
		}

		foundName = responseText.find("\"name\":\"");
	}

	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudInstance->DispatchJavaScriptMethod("arcadeHud", "onGameSchemaFetched", params);
}

void C_AnarchyManager::HardPause()
{
	engine->ClientCmd("stopsound");
	//m_pSteamBrowserManager->CloseAllInstances();
	//m_pLibretroManager->CloseAllInstances();
	//m_pAwesomiumBrowserManager->CloseAllInstances();
	//bool bOldDeferredValue = cvar->FindVar("use_deferred_texture_cleanup")->GetBool();
	//m_pCanvasManager->SetDeferredTextureCleanup(false);
	m_pCanvasManager->CleanupTextures(true);	// cleanup any deferred textures right now
	m_pCanvasManager->PrepareRefreshItemTextures("", "ALL");
	//m_pCanvasManager->SetDeferredTextureCleanup(bOldDeferredValue);
	//g_pMaterialSystem->UncacheAllMaterials();
	//g_pMaterialSystem->EndRenderTargetAllocation();
	materials->ReleaseResources();
	materials->Flush(true);
	materials->EvictManagedResources();
}

void C_AnarchyManager::WakeUp()
{
	unsigned int uNumCleanup = m_pCanvasManager->GetNumPendingTextureCleanup();
	if (uNumCleanup > 0)
		DevMsg("Num Cleanup Textures: %u\n", uNumCleanup);

	materials->ReacquireResources();
	//g_pMaterialSystem->BeginRenderTargetAllocation();
	//m_pCanvasManager->SetDeferredTextureCleanup(true);
	m_pCanvasManager->RefreshItemTextures("", "ALL");
}

void C_AnarchyManager::TaskClear()
{
	if (this->GetSelectedEntity())
		this->DeselectEntity();

	m_pCanvasManager->CloseAllInstances();
}

void C_AnarchyManager::TaskRemember(C_PropShortcutEntity* pShortcutIn)
{
	C_PropShortcutEntity* pShortcut = pShortcutIn;
	
	if (!pShortcut)
	{
		C_BaseEntity* pEntity = g_pAnarchyManager->GetSelectedEntity();
		if (pEntity)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
	}

	if (!pShortcut)
	{
		C_BaseEntity* pEntity = null;
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer && pPlayer->GetHealth() > 0)
		{
			/*
			// fire a trace line
			trace_t tr;
			this->SelectorTraceLine(tr);

			//Vector forward;
			//pPlayer->EyeVectors(&forward);
			//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

			if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
				pEntity = tr.m_pEnt;
			*/

			pEntity = C_BaseEntity::Instance(m_iSelectorTraceEntityIndex);

			// only allow prop shortcuts
			if (pEntity)
			{
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				if (pShortcut)
				{
					this->QuickRemember(pShortcut->entindex());// , "autoinspect");
					m_pAccountant->Action("aa_objects_autoplayed", 1);
					return;
				}
			}
		}
	}

	if ( pShortcut )
	{
		//std::vector<C_EmbeddedInstance*> embeddedInstances;
		//pShortcut->GetEmbeddedInstances(embeddedInstances);

		//C_EmbeddedInstance* pEmbeddedInstance;
		//C_EmbeddedInstance* testerInstance;
		//unsigned int i;
		//unsigned int size = embeddedInstances.size();
		//for (i = 0; i < size; i++)
		//{
			//pEmbeddedInstance = embeddedInstances[i];
			//if (pEmbeddedInstance->GetId() == "images")
			//{
				C_EmbeddedInstance* testerInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance("auto" + pShortcut->GetItemId());
				if (testerInstance && testerInstance->GetTexture())
				{
					//g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(testerInstance);
					g_pAnarchyManager->DeselectEntity("", false);
					//break; // only put the 1st embedded instance on continous play
				}
			//}
		//}
	}
}

void C_AnarchyManager::SetSlaveScreen(std::string objectId, bool bVal)
{
	C_PropShortcutEntity* pShortcut = null;
	C_BaseEntity* pEntity = null;
	
	if (objectId == "")
		pEntity = this->GetSelectedEntity();
	else
		pEntity = m_pInstanceManager->GetObjectEntity(objectId);

	if (pEntity)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);

	if (pShortcut)
	{
		object_t* pObject = this->GetInstanceManager()->GetInstanceObject(pShortcut->GetObjectId());
		if (pObject)
		{
			pObject->slave = bVal;	// renabled on 4/30/2018 cuz why was it disabled anyways?
			pShortcut->SetSlave(bVal);
			this->GetInstanceManager()->ApplyChanges(pShortcut);	// will also update the object

			int iVal = (bVal) ? 1 : 0;
			engine->ClientCmd(VarArgs("setslave %i %i", pEntity->entindex(), iVal));
		}
	}
}

void C_AnarchyManager::PostRender()
{
	if (!this->IsInitialized())
		return;
//	SteamAPI_RunCallbacks();
	//DevMsg("AnarchyManager: PostRender\n");

	if (engine->IsInGame() && m_pCanvasManager && !this->IsPaused())
		m_pCanvasManager->CleanupTextures();

	if (m_pNextBigScreenshotKV)
		g_pAnarchyManager->DoTakeBigScreenshot();

	//g_pAnarchyManager->VRFrameReady();
	//g_pAnarchyManager->VRFrameBegin();
}


void C_AnarchyManager::OnConnectionMetricsUpdate(std::vector<int> metrics)
{
	if (metrics.size() < 2)
		return;

	m_numberStatsState.iServerVisitors = metrics[0];
	m_numberStatsState.iServerVisits = metrics[1];
}

// Over 15x faster than: (int)floor(value)
/*
inline int Floor2Int(float a)
{
	int RetVal;
#if defined( __i386__ )
	// Convert to int and back, compare, subtract one if too big
	__m128 a128 = _mm_set_ss(a);
	RetVal = _mm_cvtss_si32(a128);
	__m128 rounded128 = _mm_cvt_si2ss(_mm_setzero_ps(), RetVal);
	RetVal -= _mm_comigt_ss(rounded128, a128);
#else
	RetVal = static_cast<int>(floor(a));
#endif
	return RetVal;
}
*/

void C_AnarchyManager::SteamTalker(std::string text, std::string voice, float flPitch, float flRate, float flVolume)
{
	//std::string talkerUrl = "asset://ui/steamTalker.html";// VarArgs("asset://ui/steamTalker.html?text=%s&voice=%s&pitch=%f&rate=%f&volume=%f", this->encodeURIComponent(text).c_str(), this->encodeURIComponent(voice).c_str(), flPitch, flRate, flVolume);


	std::string uri = "file://";
	uri += engine->GetGameDirectory();
	uri += "/resource/ui/html/";
	uri += VarArgs("steamTalk.html?text=%s&voice=%s&pitch=%f&rate=%f&volume=%f", this->encodeURIComponent(text).c_str(), this->encodeURIComponent(voice).c_str(), flPitch, flRate, flVolume);
	//uri = "file://A:/SteamLibrary/steamapps/common/Anarchy Arcade/frontend/resource/ui/html/steamTalk.html";

	C_SteamBrowserInstance* pSteamBrowserInstance = this->GetSteamBrowserManager()->FindSteamBrowserInstance("SteamTalker");
	if (!pSteamBrowserInstance)
	{
		pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
		pSteamBrowserInstance->Init("SteamTalker", uri, "AArcade SteamTalker", null, -1);
	}
	else
	{
		steamapicontext->SteamHTMLSurface()->MouseDown(pSteamBrowserInstance->GetHandle(), ISteamHTMLSurface::eHTMLMouseButton_Left);
		steamapicontext->SteamHTMLSurface()->MouseUp(pSteamBrowserInstance->GetHandle(), ISteamHTMLSurface::eHTMLMouseButton_Left);
		pSteamBrowserInstance->InjectJavaScript(VarArgs("steamSpeak(\"%s\", \"%s\", \"%f\", \"%f\", \"%f\");", text.c_str(), voice.c_str(), flPitch, flRate, flVolume));
	}
		//pSteamBrowserInstance->SetUrl(talkerUrl);
}

#include <chrono>
void C_AnarchyManager::GenerateUniqueId(char* result)
{
	std::string PUSH_CHARS = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

	double now = std::chrono::system_clock::now().time_since_epoch().count();
	now = floor(now / 64.0);
	now = floor(now / 64.0);

	while (now <= m_dLastGenerateIdTime)
		now++;

	bool duplicateTime = (now == m_dLastGenerateIdTime);
	m_dLastGenerateIdTime = now;

	//char* timeStampChars[8];
	std::string timeStampChars = "00000000";
	for (unsigned int i = 8; i > 0; i--)
	{
		timeStampChars.replace(i - 1, 1, 1, PUSH_CHARS.at(fmod(now, 64.0)));
		now = floor(now / 64.0);
	}

	if (now != 0)
	{
		DevMsg("ERROR: We should have converted the entire timestamp. %f\n", now);
	}

	std::string id = timeStampChars;
	if (!duplicateTime)
	{
		for (unsigned int i = 0; i < 12; i++)
			m_lastGeneratedChars.replace(i, 1, 1, (char)floor(random->RandomFloat() * 64.0L));
	}
	else
	{
		// If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
		unsigned int i;
		for (i = 11; i >= 0 && m_lastGeneratedChars.at(i) == 63; i--)
			m_lastGeneratedChars.replace(i, 1, 1, (char)0);

		m_lastGeneratedChars.replace(i, 1, 1, (char)(m_lastGeneratedChars.at(i) + 1));
	}

	for (unsigned int i = 0; i < 12; i++)
	{
		id += PUSH_CHARS.at(m_lastGeneratedChars.at(i));
	}

	if (id.length() != 20)
		DevMsg("ERROR: Lngth should be 20.\n");

	Q_strcpy(result, id.c_str());

	//return VarArgs("%s", id.c_str());	// works on instance menu	// GETS ALL CALLS DURING SAME TICK
	return;// id.c_str();	// works on library browser menu	// GETS CALLS ON DIFFERENT TICKS
}

const char* C_AnarchyManager::GenerateUniqueId()
{
	std::string PUSH_CHARS = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

	double now = std::chrono::system_clock::now().time_since_epoch().count();
	now = floor(now / 64.0);
	now = floor(now / 64.0);

	while (now <= m_dLastGenerateIdTime)
		now++;

	bool duplicateTime = (now == m_dLastGenerateIdTime);
	m_dLastGenerateIdTime = now;

	std::string timeStampChars = "00000000";
	for (unsigned int i = 8; i > 0; i--)
	{
		timeStampChars.replace(i-1, 1, 1, PUSH_CHARS.at(fmod(now, 64.0)));
		now = floor(now / 64.0);
	}

	if (now != 0)
	{
		DevMsg("ERROR: We should have converted the entire timestamp. %f\n", now);
	}

	std::string id = timeStampChars;
	if (!duplicateTime)
	{
		for (unsigned int i = 0; i < 12; i++)
				m_lastGeneratedChars.replace(i, 1, 1, (char)floor(random->RandomFloat() * 64.0L));
	}
	else
	{
		// If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
		unsigned int i;
		for (i = 11; i >= 0 && m_lastGeneratedChars.at(i) == 63; i--)
			m_lastGeneratedChars.replace(i, 1, 1, (char)0);

		m_lastGeneratedChars.replace(i, 1, 1, (char)(m_lastGeneratedChars.at(i) + 1));
	}

	for (unsigned int i = 0; i < 12; i++)
	{
		id += PUSH_CHARS.at(m_lastGeneratedChars.at(i));
	}

	if (id.length() != 20)
		DevMsg("ERROR: Lngth should be 20.\n");

	return VarArgs("%s", id.c_str());	// works on instance menu	// GETS ALL CALLS DURING SAME TICK
	//return id.c_str();	// works on library browser menu	// GETS CALLS ON DIFFERENT TICKS
}

const char* C_AnarchyManager::GenerateUniqueId2()
{
	std::string PUSH_CHARS = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

	double now = std::chrono::system_clock::now().time_since_epoch().count();
	now = floor(now / 64.0);
	now = floor(now / 64.0);

	while (now <= m_dLastGenerateIdTime)
		now++;

	bool duplicateTime = (now == m_dLastGenerateIdTime);
	m_dLastGenerateIdTime = now;

	std::string timeStampChars = "00000000";
	for (unsigned int i = 8; i > 0; i--)
	{
		timeStampChars.replace(i - 1, 1, 1, PUSH_CHARS.at(fmod(now, 64.0)));
		now = floor(now / 64.0);
	}

	if (now != 0)
	{
		DevMsg("ERROR: We should have converted the entire timestamp. %f\n", now);
	}

	std::string id = timeStampChars;
	if (!duplicateTime)
	{
		for (unsigned int i = 0; i < 12; i++)
			m_lastGeneratedChars.replace(i, 1, 1, (char)floor(random->RandomFloat() * 64.0L));
	}
	else
	{
		// If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
		unsigned int i;
		for (i = 11; i >= 0 && m_lastGeneratedChars.at(i) == 63; i--)
			m_lastGeneratedChars.replace(i, 1, 1, (char)0);

		m_lastGeneratedChars.replace(i, 1, 1, (char)(m_lastGeneratedChars.at(i) + 1));
	}

	for (unsigned int i = 0; i < 12; i++)
	{
		id += PUSH_CHARS.at(m_lastGeneratedChars.at(i));
	}

	if (id.length() != 20)
		DevMsg("ERROR: Lngth should be 20.\n");

	//return VarArgs("%s", id.c_str());	// works on instance menu	// GETS ALL CALLS DURING SAME TICK
	return id.c_str();	// works on library browser menu	// GETS CALLS ON DIFFERENT TICKS
}

std::string C_AnarchyManager::ExtractLegacyId(std::string itemFile, KeyValues* item)
{
	std::string alphabet = "0123456789abcdef";

	std::string nameSnip = "";
	bool bPassed = true;

	size_t found = itemFile.find(":");
	if (found != std::string::npos)
		bPassed = false;

	if (bPassed)
	{
		found = itemFile.find_last_of("/\\");
		if (found == std::string::npos)
			bPassed = false;
	}

	if (bPassed)
	{
		nameSnip = itemFile.substr(found + 1);
		found = nameSnip.find_first_of(".");

		if (found == std::string::npos)
			bPassed = false;
		else
		{
			nameSnip = nameSnip.substr(0, found);

			unsigned int nameSnipLength = nameSnip.length();
			if (nameSnipLength != 8)
				bPassed = false;
			else
			{
				unsigned int i;
				for (i = 0; i < nameSnipLength; i++)
				{
					found = alphabet.find(nameSnip[i]);
					if (found == std::string::npos)
					{
						bPassed = false;
						break;
					}
				}
			}
		}
	}
	
	//// generate a legacy ID based on the filelocation if given an item to work with
//	if (!bPassed && item)
	//	nameSnip = this->GenerateLegacyHash(item->GetString("filelocation"));

	if (!bPassed)
		nameSnip = "";

	return nameSnip;
}

const char* C_AnarchyManager::GenerateCRC32Hash(const char* text)
{
	char protectedHash[9];

	CRC32_t protectedCRC = CRC32_ProcessSingleBuffer((void*)text, strlen(text));
	Q_snprintf(protectedHash, 9, "%08x", protectedCRC);

	return VarArgs("%s", protectedHash);
}

const char* C_AnarchyManager::GenerateLegacyHash(const char* text)
{
	int inputLength = strlen(text);
	char* input = new char[inputLength+1];
	Q_strncpy(input, text, inputLength+1);

	// Convert it to lowercase & change all slashes to back-slashes
	V_FixSlashes(input);

	unsigned m_crc = 0xffffffff;

	inputLength = strlen(input);
	for (int i = 0; i < inputLength; i++)
	{
		input[i] = tolower(input[i]);
	}

	for (int i = 0; i < inputLength; i++)
	{
		unsigned c = input[i];
		m_crc ^= (c << 24);

		for (int j = 0; j < 8; j++)
		{
			const unsigned FLAG = 0x80000000;
			if ((m_crc & FLAG) == FLAG)
			{
				m_crc = (m_crc << 1) ^ 0x04C11DB7;
			}
			else
			{
				m_crc <<= 1;
			}
		}
	}

	delete[] input;

	return VarArgs("%08x", m_crc);

	/*
	char input[AA_MAX_STRING];
	//Q_strcpy(input, text);
	Q_strncpy(input, text, AA_MAX_STRING);

	// Convert it to lowercase & change all slashes to back-slashes
	V_FixSlashes(input);
	for (int i = 0; input[i] != '\0'; i++)
		input[i] = tolower(input[i]);

	char lower[AA_MAX_STRING];
	unsigned m_crc = 0xffffffff;

	int inputLength = strlen(input);
	for (int i = 0; i < inputLength; i++)
	{
		lower[i] = tolower(input[i]);
	}

	for (int i = 0; i < inputLength; i++)
	{
		unsigned c = lower[i];
		m_crc ^= (c << 24);

		for (int j = 0; j < 8; j++)
		{
			const unsigned FLAG = 0x80000000;
			if ((m_crc & FLAG) == FLAG)
			{
				m_crc = (m_crc << 1) ^ 0x04C11DB7;
			}
			else
			{
				m_crc <<= 1;
			}
		}
	}

	return VarArgs("%08x", m_crc);
	*/
}

/*
var PUSH_CHARS = '-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz';

var now = new Date().getTime();
var duplicateTime = (now === this.lastPushTime);
this.lastPushTime = now;

var timeStampChars = new Array(8);
for (var i = 7; i >= 0; i--) {
timeStampChars[i] = PUSH_CHARS.charAt(now % 64);
// NOTE: Can't use << here because javascript will convert to int and lose the upper bits.
now = Math.floor(now / 64);
}
if (now !== 0) throw new Error('We should have converted the entire timestamp.');

var id = timeStampChars.join('');

if (!duplicateTime) {
for (i = 0; i < 12; i++) {
this.lastRandChars[i] = Math.floor(Math.random() * 64);
}
} else {
// If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
for (i = 11; i >= 0 && this.lastRandChars[i] === 63; i--) {
this.lastRandChars[i] = 0;
}
this.lastRandChars[i]++;
}
for (i = 0; i < 12; i++) {
id += PUSH_CHARS.charAt(this.lastRandChars[i]);
}
if(id.length != 20) throw new Error('Length should be 20.');

return id;
*/

void C_AnarchyManager::Disconnect()
{
	// called when a player clicks the LEAVE button on the main menu.  However, there are other ways they could disconnect w/o clicking that.
	m_bIsDisconnecting = true;
	engine->ClientCmd("disconnect;\n");


	//if (m_bIsDisconnecting)//&& !Q_strcmp(this->MapName(), "")
	//{
		//m_bIsDisconnecting = false;
		//this->RunAArcade();
		//this->HandleUiToggle();
	//}
}
/*
void C_AnarchyManager::ReloadMap()
{
	// Is an instance loaded?
	instance_t* pInstance = m_pInstanceManager->GetCurrentInstance();
	if (!pInstance)
		return;

	pInstance->
}
*/
bool C_AnarchyManager::UseBuildGhosts()
{
	if (!m_pBuildGhostConVar)
		m_pBuildGhostConVar = cvar->FindVar("build_ghosts");

	return m_pBuildGhostConVar->GetBool();
}

/* moved to server code for accurate vphysics.
void C_AnarchyManager::SelectorTraceLine(trace_t &tr)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->GetHealth() <= 0)
		return;
	
	//MAX_TRACE_LENGTH, MASK_NPCSOLID

	// fire a trace line
	if (!this->IsVRActive())
	{
		Vector forward;
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	}
	else
	{
#ifdef VR_ALLOWED
		Vector vForward;
		Vector vLeft;
		Vector vUp;

		C_DynamicProp* pVRHandRight = g_pAnarchyManager->GetVRHand(1);
		if (pVRHandRight)
		{
			pVRHandRight->GetVectors(&vForward, &vLeft, &vUp);
			Vector vHandOrigin = pVRHandRight->GetAbsOrigin();
			UTIL_TraceLine(vHandOrigin, vHandOrigin + (vForward + (vUp*-1.0)) * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
		}
		else
		{
			pPlayer->EyeVectors(&vForward);
			UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + vForward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
		}
#endif
	}
}
*/

bool C_AnarchyManager::CheckVideoCardAbilities()
{
	ITexture* pTestTexture = g_pMaterialSystem->CreateProceduralTexture("test", TEXTURE_GROUP_VGUI, AA_EMBEDDED_INSTANCE_WIDTH, AA_EMBEDDED_INSTANCE_HEIGHT, IMAGE_FORMAT_BGR888, 1);
	int multiplyer = g_pAnarchyManager->GetDynamicMultiplyer();

	if (!pTestTexture || pTestTexture->IsError() || pTestTexture->GetActualWidth() * multiplyer != AA_EMBEDDED_INSTANCE_WIDTH || pTestTexture->GetActualHeight() * multiplyer != AA_EMBEDDED_INSTANCE_HEIGHT)
	{
		if (pTestTexture)
			pTestTexture->DeleteIfUnreferenced();

		return false;
	}
	else
	{
		if (pTestTexture)
			pTestTexture->DeleteIfUnreferenced();

		return true;
	}
}

void C_AnarchyManager::AnarchyStartup()
{
	if (cvar->FindVar("vrworldmenutest")->GetBool())
	{
		engine->ClientCmd("map_background background01; sbs_rendering 1;");
		return;
	}

	m_hwnd = FindWindow(null, "AArcade: Source");
	if (!m_hwnd)
	{
		this->ThrowEarlyError("Unknown error: Could not find window with title AArcade: Source");
		return;
	}

	SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES | GetWindowLongPtr(m_hwnd, GWL_EXSTYLE));

	m_bAutoRes = cvar->FindVar("auto_res")->GetBool();


#ifdef VR_ALLOWED
	//ToggleVR
	m_bVRSuspending = true;
#endif

	m_bPrecacheInstances = cvar->FindVar("precache_instances")->GetBool();
	m_bBatchObjectSpawn = cvar->FindVar("spawn_objects_in_batches")->GetBool();

	m_pShouldAllowMultipleActiveConVar = cvar->FindVar("allow_multiple_active");
	m_pLocalAutoPlaylistsConVar = cvar->FindVar("local_auto_playlists");
	m_pFreeMouseModeConVar = cvar->FindVar("freemousemode");
	m_pShouldShowWindowsTaskBarConVar = cvar->FindVar("should_show_windows_task_bar");
	m_pRightFreeMouseToggleConVar = cvar->FindVar("right_free_mouse_toggle");
	m_pAutoCloseTasksConVar = cvar->FindVar("auto_close_tasks");
	m_pNoDrawShortcutsConVar = cvar->FindVar("nodraw_shortcuts");

	m_pAlwaysAnimatingImagesConVar = cvar->FindVar("always_animating_images");

	for (unsigned int i = 0; i < 10; i++)
		m_pActionBarSlotConVars[i] = cvar->FindVar(VarArgs("abslot%i", i));

	m_pPaintTextureConVar = cvar->FindVar("paint_texture");

	m_pVRSpectatorModeConVar = cvar->FindVar("vrspectator");
	m_pVRSpectatorMirrorModeConVar = cvar->FindVar("vrspectatormirror");
	m_pVRHMDRenderConVar = cvar->FindVar("vrhmdrender");
	m_pAutoRebuildSoundCacheConVar = cvar->FindVar("autobuildsoundcache");

	m_pAttractModeActiveConVar = cvar->FindVar("attract_mode_active");
	m_pCabinetAttractModeActiveConVar = cvar->FindVar("cabinet_attract_mode_active");

	// manage window res RTFN, before going any further.
	this->ManageWindow();

	// check for black screen bug
	if (!this->CheckVideoCardAbilities())
	{
		this->ThrowEarlyError("Sorry, your video card is not currently supported by Anarchy Arcade!\nPlease notify SM Sith Lord (the developer) of what video card you\nhave so support can be added!");
		return;
	}


	bool bNeedsConfigWrite = false;
	ConVar* pClientIdConVar = cvar->FindVar("aamp_client_id");
	std::string aampClientId = pClientIdConVar->GetString();
	if (aampClientId == "")
	{
		aampClientId = this->GenerateUniqueId();
		pClientIdConVar->SetValue(aampClientId.c_str());
		bNeedsConfigWrite = true;
	}

	ConVar* pClientKeyConVar = cvar->FindVar("aamp_client_key");
	std::string aampClientKey = pClientKeyConVar->GetString();
	if (aampClientKey == "")
	{
		aampClientKey = this->GenerateUniqueId();
		pClientKeyConVar->SetValue(aampClientKey.c_str());
		bNeedsConfigWrite = true;
	}

	ConVar* pServerKeyConVar = cvar->FindVar("aamp_server_key");
	std::string aampServerKey = pServerKeyConVar->GetString();
	if (aampServerKey == "")
	{
		aampServerKey = this->GenerateUniqueId();
		pServerKeyConVar->SetValue(aampServerKey.c_str());
		bNeedsConfigWrite = true;
	}

	if ( bNeedsConfigWrite )
		engine->ClientCmd("host_writeconfig");

	DevMsg("AnarchyManager: AnarchyStartup\n");
	m_bIncrementState = true;

	m_pYouTubeEndBehaviorConVar = cvar->FindVar("youtube_end_behavior");

	m_pYouTubePlaylistBehaviorConVar = cvar->FindVar("youtube_playlist_behavior");
	m_pYouTubeVideoBehaviorConVar = cvar->FindVar("youtube_video_behavior");
	m_pYouTubeRelatedConVar = cvar->FindVar("youtube_related");
	m_pYouTubeMixesConVar = cvar->FindVar("youtube_mixes");
	m_pYouTubeAnnotationsConVar = cvar->FindVar("youtube_annotations");

	m_pWaitForInitialImagesConVar = cvar->FindVar("wait_for_initial_images");
	m_pWeaponsEnabledConVar = cvar->FindVar("r_drawviewmodel");
	m_pHoverTitlesConVar = cvar->FindVar("cl_hovertitles");
	m_pToastMsgsConVar = cvar->FindVar("cl_toastmsgs");

	ConVar* pConVar = cvar->FindVar("engine_no_focus_sleep");
	pConVar->SetValue(cvar->FindVar("default_engine_no_focus_sleep")->GetString());
	m_oldEngineNoFocusSleep = pConVar->GetString();
	pConVar->SetValue(0);

	m_pShouldShowCrosshairConVar = cvar->FindVar("should_show_crosshair");

	ConVar* pOldAutoSaveConVar = cvar->FindVar("old_auto_save");
	int oldAutoSave = pOldAutoSaveConVar->GetInt();
	pOldAutoSaveConVar->SetValue(-1);
	if (oldAutoSave >= 0)
		cvar->FindVar("auto_save")->SetValue(oldAutoSave);

	if (m_pShouldShowCrosshairConVar->GetBool())
		this->CreateArcadeCrosshair();

	//m_state = AASTATE_INPUTMANAGER;
	/*
	m_pInstanceManager = new C_InstanceManager();
	m_pMetaverseManager = new C_MetaverseManager();
	m_pInputManager = new C_InputManager();
	m_pWebManager = new C_WebManager();
	m_pWebManager->Init();
	*/
}

void C_AnarchyManager::ShowSteamGrid()
{
	system("steam://nav/games/grid");
	/*
	if (m_pSelectedEntity)
		this->DeselectEntity("asset://ui/pause.html");
	else
	{
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl("asset://ui/pause.html");
	}

	m_pInputManager->ActivateInputMode(true, true);
	*/
	/*
	FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");
	if (!launch_file)
	{
		Msg("Error creating ArcadeLauncher.bat!\n");
		return;
	}

	std::string query = "steam://nav/games/grid";
	if (query.find("\"") == std::string::npos)
	{
		std::string goodQuery = "\"" + query + "\"";
		filesystem->FPrintf(launch_file, "START \"Launching web browser...\" %s", goodQuery.c_str());
		filesystem->Close(launch_file);
		system("Arcade_Launcher.bat");
	}
	else
		DevMsg("ERROR: Invalid URL detected.  Cannot have quotes in it.\n");
		*/
}

void C_AnarchyManager::CreateArcadeCrosshair()
{
	ArcadeCrosshair->Create(enginevgui->GetPanel(PANEL_CLIENTDLL));// PANEL_INGAMESCREENS));// PANEL_GAMEDLL));
	m_pShouldShowCrosshairConVar->SetValue(true);
}

void C_AnarchyManager::DestroyArcadeCrosshair()
{
	ArcadeCrosshair->Destroy();
	m_pShouldShowCrosshairConVar->SetValue(false);
}

/*
void C_AnarchyManager::OnWebManagerReady()
{
	C_WebTab* pHudWebTab = m_pWebManager->GetHudWebTab();
	g_pAnarchyManager->GetWebManager()->SelectWebTab(pHudWebTab);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true);

	unsigned int uCount;
	std::string num;
	
	// And continue starting up
	uCount = m_pMetaverseManager->LoadAllLocalTypes();
	num = VarArgs("%u", uCount);
	pHudWebTab->AddHudLoadingMessage("progress", "", "Loading Types", "locallibrarytypes", "0", num, num);

	 //= m_pMetaverseManager->LoadAllLocalTypes();
	//std::string num = VarArgs("%u", uItemCount);
//	pHudWebTab->AddHudLoadingMessage("progress", "", "Loading Types", "locallibrarytypes", "0", num, num);

	uCount = m_pMetaverseManager->LoadAllLocalModels();
	num = VarArgs("%u", uCount);
	pHudWebTab->AddHudLoadingMessage("progress", "", "Loading Models", "locallibrarymodels", "0", num, num);
	
	//uItemCount = m_pMetaverseManager->LoadAllLocalApps();

	// load ALL local apps
	KeyValues* app = m_pMetaverseManager->LoadFirstLocalApp("MOD");
	if (app)
		pHudWebTab->AddHudLoadingMessage("progress", "", "Loading Apps", "locallibraryapps", "", "", "+", "loadNextLocalAppCallback");
	else
		this->OnLoadAllLocalAppsComplete();

}
*/

bool C_AnarchyManager::OnSteamBrowserCallback(unsigned int unHandle)
{
	/*
	C_SteamBrowserInstance* pInstance = m_pSteamBrowserManager->FindDefunctInstance(unHandle);
	if (pInstance)
	{
		g_pAnarchyManager->GetSteamBrowserManager()->DestroyDefunctInstance(pInstance);
		return false;
	}
	*/

	return true;
}

unsigned int DetectAllModelsRecursiveThreaded(const char* folder, importInfo_t* pInfo)
{
	KeyValues* pSearchInfoKV = new KeyValues("search");
	unsigned int count = 0;
	FileFindHandle_t hFileSearch;
	std::string composedFile;
	std::string modelFile;
	KeyValues* pEntry;
	KeyValues* pModel;
	KeyValues* modelInfo;
	std::string buf;
	const char* potentialFile = g_pFullFileSystem->FindFirstEx(VarArgs("%s\\*", folder), "GAME", &hFileSearch);
	while (pInfo->status == AAIMPORTSTATUS_WORKING && potentialFile)
	{
		if (!Q_strcmp(potentialFile, ".") || !Q_strcmp(potentialFile, ".."))
		{
			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}

		composedFile = std::string(folder) + "\\" + std::string(potentialFile);

		if (g_pFullFileSystem->FindIsDirectory(hFileSearch))
		{
			count += DetectAllModelsRecursiveThreaded(composedFile.c_str(), pInfo);
			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}
		else
		{
			if (V_GetFileExtension(potentialFile) && !Q_stricmp(V_GetFileExtension(potentialFile), "mdl"))
			{
				pInfo->data.push_back(composedFile);
				count++;
				pInfo->count++;
			}

			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}
	}

	g_pFullFileSystem->FindClose(hFileSearch);
	pSearchInfoKV->deleteThis();
	return count;
}

unsigned SimpleFileScan(void *params)
{
	importInfo_t* pInfo = (importInfo_t*)params;
	if (pInfo->status != AAIMPORTSTATUS_WAITING_TO_START)
		return 0;

	pInfo->status = AAIMPORTSTATUS_WORKING;
	unsigned int detectedCount = DetectAllModelsRecursiveThreaded("models", pInfo);

	if (pInfo->status == AAIMPORTSTATUS_WORKING)
		pInfo->status = AAIMPORTSTATUS_COMPLETE;
	else
		pInfo->status = AAIMPORTSTATUS_ABORTED;

	return 0;
}

void C_AnarchyManager::ThrowEarlyError(const char* msg)
{
	vgui::VPANEL gameParent = enginevgui->GetPanel(PANEL_TOOLS);
	EarlyError->Create(gameParent, msg);
}

void C_AnarchyManager::DetectAllModelsThreaded()
{
	if (m_pImportInfo->status != AAIMPORTSTATUS_NONE)
	{
		DevMsg("ERROR: Threaded scan already in process!\n");
		return;
	}

	//m_pImportInfo->count = 0;
	//m_pImportInfo->data.clear();
	m_pImportInfo->status = AAIMPORTSTATUS_WAITING_TO_START;
	m_pImportInfo->type = AAIMPORT_MODELS;
	m_uPreviousImportCount = 0;

	CreateSimpleThread(SimpleFileScan, m_pImportInfo);
}

void C_AnarchyManager::ManageImportScans()
{
	bool bNeedsClear = false;
	if (m_pImportInfo->status == AAIMPORTSTATUS_WORKING)
	{
		unsigned int nextCount = m_pImportInfo->count;
		if (m_uPreviousImportCount != nextCount)
		{
			if (m_uPreviousImportCount < nextCount)
			{
				C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Detecting Models", "detectmodels", "", "", VarArgs("%u", nextCount), "");
			}

			m_uPreviousImportCount = nextCount;
		}
	}
	else if (m_pImportInfo->status == AAIMPORTSTATUS_COMPLETE)
	{
		m_pImportInfo->status = AAIMPORTSTATUS_WAITING_FOR_PROCESSING;
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Detecting Models", "detectmodels", "", "", VarArgs("%u", m_pImportInfo->count), "");

		//m_pImportInfo->status = AAIMPORTSTATUS_PROCESSING;
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Models", "processmodels", "", VarArgs("%u", m_pImportInfo->count), "0", "processNextModelCallback");
		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "New Models Detected", "newmodels", "", "", "0", "");
		//pHudBrowserInstance->AddHudLoadingMessage("", "", "Processing New Models", "processmodels", "", "", "", "processAllModelsCallback");
	}
	else if (m_pImportInfo->status == AAIMPORTSTATUS_ABORTED)
	{
		m_pMetaverseManager->CommitTransaction();

		bNeedsClear = true;

		//m_pImportInfo->status = AAIMPORTSTATUS_WAITING_FOR_PROCESSING;
		//C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Detecting Models", "detectmodels", "", "", VarArgs("%u", m_pImportInfo->data.size()), "");
	}

	if (bNeedsClear)
	{
		m_pImportInfo->count = 0;
		m_pImportInfo->data.clear();
		m_pImportInfo->duplicates.clear();
		m_pImportInfo->status = AAIMPORTSTATUS_NONE;
		m_pImportInfo->type = AAIMPORT_NONE;
	}
}

void C_AnarchyManager::ProcessNextModel()
{
	unsigned int index;
	if (m_pImportInfo->status == AAIMPORTSTATUS_WAITING_FOR_PROCESSING)
	{
		m_pImportInfo->status = AAIMPORTSTATUS_PROCESSING;

		//m_uLastProcessedModelIndex = 0;
		m_uValidProcessedModelCount = 0;
		index = 0;
		m_uProcessBatchSize = cvar->FindVar("process_batch_size")->GetInt();
		m_uProcessCurrentCycle = 0;
	}
	else if (m_pImportInfo->status == AAIMPORTSTATUS_PROCESSING)
		index = m_uLastProcessedModelIndex + 1;
	else
	{
		DevMsg("ERROR: Invalid state for model processing. Aborting.\n");
		return;
	}

	if (index < m_pImportInfo->data.size())
	{
		if (m_pMetaverseManager->GetLibraryModel(this->GenerateLegacyHash(m_pImportInfo->data[index].c_str())))
			m_pImportInfo->duplicates.push_back(index);

		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		m_uLastProcessedModelIndex = index;
		m_uProcessCurrentCycle++;

		if (m_uProcessCurrentCycle == 0 || m_uProcessCurrentCycle < m_uProcessBatchSize)
		{
			this->ProcessNextModel();
			return;
		}
		else
		{
			m_uProcessCurrentCycle = 0;
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Models", "processmodels", "", VarArgs("%u", m_pImportInfo->count), VarArgs("%u", m_uLastProcessedModelIndex+1), "processNextModelCallback");
		}
	}
	else
	{
		m_pImportInfo->status = AAIMPORTSTATUS_WAITING_FOR_ADDING;

		m_pMetaverseManager->BeginTransaction();
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Models", "processmodels", "", VarArgs("%u", m_pImportInfo->count), VarArgs("%u", m_uLastProcessedModelIndex+1), "");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding New Models", "addingmodels", "", VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), "0", "addNextModelCallback");
	}
}

void C_AnarchyManager::AddNextModel()
{
	unsigned int index;
	if (m_pImportInfo->status == AAIMPORTSTATUS_WAITING_FOR_ADDING)
	{
		m_pImportInfo->status = AAIMPORTSTATUS_ADDING;

		m_uLastProcessedModelIndex = 0;
		m_uValidProcessedModelCount = 0;	// NOTE: This is overloaded during Adding Models (which comes AFTER the Processing Models) to hold the INDEX of the NEXT DUPLICATE to test against
		index = 0;
		//m_uProcessBatchSize = cvar->FindVar("process_batch_size")->GetInt();
		//m_uProcessCurrentCycle = 0;
	}
	else if (m_pImportInfo->status == AAIMPORTSTATUS_ADDING)
		index = m_uLastProcessedModelIndex + 1;
	else
	{
		DevMsg("ERROR: Invalid state for model adding. Aborting.\n");
		return;
	}

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	if (index < m_pImportInfo->data.size())
	{
		m_uLastProcessedModelIndex = index;

		//unsigned int uCurrentDuplicateCheckIndex = m_uValidProcessedModelCount;
		if (m_uValidProcessedModelCount < m_pImportInfo->duplicates.size() && m_pImportInfo->duplicates[m_uValidProcessedModelCount] == index)
		{
			m_uValidProcessedModelCount++;
			this->AddNextModel();
			return;
			//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding New Models", "addingmodels", "", VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), "+0", "addNextModelCallback");
		}
		else
		{
			m_pMetaverseManager->ProcessModel(m_pImportInfo->data[index]);

			m_uProcessCurrentCycle++;

			//if (m_uProcessCurrentCycle == 0 || m_uProcessCurrentCycle < m_uProcessBatchSize)
			//{
			//	this->AddNextModel();
			//	return;
			//}
			//else
			//{
			//	m_uProcessCurrentCycle = 0;
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding New Models", "addingmodels", "", VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), "+", "addNextModelCallback");
				//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding New Models", "addingmodels", "", VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), VarArgs("+%u", m_uProcessBatchSize), "addNextModelCallback");
			//}
		}
	}
	else
	{
		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Adding New Models", "addingmodels", "", VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), VarArgs("%u", m_pImportInfo->count - m_pImportInfo->duplicates.size()), "");
		pHudBrowserInstance->AddHudLoadingMessage("", "", "Finished Importing Models", "processmodelscomplete", "", "", "", "");
		g_pAnarchyManager->AddToastMessage(VarArgs("Models Imported (%u)", m_pImportInfo->count - m_pImportInfo->duplicates.size()));

		m_pMetaverseManager->CommitTransaction();

		std::vector<std::string> args;
		pHudBrowserInstance->DispatchJavaScriptMethod("eventListener", "doneDetectingModels", args);

		m_uLastProcessedModelIndex = 0;
		m_uValidProcessedModelCount = 0;

		m_pImportInfo->count = 0;
		m_pImportInfo->data.clear();
		m_pImportInfo->duplicates.clear();
		m_pImportInfo->status = AAIMPORTSTATUS_NONE;
		m_pImportInfo->type = AAIMPORT_NONE;
	}
}

void C_AnarchyManager::ManageHUDLabels()
{
	ToastSlate->ManageHUDLabels();
}

Color TransformH2(
	const Color &in,  // color to transform
	float H
	)
{
	float U = cos(H*M_PI / 180);
	float W = sin(H*M_PI / 180);

	int r, g, b, a;
	in.GetColor(r, g, b, a);

	Color ret;
	ret.SetColor((.299 + .701*U + .168*W)*r
		+ (.587 - .587*U + .330*W)*g
		+ (.114 - .114*U - .497*W)*b,
		(.299 - .299*U - .328*W)*r
		+ (.587 + .413*U + .035*W)*g
		+ (.114 - .114*U + .292*W)*b,
		(.299 - .3*U + 1.25*W)*r
		+ (.587 - .588*U - 1.05*W)*g
		+ (.114 + .886*U - .203*W)*b);
	return ret;
}

/*
void C_AnarchyManager::ManageAudioStrips()
{
	if (!m_pAudioStripMaterial)
		return;

	bool bFoundColor;
	IMaterialVar* pColorVar = m_pAudioStripMaterial->FindVar("$color2", &bFoundColor);
	if (bFoundColor)
	{
		if (!m_pAVRConVar)
			m_pAVRConVar = cvar->FindVar("avr");

		if (m_pAVRConVar->GetBool())
		{
			if (!m_pAVRAmpConVar)
				m_pAVRAmpConVar = cvar->FindVar("avramp");

			float peak = g_pAnarchyManager->GetAudioPeakValue() * m_pAVRAmpConVar->GetFloat();
			peak += 0.5;
			if (peak > 1.0f)
				peak = 1.0f;
			else if (peak < 0.2f)
				peak = 0.2f;

			int avrValue = m_pAVRConVar->GetInt();
			if (avrValue == 1 || avrValue == 2)
			{
				float hue = g_pAnarchyManager->GetHueShifter();
				m_hueColor.SetColor(255, 0, 0);
				m_hueColor = TransformH2(m_hueColor, hue);
			}
			else if (avrValue == 3)
				m_hueColor.SetColor(255, 0, 0);
			else if (avrValue == 4)
				m_hueColor.SetColor(0, 255, 0);
			else if (avrValue == 5)
				m_hueColor.SetColor(0, 0, 255);

			pColorVar->SetVecValue((m_hueColor.r() / 255.0) * peak, (m_hueColor.g() / 255.0) * peak, (m_hueColor.b() / 255.0) * peak);
		}
	}
}
*/


void C_AnarchyManager::CreateItemMenu(std::string fileLocation)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	this->GetMetaverseManager()->SetLibraryBrowserContext("items", "", "", "");

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl(VarArgs("asset://ui/createItem.html?fileLocation=%s", this->encodeURIComponent(fileLocation)));
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::CreateJSONItemMenu(std::string json)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	this->GetMetaverseManager()->SetLibraryBrowserContext("items", "", "", "");

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl(VarArgs("asset://ui/createJSONItem.html?json=%s", this->encodeURIComponent(json)));
	m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
}

void C_AnarchyManager::ManageWindow()
{
	if (m_bAutoRes)
	{
		const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
		if (!config.Windowed() || m_fNextWindowManage == 0)
		{
			int myWidth = config.m_VideoMode.m_Width;
			int myHeight = config.m_VideoMode.m_Height;

			RECT rcDesktop;
			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0))
			{
				RECT rcClient;
				RECT rcWind;

				GetClientRect(m_hwnd, &rcClient);
				GetWindowRect(m_hwnd, &rcWind);

				myWidth = rcDesktop.right - rcDesktop.left;
				myHeight = rcDesktop.bottom - rcDesktop.top;
			}

			Msg("FORCING WINDOWED MODE...\n");
			std::string myResCommand = VarArgs("mat_setvideomode %i %i 1;\n", myWidth, myHeight);
			engine->ClientCmd(myResCommand.c_str());
		}
		else
		{
			int desktopWidth = 0;
			int desktopHeight = 0;
			int clientWidth = 0;
			int clientHeight = 0;
			int windowWidth = 0;
			int windowHeight = 0;
			int borderWidth = 0;
			int borderTop = 0;

			//int flags = SWP_NOMOVE | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
			int flags = SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOREPOSITION;

			//int flags = 0x0002 | 0x0020 | 0x0001 | 0x0004 | 0x0200;
			bool BringFlashToFront = false;

			// Will we fit in the desktop?
			RECT rcDesktop;
			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0))
			{
				RECT rcClient;
				RECT rcWind;

				GetClientRect(m_hwnd, &rcClient);
				GetWindowRect(m_hwnd, &rcWind);

				desktopWidth = rcDesktop.right - rcDesktop.left;
				desktopHeight = rcDesktop.bottom - rcDesktop.top;
				clientWidth = rcClient.right - rcClient.left;
				clientHeight = rcClient.bottom - rcClient.top;
				windowWidth = rcWind.right - rcWind.left;
				windowHeight = rcWind.bottom - rcWind.top;

				borderWidth = (windowWidth - clientWidth) / 2;
				borderTop = (windowHeight - clientHeight) - borderWidth;

				if (windowWidth > desktopWidth && windowHeight > desktopHeight)
					flags &= ~0x0002;
			}

			// If we have moved...
			RECT windowRect;
			if (!(flags & SWP_NOMOVE) && GetWindowRect(m_hwnd, &windowRect) && (windowRect.top != -borderTop || windowRect.left != -borderWidth))
				SetWindowPos(m_hwnd, null, -borderWidth, -borderTop, 0, 0, flags);
		}
	}

	float fManageWindowInterval = 8.0;
	m_fNextWindowManage = engine->Time() + fManageWindowInterval;
}

void C_AnarchyManager::MainMenuLoaded()
{
	if (!m_bLevelInitialized || !engine->IsInGame())
	{
		steamapicontext->SteamFriends()->SetRichPresence("objectcount", "0");
		steamapicontext->SteamFriends()->SetRichPresence("mapname", "None");
		steamapicontext->SteamFriends()->SetRichPresence("steam_display", "#Status_AtMainMenu");
	}
}

void C_AnarchyManager::GetAllFriends(std::vector<aaSteamFriend_t*>& steamFriends)
{
	int nFriends = steamapicontext->SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
	if (nFriends == -1)
	{
		printf("GetFriendCount returned -1, the current user is not logged in.\n");
		// We always recommend resetting to 0 just in case you were to do something like allocate
		// an array with this value, or loop over it in a way that doesn't take into the -1 into account.
		nFriends = 0;
	}

	aaSteamFriend_t* pSteamFriend;
	for (int i = 0; i < nFriends; ++i)
	{
		CSteamID friendSteamID = steamapicontext->SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
		const char *friendName = steamapicontext->SteamFriends()->GetFriendPersonaName(friendSteamID);
		pSteamFriend = new aaSteamFriend_t();
		pSteamFriend->id = friendSteamID.ConvertToUint64();
		pSteamFriend->name = friendName;
		steamFriends.push_back(pSteamFriend);
	}
}
/*
aaSteamFriendRichPresence_t* C_AnarchyManager::GetFriendRichPresence(uint64 steamId)
{
	CSteamID id;
	id.SetFromUint64(steamId);

	aaSteamFriendRichPresence_t* steamFriendRichPresence = new aaSteamFriendRichPresence_t();

	int iKeyCount = steamapicontext->SteamFriends()->GetFriendRichPresenceKeyCount(id);
	for (unsigned int i = 0; i < iKeyCount; i++)
	{
		const char* keyName = steamapicontext->SteamFriends()->GetFriendRichPresenceKeyByIndex(id, i);

	}


	return steamFriendRichPresence;
}
*/

void C_AnarchyManager::FindNextAttractCamera()
{
	if (m_pCabinetAttractModeActiveConVar->GetBool())	// Abort if we are already in a cabinet attract mode.
		return;

	m_flNextAttractCameraTime = -1.0f;

	// find where we want the camera...
	std::vector<KeyValues*> screenshots;
	std::string instanceId = "id" + this->GetInstanceId();

	if (cvar->FindVar("screenshot_multiverse")->GetBool())
	{
		std::string mapId = "id" + this->GetInstanceManager()->GetCurrentInstance()->mapId;
		this->GetMetaverseManager()->GetAllMapScreenshots(screenshots, mapId);
	}
	else {
		this->GetMetaverseManager()->GetAllScreenshotsForInstance(screenshots, instanceId);
	}

	unsigned int uNumScreenshots = screenshots.size();

	if (uNumScreenshots > 0)
	{
		// Find the best screenshot to go to
		int iScreenshotIndex = -1;

		// Find our current screenshot...
		int iCurrentScreenshotIndex = -1;
		if (m_attractModeScreenshotId != "")
		{
			for (unsigned int i = 0; i < uNumScreenshots; i++)
			{
				if (!Q_strcmp(screenshots[i]->GetString("id"), m_attractModeScreenshotId.c_str()))
				{
					iCurrentScreenshotIndex = i;
					break;
				}
			}
		}

		// Just increment which screenshot we fly to for now...
		if (iCurrentScreenshotIndex == -1)
			iScreenshotIndex = 0;
		else if (iCurrentScreenshotIndex + 1 < uNumScreenshots)
			iScreenshotIndex = iCurrentScreenshotIndex + 1;
		else
			iScreenshotIndex = 0;

		// Now fly there...
		KeyValues* pScreenshotKV = screenshots[iScreenshotIndex];
		KeyValues* pScreenshotCameraKV = pScreenshotKV->FindKey("camera");
		m_attractModeScreenshotId = pScreenshotKV->GetString("id");


		std::string position = pScreenshotCameraKV->GetString("position");
		int iTransitionType = 0;

		Vector goodPosition;
		UTIL_StringToVector(goodPosition.Base(), position.c_str());


		trace_t tr;
		UTIL_TraceLine(MainViewOrigin(), goodPosition, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
		if (tr.fraction >= 0.9)
			iTransitionType = 1;
		else
			iTransitionType = 2;

		if (cvar->FindVar("attract_mode_wipe")->GetBool())
		{
			iTransitionType = 2;
			this->DoComparisonRender();
		}


		engine->ClientCmd(VarArgs("set_attract_mode_transform %s %s %i;", position.c_str(), pScreenshotCameraKV->GetString("rotation"), iTransitionType));
		cvar->FindVar("attract_mode_active")->SetValue(1);
	}
	else
		m_attractModeScreenshotId = "";
}

// FIXME: This should be merged into FindNextAttractCamera with a direction variable.
void C_AnarchyManager::FindPreviousAttractCamera()
{
	if (m_pCabinetAttractModeActiveConVar->GetBool())	// Abort if we are already in a cabinet attract mode.
		return;

	m_flNextAttractCameraTime = -1.0f;

	// find where we want the camera...
	std::vector<KeyValues*> screenshots;
	//std::string mapId = "id" + this->GetInstanceManager()->GetCurrentInstance()->mapId;
	std::string instanceId = "id" + this->GetInstanceId();

	if (cvar->FindVar("screenshot_multiverse")->GetBool())
	{
		std::string mapId = "id" + this->GetInstanceManager()->GetCurrentInstance()->mapId;
		this->GetMetaverseManager()->GetAllMapScreenshots(screenshots, mapId);
	}
	else {
		this->GetMetaverseManager()->GetAllScreenshotsForInstance(screenshots, instanceId);
	}

	unsigned int uNumScreenshots = screenshots.size();

	if (uNumScreenshots > 0)
	{
		// Find the best screenshot to go to
		int iScreenshotIndex = -1;

		// Find our current screenshot...
		int iCurrentScreenshotIndex = -1;
		if (m_attractModeScreenshotId != "")
		{
			for (unsigned int i = 0; i < uNumScreenshots; i++)
			{
				if (!Q_strcmp(screenshots[i]->GetString("id"), m_attractModeScreenshotId.c_str()))
				{
					iCurrentScreenshotIndex = i;
					break;
				}
			}
		}

		// Just increment which screenshot we fly to for now...
		if (iCurrentScreenshotIndex == -1)
			iScreenshotIndex = uNumScreenshots - 1;
		else if (iCurrentScreenshotIndex - 1 >= 0)
			iScreenshotIndex = iCurrentScreenshotIndex - 1;
		else
			iScreenshotIndex = uNumScreenshots - 1;

		// Now fly there...
		//DevMsg("Screenshot Index is: %i\n", iScreenshotIndex);
		KeyValues* pScreenshotKV = screenshots[iScreenshotIndex];
		KeyValues* pScreenshotCameraKV = pScreenshotKV->FindKey("camera");
		m_attractModeScreenshotId = pScreenshotKV->GetString("id");


		std::string position = pScreenshotCameraKV->GetString("position");
		int iTransitionType = 0;

		Vector goodPosition;
		UTIL_StringToVector(goodPosition.Base(), position.c_str());
		//if (engine->IsBoxInViewCluster(goodPosition + Vector(1, 1, 1), goodPosition + Vector(-1, -1, -1)))


		trace_t tr;
		//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		UTIL_TraceLine(MainViewOrigin(), goodPosition, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
		if (tr.fraction >= 0.9)
			iTransitionType = 1;
		else
			iTransitionType = 2;

		if (cvar->FindVar("attract_mode_wipe")->GetBool())
		{
			iTransitionType = 2;
			this->DoComparisonRender();
		}

		engine->ClientCmd(VarArgs("set_attract_mode_transform %s %s %i;", position.c_str(), pScreenshotCameraKV->GetString("rotation"), iTransitionType));
		cvar->FindVar("attract_mode_active")->SetValue(1);
	}
	else
		m_attractModeScreenshotId = "";
}

void C_AnarchyManager::AttractCameraReached()
{
	// Find the next screenshot.
	// TODO: Make this actually check for line-of-sight & distance to other screenshots to find the best one to go to next.

	// For now, just grab the next one...
	m_flNextAttractCameraTime = engine->Time() + cvar->FindVar("attract_mode_hold")->GetFloat();//gpGlobals->curtime
}

/*
bool CheckIfImageExistsInCache(std::string fieldVal);
bool C_AwesomiumBrowserInstance::CheckIfImageExistsInCache(std::string fieldVal)
{
	std::string filename = std::string("cache/urls/") + std::string(g_pAnarchyManager->GenerateLegacyHash(fieldVal.c_str())) + ".jpg";
	return g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH");
}
*/

// For when the user transitions to a different type of auto-camera directly from showcase camera...
void C_AnarchyManager::ClearAttractMode()
{
	ConVar* pAttractModeActiveConVar = cvar->FindVar("attract_mode_active");
	if (pAttractModeActiveConVar->GetBool())
	{
		engine->ClientCmd("end_attract_mode");
		cvar->FindVar("attract_mode_active")->SetValue(0);
		m_flNextAttractCameraTime = -1.0f;
	}
}

void C_AnarchyManager::ToggleAttractMode()
{
	if (m_pCabinetAttractModeActiveConVar->GetBool())	// Abort if we are already in a cabinet attract mode.
		return;

	if (m_pAttractModeActiveConVar->GetBool())
	{
		engine->ClientCmd("end_attract_mode");
		cvar->FindVar("attract_mode_active")->SetValue(0);
		m_flNextAttractCameraTime = -1.0f;
	}
	else
	{
		// find where we want the camera...
		std::vector<KeyValues*> screenshots;

		std::string instanceId = "id" + this->GetInstanceId();
		if (cvar->FindVar("screenshot_multiverse")->GetBool())
		{
			std::string mapId = "id" + this->GetInstanceManager()->GetCurrentInstance()->mapId;
			this->GetMetaverseManager()->GetAllMapScreenshots(screenshots, mapId);
		}
		else {
			this->GetMetaverseManager()->GetAllScreenshotsForInstance(screenshots, instanceId);
		}

		// Find the best screenshot to go to
		int iScreenshotIndex = -1;

		float flBestDist = -1;
		unsigned int uBestIndex = -1;

		std::string position;
		Vector goodPosition;
		KeyValues* pScreenshotKV;
		KeyValues* pScreenshotCameraKV;
		unsigned int max = screenshots.size();
		float flTestDist;
		for (unsigned int i = 0; i < max; i++)
		{
			pScreenshotKV = screenshots[i];
			pScreenshotCameraKV = pScreenshotKV->FindKey("camera");
			position = pScreenshotCameraKV->GetString("position");
			UTIL_StringToVector(goodPosition.Base(), position.c_str());

			flTestDist = goodPosition.DistTo(MainViewOrigin());
			if (flBestDist == -1 || flTestDist < flBestDist)
			{
				uBestIndex = i;
				flBestDist = flTestDist;
			}
		}

		iScreenshotIndex = uBestIndex;
		//iScreenshotIndex = (screenshots.size() > 0) ? 0 : -1;//(screenshots.size() > 0) ? floor(random->RandomFloat() * screenshots.size()) : -1;

		// AAAannnnnnddd thhhhhhheeeeennnnnnnnnnnnn
		if (iScreenshotIndex >= 0 && iScreenshotIndex < screenshots.size())
		{
			pScreenshotKV = screenshots[iScreenshotIndex];
			pScreenshotCameraKV = pScreenshotKV->FindKey("camera");
			m_attractModeScreenshotId = pScreenshotKV->GetString("id");

			position = pScreenshotCameraKV->GetString("position");
			std::string rotation = pScreenshotCameraKV->GetString("rotation");

			if (position != "" && rotation != "")
			{
				/*
				// Check if the camera has line of sight to the destination.
				trace_t tr;
				C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
				Vector forward;
				pPlayer->EyeVectors(&forward);

				UTIL_

				UTIL_TraceLine(goodPosition, pPlayer-> ->GetAbsOrigin(), MASK_BLOCKLOS, pPlayer, COLLISION_GROUP_NONE, &tr);
				*/

				int iTransitionType = 0;

				Vector goodPosition;
				UTIL_StringToVector(goodPosition.Base(), position.c_str());
				//if (engine->IsBoxInViewCluster(goodPosition + Vector(1, 1, 1), goodPosition + Vector(-1, -1, -1)))
				
				trace_t tr;
				//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
				UTIL_TraceLine(MainViewOrigin(), goodPosition, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
				if (tr.fraction >= 0.9)
					iTransitionType = 1;
				else
					iTransitionType = 2;

				if (cvar->FindVar("attract_mode_wipe")->GetBool())
				{
					iTransitionType = 2;
					this->DoComparisonRender();
				}

				std::string cmd = VarArgs("set_attract_mode_transform %s %s %i;", position.c_str(), rotation.c_str(), iTransitionType);
				engine->ClientCmd(cmd.c_str());
				cvar->FindVar("attract_mode_active")->SetValue(1);
			}
			else
			{
				DevMsg("ERROR: Invalid screenshot.\n");
			}
		}
	}
}

std::string C_AnarchyManager::GetMaterialUnderCrosshair()
{
	if (!g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance())
		return "";

	std::string materialName = "";
	trace_t tr;
	Vector forward;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->EyeVectors(&forward);
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	if (tr.DidHit())
	{
		if (Q_strcmp(tr.surface.name, "**studio**"))
		{
			IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
			if (!pMaterial || pMaterial->IsErrorMaterial())
				pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

			if (pMaterial && !pMaterial->IsErrorMaterial())
				materialName = pMaterial->GetName();
		}
	}

	return materialName;
}

std::string C_AnarchyManager::GetModelUnderCrosshair()
{
	if (!g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance())
		return "";

	std::string modelName = "";

	int iIndex = g_pAnarchyManager->GetSelectorTraceEntityIndex();
	C_BaseEntity* pEntity = C_BaseEntity::Instance(iIndex);
	if (pEntity)
	{
		const model_t* TheModel = pEntity->GetModel();
		modelName = modelinfo->GetModelName(TheModel);
	}

	return modelName;
}

void C_AnarchyManager::SetSocialMode(bool bValue)
{
	if (bValue && !m_bSocialMode)
	{
		m_bSocialMode = true;
		m_pMetaverseManager->SocialJoin();
	}
	else if (!bValue && m_bSocialMode)
	{
		m_bSocialMode = false;
		m_pMetaverseManager->SocialLeave();
	}
}

bool C_AnarchyManager::VRUpdate()
{
	if (this->IsPaused() || !this->IsVRActive())
		return false;

#ifdef VR_ALLOWED
	g_pAnarchyManager->VRFrameReady();
	g_pAnarchyManager->VRFrameBegin();

	float flHead[4][4];
	float flLeftController[4][4];
	float flRightController[4][4];
	hmdGetTrackingData(&flHead[0], &flLeftController[0], &flRightController[0]);

	VMatrix headMatrix;
	memcpy(&headMatrix, flHead, sizeof(headMatrix));
	headMatrix = g_pAnarchyManager->SMMatrixToVMatrix(headMatrix.Base());

	Vector translation = headMatrix.GetTranslation();
	if (translation.x != -0.0f || translation.y != -0.0f || translation.z != -64.0f)
	{
		m_VRHeadMatrix = headMatrix;

		if(m_pRagdollShortcut)
		{
			this->RagdollInfo(m_pRagdollShortcut);
		}

		memcpy(&m_VRLeftControllerMatrix, flLeftController, sizeof(m_VRLeftControllerMatrix));
		m_VRLeftControllerMatrix = g_pAnarchyManager->SMMatrixToVMatrix(m_VRLeftControllerMatrix.Base());

		memcpy(&m_VRRightControllerMatrix, flRightController, sizeof(m_VRRightControllerMatrix));
		m_VRRightControllerMatrix = g_pAnarchyManager->SMMatrixToVMatrix(m_VRRightControllerMatrix.Base());

		if (this->IsHandTrackingActive())
		{
			///////////////////////////
			// Oculus Touch Controls //
			///////////////////////////
			if (m_iVRAPI == 2)
			{
				/*
					VR CONTROLS
					==============================
					- Hold RIGHT BUMPER to pick up objects.
					- Release RIGHT BUMPER to cance....
					*/

				bool bIsRightBumperDown = hmdGetButtonIsPressed(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual);
				bool bIsRightBumperDownFrame = hmdGetButtonIsDownFrame(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual);

				// RIGHT TRIGGER
				if (hmdGetButtonIsDownFrame(ButtonsList::right_Trigger, ControllerTypes::controllerType_Virtual))
				{
					if (!this->GetMetaverseManager()->GetSpawningObjectEntity() && g_pAnarchyManager->GetInputManager()->GetInputMode())// && g_pAnarchyManager->GetInputManager()->GetFullscreenMode())
					{
						//DevMsg("Click down!");
						m_pInputManager->MousePress(MOUSE_LEFT);
					}
					else if (!bIsRightBumperDown)
					{
						if (g_pAnarchyManager->IsVRActive() && !this->GetVRHand(0))
							this->CreateVRHands();

						engine->ClientCmd("+attack");
					}
					//else if (g_pAnarchyManager->GetInputManager()->GetInputMode())
				}
				else if (hmdGetButtonIsUpFrame(ButtonsList::right_Trigger, ControllerTypes::controllerType_Virtual))
				{
					if (!this->GetMetaverseManager()->GetSpawningObjectEntity() && g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetInputManager()->GetFullscreenMode())
					{
						m_pInputManager->MouseRelease(MOUSE_LEFT);
					}
					else if (!bIsRightBumperDown)
						engine->ClientCmd("-attack");
					else
					{
						C_PropShortcutEntity* pShortcut = this->GetMetaverseManager()->GetSpawningObjectEntity();
						if (pShortcut)
						{
							// finished positioning & choosing model, ie: changes confirmed
							DevMsg("CHANGES CONFIRMED\n");
							C_AwesomiumBrowserInstance* pHubInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

							//std::string code = "saveTransformChangesCallback();";
							//pHubInstance->GetWebView()->ExecuteJavascript(WSLit(code.c_str()), WSLit(""));
							g_pAnarchyManager->DeactivateObjectPlacementMode(true);

							m_pInstanceManager->ApplyChanges(pShortcut);
							m_pMetaverseManager->SendObjectUpdate(pShortcut);
							m_pInstanceManager->ResetTransform();
						}
						else if (g_pAnarchyManager->GetInputManager()->GetInputMode())
						{
							//DevMsg("Click up!");
							m_pInputManager->MouseRelease(MOUSE_LEFT);
						}
					}
				}

				if (pVRHandRight && pVRHandLeft)
				{
					// RIGHT BUMPER
					if (bIsRightBumperDownFrame)
					{
						if (!this->GetInputManager()->GetInputMode() && !this->GetSelectedEntity())
						{
							//trace_t tr;
							//this->SelectorTraceLine(tr);

							//pEntity = C_BaseEntity::Instance();
							//g_pAnarchyManager->GetSelect

							C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
							if (pBaseEntity)
							{
								C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
								if (pShortcut)
								{
									float flScale = pShortcut->GetModelScale();

									Vector origin = pShortcut->GetAbsOrigin();
									QAngle angles = pShortcut->GetAbsAngles();
									g_pAnarchyManager->GetInstanceManager()->AdjustObjectRot(0, 180, 0);// angles.x, angles.y, angles.z);
									g_pAnarchyManager->GetInstanceManager()->AdjustObjectOffset(0, 0, 0);// origin.x, origin.y, origin.z);
									g_pAnarchyManager->GetInstanceManager()->AdjustObjectScale(flScale);

									bool bOldGamepadInputMode = g_pAnarchyManager->GetInputManager()->IsGamepadInputMode();
									g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
									g_pAnarchyManager->ActivateObjectPlacementMode(pShortcut, "move", bOldGamepadInputMode);
								}
							}
						}
						else
							engine->ClientCmd("+attack2");
					}
					else if (hmdGetButtonIsUpFrame(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual))
					{
						C_PropShortcutEntity* pShortcut = this->GetMetaverseManager()->GetSpawningObjectEntity();
						if (pShortcut)
						{
							g_pAnarchyManager->DeactivateObjectPlacementMode(false);

							// undo changes AND cancel
							g_pAnarchyManager->DeactivateObjectPlacementMode(false);	// FIXME: probably does absolutely NOTHING calling it twice, but make sure before removing this line.
							//engine->ClientCmd("-attack2");
						}
						else if (this->GetSelectedEntity())
						{
							engine->ClientCmd("-attack2");
						}
					}

					// RIGHT BUTTON A
					if (hmdGetButtonIsDownFrame(ButtonsList::right_ButtonA, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("+use");
					else if (hmdGetButtonIsUpFrame(ButtonsList::right_ButtonA, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("-use");

					// RIGHT BUTTON B
					if (hmdGetButtonIsDownFrame(ButtonsList::right_ButtonB, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("+jump");
					else if (hmdGetButtonIsUpFrame(ButtonsList::right_ButtonB, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("-jump");

					// RIGHT JOY CLICK
					//if (hmdGetButtonIsDownFrame(ButtonsList::right_Pad, ControllerTypes::controllerType_Virtual))
					//			engine->ClientCmd("+task_menu");
					if (hmdGetButtonIsUpFrame(ButtonsList::right_Pad, ControllerTypes::controllerType_Virtual))
					{
						if (g_pAnarchyManager->GetSelectedEntity())
							g_pAnarchyManager->TaskRemember();

						C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
						pHudBrowserInstance->SetUrl("asset://ui/tabMenu.html");
						m_pInputManager->ActivateInputMode(true, false);	// pausing the game due to a menu is now obsolete!
					}
					//engine->ClientCmd("-task_menu");

					// RIGHT MENU BUTTON
					if (hmdGetButtonIsUpFrame(ButtonsList::right_Menu, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("task_menu");

					// LEFT TRIGGER
					//if (hmdGetButtonIsDownFrame(ButtonsList::left_Trigger, ControllerTypes::controllerType_Virtual))
					//	engine->ClientCmd("invnext");
					if (hmdGetButtonIsUpFrame(ButtonsList::left_Trigger, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("invprev");// invnext");

					// LEFT BUMPER
					//if (hmdGetButtonIsDownFrame(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual))
					//	engine->ClientCmd("+attack2");

					if (hmdGetButtonIsDownFrame(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual))
					{
						if (bIsRightBumperDown)
						{
							C_PropShortcutEntity* pShortcut = this->GetMetaverseManager()->GetSpawningObjectEntity();
							if (pShortcut)
							{
								m_VRGestureLeft = this->GetVRHandMatrix(0);
								m_VRGestureRight = this->GetVRHandMatrix(1);
								m_flGestureValue = pShortcut->GetModelScale();
							}
						}
					}
					else if (hmdGetButtonIsUpFrame(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual))
					{
						C_PropShortcutEntity* pShortcut = this->GetMetaverseManager()->GetSpawningObjectEntity();
						if (pShortcut)
						{
							if (!bIsRightBumperDown && m_pInputManager->GetInputMode())
								engine->ClientCmd("invprev");
							else
							{
								m_VRGestureLeft.Identity();
								m_VRGestureRight.Identity();
							}
						}
					}

					// LEFT BUTTON A
					if (hmdGetButtonIsDownFrame(ButtonsList::left_ButtonA, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("+reload");
					else if (hmdGetButtonIsUpFrame(ButtonsList::left_ButtonA, ControllerTypes::controllerType_Virtual))
					{
						if (g_pAnarchyManager->GetInputManager()->GetFullscreenMode())
						{
							// handle escape if in fullscreen input mode (drop out of fullscreen mode)
							//if (!m_pInputManager->IsTempSelect())
							//{
							m_pInputManager->DeactivateInputMode(true);
							g_pAnarchyManager->HudStateNotify();
							m_pFreeMouseModeConVar->SetValue(false);
							//}
						}

						engine->ClientCmd("-reload");
					}

					// LEFT BUTTON B
					if (hmdGetButtonIsDownFrame(ButtonsList::left_ButtonB, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("+remote_control");
					else if (hmdGetButtonIsUpFrame(ButtonsList::left_ButtonB, ControllerTypes::controllerType_Virtual))
						engine->ClientCmd("-remote_control");

					// LEFT JOY CLICK
					//if (hmdGetButtonIsDownFrame(ButtonsList::left_Pad, ControllerTypes::controllerType_Virtual))
					//	engine->ClientCmd("+speed");
					if (hmdGetButtonIsUpFrame(ButtonsList::left_Pad, ControllerTypes::controllerType_Virtual))
					{
						////engine->ClientCmd("escape");//task_menu
						this->HandleUiToggle();
						//if (m_pInputManager->GetInputMode())
						//	m_pInputManager->DeactivateInputMode(true);
						//else
						//	engine->ClientCmd("main_menu");
						//engine->ClientCmd("task_menu");
						//engine->ClientCmd("-speed");
					}

					// LEFT MENU BUTTON
					//if (hmdGetButtonIsPressed(ButtonsList::left_Menu, ControllerTypes::controllerType_Virtual))//hmdGetButtonIsUpFrame(ButtonsList::left_Menu, ControllerTypes::controllerType_Virtual))
					//	engine->ClientCmd("escape");//task_menu

					//g_pAnarchyManager->VRFrameEnd();
					//g_pAnarchyManager->VRFrameReady();
				}
			}
			else if (m_iVRAPI == 1)
			{

			}
		}

		// snap the local player avatar
		if (this->VRSpectatorMode() == 1 && m_pLocalAvatarObject)
		{
			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			Vector origin = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();

			VMatrix headMatrix = g_pAnarchyManager->GetVRHeadMatrix();
			VMatrix composedMatrix;
			composedMatrix.SetupMatrixOrgAngles(origin, angles);

			// apply
			composedMatrix = composedMatrix * headMatrix;

			// Finally convert back to origin+angles.
			MatrixAngles(composedMatrix.As3x4(), angles, origin);
			UTIL_SetOrigin(m_pLocalAvatarObject, origin);
			m_pLocalAvatarObject->SetAbsAngles(angles);

			engine->ClientCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", m_pLocalAvatarObject->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
		}

		//g_pAnarchyManager->VRFrameReady();
		return true;
	}
	else
	{
		//g_pAnarchyManager->VRFrameReady();
		DevMsg("Skipping one.\n");
		//g_pAnarchyManager->VRFrameReady();
		return false;
	}

	//DevMsg("%02f %02f %02f\n", translation.x, translation.y, translation.z);
#endif	// end VR_ALLOWED
}

void C_AnarchyManager::VRFrameBegin()
{
#ifdef VR_ALLOWED
	hmdSetFrame();//USETHREADEDVR
#endif
}

void C_AnarchyManager::VRFrameReady()
{
#ifdef VR_ALLOWED
	//hmdBeginFrame(USETHREADEDVR);
	//hmdSubmitFrame();
	//hmdEndFrame();
#endif
}

/*void C_AnarchyManager::VRFrameEnd()
{
#ifdef VR_ALLOWED
	hmdEndFrame();//USETHREADEDVR
#endif
}*/

bool C_AnarchyManager::IsAlwaysAnimatingImagesEnabled()
{
	if (!m_pAlwaysAnimatingImagesConVar)
		return false;

	return m_pAlwaysAnimatingImagesConVar->GetBool();
}

char* C_AnarchyManager::BackBufferNamePerIndex(int i)
{
	if (i == 0)
	{
		return VR_BACK_BUFFER_0;
	}
	if (i == 1)
	{
		return VR_BACK_BUFFER_1;
	}
	if (i == 2)
	{
		return VR_BACK_BUFFER_2;
	}
	return "";
}

bool C_AnarchyManager::CheckIfFileExists(std::string file)
{
	if (file.find("../") != std::string::npos)
		return false;

	return g_pFullFileSystem->FileExists(file.c_str(), "GAME");
}

bool C_AnarchyManager::GetShouldInvertVRMatrices()
{
	if (!m_pDebugInvertVRMatricesConVar)
		m_pDebugInvertVRMatricesConVar = cvar->FindVar("debug_invert_vr_matrices");

	return m_pDebugInvertVRMatricesConVar->GetBool();
}

void C_AnarchyManager::VROff()
{
#ifdef VR_ALLOWED
	if (m_bVRActive && hmdIsConnected())
	{
		this->CloseVR();
		hmdClose();
	}
#endif
}

void C_AnarchyManager::ToggleVR()
{
	if (!m_bVRActive)
		this->InitializeVR();
	else
	{
		this->CloseVR();	// doesnt close & open properly, so leave it open.
		//hmdRecenter();
	}
}

ITexture* C_AnarchyManager::CreateVRSpectatorRenderTarget(IMaterialSystem* pMaterialSystem)
{

#ifdef VR_ALLOWED
		// spectator cam texture
		int iCameraWidth = 64;
		int iCameraHeight = 64;
		m_pVRSpectatorRenderTexture = pMaterialSystem->CreateNamedRenderTargetTextureEx2(VR_SPECTATOR_CAMERA, iCameraWidth, iCameraHeight, RT_SIZE_LITERAL, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_SEPARATE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 0);
#endif
		return m_pVRSpectatorRenderTexture;
}

ITexture* C_AnarchyManager::CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem, int i)
{
#ifdef VR_ALLOWED
	const char* name = this->BackBufferNamePerIndex(i);

	//int iScreenWidth = 2160;// 1920;// ScreenWidth();//
	//int iScreenHeight = 1200;//1080;//  ScreenHeight();//

	m_iVRBufferWidth = (int)hmdGetBufferSize().x;//ScreenWidth();
	m_iVRBufferHeight = (int)hmdGetBufferSize().y;//ScreenHeight();
	//DevMsg("BUFFER: %i x %i\n", m_iVRBufferWidth, m_iVRBufferHeight);

	//int pseudo_sdk_version = 2;
	//ITexture* pTexture = pMaterialSystem->CreateNamedRenderTargetTextureEx2(name, m_iVRBufferWidth, m_iVRBufferHeight, RT_SIZE_LITERAL, (m_iVRAPI != 2) ? pMaterialSystem->GetBackBufferFormat() : IMAGE_FORMAT_RGBA16161616F, MATERIAL_RT_DEPTH_SEPARATE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, CREATERENDERTARGETFLAGS_HDR);
	ITexture* pTexture = pMaterialSystem->CreateNamedRenderTargetTextureEx2(name, 12340 + i, m_iVRBufferHeight, RT_SIZE_LITERAL, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_SEPARATE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 0);// CREATERENDERTARGETFLAGS_HDR);
	
	m_pVRTwoEyesHMDRenderTextures[i] = pTexture;

	if (pTexture)
		DevMsg("Created VR Two Eyes HMD Render Target: %i\n", i);
	else
		DevMsg("Failed to create VR Two Eyes HMD Render Target\n");
	return pTexture;
#else
	return null;
#endif
}

ITexture* C_AnarchyManager::CreateModelPreviewRenderTarget(IMaterialSystem* pMaterialSystem)
{
	//pMaterialSystem->GetBackBufferFormat() : IMAGE_FORMAT_RGBA16161616F
	ImageFormat imageFormat = IMAGE_FORMAT_RGBA8888;//IMAGE_FORMAT_BGRA8888;// IMAGE_FORMAT_RGBA8888;// IMAGE_FORMAT_RGBA16161616F; //MATERIAL_RT_DEPTH_SEPARATE
	ITexture* pTexture = pMaterialSystem->CreateNamedRenderTargetTextureEx2("_rt_model_preview", this->GetModelThumbSize(), this->GetModelThumbSize(), RT_SIZE_LITERAL, imageFormat, MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 0);// CREATERENDERTARGETFLAGS_HDR);
	m_pModelPreviewRenderTexture = pTexture;
	return pTexture;
}

bool C_AnarchyManager::OpenVRHMD()
{
#ifdef VR_ALLOWED
	// make sure an HMD is connected
	if (m_bVRActive || hmdIsConnected())
		return true;
	
	std::string cmdLine = std::string(GetCommandLine());
	size_t found = cmdLine.find("-vr");
	if (found != std::string::npos)
	{
		cmdLine = cmdLine.substr(found + 3);
		if (cmdLine.length() > 1 && cmdLine.at(0) == ' ' && cmdLine.at(1) != '-' && cmdLine.at(1) != '+')
		{
			if (cmdLine.at(1) == '1')
				m_iVRAPI = 1;
			else// if (cmdLine.at(1) == '2')
				m_iVRAPI = 2;
		}
		else
			m_iVRAPI = 2;
	}

	// Set the SDK version to OpenVR
	hmdSetSDK(m_iVRAPI);
	
	// open the HMD connection
	bool bResponse = hmdOpen();//USETHREADEDVR
	if (bResponse)
		DevMsg("Opened VR HMD Connection\n");
	else
		DevMsg("Failed To Open VR HMD Connection\n");

	return bResponse;
#else
	return false;
#endif
}

void C_AnarchyManager::InitializeVR()
{
#ifdef VR_ALLOWED
	if (!hmdIsConnected())
	{
		this->OpenVRHMD();

		if (!hmdIsConnected())
			return;
	}

	//vgui::surface()->SetSoftwareCursor(true);

	/*
	// VRConnect
	if (hmdIsConnected())
	{
		DevMsg("VR is already initialized!\n");
		return;
	}

	// Set the SDK version to OpenVR
	hmdSetSDK(m_iVRAPI);
	*/

	// Open the HMD
	//if (this->OpenVRHMD())
	//{
		engine->ClientCmd("sbs_rendering 1\n");

		float flIPD = hmdGetIPD() * 1000;

		// Assume we're always sitting (what behavior, exactly, does this change? It has something to do with that 64 height added during matrix conversion.)
		hmdSetIsSitting(false);

		// Get the projection matracies
		m_VRLeftEyeProjectionMatrix = *(VMatrix*)(hmdGetProjection(0, false));
		m_VRRightEyeProjectionMatrix = *(VMatrix*)(hmdGetProjection(1, false));

		float fFOV;
		g_pAnarchyManager->CalcFovFromProjection(&fFOV, m_VRLeftEyeProjectionMatrix);

		//m_VRLeftEyeProjectionMatrix = g_pAnarchyManager->SMMatrixToVMatrix(m_VRLeftEyeProjectionMatrix.Base(), -1, true);
		//m_VRRightEyeProjectionMatrix = g_pAnarchyManager->SMMatrixToVMatrix(m_VRRightEyeProjectionMatrix.Base(), 1, true);

		DevMsg("Updating FOV to %f and IPD to actual VR HMD IPD: %f\n", fFOV, flIPD);
		cvar->FindVar("ipd")->SetValue(flIPD);
		cvar->FindVar("lastfov")->SetValue(fFOV);
		engine->ClientCmd(VarArgs("fov %f\n", fFOV));
		//engine->ExecuteClientCmd("mat_reset_rendertargets\n");	<-- THIS is what breaks the dynamic textures on the cabinets!

		this->AddToastMessage("VR Mode Activated");
		m_bVRActive = true;
		m_bHandTrackingActive = true;
		m_bVRSuspending = 0;

		int iRightHandIndex = (pVRHandRight) ? pVRHandRight->entindex() : -1;
		//if ( iRightHandIndex >= 0 )
		//	engine->ClientCmd(VarArgs("create_vr_spazz_fix %i", iRightHandIndex));
		//else




			engine->ClientCmd("create_vr_spazz_fix");

		//if(Q_strcmp(this->MapName(), ""))
		//	this->CreateVRHands();

		/*
	}
	else
	{
		DevMsg("VR HMD failed to open!\n");
		hmdClose();
	}
	*/
#else
	DevMsg("VR was not included with this DLL's compile.\n");
#endif	// end VR_ALLOWED
}

void C_AnarchyManager::CloseVR()
{
#ifdef VR_ALLOWED
	engine->ExecuteClientCmd("sbs_rendering 0\n");
	//engine->ClientCmd("sbs_rendering 0\n");

	if (m_bVRActive && hmdIsConnected())
	{
		m_bVRActive = false;
		m_bHandTrackingActive = false;
		this->AddToastMessage("VR Mode Deactivated");

		//int iVRSpazzFix = g_pAnarchyManager->GetVRSpazzFix(0);// > GetVRHand(1);
		//C_BaseEntity* pVRSpazzFix = C_BaseEntity::Instance(g_pAnarchyManager->GetVRSpazzFix(0));// > GetVRHand(1);
		//if (iVRSpazzFix >= 0)
		//	engine->ClientCmd(VarArgs("jump_object_pos %i %f %f %f %f %f %f;\n", iVRSpazzFix, origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
		//hmdClose();
	}
	else
	{
		m_bVRActive = false;
		m_bHandTrackingActive = false;
}

#else
	DevMsg("VR was not included with this DLL's compile.\n");
#endif	// end VR_ALLOWED
}

void C_AnarchyManager::CreateVRHands()
{
	std::string modelLeft = "models/players/hands/rift_cv1_left.mdl";
	engine->ClientCmd(VarArgs("create_vr_hands \"%s\" %i;\n", modelLeft.c_str(), 0));	// servercmdfix , false);

	std::string modelRight = "models/players/hands/rift_cv1_right.mdl";
	engine->ClientCmd(VarArgs("create_vr_hands \"%s\" %i;\n", modelRight.c_str(), 1));	// servercmdfix , false);

	//std::string modelPointer = "models/players/hands/pointer.mdl";
	//engine->ServerCmd(VarArgs("create_vr_hands \"%s\" %i;\n", modelRight.c_str(), 2), false);
}

void C_AnarchyManager::VRHandCreated(int iEntIndex, int iHandSide, int iPointerIndex, int iTeleportIndex)
{
	if (iHandSide == 0)
	{
		pVRHandLeft = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iEntIndex));
		pVRPointerLeft = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iPointerIndex));

		//Vector origin = pVRHandLeft->GetAbsOrigin();
		//QAngle angles = pVRHandLeft->GetAbsAngles();
		//angles.x += 45;
		//engine->ServerCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pVRHandLeft->entindex(), origin.x, origin.y, origin.z, 45, 0, 0), false);
		//pVRPointerLeft->SetLocalAngles(angles);
	}
	else if (iHandSide == 1)
	{
		pVRHandRight = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iEntIndex));
		pVRPointerRight = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iPointerIndex));
		pVRTeleport = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iTeleportIndex));

		//QAngle angles = pVRHandRight->GetAbsAngles();
		//angles.x += 45;
		//Vector origin = pVRHandRight->GetAbsOrigin();
		//engine->ServerCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pVRHandRight->entindex(), origin.x, origin.y, origin.z, 45, 0, 0), false);
		//pVRPointerRight->SetAbsAngles(angles);
	}
	//else if (iHandSide == 2)
	//	pVRPointer = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iEntIndex));
}

VMatrix C_AnarchyManager::GetVRHandMatrix(int iHandSide)
{
	if (iHandSide == 0)
		return m_VRLeftControllerMatrix;
	else if (iHandSide == 1)
		return m_VRRightControllerMatrix;
	else
	{
		VMatrix matrix;
		matrix.Identity();
		return matrix;
	}
}

C_DynamicProp* C_AnarchyManager::GetVRHand(int iHandSide)
{
	if (iHandSide == 0)
		return pVRHandLeft;
	else if (iHandSide == 1)
		return pVRHandRight;

	return null;
}

C_DynamicProp* C_AnarchyManager::GetVRPointer(int iHandSide)
{
	if (iHandSide == 0)
		return pVRPointerLeft;
	else if (iHandSide == 1)
		return pVRPointerRight;

	return null;
}

C_DynamicProp* C_AnarchyManager::GetVRTeleport()
{
	return pVRTeleport;
}

void C_AnarchyManager::ProcessAllModels()
{
	if (m_pImportInfo->status != AAIMPORTSTATUS_WAITING_FOR_PROCESSING)
	{
		DevMsg("ERROR: INvalid import state.\n");
		return;
	}

	unsigned int count = m_pMetaverseManager->ProcessModels(m_pImportInfo);

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing New Models", "processmodels", "", "", VarArgs("%u", count), "");

	// clear the import info, so it can be used next time.
	m_pImportInfo->count = 0;
	m_pImportInfo->data.clear();
	m_pImportInfo->status = AAIMPORTSTATUS_NONE;
	m_pImportInfo->type = AAIMPORT_NONE;
}

bool C_AnarchyManager::CheckStartWithWindows()
{
	bool bStatus = false;

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		long key = RegQueryValueExA(hKey, "AArcade", NULL, NULL, NULL, NULL);
		if (key != ERROR_FILE_NOT_FOUND)
			bStatus = true;

		RegCloseKey(hKey);
	}

	return bStatus;
}

bool C_AnarchyManager::SetStartWithWindows(bool bValue)
{
	bool bSuccess = false;

	HKEY key;
	if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Valve\\Steam"), &key) == ERROR_SUCCESS)
	{
		char value[AA_MAX_STRING];
		DWORD value_length = AA_MAX_STRING;
		DWORD flags = REG_SZ;
		RegQueryValueEx(key, "SteamPath", NULL, &flags, (LPBYTE)&value, &value_length);
		RegCloseKey(key);

		V_FixSlashes(value);

		std::string steamLocation = std::string(value) + "\\steam.exe -applaunch 266430 -autoredux";

		if (bValue)
		{
			// blindy add the key, overwriting it if it already exists.
			HKEY hkey;
			if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\Currentversion\\Run", 0, KEY_SET_VALUE, &hkey) == ERROR_SUCCESS)
			{
				char szBuf[AA_MAX_STRING];
				Q_strcpy(szBuf, steamLocation.c_str());

				RegSetValueEx(hkey, "AArcade", 0, REG_SZ, (LPBYTE)szBuf, strlen(szBuf) + 1);
				RegCloseKey(hkey);
				bSuccess = true;
			}
		}
		else
		{
			RegDeleteKeyValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\Currentversion\\Run", "AArcade");
			bSuccess = true;
		}
	}

	return bSuccess;
}

bool C_AnarchyManager::AttemptSelectEntity(C_BaseEntity* pTargetEntity, bool bIgnoreSlave)
{
	if (!g_pAnarchyManager->IsInitialized() )
		return false;

	C_EmbeddedInstance* pDisplayInstance = this->GetCanvasManager()->GetDisplayInstance();
	if (!pDisplayInstance)
		pDisplayInstance = this->GetCanvasManager()->GetFirstInstanceToDisplay();

	C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();
	if (pShortcut)
	{
		// finished positioning & choosing model, ie: changes confirmed
		DevMsg("CHANGES CONFIRMED\n");
		C_AwesomiumBrowserInstance* pHubInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");

		//std::string code = "saveTransformChangesCallback(";
		//code += VarArgs("%i", pShortcut->GetBody());	// Pass the body value back because the math is hard to do from javascript to reverse-engineer it from transform form state.
		//code += ");";
		std::string code = VarArgs("saveTransformChangesCallback(%i);", pShortcut->GetBody());

		pHubInstance->GetWebView()->ExecuteJavascript(WSLit(code.c_str()), WSLit(""));
		g_pAnarchyManager->DeactivateObjectPlacementMode(true);

		object_t* pObject = m_pInstanceManager->GetInstanceObject(pShortcut->GetObjectId());
		if (pObject && pObject->child)
		{
			pObject->child = false;
			pObject->parentEntityIndex = -1;
			engine->ClientCmd(VarArgs("setparent %i;\n", pShortcut->entindex()));
		}

		//float scale;
		float flParentScale = pShortcut->GetModelScale();
		Vector childOrigin;
		QAngle childAngles;
		C_PropShortcutEntity* pChildShortcut;
		object_t* pChildObject;
		KeyValues* pInstanceObjectsKV;
		KeyValues* pChildObjectKV;
		C_BaseEntity* pBaseEntity = pShortcut->FirstMoveChild();
		if (pBaseEntity)
			pInstanceObjectsKV = m_pInstanceManager->GetCurrentInstanceKV()->FindKey("objects", true);

		while (pBaseEntity)
		{
			pChildShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			if (pChildShortcut)
			{
				// we are a child object.
				// we must have our KV & object updated with our current values based on the new parent position.
				// the KV is automatically updated when ApplyChanges happens.
				pChildObject = m_pInstanceManager->GetInstanceObject(pChildShortcut->GetObjectId());
				if (pChildObject)
				{
					pChildObjectKV = pInstanceObjectsKV->FindKey(pChildShortcut->GetObjectId().c_str());
					if (pChildObjectKV)
					{
						// figure out our new childOrigin and childAngles
						/*
						UTIL_StringToVector(childOrigin.Base(), pChildObjectKV->GetString("local/position", "0 0 0"));	// FIXME: TODO: Failing to properly read a position at this point might indicate a broken item KV.  should try and detect it instead of having a fallback of "0 0 0"
						UTIL_StringToVector(childAngles.Base(), pChildObjectKV->GetString("local/rotation", "0 0 0"));

						childOrigin *= flParentScale;
						VMatrix childMatrix;
						childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);

						VMatrix composedMatrix;
						composedMatrix.SetupMatrixOrgAngles(pShortcut->GetAbsOrigin(), pShortcut->GetAbsAngles());
						composedMatrix = composedMatrix * childMatrix;

						// back to vecs & angles
						MatrixAngles(composedMatrix.As3x4(), childAngles, childOrigin);
						*/

						childOrigin = pChildShortcut->GetAbsOrigin();
						childAngles = pChildShortcut->GetAbsAngles();
						pChildObject->origin = childOrigin;
						pChildObject->angles = childAngles;
						pChildObjectKV->SetString("local/position", VarArgs("%f %f %f", childOrigin.x, childOrigin.y, childOrigin.z));
						pChildObjectKV->SetString("local/rotation", VarArgs("%f %f %f", childAngles.x, childAngles.y, childAngles.z));

						//engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pChildShortcut->entindex(), childOrigin.x, childOrigin.y, childOrigin.z, childAngles.x, childAngles.y, childAngles.z));//	// servercmdfix , false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.
					}
				}
			}

			pBaseEntity = pBaseEntity->NextMovePeer();
		}
		/*
		pShortcut->Next
		// if this object has children, they must have their KV & objects updated with their current values based on the new parent position.
		KeyValues* pItemKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->GetLibraryEntry("items", pObject->itemId));
		if (pItemKV)
		{
			instance_t* pInstance = m_pInstanceManager->GetInstance(pItemKV->GetString("file"));
			if (pInstance)
			{
				// we are a node.
			}
		}
		*/

		m_pInstanceManager->ApplyChanges(pShortcut);
		m_pMetaverseManager->SendObjectUpdate(pShortcut);
		m_pInstanceManager->ResetTransform();

		return false;// SelectEntity(pShortcut);
	}
	else
	{
		C_BaseEntity* pEntity = null;
		if (!pTargetEntity)
		{
			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (!pPlayer)
				return false;

			if (pPlayer->GetHealth() <= 0)
				return false;

			/*
			// fire a trace line
			trace_t tr;
			this->SelectorTraceLine(tr);
			//Vector forward;
			//pPlayer->EyeVectors(&forward);
			//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

			if( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
				pEntity = tr.m_pEnt;
			*/

			pEntity = C_BaseEntity::Instance(m_iSelectorTraceEntityIndex);
		}
		else
			pEntity = pTargetEntity;

		// only allow prop shortcuts
		if (pEntity)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);

		bool bSwitched = false;
		if (pShortcut)
		{
			// if this is a slave screen & there is a slave video playing, select THAT entity instead.
			std::string itemId = pShortcut->GetItemId();
			if (itemId != "" && itemId != pShortcut->GetModelId())
			{
				if (!bIgnoreSlave)
				{
					object_t* pObject = m_pInstanceManager->GetInstanceObject(pShortcut->GetObjectId());
					if (pObject->slave)
					{
						//C_EmbeddedInstance* pDisplayInstance = m_pCanvasManager->GetDisplayInstance();
						std::string displayItemId = (pDisplayInstance) ? pDisplayInstance->GetOriginalItemId() : "";

						if (displayItemId != "" && displayItemId != itemId)
						{
							C_BaseEntity* pDisplayBaseEntity = C_BaseEntity::Instance(pDisplayInstance->GetOriginalEntIndex());
							if (pDisplayBaseEntity)
							{
								C_PropShortcutEntity* pDisplayShortcut = dynamic_cast<C_PropShortcutEntity*>(pDisplayBaseEntity);
								if (pDisplayShortcut)
								{
									// switcharoo
									pEntity = pDisplayBaseEntity;
									pShortcut = pDisplayShortcut;
									bSwitched = true;
								}
							}
						}
					}
				}
			}
		}


		//if (pEntity->entindex() != this->GetAutoCloseTasks())
		//{
			// if there is currently a remembered task, close it
		//}

		if (pShortcut)
		{
			//C_EmbeddedInstance* pEmbeddedInstance = this->GetCanvasManager()->GetDisplayInstance();
			//if (!pEmbeddedInstance)
			//	pEmbeddedInstance = this->GetCanvasManager()->GetFirstInstanceToDisplay();

			//bool bNeesFullscreen = (!this->GetAutoCloseTasks() || (m_pSelectedEntity && m_pSelectedEntity == pEntity));
			if (m_pSelectedEntity && pEntity == m_pSelectedEntity)
			{
				if (!this->IsVRActive())
				{
					C_EmbeddedInstance* pEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
					//if (vgui::input()->WasKeyReleased(KEY_XBUTTON_RTRIGGER))
					//	m_pInputManager->SetGamepadInputMode(true);
					m_pInputManager->ActivateInputMode(true, m_pInputManager->GetMainMenuMode(), pEmbeddedInstance);
					//g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pEmbeddedInstance);
					//g_pAnarchyManager->GetInputManager()->SetFullscreenMode(true);
				}// do nothing if in VR mode.  no fullscreen mode.
			}
			else if (!this->GetAutoCloseTasks() && pDisplayInstance && pDisplayInstance->GetOriginalEntIndex() == pShortcut->entindex())
			{
				if (pShortcut->GetItemId() != pShortcut->GetModelId())
				{
					//C_EmbeddedInstance* pEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
					//this->TempSelectEntity(pShortcut->entindex());
					//m_pInputManager->ActivateInputMode(true, m_pInputManager->GetMainMenuMode(), pEmbeddedInstance);

					
					g_pAnarchyManager->SelectEntity(C_BaseEntity::Instance(pShortcut->entindex()));
					g_pAnarchyManager->GetInputManager()->SetTempSelect(true);
					C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
					if (vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER))
						m_pInputManager->SetGamepadInputMode(true);
					g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, g_pAnarchyManager->GetInputManager()->GetMainMenuMode(), pDisplayInstance);
				}
				else
				{
					// is a prop! (for real??)
					return true;
				}
			}
			else
			{
				//if (!bSwitched && this->GetAutoCloseTasks() && pDisplayInstance)	// if there is currently a remembered task, close it
				//	pDisplayInstance->Close();

				// FIXME: HERE is where you'll want to handle the difference between just dropping out of freemousemode and activating the object vs going into FULLSCREEn mode & activating the object.
				// (Applicable for when in freemousemode you click on an already selected cabinet.  This should induce the fullscreen mode & activating the object behavior!!

				bool bSuccess;
				if (pShortcut->GetItemId() != pShortcut->GetModelId())	// is NOT a prop!
					bSuccess = (this->GetAutoCloseTasks()) ? SelectEntity(pEntity) : QuickRemember(pEntity->entindex());//SelectEntity(pEntity);
				else
				{
					// Do not close display tasks when a just a prop gets selected...
					/*
					if (pDisplayInstance)// && pDisplayInstance->GetOriginalEntIndex() == pShortcut->entindex())
					{
						C_BaseEntity* pDisplayInstanceEntity = C_BaseEntity::Instance(pDisplayInstance->GetOriginalEntIndex());
						if (pDisplayInstanceEntity)
						{
							//C_PropShortcutEntity* pSelectedShortcut = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);
							C_PropShortcutEntity* pDisplayInstanceShortcut = dynamic_cast<C_PropShortcutEntity*>(pDisplayInstanceEntity);
							if (pDisplayInstanceShortcut)
							{
								if (!this->GetAutoCloseTasks())// && pDisplayInstanceShortcut->GetItemId() != pDisplayInstanceShortcut->GetModelId())
									this->TaskRemember(pDisplayInstanceShortcut);
								else
									pDisplayInstance->Close();
									//this->DeselectEntity();
							}
						}
					}
					*/

					this->SelectEntity(pEntity);
					bSuccess = true;
				}

				if (bSuccess && bSwitched)
				{
					if (!this->GetAutoCloseTasks())
						g_pAnarchyManager->GetInputManager()->SetTempSelect(true);

					g_pAnarchyManager->AddToastMessage("Master Object Selected");
				}

				return bSuccess;
			}
		}
		else
		{
			//if (this->GetAutoCloseTasks() && pDisplayInstance)	// if there is currently a remembered task, close it
			//	pDisplayInstance->Close();

			C_DynamicProp* pProp = dynamic_cast<C_DynamicProp*>(pEntity);
			if (pProp)
			{
				user_t* pUser = m_pMetaverseManager->FindInstanceUser(pProp);
				if (pUser)
				{
					// its a player.
					m_pMetaverseManager->InstanceUserClicked(pUser);
					return false;
				}
			}
			
			if (m_pSelectedEntity)
				return DeselectEntity();
			else
				return false;
		}
	}

	return false;
}

// from http://www.zedwood.com/article/cpp-urlencode-function
#include <iostream>
#include <sstream>
std::string C_AnarchyManager::encodeURIComponent(const std::string &s)
{
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for (int i = 0, ix = s.length(); i<ix; i++)
	{
		const char& c = s[i];
		if ((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}

/*
// from https://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm
std::string C_AnarchyManager::decodeURIComponent(const std::string &s)
{
	// Note from RFC1630: "Sequences which start with a percent
	// sign but are not followed by two hexadecimal characters
	// (0-9, A-F) are reserved for future extension"

	const unsigned char * pSrc = (const unsigned char *)s.c_str();
	const int SRC_LEN = s.length();
	const unsigned char * const SRC_END = pSrc + SRC_LEN;
	// last decodable '%' 
	const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

	char * const pStart = new char[SRC_LEN];
	char * pEnd = pStart;

	while (pSrc < SRC_LAST_DEC)
	{
		if (*pSrc == '%')
		{
			char dec1, dec2;
			if (-1 != (dec1 = hex2dec[*(pSrc + 1)])
				&& -1 != (dec2 = hex2dec[*(pSrc + 2)]))
			{
				*pEnd++ = (dec1 << 4) + dec2;
				pSrc += 3;
				continue;
			}
		}

		*pEnd++ = *pSrc++;
	}

	// the last 2- chars
	while (pSrc < SRC_END)
		*pEnd++ = *pSrc++;

	std::string sResult(pStart, pEnd);
	delete[] pStart;
	return sResult;
}
*/

bool C_AnarchyManager::SelectEntity(C_BaseEntity* pEntity)
{
//	DevMsg("DISABLED FOR TESTING!\n");
//	return true;
//	/*
	if (m_pSelectedEntity)
		DeselectEntity();

//	m_pWebManager->GetHudWebTab()->SetUrl("asset://ui/blank.html");

	m_pSelectedEntity = pEntity;
	AddGlowEffect(pEntity);

	

		//pMaterials[x]->ColorModulate(255, 0, 0);
	//pMaterials[x]->GetPreviewImage

	std::string itemId;
	std::string taskId;
	std::string uri;
	KeyValues* item;
	KeyValues* active;
	//C_PropShortcutEntity* pShortcut;
	//C_WebTab* pWebTab;
	//C_WebTab* pSelectedWebTab;
	C_EmbeddedInstance* pEmbeddedInstance;
	C_EmbeddedInstance* pSelectedEmbeddedInstance;

	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
	if (!this->ShouldAllowMultipleActive())
		pShortcut->PlaySequenceRegular("activated");

	std::vector<C_EmbeddedInstance*> embeddedInstances;
	pShortcut->GetEmbeddedInstances(embeddedInstances);

	unsigned int i;
	unsigned int size = embeddedInstances.size();
	for (i = 0; i < size; i++)
	{
		pEmbeddedInstance = embeddedInstances[i];
		if (!pEmbeddedInstance)
			continue;

		if (pEmbeddedInstance->GetId() == "hud")
			continue;

		if (pEmbeddedInstance->GetId() == "network")
			continue;

		bool bImagesAndHandled = false;
		if (pEmbeddedInstance->GetId() == "images")
		{
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);
			if (pShortcut)
			{
				taskId = "auto" + pShortcut->GetItemId();
				pEmbeddedInstance = m_pCanvasManager->FindEmbeddedInstance(taskId);// this->GetWebManager()->FindWebTab(tabTitle);
				if (!pEmbeddedInstance)
				{
					itemId = pShortcut->GetItemId();
					item = m_pMetaverseManager->GetLibraryItem(itemId);
					if (item)
					{
						active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(item);

						/*
						std::string uri = "asset://ui/autoInspectItem.html?mode=" + g_pAnarchyManager->GetAutoInspectMode() + "&id=" + encodeURIComponent(itemId) + "&screen=" + encodeURIComponent(active->GetString("screen")) + "&marquee=" + encodeURIComponent(active->GetString("marquee")) + "&preview=" + encodeURIComponent(active->GetString("preview")) + "&reference=" + encodeURIComponent(active->GetString("reference")) + "&file=" + encodeURIComponent(active->GetString("file"));
						WebURL url = WebURL(WSLit(uri.c_str()));
						*/

						//std::string dumbUrl = "http://smarcade.net/dlcv2/view_youtube.php?id=";
						//std::string dumbUrl = active->GetString("file");

						// If this is a video file, play it in libretro instead of the browser
						bool bDoAutoInspect = true;

						std::string gameFile = "";
						std::string coreFile = "";
						bool bShouldLibretroLaunch = (this->DetermineLibretroCompatible(item, gameFile, coreFile) && m_pLibretroManager->GetInstanceCount() == 0);

						// auto-libretro
						if (cvar->FindVar("auto_libretro")->GetBool() && bShouldLibretroLaunch && g_pFullFileSystem->FileExists(gameFile.c_str()))
						{
							C_LibretroInstance* pLibretroInstance = m_pLibretroManager->CreateLibretroInstance();
							pLibretroInstance->Init(taskId, VarArgs("%s - Libretro", active->GetString("title", "Untitled")), pShortcut->entindex());
							DevMsg("Setting game to: %s\n", gameFile.c_str());
							pLibretroInstance->SetOriginalGame(gameFile);
							pLibretroInstance->SetOriginalItemId(itemId);
							if (!pLibretroInstance->LoadCore(coreFile))	// FIXME: elegantly revert back to autoInspect if loading the core failed!
								DevMsg("ERROR: Failed to load core: %s\n", coreFile.c_str());
							else
								m_pAccountant->Action("aa_libretro_played", 1);
							pEmbeddedInstance = pLibretroInstance;
							bDoAutoInspect = false;
						}

						if (bDoAutoInspect)
						{
							std::string fileBuf = active->GetString("file");
							if (fileBuf.find("travel.html?") == 0)
							{
								std::string uri = "asset://ui/travel.html?item=" + itemId + "&entity=";
								uri += VarArgs("%i", pShortcut->entindex());
								uri += VarArgs("&object=%s&intro=1", pShortcut->GetObjectId().c_str());

								C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
								pHudBrowserInstance->SetUrl(uri);
								g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
								this->DeselectEntity("", false);
								return false;

								/*
								std::string uri = "asset://ui/travel.html?item=" + itemId + "&entity=";
								uri += VarArgs("%i", pShortcut->entindex());

								std::string title = active->GetString("title", "Untitled");
								C_AwesomiumBrowserInstance* pBrowserInstance = m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance(taskId, uri, title, false, null, pShortcut->entindex());
								//pBrowserInstance->Init(taskId, uri, title, pShortcut->entindex());
								//pBrowserInstance->SetOriginalGame(gameFile);
								pBrowserInstance->SetOriginalItemId(itemId);
								pEmbeddedInstance = pBrowserInstance;

								bDoAutoInspect = false;
								*/
							}
							else
							{
								std::string previewBuf = active->GetString("preview");
								std::string matchedField;
								if (fileBuf.find("slideshow.html?") == 0)
									matchedField = "file";
								else if (previewBuf.find("slideshow.html?") == 0)
									matchedField = "preview";

								if (matchedField != "")
								{
									std::string uri = "asset://ui/slideshow.html?item=" + itemId + "&entity=";
									uri += VarArgs("%i", pShortcut->entindex());
									uri += "&field=" + matchedField;

									std::string title = active->GetString("title", "Untitled");
									C_AwesomiumBrowserInstance* pBrowserInstance = m_pAwesomiumBrowserManager->CreateAwesomiumBrowserInstance(taskId, uri, title, false, null, pShortcut->entindex());
									//pBrowserInstance->Init(taskId, uri, title, pShortcut->entindex());
									//pBrowserInstance->SetOriginalGame(gameFile);
									pBrowserInstance->SetOriginalItemId(itemId);
									pEmbeddedInstance = pBrowserInstance;

									bDoAutoInspect = false;
								}
							}
						}

						if ( bDoAutoInspect)
						{
							/*std::string uri = "file://";
							uri += engine->GetGameDirectory();
							uri += "/resource/ui/html/autoInspectItem.html?imageflags=" + g_pAnarchyManager->GetAutoInspectImageFlags() + "&id=" + encodeURIComponent(itemId) + "&title=" + encodeURIComponent(active->GetString("title")) + "&screen=" + encodeURIComponent(active->GetString("screen")) + "&marquee=" + encodeURIComponent(active->GetString("marquee")) + "&preview=" + encodeURIComponent(active->GetString("preview")) + "&reference=" + encodeURIComponent(active->GetString("reference")) + "&stream=" + g_pAnarchyManager->encodeURIComponent(active->GetString("stream")) + "&file=" + encodeURIComponent(active->GetString("file"));
							//uri = "http://smsithlord.com/";
							// FIXME: Might want to make the slashes in the game path go foward.  Also, need to allow HTTP redirection (302).
							//DevMsg("Test URI is: %s\n", uri.c_str());

							C_SteamBrowserInstance* pSteamBrowserInstance = m_pSteamBrowserManager->CreateSteamBrowserInstance();
							pSteamBrowserInstance->Init(tabTitle, uri, "Newly selected item...", null, pShortcut->entindex());
							pSteamBrowserInstance->SetOriginalItemId(itemId);	// FIXME: do we need to do this for original entindex too???*/
							//pSteamBrowserInstance->SetOriginalEntIndex(pShortcut->entindex());	// probably NOT needed??

							C_SteamBrowserInstance* pSteamBrowserInstance = this->AutoInspect(item, taskId, pShortcut->entindex());
							pEmbeddedInstance = pSteamBrowserInstance;
						}
					}
					else
					{
						// the item specified by the shortcut was not found
						// by doing NOTHING, it lets you select the object but not bring up any menus on it
					}
				}
			}
		}
		else
		{
			pSelectedEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();// m_pWebManager->GetSelectedWebTab();
			if (pSelectedEmbeddedInstance)
			{
				pSelectedEmbeddedInstance->Deselect();
				m_pInputManager->SetEmbeddedInstance(null);
				//m_pWebManager->DeselectWebTab(pSelectedEmbeddedInstance);
			}
		}

		if (pEmbeddedInstance)
		{
			pSelectedEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
			if (pSelectedEmbeddedInstance && pSelectedEmbeddedInstance != pEmbeddedInstance)
			{
				//m_pWebManager->DeselectWebTab(pSelectedEmbeddedInstance);
				pSelectedEmbeddedInstance->Deselect();
				m_pInputManager->SetEmbeddedInstance(null);

				//m_pWebManager->SelectWebTab(pWebTab);
				pEmbeddedInstance->Select();
				m_pInputManager->SetEmbeddedInstance(pEmbeddedInstance);
			}
			else if (!pSelectedEmbeddedInstance)
			{
				//if (pEmbeddedInstance)
				//{
				pEmbeddedInstance->Select();
				m_pInputManager->SetEmbeddedInstance(pEmbeddedInstance);
				//}



			}
		}
		else
		{
			DevMsg("ERROR: No embedded instance!!\n");
			DevMsg("ID on this item is: %s", pShortcut->GetItemId().c_str());
		}
		//C_AwesomiumBrowserInstance* pHudWebTab = this->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

		//m_pWebManager->SelectWebTab(pWebTab);

		//break;
	}

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/overlay.html");
	//g_pAnarchyManager->HudStateNotify();	// because input is not always request when an object is selected?
	return true;
	//*/
}

bool C_AnarchyManager::DeselectEntity(std::string nextUrl, bool bCloseInstance)
{
	if (!m_pSelectedEntity)
		return false;

	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);
	//if (pShortcut)
	//{
		if( pShortcut->GetItemId() != pShortcut->GetModelId())	// is NOT a prop!
		{
			C_EmbeddedInstance* pEmbeddedInstance = m_pInputManager->GetEmbeddedInstance();
			C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
			//C_WebTab* pWebTab = m_pWebManager->GetSelectedWebTab();
			if (pEmbeddedInstance && pEmbeddedInstance != pHudBrowserInstance)
			{
				//pEmbeddedInstance->Deselect();
				//m_pInputManager->SetEmbeddedInstance(null);

				// ALWAYS close the selected web tab when de-selecting entities. (this has to be accounted for or changed when the continous play button gets re-enabled!)
				if (bCloseInstance && pEmbeddedInstance != pHudBrowserInstance)
					pEmbeddedInstance->Close();	// FIXME: (maybe) This might cause blank.html to be loaded into the HUD layer because the entity that initiated all this is still set as m_pSelectedEntity at this point...
			}

			if (nextUrl != "" && nextUrl != "none")
			{
				if (nextUrl != "")
					pHudBrowserInstance->SetUrl(nextUrl);
				else
					pHudBrowserInstance->SetUrl("asset://ui/overlay.html");
			}
		}
	//}
	

	RemoveGlowEffect(m_pSelectedEntity);
	//C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);


	if (!g_pAnarchyManager->ShouldAllowMultipleActive())
		pShortcut->PlaySequenceRegular("deactivated");


	m_pSelectedEntity = null;

	this->GetInputManager()->SetTempSelect(false);

	return true;
}

void C_AnarchyManager::AddHoverGlowEffect(C_BaseEntity* pEntity)
{
	g_pAnarchyManager->RemoveLastHoverGlowEffect();
	m_pHoverGlowEntity = pEntity;
	engine->ClientCmd(VarArgs("addhovergloweffect %i", pEntity->entindex()));	// servercmdfix , false);
}

void C_AnarchyManager::RemoveLastHoverGlowEffect()
{
	if (m_pHoverGlowEntity && !this->IsShuttingDown())
	{
		engine->ClientCmd(VarArgs("removehovergloweffect %i", m_pHoverGlowEntity->entindex()));	// servercmdfix , false);
		m_pHoverGlowEntity = null;
	}
}

std::string C_AnarchyManager::GetAutoInspectImageFlags()
{
	std::string flags = cvar->FindVar("autoinspect_image_flags")->GetString();
	return flags;
}

void C_AnarchyManager::AddGlowEffect(C_BaseEntity* pEntity)
{
	if (!g_pAnarchyManager->IsVRActive())
		engine->ClientCmd(VarArgs("addgloweffect %i", pEntity->entindex()));	// servercmdfix , false);
}

void C_AnarchyManager::RemoveGlowEffect(C_BaseEntity* pEntity)
{
	engine->ClientCmd(VarArgs("removegloweffect %i", pEntity->entindex()));	// servercmdfix , false);
}

void C_AnarchyManager::ShowFileBrowseMenu(std::string browseId)
{
	BrowseSlate->Create(enginevgui->GetPanel(PANEL_ROOT), browseId);
}

void C_AnarchyManager::OnBrowseFileSelected(std::string browseId, std::string response)
{
	// only the HUD web view brings up the file browse menu so far...
	std::vector<std::string> params;
	params.push_back(browseId);
	params.push_back(response);

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onBrowseFileSelected", params);
}

void C_AnarchyManager::OnWorkshopManagerReady()
{
	DevMsg("C_AnarchyManager::OnWorkshopManagerReady\n");
//	DevMsg("DISABLED FOR TESTING!\n");
//	return;
	///*

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	// mount ALL workshops

	//if (m_pWorkshopManager->IsEnabled())
	//{
		/*
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Skipping Gen 1 Legacy Workshop Subscriptions", "skiplegacyworkshops", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", "0");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", "0");
		*/
	//}
	//else
	if (!m_pWorkshopManager->IsEnabled())
	{
		pHudBrowserInstance->AddHudLoadingMessage("", "", "Skipping All Workshop", "workshopskip", "", "", "");
		g_pAnarchyManager->OnMountAllWorkshopsComplete();
	}
	else
		m_pWorkshopManager->MountFirstWorkshop();
	//*/
}

void C_AnarchyManager::ScanForLegacySave(std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack)
{
	// Legacy saves come from:
	//	1. Legacy Workshop Backpacks (GEN1)
	//	2. Legacy Workshop Subscriptions (GEN2)
	//	3. Mounted Legacy Folder (GEN1 & GEN2)

	// Legacy saves can be in:
	//	1. saves/maps/[MAP_NAME].[ADDON_ID].set (GEN1)
	//	2. maps/[BSP_NAME].[ADDON_ID].set (GEN2) (potentially a NODE)
	// This function gets called for both of those folder locations.

	// detect any .set files
	//std::string file;
	//KeyValues* kv = new KeyValues("instance");
	FileFindHandle_t findHandle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("%s*.set", path.c_str()), searchPath.c_str(), &findHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(findHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(findHandle);
			continue;
		}

		// NOTE: Nodes (which are the only kind of .set file names that are even able to have a legacy ID extracted from them) should
		// have their legacy ID extracted.  Only generate a hash using the whole name if the legacy ID *cannot* be extracted.
		std::string legacyId = g_pAnarchyManager->ExtractLegacyId(std::string(VarArgs("maps/%s", pFilename)));
		std::string instanceId = (legacyId != "") ? legacyId : g_pAnarchyManager->GenerateLegacyHash(pFilename);
		std::string filename = pFilename;
		std::string file = path + filename;

		// Does this instance already exist?
		bool bConsumed;
		instance_t* pInstance = g_pAnarchyManager->GetInstanceManager()->GetInstance(instanceId);
		if (pInstance)
		{
			DevMsg("Skipping consumption of legacy save file because it already existed in user library: %s\n", file.c_str());
			bConsumed = false;
		}
		else
			bConsumed = g_pAnarchyManager->GetInstanceManager()->ConsumeLegacyInstance(instanceId, filename, path, searchPath, workshopIds, mountIds, pBackpack);

		pFilename = g_pFullFileSystem->FindNext(findHandle);
	}

//	kv->Clear();
	g_pFullFileSystem->FindClose(findHandle);
}

void C_AnarchyManager::ScanForLegacySaveRecursive(std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack)
{
	std::string legacyPathA = path + "maps\\";
	std::string legacyPathB = path + "saves\\maps\\";
	this->ScanForLegacySave(legacyPathA, searchPath, workshopIds, mountIds, pBackpack);
	this->ScanForLegacySave(legacyPathB, searchPath, workshopIds, mountIds, pBackpack);
}

// Purpose:
// 1 - Mark all affected entities as child objects in the currently loaded instance, & save it out.
// 2 - Create (or update) the instance associated with this pInfoItemKV.
void C_AnarchyManager::ShowHubSaveMenuClient(C_PropShortcutEntity* pInfoShortcut)
{
	// NOTE: All affected entities are properly parented at this point.

	// Get the item of the given info shortcut
	KeyValues* pInfoItemKV = m_pMetaverseManager->GetActiveKeyValues(m_pMetaverseManager->GetLibraryItem(pInfoShortcut->GetItemId()));
	if (!pInfoItemKV)
		return;

	// load up the nodevolume.txt
	KeyValues* pNodeInfoKV = new KeyValues("node");
	if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Could not load nodevolume.txt.\n");
		return;
	}

	// create (or reset) this node's instance KV
	KeyValues* pNodeInstanceKV = new KeyValues("instance");
	std::string nodeInstanceId = pInfoItemKV->GetString("file");

	std::string title = pInfoItemKV->GetString("title");
	std::string style = "node_" + std::string(pNodeInfoKV->GetString("setup/style"));
	g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(0, pNodeInstanceKV, nodeInstanceId, "", title, "", "", "", style);

	instance_t* pNodeInstance = m_pInstanceManager->GetInstance(nodeInstanceId);
	if (!pNodeInstance)
	{
		DevMsg("ERROR: Failed to create new instance for this node!\n");
		return;
	}

	// the blank KV for this node's instance is already loaded into pNodeInstanceKV

	// FIRST TASK: Create (or update) the KV of this node's instance.
	// ==============================================================
	// NOTE: Since this node instance is NOT the actively loaded instance, we can bulk-update it.
	// Just modify it's KV, and save them out directly.
	// 1. loop through all affected entities.
	// 2. add each one to the KV.
	// 3. save the KV to the DB.

	// work for task 1
	float scale;
	int slave;
	int child;
	C_BaseEntity* pBaseEntity;
	C_PropShortcutEntity* pPropShortcutEntity;
	object_t* pObject;
	KeyValues* pObjectKV;
	for (KeyValues *sub = pNodeInfoKV->FindKey("setup/objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		if (pPropShortcutEntity == pInfoShortcut)
			continue;

		pObject = m_pInstanceManager->GetInstanceObject(pPropShortcutEntity->GetObjectId());

		char buf[AA_MAX_STRING];

		// position
		Vector localPosition = pPropShortcutEntity->GetLocalOrigin();
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", localPosition.x, localPosition.y, localPosition.z);
		std::string position = buf;

		// rotation
		QAngle localAngles = pPropShortcutEntity->GetLocalAngles();
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", localAngles.x, localAngles.y, localAngles.z);
		std::string rotation = buf;

		scale = pPropShortcutEntity->GetModelScale();
		slave = (pObject->slave) ? 1 : 0;
		child = 0;	// Objects inside of nodes themselves cannot have children.

		pObjectKV = pNodeInstanceKV->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()), true);
		g_pAnarchyManager->GetInstanceManager()->CreateObject(pObjectKV, pPropShortcutEntity->GetObjectId(), pPropShortcutEntity->GetItemId(), pPropShortcutEntity->GetModelId(), position, rotation, scale, "", slave, child, pObject->body, pObject->skin);

		// This entity has an entry in the node instance's KV.  Continue.
	}

	// This node's instance has been completed.  Save it out & clean-up.
	g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", nodeInstanceId.c_str(), pNodeInstanceKV);
	pNodeInstanceKV->deleteThis();
	pNodeInstanceKV = null;

	// SECOND TASK: Update the actively loaded instance with the new state of the existing objects.
	// ============================================================================================
	// All entities will exist, so just need to grab their object_t's and update their child / parentEntIndex values, then apply the changes so they get saved to the map's instance.
	// Note that this will cause the instance to be saved to the DB once-PER-object.
	// FIXME: There *should* really be a bulk save operation for doing this to the active map instance. (However, considerations for multiplayer mode must be kept in mind.)
	// 1. loop through all affected entities
	// 2. update their object_t
	// 3. apply changes to the entity

	// work for task 2
	for (KeyValues *sub = pNodeInfoKV->FindKey("setup/objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		if (pPropShortcutEntity == pInfoShortcut)
			continue;

		pObject = m_pInstanceManager->GetInstanceObject(pPropShortcutEntity->GetObjectId());
		if (!pObject)
			continue;

		if (!pObject->child || pObject->parentEntityIndex != pInfoShortcut->entindex())
		{
			pObject->child = true;
			pObject->parentEntityIndex = pInfoShortcut->entindex();
			m_pInstanceManager->ApplyChanges(pPropShortcutEntity, false);	// NOTE: Remember, multiplayer stuff will likely want 1 object synced at a time!!
		}
	}

	// save the bulk write to the DB (remember this might conflict with what multiplayer wants to do for syncing purposes.)
	m_pInstanceManager->SaveActiveInstance();
	pNodeInfoKV->deleteThis();

	// The node is now finished being processed.
	DevMsg("Finished creating/updating node.\n");
}

void C_AnarchyManager::ShowNodeManagerMenu()
{
	if (!m_pInputManager->GetInputMode() && engine->IsInGame())
	{
		if (!enginevgui->IsGameUIVisible())
		{
			C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
			if (m_pSelectedEntity)
				this->DeselectEntity("asset://ui/nodeManager.html");
			else
				pHudBrowserInstance->SetUrl("asset://ui/nodeManager.html");

			//m_pInputManager->ActivateInputMode(true, true);
			m_pInputManager->ActivateInputMode(true, false);
		}
	}
}

std::string C_AnarchyManager::GetSteamGamesCode(std::string requestId)
{
	DevMsg("Injecting Steam Games importing code...\n");
	std::string code;//
	//code += "function aaGetSteamGames(){";
	//code += "if( !!!window.rgGames ) { setTimeout(aaGetSteamGames, 1000); return; }";
	code += "var aagames = [];";
	code += "for(var aagamesi = 0; aagamesi < window.rgGames.length; aagamesi++)";
	code += "aagames.push({\"name\": window.rgGames[aagamesi][\"name\"], \"appid\": window.rgGames[aagamesi][\"appid\"]});";
	code += "document.location = 'http://www.aarcadeapicall.com.net.org/?doc=";
	code += requestId;
	code += "AAAPICALL' + encodeURIComponent(JSON.stringify(aagames));";
	//code += "AAAPICALL' + encodeURIComponent(JSON.stringify(aagames)).replace(/['()]/g, escape).replace(/\*/g, '%2A').replace(/%(?:7C|60|5E)/g, unescape);";
	//code += "AAAPICALL' + encodeURIComponent(JSON.stringify({'tester': window.aagames.length}));";
	return code;
	//code += "}";
	//code += "AAAPICALL' + encodeURIComponent(JSON.stringify(aagames));";

	//code += "aaGetSteamGames();";
	//code = "document.location = 'http://www.aarcadeapicall.com.net.org/?doc=AAAPICALL' + encodeURIComponent(JSON.stringify({'tester': window.rgGames.length}));";
}

//std::string C_AnarchyManager::ExtractYouTubeId(std::string url)
bool C_AnarchyManager::IsYouTube(std::string url)
{
	if (url == "")
		return false;

	if (url.find("http://www.anarchyarcade.com/youtube_player.php") == 0 || (url.find("youtube") != std::string::npos && url.find("v=") != std::string::npos) || (url.find("youtu.be/") != std::string::npos && url.find("&") != std::string::npos))
		return true;

	return false;
}

void C_AnarchyManager::ManageExtractOverview()
{
	if (m_fNextExtractOverviewCompleteManage == 0)
		return;

	// tick
	if (m_fNextExtractOverviewCompleteManage < engine->Time())
	{
		float fInterval = 0.5;
		m_fNextExtractOverviewCompleteManage = engine->Time() + fInterval;
		if (g_pFullFileSystem->FileExists(VarArgs("screenshots/overviews/%s.tga", this->MapName()), "DEFAULT_WRITE_PATH"))
		{
			//g_pFullFileSystem->FileExists()
			m_fNextExtractOverviewCompleteManage = 0;
			m_pMetaverseManager->OverviewExtracted();
		}
	}
}

void C_AnarchyManager::ManagePanoshot()
{
	if (m_panoshotState == PANO_NONE)
		return;

	// tick
	if (m_fNextPanoCompleteManage < engine->Time())
	{
		float fInterval = 0.5;
		m_fNextPanoCompleteManage = engine->Time() + fInterval;

		if (m_panoshotState == PANO_TAKE_SHOT_0 || m_panoshotState == PANO_TAKE_SHOT_1 || m_panoshotState == PANO_TAKE_SHOT_2 || m_panoshotState == PANO_TAKE_SHOT_3 || m_panoshotState == PANO_TAKE_SHOT_4 || m_panoshotState == PANO_TAKE_SHOT_5)
			engine->ExecuteClientCmd("jpeg;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_0)
			engine->ExecuteClientCmd("setang 0 0 0;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_1)
			engine->ExecuteClientCmd("setang 0 -90 0;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_2)
			engine->ExecuteClientCmd("setang 0 180 0;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_3)
			engine->ExecuteClientCmd("setang 0 90 0;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_4)
			engine->ExecuteClientCmd("setang 90 180 0;");
		else if (m_panoshotState == PANO_ORIENT_SHOT_5)
			engine->ExecuteClientCmd("setang -90 180 0;");
		else if (m_panoshotState == PANO_COMPLETE)
		{
			engine->ClientCmd("setang 0 0 0; fov 90;");

			// check that all 6 images exist
			std::string mapName = this->MapName();
			std::vector<std::string> panoshots;

			bool bExisted;
			FileFindHandle_t findHandle;
			const char* pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("screenshots/%s*.jpg", mapName.c_str()), "DEFAULT_WRITE_PATH", &findHandle);
			while (pFilename != NULL)
			{
				bExisted = false;

				for (unsigned int i = 0; i < m_existingMapScreenshotsForPano.size(); i++)
				{
					if (m_existingMapScreenshotsForPano[i] == std::string(pFilename))
					{
						bExisted = true;
						break;
					}
				}

				if (!bExisted)
				{
					//DevMsg("Pushing %s\n", pFilename);
					panoshots.push_back(std::string(pFilename));
				}
				//else
				//	DevMsg("NOT pushing %s\n", pFilename);

				//if (std::find(m_existingMapScreenshotsForPano.begin(), m_existingMapScreenshotsForPano.end(), std::string(pFilename)) == m_existingMapScreenshotsForPano.end())

				pFilename = g_pFullFileSystem->FindNext(findHandle);
			}
			g_pFullFileSystem->FindClose(findHandle);

			if (panoshots.size() == 6)
			{
				C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
				pHudBrowserInstance->SetUrl("asset://ui/panoview.html");
				g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true);

				// always use the same file names
				// screenshots/panoramic/pano/[image]
				g_pFullFileSystem->CreateDirHierarchy("screenshots/panoramic/pano", "DEFAULT_WRITE_PATH");

				/* DISABLED until the full panoramic screenshot system is implemented.
				// determine a folder name to create
				// screenshots/panoramic/[mapname][####]
				g_pFullFileSystem->CreateDirHierarchy("screenshots/panoramic", "DEFAULT_WRITE_PATH");

				unsigned int iGoodNumber = 0;
				while (g_pFullFileSystem->FileExists(VarArgs("screenshots/panoramic/%s%04d", mapName.c_str(), iGoodNumber), "MOD"))
				{
				iGoodNumber++;
				}
				*/

				std::vector<std::string> directions;
				directions.push_back("front");
				directions.push_back("right");
				directions.push_back("back");
				directions.push_back("left");
				directions.push_back("bottom");
				directions.push_back("top");

				for (unsigned int i = 0; i < panoshots.size(); i++)
				{
					// read the screenshot
					FileHandle_t fh = filesystem->Open(VarArgs("screenshots/%s", panoshots[i].c_str()), "rb", "DEFAULT_WRITE_PATH");
					if (fh)
					{
						int file_len = filesystem->Size(fh);
						unsigned char* pImageData = new unsigned char[file_len + 1];

						filesystem->Read((void*)pImageData, file_len, fh);
						pImageData[file_len] = 0; // null terminator

						filesystem->Close(fh);

						// write the screenshot
						// ORDER: FORWARD, RIGHT, BACK, LEFT, BOTTOM, TOP
						FileHandle_t fh2 = filesystem->Open(VarArgs("screenshots/panoramic/pano/%s.jpg", directions[i].c_str()), "wb", "DEFAULT_WRITE_PATH");
						if (fh2)
						{
							filesystem->Write(pImageData, file_len, fh2);
							filesystem->Close(fh2);
						}

						// cleanup
						delete[] pImageData;

						// SUCCESS!!

						// delete the old screenshot
						g_pFullFileSystem->RemoveFile(VarArgs("screenshots/%s", panoshots[i].c_str()), "DEFAULT_WRITE_PATH");

						// restore previous settings
						//g_pAnarchyManager->SetLastFOV(90);
						engine->ClientCmd(VarArgs("cl_drawhud %s; r_drawviewmodel %s; cl_hovertitles %s; cl_toastmsgs %s; developer %s; mat_hdr_level %s; mat_autoexposure_max %s; mat_autoexposure_min %s;", m_pPanoStuff->hud.c_str(), m_pPanoStuff->weapons.c_str(), m_pPanoStuff->titles.c_str(), m_pPanoStuff->toast.c_str(), m_pPanoStuff->developer.c_str(), m_pPanoStuff->hdrlevel.c_str(), m_pPanoStuff->exposuremax.c_str(), m_pPanoStuff->exposuremin.c_str()));
					}
				}

				m_panoshotState = PANO_NONE;
				m_existingMapScreenshotsForPano.clear();
			}
		}

		if (m_panoshotState != PANO_NONE && m_panoshotState != PANO_COMPLETE)
			m_panoshotState = static_cast<panoshotState>(static_cast<int>(m_panoshotState)+1);
	}
}

// TODO: Figure out how much of this needs to be done ONCE & then safely store the pointers.  NOTE: Don't forget about people unplugging all audio devices mid-game! (doh!)
void C_AnarchyManager::GetPeakAudio()
{
	if (!m_pMeterInfo)
	{
		HRESULT hr;
		IMMDeviceEnumerator *pEnumerator = NULL;
		IMMDevice *pDevice = NULL;
		IAudioMeterInformation *pMeterInfo = NULL;

		CoInitialize(NULL);

		// Get enumerator for audio endpoint devices.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
			NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IMMDeviceEnumerator),
			(void**)&pEnumerator);

		if (hr == S_OK)
		{
			// Get peak meter for default audio-rendering device.
			hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
			if (hr == S_OK)
			{
				hr = pDevice->Activate(__uuidof(IAudioMeterInformation),
					CLSCTX_ALL, NULL, (void**)&pMeterInfo);

				if (hr == S_OK)
				{
					m_pMeterInfo = pMeterInfo;
					/*
					static float peak = 0;
					hr = pMeterInfo->GetPeakValue(&peak);	// actually read the current peak
					if ( hr == S_OK)
					DevMsg("Tester: %f\n", peak);
					*/
				}
			}
		}
		//CoUninitialize();	// done in AA shutdown
	}

	if (m_pMeterInfo)
	{
		IAudioMeterInformation* pMeterInfo = static_cast<IAudioMeterInformation*>(m_pMeterInfo);

		static float peak = 0;
		HRESULT hr = pMeterInfo->GetPeakValue(&peak);
		if (hr == S_OK)
		{
			m_fAudioPeak = peak;
			//engine->ServerCmd(VarArgs("peak %02f"), false);
			engine->ClientCmd(VarArgs("peak %f", peak));
		}
	}
}

void C_AnarchyManager::Panoshot()
{
	if (m_panoshotState != PANO_NONE)
		return;

	m_existingMapScreenshotsForPano.clear();
	//m_fNextPanoCompleteManage = 0;
	float fInterval = 0.5;
	m_fNextPanoCompleteManage = engine->Time() + fInterval;

	// fill m_existingMapScreenshotsForPano with all existing screenshots for this map.
	std::string mapName = this->MapName();

	FileFindHandle_t findHandle;
	const char* pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("screenshots/%s*.jpg", mapName.c_str()), "DEFAULT_WRITE_PATH", &findHandle);
	if (pFilename != NULL)
	{
		m_existingMapScreenshotsForPano.push_back(std::string(pFilename));
		pFilename = g_pFullFileSystem->FindNext(findHandle);
	}
	g_pFullFileSystem->FindClose(findHandle);

	m_pPanoStuff->hud = cvar->FindVar("cl_drawhud")->GetString();
	m_pPanoStuff->weapons = cvar->FindVar("r_drawviewmodel")->GetString();
	m_pPanoStuff->titles = cvar->FindVar("cl_hovertitles")->GetString();
	m_pPanoStuff->toast = cvar->FindVar("cl_toastmsgs")->GetString();
	m_pPanoStuff->developer = cvar->FindVar("developer")->GetString();
	m_pPanoStuff->hdrlevel = cvar->FindVar("mat_hdr_level")->GetString();
	m_pPanoStuff->exposuremax = cvar->FindVar("mat_autoexposure_max")->GetString();
	m_pPanoStuff->exposuremin = cvar->FindVar("mat_autoexposure_min")->GetString();

	g_pAnarchyManager->SetLastFOV(106);
	engine->ExecuteClientCmd("firstperson; cl_drawhud 0; r_drawviewmodel 0; cl_hovertitles 0; cl_toastmsgs 0; developer 0; jpeg_quality 97; fov 106;");
	//g_pAnarchyManager->SetLastFOV(121);
	//engine->ExecuteClientCmd("firstperson; cl_drawhud 0; r_drawviewmodel 0; cl_hovertitles 0; cl_toastmsgs 0; developer 0; jpeg_quality 97; fov 121;");
	m_panoshotState = PANO_ORIENT_SHOT_0;
}

void C_AnarchyManager::TakeMediaScreenshot()
{
	//engine->ExecuteClientCmd("firstperson; cl_drawhud 0; r_drawviewmodel 0; cl_hovertitles 0; cl_toastmsgs 0; developer 0; jpeg_quality 97; fov 106;");
	DevMsg("Here yo.\n");
}

void C_AnarchyManager::DoneLoadingInitialObjectTextures()
{
	/*
	ConVar* pConVar = cvar->FindVar("engine_no_focus_sleep");
	pConVar->SetValue(g_pAnarchyManager->GetOldEngineNoFocusSleep().c_str());

	// Map has successfully loaded, so remember that.
	cvar->FindVar("last_map_loaded")->SetValue(1);
	engine->ClientCmd("host_writeconfig");
	*/

	//pHudBrowserInstance->SetUrl("asset://ui/default.html");
	ConVar* pHostNextMapConVar = cvar->FindVar("host_next_map");
	if (pHostNextMapConVar->GetBool())
	{
		pHostNextMapConVar->SetValue(false);

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl("asset://ui/hostSession.html?tab=host");
		//pHudBrowserInstance->SetUrl("asset://ui/hostSessionProgress.html");
	}
	else
	{
		if (m_bVRSuspending == 1)
			this->ToggleVR();

		//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

		std::vector<std::string> params;
		//params.push_back(cursorName);
		pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onImagesInitialized", params);
	}
}

void C_AnarchyManager::ImagesDoneLoading()
{
	if (m_bWaitingForInitialImagesToLoad)
	{
		m_bWaitingForInitialImagesToLoad = false;
		this->DoneLoadingInitialObjectTextures();
	}
}

///*
void C_AnarchyManager::OnVRSpazzFixCreated(int iIndex0, int iIndex1)
{
	m_iVRSpazzFix = iIndex0;//pBaseEntity;
	m_iVRSpazzFix2 = iIndex1;//pBaseEntity2;
}
//*/

int C_AnarchyManager::GetNumImagesLoading()
{
	C_AwesomiumBrowserInstance* pImagesBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("images");
	return pImagesBrowserInstance->GetNumImagesLoading();
}

void C_AnarchyManager::DoneSpawningInitialObjects()
{
	ConVar* pConVar = cvar->FindVar("engine_no_focus_sleep");
	pConVar->SetValue(g_pAnarchyManager->GetOldEngineNoFocusSleep().c_str());

	// Map has successfully loaded, so remember that.
	cvar->FindVar("last_map_loaded")->SetValue(1);
	engine->ClientCmd("host_writeconfig");

	if (!m_pWaitForInitialImagesConVar->GetBool() || this->GetConnectedUniverse() && !this->GetConnectedUniverse()->isHost)
		this->DoneLoadingInitialObjectTextures();
	//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Spawning Objects", "spawningobjects", "0", VarArgs("%i", g_pAnarchyManager->GetInstanceManager()->GetUnspawnedWithinRangeEstimate()), "+", "spawnNextObjectCallback");

	/*
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	JSValue value = pHudBrowserInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("textureWaitResponder"), WSLit(""));
	if (!value.IsUndefined())
	{
		std::vector<std::string> args;
		args.push_back(VarArgs("%i", this->GetNumImagesLoading()));
		pHudBrowserInstance->DispatchJavaScriptMethod("textureWaitResponder", "doneSpawningInitialObjects", args);
	}
	else
		this->DoneLoadingInitialObjectTextures();
	*/

	if (this->GetNumImagesLoading() > 0)
		m_bWaitingForInitialImagesToLoad = true;
	else
		this->DoneLoadingInitialObjectTextures();

	// let's start the quests in a few seconds from now, if we have any.
	if( !g_pAnarchyManager->GetQuestManager()->AreQuestsInitializing() )
		g_pAnarchyManager->StartQuestsSoon();

	// set rich presence
	std::string mapName = (engine->IsInGame()) ? this->MapName() : "";
	steamapicontext->SteamFriends()->SetRichPresence("mapname", mapName.c_str());
	steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", m_pInstanceManager->GetInstanceObjectCount()));
	std::string status = (this->GetConnectedUniverse()) ? "#Status_Multiplayer" : "#Status_Singleplayer";
	if (this->GetConnectedUniverse())
		steamapicontext->SteamFriends()->SetRichPresence("connect", VarArgs("+join %s", this->GetConnectedUniverse()->lobby.c_str()));
	else
		steamapicontext->SteamFriends()->SetRichPresence("connect", null);

	steamapicontext->SteamFriends()->SetRichPresence("steam_display", status.c_str());
}


void C_AnarchyManager::ShowEngineOptionsMenu()
{
	//// FIXME: If a map is loaded, input mode can be deactivated, but if at main menu that might be weird.
	//if (engine->IsInGame())
	//	g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	engine->ClientCmd("gamemenucommand OpenOptionsDialog\n");
}
/*
void C_AnarchyManager::BackwardsMinsMaxsModel(int iEntityIndex)
{
	C_BaseEntity* pEntity = C_BaseEntity::Instance(iEntityIndex);
	if (pEntity)
	{
		DevMsg("ERROR: Model has invalid materials: %s\n", MAKE_STRING(modelinfo->GetModelName(modelinfo->GetModel(pEntity->GetModelIndex()))));
	}
}
*/
void C_AnarchyManager::DeactivateObjectPlacementMode(bool confirm)
{
	C_PropShortcutEntity* pShortcut = m_pMetaverseManager->GetSpawningObjectEntity();
	if (pShortcut)
	{
		if (confirm)
		{
			engine->ClientCmd(VarArgs("makenonghost %i %i;\n", pShortcut->entindex(), this->UseBuildGhosts()));	// servercmdfix , false);

			/*
			pShortcut->SetRenderColorA(255);
			pShortcut->SetRenderMode(kRenderNormal);

			// make the prop solid
			pShortcut->SetSolid(SOLID_VPHYSICS);
			pShortcut->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
			pShortcut->SetMoveType(MOVETYPE_VPHYSICS);

			if (pShortcut->CreateVPhysics())
			{
				IPhysicsObject *pPhysics = pShortcut->VPhysicsGetObject();
				if (pPhysics)
				{
					pPhysics->EnableMotion(false);
				}
			}
			*/
		}
		else
		{
			// CANCEL
			this->GetMetaverseManager()->SetSpawningObjectEntity(null);
			this->GetMetaverseManager()->SetSpawningObject(null);

			//object_t* pObject = this->GetInstanceManager()->GetInstanceObject(pShortcut->GetObjectId());
			//if ( pObject )
			//	this->GetInstanceManager()->ResetObjectChanges(pShortcut);
			//else
			//this->GetInstanceManager()->RemoveEntity(pShortcut);
			this->GetInstanceManager()->ResetObjectChanges(pShortcut);	// this will also delete the object if there's nothing to revert to!!
		}

		m_pMetaverseManager->SetSpawningObjectEntity(null);
	}

	m_pMetaverseManager->SetSpawningObject(null);

	//C_AwesomiumBrowserInstance* pHudInstance = m_pAwesomiumBrowserManager->FindAwesomiumBrowserInstance("hud");
	//pHudInstance->SetUrl("asset://ui/default.html");

	m_pInputManager->DeactivateInputMode(true);
}

void C_AnarchyManager::ActivateObjectPlacementMode(C_PropShortcutEntity* pShortcut, const char* mode, bool bGamepadInputMode)
{
	m_pInstanceManager->AdjustObjectScale(pShortcut->GetModelScale());
	m_pMetaverseManager->ResetSpawningAngles();
	m_pMetaverseManager->SetSpawningRotationAxis(1);
	m_pMetaverseManager->SetSpawningObjectEntity(pShortcut);

	std::string objectId = pShortcut->GetObjectId();
	//DevMsg("At placement id is: %s\n", objectId.c_str());
	object_t* theObject = m_pInstanceManager->GetInstanceObject(objectId);
	//DevMsg("Object val: %s\n", theObject->itemId.c_str());
	m_pMetaverseManager->SetSpawningObject(theObject);

	bool bIsMoveMode = false;
	//std::string moreParams = "";
	if (!Q_strcmp(mode, "move"))
	{
		bIsMoveMode = true;

		//SetLibraryBrowserContext(std::string category, std::string id, std::string search, std::string filter)
		std::string category = "";
		KeyValues* itemKv = m_pMetaverseManager->GetLibraryItem(pShortcut->GetItemId());
		if (itemKv)
			category = "items";
		else
		{
			// no item found, probably a model
			// FIXME: This is just ASSUMING that it is a model.
			category = "models";
			//moreParams = "&title=" + 
			//KeyValues* modelKv = m_pMetaverseManager->GetLibraryModel(pShortcut->GetItemId());
			//if (modelKv)
			//	category = "models";
		}
		m_pMetaverseManager->SetLibraryBrowserContext(category, std::string("automove"), "", "");

		engine->ClientCmd(VarArgs("makeghost %i %i;\n", pShortcut->entindex(), g_pAnarchyManager->UseBuildGhosts()));	// servercmdfix , false);
		/*	// do this stuff server-side now
		pShortcut->SetSolid(SOLID_NONE);
		pShortcut->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		//SetRenderMode(kRenderTransTexture);
		pShortcut->SetRenderMode(kRenderTransColor);
		pShortcut->SetRenderColorA(160);
		*/

		//this->AddToastMessage("Move Mode");
	}
	//else
	//	this->AddToastMessage("Spawn Mode");

	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	//pHudInstance->SetUrl("asset://ui/cabinetSelect.html");
	//std::string category = this->GetMetaverseManager()->GetLibraryBrowserContext("category");
	//pHudInstance->SetUrl(VarArgs("asset://ui/buildMode.html?mode=spawn&category=%s", category.c_str()));

	pHudInstance->SetUrl(VarArgs("asset://ui/buildMode.html?mode=%s&itemId=%s&modelId=%s", mode, pShortcut->GetItemId().c_str(), pShortcut->GetModelId().c_str()));

	if (bGamepadInputMode)
		g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, null, false);

	/*
	// Figure out where to place it
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	Vector forward;
	pPlayer->EyeVectors(&forward);

	trace_t tr;
	UTIL_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_NPCSOLID,
		pPlayer, COLLISION_GROUP_NONE, &tr);

	// No hit? We're done.
	if (tr.fraction == 1.0)
		return;

	VMatrix entToWorld;
	Vector xaxis;
	Vector yaxis;

	if (tr.plane.normal.z == 0.0f)
	{
		yaxis = Vector(0.0f, 0.0f, 1.0f);
		CrossProduct(yaxis, tr.plane.normal, xaxis);
		entToWorld.SetBasisVectors(tr.plane.normal, xaxis, yaxis);
	}
	else
	{
		Vector ItemToPlayer;
		VectorSubtract(pPlayer->GetAbsOrigin(), Vector(tr.endpos.x, tr.endpos.y, tr.endpos.z), ItemToPlayer);

		xaxis = Vector(ItemToPlayer.x, ItemToPlayer.y, ItemToPlayer.z);

		CrossProduct(tr.plane.normal, xaxis, yaxis);
		if (VectorNormalize(yaxis) < 1e-3)
		{
			xaxis.Init(0.0f, 0.0f, 1.0f);
			CrossProduct(tr.plane.normal, xaxis, yaxis);
			VectorNormalize(yaxis);
		}
		CrossProduct(yaxis, tr.plane.normal, xaxis);
		VectorNormalize(xaxis);

		entToWorld.SetBasisVectors(xaxis, yaxis, tr.plane.normal);
	}

	QAngle angles;
	MatrixToAngles(entToWorld, angles);
	*/
}

void C_AnarchyManager::OnMountAllWorkshopsComplete()
{
	if (!m_pMountManager)	// it is our first time here
	{
		// SHOULD NEVER BE HTERE!!!!!
		// OBSOLETE!!!! 
		m_pMountManager = new C_MountManager();
		m_pMountManager->Init();
		m_pMountManager->LoadMountsFromKeyValues("mounts.txt");

		m_pBackpackManager->Init();

		m_pWorkshopManager = new C_WorkshopManager();
		m_pWorkshopManager->Init();
	}
	else
	{
		m_pBackpackManager->ActivateAllBackpacks();
		//this->GetMetaverseManager()->DetectAllMaps();

		unsigned int uCount = 0;

		if( !cvar->FindVar("detect_maps_at_startup")->GetBool())
			uCount = this->GetMetaverseManager()->LoadCachedMaps();

		if (uCount > 0)
		{
			DevMsg("Loaded %u maps from cache.\n", uCount);
			C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->GetSelectedAwesomiumBrowserInstance();
			pHudBrowserInstance->AddHudLoadingMessage("", "", VarArgs("Detected %u Cached Maps", uCount), "cachedmaps", "", "", "", "");
			this->GetMetaverseManager()->OnDetectAllMapsCompleted();
		}
		else
		{
			DevMsg("Failed to load maps from cache. Detecting all maps instead...\n");
			this->GetMetaverseManager()->DetectAllMaps();
		}
	}
}

void C_AnarchyManager::OnRebuildSoundCacheCallback()
{
	if (g_pFullFileSystem->FileExists("vanilla_aa_content.vpk", "MOD"))
		g_pFullFileSystem->AddSearchPath("vanilla_aa_content.vpk", "MOD");	// mount the vanilla stuff on top

	if (m_pAutoRebuildSoundCacheConVar->GetBool())
	{
		DevMsg("Reinitializing sound system (so sounds work in addon maps)...\n");
		//bool bIsInGame = engine->IsInGame();

		// restart the sound system so that mounted paths can play sounds
		engine->ClientCmd("snd_restart");
	}

	ConVar* pConVar = cvar->FindVar("engine_no_focus_sleep");
	pConVar->SetValue(m_oldEngineNoFocusSleep.c_str());

	// legacy fix for people with old interger style end behavior convar values
	std::string buf = m_pYouTubeEndBehaviorConVar->GetString();
	if (buf == "0")
		m_pYouTubeEndBehaviorConVar->SetValue("default");
	else if (buf == "1")
		m_pYouTubeEndBehaviorConVar->SetValue("close");
	else if (buf == "2")
		m_pYouTubeEndBehaviorConVar->SetValue("near");

	// force some F6-9 binds, for now.
	engine->ClientCmd("r_drawvgui 1");
	engine->ClientCmd("bind f6 favorites_menu; bind f7 commands_menu; bind f8 paint_menu; bind f9 players_menu; bind 7 slot7; bind 8 slot8; bind 9 slot9; bind 0 slot10;");

	C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->GetSelectedAwesomiumBrowserInstance();
	//pHudBrowserInstance->SetUrl("asset://ui/welcome.html");

	//if (!bIsInGame)
	ConVar* pFirstServerConVar = cvar->FindVar("firstserver");
	std::string firstServer = pFirstServerConVar->GetString();
	if (firstServer == "")
	{
		// This means we weren't launched with +join LOBBYID, so now check for the new ?join=LOBBYID format...
		std::string joinLobbyId = steamapicontext->SteamApps()->GetLaunchQueryParam("join");

		//DevMsg("Join Lobby ID is: %s\n", joinLobbyId.c_str());
		if (joinLobbyId != "")
		{
			pFirstServerConVar->SetValue(joinLobbyId.c_str());
			firstServer = joinLobbyId;
		}
		else
			pHudBrowserInstance->SetUrl("asset://ui/welcome.html");
	}

	g_pAnarchyManager->SetInitialized(true);

	this->InitDragDrop(m_hwnd);

	std::string ffmpegGame = engine->GetGameDirectory();
	ffmpegGame += "\\resource\\init.mpeg";	// Note that this file does nto actually exist, but that's OK cuz all we are doing is initializing libretro.
	std::string ffmpegCore = "ffmpeg_libretro.dll";

	//DevMsg("Game: %s\n", ffmpegGame.c_str());
	//DevMsg("Core: %s\n", ffmpegCore.c_str());
	
	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->CreateLibretroInstance();
	pLibretroInstance->Init("init", "Initializing Libretro...", -1);
	pLibretroInstance->SetOriginalGame(ffmpegGame);
	//pLibretroInstance->SetOriginalItemId(itemId);
	pLibretroInstance->SetOriginalEntIndex(-1);	// probably NOT needed?? (or maybe so, from here.)

	pLibretroInstance->LoadCore(ffmpegCore);
	//if (!pLibretroInstance->LoadCore(ffmpegCore))	// FIXME: elegantly revert back to autoInspect if loading the core failed!
	//	DevMsg("ERROR: Failed to load core: %s\n", ffmpegCore.c_str());

	//m_pLibretroManager->DestroyLibretroInstance(pLibretroInstance);
	//ToastSlate->Create(enginevgui->GetPanel(PANEL_ROOT));

	// if a map is ALREADY loaded, then we had a background map loaded, so lets initialize it.
	/*
	if (bIsInGame)
	{
		this->LevelInitPreEntity();
		this->LevelInitPostEntity();
		this->GetInputManager()->DeactivateInputMode(true);
	}
	*/

	m_flStartTime = engine->Time();
	m_flStatTime = engine->Time() + (60 * 60);

	if (firstServer != "")
	{
		pFirstServerConVar->SetValue("");
		this->JoinLobby(firstServer);
	}
}

void C_AnarchyManager::OnDetectAllMapsComplete()
{
	//DevMsg("DISABLED FOR TESTING!\n");
	//return;
	///*

	//DevMsg("Starting Libretro...\n");
	//m_pLibretroManager = new C_LibretroManager();

	if (m_iState < 1)
	{
		m_iState = 1;

		g_pAnarchyManager->GetMetaverseManager()->DetectAllMapScreenshots();

		// iterate through all models and assign the dynamic property to them
		// FIXME: THIS SHOULD BE DONE UPON MODEL IMPORT/LOADING!!
		m_pMetaverseManager->FlagDynamicModels();
		m_pMetaverseManager->DetectAllLegacyCabinets();

		// this is where steamGames.key could be auto-scanned to make sure all Steam games exist in the library, if wanted.
		if (false && g_pFullFileSystem->FileExists("steamGames.key", "DEFAULT_WRITE_PATH"))
		{
			KeyValues* kv = new KeyValues("steamgames");
			kv->LoadFromFile(g_pFullFileSystem, "steamGames.key", "DEFAULT_WRITE_PATH");
			g_pAnarchyManager->GetMetaverseManager()->ImportSteamGames(kv);
		}

		DevMsg("Initializing sound system...\n");
		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->GetSelectedAwesomiumBrowserInstance();
		pHudBrowserInstance->AddHudLoadingMessage("", "", "Initializing Sound System...", "rebuildsound", "", "", "", "rebuildSoundCacheCallback");
	}
	else
	{
		DevMsg("Done again!!\n");

		C_AwesomiumBrowserInstance* pHudBrowserInstance = m_pAwesomiumBrowserManager->GetSelectedAwesomiumBrowserInstance();
		pHudBrowserInstance->AddHudLoadingMessage("", "", "Finished Importing Maps", "detectmapscomplete", "", "", "", "");

		std::vector<std::string> args;
		pHudBrowserInstance->DispatchJavaScriptMethod("eventListener", "doneDetectingMaps", args);
	}
	//*/
}

void C_AnarchyManager::Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters)
{
	std::string safeStr = str;
	std::transform(safeStr.begin(), safeStr.end(), safeStr.begin(), ::tolower);

	// Skip delimiters at beginning.
	std::string::size_type lastPos = safeStr.find_first_not_of(delimiters, 0);

	// Find first "non-delimiter".
	std::string::size_type pos = safeStr.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));

		// Skip delimiters.  Note the "not_of"
		lastPos = safeStr.find_first_not_of(delimiters, pos);

		// Find next "non-delimiter"
		pos = safeStr.find_first_of(delimiters, lastPos);
	}
}

/*
void C_AnarchyManager::ReleaseFileBrowseParams()
{
	if (m_pFileParams)
	{
		delete m_pFileParams;
		m_pFileParams = null;
	}
}
*/

void C_AnarchyManager::xCastSetLiveURL()
{
	//if (!m_pXSplitLiveConVar->GetBool())
	//	return;

	//DevMsg("Do anarchybot stuff\n");
	std::string XSPlitLiveFolder = cvar->FindVar("broadcast_folder")->GetString();
	if (XSPlitLiveFolder == "")
		return;

	std::string xml = "";
	xml += "<div class=\"response\">\n";

	KeyValues* active = null;
	C_BaseEntity* pEntity = this->GetSelectedEntity();
	if (pEntity)
	{
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
		if (pShortcut)
		{
			KeyValues* pItem = m_pMetaverseManager->GetLibraryItem(pShortcut->GetItemId());
			if (pItem)
				active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItem);
		}
	}
	else
	{
		// check for other open instances
		// TODO: This code should also be used for slave screens to show other instances!!

		std::vector<C_EmbeddedInstance*> embeddedInstances;
		m_pLibretroManager->GetAllInstances(embeddedInstances);
		m_pSteamBrowserManager->GetAllInstances(embeddedInstances);
		m_pAwesomiumBrowserManager->GetAllInstances(embeddedInstances);

		C_EmbeddedInstance* pEmbeddedInstance;
		std::string originalItemId;
		unsigned int i;
		unsigned int size = embeddedInstances.size();
		for (i = 0; i < size; i++)
		{
			pEmbeddedInstance = embeddedInstances[i];
			if (!pEmbeddedInstance)
				continue;

			// ignore special instances
			if (pEmbeddedInstance->GetId() == "hud" || pEmbeddedInstance->GetId() == "network" || pEmbeddedInstance->GetId() == "images")
				continue;

			// check for an original item id
			originalItemId = pEmbeddedInstance->GetOriginalItemId();
			if (originalItemId != "")
			{
				// WE HAVE FOUND AN EMBEDDED INSTANCE THAT WAS ORIGINALLY CREATED BY AN ITEM!!

				// make sure we can find an item for it
				KeyValues* pItem = m_pMetaverseManager->GetLibraryItem(originalItemId);
				if (pItem)
				{
					active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItem);

					if ( active )
						break;
				}
			}
		}
	}

	if (active)
	{
		xml += "\t<item class=\"item_container\">\n";

		// automatically add all root-level keys & values
		for (KeyValues *sub = active->GetFirstValue(); sub; sub = sub->GetNextValue())
		{
			if (sub->GetFirstSubKey() || !Q_strcmp(sub->GetName(), "") || !Q_strcmp(sub->GetString(), ""))
				continue;

			xml += "\t\t<";
			xml += sub->GetName();
			xml += ">";

			std::string xmlBuf = sub->GetString();
			size_t found = xmlBuf.find("&");
			while (found != std::string::npos)
			{
				xmlBuf.replace(found, 1, "&amp;");
				found = xmlBuf.find("&", found + 5);
			}

			found = xmlBuf.find("<");
			while (found != std::string::npos)
			{
				xmlBuf.replace(found, 1, "&lt;");
				found = xmlBuf.find("<", found + 4);
			}

			found = xmlBuf.find(">");
			while (found != std::string::npos)
			{
				xmlBuf.replace(found, 1, "&gt;");
				found = xmlBuf.find(">", found + 4);
			}

			xml += xmlBuf;

			xml += "</";
			xml += sub->GetName();
			xml += ">\n";
		}

		// do some extra work to get the best possible image results
		std::string bestScreenImage = active->GetString("screen");
		if (bestScreenImage == "" || bestScreenImage.find(":") == 1 || bestScreenImage.find("http") != 0)
			bestScreenImage = active->GetString("marquee");
		if (bestScreenImage == "" || bestScreenImage.find(":") == 1 || bestScreenImage.find("http") != 0)
			bestScreenImage = active->GetString("file");
		if (bestScreenImage == "" || bestScreenImage.find(":") == 1 || bestScreenImage.find("http") != 0)
			bestScreenImage = "noimage.png";

		xml += "\t\t<screen2use>";
		xml += bestScreenImage;
		xml += "</screen2use>\n";

		std::string bestMarqueeImage = active->GetString("marquee");
		if (bestMarqueeImage == "" || bestMarqueeImage.find(":") == 1 || bestMarqueeImage.find("http") != 0)
			bestMarqueeImage = active->GetString("screen");
		if (bestMarqueeImage == "" || bestMarqueeImage.find(":") == 1 || bestMarqueeImage.find("http") != 0)
			bestMarqueeImage = active->GetString("file");
		if (bestMarqueeImage == "" || bestMarqueeImage.find(":") == 1 || bestMarqueeImage.find("http") != 0)
			bestMarqueeImage = "noimage.png";

		xml += "\t\t<marquee2use>";
		xml += bestMarqueeImage;
		xml += "</marquee2use>\n";

		xml += "\t\t<screenurl>";
		xml += active->GetString("screen");
		xml += "</screenurl>\n";

		xml += "\t\t<marqueeurl>";
		xml += active->GetString("marquee");
		xml += "</marqueeurl>\n";

		xml += "\t\t<bestimageurl>";
		if (bestMarqueeImage != "")
			xml += bestMarqueeImage;
		else
			xml += bestScreenImage;
		xml += "</bestimageurl>\n";


		xml += "\t\t<isremember>0</isremember>\n";
		xml += "\t</item>\n";
	}

	xml += "</div>";

	// Write this live URL out to the save file.
	FileHandle_t hFile = g_pFullFileSystem->Open(VarArgs("%s\\liveItems.xml", XSPlitLiveFolder.c_str()), "w", "");

	if (hFile)
	{
		//DevMsg("Writing to liveItems:\n");
		//DevMsg("%s\n", xml.c_str());

		/*
		// null terminate
		int len = xml.length();
		char* stuff = new char[xml.length() + 1];
		Q_strcpy(stuff, xml.c_str());
		stuff[len] = 0;	// null terminator
		*/

		//std::stringstream buffer;
		//buffer << "Text" << std::endl;

		g_pFullFileSystem->FPrintf(hFile, "%s", xml.c_str());
		g_pFullFileSystem->Close(hFile);
	}

	// Also update a JS file to force the page to re-load
	hFile = g_pFullFileSystem->Open(VarArgs("%s\\vote.js", XSPlitLiveFolder.c_str()), "a+", "");
	if (hFile)
	{
		std::string selectedItemTitle = "";
		if (active)
			selectedItemTitle = active->GetString("info/id");

		std::string rememberedItemTitle = "";
		//if (pRememberItemKV)
		//	rememberedItemTitle = pRememberItemKV->GetString("itemfilehash");

		std::string code = "gAnarchyTV.OnAArcadeCommand(\"selection\", \"";
		code += selectedItemTitle;
		code += "\", \"";
		code += rememberedItemTitle;
		code += "\");\n";

		//g_pFullFileSystem->FPrintf(hFile, VarArgs("var currentTick = \"%i\";\n", gpGlobals->tickcount));
		g_pFullFileSystem->Write(code.c_str(), code.length(), hFile);

		g_pFullFileSystem->Close(hFile);
	}

	//DevMsg("Done doing anarchybot stuff!!\n");
}

void C_AnarchyManager::TestSQLite2()
{
	DevMsg("LOAD SQL TEST...\n");
	int rc;
	char *error;

	// Open Database
	DevMsg("Opening MyDb.db ...\n");

	sqlite3 *db;
	rc = sqlite3_open("MyDb.db", &db);
	if (rc)
	{
		DevMsg("Error opening SQLite3 database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
		//return 1;
	}
	else
	{
		DevMsg("Opened MyDb.db.\n");
	}

	// Execute SQL
	sqlite3_stmt *stmt = NULL;
	rc = sqlite3_prepare(db, "SELECT * from MyTable", -1, &stmt, NULL);
	//rc = sqlite3_prepare(db, "INSERT INTO MyTable VALUES(NULL, ?)", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(db));

	int length;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		KeyValues* pTesterKV = new KeyValues("anotherTester");
		CUtlBuffer buf(0, length, 0);	// the length param should NEVER be zero.
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pTesterKV->ReadAsBinary(buf);
		buf.Purge();
		DevMsg("Value here is: %s\n", pTesterKV->GetString("originalTesterKey"));

		/*
		void* buffer = malloc(length);
		memcpy(buffer, sqlite3_column_blob(stmt, 0), length);
		buf.Put(buffer, length);
		pTesterKV->ReadAsBinary(buf);
		DevMsg("Buf value has: %s\n", pTesterKV->GetString("originalTesterKey"));
		pTesterKV->deleteThis();
		*/

	//	buf.Put
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.

	// Close Database
	DevMsg("Closing MyDb.db ...\n");
	sqlite3_close(db);
	DevMsg("Closed MyDb.db\n");
}

void C_AnarchyManager::SelectNext()
{
	if (cvar->FindVar("attract_mode_active")->GetBool())
	{
		this->FindNextAttractCamera();
		return;
	}
	
	C_PropShortcutEntity* pSelectedEntity = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);
	//if (!pSelectedEntity)
	//	return;

	C_PropShortcutEntity* pOriginalEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(m_iOriginalSelect));
	//if (!pOriginalEntity)
	//	return;
	
	// if the user is not in attract camera mode yet, but there is an object selected, use the selected object as the original object.
	if (!pOriginalEntity && pSelectedEntity)
	{
		this->SetSelectOriginal(pSelectedEntity->entindex());
		pOriginalEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(m_iOriginalSelect));
	}
	else if (!pSelectedEntity && pOriginalEntity)
	{
		this->SetSelectOriginal(-1);
		pOriginalEntity = null;
	}

	m_iLastNextDirection = 1;
	m_pInstanceManager->SelectNext(pSelectedEntity, pOriginalEntity);
}

void C_AnarchyManager::SelectPrev()
{
	if (cvar->FindVar("attract_mode_active")->GetBool())
	{
		this->FindPreviousAttractCamera();
		return;
	}

	C_PropShortcutEntity* pSelectedEntity = dynamic_cast<C_PropShortcutEntity*>(m_pSelectedEntity);
	//if (!pSelectedEntity)
	//	return;

	C_PropShortcutEntity* pOriginalEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(m_iOriginalSelect));
	//if (!pOriginalEntity)
	//	return;
	
	// if the user is not in attract camera mode yet, but there is an object selected, use the selected object as the original object.
	if (!pOriginalEntity && pSelectedEntity)
	{
		this->SetSelectOriginal(pSelectedEntity->entindex());
		pOriginalEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(m_iOriginalSelect));
	}
	else if (!pSelectedEntity && pOriginalEntity)
	{
		this->SetSelectOriginal(-1);
		pOriginalEntity = null;
	}

	m_iLastNextDirection = -1;

	m_pInstanceManager->SelectPrev(pSelectedEntity, pOriginalEntity);
}

void C_AnarchyManager::BotCheer(std::string text)
{
	// find the victim object
	if (this->IsPaused() || !g_pAnarchyManager->MapName())
		return;

	if (text == "")
		engine->ClientCmd("spawn_junk");
	else
	{
		std::string paramValue = text;
		std::transform(paramValue.begin(), paramValue.end(), paramValue.begin(), ::toupper);

		size_t found = paramValue.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
		while (found != std::string::npos)
		{
			paramValue.replace(found, 1, "");
			found = paramValue.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
		}

		if (paramValue == "")
			engine->ClientCmd("spawn_junk");
		else
			engine->ClientCmd(VarArgs("chat_ball \"%s\"\n", paramValue.c_str()));
	}
}

void C_AnarchyManager::UpdateItemTextOnObjects(std::string itemId, std::string text)
{
	std::vector<int> victims;
	
	std::map<std::string, object_t*> objectsMap = m_pInstanceManager->GetObjectsMap();

	auto it = objectsMap.begin();
	while (it != objectsMap.end())
	{
		if (it->second->itemId == itemId)
			victims.push_back(it->second->entityIndex);

		it++;
	}

	std::string paramValue = text;
	std::transform(paramValue.begin(), paramValue.end(), paramValue.begin(), ::toupper);

	for (unsigned int i = 0; i < victims.size(); i++)
	{
		//DevMsg("ParamValue: %s\n", paramValue.c_str());

		engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", victims[i], paramValue.c_str()));
	}
}

void C_AnarchyManager::ResetStatTimers()
{
	m_iTotalHours = 0;
	m_flStartTime = m_flCurrentTime;
	m_flStatTime = m_flCurrentTime + (60 * 60);
}

bool C_AnarchyManager::ShouldShowWindowsTaskBar()
{
	return (m_pShouldShowWindowsTaskBarConVar) ? m_pShouldShowWindowsTaskBarConVar->GetBool() : false;
}

void C_AnarchyManager::SetNextLoadInfo(std::string instanceId, std::string position, std::string rotation)
{
	m_pNextLoadInfo->instanceId = instanceId;
	m_pNextLoadInfo->position = position;
	m_pNextLoadInfo->rotation = rotation;
}

std::string C_AnarchyManager::GetHomeURL()
{
	std::string uri = cvar->FindVar("web_home")->GetString();
	if (uri == "" || (uri.find("http:") != 0 && uri.find("https:") != 0 && uri.find("HTTP:") != 0 && uri.find("HTTPS:") != 0))
	{
		uri = "file://";
		uri += engine->GetGameDirectory();
		//uri += "/resource/ui/html/autoInspectItem.html?mode=" + g_pAnarchyManager->GetAutoInspectMode() + "&id=" + encodeURIComponent(itemId) + "&title=" + encodeURIComponent(pItemKV->GetString("title")) + "&screen=" + encodeURIComponent(pItemKV->GetString("screen")) + "&marquee=" + encodeURIComponent(pItemKV->GetString("marquee")) + "&preview=" + encodeURIComponent(pItemKV->GetString("preview")) + "&reference=" + encodeURIComponent(pItemKV->GetString("reference")) + "&file=" + encodeURIComponent(pItemKV->GetString("file"));
		uri += "/resource/ui/html/anarchyPortal.html";
	}

	return uri;
}

void C_AnarchyManager::WaitForOverviewExtract()
{
	float fInterval = 0.5;
	m_fNextExtractOverviewCompleteManage = engine->Time() + fInterval;
}

void C_AnarchyManager::ClearConnectedUniverse()
{
	if (m_pConnectedUniverse)
	{
		delete m_pConnectedUniverse;
		m_pConnectedUniverse = null;
	}

	m_numberStatsState.iServerVisitors = 0;
	m_numberStatsState.iServerVisits = 0;

	ConVar* pOldAutoSaveConVar = cvar->FindVar("old_auto_save");
	int oldAutoSave = pOldAutoSaveConVar->GetInt();
	pOldAutoSaveConVar->SetValue(-1);
	if (oldAutoSave >= 0)
		cvar->FindVar("auto_save")->SetValue(oldAutoSave);

	m_pMetaverseManager->ClearNetworkDictionaries();

	/*
	m_pConnectedUniverse->connected = false;

	if (m_pConnectedUniverse->address != "")
		m_pConnectedUniverse->address = "";

	if (m_pConnectedUniverse->universe != "")
		m_pConnectedUniverse->universe = "";

	if (m_pConnectedUniverse->instance != "")
		m_pConnectedUniverse->instance = "";

	if (m_pConnectedUniverse->session != "")
		m_pConnectedUniverse->session = "";
		*/
}

void C_AnarchyManager::SetConnectedUniverse(bool bConnected, bool bIsHost, std::string address, std::string universeId, std::string instanceId, std::string sessionId, std::string lobbyId, bool bPublic, bool bPersistent, std::string lobbyPassword)
{
	if (m_pConnectedUniverse)
		delete m_pConnectedUniverse;

	bool bTestSandboxBuildMode = false;

	m_pConnectedUniverse = new aampConnection_t();
	m_pConnectedUniverse->connected = bConnected;
	m_pConnectedUniverse->isHost = bIsHost;
	m_pConnectedUniverse->canBuild = (m_pConnectedUniverse->isHost || bTestSandboxBuildMode);
	m_pConnectedUniverse->address = address;
	m_pConnectedUniverse->universe = universeId;
	m_pConnectedUniverse->instance = instanceId;
	m_pConnectedUniverse->session = sessionId;
	m_pConnectedUniverse->lobby = lobbyId;
	m_pConnectedUniverse->isPublic = bPublic;
	m_pConnectedUniverse->isPersistent = bPersistent;
	m_pConnectedUniverse->lobbyPassword = lobbyPassword;

	if (!bConnected)
	{
		m_numberStatsState.iServerVisitors = 0;
		m_numberStatsState.iServerVisits = 0;
	}
}

void C_AnarchyManager::TestSQLite()
{
	int rc;
	char *error;

	// create the library database
	DevMsg("Create library database...\n");

	sqlite3 *db;
	rc = sqlite3_open("aarcade_user/library.db", &db);
	if (rc)
	{
		DevMsg("Error opening SQLite3 database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
	}

	// create the tables
	const char *sqlCreateAppsTable = "CREATE TABLE apps (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateAppsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreateItemsTable = "CREATE TABLE items (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateItemsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreateMapsTable = "CREATE TABLE maps (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateMapsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreateModelsTable = "CREATE TABLE models (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateModelsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreatePlatformsTable = "CREATE TABLE platforms (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreatePlatformsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreateTypesTable = "CREATE TABLE types (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateTypesTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	const char *sqlCreateInstancesTable = "CREATE TABLE instances (id INTEGER PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(db, sqlCreateInstancesTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}

	// FIXME: TODO: Add the default stuff to the database by default?  or maybe this should be done through a higher-level system.

	// DATABASE IS NOW CREATED WITH THE PROPER TABLES, BUT IS COMPLETELY EMPTY.
	sqlite3_close(db);
	DevMsg("Library created.\n");
	// Execute SQL
	/*
	DevMsg("Inserting a value into MyTable ...\n");
	const char *sqlInsert = "INSERT INTO MyTable VALUES(NULL, 'A Value');";
	rc = sqlite3_exec(db, sqlInsert, NULL, NULL, &error);
	if (rc)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}
	else
	{
		DevMsg("Inserted a value into MyTable.\n");
	}
	*/
	/*
	DevMsg("Inserting a BINARY value into MyTable ...\n");
	sqlite3_stmt *stmt = NULL;
	rc = sqlite3_prepare(db,
		"INSERT INTO MyTable VALUES(NULL, ?)", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed!\n");

	// SQLITE_STATIC because the statement is finalized
	// before the buffer is freed:

	CUtlBuffer buf;
	KeyValues* pObjectKV = new KeyValues("originalTester");//pInstanceObjectsKV->FindKey(VarArgs("%s/local", objectId.c_str()), true);
	pObjectKV->SetString("originalTesterKey", "yup");
	pObjectKV->WriteAsBinary(buf);
	pObjectKV->deleteThis();

	int size = buf.Size();

	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("bind failed: %s\n", sqlite3_errmsg(db));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("execution failed: %s\n", sqlite3_errmsg(db));
		else
			DevMsg("execution worked!\n");
	}

	sqlite3_finalize(stmt);
	*/
	/*
	// Display MyTable
	DevMsg("Retrieving values in MyTable ...\n");
	const char *sqlSelect = "SELECT * FROM MyTable;";
	char **results = NULL;
	int rows, columns;
	sqlite3_get_table(db, sqlSelect, &results, &rows, &columns, &error);
	if (rc)
	{
		DevMsg("Error executing SQLite3 query: %s\n", sqlite3_errmsg(db));
		sqlite3_free(error);
	}
	else
	{
		// Display Table
		for (int rowCtr = 0; rowCtr <= rows; ++rowCtr)
		{
			for (int colCtr = 0; colCtr < columns; ++colCtr)
			{
				// Determine Cell Position
				int cellPosition = (rowCtr * columns) + colCtr;

				// Display Cell Value
				DevMsg("%s\t", results[cellPosition]);
				//cout.width(12);
				//cout.setf(ios::left);
				//cout << results[cellPosition] << " ";
			}

			// End Line
			//cout << endl;
			DevMsg("\n");

			// Display Separator For Header
			if (0 == rowCtr)
			{
				for (int colCtr = 0; colCtr < columns; ++colCtr)
				{
					//cout.width(12);
					//cout.setf(ios::left);
					DevMsg("~~~~~~~~~~~~");
				}
				DevMsg("\n");
			}
		}
	}
	sqlite3_free_table(results);
	*/

	// Close Database
	//DevMsg("Closing MyDb.db ...\n");
	//sqlite3_close(db);
	//DevMsg("Closed MyDb.db\n");


	/*
		CUtlBuffer buf;
		//buf.Get()
		KeyValues* pObjectKV = new KeyValues("originalTester");//pInstanceObjectsKV->FindKey(VarArgs("%s/local", objectId.c_str()), true);
		pObjectKV->SetString("originalTesterKey", "yup");
		pObjectKV->WriteAsBinary(buf);
		pObjectKV->deleteThis();

		KeyValues* pTesterKV = new KeyValues("reduxTester");
		pTesterKV->ReadAsBinary(buf);
		DevMsg("Annd here the big result is: %s\n", pTesterKV->GetString("originalTesterKey"));
		pTesterKV->deleteThis();
	*/
}

/*
void C_AnarchyManager::LevelInitPreEntity()
{
	DevMsg("AnarchyManager: LevelInitPreEntity\n");
	//m_pWebViewManager = new C_WebViewManager;
//	m_pWebViewManager->Init();
}

void C_AnarchyManager::LevelShutdownPostEntity()
{
	DevMsg("AnarchyManager: LevelShutdownPostEntity\n");
	// FIXME: Deleting the webview manager prevents it from starting up again.
	// Need to only create/delete it ONCE during the lifetime of AArcade.
	//delete m_pWebViewManager;
}
*/
/*
const char* C_AnarchyManager::GenerateHash(const char* text)
{
	char input[AA_MAX_STRING];
	Q_strcpy(input, text);

	// Convert it to lowercase & change all slashes to back-slashes
	V_FixSlashes(input);
	for( int i = 0; input[i] != '\0'; i++ )
		input[i] = tolower(input[i]);

	char lower[256];
	unsigned m_crc = 0xffffffff;

	int inputLength = strlen(input);
	for (int i = 0; i < inputLength; i++)
	{
		lower[i] = tolower(input[i]);
	}

	for (int i = 0; i < inputLength; i++)
	{
		unsigned c = lower[i];
		m_crc ^= (c << 24);

		for (int j = 0; j < 8; j++)
		{
			const unsigned FLAG = 0x80000000;
			if ((m_crc & FLAG) == FLAG)
			{
				m_crc = (m_crc << 1) ^ 0x04C11DB7;
			}
			else
			{
				m_crc <<= 1;
			}
		}
	}

	return VarArgs("%08x", m_crc);
}
*/