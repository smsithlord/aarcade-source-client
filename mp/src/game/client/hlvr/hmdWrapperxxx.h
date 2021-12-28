#pragma once
#include "stCommon.h"
#include <string>

#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)

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

//----
// Usage
//----
//---- start program
// hmdSetSDK(sdkID);
// hmdSetDevice(dxDevice);
// hmdOpen(useThreading); // hmdSetSDK & hmdSetDevice need to be called before hmdOpen
//
//----
//
// hmdSetIsSitting(isSitting);
// projMatL = hmdGetProjection(0);
// projMatR = hmdGetProjection(1);
// 
//---- perFrame
//
// hmdBeginFrame(useThreading);
// 
// - render and present here
// - backbuffer is copied to headset during present function
//
// hmdSubmitFrame();
// hmdEndFrame(useThreading);
//
//----
// hmdClose();
//----