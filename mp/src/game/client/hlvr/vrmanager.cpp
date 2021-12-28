#include "cbase.h"
#include "cdll_client_int.h"
#include "mathlib/vmatrix.h"
#include "sourcevr/isourcevirtualreality.h"
#include "weapon_selection.h"
#include "baseplayer_shared.h"
#include "c_basehlplayer.h"
#include "materialsystem/itexture.h"
#include "materialsystem/materialsystem_config.h"

#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/Controls.h"
#include "ienginevgui.h"
#include "vgui/IVGui.h"
#include "input.h"
#include <vgui/IInput.h>
#include "clientmode_shared.h"

#include "usermessages.h"

#include "../aarcade/client/aa_globals.h"

#include "proxydll.h"
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <d3d9.h>

extern ConVar hlvr_worldscale;
#define METRES_TO_GAME_UNITS hlvr_worldscale.GetFloat()

ConVar hlvr_sdk("hlvr_sdk", "1", FCVAR_ARCHIVE, "VR SDK to use. 0 = NULL, 1 = SteamVR, 2 = Oculus");
ConVar hlvr_primaryhand("hlvr_primaryhand", "1", FCVAR_ARCHIVE, "Which is the dominant hand?", true, 0, true, 1);
ConVar hlvr_oculustouch("hlvr_oculustouch", "0", FCVAR_ARCHIVE, "Enable oculus touch trigger and grip tweaks?", true, 0, true, 1);
ConVar hlvr_height_crouch("hlvr_height_crouch", "1.2", FCVAR_ARCHIVE, "The height in metres that the player's head must be below to be considered crouching");

ConVar hlvr_control_alwaysforward("hlvr_control_alwaysforward", "0", FCVAR_ARCHIVE, "Moving is always forward, regardless of trackpad position", true, 0, true, 1);
ConVar hlvr_control_alwaysfast("hlvr_control_alwaysfast", "0", FCVAR_ARCHIVE, "Moving is always full speed, regardless of trackpad position", true, 0, true, 1);
ConVar hlvr_control_turnamount("hlvr_control_turnamount", "90", FCVAR_ARCHIVE, "Turn amount in degrees. 0 = Smooth turning", true, 0, true, 360);
ConVar hlvr_control_vehiclesensitivity("hlvr_control_vehiclesensitivity", "1.5", FCVAR_ARCHIVE, "Vehicle steering sensitivity multiplier", true, 0, true, 5);
ConVar hlvr_control_teleport("hlvr_control_teleport", "0", FCVAR_ARCHIVE, "Teleport instead of smooth motion?", true, 0, true, 1);

ConVar hlvr_hand_pitch("hlvr_hand_pitch", "0", FCVAR_NONE, "The pitch offset of the hands/controllers.", true, -180, true, 180);
ConVar hlvr_cursor_ignorechecks("hlvr_cursor_ignorechecks", "0", FCVAR_ARCHIVE, "Ignore cursor checks? (Cursor may interfere with other applications when 1)", true, 0, true, 1);

//CVars that we shouldn't use
ConVar hlvr_pred("hlvr_pred", "0", FCVAR_ARCHIVE, "Prediction in seconds");
ConVar hlvr_ipdscale("hlvr_ipdscale", "1", FCVAR_ARCHIVE, "IPD scale. If you have to use this, there are massive problems.");


bool hlvr_activated = false;
bool sbs_activated = false;

C_BaseHLPlayer *gPlayer;

float hMatrix[16];
float oldHeight;

bool roomscale;

float prediction;

bool isInVehicle;
bool wasInVehicle;
float vehicleOffset;
float vsaveHeadYaw;
float joyOffset;

int frameCount = 0;

Vector initialzero;

struct TrackedObject {
	float matrix[16];
	float trigger;
	bool triggerDown;
	float grip;
	bool gripDown;
	bool buttonMenu;
	bool buttonExtra;
	float axisX;
	float axisY;
	bool axisDown;
	bool menuDown;
} lefthand, righthand, lastrighthand, lastlefthand;

bool HLVirtualReality::DllLoaded() {
	return true;
}

bool HLVirtualReality::ShouldRunInVR() {
	return hlvr_activated;
}

bool HLVirtualReality::ShouldRunInSBS() {
	return sbs_activated;
}

float moveJoy[2];

// user message handler func for suit power so it's available to the client, no idea why this wasn't already there....
void __MsgFunc_CLocalPlayer_Battery(bf_read &msg)
{
	if (!gPlayer)
		gPlayer = dynamic_cast<C_BaseHLPlayer*>(C_BasePlayer::GetLocalPlayer());
	if (gPlayer)
	{
		//gPlayer->SetSuitArmor(Clamp(msg.ReadShort(), 0, 100));
	}
}


void HLVirtualReality::LevelInitPostEntity() {
	roomscale = false;
	//engine->ClientCmd_Unrestricted("hlvr_roomscale_off\n");
	//vgui::surface()->SurfaceSetCursorPos((int)(((float)ScreenWidth()) / 2.f), (int)(((float)ScreenHeight()) / 2.f));
}

void HLVirtualReality::LevelShutdownPreEntity() {
	gPlayer = NULL;
	//engine->ClientCmd_Unrestricted("hlvr_resetrotation\n");
}

void HLVirtualReality::Update(float frametime){
#ifdef SBSOKAY
	if (!gPlayer)
		gPlayer = dynamic_cast<C_BaseHLPlayer*>(C_BasePlayer::GetLocalPlayer());
#endif

#ifdef VROKAY
	if (!UseVR())
		return;

	hmdBeginFrame();
	UpdatePoses();
	//if (!gPlayer)	// Added for Anarchy Arcade
	//	gPlayer = dynamic_cast<C_BaseHLPlayer*>(C_BasePlayer::GetLocalPlayer());
	if (prediction != hlvr_pred.GetFloat()){
		hmdSetPrediction(hlvr_pred.GetFloat());
		prediction = hlvr_pred.GetFloat();
	}
	//if (++frameCount % 60 == 0)
	//Warning("Well... frametime is : %f\n", 1.0 / frametime);
#endif
}

void HLVirtualReality::LateUpdate(){
	//hmdSubmitFrame();
}

void HLVirtualReality::Shutdown(){
	//g_pHLVR->Deactivate();
}


bool HLVirtualReality::Activate() {
#ifdef VROKAY
	sdk = hlvr_sdk.GetInt();
	if (!hmdOpen(false, sdk)) {
		Msg("Can not initalize API.");
		hlvr_activated = false;
		return false;
	}
	if (!hlvr_oculustouch.GetBool())
		engine->ClientCmd_Unrestricted("exec vive\n");
	else
		engine->ClientCmd_Unrestricted("exec rift\n");
	hlvr_activated = true;
	usermessages->HookMessage("Battery", __MsgFunc_CLocalPlayer_Battery);
	Msg("SteamVR SDK initialized\n");
	return true;
#else
	//DevMsg("Activate VR!\n");
	#ifdef SBSOKAY
		sbs_activated = true;
		return false;
	#else
		return false;
	#endif
#endif
}

void HLVirtualReality::Deactivate() {
#ifdef VROKAY
	hlvr_activated = false;
	hmdClose();
#endif

#ifdef SBSOKAY
	sbs_activated = false;
#endif
}

void HLVirtualReality::GetViewportBounds(ISourceVirtualReality::VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight) {
	if (pnX != NULL){
		*pnX = 1920 * (eEye)* 0.5f;// (int)hmdGetBufferSize().x * (eEye)* 0.5f;
		*pnY = 0;
	}
	*pnWidth = 1920 * 0.5f;// (int)hmdGetBufferSize().x * 0.5f;
	*pnHeight = 1080;// (int)hmdGetBufferSize().y;
	return;
}

ITexture *HLVirtualReality::GetRenderTarget(ISourceVirtualReality::VREye eEye, ISourceVirtualReality::EWhichRenderTarget eWhich)
{
	if (eEye > 0 && eEye < 3)
	{
		if (eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Color || (eWhich == ISourceVirtualReality::EWhichRenderTarget::RT_Depth))
		{
			return materials->FindTexture("_rt_two_eyes_VR", TEXTURE_GROUP_RENDER_TARGET);
		}
	}
	return NULL;
}

VMatrix HLVirtualReality::GetMideyePose() {
	if (!ShouldRunInVR())
		return VMatrix();
	VMatrix m_new;
	m_new = SMMatrixToVMatrix(hMatrix);
	return m_new;
}

VMatrix HLVirtualReality::GetMidEyeFromEye(ISourceVirtualReality::VREye eEye) {
#ifdef VROKAY
	return VMatrix(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, hmdGetIPD() * METRES_TO_GAME_UNITS * ((eEye == 0) ? 1.f : -1.f),
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f);
#else
	return VMatrix();
#endif
}

//This a complete bodge. I should probably find out how to properly generate a proj matrix from headset specs.
//In it's current state I think this makes zooming impossible, though beyond that, if it aint broke...
bool HLVirtualReality::GetEyeProjectionMatrix(VMatrix *pResult, ISourceVirtualReality::VREye eEye, float zNear, float zFar, float fovScale) {
#ifdef VROKAY
	float proMat[4][4];
	hmdGetProjection(eEye, proMat);
	*pResult = *(VMatrix*)&proMat;
	return true;
#else
	return false;
#endif
}

//Note that this wil be a matrix from the pose to the default mideye zero
VMatrix HLVirtualReality::SMMatrixToVMatrix(float matrix[16], bool isHand, bool isLeftSpace) {
	VMatrix returnValue;
	VMatrix initialValue = VMatrix(
		matrix[0], matrix[4], matrix[8], matrix[12],
		matrix[1], matrix[5], matrix[9], matrix[13],
		matrix[2], matrix[6], matrix[10], matrix[14],
		matrix[3], matrix[7], matrix[11], matrix[15]);

	Vector pos = initialValue.GetTranslation();
	Quaternion rot;

	rot.w = sqrt(1.f + matrix[0] + matrix[5] + matrix[10]) / 2.0;
	double w4 = (4.0 * rot.w);
	rot.x = (matrix[6] - matrix[9]) / w4;
	rot.y = (matrix[2] - matrix[8]) / w4;
	rot.z = (matrix[1] - matrix[4]) / -w4;

	QuaternionNormalize(rot);

	Vector naxis;
	float angle;

	naxis = Vector(rot.x, rot.y, rot.z).Normalized();
	angle = 2 * acosf(rot.w);

	if (angle > (3.1415f)) // Reduce the magnitude of the angle, if necessary
	{
		angle = (3.1415f * 2.f) - angle;
		naxis = naxis * (-1);
	}

	float c = cos(angle);
	float s = sin(angle);
	float t = 1.f - c;

	Vector axis;
	axis.x = naxis.z;
	axis.y = -naxis.x;
	axis.z = -naxis.y;

	returnValue = VMatrix(t*axis.x*axis.x + c, t*axis.x*axis.y - axis.z*s, t*axis.x*axis.z + axis.y*s, pos.z * -METRES_TO_GAME_UNITS,
		t*axis.x*axis.y + axis.z*s, t*axis.y*axis.y + c, t*axis.y*axis.z - axis.x*s, pos.x * -METRES_TO_GAME_UNITS,
		t*axis.x*axis.z - axis.y*s, t*axis.y*axis.z + axis.x*s, t*axis.z*axis.z + c, pos.y * METRES_TO_GAME_UNITS - 64.f,
		0.f, 0.f, 0.f, 1.f);

	/*
	VMatrix inver, delta;
	MatrixInverseTR(initialValue, inver);
	delta = returnValue * inver;
	delta.GetTranslation();
	//Do something with this ^, maybe?
	*/

	if (isHand) {	//Skew the rotation if it's a weapon
		matrix3x4_t rot;
		SetIdentityMatrix(rot);
		if (!isLeftSpace) {
			MatrixSetTranslation(Vector(handOffset.x, -handOffset.y, handOffset.z), rot);
		}
		else {
			MatrixSetTranslation(handOffset, rot);
		}
		returnValue = returnValue * rot;

		MatrixBuildRotationAboutAxis(Vector(0.f, 1.f, 0.f), hlvr_hand_pitch.GetFloat(), rot);
		returnValue = returnValue * rot;
	}

	if (gPlayer && !roomscale) {
		engine->ClientCmd_Unrestricted("hlvr_resetposition\n");
		if (false){//gPlayer->m_Local.m_vecHeadApplied != Vector(0.f, 0.f, 0.f) && gPlayer->m_Local.m_vecHeadApplied.x == gPlayer->m_eyeOffset.x && gPlayer->m_Local.m_vecHeadApplied.y == gPlayer->m_eyeOffset.y){
			roomscale = true;
			engine->ClientCmd_Unrestricted("hlvr_roomscale_on\n");
		}
	}

	VMatrix addition;
	MatrixBuildRotateZ(addition, rotationOffset + joyOffset + vehicleOffset);							//Game yaw offset
	//if (gPlayer && roomscale) addition.SetTranslation(-gPlayer->m_Local.m_vecHeadApplied);				//Recenter in game, with roomscale.
	returnValue = addition * returnValue;

	return returnValue;
}

void HLVirtualReality::OverrideWeaponMatrix(VMatrix& weaponMatrix)
{
	if (!ShouldRunInVR())
		return;
	QAngle weaponAngle;
	MatrixToAngles(weaponMatrix, weaponAngle);
	Vector weaponOrigin = weaponMatrix.GetTranslation();

	// for now let's just reuse the overrideviewmodel stuff that does exactly the same thing (from mideye view to weapon position & orientation..)
	QAngle dAngle;
	Vector dOrigin;
	UpdateViewmodelOffset(weaponOrigin, weaponAngle);

	MatrixFromAngles(weaponAngle, weaponMatrix);
	weaponMatrix.SetTranslation(weaponOrigin);
}

//THIS NEEDS TO BE SCRAPPED
void HLVirtualReality::UpdateViewmodelOffset(Vector& vmorigin, QAngle& vmangles)
{
	if (!ShouldRunInVR())
		return;

	VMatrix handMatrix;
	handMatrix = SMMatrixToVMatrix(righthand.matrix, true);
	vmorigin += handMatrix.GetTranslation();
	vmangles = GetRightHandRotation();

}

void HLVirtualReality::GetEyeToRightHandOffset(Vector& offset)
{
	if (!ShouldRunInVR())
		return;
	VMatrix weaponMatrix;
	weaponMatrix = SMMatrixToVMatrix(righthand.matrix, true);
	MatrixPosition(weaponMatrix.As3x4(), offset);
}

QAngle HLVirtualReality::GetRightHandRotation()
{
	if (!ShouldRunInVR())
		return QAngle();
	QAngle rotation;
	VMatrix weaponMatrix;
	weaponMatrix = SMMatrixToVMatrix(righthand.matrix, true);
	MatrixAngles(weaponMatrix.As3x4(), rotation);
	return rotation;
}

void HLVirtualReality::GetEyeToLeftHandOffset(Vector& offset)
{
	if (!ShouldRunInVR())
		return;
	VMatrix weaponMatrix;
	weaponMatrix = SMMatrixToVMatrix(lefthand.matrix, true, true);
	MatrixPosition(weaponMatrix.As3x4(), offset);
}

QAngle HLVirtualReality::GetLeftHandRotation()
{
	if (!ShouldRunInVR())
		return QAngle();
	VMatrix weaponMatrix;
	QAngle rotation;
	weaponMatrix = SMMatrixToVMatrix(lefthand.matrix, true, true);
	MatrixAngles(weaponMatrix.As3x4(), rotation);
	return rotation;
}

//I'm missing something with the math here... 
void HLVirtualReality::ProcessVehicleYawOffset(float toyaw){
	if (!wasInVehicle) {
		QAngle head;
		MatrixAngles(GetMideyePose().As3x4(), head);
		vsaveHeadYaw = AngleDiff(head.y, -(rotationOffset + joyOffset));
	}
	vehicleOffset = AngleNormalize(AngleDiff(toyaw, vsaveHeadYaw) + 180.f);
	wasInVehicle = true;
}

void HLVirtualReality::UpdatePoses() {
#ifdef VROKAY
	if (!ShouldRunInVR())
		return;

	float hmdmatrix[4][4], lhmatrix[4][4], rhmatrix[4][4];

	hmdGetTrackingData(hmdmatrix, lhmatrix, rhmatrix);

	memcpy(hMatrix, hmdmatrix, sizeof(hMatrix));
	memcpy(&lefthand.matrix, lhmatrix, sizeof(lhmatrix));
	memcpy(&righthand.matrix, rhmatrix, sizeof(rhmatrix));

	lefthand.axisDown = GetButtonIsPressed(ButtonsList::left_Pad, ControllerTypes::controllerType_Virtual);
	lefthand.axisX = GetButtonValue(ButtonsList::left_PadXAxis, ControllerTypes::controllerType_Virtual);
	lefthand.axisY = GetButtonValue(ButtonsList::left_PadYAxis, ControllerTypes::controllerType_Virtual);
	lefthand.buttonExtra = GetButtonIsPressed(ButtonsList::left_ButtonA, ControllerTypes::controllerType_Virtual);
	lefthand.buttonMenu = GetButtonIsPressed(ButtonsList::left_ButtonB, ControllerTypes::controllerType_Virtual);
	lefthand.grip = GetButtonValue(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual);
	lefthand.gripDown = GetButtonIsPressed(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual);
	lefthand.trigger = GetButtonValue(ButtonsList::left_Trigger, ControllerTypes::controllerType_Virtual);
	lefthand.triggerDown = GetButtonIsPressed(ButtonsList::left_Trigger, ControllerTypes::controllerType_Virtual);
	lefthand.menuDown = GetButtonIsPressed(ButtonsList::left_Menu, ControllerTypes::controllerType_Virtual) || lefthand.buttonMenu;

	righthand.axisDown = GetButtonIsPressed(ButtonsList::right_Pad, ControllerTypes::controllerType_Virtual);
	righthand.axisX = GetButtonValue(ButtonsList::right_PadXAxis, ControllerTypes::controllerType_Virtual);
	righthand.axisY = GetButtonValue(ButtonsList::right_PadYAxis, ControllerTypes::controllerType_Virtual);
	righthand.buttonExtra = GetButtonIsPressed(ButtonsList::right_ButtonA, ControllerTypes::controllerType_Virtual);
	righthand.buttonMenu = GetButtonIsPressed(ButtonsList::right_ButtonB, ControllerTypes::controllerType_Virtual);
	righthand.grip = GetButtonValue(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual);
	righthand.gripDown = GetButtonIsPressed(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual);
	righthand.trigger = GetButtonValue(ButtonsList::right_Trigger, ControllerTypes::controllerType_Virtual);
	righthand.triggerDown = GetButtonIsPressed(ButtonsList::right_Trigger, ControllerTypes::controllerType_Virtual);
	righthand.menuDown = GetButtonIsPressed(ButtonsList::right_Menu, ControllerTypes::controllerType_Virtual) || righthand.buttonMenu;

	ProcessInput();

	return;
#endif
}


void HLVirtualReality::RecalibrateControllerSides()
{

}

void HLVirtualReality::OverrideJoystickInputs(float& jx, float& jy, float& lx, float& ly) {
	if (!ShouldRunInVR())
		return;

	if (!hlvr_control_teleport.GetBool()){

		if (hlvr_primaryhand.GetBool()) {
			jx = lefthand.axisX * 32767;
			jy = lefthand.axisY * -32767;
			lx = 0;
			ly = 0;
		}
		else {
			jx = righthand.axisX * 32767;
			jy = righthand.axisY * -32767;
			lx = 0;
			ly = 0;
		}
	}
	else {
		bool touching = false;
		if (hlvr_primaryhand.GetBool())
			touching = (lefthand.axisX > 0.2f || lefthand.axisX < -0.2f || lefthand.axisY > 0.2f || lefthand.axisY < -0.2f);
		else
			touching = (righthand.axisX > 0.2f || righthand.axisX < -0.2f || righthand.axisY > 0.2f || righthand.axisY < -0.2f);

		if (touching && !isTeleportArcOn){
			engine->ServerCmd("int_teleport 1\n");
			isTeleportArcOn = true;
		}
		else if (!touching && isTeleportArcOn){
			engine->ServerCmd("int_teleport 0\n");
			isTeleportArcOn = false;
		}
	}
	float mag = 0.f;
	if (hlvr_control_alwaysfast.GetBool() || hlvr_control_alwaysforward.GetBool())
		mag = sqrt(jx * jx + jy * jy);

	if (hlvr_control_alwaysfast.GetBool() && mag > 200){
		jx = jx * (32767 / mag);
		jy = jy * (32767 / mag);
	}

	if (hlvr_control_alwaysforward.GetBool()){
		jy = -mag;
		jx = 0;
	}

	if (gPlayer) isInVehicle = gPlayer->GetVehicle();

	if (isInVehicle){
		/*
		Vector rhv = SMMatrixToVMatrix(righthand.matrix).GetTranslation();
		Vector lhv = SMMatrixToVMatrix(lefthand.matrix).GetTranslation();
		float axisdec = atan((rhv.z - lhv.z) / sqrt(Sqr(lhv.x - rhv.x) + Sqr(lhv.y - rhv.y))) / -M_PIONTWO;
		jx = axisdec * 32767 * hlvr_control_vehiclesensitivity.GetFloat();
		jy = (righthand.trigger - lefthand.trigger) * -32767;
		*/
	}
	else if (wasInVehicle){
		wasInVehicle = false;
		rotationOffset += vehicleOffset;
		vehicleOffset = 0.f;
	}

	if (hlvr_control_turnamount.GetFloat() == 0 && ((hlvr_primaryhand.GetBool() ? righthand : lefthand).axisDown || hlvr_oculustouch.GetBool()) && abs((hlvr_primaryhand.GetBool() ? righthand : lefthand).axisX) > 0.4f)
		lx = 32767 * ((hlvr_primaryhand.GetBool() ? righthand : lefthand).axisX > 0.f ? 1.f : -1.f);

}

void HLVirtualReality::ProcessInput(){

	if (hlvr_oculustouch.GetBool()){	//Rift changes
		if (lefthand.grip > 0.8f)
			lefthand.gripDown = true;
		if (righthand.grip > 0.8f)
			righthand.gripDown = true;
		if (lefthand.trigger > 0.8f)
			lefthand.triggerDown = true;
		if (righthand.trigger > 0.8f)
			righthand.triggerDown = true;
	}

	/*
	Msg("trigger: %f, grip: %f, padx: %f, pady: %f, button3: %s, button2: %s, button1: %s, button0: %s\n",
	lefthand.trigger, lefthand.grip, lefthand.axisX, lefthand.axisY,
	lefthand.axisDown ? "true" : "false", lefthand.buttonSpecial ? "true" : "false", lefthand.button1 ? "true" : "false", lefthand.button0 ? "true" : "false");
	*/

	TrackedObject* hand = hlvr_primaryhand.GetBool() ? &righthand : &lefthand;
	TrackedObject* lasthand = hlvr_primaryhand.GetBool() ? &lastrighthand : &lastlefthand;

	if (lefthand.triggerDown && !lastlefthand.triggerDown && !wasInVehicle)
		engine->ClientCmd_Unrestricted("+left_grab_attack\n");
	if (!lefthand.triggerDown && lastlefthand.triggerDown)
		engine->ClientCmd_Unrestricted("-left_grab_attack\n");

	if (righthand.triggerDown && !lastrighthand.triggerDown)
		engine->ClientCmd_Unrestricted("+right_grab_attack\n");
	if (!righthand.triggerDown && lastrighthand.triggerDown)
		engine->ClientCmd_Unrestricted("-right_grab_attack\n");

	if (((!righthand.menuDown && lastrighthand.menuDown) || (!lefthand.menuDown && lastlefthand.menuDown)) && !engine->IsLevelMainMenuBackground()){
		isPauseMenuUp = !isPauseMenuUp;
		isPauseMenuUp ? engine->ClientCmd_Unrestricted("hlvr_pausemenu 1\n") : engine->ClientCmd_Unrestricted("hlvr_pausemenu 0\n");
	}

	if (!vgui::surface()->IsCursorVisible()) {

		if (lefthand.gripDown && !lastlefthand.gripDown)
			engine->ClientCmd_Unrestricted("+left_use\n");
		if (!lefthand.gripDown && lastlefthand.gripDown)
			engine->ClientCmd_Unrestricted("-left_use\n");

		if (righthand.gripDown && !lastrighthand.gripDown)
			engine->ClientCmd_Unrestricted("+right_use\n");
		if (!righthand.gripDown && lastrighthand.gripDown)
			engine->ClientCmd_Unrestricted("-right_use\n");

		// Commented out to use new Left/Right Grab/Attack Use buttons
		// 
		if (hand->triggerDown && !lasthand->triggerDown && !isInVehicle && !isPauseMenuUp)
			engine->ClientCmd_Unrestricted("+attack\n");
		if ((!hand->triggerDown) && lasthand->triggerDown)
			engine->ClientCmd_Unrestricted("-attack\n");
		//if (hand.gripDown && !lasthand.gripDown)
		//	engine->ClientCmd_Unrestricted("+use\n");
		//if ((!hand.gripDown) && lasthand.gripDown)
		//	engine->ClientCmd_Unrestricted("-use\n");

		if (hand->axisY >= 0.3f && (hand->axisDown && !lasthand->axisDown))
			engine->ClientCmd_Unrestricted("invnext\n");
		if (hand->axisY <= -0.3f && ((hand->axisDown && !lasthand->axisDown) || (hlvr_oculustouch.GetBool() && lasthand->axisY > -0.3f)) && !isInVehicle)
			engine->ClientCmd_Unrestricted("+attack2\n");
		if ((!hand->axisDown && lasthand->axisDown) || (hlvr_oculustouch.GetBool() && lasthand->axisY <= -0.3f && hand->axisY > -0.3f))
			engine->ClientCmd_Unrestricted("-attack2\n");

		if (hlvr_control_turnamount.GetFloat() != 0.f) {
			if (hand->axisX < -0.3f && ((hand->axisDown && !lasthand->axisDown) || (hlvr_oculustouch.GetBool() && lasthand->axisX >= -0.3f)))
				joyOffset += hlvr_control_turnamount.GetFloat();
			if (hand->axisX > 0.3f && ((hand->axisDown && !lasthand->axisDown) || (hlvr_oculustouch.GetBool() && lasthand->axisX <= 0.3f)))
				joyOffset -= hlvr_control_turnamount.GetFloat();
		}

		hand = !hlvr_primaryhand.GetBool() ? &righthand : &lefthand;
		lasthand = !hlvr_primaryhand.GetBool() ? &lastrighthand : &lastlefthand;
		if (hand->axisDown && !lasthand->axisDown && !isInVehicle && !hlvr_control_teleport.GetBool())
			engine->ClientCmd_Unrestricted("+jump\n");
		if (hand->axisDown && !lasthand->axisDown && !isInVehicle && hlvr_control_teleport.GetBool())
			engine->ServerCmd("int_teleport 2\n");
		if (hand->axisDown && !lasthand->axisDown && isInVehicle)
			engine->ClientCmd_Unrestricted("+attack\n");
		if (!hand->axisDown && lasthand->axisDown) {
			engine->ClientCmd_Unrestricted("-jump\n");
			engine->ClientCmd_Unrestricted("-attack\n");
		}
		if (hand->triggerDown && !lasthand->triggerDown)
			engine->ClientCmd_Unrestricted("+reload\n");
		if ((!hand->triggerDown) && lasthand->triggerDown)
			engine->ClientCmd_Unrestricted("-reload\n");

		if (hand->buttonExtra && !lasthand->buttonExtra)
			engine->ClientCmd_Unrestricted("+reload\n");
		if ((!hand->buttonExtra) && lasthand->buttonExtra)
			engine->ClientCmd_Unrestricted("-reload\n");

		/*if (hand.gripDown && !lasthand.gripDown) {
		engine->ClientCmd_Unrestricted("impulse 100\n");
		}
		*/
		isTwoHandWeaponActive = false;//hand.gripDown;

		/*
		if (!hlvr_safety_crouching.GetBool())
		{
			if (hMatrix[13] <= hlvr_height_crouch.GetFloat() && oldHeight > hlvr_height_crouch.GetFloat())
				engine->ClientCmd_Unrestricted("toggle_duck\n");
			if (hMatrix[13] > hlvr_height_crouch.GetFloat() && oldHeight <= hlvr_height_crouch.GetFloat())
				engine->ClientCmd_Unrestricted("toggle_duck\n");
		}*/

	}

	//Surface / window space cursor
	/*
	if (vgui::surface()->IsCursorVisible()) {
		if ((hlvr_primaryhand.GetBool() ? righthand : lefthand).axisDown && !(hlvr_primaryhand.GetBool() ? lastrighthand : lastlefthand).axisDown){
			HWND myHwnd = FindWindow(null, "Half-Life 2: VR");
			HWND foregroundHwnd = GetForegroundWindow();
			int curx, cury;
			vgui::surface()->SurfaceGetCursorPos(curx, cury);
			if ((foregroundHwnd == myHwnd && curx > 0 && cury > 0 && curx < ScreenWidth() && cury < ScreenHeight()) || hlvr_cursor_ignorechecks.GetBool())
				hmdMouseDown(MouseButton::right);
		}
		if (!(hlvr_primaryhand.GetBool() ? righthand : lefthand).axisDown && (hlvr_primaryhand.GetBool() ? lastrighthand : lastlefthand).axisDown){
			hmdMouseUp(MouseButton::right);
		}

		if ((hlvr_primaryhand.GetBool() ? righthand : lefthand).triggerDown && !(hlvr_primaryhand.GetBool() ? lastrighthand : lastlefthand).triggerDown){
			HWND myHwnd = FindWindow(null, "Half-Life 2: VR");
			HWND foregroundHwnd = GetForegroundWindow();
			int curx, cury;
			vgui::surface()->SurfaceGetCursorPos(curx, cury);
			if ((foregroundHwnd == myHwnd && curx > 0 && cury > 0 && curx < ScreenWidth() && cury < ScreenHeight()) || hlvr_cursor_ignorechecks.GetBool())
				hmdMouseDown(MouseButton::left);
		}
		if (!(hlvr_primaryhand.GetBool() ? righthand : lefthand).triggerDown && (hlvr_primaryhand.GetBool() ? lastrighthand : lastlefthand).triggerDown){
			hmdMouseUp(MouseButton::left);
		}
	}
	*/
	//VGUI panel cursor
	menuPress = (righthand.triggerDown || lefthand.triggerDown);

	lastlefthand = lefthand;
	lastrighthand = righthand;
	oldHeight = hMatrix[13];
}

float HLVirtualReality::GetHudPanelAlpha(const Vector& hudPanelForward, const Vector& eyesForward, float fadePow)
{
	if (!ShouldRunInVR())
		return 1.f;
	float dot = hudPanelForward.Dot(eyesForward);
	if (dot > 0) return 0.f;
	return pow(fabs(dot), fadePow);
}

void HLVirtualReality::HandOffset(const CCommand &args)
{
	if (args.ArgC() < 3)
	{
		Msg("Current: hlvr_hand_offset %f %f %f\n", handOffset.x, handOffset.y, handOffset.z);
		Msg("Usage: hlvr_hand_offset <x> <y> <z>\n");
		return;
	}
	float x = 0.f, y = 0.f, z = 0.f;
	UTIL_StringToFloatArray(&x, 1, args.Arg(1));
	UTIL_StringToFloatArray(&y, 1, args.Arg(2));
	UTIL_StringToFloatArray(&z, 1, args.Arg(3));
	handOffset = Vector(x, y, z);
}


HLVirtualReality g_HLVR;
HLVirtualReality* g_pHLVR = &g_HLVR;

void HandOffsetCallback(const CCommand &args) { g_pHLVR->HandOffset(args); }
ConCommand hlvr_hand_offset("hlvr_hand_offset", HandOffsetCallback, "The vector offset for hands, based on right, mirrored for left. (X Y Z)", 0);



CON_COMMAND(hlvr_primaryhand_toggle, "Swap primary hand")
{
	if (hlvr_primaryhand.GetBool())
		hlvr_primaryhand.SetValue(0);
	else
		hlvr_primaryhand.SetValue(1);
}