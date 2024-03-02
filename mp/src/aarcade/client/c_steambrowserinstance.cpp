// ;..\..\portaudio\lib\portaudio_x86.lib

#include "cbase.h"
#include "aa_globals.h"

//#include "aa_globals.h"
//#include "c_steambrowserinstance.h"
#include "c_inputlistenersteambrowser.h"
#include "c_anarchymanager.h"
//#include "../../../public/vgui_controls/Controls.h"  
#include "vgui/IInput.h"
//#include "vgui/VGUI.h"
#include "c_canvasregen.h"
#include "c_embeddedinstance.h"
#include "inputsystem/iinputsystem.h"

#include "../../public/bitmap/tgawriter.h"
#include "../../public/pixelwriter.h"

//#include <mutex>
#include "../../../public/vgui_controls/HTML.h"

#include "pixelwriter.h"

// for generating timestamps to use in filenmaes of task screenshots (takeScreenshotNow)
#include <chrono>
#include <iomanip> // put_time
#include <fstream>
#include <sstream> // stringstream
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

vgui::KeyCode C_SteamBrowserInstance::KeyCode_VirtualKeyToVGUI(int key)
{
	// Some tools load vgui for localization and never use input
	if (!g_pInputSystem)
		return KEY_NONE;
	return g_pInputSystem->VirtualKeyToButtonCode(key);
}

int C_SteamBrowserInstance::KeyCode_VGUIToVirtualKey(vgui::KeyCode code)
{
	// Some tools load vgui for localization and never use input
	if (!g_pInputSystem)
		return VK_RETURN;

	return g_pInputSystem->ButtonCodeToVirtualKey(code);
}

void C_SteamBrowserInstance::GetLastMouse(float &fMouseX, float &fMouseY)
{
	fMouseX = m_fLastMouseX;
	fMouseY = m_fLastMouseY;
}

///*
//-----------------------------------------------------------------------------
// Purpose: return the bitmask of any modifier keys that are currently down
//-----------------------------------------------------------------------------
int C_SteamBrowserInstance::GetKeyModifiersAlt()
{
	// Any time a key is pressed reset modifier list as well
	int nModifierCodes = 0;
	if (vgui::input()->IsKeyDown(KEY_LCONTROL) || vgui::input()->IsKeyDown(KEY_RCONTROL))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_CtrlDown;

	if (vgui::input()->IsKeyDown(KEY_LALT) || vgui::input()->IsKeyDown(KEY_RALT))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_AltDown;

	if (vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_ShiftDown;

#ifdef OSX
	// for now pipe through the cmd-key to be like the control key so we get copy/paste
	if (vgui::input()->IsKeyDown(KEY_LWIN) || vgui::input()->IsKeyDown(KEY_RWIN))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_CtrlDown;
#endif

	return nModifierCodes;
}
//*/

C_SteamBrowserInstance::C_SteamBrowserInstance()
{
	m_uAtlasWidth = AA_ATLAS_INSTANCE_WIDTH;
	m_uAtlasHeight = AA_ATLAS_INSTANCE_HEIGHT;
	m_bTakeScreenshot = false;
	m_bWoke = false;
	m_fNeedsWakupJuice = 0.0f;
	m_pProjectorFixConVar = null;
	m_pOutputTestDomConVar = null;
	m_bSteamworksCopying = false;
	DevMsg("SteamBrowserInstance: Constructor\n");
	m_bCanGoBack = false;
	m_bCanGoForward = false;
	m_pTexture = null;
	m_iLastRenderedFrame = -1;
	m_pLastFrameData = null;
	m_bReadyForNextFrame = true;
	m_bCopyingFrame = false;
	m_bReadyToCopyFrame = false;
	m_pPostData = null;
	m_initialURL = "";
	m_bIsDirty = false;
	m_unHandle = 0;
	m_iLastVisibleFrame = -1;
	m_URL = "";
	m_title = "";
	m_id = "";
	m_iOriginalEntIndex = -1;
	m_fLastMouseX = 0.5f;
	m_fLastMouseY = 0.5f;
	m_pYouTubeEndBehavior = null;
	m_bBlankShotPending = false;
	m_bUseVideoFilters = false;
	m_iChromaLightR = -1;
	m_iChromaLightG = -1;
	m_iChromaLightB = -1;
	m_iChromaDarkR = -1;
	m_iChromaDarkG = -1;
	m_iChromaDarkB = -1;
	m_flChromaA1 = -1;
	m_flChromaA2 = -1;

//	if (!steamapicontext->SteamHTMLSurface()->Init())
	//	DevMsg("CRITICAL ERROR: Failed to initialize the Steamworks browser!\n");
}

C_SteamBrowserInstance::~C_SteamBrowserInstance()
{
	DevMsg("SteamBrowserInstance: Destructor\n");
}
/*
void C_SteamBrowserInstance::DoDefunctDestruct(bool& result)
{
	DevMsg("SteamBrowserInstance: DoDefunctDestruct %s\n", m_id.c_str());

	//if (!m_bReadyForNextFrame || m_bCopyingFrame)
	if (!m_bReadyForNextFrame || m_bCopyingFrame)
	{
		DevMsg("CRITICAL WARNING: Frame is STILL copying right now, but wants to self descruct! %i %i\n", m_bReadyForNextFrame, m_bCopyingFrame);
		result = false;
	}
	else
	{
		g_pAnarchyManager->GetCanvasManager()->UnreferenceEmbeddedInstance(this);
		g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(m_pTexture);
		g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->NotifyInstanceAboutToDie(this);

		steamapicontext->SteamHTMLSurface()->RemoveBrowser(m_unHandle);

		m_bReadyForNextFrame = false;
		m_bReadyToCopyFrame = false;

		if (m_pLastFrameData)
		{
			free(m_pLastFrameData);
			m_pLastFrameData = null;
		}

		m_pPostData = "";

//		if (m_pPostData)
	//	{
	//		free(m_pPostData);
	//		m_pPostData = null;
	//	}

		m_bDying = true;
		result = true;
		delete this;
	}

}
*/

void C_SteamBrowserInstance::SelfDestruct()
{
	/*DevMsg("SteamBrowserInstance: SelfDestruct %s\n", m_id.c_str());
	DevMsg("\tInstance Texture Name: %s\n", m_pTexture->GetName());
	DevMsg("\tIs Texture Loaded: %i\n", (g_pMaterialSystem->IsTextureLoaded(m_pTexture->GetName())));

	// TODO: poll every AArcade sub-system that could be holding a reference to: ***the texture*** or the instance
	// Including:
	// - Canvas Manager's DisplayInstance
	// - Canvas Manager's FirstInstanceToDisplay
	// - Input Manager's EmbeddedInstance
	// - Input Slate's CanvasTexture
	// - 

	DevMsg("\tIs Canvas Manager's DisplayInstance: %i\n", (g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() == this));
	//DevMsg("\tIs Canvas Manager's FirstInstanceToDisplay: %i\n", (g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay() == this));	// calling this upon map transition crashes if the firsttodisplay instance is among the open ones that gets closed all.
	DevMsg("\tIs Input Manager's EmbeddedInstance: %i\n", (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this));
	DevMsg("\tIs Input Slate's CanvasTexture: %i\n", (g_pAnarchyManager->GetInputManager()->GetInputSlateCanvasTexture() == m_pTexture));
	*/

	if (m_id == "aai")
		g_pAnarchyManager->GetAAIManager()->Reset();

	m_bIsDirty = false;

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

		//g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);


	// selected steamworks browser instance
	if (this == g_pAnarchyManager->GetSteamBrowserManager()->GetSelectedSteamBrowserInstance())
		g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(null);

	// focused steamworks browser instance
	if (this == g_pAnarchyManager->GetSteamBrowserManager()->GetFocusedSteamBrowserInstance())
		g_pAnarchyManager->GetSteamBrowserManager()->FocusSteamBrowserInstance(null);

	// display instance
	if (this == g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance())
		g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);

	// first instance to display
	// do nothing.

	// is input instance
	if (this == g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance())
	{
		if (g_pAnarchyManager->GetInputManager()->GetInputMode())
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

		g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(null);	// reduntant?
	}

	// tell the canvas manager we're on our way out
	this->CleanUpTexture();	// m_pTexture will be NULL after this.

	//DevMsg("Closing browser w/ handle %u\n", m_unHandle);
	//steamapicontext->SteamHTMLSurface()->RemoveBrowser(m_unHandle);	// This gets closed earlier on now in an attempt to avoid crashing.

	m_bReadyForNextFrame = false;
	m_bReadyToCopyFrame = false;

	if (m_pLastFrameData)
	{
		free(m_pLastFrameData);
		m_pLastFrameData = null;
	}

	m_pPostData = "";

	/* this was a const char* passed us by the Steamworks HTML Surface API. possibly invalided on their end.
	if (m_pPostData)
	{
		free(m_pPostData);
		m_pPostData = null;
	}
	*/

	//g_pAnarchyManager->GetSteamBrowserManager()->SetSystemCursor(0);
	this->DeleteAllAPIObjects();

	g_pAnarchyManager->AddToastMessage(VarArgs("Web Browser Closed (%i running)", g_pAnarchyManager->GetSteamBrowserManager()->GetInstanceCount()));
	delete this;
}

void C_SteamBrowserInstance::Init(std::string id, std::string url, std::string title, const char* pchPostData, int entindex)
{
	std::string goodTitle = (title != "") ? title : "Untitled Steamworks Browser Instance";
	m_title = title;
	m_id = id;

	m_pYouTubeEndBehavior = cvar->FindVar("youtube_end_behavior");
	m_pProjectorFixConVar = cvar->FindVar("projector_fix");

	if (m_id == "")
		m_id = g_pAnarchyManager->GenerateUniqueId();

	m_initialURL = url;
	m_pPostData = (void*)pchPostData;
	m_iOriginalEntIndex = entindex;

	g_pAnarchyManager->GetSteamBrowserManager()->AddFreshSteamBrowserInstance(this);

	// create the texture (each instance has its own texture)
	std::string textureName = "canvas_";
	textureName += m_id;

	int iWidth = (id == "hud") ? AA_HUD_INSTANCE_WIDTH : AA_EMBEDDED_INSTANCE_WIDTH;
	int iHeight = (id == "hud") ? AA_HUD_INSTANCE_HEIGHT : AA_EMBEDDED_INSTANCE_HEIGHT;

	if (id == "aai") {
		iWidth = cvar->FindVar("atlas_width")->GetInt();//AA_ATLAS_INSTANCE_WIDTH;
		iHeight = cvar->FindVar("atlas_height")->GetInt();//AA_ATLAS_INSTANCE_HEIGHT;

		m_uAtlasWidth = iWidth;// AA_ATLAS_INSTANCE_WIDTH
		m_uAtlasHeight = iHeight;// AA_ATLAS_INSTANCE_HEIGHT
	}

	if (id == "SteamTalker")
	{
		iWidth = 32;
		iHeight = 32;
	}

	//https://developer.valvesoftware.com/wiki/Valve_Texture_Format#Image_flags
	int flags = (0x0100 | 0x2000 | 0x0800 | 0x2000000);//| 0x0200

	if (g_pAnarchyManager->ShouldTextureClamp())
		flags |= (0x0004 | 0x0008);

	int multiplyer = 1.0;// g_pAnarchyManager->GetDynamicMultiplyer();
	if (!g_pMaterialSystem->IsTextureLoaded(textureName.c_str()))
	{
		//IMAGE_FORMAT_BGRA8888
		if ( true )
			m_pTexture = g_pMaterialSystem->CreateProceduralTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, iWidth * multiplyer, iHeight * multiplyer, IMAGE_FORMAT_BGRA8888, flags);//1);
		else
			m_pTexture = g_pMaterialSystem->CreateProceduralTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, iWidth * multiplyer, iHeight * multiplyer, IMAGE_FORMAT_BGR888, flags);//1);
	}
	else
	{
		m_pTexture = g_pMaterialSystem->FindTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, false, flags);
		g_pAnarchyManager->GetCanvasManager()->TextureNotDeferred(m_pTexture);
	}

	DevMsg("Init: %s\n", m_id.c_str());

	// get the regen and assign it
	CCanvasRegen* pRegen = g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen();
	m_pTexture->SetTextureRegenerator(pRegen);

	// get rid of garbage data
	pRegen->SetEmbeddedInstance(this);
	this->SetBlankShotPending(true);
	this->GetTexture()->Download();
	pRegen->SetEmbeddedInstance(null);

	//SteamAPICall_t hAPICall = steamapicontext->SteamHTMLSurface()->CreateBrowser("", "");
	g_pAnarchyManager->GetSteamBrowserManager()->GetBrowserListener()->CreateBrowser(m_id);
	//m_BrowserReadyInitial.Set(hAPICall, this, &C_SteamBrowserInstance::BrowserInstanceBrowserReadyInitial);


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

void C_SteamBrowserInstance::OnBrowserInstanceReady(unsigned int unHandle)
{
	if ( m_unHandle )
		DevMsg("CRITICAL ERROR: Steamworks browser already has a handle!\n");

	m_unHandle = unHandle;

	steamapicontext->SteamHTMLSurface()->SetBackgroundMode(unHandle, false);

	DevMsg("C_SteamBrowserInstance::OnBrowserInstanceReady w/ instanceID %s and handle %u\n", m_id.c_str(), unHandle);

	int iWidth = (m_id == "hud") ? AA_HUD_INSTANCE_WIDTH : AA_EMBEDDED_INSTANCE_WIDTH;
	int iHeight = (m_id == "hud") ? AA_HUD_INSTANCE_HEIGHT : AA_EMBEDDED_INSTANCE_HEIGHT;

	if (this->m_id == "aai") {
		iWidth = m_uAtlasWidth;// AA_ATLAS_INSTANCE_WIDTH;
		iHeight = m_uAtlasHeight;// AA_ATLAS_INSTANCE_HEIGHT;
	}

	steamapicontext->SteamHTMLSurface()->SetSize(m_unHandle, iWidth, iHeight);

	g_pAnarchyManager->GetSteamBrowserManager()->OnSteamBrowserInstanceCreated(this);
	g_pAnarchyManager->AddToastMessage(VarArgs("Web Browser Opened (%i running)", g_pAnarchyManager->GetSteamBrowserManager()->GetInstanceCount()));

	m_syncFixInitialURL = m_initialURL;
	//steamapicontext->SteamHTMLSurface()->LoadURL(m_unHandle, m_initialURL.c_str(), "");
}

void C_SteamBrowserInstance::OnBrowserInstanceWantsToClose()
{
	DevMsg("TODO: A steamworks browser instance wants to close. Acquiesce to the request.\n");
}

void C_SteamBrowserInstance::OnAPIObjectCreated(std::string sessionId, std::string objectId, int iEntityIndex, int iParentEntityIndex)
{
	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(iEntityIndex);
	C_DynamicProp* pParentBaseEntity = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iParentEntityIndex));
	if (!pBaseEntity || !pParentBaseEntity)
		return;

	aaAPIObjectTransform_t* pTransform = m_apiObjectTransforms[objectId];
	if (!pTransform)
		return;

	float flBaseScale = pParentBaseEntity->GetModelScale() * 0.75;
	Vector baseScaleVec;
	baseScaleVec.Init();
	baseScaleVec.x = flBaseScale;
	baseScaleVec.y = flBaseScale;
	baseScaleVec.z = flBaseScale;

	pTransform->entityIndex = iEntityIndex;
	pBaseEntity->SetParent(pParentBaseEntity);

	// apply our transform NOW, because we probably ahven't been able to yet.
	QAngle rot = pTransform->rotation;
	pBaseEntity->SetLocalAngles(rot);

	// now mutate the stuff to send it
	Vector goodPos = pTransform->position;
	goodPos *= baseScaleVec;
	pBaseEntity->SetLocalOrigin(goodPos);

	// now mutate the stuff to send it
	float flGoodScale = pTransform->scale * flBaseScale;
	C_BaseAnimating* pBaseAnimating = dynamic_cast<C_BaseAnimating*>(pBaseEntity);
	if (pBaseAnimating)
		pBaseAnimating->SetModelScale(flGoodScale);
}

void C_SteamBrowserInstance::DeleteAllAPIObjects()
{
	aaAPIObjectTransform_t* pVictim;
	auto objectIt = m_apiObjectTransforms.begin();
	while (objectIt != m_apiObjectTransforms.end())
	{
		pVictim = objectIt->second;
		engine->ClientCmd(VarArgs("removeobject %i", pVictim->entityIndex));
		delete pVictim;

		objectIt->second = null;
		objectIt++;
	}

	m_apiObjectTransforms.clear();
}

void C_SteamBrowserInstance::ProcessAPIObjectUpdate(std::string id, std::string model, std::string position, std::string rotation, std::string scale, std::string sequence, std::string removed)
{
	if (id == "")
		return;

	aaAPIObjectTransform_t* pTransform = null;
	auto it = m_apiObjectTransforms.find(id);

	if (removed != "")
	{
		if (it != m_apiObjectTransforms.end())
		{
			int iEntityIndex = it->second->entityIndex;
			if (iEntityIndex >= 0)
				engine->ClientCmd(VarArgs("removeobject %i;", iEntityIndex));

			delete it->second;
			m_apiObjectTransforms.erase(it);
		}
		return;
	}

	C_DynamicProp* pParentProp = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(this->GetOriginalEntIndex()));
	if (!pParentProp)
		return;

	float flBaseScale = pParentProp->GetModelScale() * 0.75;
	Vector baseScaleVec;
	baseScaleVec.Init();
	baseScaleVec.x = flBaseScale;
	baseScaleVec.y = flBaseScale;
	baseScaleVec.z = flBaseScale;

	float flScale = 1.0;
	if (scale != "")
		flScale = Q_atof(scale.c_str());

	C_BaseEntity* pBaseEntity = null;
	if (it == m_apiObjectTransforms.end())
	{
		DevMsg("Create new object!");
		//C_BaseEntity* pBaseEntity = CreateEntityByName("prop_dynamic");

		//C_BaseEntity* pBaseEntity = CreateEntityByName("waterbullet");
		//if ( pEnt->InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )

		std::string modelFile;

		/*
		std::string modelId;
		modelId = ?;
		KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId));
		modelFile = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
		*/

		QAngle rot;
		rot.Init();

		if (rotation != "")
			UTIL_StringToVector(rot.Base(), rotation.c_str());

		Vector pos;
		pos.Init();
		if (position != "")
			UTIL_StringToVector(pos.Base(), position.c_str());


		std::string objectId = id;
		modelFile = model;// "models/sithlord/giftbox.mdl";

		//m_objectMap[id] = -1;	// this gets set to an actual index after the server-side is done actually creating it.
		pTransform = new aaAPIObjectTransform_t();
		pTransform->id = id;
		pTransform->position = pos;
		pTransform->rotation = rot;
		pTransform->scale = flScale;
		pTransform->entityIndex = -1;

		m_apiObjectTransforms[id] = pTransform;

		// now mutate the stuff to send it
		Vector goodPos = pos;
		goodPos *= baseScaleVec;

		float flGoodScale = flScale * flBaseScale;

		engine->ClientCmd(VarArgs("create_api_object \"%s\" %i \"%s\" \"%s\" \"%f %f %f\" \"%f %f %f\" %f", this->GetId().c_str(), this->GetOriginalEntIndex(), objectId.c_str(), modelFile.c_str(), goodPos.x, goodPos.y, goodPos.z, rot.x, rot.y, rot.z, flGoodScale));

		//pBaseEntity = C_BaseEntity::Instance(this->GetOriginalEntIndex());
		//if (pBaseEntity)
		//	m_objectMap[id] = pBaseEntity;this->GetOriginalEntIndex()
	}
	else
	{
		pTransform = it->second;
		pBaseEntity = (pTransform->entityIndex >= 0) ? C_BaseEntity::Instance(pTransform->entityIndex) : null;
	}

	if (!pBaseEntity)
		return;

	if (rotation != "")
	{
		QAngle rot;
		rot.Init();
		UTIL_StringToVector(rot.Base(), rotation.c_str());
		pTransform->rotation = rot;

		pBaseEntity->SetLocalAngles(rot);
	}

	if (position != "")
	{
		Vector pos;
		pos.Init();
		UTIL_StringToVector(pos.Base(), position.c_str());
		pTransform->position = pos;

		// now mutate the stuff to send it
		Vector goodPos = pTransform->position;
		goodPos *= baseScaleVec;
		pBaseEntity->SetLocalOrigin(goodPos);
	}

	if (scale != "")
	{
		//float flScale = Q_atof(scale.c_str());
		pTransform->scale = flScale;

		// now mutate the stuff to send it
		float flGoodScale = flScale * flBaseScale;

		C_BaseAnimating* pBaseAnimating = dynamic_cast<C_BaseAnimating*>(pBaseEntity);
		if (pBaseAnimating)
			pBaseAnimating->SetModelScale(flGoodScale);
	}

	if (sequence != "")
	{
		//float flScale = Q_atof(scale.c_str());
		//pTransform->scale = flScale;

		// now mutate the stuff to send it
		//float flGoodScale = flScale * flBaseScale;

		C_BaseAnimating* pBaseAnimating = dynamic_cast<C_BaseAnimating*>(pBaseEntity);
		int iSequenceIndex = pBaseAnimating->LookupSequence(sequence.c_str());
		if (iSequenceIndex >= 0)
			pBaseAnimating->SetSequence(iSequenceIndex);
		//if (pBaseAnimating)
		//	pBaseAnimating->SetModelScale(flGoodScale);
	}
}

void C_SteamBrowserInstance::JavaScriptAPICommand(std::string cmd, std::vector<std::string> dataBuf)
{
	//DevMsg("Command: %s\n", cmd.c_str());
	if (cmd == "3dupdate")
	{
		std::vector<std::string> removedObjectIds;

		std::string id = "";
		std::string model = "";
		std::string position = "";
		std::string rotation = "";
		std::string scale = "";
		std::string sequence = "";
		std::string removed = "";
		for (unsigned int i = 0; i < dataBuf.size(); i += 2)
		{
			if (dataBuf[i] == "id")
			{
				if (id != "")
				{
					if (removed != "")
					{
						removedObjectIds.push_back(id);
						this->ProcessAPIObjectUpdate(id, model, position, rotation, scale, sequence, removed);
					}
					else
					{
						bool bIsRemoved = false;
						for (unsigned int i = 0; i < removedObjectIds.size(); i++)
						{
							if (removedObjectIds[i] == id)
							{
								bIsRemoved = true;
								break;
							}
						}

						if (!bIsRemoved)
							this->ProcessAPIObjectUpdate(id, model, position, rotation, scale, sequence, removed);
					}
				}

				id = dataBuf[i + 1];
				model = "";
				position = "";
				rotation = "";
				scale = "";
				sequence = "";
				removed = "";
			}
			else if (dataBuf[i] == "mdl")
				model = dataBuf[i + 1];
			else if (dataBuf[i] == "position")
				position = dataBuf[i + 1];
			else if (dataBuf[i] == "rotation")
				rotation = dataBuf[i + 1];
			else if (dataBuf[i] == "scale")
				scale = dataBuf[i + 1];
			else if (dataBuf[i] == "sequence")
				sequence = dataBuf[i + 1];
			else if (dataBuf[i] == "removed")
				removed = dataBuf[i + 1];
		}

		if (id != "")
		{
			if (removed != "")
			{
				removedObjectIds.push_back(id);
				this->ProcessAPIObjectUpdate(id, model, position, rotation, scale, sequence, removed);
			}
			else
			{
				bool bIsRemoved = false;
				for (unsigned int i = 0; i < removedObjectIds.size(); i++)
				{
					if (removedObjectIds[i] == id)
					{
						bIsRemoved = true;
						break;
					}
				}

				if (!bIsRemoved)
					this->ProcessAPIObjectUpdate(id, model, position, rotation, scale, sequence, removed);
			}
		}
	}
}

bool C_SteamBrowserInstance::IsSelected()
{
	return (this == g_pAnarchyManager->GetSteamBrowserManager()->GetSelectedSteamBrowserInstance());
}

bool C_SteamBrowserInstance::HasFocus()
{
	return (this == g_pAnarchyManager->GetSteamBrowserManager()->GetFocusedSteamBrowserInstance());
}

bool C_SteamBrowserInstance::Focus()
{
	//return g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(this);
	//g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(this);
	g_pAnarchyManager->GetSteamBrowserManager()->FocusSteamBrowserInstance(this);
	return true;
}

bool C_SteamBrowserInstance::Blur()
{
	if (this == g_pAnarchyManager->GetSteamBrowserManager()->GetFocusedSteamBrowserInstance())
		g_pAnarchyManager->GetSteamBrowserManager()->FocusSteamBrowserInstance(this, false);
	return true;
}

bool C_SteamBrowserInstance::Select()
{
	return g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(this);
}

bool C_SteamBrowserInstance::Deselect()
{
	return g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(null);
}

void C_SteamBrowserInstance::Close()
{
	m_fNeedsWakupJuice = 0.0f;
	g_pAnarchyManager->GetSteamBrowserManager()->DestroySteamBrowserInstance(this);
}

void C_SteamBrowserInstance::SetUrl(std::string url)
{
	if (!m_pTexture || !m_unHandle)
		return;

	steamapicontext->SteamHTMLSurface()->LoadURL(m_unHandle, url.c_str(), "");
}

void C_SteamBrowserInstance::GoForward()
{
	if (!m_unHandle)
		return;

	if ( m_bCanGoForward )
		steamapicontext->SteamHTMLSurface()->GoForward(m_unHandle);
}

void C_SteamBrowserInstance::GoBack()
{
	if (!m_unHandle)
		return;

	if (m_bCanGoBack)
		steamapicontext->SteamHTMLSurface()->GoBack(m_unHandle);
}

void C_SteamBrowserInstance::Reload()
{
	if (!m_unHandle)
		return;

	steamapicontext->SteamHTMLSurface()->Reload(m_unHandle);
}

void C_SteamBrowserInstance::OnBrowserInstanceStartRequest(const char* pchURL, const char* pchTarget, const char* pchPostData, bool IsRedirect)
{
	//DevMsg("Start request URL: %s\n", pchURL);
	std::string urlBuf = pchURL;
	if (urlBuf.find("http://www.aarcadeapicall.com.net.org/?doc=") == 0)
	{
		steamapicontext->SteamHTMLSurface()->AllowStartRequest(m_unHandle, false);

		std::string stuff = urlBuf.substr(43, std::string::npos);

		if (!m_pOutputTestDomConVar)
			m_pOutputTestDomConVar = cvar->FindVar("output_test_dom");

		if (m_pOutputTestDomConVar->GetBool())
		{
			FileHandle_t fh = filesystem->Open("output_dom.txt", "w", "DEFAULT_WRITE_PATH");
			if (fh)
			{
				filesystem->Write(stuff.c_str(), stuff.size(), fh);

				filesystem->Close(fh);
			}
			m_pOutputTestDomConVar->SetValue(false);
		}

		C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(pEmbeddedInstance);	// FIXME: What if the browser being scraped is NOT a Steam browser instance?

		std::vector<std::string> params;
		if (pSteamBrowserInstance)
			params.push_back(pSteamBrowserInstance->GetURL());
		else
			params.push_back("");

		params.push_back(stuff);
		//DevMsg("URL is: %s\n", params[0].c_str());
		C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudInstance->DispatchJavaScriptMethod("arcadeHud", "onDOMGot", params);
	}
	else
	{
		if (!m_unHandle)
		{
			DevMsg("What he hell?  No handle here.\n");
			return;
		}

		//DevMsg("Start request allowed on handle %u\n", m_unHandle);
		//	if (!url || !Q_stricmp(url, "about:blank"))
		//		return true; // this is just webkit loading a new frames contents inside an existing page

		// TODO: This is where URL filtering would be applied.

		this->DeleteAllAPIObjects();

		steamapicontext->SteamHTMLSurface()->AllowStartRequest(m_unHandle, true);
			//m_fNeedsWakupJuice = engine->Time() + 0.3f;

		//steamapicontext->SteamHTMLSurface()->Reload(m_unHandle);
	}
}

void C_SteamBrowserInstance::OnBrowserInstanceFinishedRequest(const char* pchURL, const char* pchPageTitle)
{
	std::string url = pchURL;
	if (url.find("http://www.anarchyarcade.com/youtube_player.php") == 0 || url.find("http://www.smarcade.net/dlcv2/view_youtube.php") == 0)
		g_pAnarchyManager->GetAccountant()->Action("aa_tubes_watched", 1);



	if (!m_bWoke && m_fNeedsWakupJuice == 0.0f && url.find("data:") != 0)
	{
		m_fNeedsWakupJuice = engine->Time() + 0.3f;
		//m_bWoke = true;

		// wake up the new web tab, so that autoplay elements can work.
		//int iVirtualKeyCode = this->KeyCode_VGUIToVirtualKey(KEY_UP);
		//steamapicontext->SteamHTMLSurface()->KeyDown(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
		//steamapicontext->SteamHTMLSurface()->KeyUp(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
	}

	/*
	// wake up the new web tab, so that autoplay elements can work.
	int iVirtualKeyCode = this->KeyCode_VGUIToVirtualKey(KEY_UP);
	steamapicontext->SteamHTMLSurface()->KeyDown(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
	steamapicontext->SteamHTMLSurface()->KeyUp(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
	*/






		/*
		steamapicontext->SteamHTMLSurface()->MouseDown(m_unHandle, ISteamHTMLSurface::eHTMLMouseButton_Left);
		steamapicontext->SteamHTMLSurface()->MouseUp(m_unHandle, ISteamHTMLSurface::eHTMLMouseButton_Left);
		*/
	//}
	/*
	if (m_bDying)
	{
		DevMsg("Critical Error: Steamworks browser instance received a callback exactly as it was dying!\n");
		return;
	}
	*/

	// THIS IS NOW TRIGGERED FROM THE HUD
	/*
	if ( m_scraperId == "")	// only continue if we have an active scraper waiting for load finished events. (FIXME: This might change when the UI's address bar gets updated.)
		return;

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	std::vector<std::string> params;
	params.push_back(std::string(pchURL));
	params.push_back(m_scraperId);
	params.push_back(m_scraperItemId);
	params.push_back(m_scraperField);
	pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onBrowserFinishedRequest", params);
	*/
}

//void C_SteamBrowserInstance::BrowserInstanceOpenLinkInTab(HTML_OpenLinkInNewTab_t* pResult)
void C_SteamBrowserInstance::OnBrowserInstanceOpenLinkInTab(const char* pchURL)
{
	DevMsg("TODO: A steamworks browser instance wants to open a URL in a new window. Acquiesce to the request (probably).\n");
}

void C_SteamBrowserInstance::OnBrowserInstanceNeedsPaint(const char* pBGRA, unsigned int unWide, unsigned int unTall, unsigned int unUpdateX, unsigned int unUpdateY, unsigned int unUpdateWide, unsigned int unUpdateTall, unsigned int unScrollX, unsigned int unScrollY, float flPageScale, unsigned int unPageSerial)
{
	if (g_pAnarchyManager->IsPaused() || m_bSteamworksCopying || m_bCopyingFrame || !m_bReadyForNextFrame || m_bIsDirty)//if (g_pAnarchyManager->IsPaused() || !m_bReadyForNextFrame)
	{
		this->IncrementSkippedFrames();
		return;
	}

	// copy the frame to our buffer, if we're not currently reading from it.

	// Only copy if this instance was seen last frame
	//if (m_iLastVisibleFrame >= gpGlobals->framecount - 1)
	
	if (!m_bWoke && m_fNeedsWakupJuice > 0.0f && m_fNeedsWakupJuice <= engine->Time())
	{
		m_bWoke = true;
		m_fNeedsWakupJuice = 0.0f;

		// wake up the new web tab, so that autoplay elements can work.
		int iVirtualKeyCode = this->KeyCode_VGUIToVirtualKey(KEY_UP);
		steamapicontext->SteamHTMLSurface()->KeyDown(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
		steamapicontext->SteamHTMLSurface()->KeyUp(m_unHandle, iVirtualKeyCode, ISteamHTMLSurface::k_eHTMLKeyModifier_None);
	}
	
	// Only copy if this instance if it has been seen b4
	//if (m_iLastVisibleFrame > m_iLastRenderedFrame )//&& m_iLastVi gpGlobals->framecount - 1)
	if (m_bReadyForNextFrame && m_iLastVisibleFrame >= gpGlobals->framecount - 1)
	{
		// Steamworks copying onto m_pLastFrameData
		this->CopyLastFrame(pBGRA, unWide, unTall, 4);
	}
}

// steamworks copying to m_pLastFrame
void C_SteamBrowserInstance::CopyLastFrame(const void* data, unsigned int width, unsigned int height, unsigned int depth)
{
	// NOTE: !m_bReadyForNextFrame is making us ignore all requests to copy until the previous frame is read. (Checking for m_bIsDirty here has the same effect as well.)
	// NOTE 2: m_bCopyingFrame is only really needed if Steamworks browser is allowed to REPLACE the frame that is waiting to render with a newer one. (because the isDirty & readyfornext checks wouldnt' be used in that case.)
	/*if (m_bSteamworksCopying || m_bCopyingFrame || !m_bReadyForNextFrame || m_bIsDirty)
	{
		this->IncrementSkippedFrames();
		return;
	}*/
	
	m_bSteamworksCopying = true;

	m_bReadyForNextFrame = false;
	m_bReadyToCopyFrame = false;	// this is only needed if steamworks browser is allowed to replace the frame that is waiting to render with a newer one.


	if (m_pLastFrameData)
	{
		free(m_pLastFrameData);	// make sure to set this to non-garbage right away
		m_pLastFrameData = null;
	}

	void* dest = malloc(width*height*depth);
	Q_memcpy(dest, data, width*height*depth);

	m_pLastFrameData = dest;
	m_bReadyToCopyFrame = true;
	m_bSteamworksCopying = false;
	m_bIsDirty = true;
	this->IncrementRenderedFrames();
	//DevMsg("FPS: %u\n", this->GetFPS());

	// if we started dying during our last copy there, really make us die now.
	//if (m_bDefunct)
	//	g_pAnarchyManager->GetSteamBrowserManager()->DestroyDefunctInstance(this);
		//g_pAnarchyManager->GetSteamBrowserManager()->DestroySteamBrowserInstance(this);
		//this->SelfDestruct();
}

void C_SteamBrowserInstance::OnBrowserInstanceURLChanged(const char* pchURL, const char* pchPostData, bool bIsRedirect, const char* pchPageTitle, bool bNewNavigation)
{
	if (Q_strcmp(pchURL, ""))
	{
		m_URL = pchURL;
	}

	// HUD notifications should ONLY happen if THIS is the instance that the HUD is displaying right now!
	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
	{
		// notify the HUD
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		if (pHudBrowserInstance)
		{
			std::vector<std::string> params;
			params.push_back(m_URL);
			params.push_back(m_scraperId);
			params.push_back(m_scraperItemId);
			params.push_back(m_scraperField);

			pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onURLChanged", params);
		}
	}
}


void C_SteamBrowserInstance::OnMouseWheeled(int delta)
{
	if (!m_unHandle || g_pAnarchyManager->IsPaused())
		return;

	DevMsg("Web Browser instance mouse wheeled: %i\n", delta);
	steamapicontext->SteamHTMLSurface()->MouseWheel(m_unHandle, 20 * delta);
	//m_pWebView->InjectMouseWheel(20 * delta, 0);
}

void C_SteamBrowserInstance::OnBrowserInstanceChangedTitle(const char* pchTitle)
{
	m_title = (pchTitle == "") ? "Untitled" : pchTitle;

	/*
	bool bHandled = false;
	if (m_pYouTubeEndBehavior && m_title.find(" - YouTube (ended)") == m_title.length()-18)
	{
		int iYouTubeEndBehavior = m_pYouTubeEndBehavior->GetInt();
		if (iYouTubeEndBehavior == 1 || iYouTubeEndBehavior == 2)
		{
			if (iYouTubeEndBehavior == 2)
			{
				// auto-play behavior
				//	When a video ends, the next nearest object (that has a YouTube video) plays.
				//	The last used nearby object PER original object must be remembered in order for this to work as continuous play.
				//	This could probably most easily be handled in an arcadeHud callback, while treating it the same as an auto-close behavior here in the C++.
				//	The arcadeHud handler will just determine the next nearby object and then request it start playing.
				//	It'd be nice to put a max-distance too, and loop through the YT videos within range.  That way it doesn't play music that is far away / unrelated.

				C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
				if (pHudBrowserInstance)
				{
					std::vector<std::string> params;
					params.push_back(this->GetId());
					pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onAYouTubeEnded", params);	// FIXME: OBSOLETE!  Get rid of that arcadeHud message (probably) and pack titles in with general status updates.
				}
			}

			// auto-close behavior
			if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
				g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

			this->Close();
			bHandled = true;
		}
	}

	if (!bHandled)
	{*/
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		if (pHudBrowserInstance)
		{
			std::vector<std::string> params;
			params.push_back(this->GetId());
			params.push_back(m_title);
			if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
				params.push_back("isSelf");
			else
				params.push_back("isNotSelf");
			pHudBrowserInstance->DispatchJavaScriptMethod("arcadeHud", "onTitleChanged", params);	// FIXME: OBSOLETE!  Get rid of that arcadeHud message (probably) and pack titles in with general status updates.
		}
	//}
}

void C_SteamBrowserInstance::OnBrowserInstanceSearchResults(unsigned int unResults, unsigned int unCurrentMatch)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceCanGoBackAndForward(bool bCanGoBack, bool bCanGoForward)
{
	bool bNeedsNotify = false;
	if (m_bCanGoBack != bCanGoBack || m_bCanGoForward != bCanGoForward)
		bNeedsNotify = true;

	m_bCanGoBack = bCanGoBack;
	m_bCanGoForward = bCanGoForward;

	// HUD notifications should ONLY happen if THIS is the instance that the HUD is displaying right now!
	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == this)
	{
		if (bNeedsNotify)	// FIXME: This probably gets called more often than it needs to, ESPECIALLY during initial page load.
			g_pAnarchyManager->HudStateNotify();
	}
}

void C_SteamBrowserInstance::OnBrowserInstanceHorizontalScroll(unsigned int unScrollMax, unsigned int unScrollCurrent, float flPageScale, bool bVisible, unsigned int unPageSize)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceVerticalScroll(unsigned int unScrollMax, unsigned int unScrollCurrent, float flPageScale, bool bVisible, unsigned int unPageSize)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceLinkAtPosition(unsigned int x, unsigned int y, const char* pchURL, bool bInput, bool bLiveLink)
{
	// NOTE: x and y are NOT currently set by the Steamworks browser in this callback.
}

void C_SteamBrowserInstance::OnBrowserInstanceJSAlert(const char* pchMessage)
{
	std::string alertText = pchMessage;
	steamapicontext->SteamHTMLSurface()->JSDialogResponse(m_unHandle, false);

	if (alertText.find("aajsapi_") != std::string::npos)
	{
		std::string buf = alertText.substr(8);
		size_t found = buf.find(":");
		if (found != std::string::npos)
		{
			std::string cmd = buf.substr(0, found);
			std::string dataBuf = buf.substr(found + 1);

			std::vector<std::string> datas;
			g_pAnarchyManager->Tokenize(dataBuf, datas, "::");
			this->JavaScriptAPICommand(cmd, datas);

			//DevMsg("JS API Thing: %s : %s\n", cmd.c_str(), value.c_str());
			
		}
	}
	else if (m_id == "aai" && alertText == "aai_readynow")
	{
		g_pAnarchyManager->GetAAIManager()->OnReadyNow();
	}
}

void C_SteamBrowserInstance::OnBrowserInstanceJSConfirm(const char* pchMessage)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceFileOpenDialog(const char* pchTitle, const char* pchInitialFile)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceNewWindow(const char* pchURL, unsigned int unX, unsigned int unY, unsigned int unWide, unsigned int unTall, unsigned int unNewWindow_BrowserHandle)
{
	// TODO:
	//	- Popup blocking for non-whitelisted source or destination URLs.

	DevMsg("Replacing Steamworks web tab with a pop-up that it opened.\n");
	g_pAnarchyManager->AddToastMessage("Web Browser Allowed Popup");

	std::string uri = pchURL;
	this->SetUrl(uri);

	DevMsg("Closing browser w/ handle %u\n", unNewWindow_BrowserHandle);
	steamapicontext->SteamHTMLSurface()->RemoveBrowser(unNewWindow_BrowserHandle);
}

void C_SteamBrowserInstance::OnBrowserInstanceSetCursor(unsigned int eMouseCursor)
{
	unsigned long cursor = g_pAnarchyManager->GetSteamBrowserManager()->GetSystemCursor(eMouseCursor);
	this->SetCursor(cursor);
	g_pAnarchyManager->GetSteamBrowserManager()->SetSystemCursor(cursor, eMouseCursor);
}

void C_SteamBrowserInstance::OnBrowserInstanceStatusText(const char* pchMsg)
{
	//DevMsg("Text is (%i): %s\n", Q_strlen(pchMsg), pchMsg);
}

void C_SteamBrowserInstance::OnBrowserInstanceShowToolTip(const char* pchMsg)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceUpdateToolTip(const char* pchMsg)
{
}

void C_SteamBrowserInstance::OnBrowserInstanceHideTollTip()
{
}

void C_SteamBrowserInstance::OnMouseMove(float x, float y)
{
	if (!m_unHandle)
		return;

	if (g_pAnarchyManager->GetSteamBrowserManager()->GetSelectedSteamBrowserInstance() != this)
		return;

	unsigned int width = (m_id == "hud") ? AA_HUD_INSTANCE_WIDTH : AA_EMBEDDED_INSTANCE_WIDTH;
	unsigned int height = (m_id == "hud") ? AA_HUD_INSTANCE_HEIGHT : AA_EMBEDDED_INSTANCE_HEIGHT;

	if (this->m_id == "aai") {
		width = m_uAtlasWidth;//  AA_ATLAS_INSTANCE_WIDTH;
		height = m_uAtlasHeight;//  AA_ATLAS_INSTANCE_HEIGHT;
	}

	int goodX = (width * x) / 1;
	int goodY = (height * y) / 1;
	m_fLastMouseX = x;
	m_fLastMouseY = y;

	steamapicontext->SteamHTMLSurface()->MouseMove(m_unHandle, goodX, goodY);
}

void C_SteamBrowserInstance::OnMousePressed(vgui::MouseCode code)
{
	if (!m_unHandle)
		return;

	ISteamHTMLSurface::EHTMLMouseButton goodButton;

	switch (code)
	{
		case vgui::MouseCode::MOUSE_LEFT:
			goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Left;
			break;

		case vgui::MouseCode::MOUSE_RIGHT:
			goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Right;
			break;

		case vgui::MouseCode::MOUSE_MIDDLE:
			goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Middle;
			break;
	}

	steamapicontext->SteamHTMLSurface()->MouseDown(m_unHandle, goodButton);
}

void C_SteamBrowserInstance::OnMouseReleased(vgui::MouseCode code)
{
	if (!m_unHandle)
		return;

	ISteamHTMLSurface::EHTMLMouseButton goodButton;

	switch (code)
	{
	case vgui::MouseCode::MOUSE_LEFT:
		goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Left;
		break;

	case vgui::MouseCode::MOUSE_RIGHT:
		goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Right;
		break;

	case vgui::MouseCode::MOUSE_MIDDLE:
		goodButton = ISteamHTMLSurface::EHTMLMouseButton::eHTMLMouseButton_Middle;
		break;
	}

	steamapicontext->SteamHTMLSurface()->MouseUp(m_unHandle, goodButton);
}

void C_SteamBrowserInstance::Update()
{
	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;

	if (g_pAnarchyManager->IsPaused())
		return;

	if (!m_unHandle)
		return;

	/*
	if (this->m_title == "MoleWhack" && m_unHandle)
	{
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(this->GetOriginalEntIndex());
		if (pBaseEntity)
		{
			QAngle angles = pBaseEntity->GetAbsAngles();
			//DevMsg("PYR: %f %f %f\n", angles.x, angles.y, angles.z);
			std::string code = VarArgs("testSetRotValue(%f, %f, %f);", angles.x, angles.y, angles.z);
			steamapicontext->SteamHTMLSurface()->ExecuteJavascript(m_unHandle, code.c_str());
		}
	}
	*/

	//if (m_info->state == 1)
	//if (m_pLastFrameData)

		this->OnProxyBind(null);
}

void C_SteamBrowserInstance::TakeScreenshot()
{
	m_bTakeScreenshot = true;
}

void C_SteamBrowserInstance::TakeScreenshotNow(ITexture* pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch, unsigned int depth)
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
	g_pFullFileSystem->CreateDirHierarchy(screenshotFolder.c_str(), "DEFAULT_WRITE_PATH");
	std::string goodFile = screenshotFolder + "/" + scrubbedItemTitle + " " + dateString + ".tga";

	/*
	unsigned int screenshotNumber = 0;
	std::string goodFile = screenshotFolder + "/" + "screenshot" + std::string(VarArgs("%04u", screenshotNumber)) + ".tga";
	while (g_pFullFileSystem->FileExists(goodFile.c_str(), "DEFAULT_WRITE_PATH"))
	{
		screenshotNumber++;
		goodFile = screenshotFolder + "/" + "screenshot" + std::string(VarArgs("%04u", screenshotNumber)) + ".tga";
	}

	DevMsg("File name: %s\n", goodFile.c_str());
	*/

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

// copying m_pLastFrameData onto world texture
void C_SteamBrowserInstance::CopyLastFrame(unsigned char* dest, unsigned int width, unsigned int height, size_t pitch, unsigned int depth)
{
	if (!m_unHandle)
		return;

	if (m_bCopyingFrame || !m_bReadyToCopyFrame || g_pAnarchyManager->GetSuspendEmbedded())
		return;

	if (g_pAnarchyManager->IsPaused())
		return;

//	DevMsg("SteamBrowserInstance: Start copy\n");

	m_bCopyingFrame = true;
	m_bReadyToCopyFrame = false;
	//memcpy(dest, m_pLastFrameData, pitch * height);

	//DevMsg("Frame number: %i\n", this->GetNumRenderedFrames());
	if (this->GetNumRenderedFrames() > 1) {
		Q_memcpy(dest, m_pLastFrameData, pitch * height);
	}

//	if (m_info->videoformat == RETRO_PIXEL_FORMAT_RGB565)
//		this->ResizeFrameFromRGB565(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);
//	else if (m_info->videoformat == RETRO_PIXEL_FORMAT_XRGB8888)
//		this->ResizeFrameFromXRGB8888(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 4, width, height, pitch, depth);
//	else
//		this->ResizeFrameFromRGB1555(m_info->lastframedata, dest, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, 3, width, height, pitch, depth);

	m_bReadyForNextFrame = true;
	m_bCopyingFrame = false;
	m_bIsDirty = false;

	// if we started dying during our last copy there, really make us die now.
	//if (m_bDying)
		//this->SelfDestruct();

//	DevMsg("SteamBrowserInstance: Finish copy\n");
}

void C_SteamBrowserInstance::OnProxyBind(C_BaseEntity* pBaseEntity)
{
	/*
	if (m_bDefunct)
	{
		g_pAnarchyManager->GetSteamBrowserManager()->DestroyDefunctInstance(this);
		return;
	}
	*/

	if (!m_unHandle)
		return;

	if (g_pAnarchyManager->IsPaused())
		return;

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

	//bool bIsPriority = g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(this);

	// visiblity test
	if (m_iLastVisibleFrame < gpGlobals->framecount)
	{
		m_iLastVisibleFrame = gpGlobals->framecount;
		//g_pAnarchyManager->GetCanvasManager()->RenderSeen(this);
		//if (g_pAnarchyManager->GetCanvasManager()->RenderSeen(this))
		//{


		if (this->IsDirty() && g_pAnarchyManager->GetCanvasManager()->RenderSeen(this) && g_pAnarchyManager->GetCanvasManager()->ShouldRender(this))// && (m_bReadyForNextFrame || m_bBlankShotPending))      //m_bIsDirty && m_bReadyToCopyFrame && !m_bSteamworksCopying &&
		//if (this->IsDirty())
		{
			//DevMsg("Is Dirty\n");
			//if (g_pAnarchyManager->GetCanvasManager()->RenderSeen(this))
			//{
				//DevMsg("Is Render Seen\n");
				//if (g_pAnarchyManager->GetCanvasManager()->ShouldRender(this))
				//{
					//DevMsg("Should Render\n");
					/*if (!bIsPriority)
					{
					if (!g_pAnarchyManager->GetCanvasManager()->RenderQueueAdd(this))
					return;
					}
					else
					{
					if (!g_pAnarchyManager->GetCanvasManager()->IncrementVisiblePriorityCanvasesCurrentFrame(this))
					return;
					}*/

					Render();
				//}
			//}
		}
		//}
	}

}

void C_SteamBrowserInstance::Render()
{
	//if (g_pAnarchyManager->IsPaused())
	//	return;

//	if (m_id == "images")
	//	return;
	//DevMsg("Rendering texture: %s\n", m_pTexture->GetName());
	//	DevMsg("Render Web Tab: %s\n", this->GetTexture()->Ge>GetId().c_str());
	//DevMsg("WebTab: Render: %s on %i\n", m_id.c_str(), gpGlobals->framecount);

	//if (m_bIsDirty && m_bReadyToCopyFrame)	// THIS IS THE ABSOLUTE LATEST THAT A RENDER CALL CAN BE ABORTED WITHOUT FLICKER SIDE-EFFECTS!@@@@@@
	//{
		g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(this);
		m_pTexture->Download();
		g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(null);
	//}

	m_iLastRenderedFrame = gpGlobals->framecount;

	//if (g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(this))
	//{
		//if (m_iLastRenderedFrame != g_pAnarchyManager->GetCanvasManager()->GetLastPriorityRenderedFrame() - 1)
		//	DevMsg("*Web Dif: %i\n", m_iLastRenderedFrame - g_pAnarchyManager->GetCanvasManager()->GetLastPriorityRenderedFrame());

		g_pAnarchyManager->GetCanvasManager()->AllowRender(this);
	//}
	//else
	//{
		//if (m_iLastRenderedFrame != g_pAnarchyManager->GetCanvasManager()->GetLastRenderedFrame() - 1)
		//	DevMsg("Web Dif: %i\n", m_iLastRenderedFrame - g_pAnarchyManager->GetCanvasManager()->GetLastRenderedFrame());

	//	g_pAnarchyManager->GetCanvasManager()->AllowRender(this);
	//}



		if (m_syncFixInitialURL != "") {
			steamapicontext->SteamHTMLSurface()->LoadURL(m_unHandle, m_syncFixInitialURL.c_str(), "");
			m_syncFixInitialURL = "";
		}
}

inline int calculateDistance(int c, int min, int max)
{
	if (c < min) return min - c;
	if (c > max) return c - max;

	return 0;
}

void C_SteamBrowserInstance::RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect)
{
	if (g_pAnarchyManager->IsPaused())
		return;

	if (g_pAnarchyManager->GetSuspendEmbedded())
		return;


	if (m_bBlankShotPending && engine->IsInGame())
	{
		//DevMsg("Applying blank shot...\n");
		CPixelWriter pixelWriter;
		pixelWriter.SetPixelMemory(pVTFTexture->Format(), pVTFTexture->ImageData(0, 0, 0), pVTFTexture->RowSizeInBytes(0));

		int xmax = pSubRect->x + pSubRect->width;
		int ymax = pSubRect->y + pSubRect->height;
		int x, y;
		for (y = pSubRect->y; y < ymax; ++y)
		{
			pixelWriter.Seek(pSubRect->x, y);
			for (x = pSubRect->x; x < xmax; ++x)
				pixelWriter.WritePixel(0, 0, 0);
		}

		m_bBlankShotPending = false;
		return;
	}
	else
	{

		if (!m_unHandle || !m_pLastFrameData)
			return;

		if (m_bIsDirty && m_bReadyToCopyFrame)// && !m_bSteamworksCopying)
		{
			//DevMsg("copy last frame\n");
			this->CopyLastFrame(pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);

			if (m_bTakeScreenshot) {
				this->TakeScreenshotNow(pTexture, pVTFTexture, pSubRect, pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
				m_bTakeScreenshot = false;
			}
			//DevMsg("copied\n");

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

				// put FPS on there
				//pixelWriter.SetPixelMemory(pVTFTexture->Format(), pVTFTexture->ImageData(0, 0, 0), pVTFTexture->RowSizeInBytes(0));
			}

			if (m_bUseVideoFilters)
			{
				CPixelWriter pixelWriter;
				pixelWriter.SetPixelMemory(pVTFTexture->Format(), pVTFTexture->ImageData(0, 0, 0), pVTFTexture->RowSizeInBytes(0));

				if (m_flChromaA1 < 0)
				{
					m_flChromaA1 = cvar->FindVar("video_chroma_a1")->GetFloat();
					m_flChromaA2 = cvar->FindVar("video_chroma_a2")->GetFloat();
				}

				int keyAlpha = 255;
				int r, g, b, a;
				float flRed, flGreen;
				float flAlpha;
				int xmax = pSubRect->x + pSubRect->width;
				int ymax = pSubRect->y + pSubRect->height;
				int x, y;
				for (y = pSubRect->y; y < ymax; ++y)
				{
					pixelWriter.Seek(pSubRect->x, y);
					for (x = pSubRect->x; x < xmax; ++x)
					{
						pixelWriter.ReadPixelNoAdvance(r, g, b, a);
						flRed = r / 255.0;
						flGreen = g / 255.0;
						flAlpha = 1.0 - m_flChromaA1*(flGreen - m_flChromaA2*flRed);
						//flAlpha *= -1.0;
						if (flAlpha < 0) {
							flAlpha = 0;
						}
						else if (flAlpha > 1) {
							flAlpha = 1;
						}

						if (flAlpha < 0.7) {
							r = 0;
							g = 0;
							b = 0;
						}

						keyAlpha = (flAlpha * 255.0) / 1;
						pixelWriter.WritePixel(r, g, b, keyAlpha);
					}
				}

				/*
				CPixelWriter pixelWriter;
				pixelWriter.SetPixelMemory(pVTFTexture->Format(), pVTFTexture->ImageData(0, 0, 0), pVTFTexture->RowSizeInBytes(0));

				if (m_iChromaLightR < 0)
				{
					Vector buf;
					UTIL_StringToVector(buf.Base(), cvar->FindVar("video_chroma_light")->GetString());

					m_iChromaLightR = buf.x / 1;
					m_iChromaLightG = buf.y / 1;
					m_iChromaLightB = buf.z / 1;

					UTIL_StringToVector(buf.Base(), cvar->FindVar("video_chroma_dark")->GetString());
					m_iChromaDarkR = buf.x / 1;
					m_iChromaDarkG = buf.y / 1;
					m_iChromaDarkB = buf.z / 1;
				}

				//int l_r = 110,
				//	l_g = 174,
				//	l_b = 127;

				//int d_r = 80,
				//	d_g = 151,
				//	d_b = 104;

				float tolerance = 0.05;
				int keyAlpha = 255;

				int r, g, b, a;

				float difference;

				int xmax = pSubRect->x + pSubRect->width;
				int ymax = pSubRect->y + pSubRect->height;
				int x, y;
				for (y = pSubRect->y; y < ymax; ++y)
				{
					pixelWriter.Seek(pSubRect->x, y);
					for (x = pSubRect->x; x < xmax; ++x)
					{
						pixelWriter.ReadPixelNoAdvance(r, g, b, a);

						difference = calculateDistance(r, m_iChromaDarkR, m_iChromaLightR) +
							calculateDistance(g, m_iChromaDarkG, m_iChromaLightG) +
							calculateDistance(b, m_iChromaDarkB, m_iChromaLightB);

						difference /= (255 * 3); // convert to percent

						if (difference < tolerance)
							keyAlpha = 0;
						else
							keyAlpha = a;

						pixelWriter.WritePixel(r, g, b, keyAlpha);
					}
				}*/
			}

			
		}
	}




	//if (m_info->state == 1)
	//DevMsg("copy last frame\n");
		//this->CopyLastFrame(pVTFTexture->ImageData(0, 0, 0), pSubRect->width, pSubRect->height, pSubRect->width * 4, 4);
//		DevMsg("Done copying frame.\n");
}

bool C_SteamBrowserInstance::IsDirty()
{
	return m_bIsDirty && m_bReadyToCopyFrame;// && !m_bReadyForNextFrame && m_bReadyToCopyFrame;// (m_bIsDirty && m_bReadyToCopyFrame && !m_bSteamworksCopying);
}

C_InputListener* C_SteamBrowserInstance::GetInputListener()
{
	return g_pAnarchyManager->GetSteamBrowserManager()->GetInputListener();
}

void C_SteamBrowserInstance::ResizeFrameFromXRGB8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth)
{
	//DevMsg("Thread ID: %u\n", ThreadGetCurrentId);
	//	uint uId = ThreadGetCurrentId();
	//	C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstance(uId);
	//	LibretroInstanceInfo_t* info = pLibretroInstance->GetInfo();
	//LibretroInstanceInfo_t* info = m_info;

	//if (!m_info->lastframedata)
	//	DevMsg("Main Lock\n");
	if (!m_pLastFrameData)
		return;

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

void C_SteamBrowserInstance::CleanUpTexture()
{
	if (m_pTexture && !m_pTexture->IsError() && m_pTexture->IsProcedural())
	{
		m_pTexture->SetTextureRegenerator(null);

		// save the last rendered image out as a TGA to use as a thumbnail
		if (m_pLastFrameData && !g_pAnarchyManager->GetCanvasManager()->GetItemTexture(m_originalItemId, "screen") && m_id != "aai" && m_id != "SteamTalker")
		{
			std::string filePath = "cache/snapshots";
			std::string fileName = filePath + "/";

			KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(this->GetOriginalItemId()));
			if (pItemKV)
			{
				fileName += g_pAnarchyManager->GenerateLegacyHash(pItemKV->GetString("file"));
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

					//this->ResizeFrameFromRGB888(m_info->lastframedata, pImage, m_info->lastframewidth, m_info->lastframeheight, m_info->lastframepitch, m_info->depth, width, height, pitch, depth);
					this->ResizeFrameFromXRGB8888(m_pLastFrameData, pImage, AA_EMBEDDED_INSTANCE_WIDTH, AA_EMBEDDED_INSTANCE_HEIGHT, AA_EMBEDDED_INSTANCE_WIDTH * 4, 4, width, height, pitch, depth);

					// allocate a buffer to write the tga into
					int iMaxTGASize = (width * height * depth);	// + 1024
					void *pTGA = malloc(iMaxTGASize);
					CUtlBuffer buffer(pTGA, iMaxTGASize);

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
		}
		// now continue with regular stuff

		DevMsg("Unref texture from: C_SteamBrowserInstance::CleanUpTexture\n");
		g_pAnarchyManager->GetCanvasManager()->UnreferenceEmbeddedInstance(this);
		g_pAnarchyManager->GetCanvasManager()->UnreferenceTexture(m_pTexture);
		g_pAnarchyManager->GetCanvasManager()->DoOrDeferTextureCleanup(m_pTexture);
		m_pTexture = null;
	}
}

C_EmbeddedInstance* C_SteamBrowserInstance::GetParentSelectedEmbeddedInstance()
{
	return g_pAnarchyManager->GetSteamBrowserManager()->GetSelectedSteamBrowserInstance();
}

void C_SteamBrowserInstance::OnKeyCodePressed(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState)
{
	if (!m_unHandle)
		return;

	// don't send alt button for now (it can cause crashes sometimes? (PROBABLY AN OBSOLETE ASSUMPTION!)
	//if (code == KEY_LALT || code == KEY_RALT || code == BUTTON_CODE_NONE || code == BUTTON_CODE_INVALID)
		//return;

	if (code == BUTTON_CODE_NONE || code == BUTTON_CODE_INVALID)
		return;

	// FIXME: Can crash at the following line, probably due to a Steamworks browser crash!! (from from the CODE_NONE and CODE_INVALID checks that didn't used to be included above...)
	// NO still crashes for multiple users.  Insufficent checks, need a better fix or figure out wtf is going on with this call.
	// -TRY: Making GetKeyModifiersAlt a MEMBER method.
	// -TRY: Getting the key modifiers BEFORE the KeyDown call.
	// - TRY: Printing debug info BEFORE the KeyDown call.
	// -TRY: Making KeyCode_VGUIToVirtualKey a MEMBER method.
	// -TRY: confirming that m_unHandle is valid. (with m_bDying)

	int iModifiers = this->GetKeyModifiersAlt();
	int iVirtualKeyCode = this->KeyCode_VGUIToVirtualKey(code);
	//DevMsg("Sending keypress to browser\n");
	steamapicontext->SteamHTMLSurface()->KeyDown(m_unHandle, iVirtualKeyCode, (ISteamHTMLSurface::EHTMLKeyModifiers)iModifiers);

	std::string s_output = this->GetOutput(code, bShiftState, bCtrlState, bAltState);
	if (s_output != "")
	{
		char a = s_output.at(0);
		const char *b = &a;

		wchar_t value;
		mbtowc(&value, b, 1);
		steamapicontext->SteamHTMLSurface()->KeyChar(m_unHandle, value, (ISteamHTMLSurface::EHTMLKeyModifiers)iModifiers);
	}
}

void C_SteamBrowserInstance::OnKeyCodeReleased(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState)
{
	if (!m_unHandle)
		return;

	// don't send alt button for now (it can cause crashes sometimes?
	if (code == KEY_LALT || code == KEY_RALT || code == BUTTON_CODE_NONE || code == BUTTON_CODE_INVALID)
		return;

	steamapicontext->SteamHTMLSurface()->KeyUp(m_unHandle, this->KeyCode_VGUIToVirtualKey(code), (ISteamHTMLSurface::EHTMLKeyModifiers)this->GetKeyModifiersAlt());
}

void C_SteamBrowserInstance::InjectJavaScript(std::string code)
{
	if (!m_unHandle)
		return;

	steamapicontext->SteamHTMLSurface()->ExecuteJavascript(m_unHandle, code.c_str());
}