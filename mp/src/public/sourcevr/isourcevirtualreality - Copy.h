//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the IHeadTrack interface, which is implemented in headtrack.dll
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef ISOURCEVIRTUALREALITY_H
#define ISOURCEVIRTUALREALITY_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "tier1/refcount.h"
#include "appframework/IAppSystem.h"
#include "mathlib/vmatrix.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class ITexture;
class IMaterialSystem;

//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

struct VRRect_t
{
	int32 nX;
	int32 nY;
	int32 nWidth;
	int32 nHeight;
};


// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define SOURCE_VIRTUAL_REALITY_INTERFACE_VERSION "SourceVirtualReality001"

//-----------------------------------------------------------------------------
// The ISourceVirtualReality interface
//-----------------------------------------------------------------------------



abstract_class ISourceVirtualReality : public IAppSystem
{
public:
	virtual ~ISourceVirtualReality() {}

	// Placeholder for API revision
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	// This enum is used to tell some of the other calls in this interface which eye
	// is being requested.
	enum VREye
	{
		VREye_Left = 0,
		VREye_Right
	};

	// Which texture is being requested in GetRenderTarget?
	enum EWhichRenderTarget
	{
		RT_Color = 0,
		RT_Depth,
	};


	// ----------------------------------------------------------------------
	// General utilities
	// ----------------------------------------------------------------------

	// Returns true if the game should run in VR mode
	virtual bool ShouldRunInVR() = 0;

	// Returns true if there is a compatible HMD connected 
	virtual bool IsHmdConnected() = 0;

	// The size and position of the viewport for the specified eye
	virtual void GetViewportBounds( VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight ) = 0;

	// Performs the distortion post-processing.
	virtual bool DoDistortionProcessing ( VREye eEye ) = 0;

	// Composites the HUD directly onto the backbuffer / render target, including undistort.
	virtual bool CompositeHud ( VREye eEye, float ndcHudBounds[4], bool bDoUndistort, bool bBlackout, bool bTranslucent ) = 0;

	// ----------------------------------------------------------------------
	// Getting the current pose
	// ----------------------------------------------------------------------

	// returns the pose relative to the zero point
	virtual VMatrix GetMideyePose() = 0;

	// All-in-one interfaces (they call GetCameraPoseZeroFromCurrent)
	// Grabs the current tracking data and sets up state for the Override* calls.
	virtual bool SampleTrackingState ( float PlayerGameFov, float fPredictionSeconds ) = 0;

	// ----------------------------------------------------------------------
	// Information about the display
	// ----------------------------------------------------------------------

	// Passes back the bounds of the window that the game should create. This might
	// span two displays if we're dealing with a two-input display. Returns true
	// if the bounds were set.
	virtual bool GetDisplayBounds( VRRect_t *pRect ) = 0;

	// Computes and returns the projection matrix for the eye
	virtual bool GetEyeProjectionMatrix ( VMatrix *pResult, VREye, float zNear, float zFar, float fovScale ) = 0;

	// Returns the transform from the mid-eye to the specified eye. Multiply this by 
	// the tweaked (for mouse rotation and WASD translation) mideye position to get the
	// view matrix. This matrix takes the user's IPD into account.
	virtual VMatrix GetMidEyeFromEye( VREye eEye ) = 0;

	// returns the adapter index to use for VR mode
	virtual int GetVRModeAdapter() = 0;

	// ----------------------------------------------------------------------
	// Information about the tracker
	// ----------------------------------------------------------------------

	virtual bool WillDriftInYaw() = 0;

	// ----------------------------------------------------------------------
	// Methods about oversized offscreen rendering
	// ----------------------------------------------------------------------

	// Sets up the pre-distortion render targets.
	virtual void CreateRenderTargets( IMaterialSystem *pMaterialSystem ) = 0;
	virtual void ShutdownRenderTargets() = 0;

	// fetches the render target for the specified eye
	virtual ITexture *GetRenderTarget( VREye eEye, EWhichRenderTarget eWhich ) = 0;

	// Returns the (possibly overridden) framebuffer size for render target sizing.
	virtual void				GetRenderTargetFrameBufferDimensions( int & nWidth, int & nHeight ) = 0;

	// ----------------------------------------------------------------------
	// Enter/leave VR mode
	// ----------------------------------------------------------------------
	virtual bool Activate() = 0;
	virtual void Deactivate() = 0;
	
	virtual bool ShouldForceVRMode() = 0;
	virtual void SetShouldForceVRMode() = 0;

};



//-----------------------------------------------------------------------------
#include "../game/client/hlvr/proxydll.h"
#ifndef VRMANAGER_H
#define VRMANAGER_H
/*
struct vr_interface_t
{
	bool(*hmdOpen)(bool isSitting, sdkType sdkID);
	//bool(*hmdOpen)(bool isSitting, int sdkID);
	bool(*hmdIsConnected)(void);
	void(*hmdClose)(void);
	void(*hmdGetProjection)(int eye, float matrix[4][4]);
	//float(**hmdGetProjection)(int eye);
	//POINT(*hmdGetResolution)(void);
	//POINT(*hmdGetBufferSize)(void);
	float(*hmdGetIPD)(void);
	void(*hmdBeginFrame)(void);
	void(*hmdGetTrackingData)(float hMatrix[4][4], float lcMatrix[4][4], float rcMatrix[4][4]);
	float(**hmdGetTrackingHMD)(void);
	float(**hmdGetTrackingLeftController)(void);
	float(**hmdGetTrackingRightController)(void);
	bool(*GetButtonHasChanged)(ButtonList buttonID, ControllerType controllerType);
	bool(*GetButtonIsTouched)(ButtonList buttonID, ControllerType controllerType);
	bool(*GetButtonIsPressed)(ButtonList buttonID, ControllerType controllerType);
	bool(*GetButtonIsDownFrame)(ButtonList buttonID, ControllerType controllerType);
	bool(*GetButtonIsUpFrame)(ButtonList buttonID, ControllerType controllerType);
	float(*GetButtonValue)(ButtonList buttonID, ControllerType controllerType);
	void(*hmdRecenter)(void);
	void(*hmdSetPrediction)(float prediction);
};
*/
class HLVirtualReality{// : public CAutoGameSystemPerFrame {
public:
	HLVirtualReality::HLVirtualReality();
	bool BuildInterface();
	int screenWidth, screenHeight;

	bool DllLoaded();

	bool ShouldRunInVR();

	bool Activate();

	void Deactivate();

	void GetViewportBounds(ISourceVirtualReality::VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight);

	ITexture *GetRenderTarget(ISourceVirtualReality::VREye eEye, ISourceVirtualReality::EWhichRenderTarget eWhich);

	VMatrix GetMideyePose();

	VMatrix GetMidEyeFromEye(ISourceVirtualReality::VREye eEye);

	bool GetEyeProjectionMatrix(VMatrix *pResult, ISourceVirtualReality::VREye eEye, float zNear, float zFar, float fovScale);

	bool SampleTrackingState(float PlayerGameFov, float fPredictionSeconds);

	bool WillDriftInYaw();

	bool DoDistortionProcessing(ISourceVirtualReality::VREye eEye);

	bool CompositeHud(ISourceVirtualReality::VREye eEye, float ndcHudBounds[4], bool bDoUndistort, bool bBlackout, bool bTranslucent);

	int GetVRModeAdapter();

	bool GetDisplayBounds(VRRect_t *pRect);

	VMatrix SMMatrixToVMatrix(float matrix[16], bool isHand = false);

	void OverrideWeaponMatrix(VMatrix& weaponMatrix);

	void UpdateViewmodelOffset(Vector& vmorigin, QAngle& vmangles);

	void GetEyeToWeaponOffset(Vector& offset);

	QAngle GetWeaponRotation();

	void GetEyeToHandOffset(Vector& offset);

	QAngle GetHandRotation();

	void UpdatePoses();

	void OverrideJoystickInputs(float& jx, float& jy, float& lx, float& ly);

	float GetHudPanelAlpha(const Vector& hudPanelForward, const Vector& eyesForward, float fadePow);

	void ProcessInput();

	void ProcessVehicleYawOffset(CBasePlayer* pPlayer);

	float rotationOffset;

	void LevelShutdownPreEntity();

	void LevelInitPostEntity();

	void Update(float frametime);

	ITexture* CreateVROneEyeTextureFullSize(IMaterialSystem* pMaterialSystem);

	void Shutdown();

//	vr_interface_t* m_pVRInterface;

//private:
//	ITexture* m_pLeftEye;
//	ITexture* m_pRightEye;
};

extern HLVirtualReality g_pHLVR;

#endif

extern ISourceVirtualReality *g_pSourceVR;

inline bool UseVR()
{
	//return g_pHLVR.ShouldRunInVR();
	return cvar->FindVar("usevr")->GetBool();// true;	// Added for Anarchy Arcade
}

inline bool ShouldForceVRActive()
{
	return false;
}

#endif // ISOURCEVIRTUALREALITY_H
