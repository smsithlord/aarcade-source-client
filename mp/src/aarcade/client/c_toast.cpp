#include "cbase.h"
#include <vgui_controls/Panel.h>

#include "c_toast.h"
#include "c_anarchymanager.h"

/*
#include "vgui_controls/MenuButton.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/ListPanel.h"
*/

#include "vgui_controls/Label.h"
#include "vgui/IPanel.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/IVGUI.h"
#include "vgui/IInput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

void HideAllChildren(Panel* pPanel)
{
	// Hide all children of this panel (including the invisible title bar that steals mouse move input
	for (int i = 0; i < pPanel->GetChildCount(); i++)
	{
		Panel* pChildPanel = pPanel->GetChild(i);
		pChildPanel->SetVisible(false);
	}
}

CToastSlate::CToastSlate(vgui::VPANEL parent) : Frame(null, "ToastSlate")
{
	DevMsg("Toast Slate Created\n");

	m_iNumTasksOpen = 0;
	m_iOldNumTasksOpen = 0;
	m_pShowArcadeHudConVar = cvar->FindVar("show_arcade_hud");
	m_iShowMode = -1;// m_pShowArcadeHudConVar->GetInt();

	// vgui constructor stuff
	SetParent( parent );
	SetProportional(false);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetWide(ScreenWidth());
	SetTall(ScreenHeight());
	SetPaintBackgroundEnabled(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	HideAllChildren(this);

	// Get a font
	vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(GetScheme());
	vgui::HFont hFont = pScheme->GetFont("Trebuchet24");
	vgui::HFont hFontMedium = pScheme->GetFont("Trebuchet18");

	m_pInitializingPanel = null;/* new Panel(this->GetPanel(), "initializingPanel");
	m_pInitializingPanel->DisableMouseInputForThisPanel(true);
	//m_pInitializingPanel->SetFont(hFontMedium);
	//m_pInitializingPanel->SetContentAlignment(Label::a_center);
	m_pInitializingPanel->SetPaintBackgroundEnabled(true);
	m_pInitializingPanel->SetBgColor(Color(0, 0, 0, 255));
	m_pInitializingPanel->SetPos(0, 0);
	m_pInitializingPanel->SetSize(ScreenWidth(), ScreenHeight());
	m_pInitializingPanel->SetVisible(true);
	//m_pMenuLabel->SetTextInset(iPadding, 0);*/

	// Toast
	m_pLabel = new Label(this, "Toast", g_pAnarchyManager->GetToastText().c_str());
	m_pLabel->SetFont(hFont);
	m_pLabel->DisableMouseInputForThisPanel(true);
	m_pLabel->SetContentAlignment(Label::a_west);
	m_pLabel->SetTextInset(20, 0);
	m_pLabel->SetPos(0, ScreenHeight() / 5);
	m_pLabel->SetPaintBackgroundEnabled(true);

	int contentWidth;
	int contentHeight;
	m_pLabel->GetContentSize(contentWidth, contentHeight);
	m_pLabel->SetSize(contentWidth + 20, contentHeight + 20);
	m_pLabel->SetBgColor(Color(0, 0, 0, 220));
	g_pAnarchyManager->AddToastLabel(m_pLabel);

	// Hover Title
	m_pHoverLabel = new Label(this, "HoverTitle", "");
	m_pHoverLabel->SetFont(hFont);
	m_pHoverLabel->DisableMouseInputForThisPanel(true);
	m_pHoverLabel->SetContentAlignment(Label::a_center);
	m_pHoverLabel->SetPaintBackgroundEnabled(true);

	//m_pHoverLabel->SetCenterWrap(false);
	//m_pHoverLabel->SetSize(1, 1);

	int hoverContentWidth;
	int hoverContentHeight;
	m_pHoverLabel->GetContentSize(hoverContentWidth, hoverContentHeight);
	//m_pHoverLabel->SetSize(hoverContentWidth + 20, hoverContentHeight + 20);

	//m_pHoverLabel->SetSize(ScreenWidth(), contentHeight + 20);
	//m_pHoverLabel->SetPos(0, ScreenHeight() - ((contentHeight + 20) / 2.0));

	//m_pHoverLabel->SetSize(1, 1);
	//m_pHoverLabel->SetCenterWrap(true);
	m_pHoverLabel->SetPos(ScreenWidth() / 2, ScreenHeight() / 2);
	m_pHoverLabel->SetBgColor(Color(0, 0, 0, 220));
	g_pAnarchyManager->InitHoverLabel(m_pHoverLabel);





	// Now lets add the new HUD elements
	m_lastMenuState = AAHUDBUTTONLABEL_NONE;
	m_lastTabsState = AAHUDBUTTONLABEL_NONE;
	m_lastContinuousState = AAHUDBUTTONLABEL_NONE;
	m_lastSelectState = AAHUDBUTTONLABEL_NONE;
	m_lastPreviousState = AAHUDBUTTONLABEL_NONE;
	m_lastBuildState = AAHUDBUTTONLABEL_NONE;
	m_lastNextState = AAHUDBUTTONLABEL_NONE;
	m_lastMouseModeState = AAHUDBUTTONLABEL_NONE;
	m_lastHudState = AAHUDBUTTONLABEL_NONE;

	int iButtonOffsetX = 6;
	int iButtonOffsetY = 4;
	int iPanelWidth = 256;
	int iPanelHeight = 128;
	int iPanelMargin = 24;
	int iActualButtonWidth = 76;
	int iButtonImageWidth = 128;
	int iActualButtonHeight = 64;
	int iButtonImageHeight = 64;
	int iPadding = 8;
	int iContentWidth, iContentHeight;

	//=================
	// MENU
	//=================

	// menuPanel
	m_pMenuPanel = new Panel(this->GetPanel(), "menuPanel");
	m_pMenuPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pMenuPanel);
	m_pMenuPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pMenuPanel->SetPos(0, iPanelMargin);
	m_pMenuPanel->SetVisible(false);

	// menuImagePanel
	m_pMenuImagePanel = new ImagePanel(m_pMenuPanel, "menuImagePanel");
	m_pMenuImagePanel->DisableMouseInputForThisPanel(true);
	m_pMenuImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pMenuImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pMenuImagePanel->SetPos(iPanelMargin, 0);

	// menuImageLabel
	std::string menuImageLabelText = "Esc";
	m_pMenuImageLabel = new Label(m_pMenuPanel, "menuImageLabel", menuImageLabelText.c_str());
	m_pMenuImageLabel->DisableMouseInputForThisPanel(true);
	m_pMenuImageLabel->SetFont(hFont);
	m_pMenuImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pMenuImageLabel->SetContentAlignment(Label::a_center);
	m_pMenuImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pMenuImageLabel->SetSize(63, 47);

	// menuLabel
	m_pMenuLabel = new Label(m_pMenuPanel, "menuLabel", "-");
	m_pMenuLabel->DisableMouseInputForThisPanel(true);
	m_pMenuLabel->SetFont(hFontMedium);
	m_pMenuLabel->SetContentAlignment(Label::a_center);
	m_pMenuLabel->SetPaintBackgroundEnabled(true);
	m_pMenuLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pMenuLabel->SetTextInset(iPadding, 0);

	//=================
	// SELECT
	//=================

	// selectPanel
	m_pSelectPanel = new Panel(this->GetPanel(), "selectPanel");
	m_pSelectPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pSelectPanel);
	m_pSelectPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pSelectPanel->SetPos(0, ScreenHeight() - (iPanelMargin * 2.0) - iActualButtonHeight);
	m_pSelectPanel->SetVisible(false);

	// selectImagePanel
	m_pSelectImagePanel = new ImagePanel(m_pSelectPanel, "selectImagePanel");
	m_pSelectImagePanel->DisableMouseInputForThisPanel(true);
	m_pSelectImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pSelectImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pSelectImagePanel->SetPos(iPanelMargin, 0);

	// selectImageLabel
	std::string selectImageLabelText = "LMB";
	m_pSelectImageLabel = new Label(m_pSelectPanel, "selectImageLabel", selectImageLabelText.c_str());
	m_pSelectImageLabel->DisableMouseInputForThisPanel(true);
	m_pSelectImageLabel->SetFont(hFont);
	m_pSelectImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pSelectImageLabel->SetContentAlignment(Label::a_center);
	m_pSelectImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pSelectImageLabel->SetSize(63, 47);

	// selectLabel
	m_pSelectLabel = new Label(m_pSelectPanel, "selectLabel", "-");
	m_pSelectLabel->DisableMouseInputForThisPanel(true);
	m_pSelectLabel->SetFont(hFontMedium);
	m_pSelectLabel->SetContentAlignment(Label::a_center);
	m_pSelectLabel->SetPaintBackgroundEnabled(true);
	m_pSelectLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pSelectLabel->SetTextInset(iPadding, 0);

	//=================
	// MOUSEMODE
	//=================

	// mouseModePanel
	m_pMouseModePanel = new Panel(this->GetPanel(), "mouseModePanel");
	m_pMouseModePanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pMouseModePanel);
	m_pMouseModePanel->SetSize(iPanelWidth, iPanelHeight);
	m_pMouseModePanel->SetPos(ScreenWidth() - (iPanelMargin * 2.0) - iActualButtonWidth, ScreenHeight() - (iPanelMargin * 2.0) - iActualButtonHeight);
	m_pMouseModePanel->SetVisible(false);

	// mouseModeImagePanel
	m_pMouseModeImagePanel = new ImagePanel(m_pMouseModePanel, "mouseModeImagePanel");
	m_pMouseModeImagePanel->DisableMouseInputForThisPanel(true);
	m_pMouseModeImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pMouseModeImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pMouseModeImagePanel->SetPos(iPanelMargin, 0);

	// mouseModeImageLabel
	std::string mouseModeImageLabelText = "RMB";
	m_pMouseModeImageLabel = new Label(m_pMouseModePanel, "mouseModeImageLabel", mouseModeImageLabelText.c_str());
	m_pMouseModeImageLabel->DisableMouseInputForThisPanel(true);
	m_pMouseModeImageLabel->SetFont(hFont);
	m_pMouseModeImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pMouseModeImageLabel->SetContentAlignment(Label::a_center);
	m_pMouseModeImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pMouseModeImageLabel->SetSize(63, 47);

	// mouseModeLabel
	m_pMouseModeLabel = new Label(m_pMouseModePanel, "mouseModeLabel", "-");
	m_pMouseModeLabel->DisableMouseInputForThisPanel(true);
	m_pMouseModeLabel->SetFont(hFontMedium);
	m_pMouseModeLabel->SetContentAlignment(Label::a_center);
	m_pMouseModeLabel->SetPaintBackgroundEnabled(true);
	m_pMouseModeLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pMouseModeLabel->SetTextInset(iPadding, 0);

	//=================
	// BUILD
	//=================

	// buildPanel
	m_pBuildPanel = new Panel(this->GetPanel(), "buildPanel");
	m_pBuildPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pBuildPanel);
	m_pBuildPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pBuildPanel->SetPos((ScreenWidth() / 2.0) - iPanelMargin - (iActualButtonWidth / 2.0), ScreenHeight() - (iPanelMargin * 2.0) - iActualButtonHeight);
	m_pBuildPanel->SetVisible(false);

	// buildImagePanel
	m_pBuildImagePanel = new ImagePanel(m_pBuildPanel, "buildImagePanel");
	m_pBuildImagePanel->DisableMouseInputForThisPanel(true);
	m_pBuildImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pBuildImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pBuildImagePanel->SetPos(iPanelMargin, 0);

	// buildImageLabel
	std::string buildImageLabelText = "MMB";
	m_pBuildImageLabel = new Label(m_pBuildPanel, "buildImageLabel", buildImageLabelText.c_str());
	m_pBuildImageLabel->DisableMouseInputForThisPanel(true);
	m_pBuildImageLabel->SetFont(hFont);
	m_pBuildImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pBuildImageLabel->SetContentAlignment(Label::a_center);
	m_pBuildImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pBuildImageLabel->SetSize(63, 47);

	// buildLabel
	m_pBuildLabel = new Label(m_pBuildPanel, "buildLabel", "-");
	m_pBuildLabel->DisableMouseInputForThisPanel(true);
	m_pBuildLabel->SetFont(hFontMedium);
	m_pBuildLabel->SetContentAlignment(Label::a_center);
	m_pBuildLabel->SetPaintBackgroundEnabled(true);
	m_pBuildLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pBuildLabel->SetTextInset(iPadding, 0);

	//=================
	// CONTINUOUS
	//=================

	// continuousPanel
	m_pContinuousPanel = new Panel(this->GetPanel(), "playPanel");
	m_pContinuousPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pContinuousPanel);
	m_pContinuousPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pContinuousPanel->SetPos(ScreenWidth() - (iPanelMargin * 2.0) - iActualButtonWidth, iPanelMargin);
	m_pContinuousPanel->SetVisible(false);

	// continuousImagePanel
	m_pContinuousImagePanel = new ImagePanel(m_pContinuousPanel, "continuousImagePanel");
	m_pContinuousImagePanel->DisableMouseInputForThisPanel(true);
	m_pContinuousImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pContinuousImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pContinuousImagePanel->SetPos(iPanelMargin, 0);

	// continuousImageLabel
	std::string continuousImageLabelText = "R";
	m_pContinuousImageLabel = new Label(m_pContinuousPanel, "continuousImageLabel", continuousImageLabelText.c_str());
	m_pContinuousImageLabel->DisableMouseInputForThisPanel(true);
	m_pContinuousImageLabel->SetFont(hFont);
	m_pContinuousImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pContinuousImageLabel->SetContentAlignment(Label::a_center);
	m_pContinuousImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pContinuousImageLabel->SetSize(63, 47);

	// continuousLabel
	m_pContinuousLabel = new Label(m_pContinuousPanel, "continuousLabel", "-");
	m_pContinuousLabel->DisableMouseInputForThisPanel(true);
	m_pContinuousLabel->SetFont(hFontMedium);
	m_pContinuousLabel->SetContentAlignment(Label::a_center);
	m_pContinuousLabel->SetPaintBackgroundEnabled(true);
	m_pContinuousLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pContinuousLabel->SetTextInset(iPadding, 0);

	//=================
	// PREVIOUS
	//=================

	// previousPanel
	m_pPreviousPanel = new Panel(this->GetPanel(), "previousPanel");
	m_pPreviousPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pPreviousPanel);
	m_pPreviousPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pPreviousPanel->SetPos((ScreenWidth() / 2.0) - iPanelMargin - (iActualButtonWidth / 2.0) - (ScreenWidth() / 8), ScreenHeight() - (iPanelMargin * 2.0) - iActualButtonHeight);
	m_pPreviousPanel->SetVisible(false);

	// previousImagePanel
	m_pPreviousImagePanel = new ImagePanel(m_pPreviousPanel, "previousImagePanel");
	m_pPreviousImagePanel->DisableMouseInputForThisPanel(true);
	m_pPreviousImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pPreviousImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pPreviousImagePanel->SetPos(iPanelMargin, 0);

	// previousImageLabel
	std::string previousImageLabelText = "MWD";
	m_pPreviousImageLabel = new Label(m_pPreviousPanel, "previousImageLabel", previousImageLabelText.c_str());
	m_pPreviousImageLabel->DisableMouseInputForThisPanel(true);
	m_pPreviousImageLabel->SetFont(hFont);
	m_pPreviousImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pPreviousImageLabel->SetContentAlignment(Label::a_center);
	m_pPreviousImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pPreviousImageLabel->SetSize(63, 47);

	// previousLabel
	m_pPreviousLabel = new Label(m_pPreviousPanel, "previousLabel", "-");
	m_pPreviousLabel->DisableMouseInputForThisPanel(true);
	m_pPreviousLabel->SetFont(hFontMedium);
	m_pPreviousLabel->SetContentAlignment(Label::a_center);
	m_pPreviousLabel->SetPaintBackgroundEnabled(true);
	m_pPreviousLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pPreviousLabel->SetTextInset(iPadding, 0);

	//=================
	// NEXT
	//=================

	// nextPanel
	m_pNextPanel = new Panel(this->GetPanel(), "nextPanel");
	m_pNextPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pNextPanel);
	m_pNextPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pNextPanel->SetPos((ScreenWidth() / 2.0) - iPanelMargin - (iActualButtonWidth / 2.0) + (ScreenWidth() / 8), ScreenHeight() - (iPanelMargin * 2.0) - iActualButtonHeight);
	m_pNextPanel->SetVisible(false);

	// nextImagePanel
	m_pNextImagePanel = new ImagePanel(m_pNextPanel, "nextImagePanel");
	m_pNextImagePanel->DisableMouseInputForThisPanel(true);
	m_pNextImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pNextImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pNextImagePanel->SetPos(iPanelMargin, 0);

	// nextImageLabel
	std::string nextImageLabelText = "MWU";
	m_pNextImageLabel = new Label(m_pNextPanel, "nextImageLabel", nextImageLabelText.c_str());
	m_pNextImageLabel->DisableMouseInputForThisPanel(true);
	m_pNextImageLabel->SetFont(hFont);
	m_pNextImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pNextImageLabel->SetContentAlignment(Label::a_center);
	m_pNextImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pNextImageLabel->SetSize(63, 47);

	// nextLabel
	m_pNextLabel = new Label(m_pNextPanel, "nextLabel", "-");
	m_pNextLabel->DisableMouseInputForThisPanel(true);
	m_pNextLabel->SetFont(hFontMedium);
	m_pNextLabel->SetContentAlignment(Label::a_center);
	m_pNextLabel->SetPaintBackgroundEnabled(true);
	m_pNextLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pNextLabel->SetTextInset(iPadding, 0);

	//=================
	// TABS
	//=================

	// tabsPanel
	m_pTabsPanel = new Panel(this->GetPanel(), "tabsPanel");
	m_pTabsPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pTabsPanel);
	m_pTabsPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pTabsPanel->SetPos((ScreenWidth() / 2.0) - iPanelMargin - (iActualButtonWidth / 2.0), iPanelMargin);
	m_pTabsPanel->SetVisible(false);

	// tabsImagePanel
	m_pTabsImagePanel = new ImagePanel(m_pTabsPanel, "tabsImagePanel");
	m_pTabsImagePanel->DisableMouseInputForThisPanel(true);
	m_pTabsImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pTabsImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pTabsImagePanel->SetPos(iPanelMargin, 0);

	// tabsImageLabel
	std::string tabsImageLabelText = "Tab";
	m_pTabsImageLabel = new Label(m_pTabsPanel, "tabsImageLabel", tabsImageLabelText.c_str());
	m_pTabsImageLabel->DisableMouseInputForThisPanel(true);
	m_pTabsImageLabel->SetFont(hFont);
	m_pTabsImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pTabsImageLabel->SetContentAlignment(Label::a_center);
	m_pTabsImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pTabsImageLabel->SetSize(63, 47);

	// tabsLabel
	m_pTabsLabel = new Label(m_pTabsPanel, "tabsLabel", "-");
	m_pTabsLabel->DisableMouseInputForThisPanel(true);
	m_pTabsLabel->SetFont(hFontMedium);
	m_pTabsLabel->SetContentAlignment(Label::a_center);
	m_pTabsLabel->SetPaintBackgroundEnabled(true);
	m_pTabsLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pTabsLabel->SetTextInset(iPadding, 0);

	//=================
	// HUD
	//=================

	// hudPanel
	m_pHudPanel = new Panel(this->GetPanel(), "hudPanel");
	m_pHudPanel->DisableMouseInputForThisPanel(true);
	HideAllChildren(m_pHudPanel);
	m_pHudPanel->SetSize(iPanelWidth, iPanelHeight);
	m_pHudPanel->SetPos(ScreenWidth()/16, iPanelMargin);
	m_pHudPanel->SetVisible(false);

	// hudImagePanel
	m_pHudImagePanel = new ImagePanel(m_pHudPanel, "hudImagePanel");
	m_pHudImagePanel->DisableMouseInputForThisPanel(true);
	m_pHudImagePanel->SetImage(scheme()->GetImage("aahudbutton", false));
	m_pHudImagePanel->SetSize(iButtonImageWidth, iButtonImageHeight);
	m_pHudImagePanel->SetPos(iPanelMargin, 0);

	// hudImageLabel
	std::string hudImageLabelText = "F1";
	m_pHudImageLabel = new Label(m_pHudPanel, "hudImageLabel", hudImageLabelText.c_str());
	m_pHudImageLabel->DisableMouseInputForThisPanel(true);
	m_pHudImageLabel->SetFont(hFont);
	m_pHudImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pHudImageLabel->SetContentAlignment(Label::a_center);
	m_pHudImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetY);
	m_pHudImageLabel->SetSize(63, 47);

	// hudLabel
	m_pHudLabel = new Label(m_pHudPanel, "hudLabel", "-");
	m_pHudLabel->DisableMouseInputForThisPanel(true);
	m_pHudLabel->SetFont(hFontMedium);
	m_pHudLabel->SetContentAlignment(Label::a_center);
	m_pHudLabel->SetPaintBackgroundEnabled(true);
	m_pHudLabel->SetBgColor(Color(0, 0, 0, 220));
	m_pHudLabel->SetTextInset(iPadding, 0);

	SetVisible(true);
	//Activate();
}

void CToastSlate::ManageHUDLabels()
{
	bool bIsReady = g_pAnarchyManager->IsInitialized();// (g_pAnarchyManager->GetState() == AASTATE_RUN);
	if (!bIsReady)
		return;

	// SHIT SHOW MODE!!
	int iShouldShowMode = m_pShowArcadeHudConVar->GetInt();
	if (m_iShowMode != iShouldShowMode)
	{
		if (iShouldShowMode == 0)
		{
			bool bVal = false;
			m_pMenuPanel->SetVisible(bVal);
			m_pSelectPanel->SetVisible(bVal);
			m_pMouseModePanel->SetVisible(bVal);
			m_pHudPanel->SetVisible(bVal);
			m_pBuildPanel->SetVisible(bVal);
			m_pContinuousPanel->SetVisible(bVal);
			m_pPreviousPanel->SetVisible(bVal);
			m_pNextPanel->SetVisible(bVal);
			m_pTabsPanel->SetVisible(bVal);
		}
		else if (iShouldShowMode == 1)
		{
			bool bVal = true;
			m_lastMenuState = AAHUDBUTTONLABEL_NONE;
			m_lastSelectState = AAHUDBUTTONLABEL_NONE;
			m_lastMouseModeState = AAHUDBUTTONLABEL_NONE;
			m_lastHudState = AAHUDBUTTONLABEL_NONE;
			m_lastBuildState = AAHUDBUTTONLABEL_NONE;
			m_lastContinuousState = AAHUDBUTTONLABEL_NONE;
			//m_lastPreviousState = AAHUDBUTTONLABEL_NONE;
			//m_lastNextState = AAHUDBUTTONLABEL_NONE;
			m_lastTabsState = AAHUDBUTTONLABEL_NONE;
		}
		else
		{
			// possible 3rd hud state (such as hiding non-dynamic buttons)
		}
		m_iShowMode = iShouldShowMode;
	}
	

	if (m_iShowMode == 0)	// FIXME: Need better logic when more than just 2 modes become available!
		return;

	bool bIsMapLoaded = (g_pAnarchyManager->MapName());
	bool bIsPaused = g_pAnarchyManager->IsPaused();
	bool bIsSpawnMode = (bool)g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject();
	bool bIsInputMode = g_pAnarchyManager->GetInputManager()->GetInputMode();
	//bool bIsGuest = (g_pAnarchyManager->GetConnectedUniverse() && !g_pAnarchyManager->GetConnectedUniverse()->isHost);
	bool bIsLookingAtObject = g_pAnarchyManager->GetHoverEntityShortcut();
	bool bIsObjectSelected = g_pAnarchyManager->GetSelectedEntity();
	
	// get all the tasks
	std::vector<C_EmbeddedInstance*> embeddedInstances;
	g_pAnarchyManager->GetLibretroManager()->GetAllInstances(embeddedInstances);
	g_pAnarchyManager->GetSteamBrowserManager()->GetAllInstances(embeddedInstances);
	//g_pAnarchyManager->GetAwesomiumBrowserManager()->GetAllInstances(embeddedInstances); // don't count Awesomium instances, for now.

	m_iNumTasksOpen = embeddedInstances.size();

	bool bIsSelectedObject = false;

	C_PropShortcutEntity* pHoveredShortcut = (bIsLookingAtObject) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetHoverEntityIndex())) : null;
	if (bIsObjectSelected && bIsLookingAtObject)
	{
		C_PropShortcutEntity* pSelectedShortcut = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
		if (pHoveredShortcut && pHoveredShortcut == pSelectedShortcut)
			bIsSelectedObject = true;
	}

	bool bIsLookingAtActiveObject = (bIsLookingAtObject && (g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstanceByEntityIndex(g_pAnarchyManager->GetHoverEntityIndex()) || g_pAnarchyManager->GetLibretroManager()->FindLibretroInstanceByEntityIndex(g_pAnarchyManager->GetHoverEntityIndex()) || g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstanceByEntityIndex(g_pAnarchyManager->GetHoverEntityIndex())));

	bool bIsLookingAtConinuousPlayObject = (bIsLookingAtActiveObject && g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance()->GetOriginalEntIndex() == g_pAnarchyManager->GetHoverEntityIndex());

	// MENU (Esc)
	if (!bIsMapLoaded || bIsInputMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MENU_NONE);
	else if (bIsPaused)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MENU_RESUME);
	else if (bIsSpawnMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MENU_CANCEL);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MENU_MENU);

	// SELECT (LMB)
	if (!bIsMapLoaded || bIsPaused)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_NONE);
	else if (bIsInputMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_CLICK);
	else if (bIsSpawnMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_CONFIRM);
	else if (bIsSelectedObject)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_FULLSCREEN);
	else if (bIsLookingAtObject)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_SELECT);
	else if (bIsObjectSelected)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_DESELECT);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_SELECT_NONE);

	// MOUSEMODE (RMB)
	if (bIsPaused || !bIsMapLoaded || bIsInputMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MOUSEMODE_NONE);
	else if (bIsSpawnMode || !bIsLookingAtActiveObject)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MOUSEMODE_NONE);	// disabled Mouse Mode* for simplicity.
	else if ( bIsLookingAtObject)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MOUSEMODE_OBJECT);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_MOUSEMODE_NONE);

	// HUD (F1)
	if (!bIsMapLoaded || !bIsInputMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_HUD_HUD);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_HUD_NONE);

	// BUILD (MMB)
	if (!bIsMapLoaded || bIsPaused || bIsInputMode || bIsSpawnMode)//|| (bIsGuest && !bIsLookingAtObject))
		this->SetHUDLabelText(AAHUDBUTTONLABEL_BUILD_NONE);
	else if (bIsLookingAtObject)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_BUILD_EDIT);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_BUILD_LIBRARY);

	// CONTINUOUS (R)
	if (!bIsMapLoaded || bIsInputMode || (!bIsObjectSelected && (bIsPaused || bIsSpawnMode || !bIsLookingAtObject || bIsLookingAtConinuousPlayObject)))
		this->SetHUDLabelText(AAHUDBUTTONLABEL_CONTINUOUS_NONE);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_CONTINUOUS_PLAY);

	// PREVIOUS (MWD)
	if ( bIsSpawnMode )
		this->SetHUDLabelText(AAHUDBUTTONLABEL_PREVIOUS_PREVIOUS);
	else if (bIsInputMode && bIsMapLoaded)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_PREVIOUS_DOWN);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_PREVIOUS_NONE);

	// NEXT (MWU)
	if (bIsSpawnMode)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_NEXT_NEXT);
	else if (bIsInputMode && bIsMapLoaded)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_NEXT_UP);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_NEXT_NONE);

	// TABS (Tab)
	if (!bIsMapLoaded || bIsPaused || bIsInputMode || bIsSpawnMode || m_iNumTasksOpen == 0)
		this->SetHUDLabelText(AAHUDBUTTONLABEL_TABS_NONE);
	else
		this->SetHUDLabelText(AAHUDBUTTONLABEL_TABS_TABS);
}

void CToastSlate::SetHUDLabelText(aaHUDButtonState state)
{
	int iPanelMargin = 24;
	int iButtonOffsetX = 6;
	int iButtonOffsetY = 4;
	int iPadding = 8;
	int iActualButtonWidth = 76;
	int iButtonImageHeight = 64;// NOTE: this value, and the many above it, should be a member variable because it's also used in the constructor

	bool bNeedsTextUpdate = false;

	bool bIsMenuLabel = false;
	bool bIsSelectLabel = false;
	bool bIsMouseModeLabel = false;
	bool bIsHudLabel = false;
	bool bIsBuildLabel = false;
	bool bIsContinuousLabel = false;
	bool bIsPreviousLabel = false;
	bool bIsNextLabel = false;
	bool bIsTabsLabel = false;

	std::string text;
	Panel* pPanel;
	Label* pLabel;
	Label* pImageLabel;
	if (state == AAHUDBUTTONLABEL_MENU_NONE)
	{
		if (m_lastMenuState != AAHUDBUTTONLABEL_MENU_NONE)
		{
			bNeedsTextUpdate = true;
			bIsMenuLabel = true;

			text = "-";
			m_lastMenuState = AAHUDBUTTONLABEL_MENU_NONE;
			pPanel = m_pMenuPanel;
			pLabel = m_pMenuLabel;
			pImageLabel = m_pMenuImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MENU_MENU)
	{
		if (m_lastMenuState != AAHUDBUTTONLABEL_MENU_MENU)
		{
			bNeedsTextUpdate = true;
			bIsMenuLabel = true;

			text = "Menu";
			m_lastMenuState = AAHUDBUTTONLABEL_MENU_MENU;
			pPanel = m_pMenuPanel;
			pLabel = m_pMenuLabel;
			pImageLabel = m_pMenuImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MENU_CANCEL)
	{
		if (m_lastMenuState != AAHUDBUTTONLABEL_MENU_CANCEL)
		{
			bNeedsTextUpdate = true;
			bIsMenuLabel = true;

			text = "Cancel";
			m_lastMenuState = AAHUDBUTTONLABEL_MENU_CANCEL;
			pPanel = m_pMenuPanel;
			pLabel = m_pMenuLabel;
			pImageLabel = m_pMenuImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MENU_RESUME)
	{
		if (m_lastMenuState != AAHUDBUTTONLABEL_MENU_RESUME)
		{
			bNeedsTextUpdate = true;
			bIsMenuLabel = true;

			text = "Resume";
			m_lastMenuState = AAHUDBUTTONLABEL_MENU_RESUME;
			pPanel = m_pMenuPanel;
			pLabel = m_pMenuLabel;
			pImageLabel = m_pMenuImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_NONE)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_NONE)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "-";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_NONE;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_CLICK)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_CLICK)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "Click";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_CLICK;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_CONFIRM)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_CONFIRM)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "Confirm";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_CONFIRM;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_FULLSCREEN)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_FULLSCREEN)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "Fullscreen";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_FULLSCREEN;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_SELECT)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_SELECT)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "Select";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_SELECT;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_SELECT_DESELECT)
	{
		if (m_lastSelectState != AAHUDBUTTONLABEL_SELECT_DESELECT)
		{
			bNeedsTextUpdate = true;
			bIsSelectLabel = true;

			text = "Deselect";
			m_lastSelectState = AAHUDBUTTONLABEL_SELECT_DESELECT;
			pPanel = m_pSelectPanel;
			pLabel = m_pSelectLabel;
			pImageLabel = m_pSelectImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MOUSEMODE_NONE)
	{
		if (m_lastMouseModeState != AAHUDBUTTONLABEL_MOUSEMODE_NONE)
		{
			bNeedsTextUpdate = true;
			bIsMouseModeLabel = true;

			text = "-";
			m_lastMouseModeState = AAHUDBUTTONLABEL_MOUSEMODE_NONE;
			pPanel = m_pMouseModePanel;
			pLabel = m_pMouseModeLabel;
			pImageLabel = m_pMouseModeImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MOUSEMODE_MODE)
	{
		if (m_lastMouseModeState != AAHUDBUTTONLABEL_MOUSEMODE_MODE)
		{
			bNeedsTextUpdate = true;
			bIsMouseModeLabel = true;

			text = "Mouse Mode*";
			m_lastMouseModeState = AAHUDBUTTONLABEL_MOUSEMODE_MODE;
			pPanel = m_pMouseModePanel;
			pLabel = m_pMouseModeLabel;
			pImageLabel = m_pMouseModeImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_MOUSEMODE_OBJECT)
	{
		if (m_lastMouseModeState != AAHUDBUTTONLABEL_MOUSEMODE_OBJECT)
		{
			bNeedsTextUpdate = true;
			bIsMouseModeLabel = true;

			text = "Mouse Mode (hold)";
			m_lastMouseModeState = AAHUDBUTTONLABEL_MOUSEMODE_OBJECT;
			pPanel = m_pMouseModePanel;
			pLabel = m_pMouseModeLabel;
			pImageLabel = m_pMouseModeImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_BUILD_NONE)
	{
		if (m_lastBuildState != AAHUDBUTTONLABEL_BUILD_NONE)
		{
			bNeedsTextUpdate = true;
			bIsBuildLabel = true;

			text = "-";
			m_lastBuildState = AAHUDBUTTONLABEL_BUILD_NONE;
			pPanel = m_pBuildPanel;
			pLabel = m_pBuildLabel;
			pImageLabel = m_pBuildImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_BUILD_EDIT)
	{
		if (m_lastBuildState != AAHUDBUTTONLABEL_BUILD_EDIT)
		{
			bNeedsTextUpdate = true;
			bIsBuildLabel = true;

			text = "Info";
			m_lastBuildState = AAHUDBUTTONLABEL_BUILD_EDIT;
			pPanel = m_pBuildPanel;
			pLabel = m_pBuildLabel;
			pImageLabel = m_pBuildImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_BUILD_LIBRARY)
	{
		if (m_lastBuildState != AAHUDBUTTONLABEL_BUILD_LIBRARY)
		{
			bNeedsTextUpdate = true;
			bIsBuildLabel = true;

			text = "Library";
			m_lastBuildState = AAHUDBUTTONLABEL_BUILD_LIBRARY;
			pPanel = m_pBuildPanel;
			pLabel = m_pBuildLabel;
			pImageLabel = m_pBuildImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_CONTINUOUS_NONE)
	{
		if (m_lastContinuousState != AAHUDBUTTONLABEL_CONTINUOUS_NONE)
		{
			bNeedsTextUpdate = true;
			bIsContinuousLabel = true;

			text = "-";
			m_lastContinuousState = AAHUDBUTTONLABEL_CONTINUOUS_NONE;
			pPanel = m_pContinuousPanel;
			pLabel = m_pContinuousLabel;
			pImageLabel = m_pContinuousImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_CONTINUOUS_PLAY)
	{
		if (m_lastContinuousState != AAHUDBUTTONLABEL_CONTINUOUS_PLAY)
		{
			bNeedsTextUpdate = true;
			bIsContinuousLabel = true;

			text = "Autoplay";
			m_lastContinuousState = AAHUDBUTTONLABEL_CONTINUOUS_PLAY;
			pPanel = m_pContinuousPanel;
			pLabel = m_pContinuousLabel;
			pImageLabel = m_pContinuousImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_PREVIOUS_NONE)
	{
		if (m_lastPreviousState != AAHUDBUTTONLABEL_PREVIOUS_NONE)
		{
			bNeedsTextUpdate = true;
			bIsPreviousLabel = true;

			text = "-";
			m_lastPreviousState = AAHUDBUTTONLABEL_PREVIOUS_NONE;
			pPanel = m_pPreviousPanel;
			pLabel = m_pPreviousLabel;
			pImageLabel = m_pPreviousImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_PREVIOUS_PREVIOUS)
	{
		if (m_lastPreviousState != AAHUDBUTTONLABEL_PREVIOUS_PREVIOUS)
		{
			bNeedsTextUpdate = true;
			bIsPreviousLabel = true;

			text = "Previous";
			m_lastPreviousState = AAHUDBUTTONLABEL_PREVIOUS_PREVIOUS;
			pPanel = m_pPreviousPanel;
			pLabel = m_pPreviousLabel;
			pImageLabel = m_pPreviousImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_PREVIOUS_DOWN)
	{
		if (m_lastPreviousState != AAHUDBUTTONLABEL_PREVIOUS_DOWN)
		{
			bNeedsTextUpdate = true;
			bIsPreviousLabel = true;

			text = "Scroll Down";
			m_lastPreviousState = AAHUDBUTTONLABEL_PREVIOUS_DOWN;
			pPanel = m_pPreviousPanel;
			pLabel = m_pPreviousLabel;
			pImageLabel = m_pPreviousImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_NEXT_UP)
	{
		if (m_lastNextState != AAHUDBUTTONLABEL_NEXT_UP)
		{
			bNeedsTextUpdate = true;
			bIsNextLabel = true;

			text = "Scroll Up";
			m_lastNextState = AAHUDBUTTONLABEL_NEXT_UP;
			pPanel = m_pNextPanel;
			pLabel = m_pNextLabel;
			pImageLabel = m_pNextImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_NEXT_NEXT)
	{
		if (m_lastNextState != AAHUDBUTTONLABEL_NEXT_NEXT)
		{
			bNeedsTextUpdate = true;
			bIsNextLabel = true;

			text = "Next";
			m_lastNextState = AAHUDBUTTONLABEL_NEXT_NEXT;
			pPanel = m_pNextPanel;
			pLabel = m_pNextLabel;
			pImageLabel = m_pNextImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_NEXT_NONE)
	{
		if (m_lastNextState != AAHUDBUTTONLABEL_NEXT_NONE)
		{
			bNeedsTextUpdate = true;
			bIsNextLabel = true;

			text = "-";
			m_lastNextState = AAHUDBUTTONLABEL_NEXT_NONE;
			pPanel = m_pNextPanel;
			pLabel = m_pNextLabel;
			pImageLabel = m_pNextImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_TABS_NONE)
	{
		if (m_lastTabsState != AAHUDBUTTONLABEL_TABS_NONE)
		{
			bNeedsTextUpdate = true;
			bIsTabsLabel = true;

			text = "-";
			m_lastTabsState = AAHUDBUTTONLABEL_TABS_NONE;
			pPanel = m_pTabsPanel;
			pLabel = m_pTabsLabel;
			pImageLabel = m_pTabsImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_TABS_TABS)
	{
		if (m_lastTabsState != AAHUDBUTTONLABEL_TABS_TABS || m_iOldNumTasksOpen != m_iNumTasksOpen)
		{
			m_iOldNumTasksOpen = m_iNumTasksOpen;
			bNeedsTextUpdate = true;
			bIsTabsLabel = true;

			text = VarArgs("Tabs (%i)", m_iNumTasksOpen);
			m_lastTabsState = AAHUDBUTTONLABEL_TABS_TABS;
			pPanel = m_pTabsPanel;
			pLabel = m_pTabsLabel;
			pImageLabel = m_pTabsImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_HUD_NONE)
	{
		if (m_lastHudState != AAHUDBUTTONLABEL_HUD_NONE)
		{
			bNeedsTextUpdate = true;
			bIsHudLabel = true;

			text = "-";
			m_lastHudState = AAHUDBUTTONLABEL_HUD_NONE;
			pPanel = m_pHudPanel;
			pLabel = m_pHudLabel;
			pImageLabel = m_pHudImageLabel;
		}
	}
	else if (state == AAHUDBUTTONLABEL_HUD_HUD)
	{
		if (m_lastHudState != AAHUDBUTTONLABEL_HUD_HUD)
		{
			bNeedsTextUpdate = true;
			bIsHudLabel = true;

			text = "HUD";
			m_lastHudState = AAHUDBUTTONLABEL_HUD_HUD;
			pPanel = m_pHudPanel;
			pLabel = m_pHudLabel;
			pImageLabel = m_pHudImageLabel;
		}
	}

	if (bNeedsTextUpdate)
	{
		pLabel->SetText(text.c_str());

		int iContentWidth, iContentHeight;
		pLabel->GetContentSize(iContentWidth, iContentHeight);

		int iLabelWidth = iContentWidth + (iPadding * 2.0);
		int iLabelHeight = iContentHeight + (iPadding * 1.0);

		pLabel->SetSize(iLabelWidth, iLabelHeight);

		int iHeight = (bIsSelectLabel || bIsMouseModeLabel || bIsBuildLabel || bIsPreviousLabel || bIsNextLabel) ? 0 : iButtonImageHeight;
		pLabel->SetPos(iPanelMargin + (iActualButtonWidth / 2) - (iLabelWidth / 2), iHeight);

		if (bIsSelectLabel)
		{
			m_pSelectImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pSelectImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		else if (bIsMouseModeLabel)
		{
			m_pMouseModeImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pMouseModeImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		/*else if (bIsHudLabel)
		{
			m_pHudImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pHudImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}*/
		else if (bIsBuildLabel)
		{
			m_pBuildImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pBuildImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		else if (bIsPreviousLabel)
		{
			m_pPreviousImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pPreviousImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		else if (bIsNextLabel)
		{
			m_pNextImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pNextImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		/*
		else if (bIsContinuousLabel)
		{
			m_pContinuousImagePanel->SetPos(iPanelMargin, iLabelHeight + iPadding);
			m_pContinuousImageLabel->SetPos(iPanelMargin + iButtonOffsetX, iButtonOffsetX + iLabelHeight + iPadding);
		}
		*/

		if (text == "-" || bIsNextLabel || bIsPreviousLabel)
			pPanel->SetVisible(false);
		else
			pPanel->SetVisible(true);
	}
}

void CToastSlate::PaintBackground()
{
	//m_pInitializingPanel->SetBgColor(Color(0, 0, 0, 255));

	if (g_pAnarchyManager->GetToastText() != "")
		m_pLabel->SetBgColor(Color(0, 0, 0, 220));
	else
		m_pLabel->SetBgColor(Color(0, 0, 0, 0));

	if (g_pAnarchyManager->GetHoverTitle() != "")
		m_pHoverLabel->SetBgColor(Color(0, 0, 0, 220));
	else
		m_pHoverLabel->SetBgColor(Color(0, 0, 0, 0));

	m_pMenuImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pMenuLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pSelectImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pSelectLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pMouseModeImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pMouseModeLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pHudImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pHudLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pBuildImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pBuildLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pContinuousImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pContinuousLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pPreviousImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pPreviousLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pNextImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pNextLabel->SetBgColor(Color(0, 0, 0, 220));

	m_pTabsImageLabel->SetFgColor(Color(30, 30, 30, 255));
	m_pTabsLabel->SetBgColor(Color(0, 0, 0, 220));
}

vgui::Panel* CToastSlate::GetPanel()
{
	return this;
}

void CToastSlate::Update()
{
}

void CToastSlate::OnMouseWheeled(int delta)
{
}

void CToastSlate::OnCursorMoved(int x, int y)
{
}

void CToastSlate::OnMouseDoublePressed(MouseCode code)
{
}

void CToastSlate::OnMousePressed(MouseCode code)
{
}

void CToastSlate::OnMouseReleased(MouseCode code)
{
}

void CToastSlate::OnKeyCodePressed(KeyCode code)
{
}

void CToastSlate::OnKeyCodeReleased(KeyCode code)
{
}

void CToastSlate::OnCommand(const char* pcCommand)
{
	if( !Q_stricmp(pcCommand, "Close") )
		BaseClass::OnCommand(pcCommand);
}

CToastSlate::~CToastSlate()
{
	DevMsg("Close toast slate\n");
	g_pAnarchyManager->RemoveToastLabel(m_pLabel);
}

class CToastSlateInterface : public IToastSlate
{
private:
	CToastSlate *ToastSlate;
public:
	CToastSlateInterface()
	{
		ToastSlate = NULL;
	}

	void Create(vgui::VPANEL parent)
	{
		ToastSlate = new CToastSlate(parent);
	}

	vgui::Panel* GetPanel()
	{
		return ToastSlate->GetPanel();
	}
	
	void Destroy()
	{
		if (ToastSlate)
		{
			ToastSlate->SetParent((vgui::Panel *)NULL);
			//ToastSlate->SelfDestruct();
			delete ToastSlate;
			ToastSlate = null;
		}
	}

	void Update()
	{
		if (ToastSlate)
			ToastSlate->Update();
	}

	void ManageHUDLabels()
	{
		if (ToastSlate)
			ToastSlate->ManageHUDLabels();
	}
};
static CToastSlateInterface g_ToastSlate;
IToastSlate* ToastSlate = (IToastSlate*)&g_ToastSlate;