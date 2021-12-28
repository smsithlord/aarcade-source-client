//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "client_virtualreality.h"

#include "materialsystem/itexture.h"
#include "materialsystem/materialsystem_config.h"
#include "view_shared.h"
#include "view_scene.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/Controls.h"
#include "sourcevr/isourcevirtualreality.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "tier0/vprof_telemetry.h"
#include <time.h>
#include "steam/steam_api.h"


const char *COM_GetModDirectory(); // return the mod dir (rather than the complete -game param, which can be a path)

CClientVirtualReality g_ClientVirtualReality;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientVirtualReality, IClientVirtualReality,
	CLIENTVIRTUALREALITY_INTERFACE_VERSION, g_ClientVirtualReality );


// --------------------------------------------------------------------
// A huge pile of VR convars
// --------------------------------------------------------------------
ConVar vr_activate_default( "vr_activate_default",		"0", FCVAR_ARCHIVE, "If this is true the game will switch to VR mode once startup is complete." );

// This are somewhat obsolete.
ConVar vr_aim_yaw_offset( "vr_aim_yaw_offset", "90", 0, "This value is added to Yaw when returning the vehicle aim angles to Source." );

ConVar vr_stereo_swap_eyes ( "vr_stereo_swap_eyes", "0", 0, "1=swap eyes." );

// Useful for debugging wacky-projection problems, separate from multi-rendering problems.
ConVar vr_stereo_mono_set_eye ( "vr_stereo_mono_set_eye", "0", 0, "0=off, Set all eyes to 1=left, 2=right, 3=middle eye" );

// Useful for examining anims, etc.
ConVar vr_debug_remote_cam( "vr_debug_remote_cam", "0" );
ConVar vr_debug_remote_cam_pos_x( "vr_debug_remote_cam_pos_x", "150.0" );
ConVar vr_debug_remote_cam_pos_y( "vr_debug_remote_cam_pos_y", "0.0" );
ConVar vr_debug_remote_cam_pos_z( "vr_debug_remote_cam_pos_z", "0.0" );
ConVar vr_debug_remote_cam_target_x( "vr_debug_remote_cam_target_x", "0.0" );
ConVar vr_debug_remote_cam_target_y( "vr_debug_remote_cam_target_y", "0.0" );
ConVar vr_debug_remote_cam_target_z( "vr_debug_remote_cam_target_z", "-50.0" );

// HUD config values
ConVar vr_hud_max_fov( "vr_hud_max_fov", "1000", FCVAR_ARCHIVE, "Max FOV of the HUD" );
ConVar vr_hud_forward( "vr_hud_forward", "15", FCVAR_ARCHIVE, "Apparent distance of the HUD in inches" );
ConVar vr_hud_display_ratio( "vr_hud_display_ratio", "0.7", FCVAR_ARCHIVE );
ConVar vr_hud_never_overlay( "vr_hud_never_overlay", "1" );

ConVar vr_hud_axis_lock_to_world( "vr_hud_axis_lock_to_world", "0", FCVAR_ARCHIVE, "Bitfield - locks HUD axes to the world - 0=pitch, 1=yaw, 2=roll" );

// Default distance clips through rocketlauncher, heavy's body, etc.
ConVar vr_projection_znear_multiplier( "vr_projection_znear_multiplier", "0.3", 0, "Allows moving the ZNear plane to deal with body clipping" );

// Should the viewmodel (weapon) translate with the HMD, or remain fixed to the in-world body (but still rotate with the head)? Purely a graphics effect - no effect on actual bullet aiming.
// Has no effect in aim modes where aiming is not controlled by the head.
ConVar vr_viewmodel_translate_with_head ( "vr_viewmodel_translate_with_head", "0", 0, "1=translate the viewmodel with the head motion." );

ConVar vr_zoom_multiplier ( "vr_zoom_multiplier", "2.0", FCVAR_ARCHIVE, "When zoomed, how big is the scope on your HUD?" );
ConVar vr_zoom_scope_scale ( "vr_zoom_scope_scale", "6.0", 0, "Something to do with the default scope HUD overlay size." );		// Horrible hack - should work out the math properly, but we need to ship.


ConVar vr_viewmodel_offset_forward( "vr_viewmodel_offset_forward", "-8", 0 );
ConVar vr_viewmodel_offset_forward_large( "vr_viewmodel_offset_forward_large", "-15", 0 );

ConVar vr_vehicle_aim_mode("vr_vehicle_aim_mode", "0", 0);

ConVar vr_force_windowed ( "vr_force_windowed", "1", FCVAR_ARCHIVE );

ConVar vr_first_person_uses_world_model ( "vr_first_person_uses_world_model", "0", FCVAR_ARCHIVE, "Causes the third person model to be drawn instead of the view model" );

static bool IsMenuUp()
{
	return ((enginevgui && enginevgui->IsGameUIVisible()) || vgui::surface()->IsCursorVisible());
}


ConVar hlvr_menu_forward("hlvr_menu_forward", "20", FCVAR_ARCHIVE);
ConVar hlvr_menu_scale("hlvr_menu_scale", "2", FCVAR_ARCHIVE);

ConVar hlvr_control_hand("hlvr_control_hand", "1", FCVAR_ARCHIVE, "Move relative to which hand? 0 = Head, 1 = Off Hand, 2 = Weapon Hand");


QAngle savedHudAngles;
Vector savedPlayerViewOrigin;
bool runyet;

// --------------------------------------------------------------------
// Purpose:  Switch to/from VR mode.
// --------------------------------------------------------------------
CON_COMMAND( vr_activate, "Switch to VR mode" )
{
	g_ClientVirtualReality.Activate();
}
CON_COMMAND( vr_deactivate, "Switch from VR mode to normal mode" )
{
	g_ClientVirtualReality.Deactivate();
}
CON_COMMAND( vr_toggle, "Toggles VR mode" )
{
	if (g_pHLVR->DllLoaded())
	{
		if (g_pHLVR->ShouldRunInVR())
			g_ClientVirtualReality.Deactivate();
		else
			g_ClientVirtualReality.Activate();
	}
	else
	{
		Msg( "VR Mode is not enabled.\n" );
	}
}

// --------------------------------------------------------------------
// Purpose: Returns true if the matrix is orthonormal
// --------------------------------------------------------------------
bool IsOrthonormal ( VMatrix Mat, float fTolerance )
{
	float LenFwd = Mat.GetForward().Length();
	float LenUp = Mat.GetUp().Length();
	float LenLeft = Mat.GetLeft().Length();
	float DotFwdUp = Mat.GetForward().Dot ( Mat.GetUp() );
	float DotUpLeft = Mat.GetUp().Dot ( Mat.GetLeft() );
	float DotLeftFwd = Mat.GetLeft().Dot ( Mat.GetForward() );
	if ( fabsf ( LenFwd - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenUp - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenLeft - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotFwdUp ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotUpLeft ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotLeftFwd ) > fTolerance )
	{
		return false;
	}
	return true;
}


// --------------------------------------------------------------------
// Purpose: Computes the FOV from the projection matrix
// --------------------------------------------------------------------
void CalcFovFromProjection ( float *pFov, const VMatrix &proj )
{
	// The projection matrices should be of the form:
	// p0  0   z1 p1
	// 0   p2  z2 p3
	// 0   0   z3 1
	// (p0 = X fov, p1 = X offset, p2 = Y fov, p3 = Y offset )
	// TODO: cope with more complex projection matrices?
	float xscale  = proj.m[0][0];
	Assert ( proj.m[0][1] == 0.0f );
	float xoffset = proj.m[0][2];
	Assert ( proj.m[0][3] == 0.0f );
	Assert ( proj.m[1][0] == 0.0f );
	float yscale  = proj.m[1][1];
	float yoffset = proj.m[1][2];
	Assert ( proj.m[1][3] == 0.0f );
	// Row 2 determines Z-buffer values - don't care about those for now.
	Assert ( proj.m[3][0] == 0.0f );
	Assert ( proj.m[3][1] == 0.0f );
	Assert ( proj.m[3][2] == -1.0f );
	Assert ( proj.m[3][3] == 0.0f );

	// The math here:
	// A view-space vector (x,y,z,1) is transformed by the projection matrix
	// / xscale   0     xoffset  0 \
	// |    0   yscale  yoffset  0 |
	// |    ?     ?        ?     ? |
	// \    0     0       -1     0 /
	//
	// Then the result is normalized (i.e. divide by w) and the result clipped to the [-1,+1] unit cube.
	// (ignore Z for now, and the clipping is slightly different).
	// So, we want to know what vectors produce a clip value of -1 and +1 in each direction, e.g. in the X direction:
	//    +-1 = ( xscale*x + xoffset*z ) / (-1*z)
	//        = xscale*(x/z) + xoffset            (I flipped the signs of both sides)
	// => (+-1 - xoffset)/xscale = x/z
	// ...and x/z is tan(theta), and theta is the half-FOV.

	float fov_px = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - xoffset ) / xscale ) ) );
	float fov_nx = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - xoffset ) / xscale ) ) );
	float fov_py = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - yoffset ) / yscale ) ) );
	float fov_ny = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - yoffset ) / yscale ) ) );

	*pFov = Max ( Max ( fov_px, fov_nx ), Max ( fov_py, fov_ny ) );
	// FIXME: hey you know, I could do the Max() series before I call all those expensive atanf()s...
}


// --------------------------------------------------------------------
// construction/destruction
// --------------------------------------------------------------------
CClientVirtualReality::CClientVirtualReality()
{
	m_PlayerTorsoOrigin.Init();
	m_PlayerTorsoAngle.Init();
	m_WorldFromWeapon.Identity();
	m_WorldFromMidEye.Identity();

	m_bOverrideTorsoAngle = false;
	m_OverrideTorsoOffset.Init();

	// Also reset our model of the player's torso orientation
	m_PlayerTorsoAngle.Init ( 0.0f, 0.0f, 0.0f );

	m_WorldZoomScale = 1.0f;
	m_hmmMovementActual = HMM_SHOOTFACE_MOVEFACE;
	m_iAlignTorsoAndViewToWeaponCountdown = 0;

	m_rtLastMotionSample = 0;
	m_bMotionUpdated = false;

#if defined( USE_SDL )
    m_nNonVRSDLDisplayIndex = 0;
#endif
}

CClientVirtualReality::~CClientVirtualReality()
{
}


// --------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------
bool			CClientVirtualReality::Connect( CreateInterfaceFn factory )
{
	if ( !factory )
		return false;

	if ( !BaseClass::Connect( factory ) )
		return false;

	return true;
}


// --------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------
void			CClientVirtualReality::Disconnect()
{
	BaseClass::Disconnect();
}


// --------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------
void *			CClientVirtualReality::QueryInterface( const char *pInterfaceName )
{
	CreateInterfaceFn factory = Sys_GetFactoryThis();	// This silly construction is necessary
	return factory( pInterfaceName, NULL );				// to prevent the LTCG compiler from crashing.
}


// --------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------
InitReturnVal_t	CClientVirtualReality::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	return INIT_OK;
}


// --------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------
void			CClientVirtualReality::Shutdown()
{
	BaseClass::Shutdown();
}


// --------------------------------------------------------------------
// Purpose: Draws the main menu in Stereo
// --------------------------------------------------------------------
void CClientVirtualReality::DrawMainMenu()
{
	// have to draw the UI in stereo via the render texture or it won't fuse properly
	// Draw it into the render target first
	ITexture *pTexture = materials->FindTexture( "_rt_vgui", NULL, false );
	Assert( pTexture );
	if( !pTexture)
		return;

	CMatRenderContextPtr pRenderContext( materials );
	int viewActualWidth = pTexture->GetActualWidth();
	int viewActualHeight = pTexture->GetActualHeight();

	int viewWidth, viewHeight;
	vgui::surface()->GetScreenSize( viewWidth, viewHeight );

	// clear depth in the backbuffer before we push the render target
	pRenderContext->ClearBuffers( false, true, true );

	// constrain where VGUI can render to the view
	pRenderContext->PushRenderTargetAndViewport( pTexture, NULL, 0, 0, viewActualWidth, viewActualHeight );
	pRenderContext->OverrideAlphaWriteEnable( true, true );

	// clear the render target
	pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
	pRenderContext->ClearBuffers( true, false );

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "VGui_DrawHud", __FUNCTION__ );

	// Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
	vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
	}
	// Same for client .dll tools
	root = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
	}

	// paint the main menu and cursor
	render->VGui_Paint( (PaintMode_t) ( PAINT_UIPANELS | PAINT_CURSOR ) );

	pRenderContext->OverrideAlphaWriteEnable( false, true );
	pRenderContext->PopRenderTargetAndViewport();
	pRenderContext->Flush();

	int leftX, leftY, leftW, leftH, rightX, rightY, rightW, rightH;
	g_pHLVR->GetViewportBounds(ISourceVirtualReality::VREye_Left, &leftX, &leftY, &leftW, &leftH);
	g_pHLVR->GetViewportBounds(ISourceVirtualReality::VREye_Right, &rightX, &rightY, &rightW, &rightH);


	// render the main view
	CViewSetup viewEye[STEREO_EYE_MAX];
	viewEye[ STEREO_EYE_MONO ].zNear = 0.1;
	viewEye[ STEREO_EYE_MONO ].zFar = 10000.f;
	viewEye[ STEREO_EYE_MONO ].angles.Init();
	viewEye[ STEREO_EYE_MONO ].origin.Zero();
	viewEye[ STEREO_EYE_MONO ].x = viewEye[ STEREO_EYE_MONO ].m_nUnscaledX =  leftX;
	viewEye[ STEREO_EYE_MONO ].y = viewEye[ STEREO_EYE_MONO ].m_nUnscaledY = leftY;
	viewEye[ STEREO_EYE_MONO ].width = viewEye[ STEREO_EYE_MONO ].m_nUnscaledWidth = leftW;
	viewEye[ STEREO_EYE_MONO ].height = viewEye[ STEREO_EYE_MONO ].m_nUnscaledHeight = leftH;

	viewEye[STEREO_EYE_LEFT] = viewEye[STEREO_EYE_RIGHT] = viewEye[ STEREO_EYE_MONO ] ;
	viewEye[STEREO_EYE_LEFT].m_eStereoEye = STEREO_EYE_LEFT;
	viewEye[STEREO_EYE_RIGHT].x = rightX;
	viewEye[STEREO_EYE_RIGHT].y = rightY;
	viewEye[STEREO_EYE_RIGHT].m_eStereoEye = STEREO_EYE_RIGHT;

	// let sourcevr.dll tell us where to put the cameras
	ProcessCurrentTrackingState( 0 );
	Vector vViewModelOrigin;
	QAngle qViewModelAngles;
	OverrideView( &viewEye[ STEREO_EYE_MONO ] , &vViewModelOrigin, &qViewModelAngles, HMM_NOOVERRIDE );
	g_ClientVirtualReality.OverrideStereoView( &viewEye[ STEREO_EYE_MONO ] , &viewEye[STEREO_EYE_LEFT], &viewEye[STEREO_EYE_RIGHT] );

	// render both eyes
	for( int nView = STEREO_EYE_LEFT; nView <= STEREO_EYE_RIGHT; nView++ )
	{
		CMatRenderContextPtr pRenderContext( materials );
		PIXEvent pixEvent( pRenderContext, nView == STEREO_EYE_LEFT ? "left eye" : "right eye" );

		ITexture *pColor = g_pHLVR->GetRenderTarget((ISourceVirtualReality::VREye)(nView - 1), ISourceVirtualReality::RT_Color);
		ITexture *pDepth = g_pHLVR->GetRenderTarget((ISourceVirtualReality::VREye)(nView - 1), ISourceVirtualReality::RT_Depth);
		render->Push3DView( viewEye[nView], VIEW_CLEAR_DEPTH|VIEW_CLEAR_COLOR, pColor, NULL, pDepth );
		RenderHUDQuad( false,  false );
		render->PopView( NULL );

		PostProcessFrame( (StereoEye_t)nView );

		OverlayHUDQuadWithUndistort( viewEye[nView], true, true, false );
	}
}


// --------------------------------------------------------------------
// Purpose:
//		Offset the incoming view appropriately.
//		Set up the "middle eye" from that.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideView ( CViewSetup *pViewMiddle, Vector *pViewModelOrigin, QAngle *pViewModelAngles, HeadtrackMovementMode_t hmmMovementOverride )
{
	if( !UseVR() )
	{
		return false;
	}

	// Incoming data may or may not be useful - it is the origin and aim of the "player", i.e. where bullets come from.
	// In some modes it is an independent thing, guided by the mouse & keyboard = useful.
	// In other modes it's just where the HMD was pointed last frame, modified slightly by kbd+mouse.
	// In those cases, we should use our internal reference (which keeps track thanks to OverridePlayerMotion)
	QAngle originalMiddleAngles = pViewMiddle->angles;
	Vector originalMiddleOrigin = pViewMiddle->origin;

	// Figure out the in-game "torso" concept, which corresponds to the player's physical torso.
	m_PlayerTorsoOrigin = pViewMiddle->origin;

	// Ignore what was passed in - it's just the direction the weapon is pointing, which was determined by last frame's HMD orientation!
	// Instead use our cached value.
	QAngle torsoAngles = m_PlayerTorsoAngle;

	VMatrix worldFromTorso;
	worldFromTorso.SetupMatrixOrgAngles( m_PlayerTorsoOrigin, torsoAngles );

	VMatrix matMideyeZeroFromMideyeCurrent = g_pHLVR->GetMideyePose();
	Vector viewTranslation = matMideyeZeroFromMideyeCurrent.GetTranslation();

	// Now figure out the three principal matrices: m_TorsoFromMideye, m_WorldFromMidEye, m_WorldFromWeapon
	// m_TorsoFromMideye is done so that OverridePlayerMotion knows what to do with WASD.

	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTFACE_MOVETORSO:
		// Aim point is down your nose, i.e. same as the view angles.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = m_WorldFromMidEye;
		break;

	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
		// Aim point is independent of view - leave it as it was, just copy it into m_WorldFromWeapon for our use.
		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromMidEye = worldFromTorso * matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon.SetupMatrixOrgAngles( originalMiddleOrigin, originalMiddleAngles );
		break;

	case HMM_SHOOTMOVELOOKMOUSE:
		// HMD is ignored completely, mouse does everything.
		m_PlayerTorsoAngle = originalMiddleAngles;

		worldFromTorso.SetupMatrixOrgAngles( m_PlayerTorsoOrigin, originalMiddleAngles );

		m_TorsoFromMideye.Identity();
		m_WorldFromMidEye = worldFromTorso;
		m_WorldFromWeapon = worldFromTorso;
		break;

	case HMM_SHOOTMOVELOOKMOUSEFACE:
		// mouse does everything, and then we add head tracking on top of that
		worldFromTorso = worldFromTorso * matMideyeZeroFromMideyeCurrent;

		m_TorsoFromMideye = matMideyeZeroFromMideyeCurrent;
		m_WorldFromWeapon = worldFromTorso;
		m_WorldFromMidEye = worldFromTorso;
		break;

	default: Assert ( false ); break;
	}

	// Finally convert back to origin+angles that the game understands.
	pViewMiddle->origin = m_WorldFromMidEye.GetTranslation();
	VectorAngles ( m_WorldFromMidEye.GetForward(), m_WorldFromMidEye.GetUp(), pViewMiddle->angles );

	*pViewModelAngles = pViewMiddle->angles;
	if ( vr_viewmodel_translate_with_head.GetBool() )
	{
		*pViewModelOrigin = pViewMiddle->origin;
	}
	else
	{
		*pViewModelOrigin = originalMiddleOrigin;
	}

	m_WorldFromMidEyeNoDebugCam = m_WorldFromMidEye;
	if ( vr_debug_remote_cam.GetBool() )
	{
		Vector vOffset ( vr_debug_remote_cam_pos_x.GetFloat(), vr_debug_remote_cam_pos_y.GetFloat(), vr_debug_remote_cam_pos_z.GetFloat() );
		Vector vLookat ( vr_debug_remote_cam_target_x.GetFloat(), vr_debug_remote_cam_target_y.GetFloat(), vr_debug_remote_cam_target_z.GetFloat() );
		pViewMiddle->origin += vOffset;
		Vector vView = vLookat - vOffset;
		VectorAngles ( vView, m_WorldFromMidEye.GetUp(), pViewMiddle->angles );

		m_WorldFromMidEye.SetupMatrixOrgAngles( pViewMiddle->origin, pViewMiddle->angles );

		m_TorsoFromMideye.Identity();
	}

	// set the near clip plane so the local player clips less
	pViewMiddle->zNear *= vr_projection_znear_multiplier.GetFloat();

	return true;
}


// --------------------------------------------------------------------
// Purpose:
//		In some aim/move modes, the HUD aim reticle lags because it's
//		using slightly stale data. This will feed it the newest data.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideWeaponHudAimVectors ( Vector *pAimOrigin, Vector *pAimDirection )
{
	if( !UseVR() )
	{
		return false;
	}

	Assert ( pAimOrigin != NULL );
	Assert ( pAimDirection != NULL );

	// So give it some nice high-fps numbers, not the low-fps ones we get from the game.
	*pAimOrigin = m_WorldFromWeapon.GetTranslation();
	*pAimDirection = m_WorldFromWeapon.GetForward();

	return true;
}


// --------------------------------------------------------------------
// Purpose:
//		Set up the left and right eyes from the middle eye if stereo is on.
//		Advise calling soonish after OverrideView().
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideStereoView( CViewSetup *pViewMiddle, CViewSetup *pViewLeft, CViewSetup *pViewRight  )
{
	// Everything in here is in Source coordinate space.
	if( !UseVR() )
	{
		return false;
	}

	VMatrix matOffsetLeft = g_pHLVR->GetMidEyeFromEye(ISourceVirtualReality::VREye_Left);
	VMatrix matOffsetRight = g_pHLVR->GetMidEyeFromEye(ISourceVirtualReality::VREye_Right);

	// Move eyes to IPD positions.
	VMatrix worldFromLeftEye  = m_WorldFromMidEye * matOffsetLeft;
	VMatrix worldFromRightEye = m_WorldFromMidEye * matOffsetRight;

	Assert ( IsOrthonormal ( worldFromLeftEye, 0.001f ) );
	Assert ( IsOrthonormal ( worldFromRightEye, 0.001f ) );

	// Finally convert back to origin+angles.
	MatrixAngles( worldFromLeftEye.As3x4(),  pViewLeft->angles, pViewLeft->origin );
	MatrixAngles( worldFromRightEye.As3x4(),  pViewRight->angles, pViewRight->origin );

	// Find the projection matrices.

	// TODO: this isn't the fastest thing in the world. Cache them?
	float headtrackFovScale = m_WorldZoomScale;
	pViewLeft->m_bViewToProjectionOverride = true;
	pViewRight->m_bViewToProjectionOverride = true;
	g_pHLVR->GetEyeProjectionMatrix (  &pViewLeft->m_ViewToProjection, ISourceVirtualReality::VREye_Left,  pViewMiddle->zNear, pViewMiddle->zFar, 1.0f/headtrackFovScale );
	g_pHLVR->GetEyeProjectionMatrix ( &pViewRight->m_ViewToProjection, ISourceVirtualReality::VREye_Right, pViewMiddle->zNear, pViewMiddle->zFar, 1.0f/headtrackFovScale );

	// And bodge together some sort of average for our cyclops friends.
	pViewMiddle->m_bViewToProjectionOverride = true;
	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			pViewMiddle->m_ViewToProjection.m[i][j] = (pViewLeft->m_ViewToProjection.m[i][j] + pViewRight->m_ViewToProjection.m[i][j] ) * 0.5f;
		}
	}

	switch ( vr_stereo_mono_set_eye.GetInt() )
	{
	case 0:
		// ... nothing.
		break;
	case 1:
		// Override all eyes with left
		*pViewMiddle = *pViewLeft;
		*pViewRight = *pViewLeft;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	case 2:
		// Override all eyes with right
		*pViewMiddle = *pViewRight;
		*pViewLeft = *pViewRight;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		break;
	case 3:
		// Override all eyes with middle
		*pViewRight = *pViewMiddle;
		*pViewLeft = *pViewMiddle;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	}

	// To make culling work correctly, calculate the widest FOV of each projection matrix.
	CalcFovFromProjection ( &(pViewLeft  ->fov), pViewLeft  ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewRight ->fov), pViewRight ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewMiddle->fov), pViewMiddle->m_ViewToProjection );

	// if we don't know the HUD FOV, figure that out now
	if( m_fHudHorizontalFov == 0.f )
	{
		// Figure out the current HUD FOV.
		m_fHudHorizontalFov = pViewLeft->fov * vr_hud_display_ratio.GetFloat() ;
		if( m_fHudHorizontalFov > vr_hud_max_fov.GetFloat() )
		{
			m_fHudHorizontalFov = vr_hud_max_fov.GetFloat();
		}
	}

	// remember the view angles so we can limit the weapon to something near those
	m_PlayerViewAngle = pViewMiddle->angles;
	m_PlayerViewOrigin = pViewMiddle->origin;



	// Figure out the HUD vectors and frustum.

	// The aspect ratio of the HMD may be something bizarre (e.g. Rift is 640x800), and the pixels may not be square, so don't use that!
	static const float fAspectRatio = 16.f/10.f;
	float fHFOV = m_fHudHorizontalFov;
	float fVFOV = m_fHudHorizontalFov / fAspectRatio;

	const float fHudForward = vr_hud_forward.GetFloat();
	m_fHudHalfWidth = tan( DEG2RAD( fHFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;
	m_fHudHalfHeight = tan( DEG2RAD( fVFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;



	QAngle HudAngles;
	if (IsMenuUp()) {
		HudAngles[YAW] = savedHudAngles[YAW]; //Use the last known yaw
		HudAngles[PITCH] = 0.0f;
		HudAngles[ROLL] = 0.0f;
		m_fHudHalfWidth *= hlvr_menu_scale.GetFloat();
		m_fHudHalfHeight *= hlvr_menu_scale.GetFloat();
	}
	else {
		HudAngles[ROLL] = 0.0f;
		QAngle aimAngles;
		HudAngles[YAW] = m_PlayerViewAngle[YAW];
		HudAngles[PITCH] = m_PlayerViewAngle[PITCH];
		savedHudAngles = HudAngles;
	}



	// when gui isn't up we'll do custom attached to weapon HUD......
	// TODO: is this still necessary at all?......
	if (!IsMenuUp() && false)
	{
		/*
		VMatrix m;
		AngleMatrix(m_PlayerTorsoAngle, m.As3x4());
		MatrixRotate(m, Vector(0, 1, 0), 50.f);
		MatrixAngles(m.As3x4(), HudAngles);
		AngleMatrix(HudAngles, m_WorldFromHud.As3x4());

		m_WorldFromHud.SetTranslation(m_PlayerViewOrigin);

		// Remember in source X forwards, Y left, Z up.
		// We need to transform to a more conventional X right, Y up, Z backwards before doing the projection.
		VMatrix WorldFromHudView;
		WorldFromHudView.SetForward(-m_WorldFromHud.GetLeft());//X vector
		WorldFromHudView.SetLeft(m_WorldFromHud.GetUp());//Y vector
		WorldFromHudView.SetUp(-m_WorldFromHud.GetForward());//Z vector
		WorldFromHudView.SetTranslation(m_PlayerViewOrigin);
		VMatrix HudProjection;
		HudProjection.Identity();

		HudProjection.m[0][0] = 0;
		HudProjection.m[1][1] = 0;
		// Z vector is not used/valid, but w is for projection.
		HudProjection.m[3][2] = -1.0f;
		// This will transform a world point into a homogeneous vector that
		//  when projected (i.e. divide by w) maps to HUD space [-1,1]
		m_HudProjectionFromWorld = HudProjection * WorldFromHudView.InverseTR();
		return true;
		*/
	}

	m_WorldFromHud.SetupMatrixOrgAngles( m_PlayerViewOrigin, HudAngles );

	// Remember in source X forwards, Y left, Z up.
	// We need to transform to a more conventional X right, Y up, Z backwards before doing the projection.
	VMatrix WorldFromHudView;
	WorldFromHudView./*X vector*/SetForward ( -m_WorldFromHud.GetLeft() );
	WorldFromHudView./*Y vector*/SetLeft    ( m_WorldFromHud.GetUp() );
	WorldFromHudView./*Z vector*/SetUp      ( -m_WorldFromHud.GetForward() );
	WorldFromHudView.SetTranslation         ( m_PlayerViewOrigin );

	VMatrix HudProjection;
	HudProjection.Identity();
	HudProjection.m[0][0] = fHudForward / m_fHudHalfWidth;
	HudProjection.m[1][1] = fHudForward / m_fHudHalfHeight;
	// Z vector is not used/valid, but w is for projection.
	HudProjection.m[3][2] = -1.0f;

	// This will transform a world point into a homogeneous vector that
	//  when projected (i.e. divide by w) maps to HUD space [-1,1]
	m_HudProjectionFromWorld = HudProjection * WorldFromHudView.InverseTR();

	return true;
}



// --------------------------------------------------------------------
// Purpose: Updates player orientation, position and motion according
//			to HMD status.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverridePlayerMotion(float flInputSampleFrametime, const QAngle &oldAngles, const QAngle &curAngles, const Vector &curMotion, QAngle *pNewAngles, Vector *pNewMotion)
{
	Assert(pNewAngles != NULL);
	Assert(pNewMotion != NULL);
	*pNewAngles = curAngles;
	*pNewMotion = curMotion;

	if (!UseVR())
	{
		return false;
	}


	m_bMotionUpdated = true;

	// originalAngles tells us what the weapon angles were before whatever mouse, joystick, etc thing changed them - called "old"
	// curAngles holds the new weapon angles after mouse, joystick, etc. applied.
	// We need to compute what weapon angles WE want and return them in *pNewAngles - called "new"



	VMatrix worldFromTorso;

	CBasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	// Whatever position is already here (set up by OverrideView) needs to be preserved.
	Vector vWeaponOrigin = m_WorldFromWeapon.GetTranslation();
	if (!runyet && pPlayer){
		g_pHLVR->rotationOffset = 0.f;
		QAngle hmdrot;
		MatrixToAngles(g_pHLVR->GetMideyePose(), hmdrot);
		g_pHLVR->rotationOffset = -hmdrot.y + pPlayer->GetAbsAngles().y;
		Msg("player: %f, head: %f, torso: %f\n", pPlayer->GetAbsAngles().y, hmdrot.y, m_PlayerTorsoAngle.y);
		savedPlayerViewOrigin = Vector(0.f, 0.f, 0.f);
		runyet = true;
	}
	if (!isnan<float>(curAngles.y)) {
		g_pHLVR->rotationOffset += curAngles.y - oldAngles.y;
	}
	g_pHLVR->rotationOffset += m_PlayerTorsoAngle.y;

	m_PlayerTorsoAngle = QAngle(0, 0, 0);
	if (g_pHLVR->rotationOffset > 180)
		g_pHLVR->rotationOffset -= 360;
	if (g_pHLVR->rotationOffset < -180)
		g_pHLVR->rotationOffset += 360;

	worldFromTorso.SetupMatrixAngles(m_PlayerTorsoAngle);


	// Weapon view = mideye view, so apply that to the torso to find the world view direction.
	m_WorldFromWeapon = worldFromTorso * m_TorsoFromMideye;

	//Override rotation, disabled as it's done later per the weapon
	//g_pHLVR->OverrideWeaponMatrix(m_WorldFromWeapon); 

	MatrixAngles(m_WorldFromWeapon.As3x4(), *pNewAngles);


	// Restore the translation.
	m_WorldFromWeapon.SetTranslation(vWeaponOrigin);

	// Figure out player motion.
	VMatrix mideyeFromWorld = m_WorldFromMidEye.InverseTR();
	VMatrix newMidEyeFromWeapon = mideyeFromWorld * m_WorldFromWeapon;
	newMidEyeFromWeapon.SetTranslation(Vector(0.0f, 0.0f, 0.0f));;
	*pNewMotion = newMidEyeFromWeapon * curMotion;

	if (hlvr_control_hand.GetInt() != 0) {
		QAngle handRotQ = QAngle(0.f, 0.f, 0.f);
		if (hlvr_control_hand.GetInt() == 1) handRotQ = (g_pHLVR->GetLeftHandRotation());
		if (hlvr_control_hand.GetInt() == 2) handRotQ = (g_pHLVR->GetRightHandRotation());
		handRotQ[YAW] = AngleDiff(curAngles[YAW], handRotQ[YAW]);

		if (pPlayer != NULL && pPlayer->GetMoveType() != MOVETYPE_WALK)
		{
			handRotQ[PITCH] = AngleDiff(curAngles[PITCH], handRotQ[PITCH]);
		}
		else {
			handRotQ[PITCH] = 0.f;
		}
		handRotQ[ROLL] = 0.f;
		matrix3x4_t handRot34;
		AngleMatrix(handRotQ, handRot34);
		*pNewMotion = (VMatrix)handRot34 * *pNewMotion;
	}

	// remember the motion for stat tracking
	m_PlayerLastMovement = *pNewMotion;

	return true;
}


// --------------------------------------------------------------------
// Purpose: Returns true if the world is zoomed
// --------------------------------------------------------------------
bool CClientVirtualReality::CurrentlyZoomed()
{
	return ( m_WorldZoomScale != 1.0f );
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to keep the torso angle of the player
//			fixed at this point until the game tells us something
//			different.
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideTorsoTransform( const Vector & position, const QAngle & angles )
{
	g_pHLVR->ProcessVehicleYawOffset(angles.y);
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to resume using its own notion of
//			where the torso is pointed.
// --------------------------------------------------------------------
void CClientVirtualReality::CancelTorsoTransformOverride()
{
	m_bOverrideTorsoAngle = false;
}


bool CClientVirtualReality::CanOverlayHudQuad()
{
	return false;
}



// --------------------------------------------------------------------
// Purpose: Returns the bounds in world space where the game should 
//			position the HUD.
// --------------------------------------------------------------------
void CClientVirtualReality::GetHUDBounds(Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR)
{
	Vector vHalfWidth = m_WorldFromHud.GetLeft() * -m_fHudHalfWidth;
	Vector vHalfHeight = m_WorldFromHud.GetUp() * m_fHudHalfHeight;

	QAngle vmAngles;
	Vector vmOrigin, hudRight, hudForward, hudUp, vHUDOrigin;
	if (!IsMenuUp())
	{
		vHUDOrigin = m_PlayerViewOrigin + m_WorldFromHud.GetForward() * vr_hud_forward.GetFloat();
		savedPlayerViewOrigin = m_PlayerViewOrigin;
	}
	else {
		if (savedPlayerViewOrigin == Vector(0.f, 0.f, 0.f))
			savedPlayerViewOrigin = m_PlayerViewOrigin;
		vHUDOrigin = savedPlayerViewOrigin + m_WorldFromHud.GetForward() * hlvr_menu_forward.GetFloat();
	}

	CBasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	if (false)
	{
		/*
		// If the menu isn't up, check for a weapon and if so mount the hud to it
		if (pPlayer != NULL)
		{
			C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
			if (pWeapon)
			{
				C_BaseViewModel *vm = pPlayer->GetViewModel(0);
				if (vm)
				{
					int iAttachment = vm->LookupAttachment("hud_left");
					vm->GetAttachment(iAttachment, vmOrigin, vmAngles);

					VMatrix worldFromPanel;
					AngleMatrix(vmAngles, worldFromPanel.As3x4());
					MatrixRotate(worldFromPanel, Vector(1, 0, 0), -90.f);
					worldFromPanel.GetBasisVectors(hudForward, hudRight, hudUp);

					static const float aspectRatio = 4.f / 3.f;
					float width = 24; // vr_hud_width.GetFloat();
					float height = width / aspectRatio;

					vHalfWidth = hudRight * width / 2.f;
					vHalfHeight = hudUp  *  height / 2.f;
				}
			}
		}

		vHUDOrigin = vmOrigin + hudRight*-5 + hudForward + hudUp*-1 + vHalfWidth;
		*/
	}

	*pViewer = m_PlayerViewOrigin;
	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;
}



// --------------------------------------------------------------------
// Purpose: Renders the HUD in the world.
// --------------------------------------------------------------------
void CClientVirtualReality::RenderHUDQuad(bool bBlackout, bool bTranslucent)
{
	if (engine->IsLevelMainMenuBackground()) 
		return;	//Main Menus are rendered onto entities

	// If we can overlay the HUD directly onto the target later, we'll do that instead (higher image quality).
	if (CanOverlayHudQuad())
		return;

	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds(&vHead, &vUL, &vUR, &vLL, &vLR);

	CMatRenderContextPtr pRenderContext(materials);

	{
		IMaterial *mymat = NULL;
		if (bTranslucent)
		{
			mymat = materials->FindMaterial("vgui/inworldui", TEXTURE_GROUP_VGUI);

			// this is mounted on the left side of the gun in game, so allow the alpha to be modulated 
			// for fade in effect...
			/* Disabled. The UI quad is a HUD again, so this would be silly
			VMatrix mWeap(m_WorldFromWeapon);
			g_pHLVR->OverrideWeaponMatrix(mWeap);
			float alpha = g_pHLVR->GetHudPanelAlpha(mWeap.GetLeft(), m_WorldFromMidEye.GetForward(), 2.5);
			mymat->AlphaModulate(alpha);
			*/
		}
		else
		{
 			mymat = materials->FindMaterial("vgui/inworldui_opaque", TEXTURE_GROUP_VGUI);
		}

		Assert(!mymat->IsErrorMaterial());

		if (!mymat->IsPrecached()) {
			Msg("UI is Not Cached, Caching now...\n");
			PrecacheMaterial(mymat->GetName());
		}

		IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

		CMeshBuilder meshBuilder;
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

		meshBuilder.Position3fv(vLR.Base());
		meshBuilder.TexCoord2f(0, 1, 1);
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv(vLL.Base());
		meshBuilder.TexCoord2f(0, 0, 1);
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv(vUR.Base());
		meshBuilder.TexCoord2f(0, 1, 0);
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv(vUL.Base());
		meshBuilder.TexCoord2f(0, 0, 0);
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.End();
		pMesh->Draw();
	}

	if (bBlackout)
	{
		Vector vbUL, vbUR, vbLL, vbLR;
		// "Reflect" the HUD bounds through the viewer to find the ones behind the head.
		vbUL = 2 * vHead - vLR;
		vbUR = 2 * vHead - vLL;
		vbLL = 2 * vHead - vUR;
		vbLR = 2 * vHead - vUL;

#if defined( HL2_EPISODIC )
		IMaterial *mymat = materials->FindMaterial("vgui/black", TEXTURE_GROUP_VGUI, false);
#else
		IMaterial *mymat = materials->FindMaterial("vgui/HLVR_logo", TEXTURE_GROUP_VGUI, false);
#endif
		IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

		// Tube around the outside.
		CMeshBuilder meshBuilder;
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 8);

		meshBuilder.Position3fv(vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();

		// Cap behind the viewer.
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

		meshBuilder.Position3fv(vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();
	}
}



// --------------------------------------------------------------------
// Purpose: Gets the amount of zoom to apply
// --------------------------------------------------------------------
float CClientVirtualReality::GetZoomedModeMagnification()
{
	return m_WorldZoomScale * vr_zoom_scope_scale.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Does some client-side tracking work and then tells headtrack
//			to do its own work.
// --------------------------------------------------------------------
bool CClientVirtualReality::ProcessCurrentTrackingState( float fGameFOV )
{
	m_WorldZoomScale = 1.0f;
	if ( fGameFOV != 0.0f )
	{
		// To compensate for the lack of pixels on most HUDs, let's grow this a bit.
		// Remember that MORE zoom equals LESS fov!
		fGameFOV *= ( 1.0f / vr_zoom_multiplier.GetFloat() );
		fGameFOV = Min ( fGameFOV, 170.0f );

		// The game has overridden the FOV, e.g. because of a sniper scope. So we need to match this view with whatever actual FOV the HUD has.
		float wantedGameTanfov = tanf ( DEG2RAD ( fGameFOV * 0.5f ) );
		// OK, so now in stereo mode, we're going to also draw an overlay, but that overlay usually covers more of the screen (because in a good HMD usually our actual FOV is much wider)
		float overlayActualPhysicalTanfov = tanf ( DEG2RAD ( m_fHudHorizontalFov * 0.5f ) );
		// Therefore... (remembering that a zoom > 1.0 means you zoom *out*)
		m_WorldZoomScale = wantedGameTanfov / overlayActualPhysicalTanfov;
	}

	return g_pHLVR->SampleTrackingState( fGameFOV, 0.f /* seconds to predict */ );
}


// --------------------------------------------------------------------
// Purpose: Returns the projection matrix to use for the HUD
// --------------------------------------------------------------------
const VMatrix &CClientVirtualReality::GetHudProjectionFromWorld()
{
	// This matrix will transform a world-space position into a homogenous HUD-space vector.
	// So if you divide x+y by w, you will get the position on the HUD in [-1,1] space.
	return m_HudProjectionFromWorld;
}

// --------------------------------------------------------------------
// Purpose: Returns the aim vector relative to the torso
// --------------------------------------------------------------------
void CClientVirtualReality::GetTorsoRelativeAim(Vector *pPosition, QAngle *pAngles)
{
	MatrixAngles(g_pHLVR->GetMideyePose().As3x4(), *pAngles);
}


// --------------------------------------------------------------------
// Purpose: Returns distance of the HUD in front of the eyes.
// --------------------------------------------------------------------
float CClientVirtualReality::GetHUDDistance()
{
	return vr_hud_forward.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Returns true if the HUD should be rendered into a render
//			target and then into the world on a quad.
// --------------------------------------------------------------------
bool CClientVirtualReality::ShouldRenderHUDInWorld()
{
	return UseVR();
}


// --------------------------------------------------------------------
// Purpose: Lets headtrack tweak the view model origin and angles to match
//			aim angles and handle strange viewmode FOV stuff
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideViewModelTransform(Vector & vmorigin, QAngle & vmangles, bool bUseLargeOverride)
{	
	g_pHLVR->UpdateViewmodelOffset(vmorigin, vmangles);
	return;
}


// --------------------------------------------------------------------
// Purpose: Tells the head tracker to reset the torso position in case
//			we're on a drifty tracker.
// --------------------------------------------------------------------
void CClientVirtualReality::AlignTorsoAndViewToWeapon()
{
	return;
}


// --------------------------------------------------------------------
// Purpose: Lets VR do stuff at the very end of the rendering process
// --------------------------------------------------------------------
void CClientVirtualReality::PostProcessFrame( StereoEye_t eEye )
{
	if( !UseVR() )
		return;

	g_pHLVR->DoDistortionProcessing( eEye == STEREO_EYE_LEFT ? ISourceVirtualReality::VREye_Left : ISourceVirtualReality::VREye_Right );
}


// --------------------------------------------------------------------
// Pastes the HUD directly onto the backbuffer / render target.
// (higher quality than the RenderHUDQuad() path but can't always be used)
// --------------------------------------------------------------------
void CClientVirtualReality::OverlayHUDQuadWithUndistort( const CViewSetup &eyeView, bool bDoUndistort, bool bBlackout, bool bTranslucent )
{
	if ( ! UseVR() )
		return;

	// If we can't overlay the HUD, it will be handled on another path (rendered into the scene with RenderHUDQuad()).
	if ( ! CanOverlayHudQuad() )
		return;

	// Get the position of the HUD quad in world space as used by RenderHUDQuad().  Then convert to a rectangle in normalized
	// device coordinates.

	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds ( &vHead, &vUL, &vUR, &vLL, &vLR );

	VMatrix worldToView, viewToProjection, worldToProjection, worldToPixels;
	render->GetMatricesForView( eyeView, &worldToView, &viewToProjection, &worldToProjection, &worldToPixels );

	Vector pUL, pUR, pLL, pLR;

	worldToProjection.V3Mul( vUL, pUL );
	worldToProjection.V3Mul( vUR, pUR );
	worldToProjection.V3Mul( vLL, pLL );
	worldToProjection.V3Mul( vLR, pLR );

	float ndcHudBounds[4];
	ndcHudBounds[0] = Min ( Min( pUL.x, pUR.x ), Min( pLL.x, pLR.x ) );
	ndcHudBounds[1] = Min ( Min( pUL.y, pUR.y ), Min( pLL.y, pLR.y ) );
	ndcHudBounds[2] = Max ( Max( pUL.x, pUR.x ), Max( pLL.x, pLR.x ) );
	ndcHudBounds[3] = Max ( Max( pUL.y, pUR.y ), Max( pLL.y, pLR.y ) );

	ISourceVirtualReality::VREye sourceVrEye = ( eyeView.m_eStereoEye == STEREO_EYE_LEFT ) ? ISourceVirtualReality::VREye_Left : ISourceVirtualReality::VREye_Right;

	g_pHLVR->CompositeHud ( sourceVrEye, ndcHudBounds, bDoUndistort, bBlackout, bTranslucent );
}


// --------------------------------------------------------------------
// Purpose: Switches to VR mode
// --------------------------------------------------------------------
void CClientVirtualReality::Activate()
{
	if (!g_pHLVR->Activate())
		return;

	//General all-game stuff
	engine->ExecuteClientCmd("mat_reset_rendertargets\n");

	//Game specific VR config
	engine->ExecuteClientCmd("exec hlvr\n");

	//Support for two cursors, 
	vgui::surface()->SetSoftwareCursor(true);

#if defined(POSIX)
	ConVarRef m_rawinput( "m_rawinput" );
	m_bNonVRRawInput = m_rawinput.GetBool();
	m_rawinput.SetValue( 1 );

	ConVarRef mat_vsync( "mat_vsync" );
	mat_vsync.SetValue( 0 );
#endif


	vgui::ivgui()->SetVRMode(true);
	g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget(0, 0, g_pHLVR->screenWidth, g_pHLVR->screenHeight, NULL);

}


void CClientVirtualReality::Deactivate()
{
	// can't deactivate when we aren't active
	if( !UseVR() )
		return;

	g_pHLVR->Deactivate();

	//g_pMatSystemSurface->ForceScreenSizeOverride(false, 0, 0 );
	//g_pMaterialSystem->GetRenderContext()->Viewport( 0, 0, m_nNonVRWidth, m_nNonVRHeight );
	//g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget( 0, 0, m_nNonVRWidth, m_nNonVRHeight, NULL );

    static ConVarRef cl_software_cursor( "cl_software_cursor" );
    vgui::surface()->SetSoftwareCursor( cl_software_cursor.GetBool() );

#if defined( USE_SDL )
    static ConVarRef sdl_displayindex( "sdl_displayindex" );
    sdl_displayindex.SetValue( m_nNonVRSDLDisplayIndex );
#endif

#if defined(POSIX)
    ConVarRef m_rawinput( "m_rawinput" );
    m_rawinput.SetValue( m_bNonVRRawInput );
#endif

	engine->ExecuteClientCmd( "mat_reset_rendertargets\n" );
}


// Called when startup is complete
void CClientVirtualReality::StartupComplete()
{
	if ( vr_activate_default.GetBool() || ShouldForceVRActive() )
		Activate();
}


void CClientVirtualReality::GetEyeAngles(QAngle & eyeAngles)
{
	MatrixAngles(m_WorldFromMidEye.As3x4(), eyeAngles);
}

CON_COMMAND(hlvr_resetrotation, "Used automatically to set player's rotation initially. Running it may break stuff"){
	runyet = false;
}