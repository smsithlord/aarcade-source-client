#ifndef C_STEAM_BROWSER_INSTANCE_H
#define C_STEAM_BROWSER_INSTANCE_H

//#include "..\..\public\steam\isteamhtmlsurface.h"
#include "../../public/steam/steam_api.h"
#include "c_inputlistenersteambrowser.h"
#include "c_embeddedinstance.h"
#include <string>
#include <vector>
#include "vgui_controls/Controls.h"
//#include <vector>

struct aaAPIObjectTransform_t {
	int entityIndex;
	std::string id;
	Vector position;
	QAngle rotation;
	float scale;
};

class C_SteamBrowserInstance : public C_EmbeddedInstance
{
public:
	C_SteamBrowserInstance();
	~C_SteamBrowserInstance();

	void SelfDestruct();
	//void DoDefunctDestruct(bool& result);

	void Init(std::string id = "", std::string url = "", std::string title = "", const char* pchPostData = null, int entindex = -1);

	/*
	CCallResult<C_SteamBrowserInstance, HTML_BrowserReady_t> m_BrowserReadyInitial;
	void BrowserInstanceBrowserReadyInitial(HTML_BrowserReady_t *pResult, bool bIOFailure);
	*/

	void OnBrowserInstanceReady(unsigned int unHandle);
	void OnBrowserInstanceWantsToClose();
	void OnBrowserInstanceStartRequest(const char* pchURL, const char* pchTarget, const char* pchPostData, bool IsRedirect);
	void OnBrowserInstanceFinishedRequest(const char* pchURL, const char* pchPageTitle);
	void OnBrowserInstanceOpenLinkInTab(const char* pchURL);
	void OnBrowserInstanceNeedsPaint(const char* pBGRA, unsigned int unWide, unsigned int unTall, unsigned int unUpdateX, unsigned int unUpdateY, unsigned int unUpdateWide, unsigned int unUpdateTall, unsigned int unScrollX, unsigned int unScrollY, float flPageScale, unsigned int unPageSerial);
	void OnBrowserInstanceURLChanged(const char* pchURL, const char* pchPostData, bool bIsRedirect, const char* pchPageTitle, bool bNewNavigation);
	void OnBrowserInstanceChangedTitle(const char* pchTitle);
	void OnBrowserInstanceSearchResults(unsigned int unResults, unsigned int unCurrentMatch);
	void OnBrowserInstanceCanGoBackAndForward(bool bCanGoBack, bool bCanGoForward);
	void OnBrowserInstanceHorizontalScroll(unsigned int unScrollMax, unsigned int unScrollCurrent, float flPageScale, bool bVisible, unsigned int unPageSize);
	void OnBrowserInstanceVerticalScroll(unsigned int unScrollMax, unsigned int unScrollCurrent, float flPageScale, bool bVisible, unsigned int unPageSize);
	void OnBrowserInstanceLinkAtPosition(unsigned int x, unsigned int y, const char* pchURL, bool bInput, bool bLiveLink);
	void OnBrowserInstanceJSAlert(const char* pchMessage);
	void OnBrowserInstanceJSConfirm(const char* pchMessage);
	void OnBrowserInstanceFileOpenDialog(const char* pchTitle, const char* pchInitialFile);
	void OnBrowserInstanceNewWindow(const char* pchURL, unsigned int unX, unsigned int unY, unsigned int unWide, unsigned int unTall, unsigned int unNewWindow_BrowserHandle);
	void OnBrowserInstanceSetCursor(unsigned int eMouseCursor);
	void OnBrowserInstanceStatusText(const char* pchMsg);
	void OnBrowserInstanceShowToolTip(const char* pchMsg);
	void OnBrowserInstanceUpdateToolTip(const char* pchMsg);
	void OnBrowserInstanceHideTollTip();

	void ProcessAPIObjectUpdate(std::string id, std::string model, std::string position, std::string rotation, std::string scale, std::string sequence, std::string removed);

	void JavaScriptAPICommand(std::string cmd, std::vector<std::string> dataBuf);


	bool IsSelected();
	bool HasFocus();
	bool Blur();
	bool Focus();
	bool Select();
	bool Deselect();

	void Close();

	void SetUrl(std::string url);
	void GoForward();
	void GoBack();
	void Reload();

	std::string GetId() { return m_id; }
	void Update();
	void TakeScreenshot();
	void TakeScreenshotNow(ITexture* pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch, unsigned int depth);

	void CopyLastFrame(const void* data, unsigned int width, unsigned int height, unsigned int depth);
	void CopyLastFrame(unsigned char* dest, unsigned int width, unsigned int height, size_t pitch, unsigned int depth);

	void OnProxyBind(C_BaseEntity* pBaseEntity);
	void Render();
	void RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect);

	void CleanUpTexture();
	//void OnSecondsUpdated(){};

	C_EmbeddedInstance* GetParentSelectedEmbeddedInstance();

	bool OnStartRequest(const char *url, const char *target, const char *pchPostData, bool bIsRedirect);
//	void OnFinishRequest(const char *url, const char *pageTitle, const CUtlMap < CUtlString, CUtlString > &headers);
	
	//void BrowserInstanceNeedsPaint(HTML_NeedsPaint_t *pCallback);

	//wchar_t GetTypedChar(vgui::KeyCode code);

	void OnMouseMove(float x, float y);
	void OnMousePressed(vgui::MouseCode code);
	void OnMouseReleased(vgui::MouseCode code);
	void OnKeyCodePressed(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState);
	void OnKeyCodeReleased(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState);
	void OnMouseWheeled(int delta);

	void InjectJavaScript(std::string code);

	//bool IsDefunct() { return m_bDefunct; }
	//bool IsDying() { return m_bDying; }

	// STEAMWORKS ONLY
	vgui::KeyCode KeyCode_VirtualKeyToVGUI(int key);
	int KeyCode_VGUIToVirtualKey(vgui::KeyCode code);
	int GetKeyModifiersAlt();

	void OnAPIObjectCreated(std::string sessionId, std::string objectId, int iEntityIndex, int iParentEntityIndex);
	void DeleteAllAPIObjects();

	void ResizeFrameFromXRGB8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);

	// accessors
	bool GetCanGoForward() { return m_bCanGoForward; }
	bool GetCanGoBack() { return m_bCanGoBack; }
	std::string GetTitle() { return m_title; }
	std::string GetURL() { return m_URL; }
	ITexture* GetTexture() { return m_pTexture; }
	int GetLastVisibleFrame() { return m_iLastVisibleFrame; }
	int GetLastRenderedFrame() { return m_iLastRenderedFrame; }
	C_InputListener* GetInputListener();
	unsigned int GetHandle() { return m_unHandle; }
	std::string GetInitialUrl() { return m_initialURL; }
	std::string GetScraperId() { return m_scraperId; }
	std::string GetScraperItemId() { return m_scraperItemId; }
	std::string GetScraperField() { return m_scraperItemId; }
	std::string GetOriginalItemId() { return m_originalItemId; }
	int GetOriginalEntIndex() { return m_iOriginalEntIndex; }
	void GetLastMouse(float &fMouseX, float &fMouseY);
	bool IsDirty();// { return m_bIsDirty; }
	bool GetUseVideoFilters() { return m_bUseVideoFilters; }

	// mutators	
	void SetTitle(std::string title) { m_title = title; }
	void SetTexture(ITexture* pTexture) { m_pTexture = pTexture; }
	void SetHandle(unsigned int unHandle) { m_unHandle = unHandle; }
	void SetActiveScraper(std::string scraperId, std::string itemId, std::string field) {
		m_scraperId = scraperId;
		m_scraperItemId = itemId;
		m_scraperField = field;
	}
	void SetOriginalItemId(std::string itemId) { m_originalItemId = itemId; }
	void SetOriginalEntIndex(int val) { m_iOriginalEntIndex = val; }
	void SetBlankShotPending(bool bValue) { m_bBlankShotPending = bValue; }
	void SetUseVideoFilters(bool bValue) { m_bUseVideoFilters = bValue; }

private:
	/*
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceBrowserReady, HTML_BrowserReady_t, m_BrowserReady);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceNeedsPaint, HTML_NeedsPaint_t, m_NeedsPaint);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceStartRequest, HTML_StartRequest_t, m_StartRequest);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceCloseBrowser, HTML_CloseBrowser_t, m_CloseBrowser);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceURLChanged, HTML_URLChanged_t, m_URLChanged);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceFinishedRequest, HTML_FinishedRequest_t, m_FinishedRequest);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceOpenLinkInTab, HTML_OpenLinkInNewTab_t, m_OpenLinkInTab);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceChangedTitle, HTML_ChangedTitle_t, m_ChangedTitle);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceSearchResults, HTML_SearchResults_t, m_SearchResults);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceCanGoBackAndForward, HTML_CanGoBackAndForward_t, m_CanGoBackAndForward);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceHorizontalScroll, HTML_HorizontalScroll_t, m_HorizontalScroll);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceVerticalScroll, HTML_VerticalScroll_t, m_VerticalScroll);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceLinkAtPosition, HTML_LinkAtPosition_t, m_LinkAtPosition);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceJSAlert, HTML_JSAlert_t, m_JSAlert);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceJSConfirm, HTML_JSConfirm_t, m_JSConfirm);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceFileOpenDialog, HTML_FileOpenDialog_t, m_FileOpenDialog);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceNewWindow, HTML_NewWindow_t, m_NewWindow);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceSetCursor, HTML_SetCursor_t, m_SetCursor);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceStatusText, HTML_StatusText_t, m_StatusText);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceShowToolTip, HTML_ShowToolTip_t, m_ShowToolTip);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceUpdateToolTip, HTML_UpdateToolTip_t, m_UpdateToolTip);
	STEAM_CALLBACK(C_SteamBrowserInstance, BrowserInstanceHideToolTip, HTML_HideToolTip_t, m_HideToolTip);
	*/

	//int m_iLastNeedsPaint;

	unsigned int m_uAtlasWidth;// AA_ATLAS_INSTANCE_WIDTH
	unsigned int m_uAtlasHeight;// AA_ATLAS_INSTANCE_HEIGHT

	bool m_bTakeScreenshot;

	bool m_bUseVideoFilters;
	int m_iChromaLightR;
	int m_iChromaLightG;
	int m_iChromaLightB;
	int m_iChromaDarkR;
	int m_iChromaDarkG;
	int m_iChromaDarkB;

	ConVar* m_pProjectorFixConVar;
	ConVar* m_pOutputTestDomConVar;

	bool m_bWoke;
	float m_fNeedsWakupJuice;

	bool m_bBlankShotPending;
	float m_fLastMouseX;
	float m_fLastMouseY;
	bool m_bSteamworksCopying;
	bool m_bCanGoBack;
	bool m_bCanGoForward;
	//bool m_bDying;
	//bool m_bDefunct;
	std::string m_scraperId;
	std::string m_scraperItemId;
	std::string m_scraperField;
	std::string m_URL;
	int m_iLastVisibleFrame;
	unsigned int m_unHandle;
	void* m_pLastFrameData;
	bool m_bReadyForNextFrame;
	bool m_bCopyingFrame;
	bool m_bReadyToCopyFrame;
	ITexture* m_pTexture;
	int m_iLastRenderedFrame;
	//HHTMLBrowser m_unBrowserHandle;
	std::string m_title;
	std::string m_id;
	std::string m_originalItemId;
	std::string m_initialURL;
	void* m_pPostData;
	bool m_bIsDirty;
	int m_iOriginalEntIndex;
	ConVar* m_pYouTubeEndBehavior;
	//std::vector<std::string> objectIds;
	//CUtlMap
	//std::map<std::string, int> m_objectMap;
	std::map<std::string, aaAPIObjectTransform_t*> m_apiObjectTransforms;
	float m_flChromaA1;
	float m_flChromaA2;

	std::string m_syncFixInitialURL;
};

#endif