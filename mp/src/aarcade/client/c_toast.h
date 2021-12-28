#ifndef TOAST_SLATE_H
#define TOAST_SLATE_H

class IToastSlate
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
	virtual vgui::Panel*		GetPanel() = 0;
	virtual void Update() = 0;
	virtual void ManageHUDLabels() = 0;
};

extern IToastSlate* ToastSlate;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include <string>

enum aaHUDButtonState
{
	AAHUDBUTTONLABEL_NONE = 0,
	AAHUDBUTTONLABEL_MENU_NONE = 1,
	AAHUDBUTTONLABEL_MENU_RESUME = 2,
	AAHUDBUTTONLABEL_MENU_CANCEL = 3,
	AAHUDBUTTONLABEL_MENU_MENU = 4,
	AAHUDBUTTONLABEL_SELECT_NONE = 5,
	AAHUDBUTTONLABEL_SELECT_CLICK = 6,
	AAHUDBUTTONLABEL_SELECT_CONFIRM = 7,
	AAHUDBUTTONLABEL_SELECT_FULLSCREEN = 8,
	AAHUDBUTTONLABEL_SELECT_SELECT = 9,
	AAHUDBUTTONLABEL_SELECT_DESELECT = 10,
	AAHUDBUTTONLABEL_MOUSEMODE_NONE = 11,
	AAHUDBUTTONLABEL_MOUSEMODE_MODE = 12,
	AAHUDBUTTONLABEL_MOUSEMODE_OBJECT = 13,
	AAHUDBUTTONLABEL_BUILD_NONE = 14,
	AAHUDBUTTONLABEL_BUILD_EDIT = 15,
	AAHUDBUTTONLABEL_BUILD_LIBRARY = 16,
	AAHUDBUTTONLABEL_CONTINUOUS_NONE = 17,
	AAHUDBUTTONLABEL_CONTINUOUS_PLAY = 18,
	AAHUDBUTTONLABEL_PREVIOUS_NONE = 19,
	AAHUDBUTTONLABEL_PREVIOUS_PREVIOUS = 20,
	AAHUDBUTTONLABEL_PREVIOUS_DOWN = 21,
	AAHUDBUTTONLABEL_NEXT_NONE = 22,
	AAHUDBUTTONLABEL_NEXT_NEXT = 23,
	AAHUDBUTTONLABEL_NEXT_UP = 24,
	AAHUDBUTTONLABEL_TABS_NONE = 25,
	AAHUDBUTTONLABEL_TABS_TABS = 26,
	AAHUDBUTTONLABEL_HUD_NONE = 27,
	AAHUDBUTTONLABEL_HUD_HUD = 28
};

namespace vgui
{
	class CToastSlate : public Frame
	{
		DECLARE_CLASS_SIMPLE(CToastSlate, Frame);

	public:
		CToastSlate(vgui::VPANEL parent);
		virtual ~CToastSlate();

		//void OnTick();
		void Update();
		void SelfDestruct();
		void PaintBackground();

		void OnCursorMoved(int x, int y);
		void OnMouseWheeled(int delta);
		void OnMousePressed(MouseCode code);
		void OnMouseReleased(MouseCode code);
		void OnMouseDoublePressed(MouseCode code);

		void OnKeyCodePressed(KeyCode code);
		void OnKeyCodeReleased(KeyCode code);

		vgui::Panel* GetPanel();
		void ManageHUDLabels();

	private:
		Panel* m_pInitializingPanel;

		int m_iShowMode;
		ConVar* m_pShowArcadeHudConVar;
		int m_iNumTasksOpen;
		int m_iOldNumTasksOpen;
		Label* m_pLabel;
		Label* m_pHoverLabel;

		Panel* m_pMenuPanel;
		Panel* m_pTabsPanel;
		Panel* m_pContinuousPanel;
		Panel* m_pSelectPanel;
		Panel* m_pPreviousPanel;
		Panel* m_pBuildPanel;
		Panel* m_pNextPanel;
		Panel* m_pMouseModePanel;
		Panel* m_pHudPanel;

		Label* m_pMenuImageLabel;
		Label* m_pTabsImageLabel;
		Label* m_pContinuousImageLabel;
		Label* m_pSelectImageLabel;
		Label* m_pPreviousImageLabel;
		Label* m_pBuildImageLabel;
		Label* m_pNextImageLabel;
		Label* m_pMouseModeImageLabel;
		Label* m_pHudImageLabel;

		Label* m_pMenuLabel;
		Label* m_pTabsLabel;
		Label* m_pContinuousLabel;
		Label* m_pSelectLabel;
		Label* m_pPreviousLabel;
		Label* m_pBuildLabel;
		Label* m_pNextLabel;
		Label* m_pMouseModeLabel;
		Label* m_pHudLabel;

		aaHUDButtonState m_lastMenuState;
		aaHUDButtonState m_lastTabsState;
		aaHUDButtonState m_lastContinuousState;
		aaHUDButtonState m_lastSelectState;
		aaHUDButtonState m_lastPreviousState;
		aaHUDButtonState m_lastBuildState;
		aaHUDButtonState m_lastNextState;
		aaHUDButtonState m_lastMouseModeState;
		aaHUDButtonState m_lastHudState;

		ImagePanel* m_pMenuImagePanel;
		ImagePanel* m_pTabsImagePanel;
		ImagePanel* m_pContinuousImagePanel;
		ImagePanel* m_pSelectImagePanel;
		ImagePanel* m_pPreviousImagePanel;
		ImagePanel* m_pBuildImagePanel;
		ImagePanel* m_pNextImagePanel;
		ImagePanel* m_pMouseModeImagePanel;
		ImagePanel* m_pHudImagePanel;

	protected:
		void OnCommand(const char* pcCommand);
		void SetHUDLabelText(aaHUDButtonState state);

	private:
	};
} // namespace vgui

#endif // TOAST_SLATE_H