#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include <KeyValues.h>
#include "mathlib/vmatrix.h"
#include "functionproxy.h"
#include "aarcade/client/c_anarchymanager.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

class CSystemTimeMaterialProxy : public IMaterialProxy
{
public:
	CSystemTimeMaterialProxy();
	virtual ~CSystemTimeMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar* m_pResultVar;
	int m_iTimeTypeValue;
	int m_iDigit;
	/*
	Color	m_hueColor;
	ConVar* m_pAVRConVar;
	ConVar* m_pAVRAmpConVar;
	IMaterialVar *m_pAudioPeakVar;	// $audioPeakVar
	CFloatInput m_UseAVR;	// $audioUseAVR
	CFloatInput m_AudioPeakMin;	// $audioPeakMin
	CFloatInput m_AudioPeakMax;	// $audioPeakMax
	CFloatInput m_AudioPeakAmp;	// $audioPeakAmp
	//CFloatInput m_TextureScrollAngle;	// $textureScrollAngle
	//CFloatInput m_TextureScale;
	*/
};

CSystemTimeMaterialProxy::CSystemTimeMaterialProxy()
{
	m_pResultVar = NULL;
	m_iTimeTypeValue = 0;
	m_iDigit = -1;
}

CSystemTimeMaterialProxy::~CSystemTimeMaterialProxy()
{
}

#include <vector>
bool CSystemTimeMaterialProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{

	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	bool foundVar;
	IMaterialVar* pResultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	m_pResultVar = pResultVar;

	char const* timeType = pKeyValues->GetString("type");
	if (!timeType)
		return false;

	/*
		TimeTypes:		
		int iDay;
		int iDate;
		int iMonths;
		int iYears;
		int iPostfix;
		int iHours;
		int iMinutes;
		int iSeconds;
	*/

	std::vector<std::string> timeTypes;
	timeTypes.push_back("day");
	timeTypes.push_back("date");
	timeTypes.push_back("months");
	timeTypes.push_back("years");
	timeTypes.push_back("postfix");
	timeTypes.push_back("hours");
	timeTypes.push_back("minutes");
	timeTypes.push_back("seconds");

	int iTimeTypeValue = -1;
	for (unsigned int i = 0; i < timeTypes.size(); i++)
	{
		if (!Q_stricmp(timeTypes[i].c_str(), timeType))
		{
			iTimeTypeValue = i;
			break;
		}
	}

	if (iTimeTypeValue < 0)
		return false;

	m_iTimeTypeValue = iTimeTypeValue;
	m_iDigit = pKeyValues->GetInt("digit", -1);

	return true;
}

void CSystemTimeMaterialProxy::OnBind(void *pC_BaseEntity)
{
	if (!m_pResultVar)
		return;

	int iValue;
	systemTimeState_t systemTimeState = g_pAnarchyManager->GetSystemTimeState();
	switch (m_iTimeTypeValue)
	{
		case 0:
			iValue = systemTimeState.iDay;
			break;

		case 1:
			iValue = systemTimeState.iDate;
			break;

		case 2:
			iValue = systemTimeState.iMonths;
			break;

		case 3:
			iValue = systemTimeState.iYears;
			break;

		case 4:
			iValue = systemTimeState.iPostfix;
			break;

		case 5:
			iValue = systemTimeState.iHours;
			if (iValue == 0)
				iValue = 12;
			break;

		case 6:
			iValue = systemTimeState.iMinutes;
			break;

		case 7:
			iValue = systemTimeState.iSeconds;
			break;

		default:
			iValue = systemTimeState.iDay;
	}

	if (m_iDigit >= 0)
	{
		int iDigitValue = 0;
		std::string digitString = VarArgs("%02i", iValue);
		unsigned int uLength = digitString.length();
		if (m_iDigit < uLength)
			iDigitValue = Q_atoi(VarArgs("%c", digitString.at(uLength - m_iDigit - 1)));
		
		m_pResultVar->SetIntValue(iDigitValue);
	}
	else
	{
		m_pResultVar->SetIntValue(iValue);
	}

	/*
	if( !m_pAudioPeakVar )
		return;

	float flMin = m_AudioPeakMin.GetFloat();
	float flMax = m_AudioPeakMax.GetFloat();
	float flRange = flMax - flMin;

	float flValue = flMin + (g_pAnarchyManager->GetAudioPeakValue() * m_pAVRAmpConVar->GetFloat() * m_AudioPeakAmp.GetFloat() * flRange);

	if (flValue > flMax)
		flValue = flMax;
	else if (flValue < flMin)
		flValue = flMin;

	float avrValue = m_UseAVR.GetFloat();//m_pAVRConVar->GetInt();
	float avrRed = 0.0f;
	float avrGreen = 0.0f;
	float avrBlue = 0.0f;
	if (avrValue > 0)
	{
		if (avrValue == 1 || avrValue == 2)
		{
			float hue = g_pAnarchyManager->GetHueShifter();
			m_hueColor.SetColor(255, 0, 0);
			m_hueColor = TransformH3(m_hueColor, hue);
		}
		else if (avrValue == 3)
			m_hueColor.SetColor(255, 0, 0);
		else if (avrValue == 4)
			m_hueColor.SetColor(0, 255, 0);
		else if (avrValue == 5)
			m_hueColor.SetColor(0, 0, 255);

		avrRed = m_hueColor.r() / 255.0 * flValue;
		avrGreen = m_hueColor.b() / 255.0 * flValue;
		avrBlue = m_hueColor.g() / 255.0 * flValue;
	}

	if (m_pAudioPeakVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		int iSize = m_pAudioPeakVar->VectorSize();
		if ( avrValue > 0 && iSize == 3 )
			m_pAudioPeakVar->SetVecValue(avrRed, avrGreen, avrBlue);
		else
		{
			if (iSize == 4)
				m_pAudioPeakVar->SetVecValue(flValue, flValue, flValue, flValue);
			else if ( iSize == 3 )
				m_pAudioPeakVar->SetVecValue(flValue, flValue, flValue);
			else if (iSize == 2)
				m_pAudioPeakVar->SetVecValue(flValue, flValue);
		}
	}
	else if (m_pAudioPeakVar->GetType() == MATERIAL_VAR_TYPE_FLOAT)
		m_pAudioPeakVar->SetFloatValue(flValue);

	if ( ToolsEnabled() )
		ToolFramework_RecordMaterialParams( GetMaterial() );
	*/
}

IMaterial *CSystemTimeMaterialProxy::GetMaterial()
{
	return null;// m_pAudioPeakVar->GetOwningMaterial();
}

EXPOSE_INTERFACE(CSystemTimeMaterialProxy, IMaterialProxy, "AASystemTime" IMATERIAL_PROXY_INTERFACE_VERSION);
