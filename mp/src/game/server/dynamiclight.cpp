//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dynamic light.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dlight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define NUM_DL_EXPONENT_BITS	8
#define MIN_DL_EXPONENT_VALUE	-((1 << (NUM_DL_EXPONENT_BITS-1)) - 1)
#define MAX_DL_EXPONENT_VALUE	((1 << (NUM_DL_EXPONENT_BITS-1)) - 1)


class CDynamicLight : public CBaseEntity
{
public:
	DECLARE_CLASS( CDynamicLight, CBaseEntity );

	void Spawn( void );
	void DynamicLightThink( void );
	bool KeyValue( const char *szKeyName, const char *szValue );

	void SetRainbowMode(); // Added for Anarchy Arcade

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	// Added for Anarchy Arcade
	void UpdateOnRemove(void);
	void RainbowThink(void);
	// End added for Anarchy Arcade

	// Turn on and off the light
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

public:
	bool m_bIsRainbow;	// Added for Anarchy Arcade
	ConVar* m_pPeakConVar;	// Added for Anarchy Arcade
	ConVar* m_pHueShiftConVar;	// Added for Anarchy Arcade
	unsigned char m_ActualFlags;
	CNetworkVar( unsigned char, m_Flags );
	CNetworkVar( unsigned char, m_LightStyle );
	bool	m_On;
	CNetworkVar( float, m_Radius );
	CNetworkVar( int, m_Exponent );
	CNetworkVar( float, m_InnerAngle );
	CNetworkVar( float, m_OuterAngle );
	CNetworkVar( float, m_SpotRadius );
};

LINK_ENTITY_TO_CLASS(light_dynamic, CDynamicLight);

BEGIN_DATADESC( CDynamicLight )

	DEFINE_FIELD( m_ActualFlags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_On, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( DynamicLightThink ),

	// Inputs
	DEFINE_INPUT( m_Radius,		FIELD_FLOAT,	"distance" ),
	DEFINE_INPUT( m_Exponent,	FIELD_INTEGER,	"brightness" ),
	DEFINE_INPUT( m_InnerAngle,	FIELD_FLOAT,	"_inner_cone" ),
	DEFINE_INPUT( m_OuterAngle,	FIELD_FLOAT,	"_cone" ),
	DEFINE_INPUT( m_SpotRadius,	FIELD_FLOAT,	"spotlight_radius" ),
	DEFINE_INPUT( m_LightStyle,	FIELD_CHARACTER,"style" ),
	
	// Input functions
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CDynamicLight, DT_DynamicLight)
	SendPropInt( SENDINFO(m_Flags), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_LightStyle), 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_Radius), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_Exponent), NUM_DL_EXPONENT_BITS),
	SendPropFloat( SENDINFO(m_InnerAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_OuterAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_SpotRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

// Added for Anarchy Arcade
void CDynamicLight::UpdateOnRemove(void)
{
	inputdata_t emptyDummy;
	this->InputTurnOff(emptyDummy);
	BaseClass::UpdateOnRemove();
}
// End added for Anarchy Arcade

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDynamicLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "_light" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
	}
	else if ( FStrEq( szKeyName, "pitch" ) )
	{
		float angle = atof(szValue);
		if ( angle )
		{
			QAngle angles = GetAbsAngles();
			angles[PITCH] = -angle;
			SetAbsAngles( angles );
		}
	}
	else if ( FStrEq( szKeyName, "spawnflags" ) )
	{
		m_ActualFlags = m_Flags = atoi(szValue);
		// Added for Anarchy Arcade
		if (m_ActualFlags & 0x10)
		{
			m_ActualFlags &= ~0x10;
			SetRainbowMode();
			m_pPeakConVar = cvar->FindVar("peak");
			m_pHueShiftConVar = cvar->FindVar("hueshift");
		}
		// End added for Anarchy Arcade
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//------------------------------------------------------------------------------
// Turn on and off the light
//------------------------------------------------------------------------------
void CDynamicLight::InputTurnOn( inputdata_t &inputdata )
{
	m_Flags = m_ActualFlags;
	m_On = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CDynamicLight::InputTurnOff( inputdata_t &inputdata )
{
	// This basically shuts it off
	m_Flags = DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_NO_WORLD_ILLUMINATION;
	m_On = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CDynamicLight::InputToggle( inputdata_t &inputdata )
{
	if (m_On)
	{
		InputTurnOff( inputdata );
	}
	else
	{
		InputTurnOn( inputdata );
	}
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CDynamicLight::Spawn( void )
{
	m_bIsRainbow = false;	// Added for Anarchy Arcade
	Precache();
	SetSolid( SOLID_NONE );
	m_On = true;
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// If we have a target, think so we can orient towards it
	if ( m_target != NULL_STRING )
	{
		SetThink( &CDynamicLight::DynamicLightThink );
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
	
	int clampedExponent = clamp( (int) m_Exponent, MIN_DL_EXPONENT_VALUE, MAX_DL_EXPONENT_VALUE );
	if ( m_Exponent != clampedExponent )
	{
		Warning( "light_dynamic at [%d %d %d] has invalid exponent value (%d must be between %d and %d).\n",
			(int)GetAbsOrigin().x, (int)GetAbsOrigin().x, (int)GetAbsOrigin().x, 
			m_Exponent.Get(),
			MIN_DL_EXPONENT_VALUE,
			MAX_DL_EXPONENT_VALUE );
		
		m_Exponent = clampedExponent;
	}
}

// Added for Anarchy Arcade
void CDynamicLight::SetRainbowMode()
{
	SetThink(&CDynamicLight::RainbowThink);
	SetNextThink(gpGlobals->curtime + 0.1);
}

// originally from https://stackoverflow.com/questions/8507885/shift-hue-of-an-rgb-color
Color TransformH(
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

void CDynamicLight::RainbowThink(void)
{
	float flAmp = 1.5;
	float audioPeakValue = m_pPeakConVar->GetFloat();
	float flHueShift = m_pHueShiftConVar->GetFloat();
	float peak = audioPeakValue * flAmp;
	if (peak > 1.0f)
		peak = 1.0f;
	else if (peak < 0.2f)
		peak = 0.2f;

	/*int avrValue = m_pAVRConVar->GetInt();
	if (avrValue == 1 || avrValue == 2)
	{
		float hue = g_pAnarchyManager->GetHueShifter();

		// per-object rainbow
		if (avrValue == 2)
		{
			hue += (pC_BaseEntity->ProxyRandomValue() * 360.0f);
			if (hue > 360.0f)
				hue = 360.0f - hue;
		}

		m_hueColor.SetColor(255, 0, 0);
		m_hueColor = TransformH(m_hueColor, hue);
	}
	else if (avrValue == 3)
		m_hueColor.SetColor(255, 0, 0);
	else if (avrValue == 4)
		m_hueColor.SetColor(0, 255, 0);
	else if (avrValue == 5)
		m_hueColor.SetColor(0, 0, 255);
		*/

	//pColorVar->SetVecValue((m_hueColor.r() / 255.0) * peak, (m_hueColor.g() / 255.0) * peak, (m_hueColor.b() / 255.0) * peak);

	Color hueColor;
	//float hue = 150.0f;// g_pAnarchyManager->GetHueShifter();

	// per-object rainbow
	/*if (avrValue == 2)
	{
		hue += (pC_BaseEntity->ProxyRandomValue() * 360.0f);
		if (hue > 360.0f)
			hue = 360.0f - hue;
	}*/

	hueColor.SetColor(255, 0, 0);
	hueColor = TransformH(hueColor, flHueShift);

	//color32 tmp;
	float flMin = 20;
	color32 tmp;
	tmp.r = flMin + (hueColor.r() * peak) / 1;
	tmp.g = flMin + (hueColor.g() * peak) / 1;
	tmp.b = flMin + (hueColor.b() * peak) / 1;

	if (tmp.r > 255)
		tmp.r = 255;
	if (tmp.g > 255)
		tmp.g = 255;
	if (tmp.b > 255)
		tmp.b = 255;

	//const color32 oldColor = GetRenderColor();
	//DevMsg("Old color: %i, %i, %i\n", oldColor.r, oldColor.g, oldColor.b);
	//DevMsg("New color: %i, %i, %i\n", tmp.r, tmp.g, tmp.b);
	SetRenderColor(tmp.r, tmp.g, tmp.b);
	SetNextThink(gpGlobals->curtime + 0.1);
}
// End added for Anarchy Arcade

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicLight::DynamicLightThink( void )
{
	if ( m_target == NULL_STRING )
		return;

	CBaseEntity *pEntity = GetNextTarget();
	if ( pEntity )
	{
		Vector vecToTarget = (pEntity->GetAbsOrigin() - GetAbsOrigin());
		QAngle vecAngles;
		VectorAngles( vecToTarget, vecAngles );
		SetAbsAngles( vecAngles );
	}
	
	SetNextThink( gpGlobals->curtime + 0.1 );
}
