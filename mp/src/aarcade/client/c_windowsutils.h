#ifndef C_WINDOWS_UTILS_H
#define C_WINDOWS_UTILS_H

// This file was created by ChatGPT
class C_WindowsUtils
{
public:
	C_WindowsUtils();
	~C_WindowsUtils();
	void Init();
	float GetWindowScaleFactor() const { return m_fScalePercent; }

private:
	float m_fScalePercent = 1.0f;
};

#endif