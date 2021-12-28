//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "iviewrender_beams.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_te_legacytempents.h"
#include "cl_mat_stub.h"
#include "tier0/vprof.h"
#include "iclientvehicle.h"
#include "engine/IEngineTrace.h"
#include "mathlib/vmatrix.h"
#include "rendertexture.h"
#include "c_world.h"
#include <KeyValues.h>
#include "igameevents.h"
#include "smoke_fog_overlay.h"
#include "bitmap/tgawriter.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/replay_screenshot.h"
#endif
#include "input.h"
#include "filesystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "toolframework_client.h"
#include "tier0/icommandline.h"
#include "ienginevgui.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "ScreenSpaceEffects.h"
#include "sourcevr/isourcevirtualreality.h"
#include "client_virtualreality.h"

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

#if defined( HL2_CLIENT_DLL ) || defined( CSTRIKE_DLL )
#define USE_MONITORS
#endif

#ifdef PORTAL
#include "c_prop_portal.h" //portal surface rendering functions
#endif

#include "../aarcade/client/c_anarchymanager.h"	// Added for Anarchy Arcade
	
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
		  
void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height );
bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );
bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles );


extern ConVar default_fov;
extern bool g_bRenderingScreenshot;

#if !defined( _X360 )
#define SAVEGAME_SCREENSHOT_WIDTH	180
#define SAVEGAME_SCREENSHOT_HEIGHT	100
#else
#define SAVEGAME_SCREENSHOT_WIDTH	128
#define SAVEGAME_SCREENSHOT_HEIGHT	128
#endif

#ifndef _XBOX
extern ConVar sensitivity;
#endif

ConVar zoom_sensitivity_ratio( "zoom_sensitivity_ratio", "1.0", 0, "Additional mouse sensitivity scale factor applied when FOV is zoomed in." );

CViewRender g_DefaultViewRender;
IViewRender *view = NULL;	// set in cldll_client_init.cpp if no mod creates their own

#if _DEBUG
bool g_bRenderingCameraView = false;
#endif


// These are the vectors for the "main" view - the one the player is looking down.
// For stereo views, they are the vectors for the middle eye.
static Vector g_vecRenderOrigin(0,0,0);
static QAngle g_vecRenderAngles(0,0,0);
static Vector g_vecPrevRenderOrigin(0,0,0);	// Last frame's render origin
static QAngle g_vecPrevRenderAngles(0,0,0); // Last frame's render angles
static Vector g_vecVForward(0,0,0), g_vecVRight(0,0,0), g_vecVUp(0,0,0);
static VMatrix g_matCamInverse;

extern ConVar cl_forwardspeed;

static ConVar v_centermove( "v_centermove", "0.15");
static ConVar v_centerspeed( "v_centerspeed","500" );

#ifdef TF_CLIENT_DLL
// 54 degrees approximates a 35mm camera - we determined that this makes the viewmodels
// and motions look the most natural.
ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_ARCHIVE, "Sets the field-of-view for the viewmodel.", true, 0.1, true, 179.9 );
#else
ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_CHEAT, "Sets the field-of-view for the viewmodel.", true, 0.1, true, 179.9 );
#endif
ConVar mat_viewportscale( "mat_viewportscale", "1.0", FCVAR_ARCHIVE, "Scale down the main viewport (to reduce GPU impact on CPU profiling)", true, (1.0f / 640.0f), true, 1.0f );
ConVar mat_viewportupscale( "mat_viewportupscale", "1", FCVAR_ARCHIVE, "Scale the viewport back up" );
ConVar cl_leveloverview( "cl_leveloverview", "0", FCVAR_CHEAT );

static ConVar r_mapextents( "r_mapextents", "16384", FCVAR_CHEAT, 
						   "Set the max dimension for the map.  This determines the far clipping plane" );

// UNDONE: Delete this or move to the material system?
ConVar	gl_clear( "gl_clear", "0");
ConVar	gl_clear_randomcolor( "gl_clear_randomcolor", "0", FCVAR_CHEAT, "Clear the back buffer to random colors every frame. Helps spot open seams in geometry." );

static ConVar r_farz( "r_farz", "-1", FCVAR_CHEAT, "Override the far clipping plane. -1 means to use the value in env_fog_controller." );
static ConVar cl_demoviewoverride( "cl_demoviewoverride", "0", 0, "Override view during demo playback" );


void SoftwareCursorChangedCB( IConVar *pVar, const char *pOldValue, float fOldValue )
{
	ConVar *pConVar = (ConVar *)pVar;
	vgui::surface()->SetSoftwareCursor( pConVar->GetBool() || UseVR() );
}
static ConVar cl_software_cursor ( "cl_software_cursor", "0", FCVAR_ARCHIVE, "Switches the game to use a larger software cursor instead of the normal OS cursor", SoftwareCursorChangedCB );


static Vector s_DemoView;
static QAngle s_DemoAngle;

static void CalcDemoViewOverride( Vector &origin, QAngle &angles )
{
	engine->SetViewAngles( s_DemoAngle );

	input->ExtraMouseSample( gpGlobals->absoluteframetime, true );

	engine->GetViewAngles( s_DemoAngle );

	Vector forward, right, up;

	AngleVectors( s_DemoAngle, &forward, &right, &up );

	float speed = gpGlobals->absoluteframetime * cl_demoviewoverride.GetFloat() * 320;
	
	s_DemoView += speed * input->KeyState (&in_forward) * forward  ;
	s_DemoView -= speed * input->KeyState (&in_back) * forward ;

	s_DemoView += speed * input->KeyState (&in_moveright) * right ;
	s_DemoView -= speed * input->KeyState (&in_moveleft) * right ;

	origin = s_DemoView;
	angles = s_DemoAngle;
}



// Selects the relevant member variable to update. You could do it manually, but...
// We always set up the MONO eye, even when doing stereo, and it's set up to be mid-way between the left and right,
// so if you don't really care about L/R (e.g. culling, sound, etc), just use MONO.
// Added for Anarchy Arcade
// This entire method is modified.
CViewSetup &CViewRender::GetView(StereoEye_t eEye)
{
	if ( eEye == STEREO_EYE_MONO )
    {
		return m_View;
    }
	/*
	else
	{
		return m_View;
	}
	*/
	else if ( eEye == STEREO_EYE_RIGHT )
	{
		//return m_ViewLeft;
		return m_View;
    }
	else
	{
		return m_View;
		//return m_ViewRight;
		//return m_ViewLeft;
		//Assert(eEye == STEREO_EYE_LEFT);
		//return m_ViewLeft;
		//return m_ViewRight;
		//return m_ViewRight;
    }
}

const CViewSetup &CViewRender::GetView(StereoEye_t eEye) const
{
    return (const_cast<CViewRender*>(this))->GetView ( eEye );
}


//-----------------------------------------------------------------------------
// Accessors to return the main view (where the player's looking)
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin()
{
	return g_vecRenderOrigin;
}

const QAngle &MainViewAngles()
{
	return g_vecRenderAngles;
}

const Vector &MainViewForward()
{
	return g_vecVForward;
}

const Vector &MainViewRight()
{
	return g_vecVRight;
}

const Vector &MainViewUp()
{
	return g_vecVUp;
}

const VMatrix &MainWorldToViewMatrix()
{
	return g_matCamInverse;
}

const Vector &PrevMainViewOrigin()
{
	return g_vecPrevRenderOrigin;
}

const QAngle &PrevMainViewAngles()
{
	return g_vecPrevRenderAngles;
}

//-----------------------------------------------------------------------------
// Compute the world->camera transform
//-----------------------------------------------------------------------------
void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse )
{
	// Compute view bases
	AngleVectors( vecAngles, pVecForward, pVecRight, pVecUp );

	for (int i = 0; i < 3; ++i)
	{
		(*pMatCamInverse)[0][i] = (*pVecRight)[i];	
		(*pMatCamInverse)[1][i] = (*pVecUp)[i];	
		(*pMatCamInverse)[2][i] = -(*pVecForward)[i];	
		(*pMatCamInverse)[3][i] = 0.0F;
	}
	(*pMatCamInverse)[0][3] = -DotProduct( *pVecRight, vecOrigin );
	(*pMatCamInverse)[1][3] = -DotProduct( *pVecUp, vecOrigin );
	(*pMatCamInverse)[2][3] =  DotProduct( *pVecForward, vecOrigin );
	(*pMatCamInverse)[3][3] = 1.0F;
}


bool R_CullSphere(
	VPlane const *pPlanes,
	int nPlanes,
	Vector const *pCenter,
	float radius)
{
	for(int i=0; i < nPlanes; i++)
		if(pPlanes[i].DistTo(*pCenter) < -radius)
			return true;
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void StartPitchDrift( void )
{
	view->StartPitchDrift();
}

static ConCommand centerview( "centerview", StartPitchDrift );

extern ConVar default_fov;



//-----------------------------------------------------------------------------
// Purpose: Initializes all view systems
//-----------------------------------------------------------------------------
void CViewRender::Init( void )
{
	memset( &m_PitchDrift, 0, sizeof( m_PitchDrift ) );

	m_bDrawOverlay = false;

	m_pDrawEntities		= cvar->FindVar( "r_drawentities" );
	m_pDrawBrushModels	= cvar->FindVar( "r_drawbrushmodels" );

	beams->InitBeams();
	tempents->Init();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
	m_ModulateSingleColor.Init( "engine/modulatesinglecolor", TEXTURE_GROUP_OTHER );
	
	extern CMaterialReference g_material_WriteZ;
	g_material_WriteZ.Init( "engine/writez", TEXTURE_GROUP_OTHER );

	// FIXME:  
	QAngle angles;
	engine->GetViewAngles( angles );
	AngleVectors( angles, &m_vecLastFacing );

#if defined( REPLAY_ENABLED )
	m_pReplayScreenshotTaker = NULL;
#endif

#if defined( CSTRIKE_DLL )
	m_flLastFOV = default_fov.GetFloat();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelInit( void )
{
	beams->ClearBeams();
	tempents->Clear();

	m_BuildWorldListsNumber = 0;
	m_BuildRenderableListsNumber = 0;

	for( int i=0; i < STEREO_EYE_MAX; i++ )
	{
		m_rbTakeFreezeFrame[ i ] = false;
	}
	m_flFreezeFrameUntil = 0;

	// Clear our overlay materials
	m_ScreenOverlayMaterial.Init( NULL );

	// Init all IScreenSpaceEffects
	g_pScreenSpaceEffects->InitScreenSpaceEffects( );
}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelShutdown( void )
{
	g_pScreenSpaceEffects->ShutdownScreenSpaceEffects( );
}

//-----------------------------------------------------------------------------
// Purpose: Called at shutdown
//-----------------------------------------------------------------------------
void CViewRender::Shutdown( void )
{
	m_TranslucentSingleColor.Shutdown( );
	m_ModulateSingleColor.Shutdown( );
	m_ScreenOverlayMaterial.Shutdown();
	m_UnderWaterOverlayMaterial.Shutdown();
	beams->ShutdownBeams();
	tempents->Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the worldlists build number
//-----------------------------------------------------------------------------

int CViewRender::BuildWorldListsNumber( void ) const
{
	return m_BuildWorldListsNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Start moving pitch toward ideal
//-----------------------------------------------------------------------------
void CViewRender::StartPitchDrift (void)
{
	if ( m_PitchDrift.laststop == gpGlobals->curtime )
	{
		// Something else is blocking the drift.
		return;		
	}

	if ( m_PitchDrift.nodrift || !m_PitchDrift.pitchvel )
	{
		m_PitchDrift.pitchvel	= v_centerspeed.GetFloat();
		m_PitchDrift.nodrift	= false;
		m_PitchDrift.driftmove	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::StopPitchDrift (void)
{
	m_PitchDrift.laststop	= gpGlobals->curtime;
	m_PitchDrift.nodrift	= true;
	m_PitchDrift.pitchvel	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the client pitch angle towards cl.idealpitch sent by the server.
// If the user is adjusting pitch manually, either with lookup/lookdown,
//   mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//-----------------------------------------------------------------------------
void CViewRender::DriftPitch (void)
{
	float		delta, move;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player || g_pAnarchyManager->IsVRActive())	// Added for Anarchy Arcade
		return;

#if defined( REPLAY_ENABLED )
	if ( engine->IsHLTV() || g_pEngineClientReplay->IsPlayingReplayDemo() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#else
	if ( engine->IsHLTV() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#endif
	{
		m_PitchDrift.driftmove = 0;
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Don't count small mouse motion
	if ( m_PitchDrift.nodrift )
	{
		if ( fabs( input->GetLastForwardMove() ) < cl_forwardspeed.GetFloat() )
		{
			m_PitchDrift.driftmove = 0;
		}
		else
		{
			m_PitchDrift.driftmove += gpGlobals->frametime;
		}
	
		if ( m_PitchDrift.driftmove > v_centermove.GetFloat() )
		{
			StartPitchDrift ();
		}
		return;
	}
	
	// How far off are we
	delta = prediction->GetIdealPitch() - player->GetAbsAngles()[ PITCH ];
	if ( !delta )
	{
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Determine movement amount
	move = gpGlobals->frametime * m_PitchDrift.pitchvel;
	// Accelerate
	m_PitchDrift.pitchvel += gpGlobals->frametime * v_centerspeed.GetFloat();
	
	// Move predicted pitch appropriately
	if (delta > 0)
	{
		if ( move > delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() + QAngle( move, 0, 0 ) );
	}
	else if ( delta < 0 )
	{
		if ( move > -delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = -delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() - QAngle( move, 0, 0 ) );
	}
}



StereoEye_t		CViewRender::GetFirstEye() const
{
	if (g_pAnarchyManager->UseSBSRendering() && (!g_pAnarchyManager->IsVRActive() || g_pAnarchyManager->GetVRHMDRender()) && (!g_pAnarchyManager->IsVRActive() || g_pAnarchyManager->VRSpectatorMode() == 0 || g_pAnarchyManager->VRSpectatorMode() == 2)) // Added for Anarchy Arcade
		return STEREO_EYE_LEFT;
	else
		return STEREO_EYE_MONO;
}

StereoEye_t		CViewRender::GetLastEye() const
{
	if (g_pAnarchyManager->UseSBSRendering() && (!g_pAnarchyManager->IsVRActive() || g_pAnarchyManager->GetVRHMDRender())) // Added for Anarchy Arcade
		return STEREO_EYE_RIGHT;
	else
		return STEREO_EYE_MONO;
}




// This is called by cdll_client_int to setup view model origins. This has to be done before
// simulation so entities can access attachment points on view models during simulation.
void CViewRender::OnRenderStart()
{
	VPROF_("CViewRender::OnRenderStart", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);

    SetUpViews();

	// Adjust mouse sensitivity based upon the current FOV
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player )
	{
		default_fov.SetValue( player->m_iDefaultFOV );

		//Update our FOV, including any zooms going on
		int iDefaultFOV = default_fov.GetInt();
		int	localFOV	= player->GetFOV();
		int min_fov		= player->GetMinFOV();

		// Don't let it go too low
		localFOV = MAX( min_fov, localFOV );

		gHUD.m_flFOVSensitivityAdjust = 1.0f;
#ifndef _XBOX
		if ( gHUD.m_flMouseSensitivityFactor )
		{
			gHUD.m_flMouseSensitivity = sensitivity.GetFloat() * gHUD.m_flMouseSensitivityFactor;
		}
		else
#endif
		{
			// No override, don't use huge sensitivity
			if ( localFOV == iDefaultFOV )
			{
#ifndef _XBOX
				// reset to saved sensitivity
				gHUD.m_flMouseSensitivity = 0;
#endif
			}
			else
			{  
				// Set a new sensitivity that is proportional to the change from the FOV default and scaled
				//  by a separate compensating factor
				if ( iDefaultFOV == 0 )
				{
					Assert(0); // would divide by zero, something is broken with iDefatulFOV
					iDefaultFOV = 1;
				}
				gHUD.m_flFOVSensitivityAdjust = 
					((float)localFOV / (float)iDefaultFOV) * // linear fov downscale
					zoom_sensitivity_ratio.GetFloat(); // sensitivity scale factor
#ifndef _XBOX
				gHUD.m_flMouseSensitivity = gHUD.m_flFOVSensitivityAdjust * sensitivity.GetFloat(); // regular sensitivity
#endif
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetViewSetup( void ) const
{
	return &m_CurrentView;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetPlayerViewSetup( void ) const
{   
    const CViewSetup &view = GetView ( STEREO_EYE_MONO );
    return &view;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DisableVis( void )
{
	m_bForceNoVis = true;
}

#ifdef DBGFLAG_ASSERT
static Vector s_DbgSetupOrigin;
static QAngle s_DbgSetupAngles;
#endif

//-----------------------------------------------------------------------------
// Gets znear + zfar
//-----------------------------------------------------------------------------
float CViewRender::GetZNear()
{
	return VIEW_NEARZ;
}

float CViewRender::GetZFar()
{
	// Initialize view structure with default values
	float farZ;
	if ( r_farz.GetFloat() < 1 )
	{
		// Use the far Z from the map's parameters.
		farZ = r_mapextents.GetFloat() * 1.73205080757f;
		
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer && pPlayer->GetFogParams() )
		{
			if ( pPlayer->GetFogParams()->farz > 0 )
			{
				farZ = pPlayer->GetFogParams()->farz;
			}
		}
	}
	else
	{
		farZ = r_farz.GetFloat();
	}

	return farZ;
}


//-----------------------------------------------------------------------------
// Sets up the view parameters
//-----------------------------------------------------------------------------
void CViewRender::SetUpViews()
{
	VPROF("CViewRender::SetUpViews");

	// Initialize view structure with default values
	float farZ = GetZFar();

    // Set up the mono/middle view.
    CViewSetup &view = m_View;

	view.zFar				= farZ;
	view.zFarViewmodel	    = farZ;
	// UNDONE: Make this farther out? 
	//  closest point of approach seems to be view center to top of crouched box
	view.zNear			    = GetZNear();
	view.zNearViewmodel	    = 1;
	view.fov				= default_fov.GetFloat();

	view.m_bOrtho			= false;
    view.m_bViewToProjectionOverride = false;
	view.m_eStereoEye		= STEREO_EYE_MONO;

	// Enable spatial partition access to edicts
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// You in-view weapon aim.
	bool bCalcViewModelView = false;
	Vector ViewModelOrigin;
	QAngle ViewModelAngles;

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->CalcView( view.origin, view.angles, view.fov );
	}
#if defined( REPLAY_ENABLED )
	else if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
	{
		ReplayCamera()->CalcView( view.origin, view.angles, view.fov );
	}
#endif
	else
	{
		// FIXME: Are there multiple views? If so, then what?
		// FIXME: What happens when there's no player?
		if (pPlayer)
		{
			pPlayer->CalcView( view.origin, view.angles, view.zNear, view.zFar, view.fov );

			// If we are looking through another entities eyes, then override the angles/origin for view
			int viewentity = render->GetViewEntity();

			if ( !g_nKillCamMode && (pPlayer->entindex() != viewentity) )
			{
				C_BaseEntity *ve = cl_entitylist->GetEnt( viewentity );
				if ( ve )
				{
					VectorCopy( ve->GetAbsOrigin(), view.origin );
					VectorCopy( ve->GetAbsAngles(), view.angles );
				}
			}

			// There is a viewmodel.
			bCalcViewModelView = true;
			ViewModelOrigin = view.origin;
			ViewModelAngles = view.angles;
		}
		else
		{
			view.origin.Init();
			view.angles.Init();
		}

		// Even if the engine is paused need to override the view
		// for keeping the camera control during pause.
		g_pClientMode->OverrideView( &view );
	}

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( view.origin, view.angles, view.fov );

	if ( engine->IsPlayingDemo() )
	{
		if ( cl_demoviewoverride.GetFloat() > 0.0f )
		{
			// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
			CalcDemoViewOverride( view.origin, view.angles );
		}
		else
		{
			s_DemoView = view.origin;
			s_DemoAngle = view.angles;
		}
	}

	//Find the offset our current FOV is from the default value
	float fDefaultFov = default_fov.GetFloat();
	float flFOVOffset = fDefaultFov - view.fov;

	//Adjust the viewmodel's FOV to move with any FOV offsets on the viewer's end
	view.fovViewmodel = g_pClientMode->GetViewModelFOV() - flFOVOffset;

	if (g_pAnarchyManager->UseSBSRendering() || UseVR() )	// Added for Anarchy Arcade
	{
		// Let the headtracking read the status of the HMD, etc.
		// This call can go almost anywhere, but it needs to know the player FOV for sniper weapon zoom, etc
		/*
		if ( flFOVOffset == 0.0f )
		{
			g_ClientVirtualReality.ProcessCurrentTrackingState ( 0.0f );
		}
		else
		{
			g_ClientVirtualReality.ProcessCurrentTrackingState ( view.fov );
		}
		*/

		//HeadtrackMovementMode_t hmmOverrideMode = g_pClientMode->ShouldOverrideHeadtrackControl();
		//g_ClientVirtualReality.OverrideView( &m_View, &ViewModelOrigin, &ViewModelAngles, hmmOverrideMode );

		// left and right stereo views should default to being the same as the mono/middle view
		m_ViewLeft = m_View;
		m_ViewRight = m_View;
		m_ViewLeft.m_eStereoEye = STEREO_EYE_LEFT;
		m_ViewRight.m_eStereoEye = STEREO_EYE_RIGHT;

		//m_ViewLeft.origin.x = m_View.origin.x - 10.0;
		//m_ViewRight.origin.x = m_View.origin.x + 10.0;

		//g_ClientVirtualReality.OverrideStereoView(&m_View, &m_ViewLeft, &m_ViewRight);
		//g_pAnarchyManager->OverrideStereoView(&m_View, &m_ViewLeft, &m_ViewRight);
	}
	else
	{
		// left and right stereo views should default to being the same as the mono/middle view
		m_ViewLeft = m_View;
		m_ViewRight = m_View;
		m_ViewLeft.m_eStereoEye = STEREO_EYE_LEFT;
		m_ViewRight.m_eStereoEye = STEREO_EYE_RIGHT;
	}

	if ( bCalcViewModelView )
	{
		Assert ( pPlayer != NULL );
		pPlayer->CalcViewModelView ( ViewModelOrigin, ViewModelAngles );
	}

	// Disable spatial partition access
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

	// Enable access to all model bones
	C_BaseAnimating::PopBoneAccess( "OnRenderStart->CViewRender::SetUpView" ); // pops the (true, false) bone access set in OnRenderStart
	C_BaseAnimating::PushAllowBoneAccess( true, true, "CViewRender::SetUpView->OnRenderEnd" ); // pop is in OnRenderEnd()

	// Compute the world->main camera transform
    // This is only done for the main "middle-eye" view, not for the various other views.
	ComputeCameraVariables( view.origin, view.angles, 
		&g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

	// set up the hearing origin...
	AudioState_t audioState;
	audioState.m_Origin = view.origin;
	audioState.m_Angles = view.angles;
	audioState.m_bIsUnderwater = pPlayer && pPlayer->AudioStateIsUnderwater( view.origin );

	ToolFramework_SetupAudioState( audioState );

    // TomF: I wonder when the audio tools modify this, if ever...
    Assert ( view.origin == audioState.m_Origin );
    Assert ( view.angles == audioState.m_Angles );
	view.origin = audioState.m_Origin;
	view.angles = audioState.m_Angles;

	engine->SetAudioState( audioState );

	g_vecPrevRenderOrigin = g_vecRenderOrigin;
	g_vecPrevRenderAngles = g_vecRenderAngles;
	g_vecRenderOrigin = view.origin;
	g_vecRenderAngles = view.angles;

#ifdef DBGFLAG_ASSERT
	s_DbgSetupOrigin = view.origin;
	s_DbgSetupAngles = view.angles;
#endif
}




void CViewRender::WriteSaveGameScreenshotOfSize( const char *pFilename, int width, int height, bool bCreatePowerOf2Padded/*=false*/,
												 bool bWriteVTF/*=false*/ )
{
#ifndef _X360
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();

	g_bRenderingScreenshot = true;

	// Push back buffer on the stack with small viewport
	pRenderContext->PushRenderTargetAndViewport( NULL, 0, 0, width, height );

	// render out to the backbuffer
    CViewSetup viewSetup = GetView ( STEREO_EYE_MONO );
	viewSetup.x = 0;
	viewSetup.y = 0;
	viewSetup.width = width;
	viewSetup.height = height;
	viewSetup.fov = ScaleFOVByWidthRatio( viewSetup.fov, ( (float)width / (float)height ) / ( 4.0f / 3.0f ) );
	viewSetup.m_bRenderToSubrectOfLargerScreen = true;

	// draw out the scene
	// Don't draw the HUD or the viewmodel
	RenderView( viewSetup, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, 0 );

	// get the data from the backbuffer and save to disk
	// bitmap bits
	unsigned char *pImage = ( unsigned char * )malloc( width * height * 3 );

	// Get Bits from the material system
	pRenderContext->ReadPixels( 0, 0, width, height, pImage, IMAGE_FORMAT_RGB888 );

	// Some stuff to be setup dependent on padded vs. not padded
	int nSrcWidth, nSrcHeight;
	unsigned char *pSrcImage;

	// Create a padded version if necessary
	unsigned char *pPaddedImage = NULL;
	if ( bCreatePowerOf2Padded )
	{
		// Setup dimensions as needed
		int nPaddedWidth = SmallestPowerOfTwoGreaterOrEqual( width );
		int nPaddedHeight = SmallestPowerOfTwoGreaterOrEqual( height );

		// Allocate
		int nPaddedImageSize = nPaddedWidth * nPaddedHeight * 3;
		pPaddedImage = ( unsigned char * )malloc( nPaddedImageSize );
		
		// Zero out the entire thing
		V_memset( pPaddedImage, 255, nPaddedImageSize );

		// Copy over each row individually
		for ( int nRow = 0; nRow < height; ++nRow )
		{
			unsigned char *pDst = pPaddedImage + 3 * ( nRow * nPaddedWidth );
			const unsigned char *pSrc = pImage + 3 * ( nRow * width );
			V_memcpy( pDst, pSrc, 3 * width );
		}

		// Setup source data
		nSrcWidth = nPaddedWidth;
		nSrcHeight = nPaddedHeight;
		pSrcImage = pPaddedImage;
	}
	else
	{
		// Use non-padded info
		nSrcWidth = width;
		nSrcHeight = height;
		pSrcImage = pImage;
	}

	// allocate a buffer to write the tga into
	CUtlBuffer buffer;

	bool bWriteResult;
	if ( bWriteVTF )
	{
		// Create and initialize a VTF texture
		IVTFTexture *pVTFTexture = CreateVTFTexture();
		const int nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_SRGB;
		if ( pVTFTexture->Init( nSrcWidth, nSrcHeight, 1, IMAGE_FORMAT_RGB888, nFlags, 1, 1 ) )
		{
			// Copy the image data over to the VTF
			unsigned char *pDestBits = pVTFTexture->ImageData();
			int nDstSize = nSrcWidth * nSrcHeight * 3;
			V_memcpy( pDestBits, pSrcImage, nDstSize );

			// Allocate output buffer
			int iMaxVTFSize = 1024 + ( nSrcWidth * nSrcHeight * 3 );
			void *pVTF = malloc( iMaxVTFSize );
			buffer.SetExternalBuffer( pVTF, iMaxVTFSize, 0 );

			// Serialize to the buffer
			bWriteResult = pVTFTexture->Serialize( buffer );
		
			// Free the VTF texture
			DestroyVTFTexture( pVTFTexture );
		}
		else
		{
			bWriteResult = false;
		}
	}
	else
	{
		// Write TGA format to buffer
		int iMaxTGASize = 1024 + ( nSrcWidth * nSrcHeight * 4 );
		void *pTGA = malloc( iMaxTGASize );
		buffer.SetExternalBuffer( pTGA, iMaxTGASize, 0 );

		bWriteResult = TGAWriter::WriteToBuffer( pSrcImage, buffer, nSrcWidth, nSrcHeight, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
	}

	if ( !bWriteResult )
	{
		//Error( "Couldn't write bitmap data snapshot.\n" );	// Added for Anarchy Arcade
		DevMsg("Couldn't write bitmap data snapshot.\n");	// Added for Anarchy Arcade
	}
	
	free( pImage );
	free( pPaddedImage );

	// async write to disk (this will take ownership of the memory)
	char szPathedFileName[_MAX_PATH];
	Q_snprintf( szPathedFileName, sizeof(szPathedFileName), "//MOD/%s", pFilename );

	filesystem->AsyncWrite( szPathedFileName, buffer.Base(), buffer.TellPut(), true );

	// restore our previous state
	pRenderContext->PopRenderTargetAndViewport();
	
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	g_bRenderingScreenshot = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot for the replay system
//-----------------------------------------------------------------------------
void CViewRender::WriteReplayScreenshot( WriteReplayScreenshotParams_t &params )
{
#if defined( REPLAY_ENABLED )
	if ( !m_pReplayScreenshotTaker )
		return;

	m_pReplayScreenshotTaker->TakeScreenshot( params );
#endif
}

void CViewRender::UpdateReplayScreenshotCache()
{
#if defined( REPLAY_ENABLED )
	// Delete the old one
	delete m_pReplayScreenshotTaker;

	// Create a new one
	m_pReplayScreenshotTaker = new CReplayScreenshotTaker( this, GetView ( STEREO_EYE_MONO ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot of the save game
//-----------------------------------------------------------------------------
void CViewRender::WriteSaveGameScreenshot( const char *pFilename )
{
	WriteSaveGameScreenshotOfSize( pFilename, SAVEGAME_SCREENSHOT_WIDTH, SAVEGAME_SCREENSHOT_HEIGHT );
}


float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets view parameters for level overview mode
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::SetUpOverView()
{
	static int oldCRC = 0;

    CViewSetup &view = GetView ( STEREO_EYE_MONO );

	view.m_bOrtho = true;

	float aspect = (float)view.width/(float)view.height;

	int size_y = 1024.0f * cl_leveloverview.GetFloat(); // scale factor, 1024 = OVERVIEW_MAP_SIZE
	int	size_x = size_y * aspect;	// standard screen aspect 

	view.origin.x -= size_x / 2;
	view.origin.y += size_y / 2;

	view.m_OrthoLeft   = 0;
	view.m_OrthoTop    = -size_y;
	view.m_OrthoRight  = size_x;
	view.m_OrthoBottom = 0;

	view.angles = QAngle( 90, 90, 0 );

	// simple movement detector, show position if moved
	int newCRC = view.origin.x + view.origin.y + view.origin.z;
	if ( newCRC != oldCRC )
	{
		Msg( "Overview: scale %.2f, pos_x %.0f, pos_y %.0f\n", cl_leveloverview.GetFloat(),
			view.origin.x, view.origin.y );
		oldCRC = newCRC;
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->ClearColor4ub( 0, 255, 0, 255 );

	// render->DrawTopView( true );
}

//-----------------------------------------------------------------------------
// Purpose: Render current view into specified rectangle
// Input  : *rect - is computed by CVideoMode_Common::GetClientViewRect()
//-----------------------------------------------------------------------------
void CViewRender::Render( vrect_t *rect )
{
	//Assert(s_DbgSetupOrigin == m_View.origin);
	//Assert(s_DbgSetupAngles == m_View.angles);

	// Added for Anarchy Arcade
	if (g_pAnarchyManager->IsPaused())
		return;

	VPROF_BUDGET( "CViewRender::Render", "CViewRender::Render" );
	tmZone(TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	
	m_View.zNear = g_pAnarchyManager->GetZNear();	// Added for Anarchy Arcade
	/*
	if (g_pAnarchyManager->IsVRActive())
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		VMatrix playerMatrix;
		playerMatrix.SetupMatrixOrgAngles(pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles());

		C_DynamicProp* pVRHandLeft = g_pAnarchyManager->GetVRHand(0);
		if (pVRHandLeft)
		{
			VMatrix handMatrix = g_pAnarchyManager->GetVRHandMatrix(0);

			Vector origin = handMatrix.GetTranslation();
			origin.z += 64.0;
			handMatrix.SetTranslation(origin);

			handMatrix = playerMatrix * handMatrix;
			origin = handMatrix.GetTranslation();

			QAngle angles;
			MatrixAngles(handMatrix.As3x4(), angles);

			engine->ServerCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pVRHandLeft->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z), false);
		}

		C_DynamicProp* pVRHandRight = g_pAnarchyManager->GetVRHand(1);
		if (pVRHandRight)
		{
			VMatrix handMatrix = g_pAnarchyManager->GetVRHandMatrix(1);

			Vector origin = handMatrix.GetTranslation();
			origin.z += 64.0;
			handMatrix.SetTranslation(origin);

			handMatrix = playerMatrix * handMatrix;
			origin = handMatrix.GetTranslation();

			QAngle angles;
			MatrixAngles(handMatrix.As3x4(), angles);

			engine->ServerCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pVRHandRight->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z), false);
		}
	}
	*/
	vrect_t vr = *rect;

	// Stub out the material system if necessary.
	CMatStubHandler matStub;

	engine->EngineStats_BeginFrame();
	
	// Assume normal vis
	m_bForceNoVis			= false;
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	//m_View.angles.x = 0;
	//m_View.angles.z = 0;
	//engine->ClientCmd(VarArgs("setang %f %f %f; ", 0, m_View.angles.y, 0));

    // Set for console commands, etc.
    render->SetMainView ( m_View.origin, m_View.angles );

	// Added for Anarchy Arcade
	Vector originalOrigin = m_View.origin;// pPlayer->GetAbsOrigin();//m_View.origin;
	QAngle originalAngles = m_View.angles;//pPlayer->GetAbsAngles();// m_View.angles;
	originalAngles.x = 0;
	//originalAngles.z = 0;

	VMatrix originalProjectionMatrix = m_View.m_ViewToProjection;
	StereoEye_t originalStereoEye = m_View.m_eStereoEye;
	Vector foward;
	Vector left;
	Vector up;
	float ipd;
	if (pPlayer)
	{
		pPlayer->EyeVectors(&foward, &left, &up);
		ipd = g_pAnarchyManager->GetIPD() * 0.0393701;
	}
	else
		ipd = 63.0 * 0.0393701;

	//POINT vrres;// = hmdGetResolution();
	//if (g_pAnarchyManager->IsVRActive())
	//{
	//	vrres.x = 1920;// 2160;
	//	vrres.y = 1080;// 1200;
	//}

	for (StereoEye_t eEye = GetLastEye(); eEye >= GetFirstEye(); eEye = (StereoEye_t)(eEye - 1))
	//for (StereoEye_t eEye = GetFirstEye(); eEye <= GetLastEye(); eEye = (StereoEye_t)(eEye + 1))
	{
		//if (g_pAnarchyManager->UseSBSRendering() && eEye != STEREO_EYE_RIGHT)
		//	continue;

		CViewSetup &view = GetView(eEye);

		//if (eEye == STEREO_EYE_MONO)
		//{
		m_View.m_eStereoEye = eEye;
		//if (pPlayer && (eEye == STEREO_EYE_LEFT))
		//{
		//	m_View.origin = originalOrigin;
		//	m_View.angles = originalAngles;
		//}
		//m_View.m_ViewToProjection = originalProjectionMatrix;
		//}

		if (pPlayer && (eEye == STEREO_EYE_LEFT || eEye == STEREO_EYE_RIGHT))//eEye != STEREO_EYE_MONO && 
		{
			//m_View.origin = originalOrigin;
			//m_View.angles = originalAngles;

			if (g_pAnarchyManager->IsVRActive())
			{
				//m_View.angles.x = 0;
				//m_View.angles.z = 0;

				//originalAngles.x = 0;
				//originalAngles.z = 0;

				// Get the headMatrix
				VMatrix headMatrix = g_pAnarchyManager->GetVRHeadMatrix();
				//headMatrix = g_pAnarchyManager->SMMatrixToVMatrix(headMatrix.Base(), false, false);

				//Vector testerOrigin;
				//QAngle testerAngles;
				//MatrixAngles(headMatrix.As3x4(), testerAngles, testerOrigin);
				//DevMsg("origin: %f %f %f\n", testerOrigin.x, testerOrigin.y, testerOrigin.z);

				// Get the eye matrix
				VMatrix eyeMatrix;
				if (eEye == STEREO_EYE_LEFT)
					eyeMatrix = g_pAnarchyManager->GetVRLeftEyeMatrix();
				else
					eyeMatrix = g_pAnarchyManager->GetVRRightEyeMatrix();

				VMatrix goodProjection = VMatrix(eyeMatrix);
				view.m_ViewToProjection = goodProjection;
				/*
				//if (!g_pAnarchyManager->GetShouldInvertVRMatrices())
				eyeMatrix = eyeMatrix.InverseTR();

				int iVal = (eEye == STEREO_EYE_LEFT) ? -1 : 1;
				eyeMatrix = g_pAnarchyManager->SMMatrixToVMatrix(eyeMatrix.Base(), iVal, true);
				*/
				/*
				float flMetersToGameUnits = 39.3701;
				Vector eyeOffset = eyeMatrix.GetTranslation();
				DevMsg("Length: %f %f %f\n", eyeOffset.x, eyeOffset.y, eyeOffset.z);
				eyeOffset.x = (eyeOffset.x * 100.0) / flMetersToGameUnits;
				eyeOffset.y = (eyeOffset.y * 100.0) / flMetersToGameUnits;
				eyeOffset.z = (eyeOffset.z * 100.0) / flMetersToGameUnits;
				*/

				//float xOffset = eyeMatrix.GetTranslation().x;

				// the eyeMatrix always returns 1 (in native VR units) as the x-offset, which gets changed into the conversion ratio upon SMMatrixToMatrix.
				// so let's replace the offset with the actual IPD on the X, but be careful to actually USE the eyematrix provided.
				//eyeMatrix.SetTranslation();

				//Vector eyeOffset = eyeMatrix.GetTranslation();
				//DevMsg("Length: %f %f %f\n", eyeOffset.x, eyeOffset.y, eyeOffset.z);
				//DevMsg("Length: %f vs %f\n", eyeOffset.x, g_pAnarchyManager->GetIPD() * 0.0393701);
				//Vector eyeOffset = eyeMatrix.GetTranslation();



				/*
				float flMetersToGameUnits = 39.3701;
				float flEyeFactor = (eEye == STEREO_EYE_LEFT) ? 1.0 : -1.0;
				Vector eyeOffset;
				eyeOffset.x = 0;
				eyeOffset.y = (ipd / 2.0 * flEyeFactor);
				eyeOffset.z = 0;
				eyeMatrix.SetTranslation(eyeOffset);
				*/



				//VMatrix eyeOffsetMatrix;
				//QAngle eyeRotation;
				//MatrixAngles(eyeMatrix.As3x4(), eyeRotation);
				//eyeOffsetMatrix.SetupMatrixOrgAngles(Vector(eyeOffset.x, eyeOffset.y + (ipd / 2.0 * flEyeFactor), eyeOffset.z), eyeRotation);



				/*
				// get the head's rotation matrix
				QAngle headRotation;
				MatrixAngles(headMatrix.As3x4(), headRotation);
				VMatrix headRotationMatrix;
				headRotationMatrix.Identity();
				headRotationMatrix.SetupMatrixOrgAngles(Vector(0, 0, 0), headRotation);

				// get the head's position
				Vector headOffset = headMatrix.GetTranslation();


				//eyeOffsetMatrix.SetupMatrixOrgAngles(vrLeft * xOffset, QAngle(0, 0, 0));
				//eyeMatrix.GetTranslation().x, 0, 0)
				*/

				/*
				float flEyeFactor = (eEye == STEREO_EYE_LEFT) ? 1.0 : -1.0;
				Vector eyeOffset = eyeMatrix.GetTranslation();
				VMatrix eyeOffsetMatrix;
				QAngle eyeRotation;
				MatrixAngles(eyeMatrix.As3x4(), eyeRotation);
				eyeOffsetMatrix.SetupMatrixOrgAngles(Vector(eyeOffset.x, eyeOffset.y + (ipd / 2.0 * flEyeFactor), eyeOffset.z), eyeRotation);
				*/

				//QAngle headRotation;
				//MatrixAngles(headMatrix.As3x4(), headRotation);

				//headMatrix = headMatrix * eyeOffsetMatrix;





				/*
				headMatrix = headMatrix * eyeMatrix;

				// Get the player matrix
				VMatrix composedMatrix;
				composedMatrix.SetupMatrixOrgAngles(m_View.origin, m_View.angles);

				// Apply worldFromEye for the current eye to the player's matrix.
				composedMatrix = composedMatrix * headMatrix;

				// Finally convert back to origin+angles.
				MatrixAngles(composedMatrix.As3x4(), m_View.angles, m_View.origin);
				*/

				///*
				//VMatrix pseudoPlayerPosition;
				//pseudoPlayerPosition.SetupMatrixOrgAngles(m_View.origin, m_View.angles);

				//VMatrix matOffset = g_pAnarchyManager->GetMidEyeFromEye(ISourceVirtualReality::VREye_Left);
				//VMatrix worldFromEye = pseudoPlayerPosition * headMatrix * matOffset;

				// Get the player matrix
				VMatrix composedMatrix;
				composedMatrix.SetupMatrixOrgAngles(originalOrigin, originalAngles);// m_View.origin, m_View.angles);

				// Apply the head matrix
				//Vector eyeTranslation = eyeMatrix.GetTranslation();
				//eyeTranslation.z = 0;
				//eyeTranslation.y = 0;
				//eyeMatrix.SetTranslation(eyeTranslation);

				// use original position
				float flEyeFactor = (eEye == STEREO_EYE_LEFT) ? 1.0 : -1.0;
				Vector pseudoOrigin = eyeMatrix.GetTranslation();
				pseudoOrigin.y += (ipd / 2.0 * flEyeFactor);

				/*
				// convert
				int iVal = (eEye == STEREO_EYE_LEFT) ? -1 : 1;
				eyeMatrix = g_pAnarchyManager->SMMatrixToVMatrix(eyeMatrix.Base(), iVal, true);

				// get rotation of the converted matrix
				QAngle pseduoAngles;
				MatrixAngles(eyeMatrix.As3x4(), pseduoAngles);
				*/
				QAngle pseudoAngles;

				// build the peseudo matrix
				VMatrix pseudoEyeMatrix;
				pseudoEyeMatrix.SetupMatrixOrgAngles(pseudoOrigin, pseudoAngles);

				// apply
				composedMatrix = composedMatrix * headMatrix * pseudoEyeMatrix;

				// Finally convert back to origin+angles.
				MatrixAngles(composedMatrix.As3x4(), m_View.angles, m_View.origin);

				// Extract the origin & angles
				//QAngle angles;
				//Vector origin;
				//MatrixAngles(composedMatrix.As3x4(), angles, origin);
				//eyeMatrix.SetupMatrixOrgAngles(origin, angles);
				//m_View.origin = origin;
				//m_View.angles = angles;
				m_View.zNear = g_pAnarchyManager->GetZNear();// VIEW_NEARZ * 0.3;
				//m_View.m_ViewToProjection = composedMatrix;
				m_View.m_bViewToProjectionOverride = true;
				//*/


				//eyeMatrix.SetupMatrixOrgAngles(m_View.origin, m_View.angles);
				//m_View.m_ViewToProjection = eyeMatrix;// eyeMatrix;
				//m_View.m_ViewToProjection = composedMatrix;// eyeMatrix;
				//POINT buf = hmdGetBufferSize();
				//m_View.m_flAspectRatio = buf.x / buf.y;
				//m_View.m_bViewToProjectionOverride = true;
			}
			else
			{
				if (g_pAnarchyManager->UseSBSRendering() && g_pAnarchyManager->GetNoDrawShortcutsValue() == 2)
				{
					//vr.x = view.width * 0.5;
					//m_View.x = view.width * 0.5;
				}
				else
				{
					if (eEye == STEREO_EYE_LEFT)
						m_View.origin -= left * (ipd / 2.0);
					else if (eEye == STEREO_EYE_RIGHT)
						m_View.origin += left * (ipd / 2.0);
				}
			}
		}
		else if (pPlayer && eEye == STEREO_EYE_MONO && g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->VRSpectatorMode() == 1)
		{
			C_BaseEntity* pCamera = g_pAnarchyManager->GetBestSpectatorCameraObject();//C_PropShortcutEntity
			if (pCamera)
			{
				//Vector cameraOrigin = pCamera->GetAbsOrigin();
				//QAngle cameraAngles = pCamera->GetAbsAngles();
				m_View.origin = pCamera->GetAbsOrigin();
				m_View.angles = pCamera->GetAbsAngles();
			}
			else
			{
				VMatrix headMatrix = g_pAnarchyManager->GetVRHeadMatrix();
				VMatrix composedMatrix;
				composedMatrix.SetupMatrixOrgAngles(originalOrigin, originalAngles);

				// apply
				composedMatrix = composedMatrix * headMatrix;

				// Finally convert back to origin+angles.
				MatrixAngles(composedMatrix.As3x4(), m_View.angles, m_View.origin);
			}

			// adjust the view origin and rotation here
			//m_View.origin = pPlayer->GetAbsOrigin();
			//m_View.origin.z += 64.0;
			//m_View.angles = pPlayer->GetAbsAngles();
			////m_View.m_ViewToProjection = originalProjectionMatrix;
			m_View.m_bViewToProjectionOverride = false;
		}


		/*
		// Shift the head matrix on the X axis the distance specified in the eye matrix
		Vector vrForward;
		Vector vrLeft;
		Vector vrUp;
		headMatrix.GetBasisVectors(vrForward, vrLeft, vrUp);

		float xOffset = eyeMatrix.GetTranslation().x;

		VMatrix eyeOffsetMatrix;
		eyeOffsetMatrix.SetupMatrixOrgAngles(vrLeft * xOffset, QAngle(0, 0, 0));
		headMatrix = headMatrix * eyeOffsetMatrix;

		// Get the rotation from the eye matrix
		eyeOffsetMatrix.Identity();
		QAngle eyeRotation;
		MatrixAngles(eyeMatrix.As3x4(), eyeRotation);
		eyeOffsetMatrix.SetupMatrixOrgAngles(Vector(0, 0, 0), eyeRotation);

		// Apply the eye rotation
		headMatrix = headMatrix * eyeOffsetMatrix;
		*/

		///* Added for Anarchy Arcade
		if (g_pAnarchyManager->IsVRActive())
			g_pAnarchyManager->SetLastFOV(view.fov);
		else if (g_pAnarchyManager->UseSBSRendering())
		{
			view.fov = g_pAnarchyManager->GetLastFOV();

			if (g_pAnarchyManager->GetNoDrawShortcutsValue() == 2)
			{
				static ConVarRef sv_restrict_aspect_ratio_fov("sv_restrict_aspect_ratio_fov");
				float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
				float limitedAspectRatio = aspectRatio;
				if ((sv_restrict_aspect_ratio_fov.GetInt() > 0 && engine->IsWindowedMode() && gpGlobals->maxClients > 1) ||
					sv_restrict_aspect_ratio_fov.GetInt() == 2)
				{
					limitedAspectRatio = MIN(aspectRatio, 1.85f * 0.75f); // cap out the FOV advantage at a 1.85:1 ratio (about the widest any legit user should be)
				}

				view.fov = ScaleFOVByWidthRatio(view.fov, limitedAspectRatio);
				view.fovViewmodel = ScaleFOVByWidthRatio(view.fovViewmodel, aspectRatio);
			}
		}
		else
		{
			static ConVarRef sv_restrict_aspect_ratio_fov("sv_restrict_aspect_ratio_fov");
			float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
			float limitedAspectRatio = aspectRatio;
			if ((sv_restrict_aspect_ratio_fov.GetInt() > 0 && engine->IsWindowedMode() && gpGlobals->maxClients > 1) ||
				sv_restrict_aspect_ratio_fov.GetInt() == 2)
			{
				limitedAspectRatio = MIN(aspectRatio, 1.85f * 0.75f); // cap out the FOV advantage at a 1.85:1 ratio (about the widest any legit user should be)
			}

			view.fov = ScaleFOVByWidthRatio(view.fov, limitedAspectRatio);
			view.fovViewmodel = ScaleFOVByWidthRatio(view.fovViewmodel, aspectRatio);
		}
		//*/


		// Let the client mode hook stuff.
		g_pClientMode->PreRender(&view);	// Added for Anarchy Arcade TODO: This should only be called once per LEFT/RIGHT eye render cycle in SBS mode!

		g_pClientMode->AdjustEngineViewport(vr.x, vr.y, vr.width, vr.height);

		ToolFramework_AdjustEngineViewport(vr.x, vr.y, vr.width, vr.height);

		float flViewportScale = mat_viewportscale.GetFloat();

		view.m_nUnscaledX = vr.x;
		view.m_nUnscaledY = vr.y;
		view.m_nUnscaledWidth = vr.width;
		view.m_nUnscaledHeight = vr.height;

		switch (eEye)
		{
		case STEREO_EYE_MONO:
		{

			// Good test mode for debugging viewports that are not full-size.
			//view.width			= vr.width * flViewportScale * 0.75f;
			//view.height			= vr.height * flViewportScale * 0.75f;
			//view.x				= vr.x + view.width * 0.10f;
			//view.y				= vr.y + view.height * 0.20f;

			view.x = vr.x * flViewportScale;
			view.y = vr.y * flViewportScale;
			view.width = vr.width * flViewportScale;
			view.height = vr.height * flViewportScale;

			float engineAspectRatio = (g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->VRSpectatorMode() == 1) ? 0.0f : engine->GetScreenAspectRatio();	// Added for Anarchy Arcade
			view.m_flAspectRatio = (engineAspectRatio > 0.0f) ? engineAspectRatio : ((float)view.width / (float)view.height);
		}
		break;

		case STEREO_EYE_RIGHT:
		case STEREO_EYE_LEFT:
		{
			// Added for Anarchy Arcade
			//g_pHLVR->GetViewportBounds((ISourceVirtualReality::VREye) (eEye - 1), &view.x, &view.y, &view.width, &view.height);
			g_pAnarchyManager->GetViewportBounds((ISourceVirtualReality::VREye) (eEye - 1), &view.x, &view.y, &view.width, &view.height);
			// End added for Anarchy Arcade

			if (g_pAnarchyManager->GetNoDrawShortcutsValue() == 2)
			{
				// doing a comparison render

				//if (eEye == STEREO_EYE_LEFT)
				//{
				//	DevMsg("Left Eye:\n");
					////view.x = view.width * 0.5;
					//view.width = view.width * 0.5;
					////view.m_flAspectRatio *= 2.0;
				//}
				//else
				//	DevMsg("Right Eye:\n");
				//DevMsg("\t%f and %f %f\n", view.zNear, view.zFar, view.fov);

				//view.x = vr.x * flViewportScale;
				//view.y = vr.y * flViewportScale;
				//view.width = vr.width * flViewportScale;
				//view.height = vr.height * flViewportScale;

				//if (eEye == STEREO_EYE_LEFT)
				//	view.x = -view.width * 0.5;

				//vr.x = -view.x;
				//view.m_bRenderToSubrectOfLargerScreen = true;

				/*view.m_nUnscaledWidth = view.width;
				view.m_nUnscaledHeight = view.height;
				view.m_nUnscaledX = view.x;
				view.m_nUnscaledY = view.y;*/

				view.x = vr.x * flViewportScale;
				view.y = vr.y * flViewportScale;
				view.width = vr.width * flViewportScale;
				view.height = vr.height * flViewportScale;

				float engineAspectRatio = engine->GetScreenAspectRatio();
				view.m_flAspectRatio = (engineAspectRatio > 0.0f) ? engineAspectRatio : ((float)view.width / (float)view.height);
			}
			else
			{
				view.m_flAspectRatio = view.width / view.height;// 1.777778 * 0.5;
				view.m_nUnscaledWidth = view.width;
				view.m_nUnscaledHeight = view.height;
				view.m_nUnscaledX = view.x;
				view.m_nUnscaledY = view.y;
			}
		}
		break;

		default:
			Assert(false);
			break;
		}

		// HLVR force actual aspect ratio
		//view.m_flAspectRatio = (float)view.width / (float)view.height;
		if (g_pAnarchyManager->IsVRActive() && view.m_flAspectRatio <= 0.f)
			view.m_flAspectRatio = (float)view.width / (float)view.height;

		int nClearFlags = VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL;

		// Determine if we should draw view model ( client mode override )
		bool drawViewModel = g_pClientMode->ShouldDrawViewModel();

		if (cl_leveloverview.GetFloat() > 0)
		{
			SetUpOverView();
			nClearFlags |= VIEW_CLEAR_COLOR;
			drawViewModel = false;
		}

		// Apply any player specific overrides
		if (pPlayer)
		{
			// Override view model if necessary
			if (!pPlayer->m_Local.m_bDrawViewmodel)
			{
				drawViewModel = false;
			}
		}

		int flags = 0;
		if ((eEye == STEREO_EYE_MONO || UseVR()) && !engine->IsTakingScreenshot())	{
			flags = RENDERVIEW_DRAWHUD;
		}
		if (drawViewModel && !engine->IsTakingScreenshot())
		{
			flags |= RENDERVIEW_DRAWVIEWMODEL;
		}

		if (eEye == STEREO_EYE_RIGHT)
		{
			// we should use the monitor view from the left eye for both eyes
			flags |= RENDERVIEW_SUPPRESSMONITORRENDERING;
		}

		RenderView(view, nClearFlags, flags);

		if (UseVR())
		{
			bool bDoUndistort = !engine->IsTakingScreenshot();

			if (bDoUndistort)
			{
				g_ClientVirtualReality.PostProcessFrame(eEye);
			}

			// logic here all cloned from code in viewrender.cpp around RenderHUDQuad:
			/*
			// figure out if we really want to draw the HUD based on freeze cam
			bool bInFreezeCam = (pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM);

			// draw the HUD after the view model so its "I'm closer" depth queues work right.
			if (!bInFreezeCam && g_ClientVirtualReality.ShouldRenderHUDInWorld())
			{
				// TODO - a bit of a shonky test - basically trying to catch the main menu, the briefing screen, the loadout screen, etc.
				bool bTranslucent = !g_pMatSystemSurface->IsCursorVisible();
				g_ClientVirtualReality.OverlayHUDQuadWithUndistort(view, bDoUndistort, g_pClientMode->ShouldBlackoutAroundHUD(), bTranslucent);
			}
			*/
		}

		// TODO: should these be inside or outside the stereo eye stuff?
		g_pClientMode->PostRender();	// Added for Anarchy Arcade TODO: This should only be called once per LEFT/RIGHT eye render cycle in SBS mode!

		//if (g_pAnarchyManager->IsVRActive() && eEye == STEREO_EYE_LEFT)
		//	g_pAnarchyManager->RenderVREyeToView(m_View, ISourceVirtualReality::VREye::VREye_Left);


		if (!g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->UseSBSRendering() && g_pAnarchyManager->GetNoDrawShortcutsValue() == 2)
		{
			// if this is the 1st eye, transpose it onto the other half of the screen.
			if (eEye == STEREO_EYE_RIGHT)
			{

			}

			// otherwise, if this is the 2nd eye, transpose both sides of the screen
		}
	}

	///*
	//m_View.m_ViewToProjection = originalProjectionMatrix;
	//m_View.m_eStereoEye = originalStereoEye;
	//m_View.origin = originalOrigin;
	//m_View.angles = originalAngles;
	//m_View.m_bViewToProjectionOverride = false;
	//*/


	if (false && !g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->UseSBSRendering() && g_pAnarchyManager->GetNoDrawShortcutsValue() == 2)
	{
		/*
		// transpose the sides of the screen.
		CMatRenderContextPtr pRenderContext(materials);
		ITexture* pRenderTexture = pRenderContext->GetRenderTarget();


		Rect_t	DestinationRect, SourceRect;

		SourceRect.width = m_View.width * 0.5;
		SourceRect.height = m_View.height;
		SourceRect.x = m_View.width * 0.5;
		SourceRect.y = 0;


		DestinationRect.width = m_View.width * 0.8;
		DestinationRect.height = m_View.height;
		DestinationRect.x = 0;
		DestinationRect.y = 0;

		//pRenderContext->CopyRenderTargetToTextureEx(pRenderTexture, 0, &SourceRect, &DestinationRect);
		pRenderContext->CopyTextureToRenderTargetEx(0, pRenderTexture, &SourceRect, &DestinationRect);
		*/

		CMatRenderContextPtr pRenderContext(materials);

		ITexture	*pFullFrameFB1 = materials->FindTexture("_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET);
		IMaterial	*pCopyMaterial = materials->FindMaterial("dev/upscale", TEXTURE_GROUP_OTHER);
		pCopyMaterial->IncrementReferenceCount();

		Rect_t	DownscaleRect, UpscaleRect;

		m_View.m_nUnscaledWidth = m_View.width * 0.5;

		DownscaleRect.x = m_View.x;
		DownscaleRect.y = m_View.y;
		DownscaleRect.width = m_View.width;
		DownscaleRect.height = m_View.height;

		UpscaleRect.x = m_View.m_nUnscaledX;
		UpscaleRect.y = m_View.m_nUnscaledY;
		UpscaleRect.width = m_View.m_nUnscaledWidth;
		UpscaleRect.height = m_View.m_nUnscaledHeight;

		pRenderContext->CopyRenderTargetToTextureEx(pFullFrameFB1, 0, &DownscaleRect, &DownscaleRect);
		pRenderContext->DrawScreenSpaceRectangle(pCopyMaterial, UpscaleRect.x, UpscaleRect.y, UpscaleRect.width, UpscaleRect.height,
			DownscaleRect.x, DownscaleRect.y, DownscaleRect.x + DownscaleRect.width - 1, DownscaleRect.y + DownscaleRect.height - 1,
			pFullFrameFB1->GetActualWidth(), pFullFrameFB1->GetActualHeight());

		pCopyMaterial->DecrementReferenceCount();

		/*
		CMatRenderContextPtr pRenderContext(materials);

		ITexture	*pFullFrameFB1 = materials->FindTexture("_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET);
		IMaterial	*pCopyMaterial = materials->FindMaterial("dev/upscale", TEXTURE_GROUP_OTHER);
		pCopyMaterial->IncrementReferenceCount();

		Rect_t	DownscaleRect, UpscaleRect;

		DownscaleRect.x = m_View.x;
		DownscaleRect.y = m_View.y;
		DownscaleRect.width = m_View.width;
		DownscaleRect.height = m_View.height;

		UpscaleRect.x = m_View.m_nUnscaledX;
		UpscaleRect.y = m_View.m_nUnscaledY;
		UpscaleRect.width = m_View.m_nUnscaledWidth;
		UpscaleRect.height = m_View.m_nUnscaledHeight;

		pRenderContext->CopyRenderTargetToTextureEx(pFullFrameFB1, 0, &DownscaleRect, &DownscaleRect);
		pRenderContext->DrawScreenSpaceRectangle(pCopyMaterial, UpscaleRect.x, UpscaleRect.y, UpscaleRect.width, UpscaleRect.height,
			DownscaleRect.x, DownscaleRect.y, DownscaleRect.x + DownscaleRect.width - 1, DownscaleRect.y + DownscaleRect.height - 1,
			pFullFrameFB1->GetActualWidth(), pFullFrameFB1->GetActualHeight());

		pCopyMaterial->DecrementReferenceCount();
		*/
	}

	//g_pAnarchyManager->VRFrameReady();
	engine->EngineStats_EndFrame();

	// Draw all of the UI stuff "fullscreen"
	// (this is not health, ammo, etc. Nor is it pre-game briefing interface stuff - this is the stuff that appears when you hit Esc in-game)
	// In stereo mode this is rendered inside of RenderView so it goes into the render target
	if (!g_ClientVirtualReality.ShouldRenderHUDInWorld() && !engine->IsTakingScreenshot())
	{
		CViewSetup view2d;
		view2d.x = rect->x;
		view2d.y = rect->y;
		view2d.width = rect->width;
		view2d.height = rect->height;

		render->Push2DView(view2d, 0, NULL, GetFrustum());
		render->VGui_Paint(PAINT_UIPANELS | PAINT_CURSOR);
		render->PopView(GetFrustum());
	}

	/*
	if (g_pAnarchyManager->IsVRActive())
	{
		g_pAnarchyManager->VRFrameReady();
		g_pAnarchyManager->VRFrameEnd();
	}
	*/
	// End Added for Anarchy Arcade


	/* Added for Anarchy Arcade (ORIGINAL CODE)

    for( StereoEye_t eEye = GetFirstEye(); eEye <= GetLastEye(); eEye = (StereoEye_t)(eEye+1) )
	{
		CViewSetup &view = GetView( eEye );

		#if 0 && defined( CSTRIKE_DLL )
			const bool bPlayingBackReplay = g_pEngineClientReplay && g_pEngineClientReplay->IsPlayingReplayDemo();
			if ( pPlayer && !bPlayingBackReplay )
			{
				C_BasePlayer *pViewTarget = pPlayer;

				if ( pPlayer->IsObserver() && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
				{
					pViewTarget = dynamic_cast<C_BasePlayer*>( pPlayer->GetObserverTarget() );
				}

				if ( pViewTarget )
				{
					float targetFOV = (float)pViewTarget->m_iFOV;

					if ( targetFOV == 0 )
					{
						// FOV of 0 means use the default FOV
						targetFOV = g_pGameRules->DefaultFOV();
					}

					float deltaFOV = view.fov - m_flLastFOV;
					float FOVDirection = targetFOV - pViewTarget->m_iFOVStart;

					// Clamp FOV changes to stop FOV oscillation
					if ( ( deltaFOV < 0.0f && FOVDirection > 0.0f ) ||
						( deltaFOV > 0.0f && FOVDirection < 0.0f ) )
					{
						view.fov = m_flLastFOV;
					}

					// Catch case where FOV overshoots its target FOV
					if ( ( view.fov < targetFOV && FOVDirection <= 0.0f ) ||
						( view.fov > targetFOV && FOVDirection >= 0.0f ) )
					{
						view.fov = targetFOV;
					}

					m_flLastFOV = view.fov;
				}
			}
		#endif

	    static ConVarRef sv_restrict_aspect_ratio_fov( "sv_restrict_aspect_ratio_fov" );
	    float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
	    float limitedAspectRatio = aspectRatio;
	    if ( ( sv_restrict_aspect_ratio_fov.GetInt() > 0 && engine->IsWindowedMode() && gpGlobals->maxClients > 1 ) ||
		    sv_restrict_aspect_ratio_fov.GetInt() == 2 )
	    {
		    limitedAspectRatio = MIN( aspectRatio, 1.85f * 0.75f ); // cap out the FOV advantage at a 1.85:1 ratio (about the widest any legit user should be)
	    }

	    view.fov = ScaleFOVByWidthRatio( view.fov, limitedAspectRatio );
	    view.fovViewmodel = ScaleFOVByWidthRatio( view.fovViewmodel, aspectRatio );

	    // Let the client mode hook stuff.
	    g_pClientMode->PreRender(&view);

	    g_pClientMode->AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

	    ToolFramework_AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

	    float flViewportScale = mat_viewportscale.GetFloat();

		view.m_nUnscaledX = vr.x;
		view.m_nUnscaledY = vr.y;
		view.m_nUnscaledWidth = vr.width;
		view.m_nUnscaledHeight = vr.height;

        switch( eEye )
		{
			case STEREO_EYE_MONO:
			{
#if 0
                // Good test mode for debugging viewports that are not full-size.
	            view.width			= vr.width * flViewportScale * 0.75f;
	            view.height			= vr.height * flViewportScale * 0.75f;
	            view.x				= vr.x + view.width * 0.10f;
	            view.y				= vr.y + view.height * 0.20f;
#else
	            view.x				= vr.x * flViewportScale;
				view.y				= vr.y * flViewportScale;
				view.width			= vr.width * flViewportScale;
				view.height			= vr.height * flViewportScale;
#endif
			    float engineAspectRatio = engine->GetScreenAspectRatio();
			    view.m_flAspectRatio	= ( engineAspectRatio > 0.0f ) ? engineAspectRatio : ( (float)view.width / (float)view.height );
			}
			break;

			case STEREO_EYE_RIGHT:
			case STEREO_EYE_LEFT:
			{
				g_pSourceVR->GetViewportBounds( (ISourceVirtualReality::VREye)(eEye - 1 ), &view.x, &view.y, &view.width, &view.height );
				view.m_nUnscaledWidth = view.width;
				view.m_nUnscaledHeight = view.height;
				view.m_nUnscaledX = view.x;
				view.m_nUnscaledY = view.y;
			}
			break;

            default:
                Assert ( false );
                break;
		}

		// if we still don't have an aspect ratio, compute it from the view size
		if( view.m_flAspectRatio <= 0.f )
		    view.m_flAspectRatio	= (float)view.width / (float)view.height;

	    int nClearFlags = VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL;

	    if( gl_clear_randomcolor.GetBool() )
	    {
		    CMatRenderContextPtr pRenderContext( materials );
		    pRenderContext->ClearColor3ub( rand()%256, rand()%256, rand()%256 );
		    pRenderContext->ClearBuffers( true, false, false );
		    pRenderContext->Release();
	    }
	    else if ( gl_clear.GetBool() )
	    {
		    nClearFlags |= VIEW_CLEAR_COLOR;
	    }
	    else if ( IsPosix() )
	    {
		    MaterialAdapterInfo_t adapterInfo;
		    materials->GetDisplayAdapterInfo( materials->GetCurrentAdapter(), adapterInfo );

		    // On Posix, on ATI, we always clear color if we're antialiasing
		    if ( adapterInfo.m_VendorID == 0x1002 )
		    {
			    if ( g_pMaterialSystem->GetCurrentConfigForVideoCard().m_nAASamples > 0 )
			    {
				    nClearFlags |= VIEW_CLEAR_COLOR;
			    }
		    }
	    }

	    // Determine if we should draw view model ( client mode override )
	    bool drawViewModel = g_pClientMode->ShouldDrawViewModel();

	    if ( cl_leveloverview.GetFloat() > 0 )
	    {
		    SetUpOverView();		
		    nClearFlags |= VIEW_CLEAR_COLOR;
		    drawViewModel = false;
	    }

	    // Apply any player specific overrides
	    if ( pPlayer )
	    {
		    // Override view model if necessary
		    if ( !pPlayer->m_Local.m_bDrawViewmodel )
		    {
			    drawViewModel = false;
		    }
	    }

	    int flags = 0;
		if( eEye == STEREO_EYE_MONO || eEye == STEREO_EYE_LEFT || ( g_ClientVirtualReality.ShouldRenderHUDInWorld() ) )
		{
			flags = RENDERVIEW_DRAWHUD;
		}
	    if ( drawViewModel )
	    {
		    flags |= RENDERVIEW_DRAWVIEWMODEL;
	    }
		if( eEye == STEREO_EYE_RIGHT )
		{
			// we should use the monitor view from the left eye for both eyes
			flags |= RENDERVIEW_SUPPRESSMONITORRENDERING;
		}

	    RenderView( view, nClearFlags, flags );

		if ( UseVR() )
		{
			bool bDoUndistort = ! engine->IsTakingScreenshot();

			if ( bDoUndistort )
			{
				g_ClientVirtualReality.PostProcessFrame( eEye );
			}

			// logic here all cloned from code in viewrender.cpp around RenderHUDQuad:

			// figure out if we really want to draw the HUD based on freeze cam
			bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

			// draw the HUD after the view model so its "I'm closer" depth queues work right.
			if( !bInFreezeCam && g_ClientVirtualReality.ShouldRenderHUDInWorld() )
			{
				// TODO - a bit of a shonky test - basically trying to catch the main menu, the briefing screen, the loadout screen, etc.
				bool bTranslucent = !g_pMatSystemSurface->IsCursorVisible();
				g_ClientVirtualReality.OverlayHUDQuadWithUndistort( view, bDoUndistort, g_pClientMode->ShouldBlackoutAroundHUD(), bTranslucent );
			}
		}
    }


	// TODO: should these be inside or outside the stereo eye stuff?
	g_pClientMode->PostRender();
	engine->EngineStats_EndFrame();

#if !defined( _X360 )
	// Stop stubbing the material system so we can see the budget panel
	matStub.End();
#endif


	// Draw all of the UI stuff "fullscreen"
    // (this is not health, ammo, etc. Nor is it pre-game briefing interface stuff - this is the stuff that appears when you hit Esc in-game)
	// In stereo mode this is rendered inside of RenderView so it goes into the render target
	if( !g_ClientVirtualReality.ShouldRenderHUDInWorld() )
	{
		CViewSetup view2d;
		view2d.x				= rect->x;
		view2d.y				= rect->y;
		view2d.width			= rect->width;
		view2d.height			= rect->height;

		render->Push2DView( view2d, 0, NULL, GetFrustum() );
		render->VGui_Paint( PAINT_UIPANELS | PAINT_CURSOR );
		render->PopView( GetFrustum() );
	}
	*/

}




static void GetPos( const CCommand &args, Vector &vecOrigin, QAngle &angles )
{
	vecOrigin = MainViewOrigin();
	angles = MainViewAngles();
	if ( args.ArgC() == 2 && atoi( args[1] ) == 2 )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			vecOrigin = pPlayer->GetAbsOrigin();
			angles = pPlayer->GetAbsAngles();
		}
	}
}

CON_COMMAND( spec_pos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );
	Warning( "spec_goto %.1f %.1f %.1f %.1f %.1f\n", vecOrigin.x, vecOrigin.y, 
		vecOrigin.z, angles.x, angles.y );
}

CON_COMMAND( getpos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );

	const char *pCommand1 = "setpos";
	const char *pCommand2 = "setang";
	if ( args.ArgC() == 2 && atoi( args[1] ) == 2 )
	{
		pCommand1 = "setpos_exact";
		pCommand2 = "setang_exact";
	}

	Warning( "%s %f %f %f;", pCommand1, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	Warning( "%s %f %f %f\n", pCommand2, angles.x, angles.y, angles.z );
}

