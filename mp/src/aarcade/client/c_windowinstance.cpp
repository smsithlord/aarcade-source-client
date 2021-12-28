#include "cbase.h"
#include "c_windowinstance.h"
//#include "aa_globals.h"
#include "c_anarchymanager.h"
//#include "c_embeddedinstance.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_WindowInstance::C_WindowInstance()
{
	DevMsg("WindowInstance: Constructor\n");
	m_title = "";
	m_id = "";
	m_hwnd = null;
	m_bHidden = false;
	m_bPresetHidden = false;
	m_pTexture = null;
	m_iLastRenderedFrame = -1;
	m_iLastVisibleFrame = -1;
	m_bIsDirty = false;
}

C_WindowInstance::~C_WindowInstance()
{
	DevMsg("WindowInstance: Destructor\n");
}


void C_WindowInstance::SelfDestruct()
{
	DevMsg("WindowInstance: SelfDestruct %s\n", m_id.c_str());

	/*
	if (g_pAnarchyManager->ShouldAllowMultipleActive() && g_pAnarchyManager->IsLevelInitialized())
	{
		C_BaseEntity* pOriginalEntity = C_BaseEntity::Instance(m_iOriginalEntIndex);
		if (pOriginalEntity)
		{
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pOriginalEntity);
			pShortcut->PlaySequenceRegular("deactivated");
		}
	}
	*/

	delete this;
}

void C_WindowInstance::Close()
{
	g_pAnarchyManager->GetWindowManager()->DestroyWindowInstance(this);
}

void C_WindowInstance::Init(std::string id, HWND hwnd, std::string title, std::string className)
{
	std::string goodTitle = (title != "") ? title : "Untitled Window Instance";
	m_title = title;
	m_className = className;
	m_id = id;

	if (m_id == "")
		m_id = g_pAnarchyManager->GenerateUniqueId();

	m_hwnd = hwnd;

	// automatically hide instances listed in the "autoHideWindows.txt" file
	m_bHidden = g_pAnarchyManager->GetWindowManager()->IsPresetHiddenWindow(m_className, m_title);
	m_bPresetHidden = m_bHidden;

	g_pAnarchyManager->GetWindowManager()->AddInstance(this);

	if (!m_bHidden && !m_bPresetHidden)
	{

		// create the texture (each instance has its own texture)
		std::string textureName = "canvas_";
		textureName += m_id;

		int iWidth = AA_EMBEDDED_INSTANCE_WIDTH;
		int iHeight = AA_EMBEDDED_INSTANCE_HEIGHT;

		int flags = (0x0100 | 0x0200 | 0x0800 | 0x2000000);

		if (g_pAnarchyManager->ShouldTextureClamp())
			flags |= (0x0004 | 0x0008);

		int multiplyer = 1.0;// g_pAnarchyManager->GetDynamicMultiplyer();
		if (!g_pMaterialSystem->IsTextureLoaded(textureName.c_str()))
			m_pTexture = g_pMaterialSystem->CreateProceduralTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, iWidth * multiplyer, iHeight * multiplyer, IMAGE_FORMAT_RGBA8888, flags);//IMAGE_FORMAT_BGR888
		else
		{
			m_pTexture = g_pMaterialSystem->FindTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, false, 1);
			g_pAnarchyManager->GetCanvasManager()->TextureNotDeferred(m_pTexture);
		}

		// get the regen and assign it
		CCanvasRegen* pRegen = g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen();
		//pRegen->SetEmbeddedInstance(this);
		m_pTexture->SetTextureRegenerator(pRegen);
	}

	/*
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
	*/
}

void C_WindowInstance::CleanUpTexture()
{
	if (m_pTexture)
	{
		m_pTexture->SetTextureRegenerator(null);

		DevMsg("Unref texture from: C_WindowInstance::CleanUpTexture\n");
		g_pAnarchyManager->GetCanvasManager()->UnreferenceEmbeddedInstance(this);
		g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(m_pTexture);
		g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(m_pTexture);
		m_pTexture = null;
	}
}

void C_WindowInstance::Render()
{
	//	if (m_id == "images")
	//	return;
	//DevMsg("Rendering texture: %s\n", m_pTexture->GetName());
	//	DevMsg("Render Web Tab: %s\n", this->GetTexture()->Ge>GetId().c_str());
	//DevMsg("WebTab: Render: %s on %i\n", m_id.c_str(), gpGlobals->framecount);
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(this);
	m_pTexture->Download();
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(null);

	m_iLastRenderedFrame = gpGlobals->framecount;

	g_pAnarchyManager->GetCanvasManager()->AllowRender(this);
	//if (g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(this))
	//	g_pAnarchyManager->GetCanvasManager()->SetLastPriorityRenderedFrame(gpGlobals->framecount);
	//else
	//	g_pAnarchyManager->GetCanvasManager()->SetLastRenderedFrame(gpGlobals->framecount);
}

void C_WindowInstance::RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect)
{
	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;

	//this->CopyLastFrame(pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
	DevMsg("Regenerate the texture bits of the Window: %s\n", m_title.c_str());

	int iWidth, iHeight;
	BYTE* pBuffer = g_pAnarchyManager->CaptureWindowSnapshot(this->GetHWND(), iWidth, iHeight);
	//	this->CopyLastFrame(pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
	this->ResizeFrameFromRGBA8888(pBuffer, pVTFTexture->ImageData(0, 0, 0), iWidth, iHeight, iWidth * 4, 4, pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
	delete[] pBuffer;

	m_bIsDirty = false;
}

void C_WindowInstance::ResizeFrameFromRGBA8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth)
{
	//DevMsg("Thread ID: %u\n", ThreadGetCurrentId);
	//	uint uId = ThreadGetCurrentId();
	//	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	//	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();
	//LibretroInstanceInfo_t* info = m_info;

	//if (!m_info->lastframedata)
	//	DevMsg("Main Lock\n");
	//if (!m_info->lastframedata)
	//	return;

	//	m_mutex.lock();
	//	if (!m_info->lastframedata || !m_info->readyfornextframe)
	//	return;


	//m_info->readyfornextframe = false;

	//DevMsg("Resizing a %ux%u %iBBP (%i pitch) image to %ux%u %iBBP (%i pitch)\n", sourceWidth, sourceHeight, sourceDepth, sourcePitch, destWidth, destHeight, destDepth, destPitch);
	//	DevMsg("Test: %s\n", pDest);

	unsigned int sourceWidthCopy = sourceWidth;
	unsigned int sourceHeightCopy = sourceHeight;
	size_t sourcePitchCopy = sourcePitch;
	unsigned int sourceDepthCopy = sourceDepth;

	//void* pSrcCopy = malloc(sourcePitchCopy * sourceHeightCopy);
	//Q_memcpy(pSrcCopy, pSrc, sourcePitchCopy * sourceHeightCopy);


	const unsigned char* pRealSrc = (const unsigned char*)pSrc;
	unsigned char* pDstRow = (unsigned char*)pDst;
	//for (int dstY = 0; dstY<destHeight; dstY++)
	for (int dstY = destHeight-1; dstY >= 0; dstY--)
	{
		unsigned int srcY = dstY * sourceHeight / destHeight;
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

	/*
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
	pDstCur[0] = pSrcRow[srcX*sourceDepth + 0];
	pDstCur[1] = pSrcRow[srcX*sourceDepth + 1];
	pDstCur[2] = pSrcRow[srcX*sourceDepth + 2];

	pDstCur[3] = 255;

	pDstCur += destDepth;
	}

	pDstRow += destPitch;
	}
	*/

	//	free(pSrcCopy);

	//m_info->readyfornextframe = true;

	//	m_mutex.unlock();
	//	DevMsg("Main Unlock\n");
}


void C_WindowInstance::OnProxyBind(C_BaseEntity* pBaseEntity)
{
	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;

	//	if (m_id == "images")
	//		return;

	/*
	if ( pBaseEntity )
	DevMsg("WebTab: OnProxyBind: %i\n", pBaseEntity->entindex());
	else
	DevMsg("WebTab: OnProxyBind\n");
	*/

	// visiblity test
	if (m_iLastVisibleFrame < gpGlobals->framecount)
	{
		m_iLastVisibleFrame = gpGlobals->framecount;
		g_pAnarchyManager->GetCanvasManager()->RenderSeen(this);
		/*
		if (!g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(this))
		{
		if (!g_pAnarchyManager->GetCanvasManager()->IncrementVisibleCanvasesCurrentFrame(this))
		return;
		}
		else
		{
		if (!g_pAnarchyManager->GetCanvasManager()->IncrementVisiblePriorityCanvasesCurrentFrame(this))
		return;
		}*/


		//if (m_iLastRenderedFrame < gpGlobals->framecount)
		//{
		/*
		if (!g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(this))
		g_pAnarchyManager->GetCanvasManager()->IncrementVisibleCanvasesCurrentFrame();
		else
		g_pAnarchyManager->GetCanvasManager()->IncrementVisiblePriorityCanvasesCurrentFrame();
		*/

		//m_info->readytocopyframe
		// 
		if (m_bIsDirty && g_pAnarchyManager->GetCanvasManager()->ShouldRender(this))
			Render();
		//}
	}
}