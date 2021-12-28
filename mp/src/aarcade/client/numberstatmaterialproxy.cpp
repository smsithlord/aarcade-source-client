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

class CNumberStatMaterialProxy : public IMaterialProxy
{
public:
	CNumberStatMaterialProxy();
	virtual ~CNumberStatMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar* m_pResultVar;
	int m_iStatTypeValue;
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

CNumberStatMaterialProxy::CNumberStatMaterialProxy()
{
	m_pResultVar = NULL;
	m_iStatTypeValue = 0;
	m_iDigit = -1;
}

CNumberStatMaterialProxy::~CNumberStatMaterialProxy()
{
}

#include <vector>
bool CNumberStatMaterialProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	bool foundVar;
	IMaterialVar* pResultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	m_pResultVar = pResultVar;

	char const* statType = pKeyValues->GetString("type");
	if (!statType)
		return false;
	/*
		StatTypes:		
		int iObjects;
		int iGlobalPlayers;
		int iServerVisitors;
		int iServerVisits;
		int iLibraryItems;
		int iLibraryMaps;
		int iLibraryModels;
		//int iDate;
		//int iMonths;
		//int iYears;
		//int iPostfix;
		//int iHours;
		//int iMinutes;
		//int iSeconds;
	*/

	std::vector<std::string> statTypes;
	statTypes.push_back("objects");
	statTypes.push_back("global_players");
	statTypes.push_back("global_time");
	statTypes.push_back("global_tubes");
	statTypes.push_back("server_visitors");
	statTypes.push_back("server_visits");
	statTypes.push_back("library_items");
	statTypes.push_back("library_maps");
	statTypes.push_back("library_models");
	//timeTypes.push_back("date");
	//timeTypes.push_back("months");
	//timeTypes.push_back("years");
	//timeTypes.push_back("postfix");
	//timeTypes.push_back("hours");
	//timeTypes.push_back("minutes");
	//timeTypes.push_back("seconds");

	int iStatTypeValue = -1;
	for (unsigned int i = 0; i < statTypes.size(); i++)
	{
		if (!Q_stricmp(statTypes[i].c_str(), statType))
		{
			iStatTypeValue = i;
			break;
		}
	}

	if (iStatTypeValue < 0)
		return false;

	m_iStatTypeValue = iStatTypeValue;
	m_iDigit = pKeyValues->GetInt("digit", -1);

	return true;
}

void CNumberStatMaterialProxy::OnBind(void *pC_BaseEntity)
{
	if (!m_pResultVar)
		return;

	int iValue;
	numberStatsState_t numberStatsState = g_pAnarchyManager->GetNumberStatsState();
	switch (m_iStatTypeValue)
	{
		case 0:
			iValue = numberStatsState.iObjects;
			break;

		case 1:
			iValue = numberStatsState.iGlobalPlayers;
			break;

		case 2:
			iValue = numberStatsState.iGlobalTime;
			break;

		case 3:
			iValue = numberStatsState.iGlobalTubes;
			break;

		case 4:
			iValue = numberStatsState.iServerVisitors;
			break;

		case 5:
			iValue = numberStatsState.iServerVisits;
			break;

		case 6:
			iValue = numberStatsState.iLibraryItems;
			break;

		case 7:
			iValue = numberStatsState.iLibraryMaps;
			break;

		case 8:
			iValue = numberStatsState.iLibraryModels;
			break;

		default:
			iValue = numberStatsState.iObjects;
	}

	if (m_iDigit >= 0)
	{
		int iDigitValue = 0;
		std::string digitString = VarArgs("%05i", iValue);
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

IMaterial *CNumberStatMaterialProxy::GetMaterial()
{
	return null;// m_pAudioPeakVar->GetOwningMaterial();
}

EXPOSE_INTERFACE(CNumberStatMaterialProxy, IMaterialProxy, "AANumberStat" IMATERIAL_PROXY_INTERFACE_VERSION);
