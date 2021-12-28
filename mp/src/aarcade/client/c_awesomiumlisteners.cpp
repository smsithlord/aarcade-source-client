#include "cbase.h"

#include "c_awesomiumlisteners.h"
#include "c_anarchymanager.h"
//#include "aa_globals.h"
//#include "Filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
namespace Awesomium
{
	const char* WebStringToCharString(WebString web_string)
	{
		int len = web_string.ToUTF8(null, 0);
		char* buf = new char[len + 1];
		web_string.ToUTF8(buf, len);
		buf[len] = 0;	// null terminator

		std::string title = buf;
		delete[] buf;

		return VarArgs("%s", title.c_str());
	}
}
*/
// MASTER
void MasterLoadListener::OnDocumentReady(WebView* caller, const WebURL& url)
{
	//DevMsg("MasterLoadListener: OnDocumentReady: %s\n", WebStringToCharString(url.spec()));

	// IT IS NOW SAFE TO CREATE GLOBAL JAVASCRIPT OBJECTS
//	C_WebBrowser* pWebBrowser = g_pAnarchyManager->GetWebManager()->GetWebBrowser();
//	int iState = pWebBrowser->GetState();
	//if (iState == 1)
	//{
		g_pAnarchyManager->GetAwesomiumBrowserManager()->OnMasterWebViewDocumentReady();
		//caller->ExecuteJavascript(WSLit("window.open('data:text/html,<html><body></body></html>', '', 'width=200,height=100');"), WSLit(""));
		/*
	}
	else if (iState == 2)
	{
		// extract a web tab id
		std::string id;
		std::string spec = WebStringToCharString(url.spec());

		size_t foundPrefix = spec.find("data:text/html,<html><body>");
		if (foundPrefix != std::string::npos)
		{
			size_t foundPostfix = spec.find("</body></html>");
			if (foundPostfix != std::string::npos)
			{
				// we probably have an id
				id = spec.substr(foundPrefix + 27, foundPostfix);
			}
		}

		DevMsg("Detected ID is: %s\n", id.c_str());
		pWebBrowser->OnCreateWebViewDocumentReady(id);
		//caller->ExecuteJavascript(WSLit("window.open('data:text/html,<html><body></body></html>;', '', 'width=200,height=100');"), WSLit(""));
	}
	*/
}

void MasterViewListener::OnAddConsoleMessage(WebView* caller, const WebString &message, int line_number, const WebString &source)
{
	DevMsg("MasterViewListener: OnAddConsoleMessage: %i\t%s\n", line_number, WebStringToCharString(message));
}

void MasterViewListener::OnShowCreatedWebView(WebView *caller, WebView *new_view, const WebURL &opener_url, const WebURL &target_url, const Rect &initial_pos, bool is_popup)
{
	DevMsg("MasterViewListener: OnShowCreatedWebView: %s\n", WebStringToCharString(target_url.spec()));

	//C_WebBrowser* pWebBrowser = g_pAnarchyManager->GetWebManager()->GetWebBrowser();

	std::string urlSpec = WebStringToCharString(target_url.spec());
	size_t foundPrefix = urlSpec.find("asset://newwindow/");
	if (foundPrefix == 0)
	{
		// extract a web tab id
		std::string id = urlSpec.substr(18);
		//DevMsg("ID here is: %s\n", id.c_str());
		g_pAnarchyManager->GetAwesomiumBrowserManager()->PrepareWebView(new_view, id);
	}
	else
	{
		new_view->Destroy();
	}
}

void MasterViewListener::OnChangeTargetURL(WebView* caller, const WebURL &url)
{
	DevMsg("MasterViewListener: OnChangeTargetURL: %s\n", WebStringToCharString(url.spec()));
}

// REGULAR
void LoadListener::OnBeginLoadingFrame(WebView* caller, int64 frame_id, bool is_main_frame, const WebURL& url, bool is_error_page)
{
	//std::string urlSpec = WebStringToCharString(url.spec());
	//if (urlSpec.find("asset://ui/mouseEZ.html") != 0)
	//	g_pAnarchyManager->BringToTop();
}

void LoadListener::OnFinishLoadingFrame(WebView* caller, int64 frame_id, bool is_main_frame, const WebURL& url)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	std::string urlSpec = WebStringToCharString(url.spec());
	if (caller == pHudBrowserInstance->GetWebView() && !g_pAnarchyManager->IsInitialized() && urlSpec == "asset://ui/startup.html" )
	{
		g_pAnarchyManager->OnStartup();
		return;
	}

	// bring to top disabled on 2/9/2019 cuz i think its what's making me rage sometimes. <-- good comment bra. real helpful.
	//if (urlSpec.find("asset://ui/welcome.html") != 0)
	//	g_pAnarchyManager->BringToTop();

	if (urlSpec.find("asset://ui/welcome.html") == 0)
		g_pAnarchyManager->MainMenuLoaded();
}

void LoadListener::OnDocumentReady(WebView* caller, const WebURL& url)
{
	//DevMsg("LoadListener: OnDocumentReady: %s\n", WebStringToCharString(url.spec()));

	//C_WebBrowser* pWebBrowser = g_pAnarchyManager->GetWebManager()->GetWebBrowser();

	std::string urlSpec = WebStringToCharString(url.spec());
	size_t foundPrefix = urlSpec.find("asset://newwindow/");
	if (foundPrefix == 0)
	{
		// extract a web tab id
		std::string id = urlSpec.substr(18);
		//DevMsg("Sally: %s\n", urlSpec.c_str());

		//if (id == "hud" && g_pAnarchyManager->GetState() == AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT )// || bHudReady
		//	g_pAnarchyManager->GetAwesomiumBrowserManager()->OnHudWebViewDocumentReady(caller, id);
		//else
			g_pAnarchyManager->GetAwesomiumBrowserManager()->OnCreateWebViewDocumentReady(caller, id);
	}
	else
	{
		C_AwesomiumBrowserInstance* pBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(caller);
		if (pBrowserInstance)
			pBrowserInstance->FirstDocumentReady();
		//DevMsg("hariy: %s\n", urlSpec.c_str());
		//g_pAnarchyManager->GetAwesomiumBrowserManager()->OnHudWebViewDocumentReady(caller, "hud");
	//	if (!g_pAnarchyManager->GetWebManager()->GetHudReady())
//			pWebBrowser->OnHudWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());





		//{
			//foundPrefix = urlSpec.find("asset://ui/loading.html");
			//if (foundPrefix == 0)
			//	pWebBrowser->OnHudWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());
			//foundPrefix = urlSpec.find("asset://ui/hud.html");
			//if (foundPrefix == 0)
				//pWebBrowser->OnHudWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());
			//else
		//	{
//				foundPrefix = urlSpec.find("asset://ui/loading.html");
//				if (foundPrefix == 0)
//					pWebBrowser->OnHudWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());
				//pWebBrowser->OnLoadingWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());
		//	}
		//}
		/*
		else
		{
			foundPrefix = urlSpec.find("asset://ui/loading.html");
			if ( foundPrefix == 0 )
				pWebBrowser->OnLoadingWebViewDocumentReady(caller, g_pAnarchyManager->GetWebManager()->GetHudWebTab()->GetId());
		}
		*/
	}
}

void ViewListener::OnChangeCursor(WebView* caller, Awesomium::Cursor cursor)
{
	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(caller);
	pAwesomiumBrowserInstance->SetSystemCursor(cursor);
}

void ViewListener::OnAddConsoleMessage(WebView* caller, const WebString &message, int line_number, const WebString &source)
{
	DevMsg("ViewListener: OnAddConsoleMessage: %i\t%s\n", line_number, WebStringToCharString(message));
}

void ViewListener::OnShowCreatedWebView(WebView *caller, WebView *new_view, const WebURL &opener_url, const WebURL &target_url, const Rect &initial_pos, bool is_popup)
{
	DevMsg("ViewListener: OnShowCreatedWebView: %s\n", WebStringToCharString(target_url.spec()));
	//C_WebBrowser* pWebBrowser = g_pAnarchyManager->GetWebManager()->GetWebBrowser();
	//pWebBrowser->PrepareWebView(new_view);
	new_view->Destroy();
}

void ViewListener::OnChangeTargetURL(WebView* caller, const WebURL &url)
{
	//DevMsg("ViewListener: OnChangeTargetURL: %s\n", WebStringToCharString(url.spec()));
	//C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(caller);
	//std::string spec = WebStringToCharString(url.spec());
	//pAwesomiumBrowserInstance->OnChangeTargetURL(spec);
}

void ViewListener::OnChangeAddressBar(WebView* caller, const WebURL &url)
{
	//DevMsg("ViewListener: OnChangeTargetURL: %s\n", WebStringToCharString(url.spec()));
	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(caller);
	std::string spec = WebStringToCharString(url.spec());
	//DevMsg("Addy is: %s\n", spec.c_str());
	pAwesomiumBrowserInstance->OnChangeTargetURL(spec);
}

void MenuListener::OnShowPopupMenu(WebView *caller, const WebPopupMenuInfo &menu_info)
{
//	DevMsg("DISABLED FOR TESTING!\n");
//	return;

	//C_WebTab* pWebTab = g_pAnarchyManager->GetWebManager()->GetWebBrowser()->FindWebTab(caller);
	//C_WebTab* pHudWebTab = g_pAnarchyManager->GetWebManager()->GetHudWebTab();
	//WebView* pHudWebView = g_pAnarchyManager->GetWebManager()->GetWebBrowser()->FindWebView(pHudWebTab);

	C_AwesomiumBrowserInstance* pAwesomiumBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance(caller);	// FIXME: This should be a general EmbeddedInstance of any type.
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	WebView* pHudWebView = pHudBrowserInstance->GetWebView();

	DevMsg("Pop menu detected!\n");

	std::vector<std::string> methodArguments;
	methodArguments.push_back(pAwesomiumBrowserInstance->GetId());
	methodArguments.push_back(VarArgs("%i", menu_info.bounds.x));
	methodArguments.push_back(VarArgs("%i", menu_info.bounds.y));
	methodArguments.push_back(VarArgs("%i", menu_info.bounds.width));
	methodArguments.push_back(VarArgs("%i", menu_info.bounds.height));
	methodArguments.push_back(VarArgs("%i", menu_info.item_height));
	methodArguments.push_back(VarArgs("%f", menu_info.item_font_size));
	methodArguments.push_back(VarArgs("%i", menu_info.selected_item));
	methodArguments.push_back(VarArgs("%i", menu_info.right_aligned));

	for (int i = 0; i < menu_info.items.size(); i++)
	{
		if (menu_info.items[i].type == kWebMenuItemType_Option)
			methodArguments.push_back("Option");
		else if (menu_info.items[i].type == kWebMenuItemType_CheckableOption)
			methodArguments.push_back("CheckableOption");
		else if (menu_info.items[i].type == kWebMenuItemType_Group)
			methodArguments.push_back("Group");
		else if (menu_info.items[i].type == kWebMenuItemType_Separator)
			methodArguments.push_back("Separator");

		methodArguments.push_back(WebStringToCharString(menu_info.items[i].label));
		methodArguments.push_back(WebStringToCharString(menu_info.items[i].tooltip));
		methodArguments.push_back(VarArgs("%i", menu_info.items[i].action));
		methodArguments.push_back(VarArgs("%i", menu_info.items[i].right_to_left));
		methodArguments.push_back(VarArgs("%i", menu_info.items[i].has_directional_override));
		methodArguments.push_back(VarArgs("%i", menu_info.items[i].enabled));
		methodArguments.push_back(VarArgs("%i", menu_info.items[i].checked));
	}
	
	std::string objectName = "window.arcadeHud";
	std::string objectMethod = "showPopupMenu";

	JSValue response = pHudWebView->ExecuteJavascriptWithResult(WSLit(objectName.c_str()), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		for (auto argument : methodArguments)
			arguments.Push(WSLit(argument.c_str()));

		object.InvokeAsync(WSLit(objectMethod.c_str()), arguments);
	}
}

void DialogListener::OnShowFileChooser(WebView *caller, const WebFileChooserInfo &chooser_info)
{
	//if (caller != static_cast<WebView*>(pClientArcadeResources->GetHDViewWeb()))
	//	return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance || caller != pNetworkInstance->GetWebView() || (chooser_info.mode != kWebFileChooserMode_Open && chooser_info.mode != kWebFileChooserMode_OpenMultiple))
		return;

	//DevMsg("Value is: %i\n", chooser_info.accept_types.size());

	/*
	WebString value = chooser_info.accept_types.size();

	// Convert the path to a string
	int len = value.ToUTF8(null, 0);
	char* buf = new char[len+1];
	value.ToUTF8(buf, len);
	buf[len] = 0;	// null terminator

	std::string default_name = buf;
	delete[] buf;
	*/

	//	DevMsg("Default name: %s\n", default_name.c_str());
	//	DevMsg("Default current is %i\n", pClientArcadeResources->GetCurrentScreenshot());
	//	DevMsg("Default is %s\n", pClientArcadeResources->GetScreenshotFile(pClientArcadeResources->GetCurrentScreenshot()));

	//JSValue myVal = caller->ExecuteJavascriptWithResult(WSLit("window.location.href;"), WSLit(""));
	//if (myVal.IsString())
	//{
		//std::string location = WebStringToCharString(myVal.ToString());

		//if (true)//location.substr(0, 42) == "http://www.smarcade.net/dlcv2/fb_share.php")
		//{
			/*
			std::string screenshotFile = engine->GetGameDirectory();
			screenshotFile += "\\";
			screenshotFile += pClientArcadeResources->GetScreenshotFile(pClientArcadeResources->GetCurrentScreenshot());
			*/
	JSValue batchId = caller->ExecuteJavascriptWithResult(WSLit("document.querySelector('#fileInput').batchId;"), WSLit(""));
	g_pAnarchyManager->GetMetaverseManager()->HandleAssetUploadBatch(WebStringToCharString(batchId.ToString()));
		//}
	//}
}

void ProcessListener::OnUnresponsive(WebView* caller)
{
	DevMsg("Web view is unresponsive!!\n");
}

void ProcessListener::OnResponsive(WebView* caller)
{
	DevMsg("Web view is responsive once again!!\n");
}

void ProcessListener::OnCrashed(WebView* caller, TerminationStatus status)
{
	DevMsg("Web view has crashed!!\n");
}