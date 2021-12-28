//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "spotlightend.h"
#include "../aarcade/server/prop_shortcut_entity.h"	// Added for Anarchy Arcade

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
#define SF_SPOTLIGHT_START_LIGHT_ON			0x1
#define SF_SPOTLIGHT_NO_DYNAMIC_LIGHT		0x2


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointSpotlight : public CPointEntity
{
	DECLARE_CLASS( CPointSpotlight, CPointEntity );
public:
	DECLARE_DATADESC();

	CPointSpotlight();

	void	Precache(void);
	void	Spawn(void);
	virtual void Activate();

	virtual void OnEntityEvent(EntityEvent_t event, void *pEventData);

	// Added for Anarchy Arcade
	void UpdateOnRemove(void);
	//bool KeyValue(const char *szKeyName, const char *szValue);
	void SetRainbowMode();
	void ShortcutThink(void);
	void SetShortcut(CPropShortcutEntity* pShortcut, bool bAudioSensitive);
	void VRSpazzFixPulse();
	// End added for Anarchy Arcade

private:
	int 	UpdateTransmitState();
	void	SpotlightThink(void);
	void	SpotlightUpdate(void);
	Vector	SpotlightCurrentPos(void);
	void	SpotlightCreate(void);
	void	SpotlightDestroy(void);

	// ------------------------------
	//  Inputs
	// ------------------------------
	void InputLightOn( inputdata_t &inputdata );
	void InputLightOff( inputdata_t &inputdata );

	// Creates the efficient spotlight 
	void CreateEfficientSpotlight();

	// Computes render info for a spotlight
	void ComputeRenderInfo();

private:
	bool m_bIsVRSpazzFix;	// Added for Anarchy Arcade
	float m_flOldScale;	// Added for Anarchy Arcade
	bool m_bIsRainbow;	// Added for Anarchy Arcade
	ConVar* m_pPeakConVar;	// Added for Anarchy Arcade
	ConVar* m_pHueShiftConVar;	// Added for Anarchy Arcade
	CPropShortcutEntity* m_pShortcut;	// Added for Anarchy Arcade
	bool	m_bSpotlightOn;
	bool	m_bEfficientSpotlight;
	bool	m_bIgnoreSolid;
	Vector	m_vSpotlightTargetPos;
	Vector	m_vSpotlightCurrentPos;
	Vector	m_vSpotlightDir;
	int		m_nHaloSprite;
	CHandle<CBeam>			m_hSpotlight;
	CHandle<CSpotlightEnd>	m_hSpotlightTarget;
	
	float	m_flSpotlightMaxLength;
	float	m_flSpotlightCurLength;
	float	m_flSpotlightGoalWidth;
	float	m_flHDRColorScale;
	int		m_nMinDXLevel;

public:
	COutputEvent m_OnOn, m_OnOff;     ///< output fires when turned on, off
};

BEGIN_DATADESC( CPointSpotlight )
	DEFINE_FIELD( m_flSpotlightCurLength, FIELD_FLOAT ),

	DEFINE_FIELD( m_bSpotlightOn,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEfficientSpotlight,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vSpotlightTargetPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpotlightCurrentPos,	FIELD_POSITION_VECTOR ),

	// Robin: Don't Save, recreated after restore/transition
	//DEFINE_FIELD( m_hSpotlight,			FIELD_EHANDLE ),
	//DEFINE_FIELD( m_hSpotlightTarget,		FIELD_EHANDLE ),

	DEFINE_FIELD( m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_nHaloSprite,			FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bIgnoreSolid, FIELD_BOOLEAN, "IgnoreSolid" ),
	DEFINE_KEYFIELD( m_flSpotlightMaxLength,FIELD_FLOAT, "SpotlightLength"),
	DEFINE_KEYFIELD( m_flSpotlightGoalWidth,FIELD_FLOAT, "SpotlightWidth"),
	DEFINE_KEYFIELD( m_flHDRColorScale, FIELD_FLOAT, "HDRColorScale" ),
	DEFINE_KEYFIELD( m_nMinDXLevel, FIELD_INTEGER, "mindxlevel" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOn",		InputLightOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOff",		InputLightOff ),
	DEFINE_OUTPUT( m_OnOn, "OnLightOn" ),
	DEFINE_OUTPUT( m_OnOff, "OnLightOff" ),

	DEFINE_THINKFUNC( SpotlightThink ),

END_DATADESC()


LINK_ENTITY_TO_CLASS(point_spotlight, CPointSpotlight);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointSpotlight::CPointSpotlight()
{
#ifdef _DEBUG
	m_vSpotlightTargetPos.Init();
	m_vSpotlightCurrentPos.Init();
	m_vSpotlightDir.Init();
#endif
	m_pShortcut = NULL;	// Added for Anarhcy Arcade
	m_bIsVRSpazzFix = false;	// Added for Anarchy Arcade
	m_flHDRColorScale = 1.0f;
	m_nMinDXLevel = 0;
	m_bIgnoreSolid = false;
}

// Added for Anarchy Arcade

// originally from https://stackoverflow.com/questions/8507885/shift-hue-of-an-rgb-color
Color TransformH2(
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

void CPointSpotlight::VRSpazzFixPulse()
{
	//inputdata_t inputdata;
	//inputdata_t myInputData;
	//inputdata.value.SetString(MAKE_STRING(sequenceName));
	//pThisAsDynamic->InputSetAnimation(myInputData);
	//this->InputLightOff(inputdata);
	//this->InputLightOn(inputdata);

	m_bIsVRSpazzFix = true;

	m_vSpotlightTargetPos = this->GetAbsOrigin();
	m_vSpotlightCurrentPos = this->GetAbsOrigin();
	m_vSpotlightDir.Init();

	if (m_bSpotlightOn)
	{
		m_bSpotlightOn = false;
		if (m_bEfficientSpotlight)
		{
			SpotlightDestroy();
		}
	}
	else if (!m_bSpotlightOn)
	{
		m_bSpotlightOn = true;
		if (m_bEfficientSpotlight)
		{
			CreateEfficientSpotlight();
		}
	}
}

void CPointSpotlight::SetShortcut(CPropShortcutEntity* pShortcut, bool bAudioSensitive)
{
	//m_pShortcut = dynamic_cast<CPropShortcutEntity*>(this->GetMoveParent());
	m_pShortcut = pShortcut;
	//m_vSpotlightCurrentPos = m_pShortcut->GetAbsOrigin();

	m_flOldScale = m_pShortcut->GetModelScale();
	//DevMsg("Starting Scale: %f\n", m_flOldScale);
	m_flSpotlightGoalWidth *= m_flOldScale;
	m_flSpotlightMaxLength *= m_flOldScale;

	if (bAudioSensitive)
	{
		m_pPeakConVar = cvar->FindVar("peak");
		m_pHueShiftConVar = cvar->FindVar("hueshift");
		m_bIsRainbow = true;
	}
}

void CPointSpotlight::ShortcutThink(void)
{
	if (m_bIsRainbow)
	{
		float flAmp = 1.0;
		float audioPeakValue = m_pPeakConVar->GetFloat();
		float flHueShift = m_pHueShiftConVar->GetFloat();
		float peak = audioPeakValue * flAmp;
		if (peak > 1.0f)
			peak = 1.0f;
		else if (peak < 0.2f)
			peak = 0.2f;

		Color hueColor;
		hueColor.SetColor(255, 0, 0);
		hueColor = TransformH2(hueColor, flHueShift);

		//color32 tmp;
		float flMin = 40;
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
		//m_hSpotlight->GetColor
		//DevMsg("Old color: %i, %i, %i\n", oldColor.r, oldColor.g, oldColor.b);
		//DevMsg("New color: %i, %i, %i\n", tmp.r, tmp.g, tmp.b);
		//SetRenderColor(tmp.r, tmp.g, tmp.b);
		//m_hSpotlight->SetColor(tmp.r, tmp.g, tmp.b);

		//DevMsg("Old color: %i, %i, %i\n", oldColor.r, oldColor.g, oldColor.b);
		//DevMsg("New color: %i, %i, %i\n", tmp.r, tmp.g, tmp.b);
		//SetRenderColor(tmp.r, tmp.g, tmp.b);
		if (m_hSpotlight)
			m_hSpotlight->SetColor(tmp.r, tmp.g, tmp.b);
	}

	if (m_flOldScale != m_pShortcut->GetModelScale())
	{
		//m_Radius
		//m_vSpotlightCurrentPos = m_pShortcut->GetAbsOrigin();
		m_flSpotlightGoalWidth = (m_flSpotlightGoalWidth / m_flOldScale) * m_pShortcut->GetModelScale();
		m_flSpotlightMaxLength = (m_flSpotlightMaxLength / m_flOldScale) * m_pShortcut->GetModelScale();
		if (m_hSpotlight)
			m_hSpotlight->SetWidth(m_flSpotlightGoalWidth);

		//DevMsg("Scale: %f\n", m_flOldScale);
		m_flOldScale = m_pShortcut->GetModelScale();
	}

	//SetNextThink(gpGlobals->curtime + 0.1);
}

/*bool CPointSpotlight::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "spawnflags"))
	{*/
		/*
		int iActualFlags = atoi(szValue);
		if (iActualFlags & 0x4)
		{
			iActualFlags &= ~0x4;
			//m_pShortcut = dynamic_cast<CPropShortcutEntity*>(this->GetMoveParent());
			if (m_pShortcut)
			{
				m_flOldScale = m_pShortcut->GetModelScale();
				DevMsg("Starting Scale: %f\n", m_flOldScale);
				m_flSpotlightGoalWidth *= m_flOldScale;
				m_flSpotlightMaxLength *= m_flOldScale;
				//m_vSpotlightCurrentPos = m_pShortcut->GetAbsOrigin();
			}

			m_pPeakConVar = cvar->FindVar("peak");
			m_pHueShiftConVar = cvar->FindVar("hueshift");
			m_bIsRainbow = true;
		}
		else*/
			/*m_bIsRainbow = false;
		return BaseClass::KeyValue(szKeyName, szValue);
	}
	else
	{
		return BaseClass::KeyValue(szKeyName, szValue);
	}

	return true;
}*/

void CPointSpotlight::UpdateOnRemove(void)
{
	//inputdata_t emptyDummy;
	//InputLightOff(emptyDummy);

	if (m_bSpotlightOn)
	{
		m_bSpotlightOn = false;
		SpotlightDestroy();
	}

	BaseClass::UpdateOnRemove();
}
// End added for Anarchy Arcade


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Precache(void)
{
	BaseClass::Precache();

	// Sprites.
	m_nHaloSprite = PrecacheModel("sprites/light_glow03.vmt");
	PrecacheModel( "sprites/glow_test02.vmt" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Spawn(void)
{
	Precache();

	UTIL_SetSize( this,vec3_origin,vec3_origin );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	m_bEfficientSpotlight = true;

	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		DevMsg("%s (%s) has an invalid spotlight length <= 0, setting to 500\n", GetClassname(), GetDebugName() );
		m_flSpotlightMaxLength = 500;
	}
	if (m_flSpotlightGoalWidth <= 0)
	{
		DevMsg("%s (%s) has an invalid spotlight width <= 0, setting to 10\n", GetClassname(), GetDebugName() );
		m_flSpotlightGoalWidth = 10;
	}
	
	if (m_flSpotlightGoalWidth > MAX_BEAM_WIDTH )
	{
		DevMsg("%s (%s) has an invalid spotlight width %.1f (max %.1f).\n", GetClassname(), GetDebugName(), m_flSpotlightGoalWidth, MAX_BEAM_WIDTH );
		m_flSpotlightGoalWidth = MAX_BEAM_WIDTH; 
	}

	// ------------------------------------
	//	Init all class vars 
	// ------------------------------------
	m_vSpotlightTargetPos	= vec3_origin;
	m_vSpotlightCurrentPos	= vec3_origin;
	m_hSpotlight			= NULL;
	m_hSpotlightTarget		= NULL;
	m_vSpotlightDir			= vec3_origin;
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;

	m_bSpotlightOn = HasSpawnFlags( SF_SPOTLIGHT_START_LIGHT_ON );

	SetThink( &CPointSpotlight::SpotlightThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Computes render info for a spotlight
//-----------------------------------------------------------------------------
void CPointSpotlight::ComputeRenderInfo()
{
	// Fade out spotlight end if past max length.  
	if ( m_flSpotlightCurLength > 2*m_flSpotlightMaxLength )
	{
		m_hSpotlightTarget->SetRenderColorA( 0 );
		m_hSpotlight->SetFadeLength( m_flSpotlightMaxLength );
	}
	else if ( m_flSpotlightCurLength > m_flSpotlightMaxLength )		
	{
		m_hSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_hSpotlight->SetFadeLength( m_flSpotlightMaxLength );
	}
	else
	{
		m_hSpotlightTarget->SetRenderColorA( 1.0 );
		m_hSpotlight->SetFadeLength( m_flSpotlightCurLength );
	}

	// Adjust end width to keep beam width constant
	float flNewWidth = m_flSpotlightGoalWidth * (m_flSpotlightCurLength / m_flSpotlightMaxLength);
	flNewWidth = clamp(flNewWidth, 0.f, MAX_BEAM_WIDTH );
	m_hSpotlight->SetEndWidth(flNewWidth);

	// Adjust width of light on the end.  
	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		// <<TODO>> - magic number 1.8 depends on sprite size
		m_hSpotlightTarget->m_flLightScale = 1.8*flNewWidth;
	}
}


//-----------------------------------------------------------------------------
// Creates the efficient spotlight 
//-----------------------------------------------------------------------------
void CPointSpotlight::CreateEfficientSpotlight()
{
	if ( m_hSpotlightTarget.Get() != NULL )
		return;

	SpotlightCreate();
	m_vSpotlightCurrentPos = SpotlightCurrentPos();
	m_hSpotlightTarget->SetAbsOrigin( m_vSpotlightCurrentPos );
	m_hSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();
	VectorSubtract( m_hSpotlightTarget->GetAbsOrigin(), m_hSpotlightTarget->m_vSpotlightOrg, m_hSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = VectorNormalize( m_hSpotlightTarget->m_vSpotlightDir );
	m_hSpotlightTarget->SetMoveType( MOVETYPE_NONE );
	ComputeRenderInfo();

	m_OnOn.FireOutput( this, this );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Activate(void)
{
	BaseClass::Activate();

	if ( GetMoveParent() )
	{
		m_bEfficientSpotlight = false;
	}

	if ( m_bEfficientSpotlight )
	{
		if ( m_bSpotlightOn )
		{
			CreateEfficientSpotlight();
		}

		// Don't think
		SetThink( NULL );
	}
}


//-------------------------------------------------------------------------------------
// Optimization to deal with spotlights
//-------------------------------------------------------------------------------------
void CPointSpotlight::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	if ( event == ENTITY_EVENT_PARENT_CHANGED )
	{
		if ( GetMoveParent() )
		{
			m_bEfficientSpotlight = false;
			if ( m_hSpotlightTarget )
			{
				m_hSpotlightTarget->SetMoveType( MOVETYPE_FLY );
			}
			SetThink( &CPointSpotlight::SpotlightThink );
			SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}

	BaseClass::OnEntityEvent( event, pEventData );
}

	
//-------------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model so spotlight gets proper position
// Input   :
// Output  :
//-------------------------------------------------------------------------------------
int CPointSpotlight::UpdateTransmitState()
{
	//if (m_bIsVRSpazzFix)	// Added for Anarchy Arcade
	//	return SetTransmitState(FL_EDICT_ALWAYS);	// Added for Anarchy Arcade

	if ( m_bEfficientSpotlight )
		return SetTransmitState( FL_EDICT_DONTSEND );

	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CPointSpotlight::SpotlightThink( void )
{
	if (m_pShortcut)	// Added for Anarchy Arcade
		ShortcutThink();	// Added for Anarchy Arcade

	if ( GetMoveParent() )
	{
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	SpotlightUpdate();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightCreate(void)
{
	if ( m_hSpotlightTarget.Get() != NULL )
		return;

	AngleVectors( GetAbsAngles(), &m_vSpotlightDir );

	Vector vTargetPos;
	if ( m_bIgnoreSolid )
	{
		vTargetPos = GetAbsOrigin() + m_vSpotlightDir * m_flSpotlightMaxLength;
	}
	else
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + m_vSpotlightDir * m_flSpotlightMaxLength, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		vTargetPos = tr.endpos;
	}

	m_hSpotlightTarget = (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_hSpotlightTarget->Spawn();
	m_hSpotlightTarget->SetAbsOrigin( vTargetPos );
	m_hSpotlightTarget->SetOwnerEntity( this );
	m_hSpotlightTarget->m_clrRender = m_clrRender;
	m_hSpotlightTarget->m_Radius = m_flSpotlightMaxLength;

	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}

	//m_hSpotlight = CBeam::BeamCreate( "sprites/spotlight.vmt", m_flSpotlightGoalWidth );
	m_hSpotlight = CBeam::BeamCreate( "sprites/glow_test02.vmt", m_flSpotlightGoalWidth );
	// Set the temporary spawnflag on the beam so it doesn't save (we'll recreate it on restore)
	m_hSpotlight->SetHDRColorScale( m_flHDRColorScale );
	m_hSpotlight->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_hSpotlight->SetColor( m_clrRender->r, m_clrRender->g, m_clrRender->b ); 
	m_hSpotlight->SetHaloTexture(m_nHaloSprite);
	m_hSpotlight->SetHaloScale(60);
	m_hSpotlight->SetEndWidth(m_flSpotlightGoalWidth);
	m_hSpotlight->SetBeamFlags( (FBEAM_SHADEOUT|FBEAM_NOTILE) );
	m_hSpotlight->SetBrightness( 64 );
	m_hSpotlight->SetNoise( 0 );
	m_hSpotlight->SetMinDXLevel( m_nMinDXLevel );

	if ( m_bEfficientSpotlight )
	{
		m_hSpotlight->PointsInit( GetAbsOrigin(), m_hSpotlightTarget->GetAbsOrigin() );
	}
	else
	{
		m_hSpotlight->EntsInit( this, m_hSpotlightTarget );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CPointSpotlight::SpotlightCurrentPos(void)
{
	AngleVectors( GetAbsAngles(), &m_vSpotlightDir );

	//	Get beam end point.  Only collide with solid objects, not npcs
	Vector vEndPos = GetAbsOrigin() + ( m_vSpotlightDir * 2 * m_flSpotlightMaxLength );
	if ( m_bIgnoreSolid )
	{
		return vEndPos;
	}
	else
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), vEndPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		return tr.endpos;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightDestroy(void)
{
	if ( m_hSpotlight )
	{
		m_OnOff.FireOutput( this, this );

		UTIL_Remove(m_hSpotlight);
		UTIL_Remove(m_hSpotlightTarget);
	}
}

//------------------------------------------------------------------------------
// Purpose : Update the direction and position of my spotlight
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightUpdate(void)
{
	// ---------------------------------------------------
	//  If I don't have a spotlight attempt to create one
	// ---------------------------------------------------
	if ( !m_hSpotlight )
	{
		if ( m_bSpotlightOn )
		{
			// Make the spotlight
			SpotlightCreate();
		}
		else
		{
			return;
		}
	}
	else if ( !m_bSpotlightOn )
	{
		SpotlightDestroy();
		return;
	}
	
	if ( !m_hSpotlightTarget )
	{
		DevWarning( "**Attempting to update point_spotlight but target ent is NULL\n" );
		SpotlightDestroy();
		SpotlightCreate();
		if ( !m_hSpotlightTarget )
			return;
	}

	m_vSpotlightCurrentPos = SpotlightCurrentPos();

	//  Update spotlight target velocity
	Vector vTargetDir;
	VectorSubtract( m_vSpotlightCurrentPos, m_hSpotlightTarget->GetAbsOrigin(), vTargetDir );
	float vTargetDist = vTargetDir.Length();

	// If we haven't moved at all, don't recompute
	if ( vTargetDist < 1 )
	{
		m_hSpotlightTarget->SetAbsVelocity( vec3_origin );
		return;
	}

	Vector vecNewVelocity = vTargetDir;
	VectorNormalize(vecNewVelocity);
	vecNewVelocity *= (10 * vTargetDist);

	// If a large move is requested, just jump to final spot as we probably hit a discontinuity
	if (vecNewVelocity.Length() > 200)
	{
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= 200;
		VectorNormalize(vTargetDir);
		m_hSpotlightTarget->SetAbsOrigin( m_vSpotlightCurrentPos );
	}
	m_hSpotlightTarget->SetAbsVelocity( vecNewVelocity );
	m_hSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();

	// Avoid sudden change in where beam fades out when cross disconinuities
	VectorSubtract( m_hSpotlightTarget->GetAbsOrigin(), m_hSpotlightTarget->m_vSpotlightOrg, m_hSpotlightTarget->m_vSpotlightDir );
	float flBeamLength	= VectorNormalize( m_hSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = (0.60*m_flSpotlightCurLength) + (0.4*flBeamLength);

	ComputeRenderInfo();

	//NDebugOverlay::Cross3D(GetAbsOrigin(),Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
	//NDebugOverlay::Cross3D(m_vSpotlightCurrentPos,Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
	//NDebugOverlay::Cross3D(m_vSpotlightTargetPos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::InputLightOn( inputdata_t &inputdata )
{
	if ( !m_bSpotlightOn )
	{
		m_bSpotlightOn = true;
		if ( m_bEfficientSpotlight )
		{
			CreateEfficientSpotlight();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::InputLightOff( inputdata_t &inputdata )
{
	if ( m_bSpotlightOn )
	{
		m_bSpotlightOn = false;
		if ( m_bEfficientSpotlight )
		{
			SpotlightDestroy();
		}
	}
}
