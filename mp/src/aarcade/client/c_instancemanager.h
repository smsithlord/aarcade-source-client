#ifndef C_INSTANCE_MANAGER_H
#define C_INSTANCE_MANAGER_H

#include "KeyValues.h"
//#include <map>
#include <vector>
#include <string>
#include "c_prop_shortcut.h"
#include "c_backpack.h"

struct object_t
{
	std::string objectId;
	unsigned int created;
	std::string owner;
	unsigned int removed;
	std::string remover;
	unsigned int modified;
	std::string modifier;
	std::string itemId;
	std::string modelId;
	std::string anim;
	Vector origin;
	QAngle angles;
	bool spawned;
	bool child;
	float scale;
	bool slave;
	int body;
	int skin;
	int entityIndex;
	int parentEntityIndex;
	int state;
	std::string twitchChannel;
	//C_BaseEntity* entity;
};

struct instance_t
{
	std::string id;
	std::string mapId;
	std::string title;
	std::string style;
	std::string file;
	std::string workshopIds;
	std::string mountIds;
	std::string autoplayId;
	//std::string backpackId;
	int legacy;
};

struct transform_t
{
	float offX;
	float offY;
	float offZ;
	float rotP;
	float rotY;
	float rotR;
	float scale;
	bool pseudo;
};

class C_InstanceManager
{
public:
	C_InstanceManager();
	~C_InstanceManager();

	void LevelShutdownPostEntity();
	void LevelShutdownPreEntity();

	void Update();

	KeyValues* LoadInstance(C_PropShortcutEntity* pParentNodeEntity, std::string instanceId, std::string position = "", std::string rotation = "", bool bDoNotSpawn = false);
	void LoadLegacyInstance(std::string instanceId, KeyValues* instanceKV);

	void ApplyChanges(C_PropShortcutEntity* pShortcut, bool bShouldSave = true, std::string objectIdOverride = "", std::string itemIdOverride = "");
	void SaveActiveInstance(KeyValues* pInstanceKV = null, bool bForceNoSave = false);	// most useful to call this after applying bulk changes, but also gets called after individual changes.
	void ResetObjectChanges(C_PropShortcutEntity* pShortcut);

	void ModelFileChanged(std::string modelId);

	void SetMaterialMod(std::string material, std::string materialVarName, std::string materialVarValue, bool bDoSave = true);
	void ClearMaterialMod(std::string material, bool bDoSave = true);
	void ClearAllMaterialMods();
	void ApplyMaterialMod(std::string material, std::string materialVarName, std::string materialVarValue);
	void UnapplyMaterialMod(std::string material);// , std::string materialVarName, std::string materialVarValue);
	
	void ApplyAllMaterialMods();
	void ResetAllMaterialMods();

	transform_t* GetTransform() { return m_pTransform; }
	void ResetTransform();
	void AdjustObjectOffset(float x, float y, float z);
	void AdjustObjectRot(float p, float y, float r);
	void AdjustObjectScale(float scale);
	void AdjustTransformPseudo(bool bValue);

	void CacheAllInstanceModels();

	void SetObjectEntity(std::string objectId, C_BaseEntity* pEntity);
	C_BaseEntity* GetObjectEntity(std::string objectId);

	object_t* GetNearestObjectToPlayerLook(object_t* pStartingObject = null, float flMaxRange = 300.0f);
	object_t* GetNearestObjectToObject(std::string mode, object_t* pOriginalObject, object_t* pCurrentObject, bool bYouTubeOnly = false, float flMaxRange = 300.0f);

	std::string GetObjectWithFile(std::string file);

	std::string CreateBlankInstance(int iLegacy = 0, KeyValues* pInstanceKV = null, std::string instanceId = "", std::string mapId = "", std::string title = "", std::string file = "", std::string workshopIds = "", std::string mountIds = "", std::string style = "", C_Backpack* pBackpack = null);
	void CreateObject(KeyValues* pObjectKV, std::string objectId, std::string itemId, std::string modelId, std::string position, std::string rotation, float scale, std::string anim, int slave, int child, int body, int skin);

	void SpawnObject(object_t* object, bool bIsAutoplayObject);
	object_t* AddObject(std::string objectId, std::string itemId, std::string modelId, Vector origin, QAngle angles, float scale, std::string anim, bool slave, int body, int skin, unsigned int created = 0, std::string owner = "", unsigned int removed = 0, std::string remover = "", unsigned int modified = 0, std::string modifier = "", bool isChild = false, int entindex = -1);
	object_t* GetInstanceObject(std::string objectId);
	unsigned int GetInstanceObjectCount();
	void RemoveEntity(C_PropShortcutEntity* pShortcutEntity, bool bBulkRemove = false, std::string objectOverrideId = "");
	bool SpawnNearestObject();
	//void SetNearestSpawnDist(double maxDist) { m_fNearestSpawnDist = (float)m_fNearestSpawnDist = maxDist; }

	void SetNearestSpawnDistFast(float flMaxDist);
	int SetTempNearestSpawnDist(double maxDist);
	int SetNearestSpawnDist(double maxDist);	// returns how many unspawned objects are within that dist
	float GetNearestSpawnDist() { return m_fNearestSpawnDist; };	// returns how many unspawned objects are within that dist

	void GetObjectsInSight(std::vector<object_t*> &objects);

	void AddInstance(int iLegacy, std::string instanceId, std::string mapId, std::string title, std::string file = "", std::string workshopIds = "", std::string mountIds = "", std::string autoplayId = "", std::string style = "");
	instance_t* GetInstance(std::string id);
	instance_t* FindInstance(std::string instanceId);
	instance_t* GetCurrentInstance();
	KeyValues* GetCurrentInstanceKV() { return m_pInstanceKV; }
	void FindAllInstances(std::string mapId, std::vector<instance_t*> &instances);
	void LegacyMapIdFix(std::string legacyMapName, std::string mapId);

	bool ConsumeLegacyInstance(std::string instanceId, std::string filename, std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack);

	object_t* GetObjectUnderPlayerAim();
	void AssignObjectItem(std::string objectId, std::string itemId);

	void CreateNewNode(std::string nodeName, C_PropShortcutEntity* pNodeEntity = null, std::string nodeItemId = "");
	void ClearNodeSpace();
	void SpawnActionPressed(bool bForceSpawnAll = false);
	void ChangeModel(C_BaseEntity* pEntity, std::string modelId, std::string in_modelFile, bool bMakeGhost = true, bool bReinit = false);
	//void SpawnItem(std::string id);

	bool ModelAssetReadyFirstMorph(std::string fileHash, std::string file, bool& bSkippedTarget);
	void OnModelAssetReady(std::string fileHash, std::string file);

	void RemoveInstance(instance_t* pInstance);

	void SelectNext(C_PropShortcutEntity* pSelectedEntity, C_PropShortcutEntity* pOriginalEntity);
	void SelectPrev(C_PropShortcutEntity* pSelectedEntity, C_PropShortcutEntity* pOriginalEntity);

	void ResetPhysicsOnObject(C_PropShortcutEntity* pShortcut);
	void CloneObject(C_PropShortcutEntity* pShortcut);

	void CloneInstance(std::string instanceId);
	void PlayNearestGIF();

	// accessors
	int GetUnspawnedWithinRangeEstimate() { return m_iUnspawnedWithinRangeEstimate; }
	std::string GetIncomingNodeId() { return m_incomingNodeId; }
	std::map<std::string, object_t*> GetObjectsMap() { return m_objects; }

	// mutators
	void SetIncomingNodeId(std::string nodeId) { m_incomingNodeId = nodeId; }
	
private:
	ConVar* m_pSpawnObjectsWithinViewOnlyConVar;
	ConVar* m_pSpawnObjectsDoubleTapConVar;
	ConVar* m_pNearestSpawnDistConVar;
	bool m_bHadDebugText;
	ConVar* m_pReShadeConVar;
	ConVar* m_pReShadeDepthConVar;
	ConVar* m_pVGUIConVar;
	ConVar* m_pDebugObjectSpawnConVar;
	ConVar* m_pSkipObjectsConVar;
	ConVar* m_pRecentModelIdConVar;
	std::string m_incomingNodeId;
	int m_iUnspawnedWithinRangeEstimate;
	transform_t* m_pTransform;
	KeyValues* m_pInstanceKV;
	KeyValues* m_pActiveMaterialMods;
	unsigned int m_uNextFlashedObject;
	float m_fLastSpawnActionPressed;
	float m_fNearestSpawnDist;
	std::map<std::string, object_t*> m_objects;
	std::vector<object_t*> m_unspawnedObjects;
	CUtlMap<std::string, instance_t*> m_instances;
	//std::map<std::string, instance_t*> m_instances;
};

#endif