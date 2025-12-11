#include "cbase.h"
#include "canarchymanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CAnarchyManager g_AnarchyManager;
extern CAnarchyManager* g_pAnarchyManager(&g_AnarchyManager);

CAnarchyManager::CAnarchyManager() : CAutoGameSystemPerFrame("CAnarchyManager")
{
	DevMsg("(server) AnarchyManager: Constructor\n");
	m_pAttractCameraEntity = NULL;

	m_bIsMapLoaded = false;
}

CAnarchyManager::~CAnarchyManager()
{
	DevMsg("(server) AnarchyManager: Destructor\n");
}


void CAnarchyManager::LevelInitPreEntity()
{
	pAttractModeDriftConVar = cvar->FindVar("attract_mode_drift");
	pAttractModeHoldConVar = cvar->FindVar("attract_mode_hold");
	pAttractModeTimeConVar = cvar->FindVar("attract_mode_time");
}

void CAnarchyManager::LevelInitPostEntity()
{
	m_bIsMapLoaded = true;
}

void CAnarchyManager::LevelShutdownPreEntity()
{
	m_bIsMapLoaded = false;
	m_pAttractCameraEntity = NULL;
	this->ClearJunkEntities();
}

void CAnarchyManager::LevelShutdownPostEntity()
{
}

void CAnarchyManager::FrameUpdatePreEntityThink()
{

}

void CAnarchyManager::SetAttractModeTransform(Vector origin, QAngle angles, int iTransitionType)
{
	if (!this->IsMapLoaded())
		return;

	CTriggerCamera* pCameraEntity;

	m_attractCameraOrigin = origin;
	m_attractCameraAngles = angles;

	if (m_pAttractCameraEntity)
	{
		pCameraEntity = m_pAttractCameraEntity;
		pCameraEntity->UpdateAttractCameraGoal(origin, angles, iTransitionType);

		if (iTransitionType == 2)
		{
			UTIL_SetOrigin(m_pAttractCameraEntity, origin, true);
			m_pAttractCameraEntity->SetAbsAngles(angles);

			// Get the current angles
			Vector forward;
			m_pAttractCameraEntity->GetVectors(&forward, NULL, NULL);

			Vector vel = forward * cvar->FindVar("attract_mode_drift")->GetFloat();
			m_pAttractCameraEntity->Teleport(&origin, &angles, &vel);

			this->OnAttractModeTransformReached();
		}
	}
	else
	{
		CBaseEntity* pBaseEntity = CreateEntityByName("point_viewcontrol");

		pCameraEntity = dynamic_cast<CTriggerCamera*>(pBaseEntity);

		CBasePlayer* pPlayer = UTIL_GetLocalPlayer();

		// Pass in standard key values
		char buf[512];

		Vector safeOrigin = pPlayer->GetAbsOrigin();
		QAngle safeAngles = pPlayer->GetAbsAngles();

		// Pass in standard key values
		Q_snprintf(buf, sizeof(buf), "%.10g %.10g %.10g", safeOrigin.x, safeOrigin.y, safeOrigin.z);
		pCameraEntity->KeyValue("origin", buf);
		Q_snprintf(buf, sizeof(buf), "%.10g %.10g %.10g", safeAngles.x, safeAngles.y, safeAngles.z);
		pCameraEntity->KeyValue("angles", buf);

		// Modify some "Hammer" values before this is spawned.
		pCameraEntity->KeyValue("interpolatepositiontoplayer", "1");
		pCameraEntity->KeyValue("spawnflags", "92");

		//		pCameraEntity->ClearSpawnFlags();
		//		pCameraEntity->AddSpawnFlags(28);	// 28 = Freeze Player, Infinite Hold Time, Snap to goal angles

		if (DispatchSpawn(pCameraEntity) != 0)
		{
			Msg("Error spawning entity on server-side code!\n");
			return;
		}

		m_pAttractCameraEntity = pCameraEntity;

		UTIL_SetOrigin(m_pAttractCameraEntity, origin, true);
		m_pAttractCameraEntity->SetAbsAngles(angles);

		Vector vel = Vector(0, 0, 0);
		m_pAttractCameraEntity->Teleport(&origin, &angles, &vel);

		m_pAttractCameraEntity->Enable();

		if (iTransitionType == 2)
		{
			m_pAttractCameraEntity->UpdateAttractCameraGoal(origin, angles, iTransitionType);

			UTIL_SetOrigin(m_pAttractCameraEntity, origin, true);
			m_pAttractCameraEntity->SetAbsAngles(angles);

			// Get the current angles
			Vector forward;
			m_pAttractCameraEntity->GetVectors(&forward, NULL, NULL);

			Vector vel = forward * cvar->FindVar("attract_mode_drift")->GetFloat();
			m_pAttractCameraEntity->Teleport(&origin, &angles, &vel);

			//this->OnAttractModeTransformReached();
		}
	}
}

void CAnarchyManager::EndAttractMode()
{
	if (this->IsMapLoaded())
	{
		m_pAttractCameraEntity->Disable();
		UTIL_Remove(m_pAttractCameraEntity);
	}

	m_pAttractCameraEntity = NULL;
}

void CAnarchyManager::OnAttractModeTransformReached()
{
	// tell the client-side code so that it can decide what screenshot to go to next.

	CBaseEntity* pPlayerEntity = UTIL_GetLocalPlayer();
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	engine->ClientCommand(pClient, "attract_camera_reached");
}

Vector CAnarchyManager::GetAttractModeOrigin()
{
	if (!m_pAttractCameraEntity)
		return Vector(0, 0, 0);

	return m_attractCameraOrigin;
}

QAngle CAnarchyManager::GetAttractModeAngles()
{
	if (!m_pAttractCameraEntity)
		return QAngle(0, 0, 0);

	return m_attractCameraAngles;
}

void CAnarchyManager::AddJunkEntity(CBaseEntity* pEntity)
{
	m_junkEntities.AddToTail(pEntity);
}

void CAnarchyManager::RemoveJunkEntities()
{
	unsigned int iMax = m_junkEntities.Size();
	for (unsigned int i = 0; i < iMax; i++)
	{
		inputdata_t emptyDummy;
		m_junkEntities.Element(i)->InputKillHierarchy(emptyDummy);
	}

	this->ClearJunkEntities();
}

void CAnarchyManager::SetTorch(CBaseEntity* pEntity)
{
	if (m_pTorchEntity)
		this->DestroyTorch();

	m_pTorchEntity = pEntity;
}

void CAnarchyManager::DestroyTorch()
{
	if (m_pTorchEntity)
	{
		inputdata_t emptyDummy;
		m_pTorchEntity->InputKillHierarchy(emptyDummy);
		m_pTorchEntity = NULL;
	}
}

void CAnarchyManager::ClearJunkEntities()
{
	m_junkEntities.Purge();
}