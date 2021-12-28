#include "cbase.h"
#include "prop_shortcut_entity.h"
//#include "assert.h"

//#include "hl2mp_player.h"

#include <KeyValues.h>
#include "Filesystem.h"
#include "../../game/server/spotlightend.h"
#include "../../game/shared/beam_shared.h"


//#include "inetchannelinfo.h"
//#include "arcaderesources.h"

#include <string>
#include <algorithm>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( prop_shortcut, CPropShortcutEntity );

BEGIN_DATADESC(CPropShortcutEntity)
	DEFINE_KEYFIELD(m_bSlave, FIELD_BOOLEAN, "slave"),
	DEFINE_KEYFIELD(m_objectId, FIELD_STRING, "objectId"),
	DEFINE_KEYFIELD(m_itemId, FIELD_STRING, "itemId"),
	DEFINE_KEYFIELD(m_modelId, FIELD_STRING, "modelId"),
	DEFINE_USEFUNC( UseFunc ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CPropShortcutEntity, DT_PropShortcutEntity)
	SendPropBool(SENDINFO(m_bSlave)),
	SendPropStringT(SENDINFO(m_objectId)),
	SendPropStringT(SENDINFO(m_itemId)),
	SendPropStringT(SENDINFO(m_modelId)),
END_SEND_TABLE()

#define	ENTITY_MODEL	"models/cabinets/two_player_arcade.mdl"

ConVar object_shadows("object_shadows", "0", FCVAR_ARCHIVE, "Set to 1 to have AArcade objects cast dynamic shadows.");

CPropShortcutEntity::CPropShortcutEntity()
{
	m_bWantsPlayerCamera = false;
	m_pSequenceBlacklistConVar = NULL;
	m_pCabinetAttractModeActiveConVar = NULL;
}

void CPropShortcutEntity::Precache(void)
{
	PrecacheModel( UTIL_VarArgs("%s", this->GetModelName()) );
	BaseClass::Precache();
}

void CPropShortcutEntity::Spawn()
{
	m_bWantsPlayerCamera = false;
//	DevMsg("Yaaaarrrr!!!!\n\n");
	if (!object_shadows.GetBool())
		AddEffects(EF_NOSHADOW);
	BaseClass::InitAsHotlink();
	BaseClass::Spawn();

	//this->SetSolid(SOLID_VPHYSICS);
	//AddSolidFlags(FSOLID_NOT_SOLID);
	//RemoveSolidFlags(FSOLID_NOT_SOLID);

	Precache();
	SetModel( UTIL_VarArgs("%s", this->GetModelName()) );	// We should figure out what our model is going to be before we get to this line.

	//SetSolid( SOLID_NONE );
	//SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_NONE);
	//SetSolid( SOLID_VPHYSICS );
	
	//UTIL_SetSize( this, -Vector(100,100,100), Vector(100,100,100) );
	//SetMoveType( MOVETYPE_VPHYSICS );
	SetUse(&CPropShortcutEntity::UseFunc);

	/*
		if( CreateVPhysics() )
		{
			IPhysicsObject *pPhysics = this->VPhysicsGetObject();
			if( pPhysics )
			{
				pPhysics->EnableMotion(!nophysics);
			}
		}

		//	SetMoveType(MOVETYPE_NONE);
		IPhysicsObject* pPhysics = this->VPhysicsGetObject();
		if (!pPhysics && this->CreateVPhysics())
			pPhysics = this->VPhysicsGetObject();

		if (pPhysics)
		{
			pPhysics->EnableMotion(false);
		}
	*/

	this->PlaySequence("inactiveidle");
}

void CPropShortcutEntity::UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	//DevMsg("Object used: %i\n", (int)useType);
	if (useType == USE_TOGGLE)
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		edict_t *pClient = engine->PEntityOfEntIndex(pPlayer->entindex());
		engine->ClientCommand(pClient, "use_shortcut %i;\n", this->entindex());

		// Set the player's camera to this entity, if this entity has an attachment point for it.
		//if (pMPPlayer->GetSelectedCabinet() == -1)
		//{
		if (!m_bWantsPlayerCamera)
		{
			m_bWantsPlayerCamera = true;
			engine->ClientCommand(pClient, UTIL_VarArgs("select %i", this->entindex()));
		}
		/*else
		{
			if (m_pCameraEntity)
				m_pCameraEntity->CamToPlayer();

			m_bWantsPlayerCamera = false;
		}*/
		//}
	}
}

void CPropShortcutEntity::PrepForTransit(char* &memberBuf, const char* value)
{
	//Q_strcpymemberBuf = value;
	///*
	if (memberBuf)
		delete[] memberBuf;

	std::string buf = value;

	//memberBuf = new char[buf.size() + 1];
	memberBuf = new char[buf.length() + 1];
	std::copy(buf.begin(), buf.end(), memberBuf);
	memberBuf[buf.length()] = '\0';
	//*/
}

bool CPropShortcutEntity::SetPlayerCamera(const char* sequenceName)
{
	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");
	//if (pMPModeVar->GetBool())
	//	return false;

	//	if (m_pCameraEntity)
	//		this->ClearPlayerCamera();

	// FIXME FIX-ME FIX ME
	// This should be limited to SINGLE PLAYER ONLY until it is modified to be MP-friendly!!

	int iAttachmentIndex = this->LookupAttachment("aacam");
	if (iAttachmentIndex == 0)
		return false;

	CTriggerCamera* pCameraEntity;

	if (m_pCameraEntity)
		pCameraEntity = m_pCameraEntity;
	else
	{
		CBaseEntity* pBaseEntity = CreateEntityByName("point_viewcontrol");

		pCameraEntity = dynamic_cast<CTriggerCamera*>(pBaseEntity);

		if (!pCameraEntity)
		{
			Msg("Could not cast to a view control entity on the server!\n");
			return false;
		}

		// Modify some "Hammer" values before this is spawned.
		pCameraEntity->KeyValue("interpolatepositiontoplayer", "1");
		pCameraEntity->KeyValue("spawnflags", "92");

		//		pCameraEntity->ClearSpawnFlags();
		//		pCameraEntity->AddSpawnFlags(28);	// 28 = Freeze Player, Infinite Hold Time, Snap to goal angles

		if (DispatchSpawn(pCameraEntity) != 0)
		{
			Msg("Error spawning entity on server-side code!\n");
			return false;
		}

		// Hotlinks must be bound AFTER spawn.
		pCameraEntity->BindHotlink(this, iAttachmentIndex, sequenceName);


		/*Vector vecAttachmentPos;
		QAngle vecAttachmentAngles;
		this->GetBaseAnimating()->GetAttachment(iAttachmentIndex, vecAttachmentPos, vecAttachmentAngles);
		pCameraEntity->SetAbsOrigin(vecAttachmentPos);
		//pCameraEntity->SetAbsAngles(vecAttachmentAngles);
		pCameraEntity->SetLocalAngles(vecAttachmentAngles);*/

		m_pCameraEntity = pCameraEntity;
	}

	/*Vector vecAttachmentPos;
	QAngle vecAttachmentAngles;
	this->GetBaseAnimating()->GetAttachment(iAttachmentIndex, vecAttachmentPos, vecAttachmentAngles);
	m_pCameraEntity->SetAbsOrigin(vecAttachmentPos);
	//m_pCameraEntity->SetAbsAngles(vecAttachmentAngles);
	m_pCameraEntity->SetLocalAngles(vecAttachmentAngles);*/

	//	m_pCameraEntity->SetParent(this);
	//	m_pCameraEntity->SetParentAttachment("Set Parent Attachment", "aacam", false);

	m_pCameraEntity->Enable();
	
	if (!m_pCabinetAttractModeActiveConVar)
		m_pCabinetAttractModeActiveConVar = cvar->FindVar("cabinet_attract_mode_active");

	m_pCabinetAttractModeActiveConVar->SetValue(1);

	return true;
}

/*
void CPropShortcutEntity::ClearPlayerCamera()
{
	if (!m_pCameraEntity)
		return;

	m_pCameraEntity->Disable();

	UTIL_Remove(m_pCameraEntity);
}
*/

void CPropShortcutEntity::Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters)
{
	std::string safeStr = str;
	std::transform(safeStr.begin(), safeStr.end(), safeStr.begin(), ::tolower);

	// Skip delimiters at beginning.
	std::string::size_type lastPos = safeStr.find_first_not_of(delimiters, 0);

	// Find first "non-delimiter".
	std::string::size_type pos = safeStr.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));

		// Skip delimiters.  Note the "not_of"
		lastPos = safeStr.find_first_not_of(delimiters, pos);

		// Find next "non-delimiter"
		pos = safeStr.find_first_of(delimiters, lastPos);
	}
}

void CPropShortcutEntity::PlaySequence(const char* sequenceName)
{
	if (!m_pSequenceBlacklistConVar)
		m_pSequenceBlacklistConVar = cvar->FindVar("sequence_blacklist");
	std::string sequenceBlacklist = m_pSequenceBlacklistConVar->GetString();
	std::vector<std::string> tokens;
	this->Tokenize(sequenceBlacklist, tokens, " ");
	unsigned int size = tokens.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (tokens[i] == sequenceName)
			return;
	}

	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");
	//if (pMPModeVar->GetBool())
	//	return;

	//DevMsg("Sequence name is: %s\n", sequenceName);
	int index = this->LookupSequence(sequenceName);

	if (index != ACT_INVALID)
	{
		if (m_bWantsPlayerCamera && Q_stricmp(sequenceName, "inactiveidle"))
		{
			if (m_pCameraEntity && !m_pCameraEntity->GetReadyToAnimateCamera())
				return;

			m_bWantsPlayerCamera = false;

			// Attempt to set the camera, but this prop might not have the rigging for it.
			if (this->SetPlayerCamera(sequenceName))
				return;
		}
		else if (!m_bWantsPlayerCamera && !Q_stricmp(sequenceName, "inactiveidle") && m_pCameraEntity)
		{
			m_pCameraEntity->CamToPlayer();
			//m_bWantsPlayerCamera = true;
		}

		//		if (this->GetBaseAnimating()->SequenceLoops())	// Why was this ever needed?
		//			this->ResetSequenceInfo();

		//this->PropSetSequence(index);
		CDynamicProp* pThisAsDynamic = dynamic_cast<CDynamicProp*>(this);
		inputdata_t myInputData;
		myInputData.value.SetString(MAKE_STRING(sequenceName));
		pThisAsDynamic->InputSetAnimation(myInputData);
		
		if (!Q_stricmp(sequenceName, "inactiveidle") || !Q_stricmp(sequenceName, "activated") || !Q_stricmp(sequenceName, "activeidle") || !Q_stricmp(sequenceName, "deactivated") )
			this->CreateSequenceEntities(sequenceName);
	}
	//else
	//{
		/*
		CDynamicProp* pThisAsDynamic = dynamic_cast<CDynamicProp*>(this);
		inputdata_t myInputData;
		myInputData.value.SetString(MAKE_STRING(sequenceName));
		pThisAsDynamic->InputSetDefaultAnimation(myInputData);
		*/
//	}
}

//void CPropShortcutEntity::UpdateOnRemove(void)
//{
//	this->CleanupSequenceEntities();
//	BaseClass::UpdateOnRemove();
//}

class CPointSpotlight : public CPointEntity
{
	DECLARE_CLASS(CPointSpotlight, CPointEntity);
public:
	DECLARE_DATADESC();

	CPointSpotlight();

	void	Precache(void);
	void	Spawn(void);
	virtual void Activate();

	virtual void OnEntityEvent(EntityEvent_t event, void *pEventData);

	// Added for Anarchy Arcade
	void UpdateOnRemove(void);
	//bool KeyValue(const char *szKeyName, const char *szValue);
	void SetRainbowMode();
	void ShortcutThink(void);
	void SetShortcut(CPropShortcutEntity* pShortcut, bool bAudioSensitive);
	void VRSpazzFixPulse();
	// End added for Anarchy Arcade

private:
	int 	UpdateTransmitState();
	void	SpotlightThink(void);
	void	SpotlightUpdate(void);
	Vector	SpotlightCurrentPos(void);
	void	SpotlightCreate(void);
	void	SpotlightDestroy(void);

	// ------------------------------
	//  Inputs
	// ------------------------------
	void InputLightOn(inputdata_t &inputdata);
	void InputLightOff(inputdata_t &inputdata);

	// Creates the efficient spotlight 
	void CreateEfficientSpotlight();

	// Computes render info for a spotlight
	void ComputeRenderInfo();

private:
	bool m_bIsVRSpazzFix;	// Added for Anarchy Arcade
	float m_flOldScale;	// Added for Anarchy Arcade
	bool m_bIsRainbow;	// Added for Anarchy Arcade
	ConVar* m_pPeakConVar;	// Added for Anarchy Arcade
	ConVar* m_pHueShiftConVar;	// Added for Anarchy Arcade
	CPropShortcutEntity* m_pShortcut;	// Added for Anarchy Arcade
	bool	m_bSpotlightOn;
	bool	m_bEfficientSpotlight;
	bool	m_bIgnoreSolid;
	Vector	m_vSpotlightTargetPos;
	Vector	m_vSpotlightCurrentPos;
	Vector	m_vSpotlightDir;
	int		m_nHaloSprite;
	CHandle<CBeam>			m_hSpotlight;
	CHandle<CSpotlightEnd>	m_hSpotlightTarget;

	float	m_flSpotlightMaxLength;
	float	m_flSpotlightCurLength;
	float	m_flSpotlightGoalWidth;
	float	m_flHDRColorScale;
	int		m_nMinDXLevel;

public:
	COutputEvent m_OnOn, m_OnOff;     ///< output fires when turned on, off
};

void CPropShortcutEntity::CreateSequenceEntities(const char* sequenceName)
{
	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");
	//if (pMPModeVar->GetBool())
//		return;

	KeyValues* pModelKV = new KeyValues("");
	if (!pModelKV->LoadFromBuffer(modelinfo->GetModelName(GetModel()), modelinfo->GetModelKeyValueText(GetModel())))
		pModelKV->deleteThis();
	else
	{
		KeyValues* pSequenceEntitiesKV = pModelKV->FindKey(UTIL_VarArgs("aa_%s", sequenceName));
		if (pSequenceEntitiesKV)
		{
			// We have entities to spawn.
			bool bResetDuplicates = pSequenceEntitiesKV->GetBool("resetduplicates");

			if (pSequenceEntitiesKV->GetBool("clearallentities"))
				this->CleanupSequenceEntities();

			/* just use an ambient_generic instead.
			KeyValues* pSoundKV = pSequenceEntitiesKV->FindKey("sound");
			if (pSoundKV)
			{
			// Add the option to play a sound from a specific bone!
			// Add an option to play multiple sounds!
			CPASAttenuationFilter filter(this);

			EmitSound_t ep;
			ep.m_nChannel = CHAN_AUTO;
			ep.m_pSoundName = pSoundKV->GetString("file");
			ep.m_flVolume = 1.0;
			ep.m_SoundLevel = SNDLVL_NORM;

			CBaseEntity::EmitSound(filter, this->entindex(), ep);
			}
			*/

			for (KeyValues *pEntityKV = pSequenceEntitiesKV->GetFirstSubKey(); pEntityKV; pEntityKV = pEntityKV->GetNextKey())
			{
				if (Q_stricmp(pEntityKV->GetName(), "entity"))
					continue;

				int iAttachmentIndex = this->LookupAttachment(pEntityKV->GetString("attachment"));
				if (iAttachmentIndex == 0)
					return;

				// Now check to see if an identical entity type is attached to this same bone.
				// This is one of the major limitations of the system.  Only 1 entity type per-bone.
				// The other limitation is that you can't specify ID's for each entity to clear SPECIFIC entities upon entering/exiting a sequence.
				CBaseEntity* pDuplicateEntity = this->FindSequenceEntity(pEntityKV->GetString("classname"), iAttachmentIndex);
				if (pDuplicateEntity)
				{
					if (!bResetDuplicates)
						continue;
					else
						this->RemoveSequenceEntity(pDuplicateEntity);
				}

				CBaseEntity* pBaseEntity = CreateEntityByName(pEntityKV->GetString("classname"));

				for (KeyValues *pPropertyKV = pEntityKV->GetFirstSubKey(); pPropertyKV; pPropertyKV = pPropertyKV->GetNextKey())
				{
					if (pPropertyKV->GetFirstSubKey() || !Q_stricmp(pPropertyKV->GetName(), "angles") || !Q_stricmp(pPropertyKV->GetName(), "origin"))
						continue;

					pBaseEntity->KeyValue(pPropertyKV->GetName(), pPropertyKV->GetString());
				}

				Vector vecAttachmentPos;
				QAngle vecAttachmentAngles;
				this->GetBaseAnimating()->GetAttachment(iAttachmentIndex, vecAttachmentPos, vecAttachmentAngles);

				char buf[512];

				Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentPos.x, vecAttachmentPos.y, vecAttachmentPos.z);
				pBaseEntity->KeyValue("origin", buf);

				Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentAngles.x, vecAttachmentAngles.y, vecAttachmentAngles.z);
				pBaseEntity->KeyValue("angles", buf);

				if (!Q_strcmp(pEntityKV->GetString("classname"), "point_spotlight"))
				{
					CPointSpotlight* pSpotlight = dynamic_cast<CPointSpotlight*>(pBaseEntity);
					if (pSpotlight)
					{
						if (pEntityKV->GetInt("spawnflags") & 0x4)
							pSpotlight->SetShortcut(this, true);
						else
							pSpotlight->SetShortcut(this, false);
					}
					//pBaseEntity->Activate();
				}

				pBaseEntity->Precache();
				if (DispatchSpawn(pBaseEntity) != 0)
				{
					Msg("Error spawning sequence entity on server-side code!\n");
					continue;
				}

				//pBaseEntity->SetParent(this);
				if (!Q_strcmp(pEntityKV->GetString("classname"), "func_smokevolume"))
					pBaseEntity->Activate();
				//pBaseEntity->SetTransmitState(FL_EDICT_ALWAYS);

				DevMsg("Created entity of type: %s\n", pEntityKV->GetString("classname"));

				m_sequenceEntities.push_back(pBaseEntity);

				UTIL_SetOrigin(pBaseEntity, vecAttachmentPos);
				pBaseEntity->SetAbsAngles(vecAttachmentAngles);

				Vector vel = Vector(0, 0, 0);
				Vector origin = vecAttachmentPos;
				QAngle angles = vecAttachmentAngles;

				//pBaseEntity->Teleport(&vecAttachmentPos, &vecAttachmentAngles, &vel);


				pBaseEntity->SetParent(this, iAttachmentIndex);
			}
		}
	}
}

CBaseEntity* CPropShortcutEntity::FindSequenceEntity(const char* classname, int iAttachmentIndex)
{
	unsigned int vecSize = m_sequenceEntities.size();
	CBaseEntity* pEntity;
	for (unsigned int i = 0; i < vecSize; i++)
	{
		pEntity = m_sequenceEntities[i];

		if (pEntity && !Q_stricmp(pEntity->GetClassname(), classname) && pEntity->GetParentAttachment() == iAttachmentIndex)
			return pEntity;
	}

	return NULL;
}

void CPropShortcutEntity::RemoveSequenceEntity(CBaseEntity* pEntity)
{
	unsigned int vecSize = m_sequenceEntities.size();
	for (unsigned int i = 0; i < vecSize; i++)
	{
		if (m_sequenceEntities[i] == pEntity)
		{
			m_sequenceEntities[i] = NULL;
			UTIL_Remove(pEntity);
			return;
		}
	}
}

void CPropShortcutEntity::CleanupSequenceEntities()
{
	/*
	CBaseEntity* pEntity = this->FirstMoveChild();
	while (pEntity)
	{
	inputdata_t emptyDummy;
	pEntity->InputKillHierarchy(emptyDummy);

	pEntity = this->FirstMoveChild();
	}
	*/

	while (!m_sequenceEntities.empty())
	{
		CBaseEntity* pEntity = m_sequenceEntities[m_sequenceEntities.size() - 1];
		if (pEntity)
		{
			inputdata_t emptyDummy;
			//pEntity->InputOff(emptyDummy);
			//pEntity->SetParent(pEntity, 0);
			/*DevMsg("Cleaning up %s\n", pEntity->GetClassname());
			if (!Q_strcmp(pEntity->GetClassname(), "point_spotlight"))
			{
				CPointSpotlight* pSpot = dynamic_cast<CPointSpotlight*>(pEntity);
				if (pSpot)
				{
					pSpot->InputLightOff(emptyDummy);
					//pSpot->InputDispatchEffect
					//pSpot->Input
					pSpot->InputKillHierarchy(emptyDummy);
				}
			}
			else*/
				pEntity->InputKillHierarchy(emptyDummy);
			//pEntity->InputKill(emptyDummy);
			//UTIL_Remove(pEntity);
		}

		m_sequenceEntities.pop_back();
	}
}

void CPropShortcutEntity::CleanupCameraEntities()
{
	if (m_pCameraEntity)
	{
		if (!m_pCabinetAttractModeActiveConVar)
			m_pCabinetAttractModeActiveConVar = cvar->FindVar("cabinet_attract_mode_active");

		m_pCabinetAttractModeActiveConVar->SetValue(0);

		m_pCameraEntity->Disable();

		inputdata_t emptyDummy;
		m_pCameraEntity->InputKillHierarchy(emptyDummy);

		m_pCameraEntity = NULL;
	}

	// If the m_entities vector is empty, then do nothing.  Otherwise, clean up all the shit in it.
}

void CPropShortcutEntity::AnimFinished()
{
	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");
	//if (pMPModeVar->GetBool())
	//	return;

	// If this animation was playing on a specific player entity, the trigger could send us that player when it calls us.

	//int iCurrentAnimIndex = this->GetSequence();
	//std::string currentAnimName = this->GetSequenceName(iCurrentAnimIndex);

	//	this->StopAnimation();

	std::string currentAnimName = this->GetSequenceName(this->GetSequence());

	//DevMsg("An animation has finished on the hotlink: %s\n", currentAnimName.c_str());

	if (currentAnimName == "activated")
	{
		this->SetCycle(0.0f);

		//		this->PlaySequence("activeidle");

		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		edict_t *pClient = engine->PEntityOfEntIndex(pPlayer->entindex());

		engine->ClientCommand(pClient, "nextsequenceready %i \"activeidle\";\n", this->entindex());
	}
	else if (currentAnimName == "deactivated")
	{
		this->SetCycle(0.0f);

		//		this->PlaySequence("inactiveidle");

		//this->CleanupEntities();
		// Now lets fly this camera back to the player.
		if (m_pCameraEntity)
			m_pCameraEntity->CamToPlayer();

		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		edict_t *pClient = engine->PEntityOfEntIndex(pPlayer->entindex());

		engine->ClientCommand(pClient, "nextsequenceready %i \"inactiveidle\";\n", this->entindex());
	}
	else
	{
		//if (currentAnimName != "inactiveidle")
		//{
			this->SetCycle(0);
			this->ResetClientsideFrame();
			//this->NetworkStateChanged();

		//}
		this->SetNextThink(gpGlobals->curtime + 0.1f);

		//this->AddSolidFlags(FSOLID_NOT_SOLID);
		//this->SetSolid(SOLID_NONE);
	}
}

/*
void CPropHotlinkEntity::SetLiveURL(const char* value)
{
	this->PrepForTransit(m_liveURLBuf, value);

	m_liveURL.Set(MAKE_STRING(m_liveURLBuf));
}
*/

bool CPropShortcutEntity::TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr)//(const Ray_t &ray, unsigned int mask, trace_t& trace)
{
	// Return a special case for scaled physics objects
	if (GetModelScale() != 1.0f)
	{
		IPhysicsObject *pPhysObject = VPhysicsGetObject();
		if (pPhysObject)
		{
			Vector vecPosition;
			QAngle vecAngles;
			pPhysObject->GetPosition(&vecPosition, &vecAngles);
			const CPhysCollide *pScaledCollide = pPhysObject->GetCollide();
			physcollision->TraceBox(ray, pScaledCollide, vecPosition, vecAngles, &tr);

			return tr.DidHit();
		}
		else
			return false;
	}

	if (IsSolidFlagSet(FSOLID_CUSTOMRAYTEST))
	{
		if (!TestHitboxes(ray, fContentsMask, tr))
			return true;

		return tr.DidHit();
	}

	// We shouldn't get here.
	Assert(0);
	return false;
}

bool CPropShortcutEntity::CreateVPhysics()
{
	BaseClass::CreateVPhysics();
	VPhysicsInitStatic();
	return true;
}

bool CPropShortcutEntity::CreateVPhysicsMotion()
{
	BaseClass::CreateVPhysics();
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false );
	return true;
}

void CPropShortcutEntity::SetItemId(std::string id)
{
	this->PrepForTransit(m_itemIdBuf, id.c_str());
	m_itemId.Set(MAKE_STRING(m_itemIdBuf));
	//NetworkStateChanged();
	//m_itemId = MAKE_STRING(id.c_str());
	//Q_strcpy(m_itemId, id.c_str());
}

void CPropShortcutEntity::SetModelId(std::string id)
{
	//DevMsg("Setting model ID to: %s\n", id.c_str());
	//string_t buf = id.c_str();
	//m_modelId.Set(MAKE_STRING(id.c_str()));

	//if (m_modelIdBuf)
		//delete[] m_modelIdBuf;

	//m_modelIdBuf = new char(id.length)

	//DevMsg("Setting model ID to: %s\n", id.c_str());
	this->PrepForTransit(m_modelIdBuf, id.c_str());
	m_modelId.Set(MAKE_STRING(m_modelIdBuf));
	//NetworkStateChanged();

	//DevMsg("And val here on server is: %s\n", m_modelId.m_Value.ToCStr());
	//m_modelId = MAKE_STRING(id.c_str());
	//Q_strcpy(m_modelIdBuf, id.c_str());
	//m_modelId = MAKE_STRING(m_modelIdBuf);
	//Q_strcpy(m_itemId, id.c_str());
}

void CPropShortcutEntity::SetSlave(bool bVal)
{
	m_bSlave.Set(bVal);
}

LINK_ENTITY_TO_CLASS(node_info, CNodeInfoEntity);

// Start of our data description for the class
BEGIN_DATADESC(CNodeInfoEntity)

// Save/restore our active state
//	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_flNextChangeTime, FIELD_TIME ),

// Links our input name from Hammer to our input member function
DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

// Declare our think function
//DEFINE_THINKFUNC( MoveThink ),

END_DATADESC()

// Name of our entity's model
#define	ENTITY_MODEL	"models/icons/wall_pad_w.mdl"

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CNodeInfoEntity::Precache(void)
{
	return;	// Disable nodes
	PrecacheModel(ENTITY_MODEL);

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CNodeInfoEntity::Spawn(void)
{
	return;	// Disable nodes
	Precache();

	//SetModel(ENTITY_MODEL);
	SetSolid(SOLID_NONE);
	UTIL_SetSize(this, -Vector(1, 1, 1), Vector(1, 1, 1));
	//UTIL_SetSize(this, -Vector(100, 100, 100), Vector(100, 100, 100));
	//UTIL_SetSize(this, -Vector(20, 20, 20), Vector(20, 20, 20));
}

/*
//-----------------------------------------------------------------------------
// Purpose: Think function to randomly move the entity
//-----------------------------------------------------------------------------
void CNodeInfoEntity::MoveThink(void)
{
// See if we should change direction again
if ( m_flNextChangeTime < gpGlobals->curtime )
{
// Randomly take a new direction and speed
Vector vecNewVelocity = RandomVector( -64.0f, 64.0f );
SetAbsVelocity( vecNewVelocity );

// Randomly change it again within one to three seconds
m_flNextChangeTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 3.0f );
}

// Snap our facing to where we're heading
Vector velFacing = GetAbsVelocity();
QAngle angFacing;
VectorAngles( velFacing, angFacing );
SetAbsAngles( angFacing );

// Think every 20Hz
SetNextThink( gpGlobals->curtime + 0.05f );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Toggle the movement of the entity
//-----------------------------------------------------------------------------
void CNodeInfoEntity::InputToggle(inputdata_t &inputData)
{
	/*
	// Toggle our active state
	if ( !m_bActive )
	{
	// Start thinking
	SetThink( &CMyModelEntity::MoveThink );

	SetNextThink( gpGlobals->curtime + 0.05f );

	// Start moving
	SetMoveType( MOVETYPE_FLY );

	// Force MoveThink() to choose a new speed and direction immediately
	m_flNextChangeTime = gpGlobals->curtime;

	// Update m_bActive to reflect our new state
	m_bActive = true;
	}
	else
	{
	// Stop thinking
	SetThink( NULL );

	// Stop moving
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );

	m_bActive = false;
	}
	*/
}