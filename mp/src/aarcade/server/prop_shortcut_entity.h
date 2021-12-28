#ifndef PROP_SHORTCUT_ENTITY_H
#define PROP_SHORTCUT_ENTITY_H

#include "props.h"
/*
#include "materialsystem/ITexture.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IMaterialProxy.h"
*/
#include "../game/server/triggers.h"
#include <vector>

#include <string>

//class CPropShortcutEntity : public CBaseAnimating
class CPropShortcutEntity : public CDynamicProp
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	//DECLARE_CLASS(CPropShortcutEntity, CBaseAnimating);
	DECLARE_CLASS(CPropShortcutEntity, CDynamicProp);
	CPropShortcutEntity();
	void Spawn();
	void Precache( void );
	//void UpdateOnRemove(void);
	bool CreateVPhysics();
	bool CreateVPhysicsMotion();
	void UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps()	{ return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE); }
	void SetItemId(std::string id);
	void SetModelId(std::string id);
	void SetSlave(bool bVal);


	void PrepForTransit(char* &memberBuf, const char* value);

	bool SetPlayerCamera(const char* sequenceName);
	void ClearPlayerCamera();
	void PlaySequence(const char* sequenceName);
	void CreateSequenceEntities(const char* sequenceName);
	CBaseEntity* FindSequenceEntity(const char* classname, int iAttachmentIndex);
	void RemoveSequenceEntity(CBaseEntity* pEntity);
	void CleanupSequenceEntities();
	void CleanupCameraEntities();
	void AnimFinished();
	void SetWantsCamera(bool bValue) { m_bWantsPlayerCamera = bValue; }

	bool TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);//(const Ray_t &ray, unsigned int mask, trace_t& trace);
	void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters);

private:
	std::vector<CBaseEntity*> m_sequenceEntities;
	ConVar* m_pSequenceBlacklistConVar;
	ConVar* m_pCabinetAttractModeActiveConVar;
	bool m_bWantsPlayerCamera;
	CTriggerCamera* m_pCameraEntity;
	CNetworkVar(bool, m_bSlave);
	CNetworkVar(string_t, m_objectId);
	char* m_objectIdBuf;
	CNetworkVar(string_t, m_itemId);
	char* m_itemIdBuf;
	CNetworkVar(string_t, m_modelId);
	char* m_modelIdBuf;
};

class CNodeInfoEntity : public CBaseAnimating
{
public:
	DECLARE_CLASS(CNodeInfoEntity, CBaseAnimating);
	DECLARE_DATADESC();

	CNodeInfoEntity()
	{
		//m_bActive = false;
	}

	void Spawn(void);
	void Precache(void);

	//void MoveThink( void );

	// Input function
	void InputToggle(inputdata_t &inputData);

private:

	//bool	m_bActive;
	//float	m_flNextChangeTime;
};

#endif //PROP_SHORTCUT_ENTITY_H