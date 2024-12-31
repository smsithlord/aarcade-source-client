#include "cbase.h"

//#include "aa_globals.h"
#include "c_awesomiumbrowsermanager.h"
#include "c_anarchymanager.h"
//#include "Filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_AwesomiumBrowserManager::C_AwesomiumBrowserManager()
{
	DevMsg("AwesomiumBrowserManager: Constructor\n");

	m_pWebCore = null;
	m_pWebSession = null;
	m_pMasterWebView = null;

	m_pMasterLoadListener = null;
	m_pMasterViewListener = null;

	m_pLoadListener = null;
	m_pViewListener = null;
	m_pMenuListener = null;
	m_pDialogListener = null;
	m_pProcessListener = null;

	m_pJSHandler = null;

	m_pInputListener = new C_InputListenerAwesomiumBrowser();
	m_pFocusedAwesomiumBrowserInstance = null;

	m_pNetworkAwesomiumBrowserInstance = null;
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
	// AWESOMIUM to SOURCE VGUI
	m_cursors[kCursor_Pointer] = vgui::dc_arrow;
	m_cursors[kCursor_Cross] = vgui::dc_crosshair;
	m_cursors[kCursor_Hand] = vgui::dc_hand;
	m_cursors[kCursor_IBeam] = vgui::dc_ibeam;
	m_cursors[kCursor_Wait] = vgui::dc_waitarrow;
	m_cursors[kCursor_Help] = vgui::dc_arrow;
	m_cursors[kCursor_EastResize] = vgui::dc_sizewe;
	m_cursors[kCursor_NorthResize] = vgui::dc_sizens;
	m_cursors[kCursor_NorthEastResize] = vgui::dc_sizenesw;
	m_cursors[kCursor_NorthWestResize] = vgui::dc_sizenwse;
	m_cursors[kCursor_SouthResize] = vgui::dc_sizens;
	m_cursors[kCursor_SouthEastResize] = vgui::dc_sizenwse;
	m_cursors[kCursor_SouthWestResize] = vgui::dc_sizenesw;
	m_cursors[kCursor_WestResize] = vgui::dc_sizewe;
	m_cursors[kCursor_NorthSouthResize] = vgui::dc_sizens;
	m_cursors[kCursor_EastWestResize] = vgui::dc_sizewe;
	m_cursors[kCursor_NorthEastSouthWestResize] = vgui::dc_sizenesw;
	m_cursors[kCursor_NorthWestSouthEastResize] = vgui::dc_sizenwse;
	m_cursors[kCursor_ColumnResize] = vgui::dc_sizewe;
	m_cursors[kCursor_RowResize] = vgui::dc_sizens;
	m_cursors[kCursor_MiddlePanning] = vgui::dc_sizeall;
	m_cursors[kCursor_EastPanning] = vgui::dc_sizewe;
	m_cursors[kCursor_NorthPanning] = vgui::dc_sizens;
	m_cursors[kCursor_NorthEastPanning] = vgui::dc_sizenesw;
	m_cursors[kCursor_NorthWestPanning] = vgui::dc_sizenwse;
	m_cursors[kCursor_SouthPanning] = vgui::dc_sizens;
	m_cursors[kCursor_SouthEastPanning] = vgui::dc_sizenwse;
	m_cursors[kCursor_SouthWestPanning] = vgui::dc_sizenesw;
	m_cursors[kCursor_WestPanning] = vgui::dc_sizewe;
	m_cursors[kCursor_Move] = vgui::dc_sizeall;
	m_cursors[kCursor_VerticalText] = vgui::dc_ibeam;
	m_cursors[kCursor_Cell] = vgui::dc_arrow;
	m_cursors[kCursor_ContextMenu] = vgui::dc_arrow;
	m_cursors[kCursor_Alias] = vgui::dc_arrow;
	m_cursors[kCursor_Progress] = vgui::dc_waitarrow;
	m_cursors[kCursor_NoDrop] = vgui::dc_no;
	m_cursors[kCursor_Copy] = vgui::dc_arrow;
	m_cursors[kCursor_None] = vgui::dc_none;
	m_cursors[kCursor_NotAllowed] = vgui::dc_no;
	m_cursors[kCursor_ZoomIn] = vgui::dc_sizens;
	m_cursors[kCursor_ZoomOut] = vgui::dc_sizens;
	m_cursors[kCursor_Grab] = vgui::dc_sizeall;
	m_cursors[kCursor_Grabbing] = vgui::dc_sizeall;
	m_cursors[kCursor_Custom] = vgui::dc_arrow;

	/*
	m_bSoundEnabled = true;
	m_pSelectedSteamBrowserInstance = null;

	steamapicontext->SteamHTMLSurface()->Init();

	m_pInputListener = new C_InputListenerSteamBrowser();

	//g_pAnarchyManager->SetState(AASTATE_WEBMANAGER);
	*/

//	DevMsg("WebBrowser: Init\n");
	//m_iState = 1;	// initializing


	using namespace Awesomium;

	WebConfig config;
	config.log_level = kLogLevel_Normal;
	config.child_process_path = WSLit("./AArcadeWebview.exe");
	//config.additional_options.Push(WSLit("--ignore-certificate-errors"));	// does not fix NSS and SSL issues w/ loading images from servers that only support modern stuff.

	m_pWebCore = WebCore::Initialize(config);

	// Create the master web view
	Awesomium::WebPreferences prefs;
	prefs.enable_plugins = false;
	//prefs.enable_web_gl = true;
	prefs.enable_web_security = false;
	prefs.enable_smooth_scrolling = true;
	prefs.user_stylesheet = WSLit("{}");// body{ background - color: #000000; }");

	std::string cachePath = engine->GetGameDirectory();// VarArgs("%s\\cache", engine->GetGameDirectory());
	cachePath = cachePath.substr(0, cachePath.find_last_of("/\\") + 1);
	cachePath += "aarcade_user\\cache";

	g_pFullFileSystem->CreateDirHierarchy("cache", "DEFAULT_WRITE_PATH");

	m_pWebSession = m_pWebCore->CreateWebSession(WSLit(cachePath.c_str()), prefs);

	NewWindowDataSource* pNewWindowDataSource = new NewWindowDataSource();
	m_pWebSession->AddDataSource(WSLit("newwindow"), pNewWindowDataSource);

	// also add the aarcade_user folder
//	DevMsg("wtf: %s\n", g_pAnarchyManager->GetAArcadeUserFolder().c_str());
	g_pFullFileSystem->AddSearchPath(VarArgs("%s\\resource\\ui\\html", g_pAnarchyManager->GetAArcadeUserFolder().c_str()), "UI");

	g_pFullFileSystem->AddSearchPath(VarArgs("%s\\resource\\ui\\html", engine->GetGameDirectory()), "UI");

	g_pFullFileSystem->AddSearchPath(VarArgs("%s\\screenshots\\panoramic", g_pAnarchyManager->GetAArcadeUserFolder().c_str()), "UI");
	g_pFullFileSystem->AddSearchPath(VarArgs("%s\\screenshots\\overviews", g_pAnarchyManager->GetAArcadeUserFolder().c_str()), "UI");

	UiDataSource* pUiDataSource = new UiDataSource();
	m_pWebSession->AddDataSource(WSLit("ui"), pUiDataSource);
	g_pFullFileSystem->AddSearchPath(VarArgs("%s\\screenshots", engine->GetGameDirectory()), "SHOTS");

	ScreenshotDataSource* pScreenshotDataSource = new ScreenshotDataSource();
	m_pWebSession->AddDataSource(WSLit("shots"), pScreenshotDataSource);

	LocalDataSource* pLocalDataSource = new LocalDataSource();
	m_pWebSession->AddDataSource(WSLit("local"), pLocalDataSource);

	CacheDataSource* pCacheDataSource = new CacheDataSource();
	m_pWebSession->AddDataSource(WSLit("cache"), pCacheDataSource);

	// MASTER
	m_pMasterLoadListener = new MasterLoadListener;
	m_pMasterViewListener = new MasterViewListener;

	// REGULAR
	m_pJSHandler = new JSHandler();

	m_pLoadListener = new LoadListener;
	m_pViewListener = new ViewListener;
	m_pMenuListener = new MenuListener;
	m_pDialogListener = new DialogListener;
	m_pProcessListener = new ProcessListener;

//	m_pMasterWebView = m_pWebCore->CreateWebView(g_pAnarchyManager->GetWebManager()->GetWebSurfaceWidth(), g_pAnarchyManager->GetWebManager()->GetWebSurfaceHeight(), m_pWebSession);
	unsigned int width = AA_MASTER_INSTANCE_WIDTH;
	unsigned int height = AA_MASTER_INSTANCE_HEIGHT;
	m_pMasterWebView = m_pWebCore->CreateWebView(width, height, m_pWebSession);
	m_pMasterWebView->set_load_listener(m_pMasterLoadListener);
	m_pMasterWebView->set_view_listener(m_pMasterViewListener);

	m_pMasterWebView->LoadURL(WebURL(WSLit("asset://newwindow/master")));
	//m_pMasterWebView->LoadURL(WebURL(WSLit("http://www.smsithlord.com/")));
}

C_AwesomiumBrowserManager::~C_AwesomiumBrowserManager()
{
	DevMsg("AwesomiumBrowserManager: Destructor\n");

	this->CloseAllInstances(true);

	m_pMasterWebView->Destroy();
	m_pMasterWebView = null;

	if (m_pInputListener)
		delete m_pInputListener;

	/*
	// iterate over all web tabs and call their destructors
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); ++it)
	{
		C_AwesomiumBrowserInstance* pInstance = it->second;
		if (pInstance == m_pSelectedAwesomiumBrowserInstance)
		{
			this->SelectAwesomiumBrowserInstance(null);
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		}

		//std::string nameTest = "";
		//nameTest += pInstance->GetId();

		//DevMsg("Remove awesomium instance %s\n", nameTest.c_str());
//		if (pInstance->GetTexture() && g_pAnarchyManager->GetInputManager()->GetInputCanvasTexture() == pInstance->GetTexture())
		if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
		{
			g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(null);
			//g_pAnarchyManager->GetInputManager()->SetInputListener(null);
			//g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(null);
		}

//		auto foundAwesomiumBrowserInstance = m_awesomiumBrowserInstances.find(pInstance->GetId());
//		if (foundAwesomiumBrowserInstance != m_awesomiumBrowserInstances.end())
//			m_awesomiumBrowserInstances.erase(foundAwesomiumBrowserInstance);

		pInstance->SelfDestruct();
	}

	m_awesomiumBrowserInstances.clear();

	m_pMasterWebView->Destroy();
	m_pMasterWebView = null;
	*/

	//if ( m_pInputListener )
//		delete m_pInputListener;

	WebCore::Shutdown();
}


unsigned long C_AwesomiumBrowserManager::GetSystemCursor(unsigned int eMouseCursor)
{
	return m_cursors[eMouseCursor];
}

void C_AwesomiumBrowserManager::SetSystemCursor(unsigned long cursor, Awesomium::Cursor acursor)
{
	std::string cursorName = "default";
	switch (acursor)
	{
		case kCursor_Pointer:
			cursorName = "default";
			break;

		case kCursor_Cross:
			cursorName = "crosshair";
			break;

		case kCursor_Hand:
			cursorName = "pointer";
			break;

		case kCursor_IBeam:
			cursorName = "text";
			break;

		case kCursor_Wait:
			cursorName = "wait";
			break;

		case kCursor_Help:
			cursorName = "help";
			break;

		case kCursor_EastResize:
			cursorName = "e-resize";
			break;

		case kCursor_NorthResize:
			cursorName = "n-resize";
			break;

		case kCursor_NorthEastResize:
			cursorName = "ne-resize";
			break;

		case kCursor_NorthWestResize:
			cursorName = "nw-resize";
			break;

		case kCursor_SouthResize:
			cursorName = "s-resize";
			break;

		case kCursor_SouthEastResize:
			cursorName = "se-resize";
			break;

		case kCursor_SouthWestResize:
			cursorName = "sw-resize";
			break;

		case kCursor_WestResize:
			cursorName = "w-resize";
			break;

		case kCursor_NorthSouthResize:
			cursorName = "ns-resize";
			break;

		case kCursor_EastWestResize:
			cursorName = "ew-resize";
			break;

		case kCursor_NorthEastSouthWestResize:
			cursorName = "nesw-resize";
			break;

		case kCursor_NorthWestSouthEastResize:
			cursorName = "nwse-resize";
			break;

		case kCursor_ColumnResize:
			cursorName = "col-resize";
			break;

		case kCursor_RowResize:
			cursorName = "row-resize";
			break;

		case kCursor_MiddlePanning:
			cursorName = "all-scroll";
			break;

		case kCursor_EastPanning:
			cursorName = "e-resize";
			break;

		case kCursor_NorthPanning:
			cursorName = "n-resize";
			break;

		case kCursor_NorthEastPanning:
			cursorName = "ne-resize";
			break;

		case kCursor_NorthWestPanning:
			cursorName = "nw-resize";
			break;

		case kCursor_SouthPanning:
			cursorName = "s-resize";
			break;

		case kCursor_SouthEastPanning:
			cursorName = "se-resize";
			break;

		case kCursor_SouthWestPanning:
			cursorName = "sw-resize";
			break;

		case kCursor_WestPanning:
			cursorName = "w-resize";
			break;

		case kCursor_Move:
			cursorName = "move";
			break;

		case kCursor_VerticalText:
			cursorName = "vertical-text";
			break;

		case kCursor_Cell:
			cursorName = "cell";
			break;

		case kCursor_ContextMenu:
			cursorName = "context-menu";
			break;

		case kCursor_Alias:
			cursorName = "alias";
			break;

		case kCursor_Progress:
			cursorName = "progress";
			break;

		case kCursor_NoDrop:
			cursorName = "no-drop";
			break;

		case kCursor_Copy:
			cursorName = "copy";
			break;

		case kCursor_None:
			cursorName = "none";
			break;

		case kCursor_NotAllowed:
			cursorName = "not-allowed";
			break;

		case kCursor_ZoomIn:
			cursorName = "zoom-in";
			break;

		case kCursor_ZoomOut:
			cursorName = "zoom-out";
			break;

		case kCursor_Grab:
			cursorName = "grab";
			break;

		case kCursor_Grabbing:
			cursorName = "kCursor_Grabbing";
			break;

		case kCursor_Custom:
			cursorName = "default";
			break;

		default:
			cursorName = "default";
			break;
	}

	g_pAnarchyManager->SetSystemCursor(cursor, cursorName);
}

void C_AwesomiumBrowserManager::RunEmbeddedAwesomiumBrowser()
{
	DevMsg("Run embedded awesomium test!\n");

//	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = this->CreateAwesomiumBrowserInstance("", "http://smarcade.net/dlcv2/view_youtube.php?id=CmRih_VtVAs&autoplay=1", false);
	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = this->CreateAwesomiumBrowserInstance("", "http://www.youtube.com/", "", false);
	pAwesomiumBrowserInstance->Select();
	//pAwesomiumBrowserInstance->Focus();
	// tell the input manager that the steam browser instance is active
	g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pAwesomiumBrowserInstance);	// including an embedded instance in the activate input mode call overrides this
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pAwesomiumBrowserInstance);

//	C_InputListenerAwesomiumBrowser* pListener = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetInputListener();
//	g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(pAwesomiumBrowserInstance->GetTexture());
//	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, (C_InputListener*)pListener);

	//pAwesomiumBrowserInstance->Init();
//	g_pAnarchyManager->IncrementState();
}

void C_AwesomiumBrowserManager::DestroyAwesomiumBrowserInstance(C_AwesomiumBrowserInstance* pInstance)
{
	if (pInstance == m_pSelectedAwesomiumBrowserInstance)
	{
		this->SelectAwesomiumBrowserInstance(null);
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
	}

	///*
	if (g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance() == pInstance)
		g_pAnarchyManager->GetCanvasManager()->SetDifferentDisplayInstance(pInstance);
	//*/


	//g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);

//	if (g_pAnarchyManager->GetInputManager()->GetInputCanvasTexture() == pInstance->GetTexture())
	if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
	{
		g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(null);
		//g_pAnarchyManager->GetInputManager()->SetInputListener(null);
		//g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(null);
	}

	auto foundAwesomiumBrowserInstance = m_awesomiumBrowserInstances.find(pInstance->GetId());
	if (foundAwesomiumBrowserInstance != m_awesomiumBrowserInstances.end())
		m_awesomiumBrowserInstances.erase(foundAwesomiumBrowserInstance);

	pInstance->SelfDestruct();
}

void C_AwesomiumBrowserManager::OnMasterWebViewDocumentReady()
{
	g_pAnarchyManager->IncrementState();
	//m_iState = 2;	// initialized
	//	g_pAnarchyManager->GetInputManager()->SetInputListener(g_pAnarchyManager->GetWebManager(), LISTENER_WEB_MANAGER);
	//g_pAnarchyManager->GetWebManager()->OnBrowserInitialized();
}

C_AwesomiumBrowserInstance* C_AwesomiumBrowserManager::CreateAwesomiumBrowserInstance(std::string id, std::string initialURL, std::string title, bool alpha, const char* pchPostData, int entindex)
{
	std::string goodId = (id != "") ? id : g_pAnarchyManager->GenerateUniqueId();
	std::string goodTitle = (title != "") ? title : "Untitled Awesomium Web Tab";

	DevMsg("Create Awesomium Web Browser: %s / %s\n", goodId.c_str(), goodTitle.c_str());

	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = new C_AwesomiumBrowserInstance();
	pAwesomiumBrowserInstance->Init(goodId, initialURL, goodTitle, alpha, pchPostData, entindex);

	m_awesomiumBrowserInstances[goodId] = pAwesomiumBrowserInstance;

	DevMsg("AwesomiumBrowserManager: CreateWebView\n");
	m_pMasterWebView->ExecuteJavascript(WSLit(VarArgs("window.open('asset://newwindow/%s', '', 'width=200,height=100');", goodId.c_str())), WSLit(""));

	//if (goodId != "hud")	// don't select the hud until its needed
	//	SelectAwesomiumBrowserInstance(pAwesomiumBrowserInstance);

	return pAwesomiumBrowserInstance;
}

C_AwesomiumBrowserInstance* C_AwesomiumBrowserManager::FindAwesomiumBrowserInstanceByEntityIndex(int iEntityIndex)
{
	// iterate over all web tabs and call their destructors
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); it++)
	{
		if (it->second->GetOriginalEntIndex() == iEntityIndex && it->second->GetId() != "hud" )
			return it->second;
	}

	return null;
}

C_AwesomiumBrowserInstance* C_AwesomiumBrowserManager::FindAwesomiumBrowserInstance(std::string id)
{
	//DevMsg("ID: %s\n", id.c_str());
	auto foundBrowserInstance = m_awesomiumBrowserInstances.find(id);
	if (foundBrowserInstance != m_awesomiumBrowserInstances.end())
	{
		return foundBrowserInstance->second;
	}
	else
		return null;
}

C_AwesomiumBrowserInstance* C_AwesomiumBrowserManager::FindAwesomiumBrowserInstance(Awesomium::WebView* pWebView)
{
	// iterate over all web tabs and call their destructors
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); ++it)
	{
		if (it->second->GetWebView() == pWebView)
			return it->second;
	}

	return null;
}

void C_AwesomiumBrowserManager::PrepareWebView(Awesomium::WebView* pWebView, std::string id)
{
	DevMsg("Adding Awesomium web view listeners...\n");
	unsigned int width = (id == "hud") ? AA_HUD_INSTANCE_WIDTH : AA_EMBEDDED_INSTANCE_WIDTH;
	unsigned int height = (id == "hud") ? AA_HUD_INSTANCE_HEIGHT : AA_EMBEDDED_INSTANCE_HEIGHT;

	unsigned int imageWidth = AA_THUMBNAIL_SIZE;
	unsigned int imageHeight = AA_THUMBNAIL_SIZE;

	unsigned int networkWidth = AA_NETWORK_INSTANCE_WIDTH;
	unsigned int networkHeight = AA_NETWORK_INSTANCE_HEIGHT;

	if (id == "images")
		pWebView->Resize(imageWidth, imageHeight);
	else if (id == "network")
		pWebView->Resize(networkWidth, networkHeight);
	else
		pWebView->Resize(width, height);

	pWebView->set_load_listener(m_pLoadListener);
	pWebView->set_view_listener(m_pViewListener);
	pWebView->set_menu_listener(m_pMenuListener);
	pWebView->set_dialog_listener(m_pDialogListener);
	pWebView->set_process_listener(m_pProcessListener);

	if (true || id == "hud" || id == "images" || id == "network" )
	{
		pWebView->set_js_method_handler(m_pJSHandler);
		this->CreateAaApi(pWebView);
	}




	//C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->FindWebTab(id);
	C_AwesomiumBrowserInstance* pBrowserInstance = this->FindAwesomiumBrowserInstance(id);
	if (pBrowserInstance)
	{
		pBrowserInstance->SetWebView(pWebView);
		pBrowserInstance->SetState(2);
		//m_webViews[pBrowserInstance] = pWebView;	// obsolete perhaps??

		ITexture* pTexture = pBrowserInstance->GetTexture();
		if (pTexture && pTexture->GetImageFormat() == IMAGE_FORMAT_BGRA8888)
			pWebView->SetTransparent(true);

		std::string initialURI = pBrowserInstance->GetInitialURL();
		std::string uri = initialURI;
		//if (id == "network")
		//		uri = initialURI;
		//else if (id == "images")
		//	uri = initialURI;	// this should never happen, so comment it out to avoid confusion
		//uri = "asset://ui/imageLoader.html";
		//else if (id == "hud")
		//	uri = initialURI;
		//uri = (initialURI == "") ? "asset://ui/default.html" : initialURI;	// this should never happen, so comment it out to avoid confusion
		//else
		//		uri = initialURI;

		DevMsg("Loading initial URL: %s\n", uri.c_str());
		pWebView->LoadURL(WebURL(WSLit(uri.c_str())));
		/*
		if (id == "hud" )	// is this too early??
		g_pAnarchyManager->IncrementState();
		else if (id == "images" && AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT)
		g_pAnarchyManager->IncrementState();
		*/
	}
}


void C_AwesomiumBrowserManager::ClearCache()
{
	m_pWebSession->ClearCache();
	m_pWebSession->ClearCookies();
}

void C_AwesomiumBrowserManager::CreateAaApi(WebView* pWebView)
{
	DevMsg("Adding AAAPI...\n");

	JSValue result = pWebView->CreateGlobalJavascriptObject(WSLit("aaapi"));
	if (!result.IsObject())
	{
		DevMsg("Failed to create AAAPI.\n");
		g_pAnarchyManager->ThrowEarlyError("Anarchy Arcade cannot detect its UI process.\nPlease adjust your anti-virus software to allow\nAArcade to communicate with its UI and try again.");
		return;
	}

	JSObject& aaapiObject = result.ToObject();
	aaapiObject.SetCustomMethod(WSLit("cmd"), false);
	aaapiObject.SetCustomMethod(WSLit("cmdEx"), true);

	/*
	systemObject.SetCustomMethod(WSLit("quit"), false);
	systemObject.SetCustomMethod(WSLit("launchItem"), false);
	systemObject.SetCustomMethod(WSLit("spawnItem"), false);	// OBSOLETE!!
	systemObject.SetCustomMethod(WSLit("spawnEntry"), false);
	systemObject.SetCustomMethod(WSLit("autoSpawnEntry"), false);
	systemObject.SetCustomMethod(WSLit("showBulkImportList"), false);
	systemObject.SetCustomMethod(WSLit("outputTestDOM"), false);
	systemObject.SetCustomMethod(WSLit("setLibraryBrowserContext"), false);
	systemObject.SetCustomMethod(WSLit("loadFirstLocalApp"), false);
	systemObject.SetCustomMethod(WSLit("loadNextLocalApp"), false);
	systemObject.SetCustomMethod(WSLit("loadLocalAppClose"), false);
	systemObject.SetCustomMethod(WSLit("detectAllMaps"), false);
	systemObject.SetCustomMethod(WSLit("detectAllModels"), false);
	systemObject.SetCustomMethod(WSLit("loadMap"), false);
	systemObject.SetCustomMethod(WSLit("loadMapNow"), false);
	systemObject.SetCustomMethod(WSLit("deactivateInputMode"), false);
	systemObject.SetCustomMethod(WSLit("reactivateInputMode"), false);
	systemObject.SetCustomMethod(WSLit("attemptSelectEntity"), false);
	systemObject.SetCustomMethod(WSLit("bringToFront"), false);
	systemObject.SetCustomMethod(WSLit("showSteamGrid"), false);
	systemObject.SetCustomMethod(WSLit("forceInputMode"), false);
	systemObject.SetCustomMethod(WSLit("hudMouseDown"), false);
	systemObject.SetCustomMethod(WSLit("hudMouseUp"), false);
	systemObject.SetCustomMethod(WSLit("requestActivateInputMode"), false);
	systemObject.SetCustomMethod(WSLit("simpleImageReady"), false);
	systemObject.SetCustomMethod(WSLit("saveLibretroKeybind"), false);
	systemObject.SetCustomMethod(WSLit("removeAppFilepath"), false);
	systemObject.SetCustomMethod(WSLit("saveLibretroOption"), false);
	systemObject.SetCustomMethod(WSLit("spawnNearestObject"), false);
	systemObject.SetCustomMethod(WSLit("fileBrowse"), false);
	systemObject.SetCustomMethod(WSLit("metaSearch"), false);
	systemObject.SetCustomMethod(WSLit("getDOM"), false);
	systemObject.SetCustomMethod(WSLit("clearAwesomiumCache"), false);
	systemObject.SetCustomMethod(WSLit("disconnect"), false);
	systemObject.SetCustomMethod(WSLit("viewStream"), false);
	systemObject.SetCustomMethod(WSLit("autoInspect"), false);
	systemObject.SetCustomMethod(WSLit("viewPreview"), false);
	systemObject.SetCustomMethod(WSLit("runLibretro"), false);
	systemObject.SetCustomMethod(WSLit("popout"), false);
	systemObject.SetCustomMethod(WSLit("cabinetSelected"), false);
	systemObject.SetCustomMethod(WSLit("modelSelected"), false);
	systemObject.SetCustomMethod(WSLit("objectHover"), false);
	systemObject.SetCustomMethod(WSLit("objectSelected"), false);
	systemObject.SetCustomMethod(WSLit("moveObject"), false);
	systemObject.SetCustomMethod(WSLit("deleteObject"), false);
	systemObject.SetCustomMethod(WSLit("beginImportSteamGames"), false);	// this loads the profile page
	systemObject.SetCustomMethod(WSLit("startImportSteamGames"), false);	// this actually starts adding stuff to the library
	systemObject.SetCustomMethod(WSLit("showEngineOptionsMenu"), false);
	systemObject.SetCustomMethod(WSLit("setSlaveScreen"), false);
	systemObject.SetCustomMethod(WSLit("navigateToURI"), false);
	systemObject.SetCustomMethod(WSLit("viewObjectInfo"), false);
	systemObject.SetCustomMethod(WSLit("resumeMainMenu"), false);
	systemObject.SetCustomMethod(WSLit("minimizeAArcade"), false);
	systemObject.SetCustomMethod(WSLit("setInputCapture"), false);
	systemObject.SetCustomMethod(WSLit("tempSelectEntity"), false);
	systemObject.SetCustomMethod(WSLit("adjustObjectOffset"), false);
	systemObject.SetCustomMethod(WSLit("adjustObjectRot"), false);
	systemObject.SetCustomMethod(WSLit("adjustObjectScale"), false);
	systemObject.SetCustomMethod(WSLit("goBack"), false);
	systemObject.SetCustomMethod(WSLit("doCopy"), false);
	systemObject.SetCustomMethod(WSLit("goForward"), false);
	systemObject.SetCustomMethod(WSLit("reload"), false);
	systemObject.SetCustomMethod(WSLit("goHome"), false);
	systemObject.SetCustomMethod(WSLit("playSound"), false);
	systemObject.SetCustomMethod(WSLit("libretroPause"), false);
	systemObject.SetCustomMethod(WSLit("libretroReset"), false);
	systemObject.SetCustomMethod(WSLit("libretroSetOverlay"), false);
	systemObject.SetCustomMethod(WSLit("acquire"), false);
	systemObject.SetCustomMethod(WSLit("libretroClearOverlay"), false);
	systemObject.SetCustomMethod(WSLit("libretroSaveOverlay"), false);
	systemObject.SetCustomMethod(WSLit("setStartWithWindows"), false);
	systemObject.SetCustomMethod(WSLit("setLibretroGUIGamepadEnabled"), false);
	systemObject.SetCustomMethod(WSLit("setLibretroGUIGamepadButtonState"), false);
	systemObject.SetCustomMethod(WSLit("clearLibretroGUIGamepadButtonStates"), false);
	systemObject.SetCustomMethod(WSLit("taskClear"), false);
	systemObject.SetCustomMethod(WSLit("closeTask"), false);
	systemObject.SetCustomMethod(WSLit("hideTask"), false);
	systemObject.SetCustomMethod(WSLit("unhideTask"), false);
	systemObject.SetCustomMethod(WSLit("switchToTask"), false);
	systemObject.SetCustomMethod(WSLit("setTabMenuFile"), false);
	systemObject.SetCustomMethod(WSLit("displayTask"), false);
	systemObject.SetCustomMethod(WSLit("takeScreenshot"), false);
	systemObject.SetCustomMethod(WSLit("deleteScreenshot"), false);
	systemObject.SetCustomMethod(WSLit("teleportScreenshot"), false);
	systemObject.SetCustomMethod(WSLit("saveNewNode"), false);
	systemObject.SetCustomMethod(WSLit("setSaveMode"), false);
	systemObject.SetCustomMethod(WSLit("performAutoSave"), false);
	systemObject.SetCustomMethod(WSLit("addToastMessage"), false);
	systemObject.SetCustomMethod(WSLit("clearNodeSpace"), false);
	systemObject.SetCustomMethod(WSLit("feedback"), false);
	systemObject.SetCustomMethod(WSLit("doPause"), false);
	systemObject.SetCustomMethod(WSLit("consoleCommand"), false);
	systemObject.SetCustomMethod(WSLit("specialReady"), false);
	systemObject.SetCustomMethod(WSLit("refreshItemTextures"), false);
	systemObject.SetCustomMethod(WSLit("selectTaskObject"), false);
	systemObject.SetCustomMethod(WSLit("get2DBBox"), false);
	systemObject.SetCustomMethod(WSLit("getValid2DBBoxes"), false);
	systemObject.SetCustomMethod(WSLit("setNextLobby"), false);
	systemObject.SetCustomMethod(WSLit("createHammerProject"), false);
	systemObject.SetCustomMethod(WSLit("openHammerProject"), false);
	systemObject.SetCustomMethod(WSLit("showRawMainMenu"), false);
	systemObject.SetCustomMethod(WSLit("setSpawningObjectModelSequence"), false);
	systemObject.SetCustomMethod(WSLit("setSocialMode"), false);
	systemObject.SetCustomMethod(WSLit("assignObjectItem"), false);
	systemObject.SetCustomMethod(WSLit("interactiveScreenshotReady"), false);
	systemObject.SetCustomMethod(WSLit("openScreenshot"), false);
	systemObject.SetCustomMethod(WSLit("storeStats"), false);
	systemObject.SetCustomMethod(WSLit("statAction"), false);
	systemObject.SetCustomMethod(WSLit("resetStats"), false);
	systemObject.SetCustomMethod(WSLit("selectNextOrPrev"), false);
	//systemObject.SetCustomMethod(WSLit("doMapTransition"), false);
	//systemObject.SetCustomMethod(WSLit("reloadMap"), false);
	networkObject.SetCustomMethod(WSLit("hostSession"), false);
	networkObject.SetCustomMethod(WSLit("objectUpdated"), false);
	networkObject.SetCustomMethod(WSLit("objectRemoved"), false);
	networkObject.SetCustomMethod(WSLit("restartNetwork"), false);
	networkObject.SetCustomMethod(WSLit("disconnected"), false);
	networkObject.SetCustomMethod(WSLit("networkEvent"), false);
	networkObject.SetCustomMethod(WSLit("followPlayer"), false);
	networkObject.SetCustomMethod(WSLit("joinLobbyWeb"), false);
	networkObject.SetCustomMethod(WSLit("joinLobby"), false);
	networkObject.SetCustomMethod(WSLit("socialChatMsg"), false);
	networkObject.SetCustomMethod(WSLit("fetchOnlineCount"), false);
	networkObject.SetCustomMethod(WSLit("reportSocialCount"), false);
	networkObject.SetCustomMethod(WSLit("banPlayer"), false);
	networkObject.SetCustomMethod(WSLit("syncPano"), false);
	networkObject.SetCustomMethod(WSLit("unbanPlayer"), false);
	networkObject.SetCustomMethod(WSLit("sendEntryUpdate"), false);
	networkObject.SetCustomMethod(WSLit("sendLocalChatMsg"), false);
	networkObject.SetCustomMethod(WSLit("sendSocialChatMsg"), false);
	networkObject.SetCustomMethod(WSLit("entryCreated"), false);
	//networkObject.SetCustomMethod(WSLit("entryChanged"), false);
	networkObject.SetCustomMethod(WSLit("connectSession"), false);
	networkObject.SetCustomMethod(WSLit("disconnectSession"), false);
	networkObject.SetCustomMethod(WSLit("sessionEnded"), false);
	networkObject.SetCustomMethod(WSLit("networkReady"), false);
	networkObject.SetCustomMethod(WSLit("setTwitchConfig"), false);
	networkObject.SetCustomMethod(WSLit("twitchAuthenticate"), false);
	networkObject.SetCustomMethod(WSLit("sendTwitchChat"), false);
	networkObject.SetCustomMethod(WSLit("openTwitchConnection"), false);
	networkObject.SetCustomMethod(WSLit("closeTwitchConnection"), false);
	networkObject.SetCustomMethod(WSLit("joinTwitchChannel"), false);
	networkObject.SetCustomMethod(WSLit("leaveTwitchChannel"), false);
	networkObject.SetCustomMethod(WSLit("setTwitchBotEnabled"), false);
	networkObject.SetCustomMethod(WSLit("setTwitchLiveStatus"), false);
	networkObject.SetCustomMethod(WSLit("saveRemoteItemChanges"), false);
	networkObject.SetCustomMethod(WSLit("saveRemoteModelChanges"), false);
	networkObject.SetCustomMethod(WSLit("getAssetUploadBatch"), false);
	networkObject.SetCustomMethod(WSLit("addSingleObjectToUploadBatch"), false);
	networkObject.SetCustomMethod(WSLit("cloudAssetAvailable"), false);
	networkObject.SetCustomMethod(WSLit("cloudAssetUnavailable"), false);
	networkObject.SetCustomMethod(WSLit("onAssetRequestAdded"), false);
	//callbacksObject.SetCustomMethod(WSLit("loadNextLocalAppCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("startupCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("defaultLibraryReadyCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("mountNextWorkshopCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("loadNextLocalItemLegacyCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("detectNextMapCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("spawnNextObjectCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("addNextDefaultLibraryCallback"), false);
	//callbacksObject.SetCustomMethod(WSLit("defaultLibraryReadyCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("updateLibraryVersionCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("readyToLoadUserLibraryCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("rebuildSoundCacheCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("processAllModelsCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("processNextModelCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("addNextModelCallback"), false);
	callbacksObject.SetCustomMethod(WSLit("importNextSteamGameCallback"), false);
	libraryObject.SetCustomMethod(WSLit("deleteApp"), false);
	libraryObject.SetCustomMethod(WSLit("fetchGameAchievements"), false);*/


	/*
	systemObject.SetCustomMethod(WSLit("getPlayerEyeOrigin"), true);
	systemObject.SetCustomMethod(WSLit("checkStartWithWindows"), true);
	systemObject.SetCustomMethod(WSLit("libretroGetAllDLLs"), true);
	systemObject.SetCustomMethod(WSLit("getLibretroActiveOverlay"), true);
	systemObject.SetCustomMethod(WSLit("getLibretroOverlays"), true);
	systemObject.SetCustomMethod(WSLit("getLibretroGUIGamepadEnabled"), true);
	systemObject.SetCustomMethod(WSLit("libretroUpdateDLL"), true);
	systemObject.SetCustomMethod(WSLit("getEntityInfo"), true);
	systemObject.SetCustomMethod(WSLit("getObjectInfo"), true);
	systemObject.SetCustomMethod(WSLit("getObject"), true);
	systemObject.SetCustomMethod(WSLit("getObjectUnderCursor"), true);	// obsolete?
	systemObject.SetCustomMethod(WSLit("getObjectPosScreenSpace"), true);	// obsolete?
	systemObject.SetCustomMethod(WSLit("getPosScreenSpace"), true);
	systemObject.SetCustomMethod(WSLit("getObjectsInSight"), true);
	systemObject.SetCustomMethod(WSLit("getAllObjectInfos"), true);
	systemObject.SetCustomMethod(WSLit("getTransformInfo"), true);
	systemObject.SetCustomMethod(WSLit("getWorldInfo"), true);
	systemObject.SetCustomMethod(WSLit("getDbSize"), true);
	systemObject.SetCustomMethod(WSLit("createDbBackup"), true);
	systemObject.SetCustomMethod(WSLit("getTaskInfo"), true);
	systemObject.SetCustomMethod(WSLit("getDisplayTaskInfo"), true);
	systemObject.SetCustomMethod(WSLit("isInGame"), true);
	systemObject.SetCustomMethod(WSLit("setNearestObjectDist"), true);	// obsolete?
	systemObject.SetCustomMethod(WSLit("getLibretroOptions"), true);
	systemObject.SetCustomMethod(WSLit("getMapInstances"), true);
	systemObject.SetCustomMethod(WSLit("getInstance"), true);
	systemObject.SetCustomMethod(WSLit("getDefaultLibretroInputDevices"), true);
	systemObject.SetCustomMethod(WSLit("generateUniqueId"), true);
	systemObject.SetCustomMethod(WSLit("generateHashId"), true);
	systemObject.SetCustomMethod(WSLit("getLibretroKeybinds"), true);
	systemObject.SetCustomMethod(WSLit("importSteamGames"), true);
	systemObject.SetCustomMethod(WSLit("getSelectedWebTab"), true);
	systemObject.SetCustomMethod(WSLit("detectAllMapScreenshots"), true);
	systemObject.SetCustomMethod(WSLit("getAllMapScreenshots"), true);
	systemObject.SetCustomMethod(WSLit("getScreenshot"), true);
	systemObject.SetCustomMethod(WSLit("getAllMaps"), true);
	systemObject.SetCustomMethod(WSLit("getMap"), true);
	systemObject.SetCustomMethod(WSLit("findMap"), true);
	systemObject.SetCustomMethod(WSLit("didCancelPopupMenu"), true);
	systemObject.SetCustomMethod(WSLit("getLibraryBrowserContext"), true);
	systemObject.SetCustomMethod(WSLit("didSelectPopupMenuItem"), true);
	systemObject.SetCustomMethod(WSLit("getSocialMode"), true);
	systemObject.SetCustomMethod(WSLit("getSpawningObjectModelSequenceNames"), true);
	systemObject.SetCustomMethod(WSLit("getSpawningObjectModelSequenceName"), true);
	systemObject.SetCustomMethod(WSLit("alphabetSafe"), true);
	systemObject.SetCustomMethod(WSLit("getAllStickers"), true);
	systemObject.SetCustomMethod(WSLit("detectAllStickers"), true);
	systemObject.SetCustomMethod(WSLit("getConVarValue"), true);
	systemObject.SetCustomMethod(WSLit("getAllMounts"), true);
	systemObject.SetCustomMethod(WSLit("getMount"), true);
	systemObject.SetCustomMethod(WSLit("getNeedsSave"), true);
	systemObject.SetCustomMethod(WSLit("getAllTasks"), true);
	systemObject.SetCustomMethod(WSLit("getAllWorkshopSubscriptions"), true);
	systemObject.SetCustomMethod(WSLit("getWorkshopSubscription"), true);
	systemObject.SetCustomMethod(WSLit("getRelativeAssetPath"), true);
	systemObject.SetCustomMethod(WSLit("getAllBackpacks"), true);
	systemObject.SetCustomMethod(WSLit("getBackpack"), true);
	systemObject.SetCustomMethod(WSLit("getNearestObjectToPlayerLook"), true);
	systemObject.SetCustomMethod(WSLit("getNextNearestObjectToPlayerLook"), true);
	systemObject.SetCustomMethod(WSLit("getNearestObjectToObject"), true);
	systemObject.SetCustomMethod(WSLit("getNodeSetupInfo"), true);
	systemObject.SetCustomMethod(WSLit("getObjectBBox"), true);
	systemObject.SetCustomMethod(WSLit("getCurrentLobby"), true);
	systemObject.SetCustomMethod(WSLit("getAllHammerProjects"), true);
	systemObject.SetCustomMethod(WSLit("getHammerProjectMapID"), true);
	systemObject.SetCustomMethod(WSLit("isPaused"), true);
	systemObject.SetCustomMethod(WSLit("getInstanceObjectCount"), true);
	systemObject.SetCustomMethod(WSLit("getObjectWithFile"), true);
	systemObject.SetCustomMethod(WSLit("getStats"), true);
	systemObject.SetCustomMethod(WSLit("getAchievements"), true);
	systemObject.SetCustomMethod(WSLit("getAutoCameraTargetTask"), true);
	//systemObject.SetCustomMethod(WSLit("precacheModel"), true);
	systemObject.SetCustomMethod(WSLit("generateModelThumbnail"), true);
	networkObject.SetCustomMethod(WSLit("getConnectedSession"), true);
	networkObject.SetCustomMethod(WSLit("getAllUserChat"), true);
	networkObject.SetCustomMethod(WSLit("getAllUsers"), true);
	networkObject.SetCustomMethod(WSLit("getUser"), true);
	networkObject.SetCustomMethod(WSLit("getNumUsers"), true);
	networkObject.SetCustomMethod(WSLit("getSyncOverview"), true);
	networkObject.SetCustomMethod(WSLit("isTwitchBotEnabled"), true);
	networkObject.SetCustomMethod(WSLit("isItemRemoteOnly"), true);
	networkObject.SetCustomMethod(WSLit("isModelRemoteOnly"), true);
	networkObject.SetCustomMethod(WSLit("isTypeRemoteOnly"), true);
	networkObject.SetCustomMethod(WSLit("isAppRemoteOnly"), true);
	networkObject.SetCustomMethod(WSLit("isItemMutated"), true);
	networkObject.SetCustomMethod(WSLit("isModelMutated"), true);
	networkObject.SetCustomMethod(WSLit("isTypeMutated"), true);
	networkObject.SetCustomMethod(WSLit("isAppMutated"), true);
	networkObject.SetCustomMethod(WSLit("getTwitchConfig"), true);
	libraryObject.SetCustomMethod(WSLit("hasLibraryEntry"), true);
	libraryObject.SetCustomMethod(WSLit("getFirstLibraryEntry"), true);
	libraryObject.SetCustomMethod(WSLit("getNextLibraryEntry"), true);
	libraryObject.SetCustomMethod(WSLit("findFirstLibraryEntry"), true);
	libraryObject.SetCustomMethod(WSLit("findNextLibraryEntry"), true);
	libraryObject.SetCustomMethod(WSLit("getAllLibraryTypes"), true);
	libraryObject.SetCustomMethod(WSLit("getLibraryType"), true);
	libraryObject.SetCustomMethod(WSLit("getAllLibraryApps"), true);
	libraryObject.SetCustomMethod(WSLit("getLibraryApp"), true);
	libraryObject.SetCustomMethod(WSLit("getFirstLibraryItem"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("getNextLibraryItem"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("getLibraryItem"), true);
	libraryObject.SetCustomMethod(WSLit("getLibraryModel"), true);
	libraryObject.SetCustomMethod(WSLit("getSelectedLibraryItem"), true);
	libraryObject.SetCustomMethod(WSLit("findFirstLibraryItem"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("findNextLibraryItem"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("findLibraryItem"), true);
	libraryObject.SetCustomMethod(WSLit("findLibraryModel"), true);
	libraryObject.SetCustomMethod(WSLit("findLibraryApp"), true);
	libraryObject.SetCustomMethod(WSLit("findLibraryType"), true);
	libraryObject.SetCustomMethod(WSLit("updateItem"), true);
	libraryObject.SetCustomMethod(WSLit("updateApp"), true);
	libraryObject.SetCustomMethod(WSLit("updateInstance"), true);
	libraryObject.SetCustomMethod(WSLit("deleteInstance"), true);
	libraryObject.SetCustomMethod(WSLit("updateModel"), true);
	libraryObject.SetCustomMethod(WSLit("updateType"), true);
	libraryObject.SetCustomMethod(WSLit("createItem"), true);
	libraryObject.SetCustomMethod(WSLit("createApp"), true);
	libraryObject.SetCustomMethod(WSLit("createType"), true);
	libraryObject.SetCustomMethod(WSLit("createModel"), true);
	libraryObject.SetCustomMethod(WSLit("saveItem"), true);
	libraryObject.SetCustomMethod(WSLit("getVolatileCounts"), true);
	libraryObject.SetCustomMethod(WSLit("getFirstLibraryModel"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("getNextLibraryModel"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("findFirstLibraryModel"), true);	// OBSOLETE!
	libraryObject.SetCustomMethod(WSLit("findNextLibraryModel"), true);	// OBSOLETE!
	systemObject.SetCustomMethod(WSLit("getOwnObjectId"), true);	// only useful for Awesomium instances!!
	libraryObject.SetCustomMethod(WSLit("getFullAssetPath"), false);
	libraryObject.SetCustomMethod(WSLit("getLibretroSeconds"), false);
	libraryObject.SetCustomMethod(WSLit("setLibretroFastForwardSeconds"), false);
	libraryObject.SetCustomMethod(WSLit("travelByScreenshot"), false);
	libraryObject.SetCustomMethod(WSLit("setRenderHold"), false);
	libraryObject.SetCustomMethod(WSLit("deselectEntity"), false);
	systemObject.SetCustomMethod(WSLit("adoptModel"), true);
	systemObject.SetCustomMethod(WSLit("isTempSelect"), true);
	systemObject.SetCustomMethod(WSLit("isFullscreenMode"), true);
	systemObject.SetCustomMethod(WSLit("getSteamPlayerCount"), true);
	systemObject.SetCustomMethod(WSLit("getGlobalStatHistory"), true);
	systemObject.SetCustomMethod(WSLit("getGlobalStat"), true);
	systemObject.SetCustomMethod(WSLit("getFriendCount"), true);
	systemObject.SetCustomMethod(WSLit("getFriendRichPresence"), true);
	systemObject.SetCustomMethod(WSLit("getDirectoryListing"), true);
	//"updateSnapshot"
	*/

	/*
	// LIBRARY
	result = pWebView->CreateGlobalJavascriptObject(WSLit("aaapi.library"));
	if (!result.IsObject())
	{
		DevMsg("Failed to create global library javascript object.\n");
		return;
	}

	JSObject& libraryObject = result.ToObject();

	// SUPER DUPER LIBRARY QUERY IN GENERALIZED ORDINARY GUY FORM
	// EACH QUERY HANDLE IS CLEARED WHEN IT IS USED TO EXHAUSTION
	// *ALL* QUERY HANDLES ARE CLEARED ON MAP TRANSITION FOR GARBAGE COLLECTION OF UNCLOSED HANDLES



	// CALLBACKS
	result = pWebView->CreateGlobalJavascriptObject(WSLit("aaapi.callbacks"));
	if (!result.IsObject())
	{
		DevMsg("Failed to create global callback javascript object.\n");
		return;
	}

	JSObject& callbacksObject = result.ToObject();

	// NETWORK
	result = pWebView->CreateGlobalJavascriptObject(WSLit("aaapi.network"));
	if (!result.IsObject())
	{
		DevMsg("Failed to create global network javascript object.\n");
		return;
	}

	JSObject& networkObject = result.ToObject();
	DevMsg("Finished adding JSAPI.\n");
	//networkObject.SetCustomMethod(WSLit("doConnect"), false);
	//networkObject.SetCustomMethod(WSLit("localClientReady"), false);
	//networkObject.SetCustomMethod(WSLit("extractOverviewTGA"), false);
	*/
}

void C_AwesomiumBrowserManager::DispatchJavaScriptMethod(C_AwesomiumBrowserInstance* pBrowserInstance, std::string objectName, std::string objectMethod, std::vector<std::string> methodArguments)
{
	//WebView* pWebView = m_webViews[pWebTab];

	JSValue response = pBrowserInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit(objectName.c_str()), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		for (auto argument : methodArguments)
			arguments.Push(WSLit(argument.c_str()));

		object.InvokeAsync(WSLit(objectMethod.c_str()), arguments);
	}

	//m_pWebBrowser->DispatchJavaScriptMethod(pWebTab, objectName, objectMethod, methodArguments);
	/*
	for (auto arg : args)
	{
	DevMsg("Argument: %s\n", arg->text.c_str());
	}
	*/
}

void C_AwesomiumBrowserManager::DispatchJavaScriptMethods(C_AwesomiumBrowserInstance* pBrowserInstance)
{
	//WebView* pWebView = m_webViews[pWebTab];

	std::string previousObjectName = "-1";

	JSValue response;
	JSObject responseObject;
	std::vector<JavaScriptMethodCall_t*>& methodCalls = pBrowserInstance->GetJavaScriptMethodCalls();
	for (auto pJavaScriptMethodCall : methodCalls)
	{
		if (previousObjectName != pJavaScriptMethodCall->objectName)
		{
			previousObjectName = pJavaScriptMethodCall->objectName;
			response = pBrowserInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit(pJavaScriptMethodCall->objectName.c_str()), WSLit(""));
			if (!response.IsObject())
				continue;

			responseObject = response.ToObject();
		}

		JSArray arguments;
		for (auto argument : pJavaScriptMethodCall->methodArguments)
			arguments.Push(WSLit(argument.c_str()));

		responseObject.InvokeAsync(WSLit(pJavaScriptMethodCall->methodName.c_str()), arguments);
	}

	//m_pWebBrowser->DispatchJavaScriptMethods(pWebTab);
}

void C_AwesomiumBrowserManager::OnCreateWebViewDocumentReady(WebView* pWebView, std::string id)
{
	// The master webview has created a new webview on demand.
	DevMsg("AwesomiumBrowserManager: OnCreateWebViewDocumentReady: %s\n", id.c_str());

	// TODO: Add global JS API object to the web view.
	/*
	//C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->FindWebTab(id);
	C_AwesomiumBrowserInstance* pBrowserInstance = this->FindAwesomiumBrowserInstance(id);
	if (pBrowserInstance)
	{
		pBrowserInstance->SetWebView(pWebView);
		pBrowserInstance->SetState(2);
		//m_webViews[pBrowserInstance] = pWebView;	// obsolete perhaps??
		
		ITexture* pTexture = pBrowserInstance->GetTexture();
		if (pTexture && pTexture->GetImageFormat() == IMAGE_FORMAT_BGRA8888)
			pWebView->SetTransparent(true);

		std::string initialURI = pBrowserInstance->GetInitialURL();
		std::string uri = initialURI;
		//if (id == "network")
	//		uri = initialURI;
		//else if (id == "images")
		//	uri = initialURI;	// this should never happen, so comment it out to avoid confusion
			//uri = "asset://ui/imageLoader.html";
		//else if (id == "hud")
		//	uri = initialURI;
			//uri = (initialURI == "") ? "asset://ui/default.html" : initialURI;	// this should never happen, so comment it out to avoid confusion
		//else
	//		uri = initialURI;

		DevMsg("Loading initial URL: %s\n", uri.c_str());
		pWebView->LoadURL(WebURL(WSLit(uri.c_str())));
	}*/
}
/*
void C_AwesomiumBrowserManager::OnHudWebViewDocumentReady(WebView* pWebView, std::string id)
{
	DevMsg("AwesomiumBrowserManager: OnHudWebViewDocumentReady: %s\n", id.c_str());
//	C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->GetHudWebTab();
	C_AwesomiumBrowserInstance* pBrowserInstance = this->FindAwesomiumBrowserInstance(id);
	if (pBrowserInstance)
	{
		pBrowserInstance->SetWebView(pWebView);
		pBrowserInstance->SetState(2);
		//m_webViews[pBrowserInstance] = pWebView;

		ITexture* pTexture = pBrowserInstance->GetTexture();
		if (pTexture && pTexture->GetImageFormat() == IMAGE_FORMAT_BGRA8888)
			pWebView->SetTransparent(true);

		// need to wait longer

		//	if (g_pAnarchyManager->GetWebManager()->GetHudWebTab() == pWebTab)	// FIXME: THIS IS POSSIBLY A RACE CONDITION.  IF AWESOMIUM WORKS SUPER FAST, THEN THIS WILL ALWAYS BE FALSE. This is what causes the web tab on the main menu to be blank?????
		//g_pAnarchyManager->GetWebManager()->OnHudWebTabReady();
//		g_pAnarchyManager->IncrementState();

		std::string initialURL = pBrowserInstance->GetInitialURL();
		std::string uri = (initialURL == "") ? "asset://ui/default.html" : initialURL;

		pWebView->LoadURL(WebURL(WSLit(uri.c_str())));

//		if (id == "hud")	// FIXME: Should wait until the hud loads its 1st page before moving on.
			//g_pAnarchyManager->IncrementState();
	}





	*/
	/*


	// The master webview has created a new webview on demand.
	DevMsg("AwesomiumBrowserManager: OnCreateWebViewDocumentReady: %s\n", id.c_str());

	// TODO: Add global JS API object to the web view.

	//C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->FindWebTab(id);
	C_AwesomiumBrowserInstance* pBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(id);
	if (pBrowserInstance)
	{
		//pBrowserInstance->SetState(2);
		m_webViews[pBrowserInstance] = pWebView;

		ITexture* pTexture = pBrowserInstance->GetTexture();
		if (pTexture && pTexture->GetImageFormat() == IMAGE_FORMAT_BGRA8888)
			pWebView->SetTransparent(true);

		std::string uri = (id == "images") ? "asset://ui/imageLoader.html" : pBrowserInstance->GetInitialURL();

		pWebView->LoadURL(WebURL(WSLit(uri.c_str())));
		DevMsg("Loading initial URL: %s\n", uri.c_str());

		if (id == "hud")
			g_pAnarchyManager->IncrementState();
	}
	*/
//}



void C_AwesomiumBrowserManager::CloseAllInstances(bool bDeleteHudAndImages)
{
	g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(null);

	std::vector<std::map<std::string, C_AwesomiumBrowserInstance*>::iterator> doomedIts;

	// iterate over all web tabs and call their destructors
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); ++it)
	{
		C_AwesomiumBrowserInstance* pInstance = it->second;
		if (!bDeleteHudAndImages && (pInstance->GetId() == "hud" || pInstance->GetId() == "images" || pInstance->GetId() == "network"))
			continue;

		if (pInstance == m_pSelectedAwesomiumBrowserInstance)
		{
			this->SelectAwesomiumBrowserInstance(null);
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		}

		//std::string nameTest = "";
		//nameTest += pInstance->GetId();

		//DevMsg("Remove awesomium instance %s\n", nameTest.c_str());
		//		if (pInstance->GetTexture() && g_pAnarchyManager->GetInputManager()->GetInputCanvasTexture() == pInstance->GetTexture())

		//if (bDeleteHudAndImages || (pInstance->GetId() != "hud" && pInstance->GetId() != "images"))
		//{
			if (g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pInstance)
			{
				g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(null);
				//g_pAnarchyManager->GetInputManager()->SetInputListener(null);
				//g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(null);
			}

			//		auto foundAwesomiumBrowserInstance = m_awesomiumBrowserInstances.find(pInstance->GetId());
			//		if (foundAwesomiumBrowserInstance != m_awesomiumBrowserInstances.end())
			//			m_awesomiumBrowserInstances.erase(foundAwesomiumBrowserInstance);

			DevMsg("Preparing to close instance w/ ID: %s\n", pInstance->GetId().c_str());

			pInstance->SelfDestruct();

			//if (!bDeleteHudAndImages)
				doomedIts.push_back(it);
		//}
	}

	if (doomedIts.size() > 0)
	{
		unsigned int max = doomedIts.size();
		DevMsg("Removing %u Awesomium instances...\n", max);
		for (unsigned int i = 0; i < max; i++)
			m_awesomiumBrowserInstances.erase(doomedIts[i]);
	}
	//else
	//	m_awesomiumBrowserInstances.clear();
}

void C_AwesomiumBrowserManager::Update()
{
	//if (m_pWebCore)
	//	m_pWebCore->Update();

	// if we are paused, do NOTHING else.  However, the web core NEEDED to process its update so that network msgs can be properly ignored instead of being queued until the next update.
	if (g_pAnarchyManager->IsPaused())
	{
		this->UpdateWebCore();
		return;
	}

	/*
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); ++it)
	{
		C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = it->second;
		pAwesomiumBrowserInstance->Update();
	}
	*/


	/* do nothing now.
	for (auto it = m_awesomiumBrowserInstances.begin(); it != m_awesomiumBrowserInstances.end(); ++it)
	{
		if (g_pAnarchyManager->GetCanvasManager()->IsPriorityEmbeddedInstance(it->second))
			it->second->Update();
	}
	*/
	C_AwesomiumBrowserInstance* pHudInstance = this->FindAwesomiumBrowserInstance("hud");
	if (pHudInstance)//&& g_pAnarchyManager->GetInputManager()->GetInputMode())
		pHudInstance->Update();
	else
		this->UpdateWebCore();	// If there is no HUD, the web core is probably still starting up.


//	if (m_pSelectedAwesomiumBrowserInstance)	// don't need
	//	m_pSelectedAwesomiumBrowserInstance->Update();


	

	//DevMsg("SteamBrowserManager: Update\n");
	//info->state = state;
//	if (m_pSelectedSteamBrowserInstance)
//		m_pSelectedSteamBrowserInstance->Update();
}
/*
C_AwesomiumBrowserInstance* C_AwesomiumBrowserManager::CreateAwesomiumBrowserInstance()
{
	C_SteamBrowserInstance* pSteamBrowserInstance = new C_SteamBrowserInstance();
	SelectSteamBrowserInstance(pSteamBrowserInstance);
	return pSteamBrowserInstance;
}
*/
bool C_AwesomiumBrowserManager::FocusAwesomiumBrowserInstance(C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance)
{
	m_pFocusedAwesomiumBrowserInstance = pAwesomiumBrowserInstance;
	return true;
}

bool C_AwesomiumBrowserManager::SelectAwesomiumBrowserInstance(C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance)
{
	m_pSelectedAwesomiumBrowserInstance = pAwesomiumBrowserInstance;
	return true;
}

void C_AwesomiumBrowserManager::GetAllInstances(std::vector<C_EmbeddedInstance*>& embeddedInstances)
{
	auto it = m_awesomiumBrowserInstances.begin();
	while (it != m_awesomiumBrowserInstances.end())
	{
		embeddedInstances.push_back(it->second);
		it++;
	}
}

void C_AwesomiumBrowserManager::UpdateWebCore()
{
	if (m_pWebCore)
		m_pWebCore->Update();
}

/*
void C_SteamBrowserManager::OnSteamBrowserInstanceCreated(C_SteamBrowserInstance* pSteamBrowserInstance)
{
	std::string id = pSteamBrowserInstance->GetId();
	m_steamBrowserInstances[id] = pSteamBrowserInstance;

	//pSteamBrowserInstance->GetInfo()->state = 1;
}

C_SteamBrowserInstance* C_SteamBrowserManager::FindSteamBrowserInstance(std::string id)
{
	auto foundSteamBrowserInstance = m_steamBrowserInstances.find(id);
	if (foundSteamBrowserInstance != m_steamBrowserInstances.end())
	{
		return foundSteamBrowserInstance->second;
			//return m_steamBrowserInstances[foundSteamBrowserInstance];
	}
	else
		return null;
}


void C_SteamBrowserManager::RunEmbeddedSteamBrowser()
{
	C_SteamBrowserInstance* pSteamBrowserInstance = this->CreateSteamBrowserInstance();
	pSteamBrowserInstance->Init("", "https://www.youtube.com/watch?v=0s4LADs8QnE", null);

	// http://anarchyarcade.com/press.html
	// https://www.youtube.com/html5
	// http://smarcade.net/dlcv2/view_youtube.php?id=CmRih_VtVAs&autoplay=1
}

void C_SteamBrowserManager::DestroySteamBrowserInstance(C_SteamBrowserInstance* pInstance)
{
	if (pInstance == m_pSelectedSteamBrowserInstance)
	{
		this->SelectSteamBrowserInstance(null);
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
	}

	if (g_pAnarchyManager->GetInputManager()->GetInputCanvasTexture() == pInstance->GetTexture())
	{
		g_pAnarchyManager->GetInputManager()->SetInputListener(null);
		g_pAnarchyManager->GetInputManager()->SetInputCanvasTexture(null);
	}

	auto foundSteamBrowserInstance = m_steamBrowserInstances.find(pInstance->GetId());
	if (foundSteamBrowserInstance != m_steamBrowserInstances.end())
		m_steamBrowserInstances.erase(foundSteamBrowserInstance);

	pInstance->SelfDestruct();
}
*/