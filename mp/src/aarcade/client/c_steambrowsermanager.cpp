#include "cbase.h"

//#include "aa_globals.h"
#include "c_steambrowsermanager.h"
#include "c_anarchymanager.h"
//#include "../../public/steam/steam_api.h"
//#include "Filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum EMouseCursor	// replica of the real one.  because these are defined privately in the steamworks browser header file.
{
	dc_user = 0,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_waitarrow,
	dc_crosshair,
	dc_up,
	dc_sizenw,
	dc_sizese,
	dc_sizene,
	dc_sizesw,
	dc_sizew,
	dc_sizee,
	dc_sizen,
	dc_sizes,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_blank, // don't show any custom cursor, just use your default
	dc_middle_pan,
	dc_north_pan,
	dc_north_east_pan,
	dc_east_pan,
	dc_south_east_pan,
	dc_south_pan,
	dc_south_west_pan,
	dc_west_pan,
	dc_north_west_pan,
	dc_alias,
	dc_cell,
	dc_colresize,
	dc_copycur,
	dc_verticaltext,
	dc_rowresize,
	dc_zoomin,
	dc_zoomout,
	dc_help,
	dc_custom,

	dc_last, // custom cursors start from this value and up
};

/*
enum CursorCode
{
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_waitarrow,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_blank, // don't show any custom vgui cursor, just let windows do it stuff (for HTML widget)
	dc_last,
	dc_alwaysvisible_push,
	dc_alwaysvisible_pop,
};
*/

C_SteamBrowserManager::C_SteamBrowserManager()
{
	DevMsg("SteamBrowserManager: Constructor\n");
	m_pBrowserListener = new C_SteamBrowserListener();
	m_bSoundEnabled = true;
	m_pSelectedSteamBrowserInstance = null;
	m_pFocusedSteamBrowserInstance = null;

	// STEAMWORKS BROWSER to NATIVE WINDOWS
	/*m_cursors[dc_user] = IDC_ARROW;
	m_cursors[dc_none] = IDC_ARROW;
	m_cursors[dc_arrow] = IDC_ARROW;
	m_cursors[dc_ibeam] = IDC_IBEAM;
	m_cursors[dc_hourglass] = IDC_WAIT;
	m_cursors[dc_waitarrow] = IDC_APPSTARTING;
	m_cursors[dc_crosshair] = IDC_CROSS;
	m_cursors[dc_up] = IDC_UPARROW;
	m_cursors[dc_sizenw] = IDC_SIZENWSE;
	m_cursors[dc_sizese] = IDC_SIZENWSE;
	m_cursors[dc_sizene] = IDC_SIZENESW;
	m_cursors[dc_sizesw] = IDC_SIZENESW;
	m_cursors[dc_sizew] = IDC_SIZEWE;
	m_cursors[dc_sizee] = IDC_SIZEWE;
	m_cursors[dc_sizen] = IDC_SIZENS;
	m_cursors[dc_sizes] = IDC_SIZENS;
	m_cursors[dc_sizewe] = IDC_SIZEWE;
	m_cursors[dc_sizens] = IDC_SIZENS;
	m_cursors[dc_sizeall] = IDC_SIZEALL;
	m_cursors[dc_no] = IDC_NO;
	m_cursors[dc_hand] = IDC_HAND;
	m_cursors[dc_blank] = IDC_ARROW;
	m_cursors[dc_middle_pan] = IDC_SIZEALL;
	m_cursors[dc_north_pan] = IDC_SIZENS;
	m_cursors[dc_north_east_pan] = IDC_SIZENESW;
	m_cursors[dc_east_pan] = IDC_SIZEWE;
	m_cursors[dc_south_east_pan] = IDC_SIZENWSE;
	m_cursors[dc_south_pan] = IDC_SIZENS;
	m_cursors[dc_south_west_pan] = IDC_SIZENESW;
	m_cursors[dc_west_pan] = IDC_SIZEWE;
	m_cursors[dc_north_west_pan] = IDC_SIZENWSE;
	m_cursors[dc_alias] = IDC_ARROW;
	m_cursors[dc_cell] = IDC_ARROW;
	m_cursors[dc_colresize] = IDC_SIZEWE;
	m_cursors[dc_copycur] = IDC_ARROW;
	m_cursors[dc_verticaltext] = IDC_IBEAM;
	m_cursors[dc_rowresize] = IDC_SIZENS;
	m_cursors[dc_zoomin] = IDC_SIZENS;
	m_cursors[dc_zoomout] = IDC_SIZENS;
	m_cursors[dc_help] = IDC_HELP;
	m_cursors[dc_custom] = IDC_ARROW;
	m_cursors[dc_last] = IDC_ARROW;*/

	/*
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_waitarrow,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_blank, // don't show any custom vgui cursor, just let windows do it stuff (for HTML widget)
	dc_last,
	dc_alwaysvisible_push,
	dc_alwaysvisible_pop,
	*/

	// STEAMWORKS BROWSER to SOURCE VGUI
	m_cursors[dc_user] = vgui::dc_user;
	m_cursors[dc_none] = vgui::dc_none;
	m_cursors[dc_arrow] = vgui::dc_arrow;
	m_cursors[dc_ibeam] = vgui::dc_ibeam;
	m_cursors[dc_hourglass] = vgui::dc_hourglass;
	m_cursors[dc_waitarrow] = vgui::dc_waitarrow;
	m_cursors[dc_crosshair] = vgui::dc_crosshair;
	m_cursors[dc_up] = vgui::dc_up;
	m_cursors[dc_sizenw] = vgui::dc_sizenwse;
	m_cursors[dc_sizese] = vgui::dc_sizenwse;
	m_cursors[dc_sizene] = vgui::dc_sizenesw;
	m_cursors[dc_sizesw] = vgui::dc_sizenesw;
	m_cursors[dc_sizew] = vgui::dc_sizewe;
	m_cursors[dc_sizee] = vgui::dc_sizewe;
	m_cursors[dc_sizen] = vgui::dc_sizens;
	m_cursors[dc_sizes] = vgui::dc_sizens;
	m_cursors[dc_sizewe] = vgui::dc_sizewe;
	m_cursors[dc_sizens] = vgui::dc_sizens;
	m_cursors[dc_sizeall] = vgui::dc_sizeall;
	m_cursors[dc_no] = vgui::dc_no;
	m_cursors[dc_hand] = vgui::dc_hand;
	m_cursors[dc_blank] = vgui::dc_blank;
	m_cursors[dc_middle_pan] = vgui::dc_sizeall;
	m_cursors[dc_north_pan] = vgui::dc_sizens;
	m_cursors[dc_north_east_pan] = vgui::dc_sizenesw;
	m_cursors[dc_east_pan] = vgui::dc_sizewe;
	m_cursors[dc_south_east_pan] = vgui::dc_sizenwse;
	m_cursors[dc_south_pan] = vgui::dc_sizens;
	m_cursors[dc_south_west_pan] = vgui::dc_sizenesw;
	m_cursors[dc_west_pan] = vgui::dc_sizewe;
	m_cursors[dc_north_west_pan] = vgui::dc_sizenwse;
	m_cursors[dc_alias] = vgui::dc_arrow;
	m_cursors[dc_cell] = vgui::dc_arrow;
	m_cursors[dc_colresize] = vgui::dc_sizewe;
	m_cursors[dc_copycur] = vgui::dc_arrow;
	m_cursors[dc_verticaltext] = vgui::dc_ibeam;
	m_cursors[dc_rowresize] = vgui::dc_sizens;
	m_cursors[dc_zoomin] = vgui::dc_sizens;
	m_cursors[dc_zoomout] = vgui::dc_sizens;
	m_cursors[dc_help] = vgui::dc_arrow;
	m_cursors[dc_custom] = vgui::dc_arrow;
	m_cursors[dc_last] = vgui::dc_last;

	// NOTE: NO STEAM ERROR MANIFESTS HERE.  Fails before devmsg can print.
	if (!steamapicontext->SteamHTMLSurface())
	{
		m_bSupported = false;
		DevMsg("CRITICAL ERROR: Failed to acquire the SteamHTMLSurface! Make sure Steam is running!\n");
		g_pAnarchyManager->ThrowEarlyError("Anarchy Arcade cannot connect to the Steamworks web browser.\nPlease restart Steam and try again.");
	}
	else if (!steamapicontext->SteamHTMLSurface()->Init())
	{
		m_bSupported = false;
		DevMsg("CRITICAL ERROR: Failed to initialize the Steamworks browser!\n");
		g_pAnarchyManager->ThrowEarlyError("Anarchy Arcade cannot connect to the Steamworks web browser.\nPlease restart Steam and try again.");
	}
	else
	{
		m_bSupported = true;
		m_pInputListener = new C_InputListenerSteamBrowser();
	}
}

C_SteamBrowserManager::~C_SteamBrowserManager()
{
	DevMsg("SteamBrowserManager: Destructor\n");
	this->CloseAllInstances();

	ISteamHTMLSurface* pHTMLSurface = steamapicontext->SteamHTMLSurface();
	if (!pHTMLSurface)
		DevMsg("ERROR: There was no SteamHTMLSurface to clean up!\n");
	else if (!pHTMLSurface->Shutdown())
		DevMsg("CRITICAL ERROR: Failed to shutdown the Steamworks browser!\n");

	if (m_pInputListener)
		delete m_pInputListener;
	
	if (m_pBrowserListener)
		delete m_pBrowserListener;
}

void C_SteamBrowserManager::Update()
{
	/*
	for (auto it = m_steamBrowserInstances.begin(); it != m_steamBrowserInstances.end(); ++it)
	{
		C_SteamBrowserInstance* pSteamBrowserInstance = it->second;
		pSteamBrowserInstance->Update();
	}
	*/

	//DevMsg("SteamBrowserManager: Update\n");
	//info->state = state;
	//if (m_pSelectedSteamBrowserInstance)
		//m_pSelectedSteamBrowserInstance->Update();

	//* do nothing now.
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetOriginalEntIndex() < 0)
			m_steamBrowserInstances[i]->Update();
		//if (g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(m_steamBrowserInstances[i]))
	}//*/

	/*
	for (auto it = m_steamBrowserInstances.begin(); it != m_steamBrowserInstances.end(); ++it)
	{
		if (g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(it->second))
			it->second->Update();
	}
	*/


	/*
	for (auto it = m_steamBrowserInstances.begin(); it != m_steamBrowserInstances.end(); ++it)
	{
		C_SteamBrowserInstance* pSteamBrowserInstance = it->second;
		if (pSteamBrowserInstance != m_pSelectedSteamBrowserInstance)
			pSteamBrowserInstance->Update();
	}
	*/
}

void C_SteamBrowserManager::CloseAllInstances()
{
	g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);

	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		C_SteamBrowserInstance* pSteamBrowserInstance = m_steamBrowserInstances[i];
		DevMsg("Removing 1 Steam instance...\n");
		pSteamBrowserInstance->SelfDestruct();
	}

	m_steamBrowserInstances.clear();
}

unsigned long C_SteamBrowserManager::GetSystemCursor(unsigned int eMouseCursor)
{
	return m_cursors[eMouseCursor];
}

void C_SteamBrowserManager::SetSystemCursor(unsigned long cursor, unsigned int ecursor)
{
	std::string cursorName = "default";
	switch (ecursor)
	{
		case dc_user:
			cursorName = "default";
			break;

		case dc_arrow:
			cursorName = "default";
			break;

		case dc_ibeam:
			cursorName = "text";
			break;

		case dc_hourglass:
			cursorName = "wait";
			break;

		case dc_waitarrow:
			cursorName = "progress";
			break;

		case dc_crosshair:
			cursorName = "crosshair";
			break;

		case dc_up:
			cursorName = "n-resize";
			break;

		case dc_sizenw:
			cursorName = "nw-resize";
			break;

		case dc_sizese:
			cursorName = "se-resize";
			break;

		case dc_sizene:
			cursorName = "ne-resize";
			break;

		case dc_sizesw:
			cursorName = "sw-resize";
			break;

		case dc_sizew:
			cursorName = "w-resize";
			break;

		case dc_sizee:
			cursorName = "e-resize";
			break;
		
		case dc_sizen:
			cursorName = "n-resize";
			break;

		case dc_sizes:
			cursorName = "s-resize";
			break;

		case dc_sizewe:
			cursorName = "ew-resize";
			break;

		case dc_sizens:
			cursorName = "ns-resize";
			break;

		case dc_sizeall:
			cursorName = "all-scroll";
			break;

		case dc_no:
			cursorName = "not-allowed";
			break;

		case dc_hand:
			cursorName = "pointer";
			break;

		case dc_blank:
			cursorName = "default";
			break;

		case dc_middle_pan:
			cursorName = "all-scroll";
			break;

		case dc_north_pan:
			cursorName = "n-resize";
			break;

		case dc_north_east_pan:
			cursorName = "ne-resize";
			break;

		case dc_east_pan:
			cursorName = "e-resize";
			break;

		case dc_south_east_pan:
			cursorName = "nwse-resize";
			break;

		case dc_south_pan:
			cursorName = "s-resize";
			break;

		case dc_south_west_pan:
			cursorName = "sw-resize";
			break;

		case dc_west_pan:
			cursorName = "w-resize";
			break;

		case dc_north_west_pan:
			cursorName = "nw-resize";
			break;

		case dc_alias:
			cursorName = "alias";
			break;

		case dc_cell:
			cursorName = "cell";
			break;

		case dc_colresize:
			cursorName = "col-resize";
			break;

		case dc_copycur:
			cursorName = "copy";
			break;

		case dc_verticaltext:
			cursorName = "vertical-text";
			break;

		case dc_rowresize:
			cursorName = "row-resize";
			break;

		case dc_zoomin:
			cursorName = "zoom-in";
			break;

		case dc_zoomout:
			cursorName = "zoom-out";
			break;
		case dc_help:
			cursorName = "help";
			break;

		case dc_custom:
			cursorName = "default";
			break;

		case dc_last:
			cursorName = "default";
			break;

		default:
			cursorName = "default";
			break;

		/*
		case kCursor_WestResize:
			cursorName = "w-resize";
			break;

		case kCursor_NorthEastSouthWestResize:
			cursorName = "nesw-resize";
			break;


		case kCursor_SouthEastPanning:
			cursorName = "se-resize";
			break;

		case kCursor_Move:
			cursorName = "move";
			break;

		case kCursor_ContextMenu:
			cursorName = "context-menu";
			break;

		case kCursor_NoDrop:
			cursorName = "no-drop";
			break;

		case kCursor_None:
			cursorName = "none";
			break;

		case kCursor_Grab:
			cursorName = "grab";
			break;

		case kCursor_Grabbing:
			cursorName = "kCursor_Grabbing";
			break;


		default:
			cursorName = "default";
			break;
			*/
	}

	g_pAnarchyManager->SetSystemCursor(cursor, cursorName);
}

C_SteamBrowserInstance* C_SteamBrowserManager::CreateSteamBrowserInstance()
{
	C_SteamBrowserInstance* pSteamBrowserInstance = new C_SteamBrowserInstance();
	SelectSteamBrowserInstance(pSteamBrowserInstance);
	return pSteamBrowserInstance;
}

bool C_SteamBrowserManager::FocusSteamBrowserInstance(C_SteamBrowserInstance* pSteamBrowserInstance, bool bValue)
{
	if (pSteamBrowserInstance && pSteamBrowserInstance->GetHandle())
	{
		if (bValue)
		{
			steamapicontext->SteamHTMLSurface()->SetKeyFocus(pSteamBrowserInstance->GetHandle(), true);
			m_pFocusedSteamBrowserInstance = pSteamBrowserInstance;
		}
		else
		{
			steamapicontext->SteamHTMLSurface()->SetKeyFocus(pSteamBrowserInstance->GetHandle(), false);
			m_pFocusedSteamBrowserInstance = null;
		}
	}
	return true;
}

bool C_SteamBrowserManager::SelectSteamBrowserInstance(C_SteamBrowserInstance* pSteamBrowserInstance)
{
	m_pSelectedSteamBrowserInstance = pSteamBrowserInstance;
	return true;
}

bool C_SteamBrowserManager::IsAlreadyInInstances(C_SteamBrowserInstance* pSteamBrowserInstance)
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i] == pSteamBrowserInstance)
			return true;
	}

	return false;
}

void C_SteamBrowserManager::AddFreshSteamBrowserInstance(C_SteamBrowserInstance* pSteamBrowserInstance)
{
	if (this->IsAlreadyInInstances(pSteamBrowserInstance))
		DevMsg("Warning: Fresh Steam browser instance already exists in the list!\n");
	else
		m_steamBrowserInstances.push_back(pSteamBrowserInstance);

	//std::string id = pSteamBrowserInstance->GetId();
	//m_steamBrowserInstances[id] = pSteamBrowserInstance;
}

void C_SteamBrowserManager::OnSteamBrowserInstanceCreated(C_SteamBrowserInstance* pSteamBrowserInstance)
{
	//std::string id = pSteamBrowserInstance->GetId();
	//m_steamBrowserInstances[id] = pSteamBrowserInstance;












	// old unknown shit
	//m_steamBrowserInstanceIds[pSteamBrowserInstance->GetHandle()] = pSteamBrowserInstance->GetId();
	//pSteamBrowserInstance->GetInfo()->state = 1;
}


C_SteamBrowserInstance* C_SteamBrowserManager::FindSteamBrowserInstanceByEntityIndex(int iEntityIndex)
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetOriginalEntIndex() == iEntityIndex)
			return m_steamBrowserInstances[i];
	}

	return null;
}

C_SteamBrowserInstance* C_SteamBrowserManager::FindSteamBrowserInstance(unsigned int unHandle)
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetHandle() == unHandle)// && !foundSteamBrowserInstance->second->IsDefunct())
			return m_steamBrowserInstances[i];
	}

	return null;
}

C_SteamBrowserInstance* C_SteamBrowserManager::FindSteamBrowserInstance(std::string id)
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetId() == id)
			return m_steamBrowserInstances[i];
	}

	return null;
}

C_SteamBrowserInstance* C_SteamBrowserManager::GetPendingSteamBrowserInstance()
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetHandle() == 0)
			return m_steamBrowserInstances[i];
	}

	return null;
}

void C_SteamBrowserManager::RunEmbeddedSteamBrowser()
{
	C_SteamBrowserInstance* pSteamBrowserInstance = this->CreateSteamBrowserInstance();

//	pSteamBrowserInstance->Init("", "https://www.netflix.com/watch/217258", null);
	pSteamBrowserInstance->Init("", "http://www.youtube.com/", "Manual Steamworks Browser Tab", null);

//	pSteamBrowserInstance->Init("", "file:///C:/Users/Owner/Desktop/wowvr/index.html", null);
	pSteamBrowserInstance->Focus();
	//g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pSteamBrowserInstance);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pSteamBrowserInstance);

	// http://anarchyarcade.com/press.html
	// https://www.youtube.com/html5
	// http://smarcade.net/dlcv2/view_youtube.php?id=CmRih_VtVAs&autoplay=1
}

void C_SteamBrowserManager::DestroySteamBrowserInstance(C_SteamBrowserInstance* pInstance)
{
	steamapicontext->SteamHTMLSurface()->RemoveBrowser(pInstance->GetHandle());

	/*
	if (pInstance == m_pSelectedSteamBrowserInstance)
	{
		DevMsg("Was the selected Steamworks browser instance.\n");
		this->SelectSteamBrowserInstance(null);
	}

	if (pInstance == m_pFocusedSteamBrowserInstance)
	{
		DevMsg("Was the focused Steamworks browser instance.\n");
		this->FocusSteamBrowserInstance(null);
	}

	//if (g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() == pInstance)
	//	g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);

	//if (g_pAnarchyManager->GetInputManager()->GetInputCanvasTexture() == pInstance->GetTexture())
	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
	{
		DevMsg("Was the input manager embedded instance.\n");
		g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(null);

		if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		{
			DevMsg("Was in input mode.\n");
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		}
		//g_pAnarchyManager->GetInputManager()->SetInputListener(null);
		//g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(null);
	}
	*/

	bool bWorked = false;
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
	{
		if (m_steamBrowserInstances[i]->GetId() == pInstance->GetId())
		{
			m_steamBrowserInstances.erase(m_steamBrowserInstances.begin() + i);
			bWorked = true;
		}
	}

	if (!bWorked)
		DevMsg("WARNING: Failed to remove Steam Browser instance!\n");

	pInstance->SelfDestruct();
}

unsigned int C_SteamBrowserManager::GetInstanceCount()
{
	return m_steamBrowserInstances.size();
}

// TODO: This function is kinda pointless now that m_steamBrowserInstances is a vector itself.  Could just return a pointer directly to it.
void C_SteamBrowserManager::GetAllInstances(std::vector<C_EmbeddedInstance*>& embeddedInstances)
{
	unsigned int max = m_steamBrowserInstances.size();
	for (unsigned int i = 0; i < max; i++)
		embeddedInstances.push_back(m_steamBrowserInstances[i]);
}

/*
C_SteamBrowserInstance* C_SteamBrowserManager::FindDefunctInstance(unsigned int unHandle)
{
	std::map<std::string, C_SteamBrowserInstance*>::iterator foundDefuncSteamBrowserInstance = m_defunctSteamBrowserInstances.begin();
	while (foundDefuncSteamBrowserInstance != m_defunctSteamBrowserInstances.end())
	{
		if (foundDefuncSteamBrowserInstance->second->GetHandle() == unHandle)
			return foundDefuncSteamBrowserInstance->second;
		else
			foundDefuncSteamBrowserInstance++;
	}

	return null;
}

void C_SteamBrowserManager::AddDefunctInstance(C_SteamBrowserInstance* pInstance)
{
	m_defunctSteamBrowserInstances[pInstance->GetId()] = pInstance;
}

bool C_SteamBrowserManager::DestroyDefunctInstance(C_SteamBrowserInstance* pInstance)
{
	DevMsg("Attempting to destroy defunct instance...\n");
	std::string id = pInstance->GetId();

	bool response;
	pInstance->DoDefunctDestruct(response);

	if (response)
	{
		DevMsg("Success!\n");
		std::map<std::string, C_SteamBrowserInstance*>::iterator it = m_defunctSteamBrowserInstances.find(id);
		if (it != m_defunctSteamBrowserInstances.end())
			m_defunctSteamBrowserInstances.erase(it);

		return true;
	}
	else
	{
		DevMsg("Failed!\n");
		return false;
	}
}
*/