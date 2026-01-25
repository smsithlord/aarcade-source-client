#include "cbase.h"
#include "c_windowsutils.h"
#include "c_anarchymanager.h"
#include <Windows.h>
typedef UINT(WINAPI *GetDpiForWindowFn)(HWND);

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// This file was created by ChatGPT
C_WindowsUtils::C_WindowsUtils()
{
}

C_WindowsUtils::~C_WindowsUtils()
{
}

void C_WindowsUtils::Init()
{
	m_fScalePercent = 1.0f;

	HWND hWnd = g_pAnarchyManager ? g_pAnarchyManager->GetHWND() : nullptr;
	if (!hWnd)
		return;

	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (!hMon)
		return;

	// Win 8.1+ - reliable even under DPI virtualization
	typedef HRESULT(WINAPI *GetScaleFactorForMonitorFn)(HMONITOR, int* /*DEVICE_SCALE_FACTOR*/);

	HMODULE hShcore = LoadLibraryA("shcore.dll");
	if (hShcore)
	{
		GetScaleFactorForMonitorFn pGetScaleFactorForMonitor =
			(GetScaleFactorForMonitorFn)GetProcAddress(hShcore, "GetScaleFactorForMonitor");

		if (pGetScaleFactorForMonitor)
		{
			int scalePercent = 0;
			HRESULT hr = pGetScaleFactorForMonitor(hMon, &scalePercent);
			if (SUCCEEDED(hr) && scalePercent > 0)
			{
				// 100 / 125 / 150 / 200 -> 1.0 / 1.25 / 1.5 / 2.0
				m_fScalePercent = (float)scalePercent / 100.0f;
			}
		}

		FreeLibrary(hShcore);
	}

	// Final safety clamp
	if (m_fScalePercent <= 0.0f)
		m_fScalePercent = 1.0f;

	//Msg("Windows display scale initialized: %.2fx\n", m_fScalePercent);
}
