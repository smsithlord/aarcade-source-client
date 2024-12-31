#ifndef CANARCHY_MANAGER_H
#define CANARCHY_MANAGER_H

//#include "props.h"
#include "igamesystem.h"
#include "../game/server/triggers.h"

class CAnarchyManager : public CAutoGameSystemPerFrame
{
public:
	CAnarchyManager();
	~CAnarchyManager();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void FrameUpdatePreEntityThink();

	bool IsMapLoaded() { return m_bIsMapLoaded; }

	void SetAttractModeTransform(Vector origin, QAngle angles, int iTransitionType = 0);
	void EndAttractMode();
	void OnAttractModeTransformReached();

	Vector GetAttractModeOrigin();
	QAngle GetAttractModeAngles();

	float GetAttractModeHold() { return pAttractModeHoldConVar->GetFloat(); }
	float GetAttractModeDrift() { return pAttractModeDriftConVar->GetFloat(); }
	float GetAttractModeTime() { return pAttractModeTimeConVar->GetFloat(); }

	CTriggerCamera* GetAttractCameraEntity() { return m_pAttractCameraEntity; }
	void AddJunkEntity(CBaseEntity* pEntity);
	void ClearJunkEntities();
	void RemoveJunkEntities();

	void SetTorch(CBaseEntity* pEntity);
	void DestroyTorch();

private:
	CUtlVector<CBaseEntity*> m_junkEntities;
	ConVar* pAttractModeDriftConVar;
	ConVar* pAttractModeHoldConVar;
	ConVar* pAttractModeTimeConVar;
	CTriggerCamera* m_pAttractCameraEntity;
	Vector m_attractCameraOrigin;
	QAngle m_attractCameraAngles;
	CBaseEntity* m_pTorchEntity;

	bool m_bIsMapLoaded;
};

extern CAnarchyManager* g_pAnarchyManager;

#endif