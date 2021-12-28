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

class CAudioPeakMaterialProxy : public IMaterialProxy
{
public:
	CAudioPeakMaterialProxy();
	virtual ~CAudioPeakMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
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
};

CAudioPeakMaterialProxy::CAudioPeakMaterialProxy()
{
	m_pAudioPeakVar = NULL;
}

CAudioPeakMaterialProxy::~CAudioPeakMaterialProxy()
{
}


bool CAudioPeakMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	m_pAVRConVar = cvar->FindVar("avr");
	m_pAVRAmpConVar = cvar->FindVar("avramp");

	char const* pPeakVarName = pKeyValues->GetString( "audioPeakVar" );
	if( !pPeakVarName )
		return false;

	bool foundVar;
	m_pAudioPeakVar = pMaterial->FindVar( pPeakVarName, &foundVar, false );
	if( !foundVar )
		return false;

	m_AudioPeakMin.Init(pMaterial, pKeyValues, "audioPeakMin", 0.0f);
	m_AudioPeakMax.Init(pMaterial, pKeyValues, "audioPeakMax", 1.0f);
	m_UseAVR.Init(pMaterial, pKeyValues, "audioUseAVR", 0.0f);
	m_AudioPeakAmp.Init(pMaterial, pKeyValues, "audioPeakAmp", 1.0f);

	return true;
}

Color TransformH3(
	const Color &in,  // color to transform
	float H
	)
{
	float U = cos(H*M_PI / 180);
	float W = sin(H*M_PI / 180);

	int r, g, b, a;
	in.GetColor(r, g, b, a);

	Color ret;
	ret.SetColor((.299 + .701*U + .168*W)*r
		+ (.587 - .587*U + .330*W)*g
		+ (.114 - .114*U - .497*W)*b,
		(.299 - .299*U - .328*W)*r
		+ (.587 + .413*U + .035*W)*g
		+ (.114 - .114*U + .292*W)*b,
		(.299 - .3*U + 1.25*W)*r
		+ (.587 - .588*U - 1.05*W)*g
		+ (.114 + .886*U - .203*W)*b);
	return ret;
}

void CAudioPeakMaterialProxy::OnBind( void *pC_BaseEntity )
{
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
}

IMaterial *CAudioPeakMaterialProxy::GetMaterial()
{
	return m_pAudioPeakVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CAudioPeakMaterialProxy, IMaterialProxy, "AAAudioPeak" IMATERIAL_PROXY_INTERFACE_VERSION );
