#include "cbase.h"
#include <vgui_controls/Panel.h>

#include "c_arcadecrosshair.h"
#include "c_anarchymanager.h"

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

CArcadeCrosshair::CArcadeCrosshair(vgui::VPANEL parent) : Frame(null, "ArcadeCrosshair")
{
	// vgui constructor stuff
	SetParent( parent );
	SetProportional(false);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	//SetWide(ScreenWidth());
	//SetTall(ScreenHeight());
	//SetPaintBackgroundEnabled(true);
	SetPaintEnabled(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// Hide all children of this panel (including the invisible title bar that steals mouse move input
	for (int i = 0; i<GetChildCount(); i++)
	{
		Panel* pPanel = GetChild(i);
		pPanel->SetVisible(false);
	}

	this->SetScheme("ClientScheme.res");
	vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(this->GetScheme());
	vgui::HFont hFont = pScheme->GetFont("Trebuchet24");

	int colorBuf[4];
	colorBuf[0] = 200;
	colorBuf[1] = 200;
	colorBuf[2] = 200;
	colorBuf[3] = 150;

	UTIL_StringToIntArray(colorBuf, 4, cvar->FindVar("crosshair_color")->GetString());
	m_pColor = new Color(colorBuf[0], colorBuf[1], colorBuf[2], colorBuf[3]);

	int size = 20;

	m_pLabel = new Label(this, "crosshair", "");
	//m_pLabel->SetScheme("ClientScheme.res");
	m_pLabel->SetFont(hFont);

	m_pLabel->SetContentAlignment(vgui::Label::Alignment::a_center);
	m_pLabel->SetSize(size, size);
	m_pLabel->SetPos(0, 0);

	this->DisableMouseInputForThisPanel(true);
	this->SetWide(size);
	this->SetTall(size);
	this->SetPos(((ScreenWidth() / 2.0) - (size / 2.0)) * 1, ((ScreenHeight() / 2.0) - (size / 2.0)) * 1);

	m_pLabel->SetFgColor(*m_pColor);
	SetVisible(true);
}

void CArcadeCrosshair::Paint()
{
	m_pLabel->SetFgColor(*m_pColor);

	//if (g_pAnarchyManager->GetHoverEntityIndex() >= 0)
	if (g_pAnarchyManager->GetHoverEntityShortcut())
	{
		m_pLabel->SetPos(0, -1);
		m_pLabel->SetText("o");
	}
	else
	{
		m_pLabel->SetPos(0, -2);
		m_pLabel->SetText(".");
	}
}

void CArcadeCrosshair::PaintBackground()
{
	/*
	if (g_pAnarchyManager->ShouldShowCrosshair())
	{
		//this->SetBgColor(*m_pColor);

		if (g_pAnarchyManager->GetHoverTitle() != "")
			m_pLabel->SetText(".");
		else
			m_pLabel->SetText("o");
	}
	else
		m_pLabel->SetText("");
	//else
	//	this->SetBgColor(*m_pClearColor);
	*/
}

vgui::Panel* CArcadeCrosshair::GetPanel()
{
	return this;
}

void CArcadeCrosshair::Update()
{
}

void CArcadeCrosshair::OnMouseWheeled(int delta)
{
}

void CArcadeCrosshair::OnCursorMoved(int x, int y)
{
}

void CArcadeCrosshair::OnMouseDoublePressed(MouseCode code)
{
}

void CArcadeCrosshair::OnMousePressed(MouseCode code)
{
}

void CArcadeCrosshair::OnMouseReleased(MouseCode code)
{
}

void CArcadeCrosshair::OnKeyCodePressed(KeyCode code)
{
}

void CArcadeCrosshair::OnKeyCodeReleased(KeyCode code)
{
}

void CArcadeCrosshair::OnCommand(const char* pcCommand)
{
	if( !Q_stricmp(pcCommand, "Close") )
		BaseClass::OnCommand(pcCommand);
}

CArcadeCrosshair::~CArcadeCrosshair()
{
	delete m_pColor;
	//delete m_pClearColor;
	delete m_pLabel;
}

class CArcadeCrosshairInterface : public IArcadeCrosshair
{
private:
	CArcadeCrosshair *ArcadeCrosshair;
public:
	CArcadeCrosshairInterface()
	{
		ArcadeCrosshair = NULL;
	}

	void Create(vgui::VPANEL parent)
	{
		if (!ArcadeCrosshair)
			ArcadeCrosshair = new CArcadeCrosshair(parent);
	}

	vgui::Panel* GetPanel()
	{
		return ArcadeCrosshair->GetPanel();
	}
	
	void Destroy()
	{
		if (ArcadeCrosshair)
		{
			ArcadeCrosshair->SetParent((vgui::Panel *)NULL);
			delete ArcadeCrosshair;
			ArcadeCrosshair = null;
		}
	}

	void Update()
	{
		if (ArcadeCrosshair)
			ArcadeCrosshair->Update();
	}
};
static CArcadeCrosshairInterface g_ArcadeCrosshair;
IArcadeCrosshair* ArcadeCrosshair = (IArcadeCrosshair*)&g_ArcadeCrosshair;