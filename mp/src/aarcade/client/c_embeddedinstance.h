#ifndef C_EMBEDDED_INSTANCE_H
#define C_EMBEDDED_INSTANCE_H

#include "c_inputlistener.h"
//#include "c_canvas.h"
#include <string>
#include <map>

class C_EmbeddedInstance
{
public:
	C_EmbeddedInstance();
	~C_EmbeddedInstance();
	// TODO: Make the rest of these pertinent methods pure virtual.  And un-virtual the ones that don't need to be plublic and do more BaseClase:: shit from the derived classes.

	virtual void OnProxyBind(C_BaseEntity* pBaseEntity) = 0;// { DevMsg("ERROR: Base method called!\n"); };
	virtual void Render() = 0;// { DevMsg("ERROR: Base method called!\n"); };
	virtual void RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect) = 0;// { DevMsg("ERROR: Base method called!\n"); };
	virtual bool IsSelected() = 0;// { return false; }
	virtual bool HasFocus() { return false; }	// This should be IsSelected!! (HasFocus can't always be determined, depending on the embedded instance.)
	virtual bool Focus() { return false; }
	virtual bool Blur() { return false; }
	virtual bool Select() { return false; }
	virtual bool Deselect() { return false; }
	virtual C_EmbeddedInstance* GetParentSelectedEmbeddedInstance() { return null; }
	virtual void Update() { DevMsg("ERROR: Base method called!\n"); };
	virtual void Close() {};
	virtual void TakeScreenshot(std::string nextTaskScreenshotName = "") {};
	virtual std::string GetURL() { return ""; }
	virtual void CleanUpTexture() {};

	std::string GetOutput(vgui::KeyCode code, bool bShift = false, bool bCtrl = false, bool bAlt = false, bool bWin = false, bool bAutorepeat = false);
	virtual void GetFullscreenInfo(float& fPositionX, float& fPositionY, float& fSizeX, float& fSizeY, std::string& overlayId);

	//void IncrementFrames();

	// mostly for libretro stuff
	virtual int GetLibretroStartSeconds() { return 0; }
	virtual int GetLibretroSeconds() { return 0; }
	virtual void OnSecondsUpdated() {}

	unsigned int GetFPS();
	void IncrementRenderedFrames();
	void IncrementSkippedFrames();

	// accessors
	float GetSamplerate() { return m_flSamplerate; }
	float GetFramerate() { return m_flFramerate; }
	virtual unsigned long GetCursor() { return m_cursor; }
	virtual bool IsDirty() { return false; }
	virtual std::string GetId();
	virtual std::string GetTitle();
	virtual int GetLastRenderedFrame() { return -1; }
	virtual int GetLastVisibleFrame() { return -1; }
	virtual ITexture* GetTexture() { return null; }
	virtual C_InputListener* GetInputListener() { return null; }
	virtual std::string GetOriginalItemId() { return ""; }
	virtual int GetOriginalEntIndex() { return -1; }
	virtual int GetNumSkippedFrames() { return m_uSkippedFrames; }
	virtual int GetNumFrames() { return m_uFrames; }
	virtual int GetNumRenderedFrames() { return m_uRenderedFrames; }
	virtual int GetNumFramesLastSecond() { return m_uFramesLastSecond; }
	virtual float GetLastFrameTime() { return m_flLastFrameTime; }
	virtual void GetLastMouse(float &fMouseX, float &fMouseY) { fMouseX = 0; fMouseY = 0; }
	
	// mutators
	void SetFramerate(float flRate) { m_flFramerate = flRate; }
	void SetSamplerate(float flRate) { m_flSamplerate = flRate; }
	virtual void SetCursor(unsigned long cursor) { m_cursor = cursor; }
	virtual void SetOriginalItemId(std::string itemId) {};
	virtual void SetOriginalEntIndex(int index) {};
	virtual void SetTitle(std::string title) {};	// FIXME: This shouldn't need to be pure-virtual because EVERYTHING that derrives from C_EmbededInstance uses this!
	
private:
	unsigned int m_flLastSavedSeconds;
	unsigned int m_uSkippedFrames;
	unsigned int m_uFrames;
	unsigned int m_uRenderedFrames;
	unsigned int m_uFramesLastSecond;
	unsigned int m_uFramesPerSecond;
	float m_flLastFrameTime;

	float m_flSamplerate;
	float m_flFramerate;

	unsigned long m_cursor;
	static std::map<ButtonCode_t, const char*> s_buttonMap;
	static std::map<ButtonCode_t, const char*> s_shiftedButtonMap;
};

#endif