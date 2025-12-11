#ifndef C_ANARCHY_MANAGER_H
#define C_ANARCHY_MANAGER_H

#include <KeyValues.h>
#include "sourcevr/isourcevirtualreality.h"
#include "c_canvasmanager.h"
//#include "c_webmanager.h"
//#include "c_loadingmanager.h"
#include "c_libretromanager.h"
#include "c_steambrowsermanager.h"
#include "c_awesomiumbrowsermanager.h"
#include "c_inputmanager.h"
#include "c_mountmanager.h"
#include "c_toast.h"
#include "c_workshopmanager.h"
#include "c_metaversemanager.h"
#include "c_aaimanager.h"
#include "c_backpackmanager.h"
#include "c_instancemanager.h"
#include "c_windowmanager.h"
#include "c_questmanager.h"
#include <vector>
#include "vgui/ISystem.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Controls.h"
#include "hlvr/proxydll.h"
#include "c_accountant.h"

enum aaState
{
	AASTATE_NONE = 0,
	AASTATE_STATS = 1,
	AASTATE_INPUTMANAGER = 2,
	AASTATE_CANVASMANAGER = 3,
	AASTATE_LIBRETROMANAGER = 4,
	AASTATE_STEAMBROWSERMANAGER = 5,
	AASTATE_AWESOMIUMBROWSERMANAGER = 6,
	AASTATE_AWESOMIUMBROWSERMANAGERWAIT = 7,
	AASTATE_AWESOMIUMBROWSERMANAGERHUD = 8,
	AASTATE_AWESOMIUMBROWSERMANAGERHUDWAIT = 9,
	AASTATE_AWESOMIUMBROWSERMANAGERHUDINIT = 10,
	AASTATE_AWESOMIUMBROWSERMANAGERNETWORK = 11,
	AASTATE_AWESOMIUMBROWSERMANAGERNETWORKWAIT = 12,
	AASTATE_AWESOMIUMBROWSERMANAGERNETWORKINIT = 13,
	AASTATE_AWESOMIUMBROWSERMANAGERIMAGES = 14,
	AASTATE_AWESOMIUMBROWSERMANAGERIMAGESWAIT = 15,
	AASTATE_AWESOMIUMBROWSERMANAGERIMAGESINIT = 16,
	AASTATE_LIBRARYMANAGER = 17,
	AASTATE_MOUNTMANAGER = 18,
	AASTATE_WORKSHOPMANAGER = 19,
	AASTATE_INSTANCEMANAGER = 20,
	AASTATE_RUN = 21
};

enum launchErrorType_t {
	NONE = 0,
	UNKNOWN_ERROR = 1,
	ITEM_NOT_FOUND = 2,
	ITEM_FILE_NOT_FOUND = 3,
	ITEM_FILE_PATH_RESTRICTED = 4,
	APP_NOT_FOUND = 5,
	APP_FILE_NOT_FOUND = 6,
	APP_PATH_NOT_FOUND = 7
};

enum saveModeType_t {
	SAVEMODE_NONE = 0,
	SAVEMODE_SAVE = 1,
	SAVEMODE_DISCARD = 2
};

struct aaSteamFriendRichPresence_t {
	std::string name;
	uint64 id;
};

struct aaSteamFriend_t {
	std::string name;
	uint64 id;
}; 

enum panoshotState {
	PANO_NONE,
	PANO_ORIENT_SHOT_0,
	PANO_TAKE_SHOT_0,
	PANO_ORIENT_SHOT_1,
	PANO_TAKE_SHOT_1,
	PANO_ORIENT_SHOT_2,
	PANO_TAKE_SHOT_2,
	PANO_ORIENT_SHOT_3,
	PANO_TAKE_SHOT_3,
	PANO_ORIENT_SHOT_4,
	PANO_TAKE_SHOT_4,
	PANO_ORIENT_SHOT_5,
	PANO_TAKE_SHOT_5,
	PANO_ORIENT_SHOT_6,
	PANO_TAKE_SHOT_6,
	PANO_COMPLETE
};

struct panoStuff_t {
	std::string weapons;
	std::string hud;
	std::string titles;
	std::string toast;
	std::string developer;
	std::string exposuremin;
	std::string exposuremax;
	std::string hdrlevel;
	std::string fov;
	//std::string firstperson;	// not sure how to check if we're already in 1st person, so gonna have to just switch into 1st and not switch back.
};

struct nextLoadInfo_t {
	std::string instanceId;
	std::string position;
	std::string rotation;
};

struct aampConnection_t {
	bool connected;
	bool isHost;
	bool canBuild;
	std::string address;
	std::string universe;
	std::string instance;
	std::string user;
	std::string session;
	std::string lobby;
	std::string lobbyPassword;
	bool isPublic;
	bool isPersistent;
};

struct taskinfo_t {
	std::string id;
	std::string title;
	std::string itemTitle;
	bool isDisplayTask;
	bool isWindowsTask;
	bool isHiddenTask;
	bool isPresetHiddenTask;
	std::string embeddedType;
	int entityIndex;
	std::string itemId;
	std::string objectId;
	std::string modelId;
};

struct systemTimeState_t {
	int iDay;
	int iDate;
	int iMonths;
	int iYears;
	int iPostfix;
	int iHours;
	int iMinutes;
	int iSeconds;
};

struct numberStatsState_t {
	int iObjects;
	int iGlobalPlayers;
	int iGlobalTime;
	int iGlobalTubes;
	int iServerVisits;
	int iServerVisitors;
	int iLibraryItems;
	int iLibraryMaps;
	int iLibraryModels;
};

struct pet_t {
	int iEntityIndex;
	KeyValues* pConfigKV;
	int iState;
	int iWaterState;
	int iCurSequence;
	int iBehavior;
	//Vector wanderHome;
	Vector pos;
	QAngle rot;
	std::string outfitId;
	std::map<std::string, std::vector<std::string>> attachments;
	//std::vector<std::string> attachments;
	//int iTargetEntityIndex;
};

struct steamHTTPImageDownload_t {
	std::string url;
	std::string status;	// "pending", "success", "failure"
};

enum aaPetState
{
	AAPETSTATE_NONE = -1,
	AAPETSTATE_IDLE,
	AAPETSTATE_WALK,
	AAPETSTATE_RUN,
	AAPETSTATE_FALL
};

enum aaPetBehavior
{
	AAPETBEHAVIOR_NONE = 0,
	AAPETBEHAVIOR_LOOK,
	AAPETBEHAVIOR_FOLLOW,
	AAPETBEHAVIOR_WANDER
};

enum aaPetStaggerPattern
{
	PET_STAGGER_SINGLE_FILE = 0,      // Near single file, with slight variations
	PET_STAGGER_THEME_PARK_LINE,  // Staggered queue formation like in a theme park
	PET_STAGGER_CIRCLE,           // Surrounding the target in a circular crowd
	PET_STAGGER_SEMI_CIRCLE,      // Semi-circle around the target
	PET_STAGGER_V_SHAPE,          // V-formation facing forward
	PET_STAGGER_RANDOM_BLOB       // Loose, natural cluster around the target
};

class C_AnarchyManager : public CAutoGameSystemPerFrame
{
public:
	C_AnarchyManager();
	~C_AnarchyManager();

	void Tester();
	void DownloadSingleFile(std::string url);
	void DownloadSteamHTTPImage(std::string url);
	unsigned int HexStrToUint32(const char* hex);
	void ClearSteamHTTPImageDownloadRequests();

	virtual bool Init();
	virtual void PostInit();
	bool sortPlayerDist(object_t* pObjectA, object_t* pObjectB);
	void OnAccountantReady();
	void OnStartup();
	void OnStartupCallback(bool bForceDefaultAdd = false);
	void OnStartupCallbackB();	// Same as above, but auto-detects if default add should be forced.
	void OnAddNextDefaultLibraryCallback();
	void OnDefaultLibraryReadyCallback();
	void OnDefaultLibraryReady();
	void OnUpdateLibraryVersionCallback();
	void OnReadyToLoadUserLibrary();
	void OnRebuildSoundCacheCallback();
	virtual void Shutdown();
	bool IsShuttingDown() { return m_bIsShuttingDown; }

	//const char* GetProjectorMaterialName();

	void ResetImageLoader();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreClearSteamAPIContext();
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	bool ShouldPrecacheInstance() { return m_bPrecacheInstances; }
	bool ShouldBatchObjectSpawn() { return m_bBatchObjectSpawn; }

	void InstaLaunchItem(std::string itemId);
	void GamePrecache();

	void PaintMaterial();
	void UnpaintMaterial();
	void PickPaintTexture();
	void AdoptSky(std::string skyname);

	std::string GenerateTextureThumb(std::string textureName);

	void SaveCurrentPetInfo();
	void RestoreCurrentPetInfo(bool bPlayAsNext = true);

	void SavePetStates();
	void RestorePetStates();
	void ProcessPendingPetsRestore();
	void AfterPendingPetsRestore();
	void AfterPendingPetsLoadBatch();

	// for cylcing between different batches of pets (virtual TV stuff show stuff in mind.)
	void LoadPetBatch(std::string batchName);
	void SavePetBatch(std::string batchName);

	pet_t* GetPlayAsPet();
	void SetPlayAsPet(pet_t* pPet);
	//C_DynamicProp* GetPlayAsPetEntity();	// depreciate me!!
	pet_t* GetNearestPetToPlayerLook(float flMaxRange = 300.0f);
	void SpawnPet(std::string model, std::string run, std::string walk, std::string idle, std::string fall, float flScale, std::string rot, std::string pos, float flNear, float flFar, float flRunSpeed, float flWalkSpeed, std::string outfit, std::string behavior, std::string sequence);
	//void UpdatePet(int iEntIndex, std::string run = "", std::string walk = "", std::string idle = "", std::string fall = "", float flScale = 0.0f, std::string rot = "", std::string pos = "", float flNear = 0.0f, float flFar = 0.0f, float flRunSpeed = 0.0f, float flWalkSpeed = 0.0f);
	void PetCreated(int iEntIndex);
	void DestroyPet(int iEntIndex);
	void DestroyAllPets();
	void ProcessAllPets();
	pet_t* GetPetByEntIndex(int iEntIndex);

	void LookspotCreated(int iSpotIndex, int iHaloIndex, int iDirIndex);
	void ProcessLookspot();
	void ToggleLookspot(int iValue = -1);
	void DestroyLookspot();

	bool IsLevelInitialized() { return m_bLevelInitialized; }

	virtual void OnSave();
	virtual void OnRestore();
	virtual void SafeRemoveIfDesired();

	virtual bool IsPerFrame();

	// Called before rendering
	virtual void PreRender();

	// Gets called each frame
	virtual void Update(float frametime);

	// Called after rendering
	virtual void PostRender();
	//void ReloadMap();

	bool IsAlwaysAnimatingImagesEnabled();

	char* BackBufferNamePerIndex(int i);

	bool CheckIfFileExists(std::string file);

	void TestVRStuff();
	bool GetShouldInvertVRMatrices();
	void ToggleVR();
	void VROff();
	void InitializeVR();
	bool OpenVRHMD();
	ITexture* C_AnarchyManager::CreateVRSpectatorRenderTarget(IMaterialSystem* pMaterialSystem);
	ITexture* CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem, int i);
	ITexture* GetRenderTarget(ISourceVirtualReality::VREye eEye, ISourceVirtualReality::EWhichRenderTarget eWhich);
	void InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig);
	void ShutdownClientRenderTargets();
	int GetVRBufferWidth() { return m_iVRBufferWidth; }
	int GetVRBufferHeight() { return m_iVRBufferHeight; }
	void GetViewportBounds(ISourceVirtualReality::VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight);
	bool VRUpdate();
	void VRFrameBegin();
	void VRFrameReady();
	//void VRFrameEnd();
	void CloseVR();
	void CreateVRHands();
	void VRHandCreated(int iEntIndex, int iHandSide, int iPointerIndex, int iTeleportIndex);
	VMatrix GetVRHandMatrix(int iHandSide);
	C_DynamicProp* GetVRHand(int iHandSide);
	C_DynamicProp* GetVRPointer(int iHandSide);
	C_DynamicProp* GetVRTeleport();
	VMatrix SMMatrixToVMatrix(float matrix[16], int iEyeFactor = 0, bool bNoZOffset = false);
	void VRControllerVibrateStart(int iHandSide, int iPercent);
	void VRControllerVibrateStop(int iHandSide);
	void CalcFovFromProjection(float *pFov, const VMatrix &proj);
	VMatrix GetMidEyeFromEye(ISourceVirtualReality::VREye eEye);
	void RenderVREyeToView(const CViewSetup &view, ISourceVirtualReality::VREye eyeToCopy);// , bool isMenuUp);
	void RenderMonoEyeToVRSpectatorCamera(const CViewSetup &view);
	//bool OverrideStereoView(CViewSetup *pViewMiddle, CViewSetup *pViewLeft, CViewSetup *pViewRight);

	//void SelectorTraceLine(trace_t &tr);
	//void SelectorHand();

	bool UseBuildGhosts();

	bool CheckVideoCardAbilities();

	void ThrowEarlyError(const char* msg);

	void GetSteamPlayerCount();
	void GetSteamPlayerCountResponse(NumberOfCurrentPlayers_t *pResult, bool bIOFailure);
	void RequestGlobalStatsResponse(GlobalStatsReceived_t *pResult, bool bIOFailure);

	void DetectAllModelsThreaded();
	void ManageImportScans();
	void ProcessAllModels();
	void ProcessNextModel();
	void AddNextModel();

	void CreateItemMenu(std::string fileLocation);
	void CreateJSONItemMenu(std::string json);
	void CaptureWindowSnapshotsAll();
	BYTE* CaptureWindowSnapshot(HWND hwnd, int& iWidth, int& iHeight);

	void ManageWindow();
	void ManageHUDLabels();

	void ManageAudioStrips();

	void MainMenuLoaded();
	void GetAllFriends(std::vector<aaSteamFriend_t*>& steamFriends);
	//aaSteamFriendRichPresence_t* GetFriendRichPresence(uint64 steamId);

	void ClearAttractMode();
	void ToggleAttractMode();
	void FindNextAttractCamera();
	void FindPreviousAttractCamera();
	void AttractCameraReached();

	void ShowAdoptAssetMenu();

	bool CAM_IsThirdPerson();

	std::string GetMaterialUnderCrosshair();
	std::string GetModelUnderCrosshair();

	void SetSocialMode(bool bValue);
	bool CheckStartWithWindows();
	bool SetStartWithWindows(bool bValue);
	void InitHoverLabel(vgui::Label* pHoverLabel) { m_pHoverLabel = pHoverLabel; }
	void UpdateGamepadAxisInput(float flPreviousJoystickForward, float flPreviousJoystickSide, float flPreviousJoystickPitch, float flPreviousJoystickYaw);
	void VRMutateGamepadInput(float* flPreviousJoystickForward, float* flPreviousJoystickSide, float* flPreviousJoystickPitch, float* flPreviousJoystickYaw);
	void ManageGamepadInput(float flFrametime);
	void ManageHoverLabel();
	void UpdateHoverLabel();
	std::string GetHoverTitle() { return m_hoverTitle; }
	int GetHoverEntityIndex() { return m_iHoverEntityIndex; }
	bool GetHoverEntityShortcut() { return m_bHoverEntityShortcut; }
	//void SetHoverLabel(int iEntityIndex, std::string title);

	void EnableSBSRendering();
	void DisableSBSRendering();
	
	void OnSelectorTraceResponse(int iEntity, float flX, float flY, float flZ, float flNormalX, float flNormalY, float flNormalZ);

	std::string ExtractRelativeAssetPath(std::string fullPath);
	bool UseSBSRendering();

	object_t* GetObjectUnderCursor(int iMouseX, int iMouseY);
	void ScreenToWorld(int mousex, int mousey, float fov, const Vector& vecRenderOrigin, const QAngle& vecRenderAngles, Vector& vecPickingRay);

	bool GetTaskInfo(C_EmbeddedInstance* pEmbeddedInstance, taskinfo_t &taskinfo);

	void OnSpawnObjectsButtonDown();
	void OnSpawnObjectsButtonUp();

	void BringToTop();

	void JoinLobbyWeb(std::string lobbyId);
	void JoinLobby(std::string lobbyId);
	//void DoConnect(std::string lobbyId);
	void PopToast();
	void AddToastMessage(std::string text, bool bForce = false);
	void SetNextToastExpiration(float fValue) { m_fNextToastExpiration = fValue; }
	float GetNextToastExpiration() { return m_fNextToastExpiration; }
	void AddToastLabel(vgui::Label* pLabel);
	void RemoveToastLabel(vgui::Label* pLabel);
	void SetToastText(std::string text);
	void UpdateToastText();
	std::string GetToastText() { return m_toastText; }

	void DoTakeBigScreenshot();
	void InteractiveScreenshotReady(std::string screenshotId, std::string codeText);
	void OpenScreenshot(std::string screenshotId);

	//void CalculateDynamicMultiplyer();
	void CheckPicMip();
	int GetDynamicMultiplyer();
	//void SetDynamicMultiplyer(int val) { m_iDynamicMultiplyer = val; }

	void ShowWindowsTaskBar();

	void SpecialReady(C_AwesomiumBrowserInstance* pInstance);

	bool QuickRemember(int iEntityIndex, std::string mode = "");
	bool TempSelectEntity(int iEntityIndex);

	bool HandleUiToggle(bool bIsFromToggleHandler = false);
	bool HandleCycleToNextWeapon();
	bool HandleCycleToPrevWeapon();
	void DoPause(int iPauseModeOverride = -1);
	void Pause(int iPauseModeOverride = -1);
	void Unpause();
	bool IsPaused() { return m_bPaused; }

	std::string GetLastLaunchedItemID() { return m_lastLaunchedItemId; }
	void SetLastLaunchedItemId(std::string value);

	float GetZNear();

	void PrepPano();
	void FinishPano();

	C_SteamBrowserInstance* AutoInspect(KeyValues* pItemKV, std::string tabId = "", int iEntIndex = -1, std::string imageFlagsOverride = "");

	launchErrorType_t LaunchItem(std::string id);
	bool AlphabetSafe(std::string text, std::string in_alphabet = "");
	bool PrefixSafe(std::string text);
	bool DirectorySafe(std::string text);
	bool ExecutableSafe(std::string text);

	void ClearLookAtObjects();
	void ToggleAlwaysLookObject(C_PropShortcutEntity* pShortcut);
	void AddAlwaysLookObject(C_PropShortcutEntity* pShortcut);
	void RemoveAlwaysLookObject(C_PropShortcutEntity* pShortcut);
	void ManageAlwaysLookObjects();

	int GetFixedCameraSpectateMode();
	KeyValues* GetBestNonVRSpectatorCameraObject();
	C_PropShortcutEntity* GetBestSpectatorCameraObject();
	void LocalAvatarObjectCreated(int iEntIndex);

	bool GetBestSpectatorCameraScreenshot(Vector& origin, QAngle& angle);

	void AddSubKeysToKeys(KeyValues* kv, KeyValues* targetKV);	// TODO: Make the sibling to this weirdly named function a method of the anarchy manager too.

	bool WeaponsEnabled();
	bool LoadMapCommand(std::string mapId, std::string instanceId, std::string position, std::string rotation, std::string screenshotId);

	uint64 GetTimeNumber();
	std::string GetTimeString();

	void BeginImportSteamGames(std::string tabName);
	void Acquire(std::string query, bool bQuiteRun = false, bool bShouldPause = false, bool bAllowLocal = false);

	void ArcadeCreateProcess(std::string executable, std::string executableDirectory, std::string masterCommands);
	
	//void Run();
	void RunAArcade();	// initializes AArcade's loading of libraries and stuff.
	void ShowConnect(std::string lobbyId);	// shows the connect menu

	void HudStateNotify();
	void SetSlaveScreen(std::string objectId, bool bVal);

	bool CompareLoadedFromKeyValuesFileId(const char* testId, const char* baseId);

	void Feedback(std::string type);
	void Popout(std::string popoutId, std::string auxId = "", bool bQuietRun = false);
	void CreateHammerProject(std::string projectName);
	void OpenHammerProject(std::string projectName);
	void ShowRawMainMenu();//bool bForce p= false
	void GetAllHammerProjects(std::vector<std::string> &projects);
	std::string GetHammerProjectMapID(std::string projectName);
	void MapTransition(std::string mapfile, std::string spawnEntityName = "", std::string screenshot = "", std::string lobby = "", std::string lobbytitle = "", std::string instance = "", std::string pos = "", std::string rot = "");
	//void DoMapTransition(std::string mapfile, std::string spawnEntityName);

	void PlaySound(std::string file);
	void SetModelSequence(C_PropShortcutEntity* pShortcut, std::string sequenceName);
	void SetModelSkin(C_PropShortcutEntity* pShortcut, int iSkin);

	void DetectAllStickerPNGs();
	KeyValues* GetAllStickersKV();

	void NotifyGameSchemaFetched(std::string responseText);

	void HardPause();
	void WakeUp();
	void TaskClear();
	void TaskRemember(C_PropShortcutEntity* pShortcutIn = null);
	void ShowTaskMenu(bool bForceTaskTab = false);
	void HideTaskMenu();
	void ShowMouseMenu();
	void HideMouseMenu();
	void ObsoleteLegacyCommandReceived();

	bool ShouldTextureClamp();

	void StartHoldingPrimaryFire();
	void StopHoldingPrimaryFire();

	void MinimizeAArcade();
	void EndTempFreeLook();

	object_t* GetLastNearestObjectToPlayerLook() { return m_pLastNearestObjectToPlayerLook; }

	void ManualPause();
	void ShowVehicleMenu();
	void ShowScreenshotMenu();
	void HideScreenshotMenu();
	bool TakeScreenshot(bool bCreateBig = false, std::string id = "", bool bCreateInteractiveViewer = false);
	void TeleportToScreenshot(std::string id, bool bDeactivateInputMode = true);

	void ShowBulkImportList(std::string listId);
	void ShowFavoritesMenu();
	void ShowCommandsMenu();
	void ShowPaintMenu();
	void ShowPlayersMenu();
	void ShowPetsMenu();

	// TRY TO KEEP THESE IN CHRONOLOGICAL ORDER, AT LEAST FOR THE STARTUP SEQUENCE!
	void Disconnect();
	void AnarchyStartup();
	void OnWebManagerReady();
	void OnWorkshopManagerReady();
	void OnMountAllWorkshopsComplete();
	void OnDetectAllMapsComplete();

	void ShowSteamGrid();

	void CreateArcadeCrosshair();
	void DestroyArcadeCrosshair();
	bool ShouldShowCrosshair() { return m_pShouldShowCrosshairConVar->GetBool(); }

	bool OnSteamBrowserCallback(unsigned int unHandle);

	//void OnLoadingManagerReady();
	bool AttemptSelectEntity(C_BaseEntity* pTargetEntity = null, bool bIgnoreSlave = false);
	bool SelectEntity(C_BaseEntity* pEntity);
	bool DeselectEntity(std::string nextUrl = "", bool bCloseInstance = true);
	void AddGlowEffect(C_BaseEntity* pEntity);
	void RemoveGlowEffect(C_BaseEntity* pEntity);

	void AddHoverGlowEffect(C_BaseEntity* pEntity);
	void RemoveLastHoverGlowEffect();

	std::string GetAutoInspectImageFlags();

	void ShowFileBrowseMenu(std::string browseId);// const char* keyFieldName, KeyValues* itemKV);
	void OnBrowseFileSelected(std::string browseId, std::string response);
	//void ReleaseFileBrowseParams();

	void ScanForLegacySaveRecursive(std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack);

	void ActivateObjectPlacementMode(C_PropShortcutEntity* pShortcut, const char* mode = "spawn", bool bGamepadInputMode = false);
	void DeactivateObjectPlacementMode(bool confirm);

	void ShowHubSaveMenuClient(C_PropShortcutEntity* pInfoShortcut);
	void ShowNodeManagerMenu();
	void ShowEngineOptionsMenu();

	//void BackwardsMinsMaxsModel(int iEntityIndex);

	void OnVRSpazzFixCreated(int iIndex0, int iIndex1);

	int GetNumImagesLoading();
	void ImagesDoneLoading();
	void DoneSpawningInitialObjects();
	void DoneLoadingInitialObjectTextures();

	std::string GetSteamGamesCode(std::string requestId);

	bool IsYouTube(std::string url);
	void Panoshot();
	void GetPeakAudio();
	void TakeMediaScreenshot();
	void ManagePanoshot();

	float GetVRGestureDist();
	float GetVRGestureValue() { return m_flGestureValue; }

	void VRGamepadInputPostProcess();
	
	void UpdateSystemTimeState();
	systemTimeState_t GetSystemTimeState() { return m_systemTimeState; }

	void UpdateNumberStatsState(int iNumberStatType = -1);
	numberStatsState_t GetNumberStatsState() { return m_numberStatsState; }

	void OnConnectionMetricsUpdate(std::vector<int> metrics);

	void ShmotimeJavaScriptInject(std::string text);
	void SteamworksBrowserJavaScriptInject(std::string text);	// To play audio from the HUD in the AAI Steamworks tab, if it exists.
	void SteamTalker(std::string text, std::string voice, float flPitch, float flRate, float flVolume);
	void OnWebResponse(std::string responseText);
	void OnAIChatBotResponse(std::string responseText);
	void OnAIChatBotSpeakStart(std::string botType);
	void OnAIChatBotSpeakEnd(std::string botType);

	// helpers
	void GenerateUniqueId(char* result);
	//long long DecodeTimestampFromId(std::string id);
	const char* GenerateUniqueId();
	const char* GenerateUniqueId2();
	std::string ExtractLegacyId(std::string itemFile, KeyValues* item = null);
	const char* GenerateLegacyHash(const char* text);
	const char* GenerateCRC32Hash(const char* text);
	void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters);
	std::string C_AnarchyManager::encodeURIComponent(const std::string &s);
	//std::string decodeURIComponent(const std::string &s);

	//void SetNextInstanceId(std::string instanceId) { m_nextInstanceId = instanceId; }

	void SetSystemCursor(unsigned long cursor = vgui::dc_arrow, std::string cursorName = "");
	unsigned long GetSystemCursor() { return m_cursor; }

	bool DetermineLibretroCompatible(KeyValues* pItemKV, std::string& gameFile, std::string& coreFile);
	bool DetermineStreamCompatible(KeyValues* pItemKV);
	bool DeterminePreviewCompatible(KeyValues* pItemKV);

	void WriteBroadcastGame(std::string gameTitle);
	//void xCastSetGameName();
	void xCastSetLiveURL();
	void TestSQLite();
	void TestSQLite2();

	void SelectNext();
	void SelectPrev();

	pet_t* m_pPlayAsPet;
	std::string NormalizeModelFilename(std::string modelFile);
	std::vector<pet_t*> GetAllLivingPets();
	std::vector<std::string> GetAllPetOutfits(std::string modelFile);
	pet_t* FindPetByModel(std::string modelFile);

	void UpdateItemTextOnObjects(std::string itemId, std::string text);
	void BotCheer(std::string text = "");

	void ResetStatTimers();

	bool ShouldShowWindowsTaskBar();

	void SetNextLoadInfo(std::string instanceId = "", std::string position = "", std::string rotation = "");
	std::string GetHomeURL();
	void ClearConnectedUniverse();

	void WaitForOverviewExtract();
	void ManageExtractOverview();
	void ManagePets();
	void SetPetTargetPos(Vector pos);
	void TogglePetTargetPos(Vector pos);

	void PerformAutoSave(saveModeType_t saveMode = SAVEMODE_NONE, bool bDoSaveInstance = false, bool bOnlyNew = false);
	void IncrementHueShifter(float frametime);

	float GetIPD();
	float GetLastFOV();
	void SetLastFOV(float fValue);

	ITexture* CreateModelPreviewRenderTarget(IMaterialSystem* pMaterialSystem = null);
	ITexture* GetModelPreviewRenderTarget();

	void PrecacheModel(std::string file);
	std::string CreateModelPreview(std::string givenModelName, bool bOverwrite = false);
	int GetModelThumbSize();
	void TravelByScreenshot(std::string screenshotId);

	ITexture* GetVRSpectatorTexture();
	bool GetVRHMDRender() { return m_pVRHMDRenderConVar->GetBool();	}

	void Join(std::string lobby);
	void RefreshImages(std::string itemId, std::string modelId);

	std::string ModifyMaterial(std::string materialName, std::string materialVar, std::string value);
	bool ActionBarUse(int iSlot);

	void DoComparisonRender();
	void OnComparisonRenderFinished();
	float GetComparisonRenderAmount();

	void MakeRagdoll(C_PropShortcutEntity* pShortcut);
	void RagdollInfo(C_PropShortcutEntity* pShortcut);

	void FetchGlobalStats();

	void StartQuestsSoon();

	void LocalPlayerSpawned();
	void LocalPlayerDied();

	C_PropShortcutEntity* GetInspectShortcut();
	void ActivateInspectObject(C_PropShortcutEntity* pShortcut);
	void DeactivateInspectObject();
	void InspectModeTick(float flFrameTime);
	void ApplyCarryData(std::string origin);

	void PlaySequenceRegularOnProp(C_DynamicProp* pProp, const char* sequenceTitle);
	void OnHotlinkDraw(C_BaseAnimating* pBaseAnimating, bool bWasDrawn);

	//void SetForegroundShortcut(C_PropShortcutEntity* pShortcut);
	//C_PropShortcutEntity* GetForegroundShortcut() { return m_pForegroundShortcut; }

	Vector GetPetStaggeredPosition(unsigned int u, const Vector &vTargetPos, const QAngle &qTargetRot, pet_t* pPet, float minDist);
	void ClearPetStaggeredPositions();
	void CycleStaggerPattern(int iDirection = 1);

	void SetNextTaskScreenshot(std::string filepath);
	std::string GetNextTaskScreenshot();

	void PerformAutoScreenshot();

	// accessors
	aaPetStaggerPattern GetCurrentStaggerPattern() { return m_eStaggerPattern; }
	std::string GetFailedModelThumbName() { return failedModelName; }
	ISourceVirtualReality::VREye GetEye() { return m_eye; }
	int GetNoDrawShortcutsValue();
	int GetSelectorTraceEntityIndex() { return m_iSelectorTraceEntityIndex; }
	Vector GetSelectorTraceVector() { return m_selectorTraceVector; }
	Vector GetSelectorTraceNormal() { return m_selectorTraceNormal; }
	int GetVRSpazzFix(int iIndex);
	int GetSteamPlayerCountNumber();
	ConVar* GetYouTubeEndBehaviorConVar() { return m_pYouTubeEndBehaviorConVar; }
	ConVar* GetYouTubePlaylistBehaviorConVar() { return m_pYouTubePlaylistBehaviorConVar; }
	ConVar* GetYouTubeVideoBehaviorConVar() { return m_pYouTubeVideoBehaviorConVar; }
	ConVar* GetYouTubeRelatedConVar() { return m_pYouTubeRelatedConVar; }
	ConVar* GetYouTubeMixesConVar() { return m_pYouTubeMixesConVar; }
	ConVar* GetYouTubeAnnotationsConVar() { return m_pYouTubeAnnotationsConVar; }
	int GetLastNextDirection() { return m_iLastNextDirection; }
	int GetSelectOriginal() { return m_iOriginalSelect; }
	VMatrix GetVRLeftEyeMatrix() { return m_VRLeftEyeProjectionMatrix; }
	VMatrix GetVRRightEyeMatrix() { return m_VRRightEyeProjectionMatrix; }
	bool IsVRActive() { return m_bVRActive; }
	bool IsHandTrackingActive() { return m_bHandTrackingActive; }
	VMatrix GetVRHeadMatrix() { return m_VRHeadMatrix; }
	float GetHueShifter() { return m_fHueShifter; }
	float GetAudioPeakValue() { return m_fAudioPeak; }
	bool GetAutoSave();
	std::string GetCurrentLobby() { return m_currentLobby; }
	std::string GetCurrentLobbyTitle() { return m_currentLobbyTitle; }
	//std::string GetNextLobby() { return m_nextLobby; }
	bool GetSuspendEmbedded() { return m_bSuspendEmbedded; }
	bool IsInitialized() { return m_bInitialized; }
	aaState GetState() { return m_state; }
	std::string GetInstanceId() { return m_instanceId; }
	//std::string GetNextInstanceId() { return m_nextInstanceId; }
	nextLoadInfo_t* GetNextLoadInfo() { return m_pNextLoadInfo; }
	C_InputManager* GetInputManager() { return m_pInputManager; }
	//C_WebManager* GetWebManager() { return m_pWebManager; }
	//C_LoadingManager* GetLoadingManager() { return m_pLoadingManager; }
	C_CanvasManager* GetCanvasManager() { return m_pCanvasManager; }
	C_SteamBrowserManager* GetSteamBrowserManager() { return m_pSteamBrowserManager; }
	C_WindowManager* GetWindowManager() { return m_pWindowManager; }
	C_QuestManager* GetQuestManager() { return m_pQuestManager; }
	C_AwesomiumBrowserManager* GetAwesomiumBrowserManager() { return m_pAwesomiumBrowserManager; }
	//C_AwesomiumBrowserManager* GetAwesomiumBrowserManager() { return m_pAwesomiumBrowserManager; }
	C_LibretroManager* GetLibretroManager() { return m_pLibretroManager; }
	C_MountManager* GetMountManager() { return m_pMountManager; }
	C_WorkshopManager* GetWorkshopManager() { return m_pWorkshopManager; }
	C_MetaverseManager* GetMetaverseManager() { return m_pMetaverseManager; }
	C_AAIManager* GetAAIManager() { return m_pAAIManager; }
	C_BackpackManager* GetBackpackManager() { return m_pBackpackManager; }
	C_InstanceManager* GetInstanceManager() { return m_pInstanceManager; }
	C_BaseEntity* GetSelectedEntity() { return m_pSelectedEntity; }
	//ThreadedFileBrowseParams_t* GetFileBrowseParams() { return m_pFileParams; }
	std::string GetLegacyFolder() { return m_legacyFolder; }
	std::string GetWorkshopFolder() { return m_workshopFolder; }
	std::string GetAArcadeUserFolder() { return m_aarcadeUserFolder; }
	std::string GetAArcadeToolsFolder() { return m_aarcadeToolsFolder; }
	std::string GetOldEngineNoFocusSleep() { return m_oldEngineNoFocusSleep; }
	C_BaseEntity* GetLastHoverGlowEntity() { return m_pHoverGlowEntity; }
	std::string GetTabMenuFile() { return m_tabMenuFile; }
	//bool GetIgnoreNextFire() { return m_bIgnoreNextFire; }
	aampConnection_t* GetConnectedUniverse() { return m_pConnectedUniverse; }
	panoshotState GetPanoshotState() { return m_panoshotState; }
	bool GetRightFreeMouseToggle() { return m_pRightFreeMouseToggleConVar->GetBool(); }
	bool GetAutoCloseTasks() { return m_pAutoCloseTasksConVar->GetBool(); }
	bool GetIsDisconnecting() { return m_bIsDisconnecting; }	// NOTE: This bool is only properly set if AArcade's Disconnect method is used!! (an attempt to set it at map unload too, but that may be too late.
	bool GetSocialMode() { return m_bSocialMode; }
	bool IsInSourceGame() { return m_bIsInSourceGame; }
	int VRSpectatorMode() { return m_pVRSpectatorModeConVar->GetInt(); }
	int SpectatorMode() { return 0; }// m_pSpectatorModeConvar->GetInt();}
	bool VRSpectatorMirrorMode() { return m_pVRSpectatorMirrorModeConVar->GetBool(); }

	// mutators
	void SetSuspendEmbedded(bool bValue) { m_bSuspendEmbedded = bValue; };

	void SetEye(ISourceVirtualReality::VREye eye) { m_eye = eye; }
	void SetRightFreeMouseToggle(bool bVal) { m_pRightFreeMouseToggleConVar->SetValue(bVal); }
	void SetLastNextDirection(int iValue) { m_iLastNextDirection = iValue; }
	void SetSelectOriginal(int iEntityIndex);
	void SetNextLobby(std::string lobbyId, std::string title) { m_nextLobby = lobbyId; m_nextLobbyTitle = title; }
	void SetSaveMode(saveModeType_t saveMode) { m_saveMode = saveMode; }
	void SetLastNearestObjectToPlayerLook(object_t* pObject) { m_pLastNearestObjectToPlayerLook = pObject; }
	void SetInitialized(bool bValue) { m_bInitialized = bValue; }
	void SetState(aaState state) { m_state = state; }
	void IncrementState();
	void SetLegacyFolder(std::string val) { m_legacyFolder = val; }
	void SetWorkshopFolder(std::string val) { m_workshopFolder = val; }
	void SetAArcadeUserFolder(std::string val) { m_aarcadeUserFolder = val; }
	void SetAArcadeToolsFolder(std::string val) { m_aarcadeToolsFolder = val; }
	void SetOldEngineNoFocusSleep(std::string val) { m_oldEngineNoFocusSleep = val; }
	void SetTabMenuFile(std::string url) { m_tabMenuFile = url; }
	//void SetIgnoreNextFire(bool bValue) { m_bIgnoreNextFire = bValue; }
	void SetConnectedUniverse(bool bConnected, bool bIsHost, std::string address, std::string universeId, std::string instanceId, std::string sessionId, std::string lobbyId, bool bPublic, bool bPersistent, std::string lobbyPassword);

	void ConvertAndPaint(std::string fileLocation);

	void SetImagesReady(bool bValue) { m_bImagesReady = bValue; }
	bool GetImagesReady() { return m_bImagesReady; }

	void SetImagesResetting(bool bValue) { m_bImagesResetting = bValue; }
	bool GetImagesResetting() { return m_bImagesResetting; }

	C_Accountant* GetAccountant() { return m_pAccountant; }

	CCallResult<C_AnarchyManager, HTML_BrowserReady_t> m_SteamPlayerCountReceived;

	void HTTPResponse(HTTPRequestCompleted_t *pResult, bool bIOFailure);
	CCallResult<C_AnarchyManager, HTTPRequestCompleted_t> m_HTTPResponseCallback;

	void DownloadFileResponse(HTTPRequestCompleted_t *pResult, bool bIOFailure);
	CCallResult<C_AnarchyManager, HTTPRequestCompleted_t> m_DownloadFileCallback;

	void SteamHTTPImageDownloadResponse(HTTPRequestCompleted_t *pResult, bool bIOFailure);
	CCallResult<C_AnarchyManager, HTTPRequestCompleted_t> m_SteamHTTPImageDownloadCallback;

	void SteamHTTPImageDownloadHeaderCheck(HTTPRequestCompleted_t *pResult, bool bIOFailure);
	CCallResult<C_AnarchyManager, HTTPRequestCompleted_t> m_SteamHTTPImageDownloadHeaderCheck;

	//void SteamHTTPImageDownloadHeadersReceived(HTTPRequestHeadersReceived_t *pResult);
	//CCallResult<C_AnarchyManager, HTTPRequestHeadersReceived_t> m_SteamHTTPImageDownloadHeadersReceived;

	//LRESULT CALLBACK HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void InitDragDrop(HWND hWnd);
	void HandleDragDrop(HDROP hDrop);
	void HandleTextDragDrop(std::string text);
	bool DragEnter();
	void DragLeave();

	HWND GetHWND() { return m_hwnd; }

	WNDPROC GetOriginalWndProc() { return m_pOriginalWndProc; }

	bool ShouldAllowMultipleActive();

	bool GetAttractMode() { return m_pAttractModeActiveConVar->GetBool(); }
	bool GetCabinetAttractMode() { return m_pCabinetAttractModeActiveConVar->GetBool(); }
	bool GetCamcutAttractMode() { return m_pCamcutAttractModeActiveConVar->GetBool(); }

protected:
	void ScanForLegacySave(std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack);

private:
	std::map<uint64, steamHTTPImageDownload_t*> m_steamHTTPImageDownloadRequests;

	std::string m_nextTaskScreenshot;

	Vector AdjustForMinDist(Vector vPos, float minDist);
	Vector ComputeSingleFilePosition(unsigned int u, const Vector &vTargetPos, const QAngle &qTargetRot);
	Vector ComputeThemeParkLinePosition(unsigned int u, const Vector &vTargetPos, const QAngle &qTargetRot);
	Vector ComputeCirclePosition(unsigned int u, const Vector &vTargetPos);
	Vector ComputeSemiCirclePosition(unsigned int u, const Vector &vTargetPos, const QAngle &qTargetRot);
	Vector ComputeVShapePosition(unsigned int u, const Vector &vTargetPos, const QAngle &qTargetRot);
	Vector ComputeRandomBlobPosition(unsigned int u, const Vector &vTargetPos);
	aaPetStaggerPattern m_eStaggerPattern;// Current stagger pattern
	std::vector<Vector> m_vPetPositions;// Stores positions of pets for collision avoidance

	Vector m_previousPetTargetPos;

	ConVar* m_pFixedCameraMaxDistConVar;
	ConVar* m_pFixedCameraMinDistConVar;
	std::string m_lastBestNonVRSpectateScreenshotId;
	ConVar* m_pFixedCameraSpectateModeConVar;
	std::vector<pet_t*> m_pets;
	Vector m_petTargetPos;

	// All of the nextPet variables must be set together or risk garbage.
	std::string m_nextPetModel;
	std::string m_nextPetRun;
	std::string m_nextPetWalk;
	std::string m_nextPetIdle;
	std::string m_nextPetFall;
	float m_flNextPetScale;
	std::string m_nextPetPos;
	std::string m_nextPetRot;
	float m_flNextPetNear;
	float m_flNextPetFar;
	float m_flNextPetRunSpeed;
	float m_flNextPetWalkSpeed;
	std::string m_nextPetOutfit;
	std::string m_nextPetBehavior;
	std::string m_nextPetSequence;

	KeyValues* m_pPendingPetsRestoreKV;

	ConVar* m_pJoystickConVar;
	C_PropShortcutEntity* m_pInspectShortcut;
	QAngle m_inspectStartingAngles;
	Vector m_inspectStartingOrigin;
	float m_flInspectStartingScale;
	//Vector m_inspectGoodOrigin;
	float m_flInspectGoodOriginDist;
	Vector m_inspectCenterOffset;
	Vector m_inspectCenterOffsetFudge;
	QAngle m_inspectGoodAngles;
	Vector m_inspectOffsetOrigin;
	QAngle m_inspectOffsetAngles;
	float m_flInspectOffsetScale;
	float m_flInspectGoodScale;
	float m_flInspectPreviousScale;

	int m_iOldAlwaysAnimatingImagesValue;
	ConVar* m_pAlwaysAnimatingImagesConVar;

	float m_flStartQuestsSoon;

//	C_SteamBrowserInstance* m_pTalkerSteamBrowserInstance;

	float m_flOldZNear;
	ConVar* m_pInspectModelIdConVar;

	float m_flLastGlobalStatFetchTime;
	float m_flNextGlobalPlayerCountUpdateTime;
	float m_flNextGlobalStatRefreshTime;

	C_PropShortcutEntity* m_pRagdollShortcut;
	float m_flNextSystemTimeUpdateTime;
	systemTimeState_t m_systemTimeState;
	float m_flNextNumberStatsUpdateTime;
	numberStatsState_t m_numberStatsState;
	float m_flComparisonRenderTime;
	ConVar* m_pCamcutAttractModeActiveConVar;
	ConVar* m_pCabinetAttractModeActiveConVar;
	ConVar* m_pAttractModeActiveConVar;
	float m_flNextAttractCameraTime;
	std::string m_attractModeScreenshotId;

	float m_flSpawnObjectsButtonDownTime;

	ConVar* m_pShouldAllowMultipleActiveConVar;
	std::string failedModelName;

	//FileHandle_t m_crashModelLogHandle;
	bool m_bPrecacheInstances;
	bool m_bBatchObjectSpawn;

	bool m_bWasVRSnapTurn;
	bool m_bWasVRTeleport;
	bool m_bNeedVRTeleport;
	int m_iVRSnapTurn;
	ConVar* m_pActionBarSlotConVars[10];

	CCallResult<C_AnarchyManager, NumberOfCurrentPlayers_t> m_GetNumberOfPlayersHelper;
	CCallResult<C_AnarchyManager, GlobalStatsReceived_t> m_RequestGlobalStatsHelper;

	ConVar* m_pNoDrawShortcutsConVar;
	ConVar* m_pPaintTextureConVar;
	ConVar* m_pLocalAutoPlaylistsConVar;

	ISourceVirtualReality::VREye m_eye;

	//IMaterial* m_pAudioStripMaterial;
	ConVar* m_pAVRConVar;
	ConVar* m_pAVRAmpConVar;
	Color	m_hueColor;

	C_DynamicProp* m_pLocalAvatarObject;

	//bool m_bVRSpectatorMode;
	bool m_bIsCreatingLocalAvatar;
	ConVar* m_pVRSpectatorModeConVar;
	//ConVar* m_pSpectatorModeConVar;
	ConVar* m_pVRSpectatorMirrorModeConVar;
	ConVar* m_pVRHMDRenderConVar;

	ConVar* m_pAutoRebuildSoundCacheConVar;
	//ITexture* m_pVRSpectatorCameraTexture;

	bool m_bVRSuspending;

	int m_iPlayerCount;
	//HCURSOR m_hCursor;
	//std::map<const TCHAR*, HCURSOR> m_systemCursors;
	unsigned long m_cursor;

	float m_flStartTime;
	float m_flCurrentTime;
	float m_flStatTime;
	int m_iTotalHours;

	int m_iLastNextDirection;

	ConVar* m_pModelThumbSizeConVar;
	C_DynamicProp* pModelPreview;

	ConVar* m_pYouTubeEndBehaviorConVar;
	ConVar* m_pYouTubePlaylistBehaviorConVar;
	ConVar* m_pYouTubeVideoBehaviorConVar;
	ConVar* m_pYouTubeRelatedConVar;
	ConVar* m_pYouTubeMixesConVar;
	ConVar* m_pYouTubeAnnotationsConVar;

	int m_iVRSpazzFix;//C_BaseEntity*
	int m_iVRSpazzFix2;

	int m_iSelectorTraceEntityIndex;
	Vector m_selectorTraceVector;
	Vector m_selectorTraceNormal;

	C_Accountant* m_pAccountant;
	C_DynamicProp* pVRHandRight;
	VMatrix m_VRGestureLeft;
	C_DynamicProp* pVRPointerRight;
	C_DynamicProp* pVRHandLeft;
	VMatrix m_VRGestureRight;
	C_DynamicProp* pVRPointerLeft;
	C_DynamicProp* pVRTeleport;
	float m_flGestureValue;
	std::vector<C_PropShortcutEntity*> m_alwaysLookObjects;

	ConVar* m_pWaitForInitialImagesConVar;

	int m_iOriginalSelect;

	CTextureReference m_modelPreviewRenderTarget;
	ITexture* m_pModelPreviewRenderTexture;

	//CTextureReference m_VRTwoEyesHMDRenderTarget;
	CTextureReference m_VRSpectatorRenderTarget;
	ITexture* m_pVRSpectatorRenderTexture;

	CTextureReference m_VRTwoEyesHMDRenderTargets[3];
	ITexture* m_pVRTwoEyesHMDRenderTextures[3];

	int m_iVRAPI;
	int m_iVRBufferWidth;
	int m_iVRBufferHeight;
	ConVar* m_pDebugInvertVRMatricesConVar;
	KeyValues* m_pNextBigScreenshotKV;
	bool m_bWaitingForInitialImagesToLoad;

	bool m_bVRActive;
	bool m_bHandTrackingActive;
	//float *m_flVRLeftEyeMatrix;
	//float *m_flVRRightEyeMatrix;
	VMatrix m_VRLeftEyeProjectionMatrix;
	VMatrix m_VRRightEyeProjectionMatrix;
	VMatrix m_VRHeadMatrix;
	VMatrix m_VRLeftControllerMatrix;
	VMatrix m_VRRightControllerMatrix;

	std::string m_lastLaunchedItemId;

	bool m_bPreviousJoystickClickDown;
	float m_flPreviousJoystickForward;
	float m_flPreviousJoystickSide;
	float m_flPreviousJoystickPitch;
	float m_flPreviousJoystickYaw;
	ConVar* m_pLastFOVConVar;
	ConVar* m_pIPDConVar;
	ConVar* m_pUseSBSRenderingConVar;

	float m_fHueShifter;
	float m_fAudioPeak;
	void* m_pMeterInfo;
	bool m_bIsInSourceGame;
	bool m_bImagesResetting;
	bool m_bImagesReady;
	bool m_bLevelInitialized;
	WNDPROC m_pOriginalWndProc;
	bool m_bSocialMode;
	KeyValues* m_pSourceEngineGamesKV;
	std::string m_currentLobby;
	std::string m_currentLobbyTitle;
	std::string m_nextLobby;
	std::string m_nextLobbyTitle;
	saveModeType_t m_saveMode;
	ConVar* m_pAutoSaveConVar;
	HWND m_hwnd;
	ConVar* m_pTextureClampConVar;
	KeyValues* m_pAllStickersKV;
	ConVar* m_pAutoCloseTasksConVar;
	ConVar* m_pRightFreeMouseToggleConVar;
	ConVar* m_pShouldShowWindowsTaskBarConVar;
	ConVar* m_pFreeMouseModeConVar;
	ConVar* m_pShouldShowCrosshairConVar;
	panoStuff_t* m_pPanoStuff;
	float m_fNextExtractOverviewCompleteManage;
	float m_fNextPanoCompleteManage;
	std::vector<std::string> m_existingMapScreenshotsForPano;
	panoshotState m_panoshotState;
	float m_fNextWindowManage;
	bool m_bAutoRes;
	ConVar* m_pBuildGhostConVar;
	aampConnection_t* m_pConnectedUniverse;
	unsigned int m_uProcessBatchSize;
	unsigned int m_uProcessCurrentCycle;
	unsigned int m_uValidProcessedModelCount;
	unsigned int m_uLastProcessedModelIndex;
	unsigned int m_uPreviousImportCount;
	importInfo_t* m_pImportInfo;
	ConVar* m_pHoverTitlesConVar;
	ConVar* m_pToastMsgsConVar;
	std::string m_hoverTitle;
	int m_iHoverEntityIndex;
	bool m_bHoverEntityShortcut;
	float m_fHoverTitleExpiration;
	vgui::Label* m_pHoverLabel;
	float m_fNextToastExpiration;
	KeyValues* m_pToastMessagesKV;
	std::vector<vgui::Label*> m_toastLabels;
	std::string m_toastText;
	//bool m_bIgnoreNextFire;
	bool m_bIsDisconnecting;
	ConVar* m_pWeaponsEnabledConVar;
	std::string m_tabMenuFile;
	int m_iLastDynamicMultiplyer;
	ConVar* m_pPicMipConVar;
	object_t* m_pLastNearestObjectToPlayerLook;
	C_BaseEntity* m_pHoverGlowEntity;
	bool m_bIsShuttingDown;
	bool m_bIsHoldingPrimaryFire;
	std::map<std::string, bool> m_specialInstances;
	//ThreadedFileBrowseParams_t* m_pFileParams;
	//ThreadedFolderBrowseParams_t* m_pFolderParams;

	nextLoadInfo_t* m_pNextLoadInfo;

	int m_iLookspotIndex;
	int m_iLookspotHaloIndex;
	int m_iDirIndex;

	bool m_bSuspendEmbedded;
	bool m_bInitialized;
	bool m_bIncrementState;
	aaState m_state;
	bool m_bPaused;
	std::string m_instanceId;
	//std::string m_nextInstanceId;
	int m_iState;
	double m_dLastGenerateIdTime;
	std::string m_lastGeneratedChars;

	C_CanvasManager* m_pCanvasManager;
	//C_WebManager* m_pWebManager;
	//C_LoadingManager* m_pLoadingManager;
	C_LibretroManager* m_pLibretroManager;
	C_SteamBrowserManager* m_pSteamBrowserManager;
	C_WindowManager* m_pWindowManager;
	C_QuestManager* m_pQuestManager;
	C_AwesomiumBrowserManager* m_pAwesomiumBrowserManager;
	C_InputManager* m_pInputManager;
	C_MountManager* m_pMountManager;
	C_WorkshopManager* m_pWorkshopManager;
	C_MetaverseManager* m_pMetaverseManager;
	C_BackpackManager* m_pBackpackManager;
	C_InstanceManager* m_pInstanceManager;
	C_AAIManager* m_pAAIManager;

	C_BaseEntity* m_pSelectedEntity;

	//C_PropShortcutEntity* m_pForegroundShortcut;

	std::string m_aarcadeUserFolder;
	std::string m_aarcadeToolsFolder;
	std::string m_legacyFolder;
	std::string m_workshopFolder;
	std::string m_oldEngineNoFocusSleep;

	ConVar* m_pInspectYawConVar;
	ConVar* m_pInspectPitchConVar;
	ConVar* m_pInspectHorizConVar;
	ConVar* m_pInspectVertConVar;
	ConVar* m_pInspectTallConVar;
};

extern C_AnarchyManager* g_pAnarchyManager;

#endif