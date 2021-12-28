#ifndef ARCADE_CROSSHAIR_H
#define ARCADE_CROSSHAIR_H

class IArcadeCrosshair
{
public:
	virtual void				Create(vgui::VPANEL parent) = 0;
	virtual void				Destroy(void) = 0;
	virtual vgui::Panel*		GetPanel() = 0;
	virtual void Update() = 0;
};

extern IArcadeCrosshair* ArcadeCrosshair;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>

namespace vgui
{
	class CArcadeCrosshair : public Frame
	{
		DECLARE_CLASS_SIMPLE(CArcadeCrosshair, Frame);

	public:
		CArcadeCrosshair(vgui::VPANEL parent);
		virtual ~CArcadeCrosshair();

		//void OnTick();
		void Update();
		void SelfDestruct();
		void PaintBackground();
		void Paint();

		void OnCursorMoved(int x, int y);
		void OnMouseWheeled(int delta);
		void OnMousePressed(MouseCode code);
		void OnMouseReleased(MouseCode code);
		void OnMouseDoublePressed(MouseCode code);

		void OnKeyCodePressed(KeyCode code);
		void OnKeyCodeReleased(KeyCode code);

		vgui::Panel* GetPanel();

	protected:
		void OnCommand(const char* pcCommand);
		Color* m_pColor;
		//Color* m_pClearColor;
		Label* m_pLabel;

	private:
	};
} // namespace vgui

#endif // ARCADE_CROSSHAIR_H