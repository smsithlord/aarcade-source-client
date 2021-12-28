#pragma once
#include "stCommon.h"

#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)

//----
// Only use hmdBeginFrame / hmdEndFrame if hmdOpen useThreaded = false
//----
// Called at the start of the frame to setup head / controller tracking values for the frame
void  DllExport hmdBeginFrame();

// Called at the end of the frame to tell the headset to render the frame
void  DllExport hmdEndFrame();

//----
// Global functions for external use
//----

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
void  DllExport hmdGetProjection(int eye, float matrix[4][4]);
float DllExport *hmdGetProjection(int eye);

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


//----
// Other functions
//----
void DllExport hmdMouseDown(MouseButton button);
void DllExport hmdMouseUp(MouseButton button);
void DllExport hmdEscDown();
void DllExport hmdEscUp();


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