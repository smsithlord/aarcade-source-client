#ifndef PROXYDLL
#define PROXYDLL

#ifdef VR_ALLOWED
#include "../aarcade/client/aa_globals.h"
//#pragma once
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
#include <d3d9.h>
#include "../game/client/hlvr/stCommon.h"

#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)

// Function to extend the dx9 buffer by the given width or height
// If negative number is used, that value is set rather than added
void DllExport hmdExtendDX9Buffer(int width, int height);

// Changes the next draw call information to modify viewport and projection matrix
void DllExport SetDrawState(int renderStatus, int sOffProj, int sOffView);

// Called to set the next image as part of the headset swapchain
void DllExport hmdSetNextTextureType(hmdTextureType textureType, hmdTextureType index);

IDirect3DSurface9 DllExport *hmdGetSharedSurface();

// Called at the start of the frame to setup head / controller tracking values for the frame
void  DllExport hmdSetFrame();

// Called at the end of the frame to tell the headset to render the frame
void  DllExport hmdEndFrame();

// Sets the directx9 device (doing this within the dll causes issues when the dll is loaded multiple times)
void  DllExport hmdSetDevice(IDirect3DDevice9* D3D9Device);

// Sets the sdk type : openvr, oculus etc
void  DllExport hmdSetSDK(sdkType sdkID);
void  DllExport hmdSetSDK(int sdkID);

// Lets the sdk know if the user is standing or sitting
void  DllExport hmdSetIsSitting(bool isSitting);

// Opens the headset connection. useThreaded runs hmdBegin/EndFrame in a seporate thread so the main game is uncapped fps
bool  DllExport hmdOpen();

// Returns true if a headset connection is open
bool  DllExport hmdIsConnected();

// Closes the open headset connection
void  DllExport hmdClose();

// Gets the hmd projection matrix for a paticular eye : 0 = left, 1 = right
void  DllExport hmdGetProjection(int eye, float matrix[4][4], bool invert = false);
float DllExport *hmdGetProjection(int eye, bool invert = false);

// Gets the hmd eye to hmd matrix for a paticular eye : 0 = left, 1 = right
void  DllExport hmdGetEyeView(int eye, float matrix[4][4], bool invert = false);
float DllExport *hmdGetEyeView(int eye, bool invert = false);

// Gets the hmd resolution / prefered buffer size / IPD
POINT DllExport hmdGetResolution();
POINT DllExport hmdGetBufferSize();
float DllExport hmdGetIPD();

// Returns the tracking data for the headset and controllers
void  DllExport hmdGetTrackingData(float hMatrix[4][4], float lcMatrix[4][4], float rcMatrix[4][4]);
float DllExport *hmdGetTrackingHMD();
float DllExport *hmdGetTrackingLeftController();
float DllExport *hmdGetTrackingRightController();

// Resets the saved controller button information (is down frame, is up frame)
void  DllExport hmdResetTracking();

// Gets the button information
bool  DllExport hmdGetButtonHasChanged(ButtonList buttonID, ControllerType controllerType);
bool  DllExport hmdGetButtonIsTouched(ButtonList buttonID, ControllerType controllerType);
bool  DllExport hmdGetButtonIsPressed(ButtonList buttonID, ControllerType controllerType);
bool  DllExport hmdGetButtonIsDownFrame(ButtonList buttonID, ControllerType controllerType);
bool  DllExport hmdGetButtonIsUpFrame(ButtonList buttonID, ControllerType controllerType);
float DllExport hmdGetButtonValue(ButtonList buttonID, ControllerType controllerType);

// Recenters the headset
void  DllExport hmdRecenter();

// Sets a prediction value for the headset
void  DllExport hmdSetPrediction(float prediction);

// Gets the number of textures used in the swap chain
int   DllExport hmdGetFrameCount();

// Gets the current swap chain index
int   DllExport hmdGetCurrentTextureIndex();

// Starts / stops the controller vibration : controllerID 0 = left, 1 = right, vibratePercent = 0 -> 100
void  DllExport hmdControllerVibrateStart(int controllerID, int vibratePercent);
void  DllExport hmdControllerVibrateStop(int controllerID);

// Sets the texture or surface that is being render too either for both eyes at once, or seporate
void DllExport hmdSetTexture(IDirect3DTexture9 *bothEyes, bool manualSet = false);
void DllExport hmdSetTexture(HANDLE textureID, IDirect3DTexture9 *bothEyes, bool manualSet = false);
void DllExport hmdSetSurface(IDirect3DSurface9 *bothEyes, bool manualSet = false);
void DllExport hmdSetTexture(IDirect3DTexture9 *leftEye, IDirect3DTexture9 *rightEye, bool manualSet = false);
void DllExport hmdSetSurface(IDirect3DSurface9 *leftEye, IDirect3DSurface9 *rightEye, bool manualSet = false);

// Saves the render surface to a texture, if rect is null or not included then it captures the whole surface
void DllExport hmdCaptureScreen(char *fileName);
void DllExport hmdCaptureScreen(char *fileName, RECT *rect);
void DllExport hmdCaptureScreen(char *fileName, int left, int top, int right, int bottom);

void DllExport hmdCaptureScreen(const char fileName[], int left, int top, int right, int bottom);


// Added for Anarchy Arcade
// Explicit typedef to avoid including d3d9.h (which turns certain Source engine methods into defines.)
//
/*
typedef struct tagPOINT
{
LONG  x;
LONG  y;
} POINT;// , *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;

*/
//typedef long LONG;
/*
struct tagPOINT_t
{
	int  x;
	int  y;
};// , *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;
// End added for Anarchy Arcade

#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)
*/
/*
namespace sdkTypes {
	enum _sdkTypes
	{
		NA = 0,
		openvr = 1,
		oculus = 2,
	};
};

namespace MouseButtons
{
	enum _MouseButtons
	{
		left,
		middle,
		right,
	};
};

namespace ControllerID
{
	enum _ControllerID
	{
		NA = 0,
		openvrVive = 1,
		openvrTouch = 2,
		oculusTouch = 3,
	};
};

namespace HeadsetIDs {
	enum _HeadsetIDs
	{
		NA = 0,
		vive = 1,
		rift = 2,
	};
};

typedef sdkTypes::_sdkTypes sdkType;
typedef ButtonsList::_ButtonsList ButtonList;
typedef ControllerTypes::_ControllerTypes ControllerType;
typedef MouseButtons::_MouseButtons AMouseButton;
typedef ControllerID::_ControllerID ControllerList;
typedef HeadsetIDs::_HeadsetIDs HeadsetList;
*/

// Blank API just so I can disable the VR DLL
/*
void  hmdMouseDown(AMouseButton button) { return; }
void  hmdMouseUp(AMouseButton button) { return; }
void  hmdEscDown() { return; }
void  hmdEscUp() { return; }

bool  hmdOpen(bool isSitting, sdkType sdkID) { return false; }
bool  hmdOpen(bool isSitting, int sdkID) { return false; }
bool  hmdIsConnected() { return false; }
void  hmdClose() { return; }
void  hmdGetProjection(int eye, float matrix[4][4]) { return; }
float *hmdGetProjection(int eye);
//tagPOINT_t DllExport hmdGetResolution();
//tagPOINT_t DllExport hmdGetBufferSize();
float hmdGetIPD() { return 0; }
void  hmdBeginFrame() { return; }
void  hmdGetTrackingData(float hMatrix[4][4], float lcMatrix[4][4], float rcMatrix[4][4]) { return; }
float *hmdGetTrackingHMD() { return 0; }
float *hmdGetTrackingLeftController() { return 0; }
float *hmdGetTrackingRightController() { return 0; }
float *hmdGetTrackingGeneric(int index) { return 0; }
bool  GetButtonHasChanged(ButtonList buttonID, ControllerType controllerType) { return false; }
bool  GetButtonIsTouched(ButtonList buttonID, ControllerType controllerType) { return false; }
bool  GetButtonIsPressed(ButtonList buttonID, ControllerType controllerType) { return false; }
bool  GetButtonIsDownFrame(ButtonList buttonID, ControllerType controllerType) { return false; }
bool  GetButtonIsUpFrame(ButtonList buttonID, ControllerType controllerType) { return false; }
float GetButtonValue(ButtonList buttonID, ControllerType controllerType) { return 0; }
void  hmdRecenter() { return; }
void  hmdSetPrediction(float prediction) { return; }
HeadsetList hmdGetHeadsetType();
ControllerList hmdGetControllerType();
*/

/*
#pragma comment (lib, "pd3d9.lib")
void  DllExport hmdMouseDown(AMouseButton button);
void  DllExport hmdMouseDown(AMouseButton button);
void  DllExport hmdMouseUp(AMouseButton button);
void  DllExport hmdEscDown();
void  DllExport hmdEscUp();

bool  DllExport hmdOpen(bool isSitting, sdkType sdkID);
bool  DllExport hmdOpen(bool isSitting, int sdkID);
bool  DllExport hmdIsConnected();
void  DllExport hmdClose();
void  DllExport hmdGetProjection(int eye, float matrix[4][4]);
float DllExport *hmdGetProjection(int eye);
tagPOINT_t DllExport hmdGetResolution();
tagPOINT_t DllExport hmdGetBufferSize();
float DllExport hmdGetIPD();
void  DllExport hmdBeginFrame();
void  DllExport hmdGetTrackingData(float hMatrix[4][4], float lcMatrix[4][4], float rcMatrix[4][4]);
float DllExport *hmdGetTrackingHMD();
float DllExport *hmdGetTrackingLeftController();
float DllExport *hmdGetTrackingRightController();
float DllExport *hmdGetTrackingGeneric(int index);
bool  DllExport GetButtonHasChanged(ButtonList buttonID, ControllerType controllerType);
bool  DllExport GetButtonIsTouched(ButtonList buttonID, ControllerType controllerType);
bool  DllExport GetButtonIsPressed(ButtonList buttonID, ControllerType controllerType);
bool  DllExport GetButtonIsDownFrame(ButtonList buttonID, ControllerType controllerType);
bool  DllExport GetButtonIsUpFrame(ButtonList buttonID, ControllerType controllerType);
float DllExport GetButtonValue(ButtonList buttonID, ControllerType controllerType);
void  DllExport hmdRecenter();
void  DllExport hmdSetPrediction(float prediction);
HeadsetList DllExport hmdGetHeadsetType();
ControllerList DllExport hmdGetControllerType();
*/

///*
//#include "stCommon.h"

//#pragma comment (lib, "pd3d9.lib")

//typedef ButtonsList::_ButtonsList ButtonList;
//typedef ControllerTypes::_ControllerTypes ControllerType;

//----
// Global functions for external use
//----

/*
// Called at the start of the frame to setup head / controller tracking values for the frame
void  DllExport hmdBeginFrame(bool useThreaded);

// Called at the end of the frame to tell the headset to render the frame
void  DllExport hmdEndFrame(bool useThreaded);

// Sets the directx9 device (doing this within the dll causes issues when the dll is loaded multiple times)
void  DllExport hmdSetDevice(IDirect3DDevice9* D3D9Device);

// Sets the sdk type : openvr, oculus etc
void  DllExport hmdSetSDK(sdkType sdkID);
void  DllExport hmdSetSDK(int sdkID);

// Lets the sdk know if the user is standing or sitting
void  DllExport hmdSetIsSitting(bool isSitting);

// Opens the headset connection. useThreaded runs hmdBegin/EndFrame in a seporate thread so the main game is uncapped fps
bool  DllExport hmdOpen(bool useThreaded);

// Returns true if a headset connection is open
bool  DllExport hmdIsConnected();

// Closes the open headset connection
void  DllExport hmdClose();

// Tells the headset we have finished with the current frame
void  DllExport hmdSubmitFrame();

// Gets the hmd projection matrix for a paticular eye : 0 = left, 1 = right
void  DllExport hmdGetProjection(int eye, float matrix[4][4], bool invert = false);
float DllExport *hmdGetProjection(int eye, bool invert = false);

// Gets the hmd resolution / prefered buffer size / IPD
POINT DllExport hmdGetResolution();
POINT DllExport hmdGetBufferSize();
float DllExport hmdGetIPD();

// Returns the tracking data for the headset and controllers
void  DllExport hmdGetTrackingData(float hMatrix[4][4], float lcMatrix[4][4], float rcMatrix[4][4]);
float DllExport *hmdGetTrackingHMD();
float DllExport *hmdGetTrackingLeftController();
float DllExport *hmdGetTrackingRightController();

// Resets the saved controller button information (is down frame, is up frame)
void  DllExport hmdResetTracking();

// Gets the button information
bool  DllExport hmdGetButtonHasChanged(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);
bool  DllExport hmdGetButtonIsTouched(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);
bool  DllExport hmdGetButtonIsPressed(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);
bool  DllExport hmdGetButtonIsDownFrame(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);
bool  DllExport hmdGetButtonIsUpFrame(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);
float DllExport hmdGetButtonValue(ButtonsList::_ buttonID, ControllerTypes::_ controllerType);

// Recenters the headset
void  DllExport hmdRecenter();

// Sets a prediction value for the headset
void  DllExport hmdSetPrediction(float prediction);

// Starts / stops the controller vibration : controllerID 0 = left, 1 = right, vibratePercent = 0 -> 100
void  DllExport hmdControllerVibrateStart(int controllerID, int vibratePercent);
void  DllExport hmdControllerVibrateStop(int controllerID);

// Sets the texture or surface that is being render too either for both eyes at once, or seporate
void DllExport hmdSetTexture(IDirect3DTexture9 *bothEyes);
void DllExport hmdSetSurface(IDirect3DSurface9 *bothEyes);
void DllExport hmdSetTexture(IDirect3DTexture9 *leftEye, IDirect3DTexture9 *rightEye);
void DllExport hmdSetSurface(IDirect3DSurface9 *leftEye, IDirect3DSurface9 *rightEye);

// Saves the render surface to a texture, if rect is null or not included then it captures the whole surface
void DllExport hmdCaptureScreen(char *fileName);
void DllExport hmdCaptureScreen(char *fileName, RECT *rect);
void DllExport hmdCaptureScreen(char *fileName, int left, int top, int right, int bottom);
*/

//----
// Usage
//----
//---- start program
//---- need to be called in this order
// hmdSetSDK(sdkID);
// hmdOpen(useThreading); 
// hmdSetDevice(dxDevice);
//
//----
//
// hmdSetIsSitting(isSitting);
// projMatL = hmdGetProjection(0);
// projMatR = hmdGetProjection(1);
// 
//---- perFrame
//
// hmdBeginFrame(); (only if threading = false)
// 
// - render and present here
// - backbuffer is copied to headset during present function
//
// hmdSubmitFrame();
// hmdEndFrame(); (only if threading = false)
//
//----
// hmdClose();
//----
//*/

#endif	// VR_ALLOWED
#endif	// PROXYDLL