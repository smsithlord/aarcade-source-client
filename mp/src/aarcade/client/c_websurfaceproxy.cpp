#include "cbase.h"
#include "c_websurfaceproxy.h"

#include <string>
#include "Filesystem.h"
#include <KeyValues.h>
#include "c_anarchymanager.h"
//#include "c_simple_image_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//std::map<std::string, std::map<std::string, ITexture*>> CWebSurfaceProxy::s_simpleImages;
CUtlMap<std::string, ITexture*> CWebSurfaceProxy::s_avatarSimpleImages;
CUtlMap<std::string, ITexture*> CWebSurfaceProxy::s_screenSimpleImages;
CUtlMap<std::string, ITexture*> CWebSurfaceProxy::s_marqueeSimpleImages;
bool CWebSurfaceProxy::s_bLessThanReady = false;

//int CWebSurfaceProxy::s_textureCount = 0;
CCanvasRegen* CWebSurfaceProxy::s_pCanvasRegen = null;

void CWebSurfaceProxy::OnSimpleImageRendered(std::string channel, std::string itemId, std::string field, ITexture* pTexture)
{
	// Assume that channel & field can ONLY be "screen" or "marquee"
	
	
	//DevMsg("Simple image render: \"%s\" and \"%s\"\n", channel.c_str(), field.c_str());


	// TODO: Figure out WHY the "field" value is sometimes "file".  It might have to do with requesting something re-load?


//	DevMsg("WebSurfaceProxy: OnSimpleImageRendered %s %s %s\n", channel.c_str(), itemId.c_str(), field.c_str());

	/* OLD STD::MAP WAY
	s_simpleImages[channel][itemId] = pTexture;

	if (field != "" && field != channel && field != "file")	// FIXME: more intellegent look-ahead before requesting images be rendered would speed stuff up a lot.
		s_simpleImages[field][itemId] = pTexture;
	*/

	// New CUtlMap way
	CUtlMap<std::string, ITexture*>* pUtlMap;
	if (channel == "marquee")
		pUtlMap = &s_marqueeSimpleImages;
	else if( channel == "avatar")
		pUtlMap = &s_avatarSimpleImages;
	else
		pUtlMap = &s_screenSimpleImages;

	pUtlMap->InsertOrReplace(itemId, pTexture);

	/* DISABLED FOR NOW, not sure if it's still needed??
	if (field != "" && field != channel && field != "file")	// FIXME: more intellegent look-ahead before requesting images be rendered would speed stuff up a lot.
	{
		int iFieldSimpleImagesMapIndex = s_simpleImages.Find(field);
		if (iFieldSimpleImagesMapIndex == s_simpleImages.InvalidIndex())
		{
			// Create the field
			CUtlMap<std::string, ITexture*> itemMap;	// FIXME: Shouldn't use HEAP for this. (probably)
			SetDefLessFunc(itemMap);
			iFieldSimpleImagesMapIndex = s_simpleImages.Insert(field, itemMap);
		}

		// We now have a VALID iFieldSimpleImagesMapIndex
		CUtlMap<std::string, ITexture*> itemMap = s_simpleImages.Element(iFieldSimpleImagesMapIndex);
		itemMap.InsertOrReplace(itemId, pTexture);
	}
	*/
}

CWebSurfaceProxy::CWebSurfaceProxy()
{
	DevMsg("WebSurfaceProxy: Constructor\n");
	m_iState = 0;
	m_pAVRConVar = null;
	m_pAVRAmpConVar = null;
	m_pPlayEverywhereConVar = null;
	m_hueColor = Color(0, 255, 0);
	//m_id = "";
	m_pCurrentEmbeddedInstance = null;
	m_pMaterial = null;
	m_pCurrentTexture = null;
	m_pOriginalTexture = null;
	m_originalTextureName = "";
	m_materialName = "";
	m_materialGroupName = "";
	m_pMaterialTextureVar = null;
	m_pMaterialDetailBlendFactorVar = null;
	m_pMaterialBaseTextureTransformVar = null;
	m_pEmbeddedInstance = null;
	m_originalId = "";
	m_iOriginalAutoCreate = 1;
	m_originalUrl = "";
	m_originalSimpleImageChannel = "";
	
	if (!s_bLessThanReady)
	{
		SetDefLessFunc(s_screenSimpleImages);
		SetDefLessFunc(s_marqueeSimpleImages);
		SetDefLessFunc(s_avatarSimpleImages);
		s_bLessThanReady = true;
	}

	//g_pAnarchyManager->GetCanvasManager()->RegisterProxy(this);
}

CWebSurfaceProxy::~CWebSurfaceProxy()
{
	DevMsg("WebSurfaceProxy: Destructor\n");
//	if( m_pOriginalTexture != null )	// FIXME When would this ever not exist for this type of proxy?
	//{
//		m_pOriginalTexture->SetTextureRegenerator(null);
	//}
	// FIXME: Do we need to reduce the texture reference or delete the m_pTexture as well?
	// FIXME: Does this get called every map load? Should all the textureregens in the m_pImageInstances get NULL'd too?
}

bool CWebSurfaceProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	DevMsg("WebSurfaceProxy: Init\n");

	m_pMaterial = pMaterial;
	m_materialName = m_pMaterial->GetName();
	m_materialGroupName = m_pMaterial->GetTextureGroupName();

	// set all the original stuff
	bool found;
	IMaterialVar* pMaterialVar = m_pMaterial->FindVar("$basetexture", &found, false);

	if( !found )
	{
		DevMsg("ERROR: No $basetexture found!\n");
		return false;
	}

	//g_pAnarchyManager->GetWebManager()->RegisterProxy(this);
	g_pAnarchyManager->GetCanvasManager()->RegisterProxy(this);

	m_pMaterialTextureVar = pMaterialVar;
	m_pOriginalTexture = pMaterialVar->GetTextureValue();
	m_pOriginalTexture->IncrementReferenceCount();
	m_originalTextureName = (m_pOriginalTexture && !m_pOriginalTexture->IsError() ) ? m_pOriginalTexture->GetName() : "";
//	m_pOriginalTexture->SetTextureRegenerator(s_pWebSurfaceRegen);

	bool bFoundDetailBlendFactorVar;
	m_pMaterialDetailBlendFactorVar = m_pMaterial->FindVar("$detailblendfactor", &bFoundDetailBlendFactorVar, false);

	//bool bFoundBaseTextureTransformVar;
	//m_pMaterialBaseTextureTransformVar = m_pMaterial->FindVar("$basetexturetransform", &bFoundBaseTextureTransformVar, false);

	//if (!bFoundBaseTextureTransformVar || m_pMaterialBaseTextureTransformVar->GetType() != MATERIAL_VAR_TYPE_MATRIX)
	//	m_pMaterialBaseTextureTransformVar = null;

	pMaterialVar = m_pMaterial->FindVar("id", &found, false);
	m_originalId = (found) ? pMaterialVar->GetStringValue() : "";

	pMaterialVar = m_pMaterial->FindVar("url", &found, false);
	m_originalUrl = (found) ? pMaterialVar->GetStringValue() : "";

	/*DevMsg("Original URL is: %s\n", m_originalUrl.c_str());*/

	pMaterialVar = m_pMaterial->FindVar("autocreate", &found, false);
	m_iOriginalAutoCreate = (found) ? pMaterialVar->GetIntValue() : 0;

	//pMaterialVar = m_pMaterial->FindVar("simpleimage", &found, false);
	//m_bOriginalSimpleImage = (found) ? (pMaterialVar->GetIntValue() != 0) : false;

	pMaterialVar = m_pMaterial->FindVar("simpleimagechannel", &found, false);
	m_originalSimpleImageChannel = (found) ? pMaterialVar->GetStringValue() : "screen";
	DevMsg("Material Dynamic Image Channel: %s\n", m_originalSimpleImageChannel.c_str());
	return true;
}

//void ReleaseSimpleImages(CUtlMap<std::string, CUtlMap<std::string, ITexture*>>& simpleImages)
void ReleaseSimpleImages(CUtlMap<std::string, ITexture*>& simpleImages, CUtlMap<ITexture*, bool>& usedTextures)
{
	// set all owned textures's regenerator to null
	ITexture* pTexture;
	//CUtlMap<ITexture*, bool> usedTextures;
	//SetDefLessFunc(usedTextures);

	for (int iSimpleImagesMapIndex = simpleImages.FirstInorder(); iSimpleImagesMapIndex != simpleImages.InvalidIndex(); iSimpleImagesMapIndex = simpleImages.NextInorder(iSimpleImagesMapIndex))
	{
		pTexture = simpleImages.Element(iSimpleImagesMapIndex);

		if (pTexture)
			usedTextures.InsertOrReplace(pTexture, true);
	}

	//for (int iSimpleImagesMapIndex = simpleImages.FirstInorder(); iSimpleImagesMapIndex != simpleImages.InvalidIndex(); iSimpleImagesMapIndex = simpleImages.NextInorder(iSimpleImagesMapIndex))
	//	simpleImages.Element(iSimpleImagesMapIndex).PurgeAndDeleteElements();

	//simpleImages.PurgeAndDeleteElements();
	//usedTextures.PurgeAndDeleteElements();
	//DevMsg("Mark one\n");
	simpleImages.RemoveAll();










	// OLD WAY BELOW HERE!! (but might have useful insta-texture clean up code.)


	// set all owned textures's regenerator to null
	/*
	ITexture* pTexture;
	std::map<ITexture*, bool> usedTextures;
	std::map<std::string, std::map<std::string, ITexture*>>::iterator it = simpleImages.begin();
	std::map<std::string, ITexture*>::iterator it2;
	while (it != simpleImages.end())
	{
		it2 = it->second.begin();
		while (it2 != it->second.end())
		{
			pTexture = it2->second;
			if (pTexture)
			{
				usedTextures[pTexture] = true;
			//	pTexture->DecrementReferenceCount();	// FIXME: Disabled on 10/21/2016 for testing
			}
			*/
			/*
			if (usedTextures.find(it2->second) == usedTextures.end())
			{
				usedTextures[it2->second] = true;

				//do work
				pTexture = it2->second;

				if (pTexture)
				{
					//pTexture->SetTextureRegenerator(null);
					pTexture->DecrementReferenceCount();
					//pTexture->DeleteIfUnreferenced();
				}
			}
			else
			{
				//do work
				pTexture = it2->second;

				if (pTexture)
				{
					//pTexture->SetTextureRegenerator(null);
					pTexture->DecrementReferenceCount();
					//pTexture->DeleteIfUnreferenced();
				}
			}
			*/
	/*
			it2++;
		}

		//it->second.clear();
		it++;
	}

	auto usedIt = usedTextures.begin();
	while (usedIt != usedTextures.end())
	{
		pTexture = usedIt->first;
		if (pTexture)
		{
			pTexture->DecrementReferenceCount();	// FIXME: Disabled on 10/21/2016 for testing
			pTexture->SetTextureRegenerator(null);
			pTexture->DeleteIfUnreferenced();
		}

		usedIt++;
	}

	it = simpleImages.begin();
	while (it != simpleImages.end())
	{
		it->second.clear();
		it++;
	}

	simpleImages.clear();
	usedTextures.clear();
	*/
}

void CWebSurfaceProxy::StaticLevelShutdownPreEntity()
{
	// THIS SHOULD ONLY BE CALLED ONCE!!!!
	//ReleaseSimpleImages(s_simpleImages);
}

void CWebSurfaceProxy::StaticLevelShutdownPostEntity()
{
	// THIS SHOULD ONLY BE CALLED ONCE!!!!
	CUtlMap<ITexture*, bool> usedTextures;
	SetDefLessFunc(usedTextures);

	ReleaseSimpleImages(s_screenSimpleImages, usedTextures);
	ReleaseSimpleImages(s_marqueeSimpleImages, usedTextures);
	ReleaseSimpleImages(s_avatarSimpleImages, usedTextures);

	ITexture* pTexture;
	for (int iUsedTexturesMapIndex = usedTextures.FirstInorder(); iUsedTexturesMapIndex != usedTextures.InvalidIndex(); iUsedTexturesMapIndex = usedTextures.NextInorder(iUsedTexturesMapIndex))
	{
		pTexture = usedTextures.Key(iUsedTexturesMapIndex);

		if (pTexture)
		{
			pTexture->DecrementReferenceCount();
			pTexture->SetTextureRegenerator(null);
			pTexture->DeleteIfUnreferenced();
		}
	}
	usedTextures.Purge();
}

void CWebSurfaceProxy::LevelShutdownPreEntity()
{
}

void CWebSurfaceProxy::ReleaseCurrent()
{
	DevMsg("CWebSurfaceProxy::Release Current\n");
	if (m_pMaterialTextureVar && m_pCurrentTexture && m_pCurrentTexture != m_pOriginalTexture)
	{
		m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
		m_pCurrentTexture = m_pOriginalTexture;
	}
}

void CWebSurfaceProxy::ReleaseStuff()
{
	/*
	DevMsg("WebSurfaceProxy: Release Stuff: %s\n", m_originalSimpleImageChannel.c_str());
	if (this->GetMaterial() && m_pMaterialTextureVar && m_pOriginalTexture && m_pCurrentTexture && m_pOriginalTexture != m_pCurrentTexture)
	{
		m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
		m_pCurrentTexture = m_pOriginalTexture;
	}
	return;
	*/

	///*
	//DevMsg("WebSurfaceProxy: Release Stuff: %s\n", m_originalSimpleImageChannel.c_str());
	if (this->GetMaterial() && m_pMaterialTextureVar && m_pOriginalTexture && m_pCurrentTexture && m_pOriginalTexture != m_pCurrentTexture)
	{
		//DevMsg("Route A\n");
		m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
		m_pCurrentTexture = m_pOriginalTexture;
	}

	//if (!g_pAnarchyManager->IsShuttingDown())
	if (!g_pAnarchyManager->IsShuttingDown() && (!g_pAnarchyManager->ShouldPrecacheInstance() || g_pAnarchyManager->IsLevelInitialized()))
	{
		//if ( m_pOriginalTexture )

		m_iState = 0;
		//m_id = "";
		m_pCurrentEmbeddedInstance = null;
		m_pMaterial = null;
		m_pCurrentTexture = null;
		m_pOriginalTexture = null;
		m_pMaterialTextureVar = null;
		m_pMaterialDetailBlendFactorVar = null;
		m_pMaterialBaseTextureTransformVar = null;
		m_pEmbeddedInstance = null;
		m_originalId = "";
		m_iOriginalAutoCreate = 1;
		m_originalUrl = "";
		m_originalSimpleImageChannel = "";
	}
	//*/
}

void CWebSurfaceProxy::Release()
{
	DevMsg("WebSurfaceProxy: Release\n");	// FIXME: This is causing some fucked up issues.  wtf is going on bra, fix it.
//	if (g_pAnarchyManager->GetSuspendEmbedded())	// hmmm, does this ever get triggered when embeds ARE NOT suspended?
//		return;	// FIXME: disabled for resting on 10/4/2016 and again on 10/21/2016
//	else
//		DevMsg("Embededs ARE NOT suspended %s\n", this->m_originalId.c_str());	// well that'll answer that.

	/*
	if (m_pMaterialTextureVar && m_pMaterialTextureVar->IsDefined() && m_pMaterialTextureVar->IsTexture() && m_pOriginalTexture && m_pCurrentTexture && m_pOriginalTexture != m_pCurrentTexture)
	{
		m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
		m_pCurrentTexture = m_pOriginalTexture;
	}

	m_iState = 0;
	//m_id = "";
	m_pCurrentEmbeddedInstance = null;
	m_pMaterial = null;
	m_pCurrentTexture = null;
	m_pOriginalTexture = null;
	m_pMaterialTextureVar = null;
	m_pMaterialDetailBlendFactorVar = null;
	m_pEmbeddedInstance = null;
	m_originalId = "";
	m_iOriginalAutoCreate = 1;
	m_originalUrl = "";
	m_originalSimpleImageChannel = "";
	*/

	/*
	// Iterate through all our SimpleImages
	for(std::map<int, DynamicImage*>::iterator it = m_pImageInstances.begin(); it != m_pImageInstances.end(); it++)
		it->second->texture->DecrementReferenceCount();
	*/

	// Release our Loading Image
	//if (m_pOriginalTexture)
	//{
		//m_pOriginalTexture->DecrementReferenceCount();
		//m_pOriginalTexture->SetTextureRegenerator(null);
	//}

	// FIXME: Do we need to delete the class-scope members too? YOUT HINK I KNO W? SHIIIIT

//	delete this;

	//m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
	//m_pCurrentTexture = m_pOriginalTexture;

	// release the simple images
	//ReleaseSimpleImages(s_simpleImages);

	//if (g_pAnarchyManager->GetSuspendEmbedded())
	//	s_simpleImages.clear();
}

//ITexture* CWebSurfaceProxy::CreateTexture(C_BaseEntity* pEntity = null)
//{
	//DevMsg("WebSurfaceProxy: CreateTexture\n");
	//return null;
	/*
	// Create the texture for this proxy instance
	C_DynamicImageWebView* pDynamicImageWebView = C_AnarchyManager::GetSelf()->GetWebViewManager()->GetDynamicImageWebView();
	int width = pDynamicImageWebView->GetWidth();
	int height = pDynamicImageWebView->GetHeight();

	std::string textureName = "simple_image_";
	textureName += C_AnarchyManager::GetSelf()->GenerateHash(m_pMaterial->GetName());

	if( pEntity )
	{
		textureName += "_";
		textureName += VarArgs("%i", pEntity->entindex());
	}

	ITexture* pTexture = g_pMaterialSystem->CreateProceduralTexture( textureName.c_str(), TEXTURE_GROUP_VGUI, width, height, IMAGE_FORMAT_BGR888, 1 );
	pTexture->SetTextureRegenerator(s_pDynamicRegen);

	return pTexture;
	*/
//}

// originally from https://stackoverflow.com/questions/8507885/shift-hue-of-an-rgb-color
Color TransformH(
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

void CWebSurfaceProxy::OnBind(C_BaseEntity *pC_BaseEntity)
{
	if (g_pAnarchyManager->IsPaused() || g_pAnarchyManager->IsInSourceGame())
		return;

	if (!g_pAnarchyManager->GetImagesReady())
		return;

	if (!m_pPlayEverywhereConVar)
		m_pPlayEverywhereConVar = cvar->FindVar("play_everywhere");

	//if (m_originalSimpleImageChannel == "")
	//	return;

	//C_EmbeddedInstance* pSelectedEmebeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
	C_EmbeddedInstance* pSelectedEmebeddedInstance = null;
	
	if (m_pEmbeddedInstance)
		pSelectedEmebeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();// m_pEmbeddedInstance->GetParentSelectedEmbeddedInstance();

	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(m_pEmbeddedInstance);
	/*if (pAwesomiumBrowserInstance)
	{
		if (pAwesomiumBrowserInstance->GetId() != "images")
			DevMsg("Proxy bind: %s\n", pAwesomiumBrowserInstance->GetId().c_str());
	}*/

	// TEST STUFF
	//if (pAwesomiumBrowserInstance && pAwesomiumBrowserInstance->GetId() != "images")
	//{
	//	C_PropShortcutEntity* pTestShortcut = dynamic_cast<C_PropShortcutEntity*>(pC_BaseEntity);
	//	if (pTestShortcut)
	//	{
	//		DevMsg("Proxy Bind ID: %s\n", pAwesomiumBrowserInstance->GetId().c_str());
	//		//DevMsg("Bind Proxy On Entity: %s\n", pTestShortcut->GetItemId().c_str());
	//	}
	//}
	// END TEST STUFF

	bool bSwappedEmbeddedInstanceIn = false;

	bool bShouldBind = false;
	if (!m_pEmbeddedInstance)
	{
		if (m_iState == 0)
		{
			if (m_originalId != "")
			{
				// does a web tab for this id already exist?
				//C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->FindWebTab(m_originalId);
				//C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
				C_EmbeddedInstance* pEmbeddedInstance = (m_originalId == "images") ? g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("images") : g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(m_originalId);
				if (pEmbeddedInstance)
				{
					m_pEmbeddedInstance = pEmbeddedInstance;
					m_pCurrentEmbeddedInstance = pEmbeddedInstance;
					g_pAnarchyManager->GetCanvasManager()->SetMaterialEmbeddedInstanceId(m_pMaterial, pEmbeddedInstance->GetId());
					//g_pAnarchyManager->GetWebManager()->SetMaterialWebTabId(m_pMaterial, m_pWebTab->GetId());
				}
			}

			if (!m_pEmbeddedInstance)
			{
				// check if we should create a web tab
				if (m_iOriginalAutoCreate == 1 )	//&& m_iState == 0 )	// redundant
				{
					// create a web tab
					//m_pEmbeddedInstance = g_pAnarchyManager->GetWebManager()->CreateWebTab(m_originalUrl, m_originalId);
				//	m_pEmbeddedInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->CreateAwesomiumBrowserInstance(m_originalId, m_originalUrl, false);

					// try to get the object ID
					C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
					C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pC_BaseEntity);
					pSteamBrowserInstance->Init(m_originalId, m_originalUrl, "Auto Steamworks Browser", null, (pShortcut) ? pShortcut->entindex() : -1);

					m_pEmbeddedInstance = pSteamBrowserInstance;
					m_pCurrentEmbeddedInstance = m_pEmbeddedInstance;
					//g_pAnarchyManager->GetWebManager()->SetMaterialWebTabId(m_pMaterial, m_pWebTab->GetId());
					m_iState = 1;	// initializing
				}  
			}

			if (m_pEmbeddedInstance)
				g_pAnarchyManager->GetCanvasManager()->SetMaterialEmbeddedInstanceId(m_pMaterial, m_pEmbeddedInstance->GetId());
				//g_pAnarchyManager->GetWebManager()->SetMaterialWebTabId(m_pMaterial, m_pEmbeddedInstance->GetId());
		}
	}
	else
	{
		if (pAwesomiumBrowserInstance && pAwesomiumBrowserInstance->GetState() == 2)
			m_iState = 2;

		/*
		if (m_pMaterialDetailBlendFactorVar && (!g_pAnarchyManager->GetWebManager()->GetSelectedWebTab() || !g_pAnarchyManager->GetInputManager()->GetInputMode() || m_pWebTab != g_pAnarchyManager->GetWebManager()->GetSelectedWebTab()))
			m_pMaterialDetailBlendFactorVar->SetFloatValue(0);
		else if (m_pMaterialDetailBlendFactorVar)
			m_pMaterialDetailBlendFactorVar->SetFloatValue(1);
		*/

		// a regular proxy will need to grab the web tab's texture before it binds
		if (m_originalSimpleImageChannel == "")
		{
			ITexture* pTexture = m_pEmbeddedInstance->GetTexture();
			if (pTexture && m_pMaterialTextureVar)
			{
				//DevMsg("Tex name A: %s\n", pTexture->GetName());
				m_pMaterialTextureVar->SetTextureValue(pTexture);
				m_pCurrentTexture = pTexture;
				//			m_iState = 2;

				if (m_pMaterialDetailBlendFactorVar && ((pC_BaseEntity && g_pAnarchyManager->GetSelectedEntity() != pC_BaseEntity) || !pSelectedEmebeddedInstance || !g_pAnarchyManager->GetInputManager()->GetInputMode() || m_pEmbeddedInstance != pSelectedEmebeddedInstance))
					m_pMaterialDetailBlendFactorVar->SetFloatValue(0);
				else if (m_pMaterialDetailBlendFactorVar)
					m_pMaterialDetailBlendFactorVar->SetFloatValue(1);

				m_pEmbeddedInstance->OnProxyBind(pC_BaseEntity);
			}
		}
		else
		{
			ITexture* pTexture = null;

			if (m_pMaterialDetailBlendFactorVar && ((pC_BaseEntity && g_pAnarchyManager->GetSelectedEntity() != pC_BaseEntity) || !pSelectedEmebeddedInstance || !g_pAnarchyManager->GetInputManager()->GetInputMode()))
				m_pMaterialDetailBlendFactorVar->SetFloatValue(0);
			else if (m_pMaterialDetailBlendFactorVar)
				m_pMaterialDetailBlendFactorVar->SetFloatValue(1);

			//bool bSwappedEmbeddedInstanceIn = false;
			// if a texture exists for this shortcut's item's id for this channel, then we're done already.
			// otherwise, we have to request the web tab to render us, which will be ignored 90% of the time, but thats OK.
			bool bTextureExists = false;
			std::string itemId = "";
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pC_BaseEntity);
			C_DynamicProp* pDynamicProp = null;
			if (!pShortcut)
			{
				pDynamicProp = dynamic_cast<C_DynamicProp*>(pC_BaseEntity);

				user_t* pInstanceUser = g_pAnarchyManager->GetMetaverseManager()->FindInstanceUser(pDynamicProp);
				if (pInstanceUser)
					itemId = pInstanceUser->userId;// "wtf";// https://steamcdn-a.akamaihd.net/steamcommunity/public/images/avatars/c9/c9ef4b52bc9044683951d93c48277617b665703e_full.jpg";
				else
				{
					itemId = "localAvatar";
				}
			}
			else //if (pShortcut)
				itemId = pShortcut->GetItemId();

			//if ( pShortcut)
			//{
				if (true)//itemId != "")
				{
					// we're swapping in the web browser tab even if no simple image is anywhere.
					if (pShortcut && (m_originalSimpleImageChannel == "screen" || itemId == ""))
					{
						C_EmbeddedInstance* testerInstance = null;	// if initialized to null, this might work still!!!
						std::string taskId;

						if (itemId != "")
							testerInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance("auto" + itemId);	// WARNING: If instances are never removed, this returns even dead instances!!

						taskId = (g_pAnarchyManager->GetSelectedEntity()) ? "auto" + static_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity())->GetItemId() : "";

						C_EmbeddedInstance* selectedInstance = (g_pAnarchyManager->GetSelectedEntity()) ? g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(taskId) : null;

						C_EmbeddedInstance* displayInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
						if (!displayInstance)
						{
							// if there's not a display instance, just grab the 1st instance found
							displayInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();
						}

						// FIXME: This should be a required method of all embedded instances!!
					//	bool bInstanceTextureReady = true;
						//C_LibretroInstance* pLibretroInstance = (selectedInstance) ? dynamic_cast<C_LibretroInstance*>(selectedInstance) : null;
					//	if (pLibretroInstance)
						//	bInstanceTextureReady = (pLibretroInstance->GetInfo()->state == 5);

						//bool bIsPotentialSlaveSwap = false;
							//if (g_pAnarchyManager->GetSelectedEntity() && !testerInstance && selectedInstance && (pShortcut->GetSlave() || itemId == ""))//g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance())
						if (!testerInstance && (selectedInstance || displayInstance) && ((m_pPlayEverywhereConVar->GetBool()) || ((pShortcut->GetSlave() || itemId == "") && g_pAnarchyManager->GetPanoshotState() == PANO_NONE && ((selectedInstance && selectedInstance->GetOriginalItemId() != "-p7WtqC4ppDCVd7u0mfZ") || (!selectedInstance && displayInstance && displayInstance->GetOriginalItemId() != "-p7WtqC4ppDCVd7u0mfZ")) && ((selectedInstance && selectedInstance->GetId() != "SteamTalker") || (!selectedInstance && displayInstance && displayInstance->GetId() != "SteamTalker")) && ((selectedInstance && selectedInstance->GetId() != "aai") || (!selectedInstance && displayInstance && displayInstance->GetId() != "aai")))))// the itemID stuff is a hack to not show my web cam item on the video mirrors :S
						{
							//	DevMsg("Swapped slave in!\n");
							testerInstance = (selectedInstance) ? selectedInstance : displayInstance;
							//testerInstance->Update();
							//testerInstance->OnProxyBind(pC_BaseEntity);

							//bIsPotentialSlaveSwap = true;

							//if (m_pMaterialDetailBlendFactorVar && (!g_pAnarchyManager->GetInputManager()->GetInputMode() || m_originalSimpleImageChannel != "screen"))
							//if (true && m_pMaterialDetailBlendFactorVar)

							//if (testerInstance->GetLastRenderedFrame() >= 0)
							//{
								// FIXME: Improve this to make sure THIS instance is the input instance before thinking we need to draw the UI.
								if (m_pMaterialDetailBlendFactorVar && (!g_pAnarchyManager->GetInputManager()->GetInputMode() || !g_pAnarchyManager->GetSelectedEntity() || m_originalSimpleImageChannel != "screen"))
									m_pMaterialDetailBlendFactorVar->SetFloatValue(0);
								else if (m_pMaterialDetailBlendFactorVar)
									m_pMaterialDetailBlendFactorVar->SetFloatValue(1);
							//}
						}

						if (m_pMaterialTextureVar && testerInstance && testerInstance->GetTexture())
						{
							//ITexture* pTesterTexture = testerInstance->GetTexture();
							//m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
							//m_pCurrentTexture = pTesterTexture;

							// instead, try to use same logic as ShouldRender
							//if (g_pAnarchyManager->GetCanvasManager()->ShouldRender(testerInstance, true))	// not enough to fix the issue.

							if (testerInstance->GetLastVisibleFrame() != gpGlobals->framecount)	// FIXME: This doesn't take into account that this slave web tab is the same as the original
								testerInstance->Update();

							//if (testerInstance->IsDirty())
							if (testerInstance->GetLastRenderedFrame() >= 0)
							{
								ITexture* pTesterTexture = testerInstance->GetTexture();
								m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
								m_pCurrentTexture = pTesterTexture;
								bSwappedEmbeddedInstanceIn = true;
							}
						}
					}

					if ( itemId != "")
					{
						bool bSpecialAvatarCase = false;
						if (itemId == "localAvatar")
						{
							C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance("auto-p7WtqC4ppDCVd7u0mfZ");
							if (pEmbeddedInstance)
							{
								ITexture* pTesterTexture = pEmbeddedInstance->GetTexture();
								m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
								m_pCurrentTexture = pTesterTexture;
								bSwappedEmbeddedInstanceIn = true;
								bSpecialAvatarCase = true;
							}
						}

						// we still need to find the texture so we can process the image loading still.
						//CUtlMap<std::string, CUtlMap<std::string, ITexture*>>
						CUtlMap<std::string, ITexture*>* pUtlMap;// = (m_originalSimpleImageChannel == "screen") ? &s_screenSimpleImages : &s_marqueeSimpleImages;
						if (bSpecialAvatarCase)
							pUtlMap = &s_screenSimpleImages;
						else if (m_originalSimpleImageChannel == "marquee")
							pUtlMap = &s_marqueeSimpleImages;
						else if (m_originalSimpleImageChannel == "avatar")
							pUtlMap = &s_avatarSimpleImages;
						else
							pUtlMap = &s_screenSimpleImages;

						if (!bSpecialAvatarCase)
						{
							int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
							if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
							{
								if (!bSwappedEmbeddedInstanceIn && m_pMaterialTextureVar)
								{
									// we have found our texture.  swap it in and we're done.
									pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
									if (pTexture && !pTexture->IsError() && pTexture->IsProcedural())
									{
										//DevMsg("Tex name B: %s\n", pTexture->GetName());
										m_pMaterialTextureVar->SetTextureValue(pTexture);
										m_pCurrentTexture = pTexture;
									}
									else if (m_pOriginalTexture)
									{
										m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
										m_pCurrentTexture = m_pOriginalTexture;
									}
								}

								bTextureExists = true;
							}
						}




						// OLD STD::MAP WAY HERE!
						/*
						std::map<std::string, std::map<std::string, ITexture*>>::iterator it = s_simpleImages.find(m_originalSimpleImageChannel);
						if (it != s_simpleImages.end())
						{
							std::map<std::string, ITexture*>::iterator it2 = it->second.find(itemId);
							if (it2 != it->second.end())
							{
								if (!bSwappedEmbeddedInstanceIn && m_pMaterialTextureVar)
								{
									// we have found our texture.  swap it in and we're done.
									if (it2->second)
									{
										m_pMaterialTextureVar->SetTextureValue(it2->second);
										m_pCurrentTexture = it2->second;
									}
									else
									{
										m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
										m_pCurrentTexture = m_pOriginalTexture;
									}
								}

								bTextureExists = true;
							}
						}
						*/

						if (!bTextureExists)
						{
							C_AwesomiumBrowserInstance* pImagesBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("images");
							if (pImagesBrowserInstance && pImagesBrowserInstance->RequestLoadSimpleImage(m_originalSimpleImageChannel, itemId))
							{
								// If the request was accepted, then we need to get rdy to get the result.
								//s_simpleImages[m_originalSimpleImageChannel][itemId] = null;
								//int iSimpleImagesMapIndex = pUtlMap->Find(m_originalSimpleImageChannel);
								//if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
								pUtlMap->InsertOrReplace(itemId, null);	// FIXME: This *might* be able to just remove the element right away if we areon't doing deferred texture cleanup.
							}
						}
					}
				}
			//}
			//else
			//{
			//	DevMsg("huh?\n");
			//}
				/*
			if (!bTextureExists)
			{
				m_pMaterial = null;
				m_pOriginalTexture = null;
				m_pMaterialTextureVar = null;
			}
*/
			if (!bTextureExists && !bSwappedEmbeddedInstanceIn && m_pOriginalTexture && !m_pOriginalTexture->IsError())
			{
				m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);	// Note that there is 1 case where bTextureExists can be true but the original texture still gets swapped in.
				m_pCurrentTexture = m_pOriginalTexture;
			}
			else if (g_pAnarchyManager->VRSpectatorMirrorMode() && g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->VRSpectatorMode() == 1)	// show what the user sees on his own face.
			{
				C_PropShortcutEntity* pBestCamera = g_pAnarchyManager->GetBestSpectatorCameraObject();
				if (pBestCamera == pC_BaseEntity)
				{
					ITexture* pCameraFrame = g_pAnarchyManager->GetVRSpectatorTexture();
					if (pCameraFrame)
					{
						m_pMaterialTextureVar->SetTextureValue(pCameraFrame);
						m_pCurrentTexture = pCameraFrame;
					}
				}
			}
			/*else if (bSwappedEmbeddedInstanceIn && pSelectedEmebeddedInstance && pSelectedEmebeddedInstance->GetLastRenderedFrame() < 0)
			{
				if (pTexture)
				{
					m_pMaterialTextureVar->SetTextureValue(pTexture);
					m_pCurrentTexture = pTexture;
				}
				else
				{
					m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
					m_pCurrentTexture = m_pOriginalTexture;
				}
			}*/
		}
	}

	///* show selected texture on broken cabinets
	if (!m_pEmbeddedInstance)
	{
		//C_WebTab* pSelectedWebTab = g_pAnarchyManager->GetWebManager()->GetSelectedWebTab();
		if (pSelectedEmebeddedInstance)
		{
			ITexture* pSelectedTexture = pSelectedEmebeddedInstance->GetTexture();
			m_pMaterialTextureVar->SetTextureValue(pSelectedTexture);
			m_pCurrentTexture = pSelectedTexture;
		}
	}

	bool bAlteredUVs = false;
	if (g_pAnarchyManager->IsAlwaysAnimatingImagesEnabled() && m_pMaterial->HasProxy() && m_iState == 2 && m_pEmbeddedInstance && m_originalId == "images" && m_originalSimpleImageChannel == "screen")
	{
		// TEMP TEST: Test some offset stuff.
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pC_BaseEntity);
		if (pShortcut)
		{

			/*

			Always Animating Images (AAI)
			================================
			1. Add bookkeeping for Always Animating Images to the C++.
			- Create an AAI manager class that creates the web tab and handles all of the related messages
			  for addin, removing, resetting, mapping, etc. for the animted image atlas.
			- Maintain a KeyValues file that contains the IDs of items that want to always animate.
			2. Add a mechanic for players to mark an item as Animated Image that gets remembered per-item.
			3. Fin
			
			0 = x scale (extended width)
5 = y scale (extended down)
10 = 
15 = 



1 = bottom right corner X
2 = UNKNOWN
3 = x offset
4 = rotation
6 = UNKNOWN
7 = y offset
8 = UNKNOWN
9 = UNKNOWN
11 = UNKNOWN
12 = UNKNOWN
13 = UNKNOWN
14 = UNKNOWN
			
			*/

			//DevMsg("Rendering %s\n", objectId.c_str());

			if (!m_pMaterialBaseTextureTransformVar)
			{
				bool bFoundBaseTextureTransformVar;
				m_pMaterialBaseTextureTransformVar = m_pMaterial->FindVar("$basetexturetransform", &bFoundBaseTextureTransformVar, false);

				if (!bFoundBaseTextureTransformVar || !m_pMaterialBaseTextureTransformVar || m_pMaterialBaseTextureTransformVar->GetType() != MATERIAL_VAR_TYPE_MATRIX)
					m_pMaterialBaseTextureTransformVar = null;
			}

			//if (g_pAnarchyManager->GetEye() == ISourceVirtualReality::VREye::VREye_Left) //(objectId == "-sp10UstlmpuJI-BTvY1")
			if (m_pMaterialBaseTextureTransformVar)
			{
				if (!bSwappedEmbeddedInstanceIn && g_pAnarchyManager->GetAAIManager()->ShouldAnimateItem(pShortcut->GetItemId()))
				{
					std::string objectId = pShortcut->GetObjectId();
					//C_EmbeddedInstance* testerInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
					//if (!testerInstance)
					//testerInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();
					C_EmbeddedInstance* testerInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("aai");
					if (!testerInstance)
					{
						g_pAnarchyManager->GetAAIManager()->CreateBrowserInstance();
					}
					else
					{
						if (testerInstance->GetLastVisibleFrame() != gpGlobals->framecount)
							testerInstance->Update();

						if (m_pMaterialTextureVar && testerInstance && testerInstance->GetTexture())
						{
							ITexture* pTesterTexture = testerInstance->GetTexture();
							m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
							m_pCurrentTexture = pTesterTexture;


							//bool bFoundTransform = false;
							//IMaterialVar* pTransformVar = null;
							//if (m_pMaterial)
							//	pTransformVar = m_pMaterial->FindVar("$basetexturetransform", &bFoundTransform, false);
							//else
							//	bFoundTransform = false;

							int scaleX = 0;
							int scaleY = 5;
							int offsetX = 3;
							int offsetY = 7;
							//MaterialVarType_t varType = m_pMaterialBaseTextureTransformVar->GetType();
							//if (varType == MATERIAL_VAR_TYPE_MATRIX)
							//{
								VMatrix matrix;// = m_pMaterialBaseTextureTransformVar->GetMatrixValue();
								matrix.Identity();

								float* flBase = matrix.Base();
								//if (flBase)
								//{
									float flScaleX, flScaleY, flOffsetX, flOffsetY;
									g_pAnarchyManager->GetAAIManager()->GetItemMapping(pShortcut->GetItemId(), flScaleX, flScaleY, flOffsetX, flOffsetY);
									//g_pAnarchyManager->GetAAIManager()->DidRender(pShortcut->GetItemId());

									flBase[scaleX] = flScaleX;
									flBase[scaleY] = flScaleY;
									flBase[offsetX] = flOffsetX;
									flBase[offsetY] = flOffsetY;
									m_pMaterialBaseTextureTransformVar->SetMatrixValue(matrix);

									bAlteredUVs = true;
								//}

								/*
								// TOP RIGHT 25%
								VMatrix matrix = pTransformVar->GetMatrixValue();
								matrix.Base()[scaleX] = 0.5f;
								matrix.Base()[scaleY] = 0.5f;
								matrix.Base()[offsetX] = 0.5f;
								matrix.Base()[offsetY] = 0.0f;
								pTransformVar->SetMatrixValue(matrix);
								*/
							//}
						}
					}
				}

				if (!bAlteredUVs)
				{
					if (m_pMaterialTextureVar)
					{
						int scaleX = 0;
						int scaleY = 5;
						int offsetX = 3;
						int offsetY = 7;
						//MaterialVarType_t varType = m_pMaterialBaseTextureTransformVar->GetType();
						//if (varType == MATERIAL_VAR_TYPE_MATRIX)
						//{
							// FULL 100%
							/*VMatrix matrix = pTransformVar->GetMatrixValue();
							matrix.Base()[scaleX] = 1.0f;
							matrix.Base()[scaleY] = 1.0f;
							matrix.Base()[offsetX] = 0.0f;
							matrix.Base()[offsetY] = 0.0f;
							pTransformVar->SetMatrixValue(matrix);*/

							VMatrix matrix;// = m_pMaterialBaseTextureTransformVar->GetMatrixValue();
							matrix.Identity();

							float* flBase = matrix.Base();
							if (flBase)
							{
								flBase[scaleX] = 1.0f;
								flBase[scaleY] = 1.0f;
								flBase[offsetX] = 0.0f;
								flBase[offsetY] = 0.0f;
								m_pMaterialBaseTextureTransformVar->SetMatrixValue(matrix);
							}
						//}
					}
				}
				/*else if (g_pAnarchyManager->GetEye() == ISourceVirtualReality::VREye::VREye_Right)//(objectId == "-sp10Q3FnlAgBAwUhnwn")
				{
				C_EmbeddedInstance* testerInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
				if (!testerInstance)
				testerInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

				if (testerInstance)
				{
				if (testerInstance->GetLastVisibleFrame() != gpGlobals->framecount)
				testerInstance->Update();

				if (m_pMaterialTextureVar && testerInstance && testerInstance->GetTexture())
				{
				ITexture* pTesterTexture = testerInstance->GetTexture();
				m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
				m_pCurrentTexture = pTesterTexture;


				bool bFoundTransform;
				IMaterialVar* pTransformVar = null;
				if (m_pMaterial)
				pTransformVar = m_pMaterial->FindVar("$basetexturetransform", &bFoundTransform, false);
				else
				bFoundTransform = false;

				int scaleX = 0;
				int scaleY = 5;
				int offsetX = 3;
				int offsetY = 7;
				if (bFoundTransform && pTransformVar->GetType() == MATERIAL_VAR_TYPE_MATRIX)
				{
				// TOP LEFT 25%
				VMatrix matrix = pTransformVar->GetMatrixValue();
				matrix.Base()[scaleX] = 0.5f;
				matrix.Base()[scaleY] = 0.5f;
				matrix.Base()[offsetX] = 0.0f;
				matrix.Base()[offsetY] = 0.0f;
				pTransformVar->SetMatrixValue(matrix);
				}
				}
				}
				}*/
				/*else
				{
				C_EmbeddedInstance* testerInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
				if (!testerInstance)
				testerInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

				if (testerInstance)
				{
				if (testerInstance->GetLastVisibleFrame() != gpGlobals->framecount)
				testerInstance->Update();

				if (m_pMaterialTextureVar && testerInstance && testerInstance->GetTexture())
				{
				ITexture* pTesterTexture = testerInstance->GetTexture();
				m_pMaterialTextureVar->SetTextureValue(pTesterTexture);
				m_pCurrentTexture = pTesterTexture;


				bool bFoundTransform;
				IMaterialVar* pTransformVar = null;
				if (m_pMaterial)
				pTransformVar = m_pMaterial->FindVar("$basetexturetransform", &bFoundTransform, false);
				else
				bFoundTransform = false;

				int scaleX = 0;
				int scaleY = 5;
				int offsetX = 3;
				int offsetY = 7;
				if (bFoundTransform && pTransformVar->GetType() == MATERIAL_VAR_TYPE_MATRIX)
				{
				// FULL 100%
				VMatrix matrix = pTransformVar->GetMatrixValue();
				matrix.Base()[scaleX] = 1.0f;
				matrix.Base()[scaleY] = 1.0f;
				matrix.Base()[offsetX] = 0.0f;
				matrix.Base()[offsetY] = 0.0f;
				pTransformVar->SetMatrixValue(matrix);
				}
				}
				}
				}*/
			}
		}
	}

	bool bFoundColor;
	IMaterialVar* pColorVar = null;
	if (m_pMaterial)
		pColorVar = m_pMaterial->FindVar("$envmaptint", &bFoundColor, false);
	else
		bFoundColor = false;

	if (bFoundColor && pColorVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		if (!m_pAVRConVar)
			m_pAVRConVar = cvar->FindVar("avr");

		if (m_pAVRConVar->GetBool())
		{
			if (!m_pAVRAmpConVar)
				m_pAVRAmpConVar = cvar->FindVar("avramp");

			float peak = g_pAnarchyManager->GetAudioPeakValue() * m_pAVRAmpConVar->GetFloat();
			if (peak > 1.0f)
				peak = 1.0f;
			else if (peak < 0.2f)
				peak = 0.2f;
				
			int avrValue = m_pAVRConVar->GetInt();
			if (avrValue == 1 || avrValue == 2)
			{
				float hue = g_pAnarchyManager->GetHueShifter();

				// per-object rainbow
				if (avrValue == 2)
				{
					hue += (pC_BaseEntity->ProxyRandomValue() * 360.0f);
					while (hue > 360.0f)
						hue = hue - 360.0f;// -hue;
				}

				m_hueColor.SetColor(255, 0, 0);
				m_hueColor = TransformH(m_hueColor, hue);
			}
			else if (avrValue == 3)
				m_hueColor.SetColor(255, 0, 0);
			else if (avrValue == 4)
				m_hueColor.SetColor(0, 255, 0);
			else if (avrValue == 5)
				m_hueColor.SetColor(0, 0, 255);

			//pColorVar->SetVecValue // willw ant to use this one when actually tossing 3 floats at this vector every time.
			//DevMsg("Peak: %02f\tGreen: %i\tFactor: %02f\n", peak, m_hueColor.g(), (m_hueColor.g() / 255.0));
			pColorVar->SetVecValue((m_hueColor.r() / 255.0) * peak, (m_hueColor.g() / 255.0) * peak, (m_hueColor.b() / 255.0) * peak);
			//m_pMaterial->ColorModulate(peak * 2.0, peak * 2.0, peak * 2.0);//	<-- this doesn't do shit to the web textures.
			//pColorVar->SetVecComponentValue(peak, 1);
			//pColorVar->SetVecValue((m_hueColor.r() / 255.0) * peak, (m_hueColor.g() / 255.0) * peak, (m_hueColor.b() / 255.0) * peak);
		}
	}


	//if (pSelectedEmebeddedInstance && pSelectedEmebeddedInstance->GetLastRenderedFrame() < 0)
	//{
	//	m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
	//	m_pCurrentTexture = pOriginalTexture;
	//}


	//*/
	
	// even if we didn't find a new web tab this bind, continue acting as if the old one is still active.
	/*
	if (m_pCurrentEmbeddedInstance && m_pCurrentEmbeddedInstance != m_pEmbeddedInstance)
	{
		m_pCurrentEmbeddedInstance->OnProxyBind(pC_BaseEntity);
	}
	*/
}

void CWebSurfaceProxy::PrepareRefreshItemTextures(std::string itemId, std::string channel, bool bStaticInvoke)
{
	if (!m_pMaterialTextureVar)
		return;

	std::string textureName;
	CUtlMap<std::string, ITexture*>* pUtlMap;// = (channel == "marquee") ? &s_marqueeSimpleImages : &s_screenSimpleImages;
	if (channel == "marquee")
		pUtlMap = &s_marqueeSimpleImages;
	else if (channel == "avatar")
		pUtlMap = &s_avatarSimpleImages;
	else
		pUtlMap = &s_screenSimpleImages;

	if (itemId != "")
	{
		int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
		if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
		{
			ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
			//if (pTexture && !pTexture->IsError() && pTexture->IsProcedural())
			//	textureName = pTexture->GetName();

			//if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture && !pTexture->IsError() && pTexture->IsProcedural() )//&& (textureName.find("image_") == 0))
			if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture)
			{
				m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
				m_pCurrentTexture = m_pOriginalTexture;
			}
		}
	}
	else
	{
		if (m_originalId == "images" && m_pOriginalTexture && !m_pOriginalTexture->IsError())//m_originalId.find("image_") == 0
		{
			unsigned int count = pUtlMap->Count();
			for (int i = count - 1; i >= 0; i--)
			{
				ITexture* pTexture = pUtlMap->Element(i);
				if (pTexture)
					textureName = pTexture->GetName();

				if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture && !pTexture->IsError() && pTexture->IsProcedural() && (textureName.find("image_") == 0))// || textureName.find("canvas_") == 0))
				{
					m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
					m_pCurrentTexture = m_pOriginalTexture;
					pTexture->SetTextureRegenerator(null);

					/*
					pTexture->SetTextureRegenerator(null);
					g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(pTexture);
					g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(pTexture);
					*/
				}
			}
		}
	}

	if (channel == "ALL")
	{
		pUtlMap = &s_marqueeSimpleImages;
		if (itemId != "")
		{
			// Do the exact same thing, but using the new pUtlMap...
			// TODO: Generalize this logic.
			int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
			if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
			{
				ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
				if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture)
				{
					m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
					m_pCurrentTexture = m_pOriginalTexture;
				}
			}
		}
		else
		{
			if (m_originalId == "images" && m_pOriginalTexture && !m_pOriginalTexture->IsError())
			{
				unsigned int count = pUtlMap->Count();
				for (int i = count-1; i >= 0; i--)
				{
					ITexture* pTexture = pUtlMap->Element(i);
					if (pTexture)
						textureName = pTexture->GetName();

					if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture && !pTexture->IsError() && pTexture->IsProcedural() && (textureName.find("image_") == 0))
					{
						m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
						m_pCurrentTexture = m_pOriginalTexture;
						pTexture->SetTextureRegenerator(null);

						/*
						pTexture->SetTextureRegenerator(null);
						g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(pTexture);
						g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(pTexture);
						*/
					}
				}
			}
		}


		pUtlMap = &s_avatarSimpleImages;
		if (itemId != "")
		{
			// Do the exact same thing, but using the new pUtlMap...
			// TODO: Generalize this logic.
			int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
			if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
			{
				ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
				if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture)
				{
					m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
					m_pCurrentTexture = m_pOriginalTexture;
				}
			}
		}
		else
		{
			if (m_originalId == "images" && m_pOriginalTexture && !m_pOriginalTexture->IsError())
			{
				unsigned int count = pUtlMap->Count();
				for (int i = count - 1; i >= 0; i--)
				{
					ITexture* pTexture = pUtlMap->Element(i);
					if (pTexture)
						textureName = pTexture->GetName();

					if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture && !pTexture->IsError() && pTexture->IsProcedural() && (textureName.find("image_") == 0))
					{
						m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
						m_pCurrentTexture = m_pOriginalTexture;
						pTexture->SetTextureRegenerator(null);

						/*
						pTexture->SetTextureRegenerator(null);
						g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(pTexture);
						g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(pTexture);
						*/
					}
				}
			}
		}
	}


	// OLD STD::MAP STUFF HERE...
	/*
	auto channelIt = s_simpleImages.begin();
	while (channelIt != s_simpleImages.end())
	{
		if (channel == "ALL" || channelIt->first == channel)
		{
			auto itemIt = channelIt->second.find(itemId);
			if (itemIt != channelIt->second.end())
			{
				ITexture* pTexture = itemIt->second;
				if (pTexture && m_pMaterialTextureVar->GetTextureValue() == pTexture)
				{
					m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
					m_pCurrentTexture = m_pOriginalTexture;
				}
			}

			if (channel != "ALL")
				break;
		}

		channelIt++;
	}
	*/

	//if (g_pAnarchyManager->IsInSourceGame())
	//{
	//if ((textureName.find("image_") == 0))

	/* MOSTLY WORKS FINE...
	if (m_originalId.find("image_") == 0)
	{
		m_pMaterial = null;
		m_pOriginalTexture = null;
		m_pMaterialTextureVar = null;
		m_pMaterialDetailBlendFactorVar = null;
	}
	*/
}

void CWebSurfaceProxy::UnreferenceTexture(ITexture* pTexture)
{
	//if (g_pAnarchyManager->IsShuttingDown())
	if (g_pAnarchyManager->IsShuttingDown() || (g_pAnarchyManager->ShouldPrecacheInstance() && !g_pAnarchyManager->IsLevelInitialized()))//if (g_pAnarchyManager->ShouldPrecacheInstance() || g_pAnarchyManager->IsShuttingDown())
		return;	// Disabled (unless precache_instances is FALSE) on January 9th for the model precache update, as a result of dynamic WebSurfaceProxies now doing nothing in ReleaseStuff - causing their proxy's initialized msg not to be sent again on the next map.

	//DevMsg("CWebSurfaceProxy::UnreferenceTexture\n");

	if (m_pCurrentTexture == pTexture && m_pCurrentTexture != m_pOriginalTexture && m_pMaterialTextureVar && m_pMaterialTextureVar->IsDefined() && m_pMaterialTextureVar->IsTexture() && m_pMaterialTextureVar->GetTextureValue() == pTexture)
	{
//		m_pOriginalTexture = g_pMaterialSystem->FindTexture(m_originalTextureName.c_str(), TEXTURE_GROUP_MODEL);// , m_originalTextureGroupName.c_str());
		if (m_pOriginalTexture->IsError())//!m_pOriginalTexture || 
		{
			DevMsg("ERROR: Original texture was error!\n");
			//m_pOriginalTexture = null;
		}
		else if (!m_pOriginalTexture)
		{
			DevMsg("WARNING: Original texture was NULL!\n");
		}
		else
		{
			m_pMaterialTextureVar->SetTextureValue(m_pOriginalTexture);
			//m_pOriginalTexture->DecrementReferenceCount();	// because it was incremented earlier.
			m_pCurrentTexture = m_pOriginalTexture;
			//m_pOriginalTexture->DecrementReferenceCount();
			//m_pOriginalTexture->SetTextureRegenerator(null);
		}
	}
}

void CWebSurfaceProxy::UnreferenceEmbeddedInstance(C_EmbeddedInstance* pEmbeddedInstance)
{
	if (m_pCurrentEmbeddedInstance == pEmbeddedInstance)
	{
		DevMsg("Unreferencing embedded instance from material proxy: m_pCurrentEmbeddedInstance\n");
		m_pCurrentEmbeddedInstance = null;
	}

	if (m_pEmbeddedInstance == pEmbeddedInstance)
	{
		DevMsg("Unreferencing embedded instance from material proxy: m_pEmbeddedInstance\n");
		m_pEmbeddedInstance = null;
	}
}

ITexture* CWebSurfaceProxy::GetItemTexture(std::string itemId, std::string channel)
{
	CUtlMap<std::string, ITexture*>* pUtlMap;// = (m_originalSimpleImageChannel == "screen") ? &s_screenSimpleImages : &s_marqueeSimpleImages;
	if (channel == "marquee")
		pUtlMap = &s_marqueeSimpleImages;
	else if (channel == "avatar")
		pUtlMap = &s_avatarSimpleImages;
	else
		pUtlMap = &s_screenSimpleImages;

	int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
	if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
		return pUtlMap->Element(iSimpleImagesMapIndex);
	return null;
}

void CWebSurfaceProxy::RefreshItemTextures(std::string itemId, std::string channel, bool bStaticInvoke)
{
	if (bStaticInvoke)
	{
		CUtlMap<std::string, ITexture*>* pUtlMap;// = (channel == "marquee") ? &s_marqueeSimpleImages : &s_screenSimpleImages;
		if (channel == "marquee")
			pUtlMap = &s_marqueeSimpleImages;
		else if (channel == "avatar")
			pUtlMap = &s_avatarSimpleImages;
		else
			pUtlMap = &s_screenSimpleImages;


		if (itemId != "")
		{
			int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
			if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
			{
				ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
				if (pTexture)
				{
					// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
					// FIXME: do work...

					// aaaand theeeeen....
					pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
					//DevMsg("Deleted texture.\n");
				}

				//delete pTexture;	// To simulate what std::map does with its erase method!
				pUtlMap->RemoveAt(iSimpleImagesMapIndex);
			}
		}
		else
		{
			unsigned int count = pUtlMap->Count();
			for (unsigned int i = 0; i < count; i++)
			{
				ITexture* pTexture = pUtlMap->Element(i);
				if (pTexture)
				{
					// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
					// FIXME: do work...

					// aaaand theeeeen....
					//g_pMaterialSystem->ReloadTextures();
					if (!pTexture->IsError() && pTexture->IsProcedural())// && !g_pAnarchyManager->IsInSourceGame())
						pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
					//DevMsg("Deleted texture.\n");
				}

				//delete pTexture;	// To simulate what std::map does with its erase method!
				//pUtlMap->RemoveAt(i);
			}

			for (unsigned int i = 0; i < count; i++)
				pUtlMap->RemoveAt(i);
		}

		if (channel == "ALL")
		{
			pUtlMap = &s_marqueeSimpleImages;

			if (itemId != "")
			{
				// Do the exact same thing, but using the new pUtlMap...
				// TODO: Generalize this logic.
				int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
				if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
				{
					ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
					if (pTexture)
					{
						// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
						// FIXME: do work...

						// aaaand theeeeen....
						pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
						//DevMsg("Deleted texture 2.\n");
					}

					//delete pTexture;	// To simulate what std::map does with its erase method!
					pUtlMap->RemoveAt(iSimpleImagesMapIndex);
				}
			}
			else
			{
				unsigned int count = pUtlMap->Count();
				for (unsigned int i = 0; i < count; i++)
				{
					ITexture* pTexture = pUtlMap->Element(i);
					if (pTexture)
					{
						// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
						// FIXME: do work...

						// aaaand theeeeen....
						if (!pTexture->IsError() && pTexture->IsProcedural())// && !g_pAnarchyManager->IsInSourceGame())
							pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
						//DevMsg("Deleted texture.\n");
					}

					//delete pTexture;	// To simulate what std::map does with its erase method!
					//pUtlMap->RemoveAt(i);
				}

				for (unsigned int i = 0; i < count; i++)
					pUtlMap->RemoveAt(i);
			}

			pUtlMap = &s_avatarSimpleImages;
			if (itemId != "")
			{
				// Do the exact same thing, but using the new pUtlMap...
				// TODO: Generalize this logic.
				int iSimpleImagesMapIndex = pUtlMap->Find(itemId);
				if (iSimpleImagesMapIndex != pUtlMap->InvalidIndex())
				{
					ITexture* pTexture = pUtlMap->Element(iSimpleImagesMapIndex);
					if (pTexture)
					{
						// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
						// FIXME: do work...

						// aaaand theeeeen....
						pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
						//DevMsg("Deleted texture 2.\n");
					}

					//delete pTexture;	// To simulate what std::map does with its erase method!
					pUtlMap->RemoveAt(iSimpleImagesMapIndex);
				}
			}
			else
			{
				unsigned int count = pUtlMap->Count();
				for (unsigned int i = 0; i < count; i++)
				{
					ITexture* pTexture = pUtlMap->Element(i);
					if (pTexture)
					{
						// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
						// FIXME: do work...

						// aaaand theeeeen....
						if (!pTexture->IsError() && pTexture->IsProcedural())// && !g_pAnarchyManager->IsInSourceGame())
							pTexture->SetTextureRegenerator(null);	// we WANT to delete this texture rtfn, but safer to let it stick around until map transition.
						//DevMsg("Deleted texture.\n");
					}

					//delete pTexture;	// To simulate what std::map does with its erase method!
					//pUtlMap->RemoveAt(i);
				}

				for (unsigned int i = 0; i < count; i++)
					pUtlMap->RemoveAt(i);
			}
		}

		//m_originalMaterialName m_original
	}

	/*
	for (int iSimpleImagesMapIndex = s_simpleImages.FirstInorder(); iSimpleImagesMapIndex != s_simpleImages.InvalidIndex(); iSimpleImagesMapIndex = s_simpleImages.NextInorder(iSimpleImagesMapIndex))
	{
		if (channel == "ALL" || s_simpleImages.Key(iSimpleImagesMapIndex) == channel)
		{
			CUtlMap<std::string, ITexture*> itemMap = s_simpleImages.Element(iSimpleImagesMapIndex);

			int iItemMapIndex = itemMap.Find(itemId);
			if (iItemMapIndex != itemMap.InvalidIndex())
			{
				ITexture* pTexture = itemMap.Element(iItemMapIndex);
				if (pTexture)
				{
					// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
					// FIXME: do work...

					// aaaand theeeeen....
				}

				delete pTexture;	// To simulate what std::map does with its erase method!
				itemMap.RemoveAt(iItemMapIndex);
			}

			if (channel != "ALL")
				break;
		}
	}
	*/


	/*
	// OLD STD::STRING STUFF HERE...
	auto channelIt = s_simpleImages.begin();
	while (channelIt != s_simpleImages.end())
	{
		if (channel == "ALL" || channelIt->first == channel)
		{
			auto itemIt = channelIt->second.find(itemId);
			if (itemIt != channelIt->second.end())
			{
				ITexture* pTexture = itemIt->second;
				if (pTexture)
				{
					// if there's already a texture for this item, we're gonna have to replace it.  So that means releasing it first.
					// FIXME: do work...

					// aaaand theeeeen....
				}

				channelIt->second.erase(itemIt);
			}

			if (channel != "ALL")
				break;
		}

		channelIt++;
	}
	*/

	//if (g_pAnarchyManager->IsInSourceGame())
	//{
		// re-acquire the material, if we need to
		//engine->ExecuteClientCmd(VarArgs("mat_reloadmaterial \"%s\"", m_materialName.c_str()));

	/* MOSTLY WORKS
		engine->ExecuteClientCmd(VarArgs("mat_reloadmaterial \"%s\"", m_materialName.c_str()));
		IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(m_materialName.c_str(), m_materialGroupName.c_str());
		if (pMaterial)
		{
			m_pMaterial = pMaterial;
			m_materialName = m_pMaterial->GetName();
			m_materialGroupName = m_pMaterial->GetTextureGroupName();

			// set all the original stuff
			bool found;
			IMaterialVar* pMaterialVar = m_pMaterial->FindVar("$basetexture", &found, false);

			if (found)
			{
				m_pMaterialTextureVar = pMaterialVar;
				m_pOriginalTexture = pMaterialVar->GetTextureValue();
				m_originalTextureName = (m_pOriginalTexture && !m_pOriginalTexture->IsError()) ? m_pOriginalTexture->GetName() : "";
				//	m_pOriginalTexture->SetTextureRegenerator(s_pWebSurfaceRegen);

				bool bFoundDetailBlendFactorVar;
				m_pMaterialDetailBlendFactorVar = m_pMaterial->FindVar("$detailblendfactor", &bFoundDetailBlendFactorVar);

				//pMaterialVar = m_pMaterial->FindVar("id", &found, false);
				//m_originalId = (found) ? pMaterialVar->GetStringValue() : "";

				//pMaterialVar = m_pMaterial->FindVar("url", &found, false);
				//m_originalUrl = (found) ? pMaterialVar->GetStringValue() : "";

				//DevMsg("Original URL is: %s\n", m_originalUrl.c_str());

				//pMaterialVar = m_pMaterial->FindVar("autocreate", &found, false);
				//m_iOriginalAutoCreate = (found) ? pMaterialVar->GetIntValue() : 0;

				//pMaterialVar = m_pMaterial->FindVar("simpleimage", &found, false);
				//m_bOriginalSimpleImage = (found) ? (pMaterialVar->GetIntValue() != 0) : false;

				pMaterialVar = m_pMaterial->FindVar("simpleimagechannel", &found, false);
				//m_originalSimpleImageChannel = (found) ? pMaterialVar->GetStringValue() : "";
				//DevMsg("Material Dynamic Image Channel: %s\n", m_originalSimpleImageChannel.c_str());
			}
		}
		*/
	//}
}

	//DevMsg("WebSurfaceProxy: OnBind\n");
	// The objective is to fill these two values
	//DynamicImage* pImage = null;
	//ITexture* pTexture = null;

	// If we don't have a main texture for this proxy yet, create it now.
	// TODO: Tests to confirm what the above comment means. (When does that happen??)
//	if( !m_pTexture )
//	{
//		m_pTexture = this->CreateTexture();
//	}

//	C_LiveView* pLiveView = null;
	// Check if the proxy is being used on a simple image entity
//	if( pC_BaseEntity )
	//{
		/*
		C_PropSimpleImageEntity* pPropSimpleImageEntity = null;
		pPropSimpleImageEntity = dynamic_cast<C_PropSimpleImageEntity*>(pC_BaseEntity);
		if( pPropSimpleImageEntity )
		{
			if( m_pImageInstances.find(pC_BaseEntity->entindex()) == m_pImageInstances.end() )
			{
				// We need to create a new SimpleImage and ITexture for this entity instance
				pImage = new DynamicImage;
				//pImage->file = pPropSimpleImageEntity->GetImageURL(m_channel);
				pImage->status = 0;
				pImage->texture = this->CreateTexture(pC_BaseEntity);
				pTexture = pImage->texture;

				// Store the pointer for next time
				m_pImageInstances[pC_BaseEntity->entindex()] = pImage;
			}
			else
			{
				pImage = m_pImageInstances[pC_BaseEntity->entindex()];
				pTexture = pImage->texture;
			}

			// Now that we have a pImage, check if we need to flag it as active on an entity
			if( pPropSimpleImageEntity == pPropSimpleImageEntity->GetActiveEntity() && m_channel == 1 )
			{
				pLiveView = dynamic_cast<C_LiveView*>(C_AnarchyManager::GetSelf()->GetWebViewManager()->GetLiveViewDetailsWebView());
			}
		}
		*/
//	}

	// If proxy is not on an entity, then use the default Simple Image and texture
	/*
	if( !pImage )
	{
		pImage = &m_image;
		pImage->texture = m_pTexture;

		pTexture = pImage->texture;
	}
	*/

	//g_pAnarchyManager->GetWebManager()->Update();	// This gets handled elsewhere

	/*
	if( pImage->status == 0 )
	{
		s_pDynamicRegen->SetDynamicImage(pImage);
		pTexture->Download();

		// Set the loading texture
		m_pTextureVar->SetTextureValue(m_pLoadingTexture);
	}
	else if( pImage->status == 1 )
		// Set the loading texture
		m_pTextureVar->SetTextureValue(m_pLoadingTexture);
	else if( pImage->status == 2 )
	{
		// Set the custom texture
		m_pTextureVar->SetTextureValue(pTexture);
	}

	// Check if this SimpleImage is owned by a LiveView
	if( pLiveView && pLiveView->IsEarlyReady() )
	{
		// Swap in the LiveView's texture
		m_pTextureVar->SetTextureValue(pLiveView->GetTexture());

		// Call the update logic on our owner. It will re-render the texture if needed
		pLiveView->Update();
	}
	*/
//}