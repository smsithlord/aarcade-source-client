#include "cbase.h"

#include "c_inputmanager.h"
#include "c_anarchymanager.h"
#include "ienginevgui.h"
#include "c_inputslate.h"
#include "vgui/IInput.h"
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_InputManager::C_InputManager()
{
	DevMsg("InputManager: Constructor\n");
	m_bDidLeftVRSpazzFixLast = false;
	m_flLastRepeat = 0;
	m_flStartRepeat = 0;
	m_bIsGamepadInputMode = false;
	m_pInputSlate = null;
	m_bTempSelect = false;
	m_bInputMode = false;
	m_bForcedInputMode = false;
	m_bWasForcedInputMode = false;
	m_bInputCapture = true;
	m_bFullscreenMode = false;
	//m_bOverlayMode = false;
	m_bMainMenuMode = false;
	//m_pInputListener = null;
	//m_pInputCanvasTexture = null;
	m_pReShadeConVar = null;
	m_pReShadeDepthConVar = null;
	m_pDrawVGUIConVar = null;
	m_pEmbeddedInstance = null;
	m_pHeldKeys = new KeyValues("keys");

	m_sourceKeyEnumMap["KEY_0"] = KEY_0;
	m_sourceKeyEnumMap["KEY_1"] = KEY_1;
	m_sourceKeyEnumMap["KEY_2"] = KEY_2;
	m_sourceKeyEnumMap["KEY_3"] = KEY_3;
	m_sourceKeyEnumMap["KEY_4"] = KEY_4;
	m_sourceKeyEnumMap["KEY_5"] = KEY_5;
	m_sourceKeyEnumMap["KEY_6"] = KEY_6;
	m_sourceKeyEnumMap["KEY_7"] = KEY_7;
	m_sourceKeyEnumMap["KEY_8"] = KEY_8;
	m_sourceKeyEnumMap["KEY_9"] = KEY_9;
	m_sourceKeyEnumMap["KEY_A"] = KEY_A;
	m_sourceKeyEnumMap["KEY_B"] = KEY_B;
	m_sourceKeyEnumMap["KEY_C"] = KEY_C;
	m_sourceKeyEnumMap["KEY_D"] = KEY_D;
	m_sourceKeyEnumMap["KEY_E"] = KEY_E;
	m_sourceKeyEnumMap["KEY_F"] = KEY_F;
	m_sourceKeyEnumMap["KEY_G"] = KEY_G;
	m_sourceKeyEnumMap["KEY_H"] = KEY_H;
	m_sourceKeyEnumMap["KEY_I"] = KEY_I;
	m_sourceKeyEnumMap["KEY_J"] = KEY_J;
	m_sourceKeyEnumMap["KEY_K"] = KEY_K;
	m_sourceKeyEnumMap["KEY_L"] = KEY_L;
	m_sourceKeyEnumMap["KEY_M"] = KEY_M;
	m_sourceKeyEnumMap["KEY_N"] = KEY_N;
	m_sourceKeyEnumMap["KEY_O"] = KEY_O;
	m_sourceKeyEnumMap["KEY_P"] = KEY_P;
	m_sourceKeyEnumMap["KEY_Q"] = KEY_Q;
	m_sourceKeyEnumMap["KEY_R"] = KEY_R;
	m_sourceKeyEnumMap["KEY_S"] = KEY_S;
	m_sourceKeyEnumMap["KEY_T"] = KEY_T;
	m_sourceKeyEnumMap["KEY_U"] = KEY_U;
	m_sourceKeyEnumMap["KEY_V"] = KEY_V;
	m_sourceKeyEnumMap["KEY_W"] = KEY_W;
	m_sourceKeyEnumMap["KEY_X"] = KEY_X;
	m_sourceKeyEnumMap["KEY_Y"] = KEY_Y;
	m_sourceKeyEnumMap["KEY_Z"] = KEY_Z;
	m_sourceKeyEnumMap["KEY_PAD_0"] = KEY_PAD_0;
	m_sourceKeyEnumMap["KEY_PAD_1"] = KEY_PAD_1;
	m_sourceKeyEnumMap["KEY_PAD_2"] = KEY_PAD_2;
	m_sourceKeyEnumMap["KEY_PAD_3"] = KEY_PAD_3;
	m_sourceKeyEnumMap["KEY_PAD_4"] = KEY_PAD_4;
	m_sourceKeyEnumMap["KEY_PAD_5"] = KEY_PAD_5;
	m_sourceKeyEnumMap["KEY_PAD_6"] = KEY_PAD_6;
	m_sourceKeyEnumMap["KEY_PAD_7"] = KEY_PAD_7;
	m_sourceKeyEnumMap["KEY_PAD_8"] = KEY_PAD_8;
	m_sourceKeyEnumMap["KEY_PAD_9"] = KEY_PAD_9;
	m_sourceKeyEnumMap["KEY_PAD_DIVIDE"] = KEY_PAD_DIVIDE;
	m_sourceKeyEnumMap["KEY_PAD_MULTIPLY"] = KEY_PAD_MULTIPLY;
	m_sourceKeyEnumMap["KEY_PAD_MINUS"] = KEY_PAD_MINUS;
	m_sourceKeyEnumMap["KEY_PAD_PLUS"] = KEY_PAD_PLUS;
	m_sourceKeyEnumMap["KEY_PAD_ENTER"] = KEY_PAD_ENTER;
	m_sourceKeyEnumMap["KEY_PAD_DECIMAL"] = KEY_PAD_DECIMAL;
	m_sourceKeyEnumMap["KEY_LBRACKET"] = KEY_LBRACKET;
	m_sourceKeyEnumMap["KEY_RBRACKET"] = KEY_RBRACKET;
	m_sourceKeyEnumMap["KEY_SEMICOLON"] = KEY_SEMICOLON;
	m_sourceKeyEnumMap["KEY_APOSTROPHE"] = KEY_APOSTROPHE;
	m_sourceKeyEnumMap["KEY_BACKQUOTE"] = KEY_BACKQUOTE;
	m_sourceKeyEnumMap["KEY_COMMA"] = KEY_COMMA;
	m_sourceKeyEnumMap["KEY_PERIOD"] = KEY_PERIOD;
	m_sourceKeyEnumMap["KEY_SLASH"] = KEY_SLASH;
	m_sourceKeyEnumMap["KEY_BACKSLASH"] = KEY_BACKSLASH;
	m_sourceKeyEnumMap["KEY_MINUS"] = KEY_MINUS;
	m_sourceKeyEnumMap["KEY_EQUAL"] = KEY_EQUAL;
	m_sourceKeyEnumMap["KEY_ENTER"] = KEY_ENTER;
	m_sourceKeyEnumMap["KEY_SPACE"] = KEY_SPACE;
	m_sourceKeyEnumMap["KEY_BACKSPACE"] = KEY_BACKSPACE;
	m_sourceKeyEnumMap["KEY_TAB"] = KEY_TAB;
	m_sourceKeyEnumMap["KEY_CAPSLOCK"] = KEY_CAPSLOCK;
	m_sourceKeyEnumMap["KEY_NUMLOCK"] = KEY_NUMLOCK;
	m_sourceKeyEnumMap["KEY_ESCAPE"] = KEY_ESCAPE;
	m_sourceKeyEnumMap["KEY_SCROLLLOCK"] = KEY_SCROLLLOCK;
	m_sourceKeyEnumMap["KEY_INSERT"] = KEY_INSERT;
	m_sourceKeyEnumMap["KEY_DELETE"] = KEY_DELETE;
	m_sourceKeyEnumMap["KEY_HOME"] = KEY_HOME;
	m_sourceKeyEnumMap["KEY_END"] = KEY_END;
	m_sourceKeyEnumMap["KEY_PAGEUP"] = KEY_PAGEUP;
	m_sourceKeyEnumMap["KEY_PAGEDOWN"] = KEY_PAGEDOWN;
	m_sourceKeyEnumMap["KEY_BREAK"] = KEY_BREAK;
	m_sourceKeyEnumMap["KEY_LSHIFT"] = KEY_LSHIFT;
	m_sourceKeyEnumMap["KEY_RSHIFT"] = KEY_RSHIFT;
	m_sourceKeyEnumMap["KEY_LALT"] = KEY_LALT;
	m_sourceKeyEnumMap["KEY_RALT"] = KEY_RALT;
	m_sourceKeyEnumMap["KEY_LCONTROL"] = KEY_LCONTROL;
	m_sourceKeyEnumMap["KEY_RCONTROL"] = KEY_RCONTROL;
	m_sourceKeyEnumMap["KEY_LWIN"] = KEY_LWIN;
	m_sourceKeyEnumMap["KEY_RWIN"] = KEY_RWIN;
	m_sourceKeyEnumMap["KEY_APP"] = KEY_APP;
	m_sourceKeyEnumMap["KEY_UP"] = KEY_UP;
	m_sourceKeyEnumMap["KEY_LEFT"] = KEY_LEFT;
	m_sourceKeyEnumMap["KEY_DOWN"] = KEY_DOWN;
	m_sourceKeyEnumMap["KEY_RIGHT"] = KEY_RIGHT;
	m_sourceKeyEnumMap["KEY_F1"] = KEY_F1;
	m_sourceKeyEnumMap["KEY_F2"] = KEY_F2;
	m_sourceKeyEnumMap["KEY_F3"] = KEY_F3;
	m_sourceKeyEnumMap["KEY_F4"] = KEY_F4;
	m_sourceKeyEnumMap["KEY_F5"] = KEY_F5;
	m_sourceKeyEnumMap["KEY_F6"] = KEY_F6;
	m_sourceKeyEnumMap["KEY_F7"] = KEY_F7;
	m_sourceKeyEnumMap["KEY_F8"] = KEY_F8;
	m_sourceKeyEnumMap["KEY_F9"] = KEY_F9;
	m_sourceKeyEnumMap["KEY_F10"] = KEY_F10;
	m_sourceKeyEnumMap["KEY_F11"] = KEY_F11;
	m_sourceKeyEnumMap["KEY_F12"] = KEY_F12;
	m_sourceKeyEnumMap["KEY_CAPSLOCKTOGGLE"] = KEY_CAPSLOCKTOGGLE;
	m_sourceKeyEnumMap["KEY_NUMLOCKTOGGLE"] = KEY_NUMLOCKTOGGLE;
	m_sourceKeyEnumMap["KEY_SCROLLLOCKTOGGLE"] = KEY_SCROLLLOCKTOGGLE;
	m_sourceKeyEnumMap["MOUSE_LEFT"] = MOUSE_LEFT;
	m_sourceKeyEnumMap["MOUSE_RIGHT"] = MOUSE_RIGHT;
	m_sourceKeyEnumMap["MOUSE_MIDDLE"] = MOUSE_MIDDLE;
	m_sourceKeyEnumMap["MOUSE_4"] = MOUSE_4;
	m_sourceKeyEnumMap["MOUSE_5"] = MOUSE_5;
	m_sourceKeyEnumMap["MOUSE_WHEEL_UP"] = MOUSE_WHEEL_UP;
	m_sourceKeyEnumMap["MOUSE_WHEEL_DOWN"] = MOUSE_WHEEL_DOWN;
	m_sourceKeyEnumMap["KEY_XBUTTON_UP"] = KEY_XBUTTON_UP;
	m_sourceKeyEnumMap["KEY_XBUTTON_RIGHT"] = KEY_XBUTTON_RIGHT;
	m_sourceKeyEnumMap["KEY_XBUTTON_DOWN"] = KEY_XBUTTON_DOWN;
	m_sourceKeyEnumMap["KEY_XBUTTON_LEFT"] = KEY_XBUTTON_LEFT;
	m_sourceKeyEnumMap["KEY_XBUTTON_A"] = KEY_XBUTTON_A;
	m_sourceKeyEnumMap["KEY_XBUTTON_B"] = KEY_XBUTTON_B;
	m_sourceKeyEnumMap["KEY_XBUTTON_X"] = KEY_XBUTTON_X;
	m_sourceKeyEnumMap["KEY_XBUTTON_Y"] = KEY_XBUTTON_Y;
	m_sourceKeyEnumMap["KEY_XBUTTON_LEFT_SHOULDER"] = KEY_XBUTTON_LEFT_SHOULDER;
	m_sourceKeyEnumMap["KEY_XBUTTON_RIGHT_SHOULDER"] = KEY_XBUTTON_RIGHT_SHOULDER;
	m_sourceKeyEnumMap["KEY_XBUTTON_BACK"] = KEY_XBUTTON_BACK;
	m_sourceKeyEnumMap["KEY_XBUTTON_START"] = KEY_XBUTTON_START;
	m_sourceKeyEnumMap["KEY_XBUTTON_STICK1"] = KEY_XBUTTON_STICK1;
	m_sourceKeyEnumMap["KEY_XBUTTON_STICK2"] = KEY_XBUTTON_STICK2;
	m_sourceKeyEnumMap["KEY_XSTICK1_RIGHT"] = KEY_XSTICK1_RIGHT;
	m_sourceKeyEnumMap["KEY_XSTICK1_LEFT"] = KEY_XSTICK1_LEFT;
	m_sourceKeyEnumMap["KEY_XSTICK1_DOWN"] = KEY_XSTICK1_DOWN;
	m_sourceKeyEnumMap["KEY_XSTICK1_UP"] = KEY_XSTICK1_UP;
	m_sourceKeyEnumMap["KEY_XBUTTON_LTRIGGER"] = KEY_XBUTTON_LTRIGGER;
	m_sourceKeyEnumMap["KEY_XBUTTON_RTRIGGER"] = KEY_XBUTTON_RTRIGGER;
	m_sourceKeyEnumMap["KEY_XSTICK2_RIGHT"] = KEY_XSTICK2_RIGHT;
	m_sourceKeyEnumMap["KEY_XSTICK2_LEFT"] = KEY_XSTICK2_LEFT;
	m_sourceKeyEnumMap["KEY_XSTICK2_DOWN"] = KEY_XSTICK2_DOWN;
	m_sourceKeyEnumMap["KEY_XSTICK2_UP"] = KEY_XSTICK2_UP;
}

C_InputManager::~C_InputManager()
{
	DevMsg("InputManager: Destructor\n");
	m_pHeldKeys->deleteThis();
	m_pHeldKeys = null;
}

/*
void C_InputManager::SetInputListener(void* pInputListener, listener_t type)
{
	m_pInputListener = pInputListener;
	m_inputListenerType = type;
}
*/

void C_InputManager::Update()
{
	if (g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->IsHandTrackingActive())
	{
#ifdef VR_ALLOWED
		//float acceleration = 1.3;

		float flPreviousJoystickForward = -hmdGetButtonValue(ButtonsList::right_PadYAxis, ControllerTypes::controllerType_Virtual);// *2.0;
		//m_flPreviousJoystickForward *= acceleration;

		float flPreviousJoystickSide = hmdGetButtonValue(ButtonsList::right_PadXAxis, ControllerTypes::controllerType_Virtual);
		//m_flPreviousJoystickSide *= acceleration;

		float flPreviousJoystickPitch = -hmdGetButtonValue(ButtonsList::left_PadYAxis, ControllerTypes::controllerType_Virtual);;
		float flPreviousJoystickYaw = -hmdGetButtonValue(ButtonsList::left_PadXAxis, ControllerTypes::controllerType_Virtual);;
		g_pAnarchyManager->UpdateGamepadAxisInput(flPreviousJoystickForward, flPreviousJoystickSide, flPreviousJoystickPitch, flPreviousJoystickYaw);
		
		//if (g_pAnarchyManager->IsVRActive())
		//{
			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (pPlayer)
			{
				VMatrix playerMatrix;
				playerMatrix.SetupMatrixOrgAngles(pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles());
				///*

					int iVRSpazzFix2 = g_pAnarchyManager->GetVRSpazzFix(1);

					int iVRSpazzFix = g_pAnarchyManager->GetVRSpazzFix(0);// > GetVRHand(1);
					//C_BaseEntity* pVRSpazzFix = C_BaseEntity::Instance(g_pAnarchyManager->GetVRSpazzFix(0));// > GetVRHand(1);
					if (iVRSpazzFix >= 0 && iVRSpazzFix2 >= 0)
						//if (pVRSpazzFix)
					{
						//Vector origin = pPlayer->GetAbsOrigin();
						//origin.z += 12.0;
						//QAngle angles;
						//angles.Init();

						//UTIL_SetOrigin(pVRSpazzFix, origin);
						//pVRSpazzFix->SetAbsOrigin(origin);
						int iLast = (m_bDidLeftVRSpazzFixLast) ? 1 : 0;
						engine->ClientCmd(VarArgs("jump_object_pos %i %i %i;\n", iLast, iVRSpazzFix, iVRSpazzFix2));// %f %f %f %f %f %f , origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
						//engine->ClientCmd(VarArgs("jump_object_pos %i %f %f %f %f %f %f;\n", pVRSpazzFix->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
					}

					m_bDidLeftVRSpazzFixLast = !m_bDidLeftVRSpazzFixLast;
				/*else
				{
					int iVRSpazzFix2 = g_pAnarchyManager->GetVRSpazzFix(1);// > GetVRHand(1);
					//C_BaseEntity* pVRSpazzFix2 = C_BaseEntity::Instance(g_pAnarchyManager->GetVRSpazzFix(1));// > GetVRHand(1);
					if (iVRSpazzFix2 >= 0)
						//if (pVRSpazzFix2)
					{
						Vector origin = pPlayer->GetAbsOrigin();
						QAngle angles;
						angles.Init();

						//UTIL_SetOrigin(pVRSpazzFix2, origin);
						//pVRSpazzFix2->SetAbsOrigin(origin);

						engine->ClientCmd(VarArgs("jump_object_pos %i %f %f %f %f %f %f;\n", iVRSpazzFix2, origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
						//engine->ClientCmd(VarArgs("jump_object_pos %i %f %f %f %f %f %f;\n", pVRSpazzFix2->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
					}

					m_bDidLeftVRSpazzFixLast = false;
				}*/
				//*/
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

					//UTIL_SetOrigin(pVRHandLeft, origin);
					//pVRHandLeft->SetAbsAngles(angles);

					engine->ClientCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", pVRHandLeft->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
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

					//UTIL_SetOrigin(pVRHandRight, origin);
					//pVRHandRight->SetAbsAngles(angles);

					engine->ClientCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", pVRHandRight->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));	// servercmdfix, false);
				}

				C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();
				if (pShortcut)
				{
					bool bIsRightBumperDown = hmdGetButtonIsPressed(ButtonsList::right_Bumper, ControllerTypes::controllerType_Virtual);
					bool bIsLeftBumperDown = hmdGetButtonIsPressed(ButtonsList::left_Bumper, ControllerTypes::controllerType_Virtual);

					if (bIsRightBumperDown && bIsLeftBumperDown)
					{
						float flOldDist = g_pAnarchyManager->GetVRGestureDist();

						Vector originA = g_pAnarchyManager->GetVRHandMatrix(0).GetTranslation();
						Vector originB = g_pAnarchyManager->GetVRHandMatrix(1).GetTranslation();
						float flDist = originA.DistTo(originB);
						float flOriginal = g_pAnarchyManager->GetVRGestureValue();

						float flScale = flDist / flOldDist;
						transform_t* pTransform = g_pAnarchyManager->GetInstanceManager()->GetTransform();
						pTransform->scale = flOriginal  * flScale;
					}
				}
			}
		//}
#endif
	}

	InputSlate->Update();

	float fCurrentTime = engine->Time();
	float fInterval = 0.04;
	float flStartDelay = 0.5;
	if (fCurrentTime - m_flLastRepeat >= fInterval)
	{
		m_flLastRepeat = fCurrentTime;
		for (KeyValues *sub = m_pHeldKeys->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (fCurrentTime - sub->GetFloat("start") > flStartDelay)
				this->KeyCodePressed((vgui::KeyCode)sub->GetInt("code"), sub->GetBool("shift"), sub->GetBool("ctrl"), sub->GetBool("alt"), sub->GetBool("win"), true);
		}
	}
}

vgui::KeyCode C_InputManager::StringToSteamKeyEnum(std::string text)
{
	auto it = m_sourceKeyEnumMap.find(text);
	if (it != m_sourceKeyEnumMap.end())
		return it->second;

	return KEY_NONE;
}

ITexture* C_InputManager::GetInputSlateCanvasTexture()
{
	return InputSlate->GetCanvasTexture();
}

void C_InputManager::ResetRepeatKeys()
{
	m_pHeldKeys->deleteThis();
	m_pHeldKeys = new KeyValues("keys");
}

void C_InputManager::SetFullscreenMode(bool value)
{
	//bool bNeedsNotify = (m_bFullscreenMode != value);
	m_bFullscreenMode = value;

	if (!m_pReShadeConVar)
	{
		m_pReShadeConVar = cvar->FindVar("reshade");
		m_pReShadeDepthConVar = cvar->FindVar("reshadedepth");
		m_pDrawVGUIConVar = cvar->FindVar("r_drawvgui");
	}

	if (m_bFullscreenMode && m_pReShadeConVar->GetBool() && !m_pDrawVGUIConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
		m_pDrawVGUIConVar->SetValue(true);
	else if (engine->IsInGame() && !m_bFullscreenMode && m_pReShadeConVar->GetBool() && m_pDrawVGUIConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
		m_pDrawVGUIConVar->SetValue(false);

	//if ( bNeedsNotify)
	//	g_pAnarchyManager->HudStateNotify();
}

void C_InputManager::ForceInputMode()
{
	if (!m_bInputMode)
		return;

	m_bForcedInputMode = true;
	m_bWasForcedInputMode = true;
}

void C_InputManager::ActivateInputMode(bool bFullscreen, bool bMainMenu, C_EmbeddedInstance* pEmbeddedInstance, bool bInputCapture, bool bClearHud)//C_InputListener* pListener)
{
	if (g_pAnarchyManager->IsPaused())
		return;

//	/*
	if (m_bInputMode || (!bFullscreen && !g_pAnarchyManager->GetSelectedEntity() && bInputCapture))
		return;
//	*/

	m_bMainMenuMode = bMainMenu;

		//engine->ClientCmd("pause");

	m_bInputMode = true;

//	/*
	m_bFullscreenMode = (bFullscreen || !g_pAnarchyManager->GetSelectedEntity());	// !g_pAnarchyManager->GetSelectedEntity();// (bFullscreen || !g_pAnarchyManager->GetSelectedEntity());	// Only allow non-fullscreen mode if there is an entity selected
//	*/

	//m_bFullscreenMode = bFullscreen;	// this is probably not supposed to be here.  delete it & this comment.
	m_bInputCapture = bInputCapture;

	//m_pInputListener = pListener;
	m_pEmbeddedInstance = pEmbeddedInstance;

//	if (bFullscreen)
//		m_bWasForcedInputMode = true;

	// if no web tab is selected, then select the hud web tab.
//	/*
	//C_WebManager* pWebManager = g_pAnarchyManager->GetWebManager();

//	g_pAnarchyManager->GetSelectedEntity()->webt
	//C_EmbeddedInstance* pSelectedEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
//	if (pSelectedEmbeddedInstance)
		//pS

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (!m_pEmbeddedInstance)
	{
		pHudBrowserInstance->Select();
		pHudBrowserInstance->Focus();
		this->SetEmbeddedInstance(pHudBrowserInstance);
		cvar->FindVar("glow_enabled")->SetValue(true);
	}
	else if (m_pEmbeddedInstance == pHudBrowserInstance)
	{
		pHudBrowserInstance->Select();
		pHudBrowserInstance->Focus();
		cvar->FindVar("glow_enabled")->SetValue(true);
		//this->SetEmbeddedInstance(pHudBrowserInstance);
	}
	else
	{
		if (m_pEmbeddedInstance)
		{
			pHudBrowserInstance->Blur();
			m_pEmbeddedInstance->Select();
			m_pEmbeddedInstance->Focus();
		}

		cvar->FindVar("glow_enabled")->SetValue(false);
	}

//	C_EmbeddedInstance* pSelectedEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
//	if (pSelectedEmbeddedInstance && )

		//		g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pHudBrowserInstance);
//	if (!g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance())
//		g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pHudBrowserInstance);
		//pWebManager->SelectWebTab(pWebManager->GetHudWebTab());
//	*/

	//ShowCursor(true);
	InputSlate->Create(enginevgui->GetPanel(PANEL_ROOT));

	if (m_bFullscreenMode && bMainMenu && engine->IsInGame())
		engine->ClientCmd("setpause");	// servercmdfix

	if (!m_pReShadeConVar)
	{
		m_pReShadeConVar = cvar->FindVar("reshade");
		m_pReShadeDepthConVar = cvar->FindVar("reshadedepth");
		m_pDrawVGUIConVar = cvar->FindVar("r_drawvgui");
	}

	if (m_bFullscreenMode && m_pReShadeConVar->GetBool() && !m_pDrawVGUIConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
		m_pDrawVGUIConVar->SetValue(true);

	// do a pre-render of blankness to the UI
	if (bClearHud)
	{
		g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(pHudBrowserInstance);
		pHudBrowserInstance->SetBlankShotPending(true);
		pHudBrowserInstance->GetTexture()->Download();
		g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(null);
	}

	if (bFullscreen && g_pAnarchyManager->GetAttractMode())
	{
		g_pAnarchyManager->ToggleAttractMode();
	}

	//g_pAnarchyManager->GetWebManager()->OnActivateInputMode();

	//if (m_bMainMenuMode)
	//{
		//e
		//engine->ClientCmd("toggleconsole;\n");
		//engine->ExecuteClientCmd("toggleconsole; toggleconsole;");
		//engine->ClientCmd_Unrestricted("toggleconsole; toggleconsole;");
		//engine->ClientCmd_Unrestricted("setpause");
		//engine->ServerCmd("pause");
		//		engine->ClientCmd("pause;\n");
		//engine->ExecuteClientCmd("unpause\n");
		//engine->ExecuteClientCmd("pause\n");
	//}

	//InputSlate->Create(enginevgui->GetPanel(PANEL_GAMEDLL));
	//InputSlate->Create(enginevgui->GetPanel(PANEL_TOOLS));
	//InputSlate->Create(enginevgui->GetPanel(PANEL_INGAMESCREENS));
	//InputSlate->Create(enginevgui->GetPanel(PANEL_CLIENTDLL));

//	DevMsg("Activated input mode.\n");
}
/*
void C_InputManager::ShutdownInputMode()
{
	if (!m_bInputMode)
		return;

	C_PropShortcutEntity* pSelectedEntity = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
	if (m_pEmbeddedInstance && !pSelectedEntity)
	{
		m_pEmbeddedInstance->Deselect();
		m_pEmbeddedInstance->Blur();
	}

	// THE DEFAULT VALUES OF STUFF (mostly from the constructor)
	m_bInputMode = false;
	m_bForcedInputMode = false;
	m_bWasForcedInputMode = false;
	m_bFullscreenMode = false;
	//m_bOverlayMode = false;
	m_bInputCapture = true;
	m_bMainMenuMode = false;
	//m_pEmbeddedInstance = null;
	//ShowCursor(false);

	InputSlate->Destroy();
}
*/

void C_InputManager::DeactivateInputMode(bool bForce)
{
	this->SetGamepadInputMode(false);
	//g_pAnarchyManager->BringToTop();

	if (g_pAnarchyManager->IsPaused())
		return;

	if (!bForce && m_bForcedInputMode)
	{
		m_bForcedInputMode = false;
		return;
	}
	
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	//DevMsg("Yadda: %s\n", pHudBrowserInstance->GetURL());
	if (pHudBrowserInstance->GetURL().find("asset://ui/loading.html") != std::string::npos)
		return;
	
	g_pAnarchyManager->RemoveLastHoverGlowEffect();

	if (!m_bInputMode)
		return;
	pHudBrowserInstance->SetRenderHold(false);
	bool bWasFullscreen = this->GetFullscreenMode();
	C_EmbeddedInstance* pOldInstance = m_pEmbeddedInstance;

	//if (m_bMainMenuMode)
	//	engine->ClientCmd("toggleconsole;\n");
		//engine->ClientCmd_Unrestricted("unpause");
	//	engine->ServerCmd("unpause");
		//engine->ClientCmd("unpause;\n");
		//engine->ExecuteClientCmd("unpause\n");
		//engine->ClientCmd("unpause");

//	/*
	if (g_pAnarchyManager->GetInputManager()->GetMainMenuMode())
	{
		//g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud")->SetUrl("asset://ui/default.html");

		if (!engine->IsInGame())
		{
			DevMsg("Warning: Did the main menu just get hidden and now the default one is showing?\n");
			//g_pAnarchyManager->RunAArcade();

			//C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
			//pHudBrowserInstance->SetUrl("asset://ui/welcome.html");
			//g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);
			//m_bSuspendEmbedded = false;
		}
	}
//		g_pAnarchyManager->GetWebManager()->GetHudWebTab()->SetUrl("asset://ui/blank.html");

	if (m_bFullscreenMode && m_bInputCapture)	// TODO: Add more checks here, like if the selected entity's web tab is also the selected entity.
	{
	//	C_WebManager* pWebManager = g_pAnarchyManager->GetWebManager();
		//if (pWebManager->GetSelectedWebTab() && !g_pAnarchyManager->GetSelectedEntity())
		C_PropShortcutEntity* pSelectedEntity = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
		if (m_pEmbeddedInstance && !pSelectedEntity)
		{
			m_pEmbeddedInstance->Deselect();
			m_pEmbeddedInstance->Blur();
		}
		/*
		else if (pSelectedEntity)
		{
			// if there is still an entity selected when this (probably context) menu is closed, set the 1st embedded instance found on it as the focused embedded instance.
			std::vector<C_EmbeddedInstance*> embeddedInstances;
			pSelectedEntity->GetEmbeddedInstances(embeddedInstances);

			if (embeddedInstances.size() > 0 && embeddedInstances[0] != m_pEmbeddedInstance)
			{
				DevMsg("Re-selecting embedded instance of the selected entity.\n");
				g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(embeddedInstances[0]);
			}
		}
		*/
			//pWebManager->DeselectWebTab(pWebManager->GetSelectedWebTab());
		
		if (g_pAnarchyManager->GetInputManager()->GetMainMenuMode() && engine->IsInGame() )
		{
			engine->ClientCmd("gamemenucommand ResumeGame");
		}
	}
//	*/

	// THE DEFAULT VALUES OF STUFF (mostly from the constructor)
	cvar->FindVar("glow_enabled")->SetValue(true);
	m_bInputMode = false;
	m_bForcedInputMode = false;
	m_bWasForcedInputMode = false;
	m_bFullscreenMode = false;
	//m_bOverlayMode = false;
	m_bInputCapture = true;
	m_bMainMenuMode = false;
	//m_pEmbeddedInstance = null;
	//ShowCursor(false);

	g_pAnarchyManager->SetRightFreeMouseToggle(false);

	this->ResetRepeatKeys();
	InputSlate->Destroy();

	// automatically destroy instances that have "scrape" in the front of their ID
	C_SteamBrowserInstance* pSteamBrowserInstance = dynamic_cast<C_SteamBrowserInstance*>(pOldInstance);
	if (pSteamBrowserInstance && pSteamBrowserInstance->GetId().find("scrape") == 0)
		g_pAnarchyManager->GetSteamBrowserManager()->DestroySteamBrowserInstance(pSteamBrowserInstance);

	if (bWasFullscreen && cvar->FindVar("freemousemode")->GetBool() )
		g_pAnarchyManager->BringToTop();

	pHudBrowserInstance->SetBlankShotPending(true);
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(pHudBrowserInstance);
	pHudBrowserInstance->GetTexture()->Download();
	g_pAnarchyManager->GetCanvasManager()->GetOrCreateRegen()->SetEmbeddedInstance(null);

	if (!m_pReShadeConVar)
	{
		m_pReShadeConVar = cvar->FindVar("reshade");
		m_pReShadeDepthConVar = cvar->FindVar("reshadedepth");
		m_pDrawVGUIConVar = cvar->FindVar("r_drawvgui");
	}

	if (engine->IsInGame() && m_pReShadeConVar->GetBool() && m_pDrawVGUIConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
		m_pDrawVGUIConVar->SetValue(false);
	else if (!engine->IsInGame() && !m_pDrawVGUIConVar->GetBool())
		m_pDrawVGUIConVar->SetValue(true);
}

void C_InputManager::MouseMove(float x, float y)
{
	if (m_pEmbeddedInstance)
	{
		C_InputListener* pInputListener = m_pEmbeddedInstance->GetInputListener();
		if (pInputListener)
			pInputListener->OnMouseMove(x, y);
	}

	C_AwesomiumBrowserInstance* pHudEmbeddedInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	C_InputListenerAwesomiumBrowser* pInputListenerAwesomiumBrowser = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetInputListener();
	if (pInputListenerAwesomiumBrowser)
		pInputListenerAwesomiumBrowser->OnMouseMove(x, y, pHudEmbeddedInstance);
	/*
	C_InputListenerAwesomiumBrowser* pInputListenerAwesomiumBrowser = dynamic_cast<C_InputListenerAwesomiumBrowser*>(pHudEmbeddedInstance->GetInputListener());	// this actually returns the 1 input listener that is used for ALL awesomium browser instances.
	if (pInputListenerAwesomiumBrowser)
		pInputListenerAwesomiumBrowser->OnMouseMove(x, y, pInputListenerAwesomiumBrowser);
	*/

	/*
	// forward this info to any listeners
	if (m_pInputListener)
	{
		if (m_inputListenerType == LISTENER_WEB_MANAGER)
		{
			C_WebManager* pWebManager = static_cast<C_WebManager*>(m_pInputListener);
			if (pWebManager)
				pWebManager->OnMouseMove(x, y);
		}
	}
	*/
}

void C_InputManager::MousePress(vgui::MouseCode code)
{
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	float flLastX, flLastY;
	pHudInstance->GetLastMouse(flLastX, flLastY);
	if (pHudInstance->GetAlphaAtPoint(round(flLastX*AA_HUD_INSTANCE_WIDTH), round(flLastY*AA_HUD_INSTANCE_HEIGHT)) == 255)
	{
		//pHudBrowserInstance->Select();
		if (m_pEmbeddedInstance && pHudInstance != m_pEmbeddedInstance)
			m_pEmbeddedInstance->Blur();

		pHudInstance->Focus();
		pHudInstance->GetInputListener()->OnMousePressed(code);
	}
	else if (m_pEmbeddedInstance)
	{
		pHudInstance->Blur();

		m_pEmbeddedInstance->Focus();
		m_pEmbeddedInstance->GetInputListener()->OnMousePressed(code);
	}
//	if (m_pInputListener)
	//	m_pInputListener->OnMousePressed(code);
	/*
	// forward this info to any listeners
	if (m_pInputListener)
	{
		if (m_inputListenerType == LISTENER_WEB_MANAGER)
		{
			C_WebManager* pWebManager = static_cast<C_WebManager*>(m_pInputListener);
			if (pWebManager)
				pWebManager->OnMousePress(code);
		}
	}
	*/
}

void C_InputManager::MouseRelease(vgui::MouseCode code)
{
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	float flLastX, flLastY;
	pHudInstance->GetLastMouse(flLastX, flLastY);
	if (pHudInstance->GetAlphaAtPoint(round(flLastX*AA_HUD_INSTANCE_WIDTH), round(flLastY*AA_HUD_INSTANCE_HEIGHT)) == 255)
	{
		//pHudBrowserInstance->Select();
		if (m_pEmbeddedInstance && pHudInstance != m_pEmbeddedInstance)
			m_pEmbeddedInstance->Blur();

		pHudInstance->Focus();
		pHudInstance->GetInputListener()->OnMouseReleased(code);
	}
	else if (m_pEmbeddedInstance)
	{
		pHudInstance->Blur();

		m_pEmbeddedInstance->Focus();
		m_pEmbeddedInstance->GetInputListener()->OnMouseReleased(code);
	}

	//C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	//if (pHudBrowserInstance)
	//	pHudBrowserInstance->GetInputListener()->OnMouseReleased(code);

	//if (m_pInputListener)
	//	m_pInputListener->OnMouseReleased(code);
	/*
	// forward this info to any listeners
	if (m_pInputListener)
	{
		if (m_inputListenerType == LISTENER_WEB_MANAGER)
		{
			C_WebManager* pWebManager = static_cast<C_WebManager*>(m_pInputListener);
			if (pWebManager)
				pWebManager->OnMouseRelease(code);
		}
	}
	*/
}

void C_InputManager::KeyCodePressed(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (pHudBrowserInstance->HasFocus())
	{
		C_InputListener* pInputListener = pHudBrowserInstance->GetInputListener();
		pInputListener->OnKeyCodePressed(code, bShiftState, bCtrlState, bAltState, bWinState, bAutorepeatState);
	}
	else if (m_pEmbeddedInstance)
	{
		C_InputListener* pInputListener = m_pEmbeddedInstance->GetInputListener();
		pInputListener->OnKeyCodePressed(code, bShiftState, bCtrlState, bAltState, bWinState, bAutorepeatState);
	}

	// skip keys that don't need repeating...
	if (code == vgui::KeyCode::KEY_LALT || code == vgui::KeyCode::KEY_RALT || code == vgui::KeyCode::KEY_LSHIFT || code == vgui::KeyCode::KEY_RSHIFT || code == vgui::KeyCode::KEY_LCONTROL || code == vgui::KeyCode::KEY_RCONTROL || code == vgui::KeyCode::KEY_LWIN || code == vgui::KeyCode::KEY_RWIN)
		return;

	KeyValues* pButtonKey = m_pHeldKeys->FindKey(VarArgs("code%i", code));
	if (!pButtonKey)	// could use bAutorepeatState instead?
	{
		pButtonKey = m_pHeldKeys->CreateNewKey();
		pButtonKey->SetName(VarArgs("code%i", code));
		pButtonKey->SetInt("code", code);
		pButtonKey->SetInt("shift", bShiftState);
		pButtonKey->SetInt("ctrl", bCtrlState);
		pButtonKey->SetInt("alt", bAltState);
		pButtonKey->SetInt("win", bWinState);
		pButtonKey->SetFloat("start", engine->Time());// gpGlobals->curtime);
		//pButtonKey->SetInt("repeat", bAutorepeatState);
	}

	/*
	// forward this info to any listeners
	if (m_pInputListener)
	{
		if (m_inputListenerType == LISTENER_WEB_MANAGER)
		{
			C_WebManager* pWebManager = static_cast<C_WebManager*>(m_pInputListener);
			if (pWebManager)
			{
				pWebManager->OnKeyCodePressed(code, bShiftState, bCtrlState, bAltState);
			}
		}
	}
	*/
}

void C_InputManager::OnMouseWheeled(int delta)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (pHudBrowserInstance->HasFocus())
	{
		C_InputListener* pInputListener = pHudBrowserInstance->GetInputListener();
		pInputListener->OnMouseWheeled(delta);
	}
	else if (m_pEmbeddedInstance)
	{
		C_InputListener* pInputListener = m_pEmbeddedInstance->GetInputListener();
		pInputListener->OnMouseWheeled(delta);
	}
}

void C_InputManager::MouseWheelDown()
{
	if (!this->GetInputMode())
		return; // DO DEFAULT MAPPED MOUSE WHEEL DOWN ACTION!!

	this->OnMouseWheeled(-10);
}

void C_InputManager::MouseWheelUp()
{
	if (!this->GetInputMode())
		return; // DO DEFAULT MAPPED MOUSE WHEEL DOWN ACTION!!

	this->OnMouseWheeled(10);
}

void C_InputManager::KeyCodeReleased(vgui::KeyCode code, bool bShiftState, bool bCtrlState, bool bAltState, bool bWinState, bool bAutorepeatState)
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (pHudBrowserInstance->HasFocus())
	{
		C_InputListener* pInputListener = pHudBrowserInstance->GetInputListener();
		pInputListener->OnKeyCodeReleased(code, bShiftState, bCtrlState, bAltState, bWinState, bAutorepeatState);
		//C_InputListenerAwesomiumBrowser* listener = dynamic_cast<C_InputListenerAwesomiumBrowser*>(pInputListener);
		//listener->OnKeyCodeReleased(code, bShiftState, bCtrlState, bAltState, bWinState, bAutorepeatState);
	}
	else if (m_pEmbeddedInstance)
	{
		C_InputListener* pInputListener = m_pEmbeddedInstance->GetInputListener();
		pInputListener->OnKeyCodeReleased(code, bShiftState, bCtrlState, bAltState, bWinState, bAutorepeatState);
	}

	KeyValues* pButtonKey = m_pHeldKeys->FindKey(VarArgs("code%i", code));
	if (pButtonKey)
		m_pHeldKeys->RemoveSubKey(pButtonKey);
	/*
	// forward this info to any listeners
	if (m_pInputListener)
	{
		if (m_inputListenerType == LISTENER_WEB_MANAGER)
		{
			C_WebManager* pWebManager = static_cast<C_WebManager*>(m_pInputListener);
			if (pWebManager)
			{
				pWebManager->OnKeyCodeReleased(code, bShiftState, bCtrlState, bAltState);
			}
		}
	}
	*/
}