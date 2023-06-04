#ifndef C_AWESOMIUM_BROWSER_INSTANCE_H
#define C_AWESOMIUM_BROWSER_INSTANCE_H

//#include "..\..\public\steam\isteamhtmlsurface.h"
#include "c_awesomiumjshandlers.h"
#include "../../public/steam/steam_api.h"
#include "c_embeddedinstance.h"
#include <string>
#include "vgui_controls/Controls.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>
#include <vector>

class C_AwesomiumBrowserInstance : public C_EmbeddedInstance
{
public:
	C_AwesomiumBrowserInstance();
	~C_AwesomiumBrowserInstance();

	void SelfDestruct();

	void Init(std::string id = "", std::string url = "", std::string title = "", bool alpha = false, const char* pchPostData = null, int entindex = -1);

	void Update();
	void TakeScreenshot();

	void CleanUpTexture();
	//void OnSecondsUpdated(){};
	void SetUrl(std::string url);
	bool IsSelected();
	bool HasFocus();
	bool Focus();
	bool Blur();
	bool Select();
	bool Deselect();

	std::string GetURL() { return m_URL; }

	void Close();

	void ResizeFrameFromRGB565(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void ResizeFrameFromRGB1555(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void ResizeFrameFromXRGB8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void CopyLastFrame(unsigned char* dest, unsigned int width, unsigned int height, size_t pitch, unsigned int depth);

	void OnProxyBind(C_BaseEntity* pBaseEntity = null);
	void Render();
	void RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect);

	unsigned int GetAlphaAtPoint(int x, int y);
	void SetSystemCursor(Awesomium::Cursor acursor);

	void OnMouseMove(float x, float y);
	void OnMousePressed(vgui::MouseCode code);
	void OnMouseReleased(vgui::MouseCode code);
	void OnMouseWheeled(int delta);
	void OnKeyPressed(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState);
	void OnKeyReleased(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState);

	void DispatchJavaScriptMethod(std::string objectName, std::string objectMethod, std::vector<std::string> methodArguments);
	void DispatchJavaScriptMethodCalls();
	//void DispatchJavaScriptEventMessages(std::string objectName, std::string objectMethod, std::vector<JSEventMessage_t*> eventArgs);
	//void DispatchJavaScriptMethod(std::string objectName, std::string objectMethod, std::vector<std::string> methodArguments);
	//void DispatchJavaScriptMethodBatch(C_WebTab* pWebTab, std::vector<MethodBatch_t*> batch);

	// hud webtab only
	void SetHudTitle(std::string title);
	void AddHudLoadingMessage(std::string type, std::string text, std::string title = "", std::string id = "", std::string min = "", std::string max = "", std::string current = "", std::string callbackMethod = "");
	//std::vector<JavaScriptMethodCall_t*> GetJavaScriptMethodCalls() { return m_javaScriptMethodCalls; }

	// images webtab only
	void ResetImagesSession();
	std::string GetImagesSessionId() { return m_imagesSessionId; }
	void DecrementNumImagesLoading();
	void IncrementNumImagesLoading();
	//void SetNumImagesLoading(int num) { m_iNumImagesLoading = num; }
	int GetNumImagesLoading() { return m_iNumImagesLoading; }
	void ClearNumImagesLoading();
	bool RequestLoadSimpleImage(std::string channel, std::string itemId);	// images web-views only!
	void OnSimpleImageReady(std::string sessionId, std::string channel, std::string itemId, std::string field, ITexture* pTexture);	// images web-views only!
	void SaveImageToCache(std::string fieldVal);

	C_EmbeddedInstance* GetParentSelectedEmbeddedInstance();

	bool IsDirty();
	void FirstDocumentReady();
	void OnChangeTargetURL(std::string url);

	// accessors
	std::string GetId() { return m_id; }
	std::string GetTitle() { return m_title; }
	ITexture* GetTexture() { return m_pTexture; }
	int GetLastVisibleFrame() { return m_iLastVisibleFrame; }
	int GetLastRenderedFrame() { return m_iLastRenderedFrame; }
	std::string GetInitialURL() { return m_initialURL; }
	Awesomium::WebView* GetWebView() { return m_pWebView; }
	std::vector<JavaScriptMethodCall_t*> GetJavaScriptMethodCalls() { return m_javaScriptMethodCalls; }
	int GetState() { return m_iState; }
	C_InputListener* GetInputListener();
	std::string GetOriginalItemId() { return m_originalItemId; }
	void GetLastMouse(float &fMouseX, float &fMouseY);
	int GetOriginalEntIndex() { return m_iOriginalEntIndex; }

	// mutators	
	void SetRenderHold(bool bVal) { m_bRenderHold = bVal; }
	//void SetState(int val) { m_iState = val; }
	//void SetHasLoaded(bool bVal) { m_bHasLoaded = bVal; }
	void SetBlankShotPending(bool bValue) { m_bBlankShotPending = bValue; }
	void SetTitle(std::string title) { m_title = title; }
	void SetState(int state) { m_iState = state; }
	void SetWebView(Awesomium::WebView* pWebView) { m_pWebView = pWebView; }
	void SetOriginalItemId(std::string itemId) { m_originalItemId = itemId; }
	void SetOriginalEntIndex(int val) { m_iOriginalEntIndex = val; }

private:
	bool m_bRenderHold;
	bool m_bHasLoaded;
	bool m_bIsDirty;
	bool m_bNotHovered;
	bool m_bBlankShotPending;
	float m_fLastMouseX;
	float m_fLastMouseY;

	std::string m_URL;
	std::string m_imagesSessionId;
	int m_iNumImagesLoading;
	int m_iMaxImagesLoading;

	std::vector<JavaScriptMethodCall_t*> m_javaScriptMethodCalls;

	int m_iLastVisibleFrame;
	int m_iState;
	void* m_pLastFrameData;
	bool m_bReadyForNextFrame;
	bool m_bCopyingFrame;
	bool m_bReadyToCopyFrame;
	ITexture* m_pTexture;
	int m_iLastRenderedFrame;
	HHTMLBrowser m_unBrowserHandle;
	std::string m_title;
	std::string m_id;
	std::string m_originalItemId;
	int m_iOriginalEntIndex;
	std::string m_initialURL;
	bool m_bAlpha;
	void* m_pPostData;
	//bool m_bIsDirty;
	Awesomium::WebView* m_pWebView;
};

#endif