#include "cbase.h"
#include "prop_shortcut_entity.h"
#include "hl2mp_player.h"
#include "KeyValues.h"
#include "Filesystem.h"
#include <vector>
#include "../game/server/triggers.h"
#include "../func_dust_shared.h"
#include "CAnarchyManager.h"
#include <algorithm>

//#include "dmxloader/dmxloader.h"
//#include "dmxloader/dmxelement.h"
//#include "../../game/server/func_dust.cpp"
//#include ".."
//#include "../../game/client/glow_outline_effect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
/*
void AddGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->AddGlowEffect();
}
ConCommand addgloweffect("addgloweffect", AddGlowEffect, "Adds a glow around the entity.", FCVAR_HIDDEN);

void RemoveGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->RemoveGlowEffect();
}
ConCommand removegloweffect("removegloweffect", RemoveGlowEffect, "Removes a glow around the entity.", FCVAR_HIDDEN);
*/

#ifndef MAPBASE
#define MAPBASE 1
#endif

ConVar junkmodel("junkmodel", "models\\de_halloween\\jacklight.mdl", FCVAR_ARCHIVE, "The model that gets tossed out with spawn_junk.");
ConVar hueshift("hueshift", "0", FCVAR_NONE, "Internal. It's what is used to calculate the hue shift of audio-sensitive fx.");
ConVar peak("peak", "0.5", FCVAR_NONE, "Set to 0.5 for default?");
ConVar player_spawn_override("player_spawn_override", "", FCVAR_NONE, "Entity name to spawn players at (if it exists.)");	// For map transitions, but also to alter respawn position during gameplay via console or map scripting.
ConVar sequence_blacklist("sequence_blacklist", "", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Space-separated list of animation sequence names you want to blacklist.");
ConVar pointWithinNode("point_within_node", "-1", FCVAR_REPLICATED, "Set to -1 if not in a node space.  Otherwise it is set to the node info entity.");
ConVar build_ghosts("build_ghosts", "0", FCVAR_ARCHIVE, "Set to 1 to make objects transparent while spawning them.");
ConVar scale_collisions("scale_collisions", "1", FCVAR_NONE, "Set to 0 to disable collision scaling, if your arcades are failing to load.");
ConVar damage_physics("damage_physics", "0", FCVAR_ARCHIVE, "If enabled, objects will automatically have physics enabled when they get damanged by a weapon.");
ConVar precacheinstances("precache_instances", "1", FCVAR_ARCHIVE, "BETA: If enabled, instances will precache all their assets before loading. This speeds up load times, but sometimes still leads to crashes due to bugs.");
ConVar aapropfademin("aapropfademin", "-1", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Distance AArcade objects will start to fade out at. Set to -1 to disable fade. Takes effect upon map reload.");
ConVar aapropfademax("aapropfademax", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Distance AArcade objects will be fully faded out at. Set to 0 to disable fade. Takes effect upon map reload.");
ConVar attract_mode_drift("attract_mode_drift", "2.0", FCVAR_ARCHIVE, "The speed to drift at after reaching a screenshot position.  Set to 0 to turn off drift.");
ConVar attract_mode_hold("attract_mode_hold", "8.0", FCVAR_ARCHIVE, "The amount of time, in seconds, to hold each screenshot position while in attract mode.");
ConVar attract_mode_time("attract_mode_time", "4.0", FCVAR_ARCHIVE, "The amount of time, in seconds, that it takes to reach each screenshot position while in attract mode.");
ConVar cabinet_attract_mode_active("cabinet_attract_mode_active", "0", FCVAR_REPLICATED | FCVAR_HIDDEN, "Internal use only.");


/* WORKING server-side DMX iterating
// start PCF reading tests
void IterateDmxElementServer(CDmxElement* pRoot)
{
	for (int i = 0; i<pRoot->AttributeCount(); i++)
	{
		CDmxAttribute* pCur = pRoot->GetAttribute(i);
		CDmxElement* subElem;
		DmAttributeType_t type = pCur->GetType();

		if (type == AT_STRING)
		{
			//Msg("Name: %s\tType: %i\tNumComponents: %i\tValue: %s\n", pCur->GetName(), type, NumComponents(type), pCur->GetValue<CUtlString>().Get());
			//if (Q_strcmp(pCur->GetName(), "name") && Q_strcmp(pCur->GetName(), "functionName"))
			//	Msg("%s\t%s\n", pCur->GetName(), pCur->GetValue<CUtlString>().Get());
			if (!Q_strcmp(pCur->GetName(), "material"))
			{
				Msg("%s\t%s\n", pCur->GetName(), pCur->GetValue<CUtlString>().Get());
			}
		}
		else if (IsArrayType(type))
		{
			//Msg("Name: %s\tType: %i\tNumComponents: %i\tValue: %s\n", pCur->GetName(), type, NumComponents(type), pCur->GetValue<CUtlString>().Get());

			// assume the array type is AT_ELEMENT_ARRAY
			// output the array length here
			const CUtlVector<CDmxElement*>& array = pCur->GetArray<CDmxElement*>();
			int arrayLength = array.Count();
			//Msg("Array Length: %i\n", arrayLength);

			// Optionally iterate through the array elements
			for (int j = 0; j < arrayLength; j++)
			{
				subElem = array[j];
				// Process subElem here, such as recursively calling IterateDmxElement
				IterateDmxElementServer(subElem);
			}

			//const CUtlVector<CDmxElement>& dmxElementArray = pCur->GetArray<CDmxElement>();
			//const CUtlVector<DmElementArray_t> elementArray = pCur->GetArray();
			//for (int i = 0; i < dmxElementArray.Count(); ++i)
			//{
				//Msg("Array element detected.\n");
				//const CDmxElement& arrayElem = dmxElementArray[i];
				//CDmxElement* arrayElemPointer = ?;
				//IterateDmxElement(arrayElemPointer);


				//const CDmxElement& arrayElem = dmxElementArray[i];
				//CDmxElement* arrayElemPointer = const_cast<CDmxElement*>(&arrayElem);
				//IterateDmxElement(arrayElemPointer);

				//Msg("Name: %s\tType: %i\tNumComponents: %i\tValue: %s\n", pCur->GetName(), type, NumComponents(type), pCur->GetValue<CUtlString>().Get());
			//}
		}
		else {
			//Msg("Name: %s\tType: %i\tNumComponents: %i\n", pCur->GetName(), type, NumComponents(type));
		}
	}
}

CON_COMMAND(dmx_iterate_server, "Prints a DMX file to the console")
{
	DECLARE_DMX_CONTEXT();
	Msg("Loading: %s\n", args[1]);
	CDmxElement* DMX = (CDmxElement*)DMXAlloc(50000000);

	if (UnserializeDMX(args[1], "MOD", false, &DMX))
	{
		IterateDmxElementServer(DMX);
	}
	else
		Warning("Could not read DMX file %s\n", args[1]);
}

// end PCF reading tests

*/





void AddGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->AddGlowEffect();
}
ConCommand addgloweffect("addgloweffect", AddGlowEffect, "Adds a glow around the entity.", FCVAR_HIDDEN);

void RemoveGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->RemoveGlowEffect();
}
ConCommand removegloweffect("removegloweffect", RemoveGlowEffect, "Removes a glow around the entity.", FCVAR_HIDDEN);

void AddHoverGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->AddGlowEffect();
}
ConCommand addhovergloweffect("addhovergloweffect", AddHoverGlowEffect, "Adds a hover glow around the entity.", FCVAR_HIDDEN);
/*
void ReparentShortcuts(const CCommand &args)
{
	// get a list of every prop_shortcut that is within ANY of the volumes that match the name.
	//...

	// get the parent entity
	// ...

	// loop 



	int total = 0;

	CBaseEntity *pEntity = NULL;
	std::string entityName = args[1];
	std::string parentEntityName = args[2];
	std::string triggerEntityIndexes = "";
	unsigned int uTotal = 0;
	while ((pEntity = gEntList.FindEntityByName(pEntity, entityName.c_str())) != NULL)
	{
		uTotal++;
		if (uTotal > 1) {
			triggerEntityIndexes += " ";
		}
		triggerEntityIndexes += UTIL_VarArgs("%i", pEntity->entindex());
	}

	if (uTotal > 0) {
		CBaseEntity* pParentEntity = gEntList.FindEntityByName(NULL, parentEntityName.c_str());
		if (pParentEntity){
			CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
			edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
			engine->ClientCommand(pClient, UTIL_VarArgs("reparent_shortcuts_ready \"%s\" %i;", triggerEntityIndexes.c_str(), pParentEntity->entindex()));
		}
	}
	else {
		DevMsg("WARNING: Could not find any entities within volume to attach.\n");
	}
}
ConCommand reparentshortcuts("reparent_shortcuts", ReparentShortcuts, "Reparents the shortcuts that are inside of the provided trigger entity to the provided parent entity.", FCVAR_NONE);
*/
void RemoveJunkObjects(const CCommand &args)
{
	g_pAnarchyManager->RemoveJunkEntities();
}
ConCommand removejunkobjects("removejunkobjects", RemoveJunkObjects, "Removes all the junk objects from your session.", FCVAR_NONE);

void PlaySequence(const CCommand &args)
{
	CPropShortcutEntity* pProp = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (!pProp)
		return;

	//pProp->SetModelScale(2.0);	// WORKING WORKING WORKING ITEM SCALING!  JUST LEAVE LAST PARAM AS ZERO!
	pProp->PlaySequence(args[2]);
}
ConCommand playsequence("playsequence", PlaySequence, "Internal.");

void SetActiveSpawnedShadows(const CCommand &args)
{
	//bool bVal = (Q_atoi(args[1]) != 0);
	bool bVal = (args.ArgC() > 1) ? Q_atoi(args[1]) != 0 : cvar->FindVar("object_shadows")->GetInt() != 0;

	CBaseEntity* pEntity = gEntList.FindEntityByClassname(NULL, "prop_shortcut");
	while (pEntity)
	{
		if (bVal)
			pEntity->RemoveEffects(EF_NOSHADOW);
		else
			pEntity->AddEffects(EF_NOSHADOW);
		pEntity = gEntList.FindEntityByClassname(pEntity, "prop_shortcut");
	}
}
ConCommand setactivespawnedshadows("set_active_spawned_shadows", SetActiveSpawnedShadows, "Internal.");

/*
void NextShorcut(const CCommand &args)
{
	// arg 1 is a starting entity index
	CBaseEntity* pStartingEntity = NULL;

	if (args.ArgC() > 1)
	{
		pStartingEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	}

	CBasePlayer* pPlayerBaseEntity = dynamic_cast<CBasePlayer*>(UTIL_GetCommandClient());

	trace_t tr;
	Vector forward;
	pPlayerBaseEntity->EyeVectors(&forward);

	UTIL_TraceLine(pPlayerBaseEntity->EyePosition(), pPlayerBaseEntity->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayerBaseEntity, COLLISION_GROUP_NONE, &tr);

	// No hit? We're done.
	if (tr.fraction == 1.0)
		return;

	CBaseEntity* pEntity = gEntList.FindEntityByClassnameWithin(pStartingEntity, "prop_hotlink", tr.endpos, 300);
	if (pEntity)
	{
		edict_t *pClient = engine->PEntityOfEntIndex(UTIL_GetCommandClient()->entindex());
		engine->ClientCommand(pClient, UTIL_VarArgs("select %i;\n", pEntity->entindex()));
	}
	//else {
	//	pEntity = gEntList.FindEntityByClassnameWithin(NULL, "prop_shortcut", tr.endpos, 200);

	//	if (pEntity)
	//	{
	//		edict_t *pClient = engine->PEntityOfEntIndex(UTIL_GetCommandClient()->entindex());
	//		engine->ClientCommand(pClient, UTIL_VarArgs("select %i;\n", pEntity->entindex()));
	//	}
	//}
}
ConCommand nextshorcut("next_shortcut", NextShorcut, "Usegae: Selects the next nearest shortcut to the player in his/her line of sight.", FCVAR_HIDDEN);
*/
void RemoveHoverGlowEffect(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->RemoveGlowEffect();
}
ConCommand removehovergloweffect("removehovergloweffect", RemoveHoverGlowEffect, "Removes a hover glow around the entity.", FCVAR_HIDDEN);
/*
void UnstickServer(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
		pEntity->RemoveGlowEffect();

	edict_t *pClient = engine->PEntityOfEntIndex(pEntity->entindex());
	if (pClient)
		engine->ClientCommand(pClient, "unstick_player");
}
ConCommand unstickserver("unstick_server", UnstickServer, "", FCVAR_HIDDEN);
*/
/*
void ServerPause(const CCommand &args)
{
	if( !engine->IsPaused() )
		engine->ServerCommand("pause");
}
ConCommand serverpause("serverpause", ServerPause, "Pauses the server.", FCVAR_HIDDEN);
*/
void TeleportPlayer(const CCommand &args)
{
	//engine->ServerCmd(VarArgs("teleport_player %i %s %s\n", pPlayer->entindex()));

	if (args.ArgC() < 5)
		return;

	int TheEntity = Q_atoi(args.Arg(1));

	CBaseEntity* pEntity = NULL;
	edict_t *pEdict = INDEXENT(TheEntity);
	if (pEdict && !pEdict->IsFree())
	{
		pEntity = GetContainingEntity(pEdict);
	}
	else
		pEntity = UTIL_GetCommandClient();

	if (pEntity)
	{
		//CPropHotlinkEntity* pEntity = (CPropHotlinkEntity*)GetContainingEntity(pHotlinkEdict);
		Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));

		QAngle angles;
		if (args.ArgC() >= 8)
			angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));
		else
			angles = QAngle(0.0f, 0.0f, 0.0f);

		QAngle cameraAngles;
		if (args.ArgC() >= 11)
		{
			cameraAngles = QAngle(Q_atof(args.Arg(8)), Q_atof(args.Arg(9)), Q_atof(args.Arg(10)));
			angles = cameraAngles;
		}
		else
		{
			cameraAngles = QAngle(0.0f, 0.0f, 0.0f);
		}

		//UTIL_SetOrigin(pEntity, origin, true);
		//pEntity->SetAbsAngles(angles);

		//origin.z += 10.0;
		pEntity->SetAbsOrigin(origin);
		pEntity->SetAbsAngles(angles);

		Vector vel = Vector(0, 0, 0);
		pEntity->Teleport(&origin, &angles, &vel);

		edict_t *pClient = engine->PEntityOfEntIndex(pEntity->entindex());
		engine->ClientCommand(pClient, UTIL_VarArgs("setang %f %f %f; ", angles.x, angles.y, angles.z));
	}
}

ConCommand teleport_player("teleport_player", TeleportPlayer, "For internal use only.");

class CFunc_Dust : public CBaseEntity
{
public:
	DECLARE_CLASS(CFunc_Dust, CBaseEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CFunc_Dust();
	virtual 		~CFunc_Dust();


	// CBaseEntity overrides.
public:

	virtual void	Spawn();
	virtual void	Activate();
	virtual void	Precache();
	virtual bool	KeyValue(const char *szKeyName, const char *szValue);


	// Input handles.
public:

	void InputTurnOn(inputdata_t &inputdata);
	void InputTurnOff(inputdata_t &inputdata);


	// FGD properties.
public:

	CNetworkVar(color32, m_Color);
	CNetworkVar(int, m_SpawnRate);

	CNetworkVar(float, m_flSizeMin);
	CNetworkVar(float, m_flSizeMax);

	CNetworkVar(int, m_SpeedMax);

	CNetworkVar(int, m_LifetimeMin);
	CNetworkVar(int, m_LifetimeMax);

	CNetworkVar(int, m_DistMax);

	CNetworkVar(float, m_FallSpeed);

public:

	CNetworkVar(int, m_DustFlags);	// Combination of DUSTFLAGS_

private:
	int			m_iAlpha;

};

class CFunc_DustCloud : public CFunc_Dust
{
	DECLARE_CLASS(CFunc_DustCloud, CFunc_Dust);
public:
};

#include "baseparticleentity.h"
class CFuncSmokeVolume : public CBaseParticleEntity
{
public:
	DECLARE_CLASS(CFuncSmokeVolume, CBaseParticleEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CFuncSmokeVolume();
	void Spawn();
	void Activate(void);

	// Set the times it fades out at.
	void SetDensity(float density);

private:
	CNetworkVar(color32, m_Color1);
	CNetworkVar(color32, m_Color2);
	CNetworkString(m_MaterialName, 255);
	string_t m_String_tMaterialName;
	CNetworkVar(float, m_ParticleDrawWidth);
	CNetworkVar(float, m_ParticleSpacingDistance);
	CNetworkVar(float, m_DensityRampSpeed);
	CNetworkVar(float, m_RotationSpeed);
	CNetworkVar(float, m_MovementSpeed);
	CNetworkVar(float, m_Density);
};

void FogTest(const CCommand &args)
{
	CBaseEntity* pPlayerBaseEntity = UTIL_GetCommandClient();
	CHL2MP_Player* pMPPlayer = dynamic_cast<CHL2MP_Player*>(pPlayerBaseEntity);
	Vector origin = pMPPlayer->GetAbsOrigin();
	QAngle angles = pMPPlayer->GetAbsAngles();

	// Now spawn it
	CBaseEntity* pBaseEntity = CreateEntityByName("func_dustcloud");
	CFunc_DustCloud* pDustCloudEntity = dynamic_cast<CFunc_DustCloud*>(pBaseEntity);

	// Pass in standard key values
	char buf[512];
	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pDustCloudEntity->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pDustCloudEntity->KeyValue("angles", buf);

	pDustCloudEntity->KeyValue("Alpha", "30");
	pDustCloudEntity->KeyValue("Color", "255 255 255");
	pDustCloudEntity->KeyValue("DistMax", "1024");
	pDustCloudEntity->KeyValue("Frozen", "0");
	pDustCloudEntity->KeyValue("LifetimeMax", "5");
	pDustCloudEntity->KeyValue("LifetimeMin", "3");
	pDustCloudEntity->KeyValue("SizeMax", "200");
	pDustCloudEntity->KeyValue("SizeMin", "100");
	pDustCloudEntity->KeyValue("SpawnRate", "40");
	pDustCloudEntity->KeyValue("SpeedMax", "13");
	pDustCloudEntity->KeyValue("StartDisabled", "0");

	pDustCloudEntity->Precache();
	DispatchSpawn(pDustCloudEntity);
	pDustCloudEntity->Activate();

	pDustCloudEntity->SetTransmitState(FL_EDICT_ALWAYS);

	inputdata_t myInputData;
	pDustCloudEntity->InputTurnOn(myInputData);
}
ConCommand fog_test("fog_test", FogTest, "For internal use only.");

void SmokeTest(const CCommand &args)
{
	CBaseEntity* pPlayerBaseEntity = UTIL_GetCommandClient();
	CHL2MP_Player* pMPPlayer = dynamic_cast<CHL2MP_Player*>(pPlayerBaseEntity);
	Vector origin = pMPPlayer->GetAbsOrigin();
	QAngle angles = pMPPlayer->GetAbsAngles();

	// Now spawn it
	CBaseEntity* pBaseEntity = CreateEntityByName("func_smokevolume");
	CFuncSmokeVolume* pDSmokeVolumeEntity = dynamic_cast<CFuncSmokeVolume*>(pBaseEntity);

	// Pass in standard key values
	char buf[512];
	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pDSmokeVolumeEntity->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pDSmokeVolumeEntity->KeyValue("angles", buf);


	pDSmokeVolumeEntity->KeyValue("Color1", "200 200 200");
	pDSmokeVolumeEntity->KeyValue("Color2", "255 255 255");
	pDSmokeVolumeEntity->KeyValue("Density", "1");
	pDSmokeVolumeEntity->KeyValue("DensityRampSpeed", "1");
	pDSmokeVolumeEntity->KeyValue("material", "particle/particle_smokegrenade");
	pDSmokeVolumeEntity->KeyValue("MovementSpeed", "2");
	pDSmokeVolumeEntity->KeyValue("ParticleDrawWidth", "128");
	pDSmokeVolumeEntity->KeyValue("ParticleSpacingDistance", "100");
	pDSmokeVolumeEntity->KeyValue("RotationSpeed", "2");
	pDSmokeVolumeEntity->KeyValue("spawnflags", "0");
	//pDSmokeVolumeEntity->KeyValue("targetname", "");

	pDSmokeVolumeEntity->Precache();
	DispatchSpawn(pDSmokeVolumeEntity);
	pDSmokeVolumeEntity->Activate();

	pDSmokeVolumeEntity->SetTransmitState(FL_EDICT_ALWAYS);

	//inputdata_t myInputData;
	//pDSmokeVolumeEntity->InputTurnOn(myInputData);
}
ConCommand smoke_test("smoke_test", SmokeTest, "For internal use only.");

	// OBSOLETE TEST FUNC!
void SetTransmitStateSV(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (pShortcut)
	{
		bool bState = (Q_atoi(args[2]) != 0);
		//Msg("Transmit State Default: %i\n", pShortcut->GetTransmitState());// 421
		if (bState)
		{
			pShortcut->SetTransmitState(FL_EDICT_ALWAYS);
		}
		else {
			pShortcut->SetTransmitState(421);
		}
	}
}
ConCommand set_transmit_state_sv("set_transmit_state_sv", SetTransmitStateSV, "Change the transmit state of an entity between FL_EDICT_ALWAYS and default.");

void SpawnShortcut(const CCommand &args)
{
	Vector origin(Q_atof(args[5]), Q_atof(args[6]), Q_atof(args[7]));
	QAngle angles(Q_atof(args[8]), Q_atof(args[9]), Q_atof(args[10]));

	// Now spawn it
	CPropShortcutEntity *pShortcut = dynamic_cast<CPropShortcutEntity*>(CreateEntityByName("prop_shortcut"));

	// Pass in standard key values
	char buf[512];
	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pShortcut->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pShortcut->KeyValue("angles", buf);

	std::vector<std::string> badModels;
	badModels.push_back("models\\sibenik\\sib_table01.mdl");

	std::string modelFile = args[4];
	if (!g_pFullFileSystem->FileExists(modelFile.c_str(), "GAME") || (std::find(badModels.begin(), badModels.end(), modelFile) != badModels.end()))
		modelFile = "models/icons/missing.mdl";
	pShortcut->KeyValue("model", modelFile.c_str());

	pShortcut->KeyValue("solid", "6");		// for 	V_PHYSICS
	//pShortcut->KeyValue("solid", "0");		// for 	V_PHYSICS
	//pShortcut->KeyValue("solid", "2");
	//pShortcut->KeyValue("modelscale", args[11]);

	//pShortcut->KeyValue("fademindist", "-1");
	//pShortcut->KeyValue("fademaxdist", "0");
	pShortcut->KeyValue("fademindist", aapropfademin.GetString());
	pShortcut->KeyValue("fademaxdist", aapropfademax.GetString());
	pShortcut->KeyValue("fadescale", "1");

	pShortcut->KeyValue("MinAnimTime", "5");
	pShortcut->KeyValue("MaxAnimTime", "10");
	pShortcut->KeyValue("DisableBoneFollowers", "1");
	
	int iSpawnFlags = 8;
	pShortcut->KeyValue("spawnflags", UTIL_VarArgs("%i", iSpawnFlags));
	pShortcut->KeyValue("objectId", args[1]);
	pShortcut->KeyValue("itemId", args[2]);
	pShortcut->KeyValue("modelId", args[3]);
	pShortcut->KeyValue("slave", args[12]);
	//pShortcut->KeyValue("body", args[13]);
	//pShortcut->KeyValue("skin", args[14]);

	int iParentEntityIndex = Q_atoi(args[15]);
	bool bShouldGhost = (Q_atoi(args[16]) != 0);
	bool bIsNewObject = (Q_atoi(args[17]) != 0);

	pShortcut->Precache();
	DispatchSpawn(pShortcut);
	pShortcut->Activate();

	int iBody = Q_atoi(args[13]);
	int iSkin = Q_atoi(args[14]);
	//DevMsg("Skin & Body: %i %i\n", iSkin, iBody);

	pShortcut->m_nBody.Set(iBody);
	pShortcut->m_nSkin.Set(iSkin);

	//pShortcut->SetModelScale(fScale, 0);

//	pShortcut->SetMoveType(MOVETYPE_NONE);

	CBaseEntity* pParentEntity = (iParentEntityIndex >= 0) ? pParentEntity = CBaseEntity::Instance(iParentEntityIndex) : NULL;

	if (!bIsNewObject && (!pParentEntity || pParentEntity->GetSolid() != SOLID_NONE))
	{
		pShortcut->SetSolid(SOLID_VPHYSICS);

		bool bSpawnNoMotion = (iSpawnFlags == 8);
		if (bSpawnNoMotion)
			pShortcut->CreateVPhysics();
		else
			pShortcut->CreateVPhysicsMotion();

		//IPhysicsObject* pPhysics = pShortcut->VPhysicsGetObject();
		//if (!pPhysics && pShortcut->CreateVPhysics())
		//	pPhysics = pShortcut->VPhysicsGetObject();

		IPhysicsObject* pPhysics = pShortcut->VPhysicsGetObject();
		if (pPhysics)
		{
			if (iSpawnFlags == 8)
				pPhysics->EnableMotion(false);
			else
				pPhysics->EnableMotion(true);
		}
	}
	else
	{
		if (bShouldGhost)
		{
			pShortcut->SetRenderMode(kRenderTransColor);
			pShortcut->SetRenderColorA(160);
		}
	}

	pShortcut->SetMoveType(MOVETYPE_NONE);

	float fScale = Q_atof(args[11]);
	if (bIsNewObject || fScale != 1.0)
	{
		if (scale_collisions.GetBool() && pShortcut->VPhysicsGetObject())
		{
			UTIL_CreateScaledPhysObject(pShortcut->GetBaseAnimating(), fScale);		

			//pShortcut->SetSize(pShortcut->WorldAlignMins(), pShortcut->WorldAlignMaxs());

			//Vector mins = pShortcut->CollisionProp()->OBBMins();
			//Vector maxs = pShortcut->CollisionProp()->OBBMins();
			//DevMsg("Mins & Maxs: %f/%f/%f vs %f/%f/%f\n", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
			//pShortcut->SetSize(mins, maxs);

			pShortcut->CollisionProp()->RefreshScaledCollisionBounds();
			pShortcut->NetworkStateChanged();
		}
		else
			pShortcut->SetModelScale(fScale, 0);
	}

	if (pParentEntity)
		pShortcut->SetParent(pParentEntity, -1);

	if ( bIsNewObject || (pParentEntity && pParentEntity->GetSolid() == SOLID_NONE))
		pShortcut->SetSolid(SOLID_NONE);
	//pShortcut->KeyValue("body", args[13]);
	//pShortcut->KeyValue("skin", args[14]);



	// CHECK THIS OUT WHEN YOU'RE FIXING SCALED COLLISION MESHES (AGAIN, DOH)
	//pShortcut->UpdateModelScale();



	bool bIsAutoplayObject = (Q_atoi(args[18]) != 0);
	if (bIsAutoplayObject)
	{
		pShortcut->SetTransmitState(FL_EDICT_ALWAYS);
		//pShortcut->DispatchUpdateTransmitState();
	}
		//pShortcut->GetNetworkable()->GetEdict()->StateChanged();
	//pShortcut->NetworkStateChanged();

//	pShortcut->SetModelScale(Q_atof(args[9]), 0);
	/*	// from server-side code...
	float flScale = Q_atof(args[2]);
	float flMin = Q_atof(args[3]);
	float flMax = Q_atof(args[4]);

	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
	{
		C_PropHotlinkEntity* pHotlink = dynamic_cast<CPropHotlinkEntity*>(pEntity);
		pHotlink->SetModelScale(flScale, 0.0f);
		*/
	//pShortcut->NetworkStateChanged();
}
ConCommand spawnshortcut("spawnshortcut", SpawnShortcut, "Spawns a shortcut.", FCVAR_NONE);

void SpawnInstance(const CCommand &args)
{
	DevMsg("Spawn Instance: %s\n", args[1]);
}
ConCommand spawninstance("spawninstance", SpawnInstance, "Spawns an entire instance.  Many shortcuts.", FCVAR_NONE);

void SetSkin(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (pShortcut)
	{
		int iSkinValue = Q_atoi(args[2]);
		pShortcut->m_nSkin.Set(iSkinValue);
	}
}
ConCommand setskin("setskin", SetSkin, "Internal.", FCVAR_NONE);

void SetBodyGroup(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (pShortcut)
	{
		int iBodyGroup = Q_atoi(args[2]);
		int iBodyGroupValue = Q_atoi(args[3]);
		pShortcut->SetBodygroup(iBodyGroup, iBodyGroupValue);
	}
}
ConCommand setbodygroup("setbodygroup", SetBodyGroup, "Internal.", FCVAR_NONE);

void SetBody(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (pShortcut)
	{
		int iBody = Q_atoi(args[2]);
		pShortcut->m_nBody.Set(iBody);
	}
}
ConCommand setbody("setbody", SetBody, "Internal.", FCVAR_NONE);

void SetParent(const CCommand &args)
{
	CBaseEntity* pChildEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	CBaseEntity* pParentEntity = (args.ArgC() > 2) ? CBaseEntity::Instance(Q_atoi(args[2])) : pChildEntity;
	pChildEntity->SetParent(pParentEntity);
}
ConCommand setparent("setparent", SetParent, "Sets the entity's parent.", FCVAR_NONE);

int iSelectorEntityIndex = -1;
void SelectorTrace(const CCommand &args)
{
	CBasePlayer* pPlayerBaseEntity = dynamic_cast<CBasePlayer*>(UTIL_GetCommandClient());

	if (!pPlayerBaseEntity || pPlayerBaseEntity->GetHealth() <= 0)
	{
		iSelectorEntityIndex = -1;
		return;
	}

	// arg 1 is a starting entity index
	CBaseEntity* pStartingEntity = NULL;

	if (args.ArgC() > 1)
	{
		pStartingEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	}

	trace_t tr;
	if (!pStartingEntity)
	{
		Vector vForward;
		pPlayerBaseEntity->EyeVectors(&vForward);
		UTIL_TraceLine(pPlayerBaseEntity->EyePosition(), pPlayerBaseEntity->EyePosition() + vForward * MAX_COORD_RANGE, MASK_SOLID, pPlayerBaseEntity, COLLISION_GROUP_NONE, &tr);
	}
	else
	{
		Vector vForward;
		Vector vLeft;
		Vector vUp;
		pStartingEntity->GetVectors(&vForward, &vLeft, &vUp);
		UTIL_TraceLine(pStartingEntity->GetAbsOrigin(), pStartingEntity->GetAbsOrigin() + (vForward + (vUp*-1.0)) * MAX_COORD_RANGE, MASK_SOLID, pPlayerBaseEntity, COLLISION_GROUP_NONE, &tr);
	}

	// No hit? We're done.
	if (tr.fraction == 1.0)
	{
		iSelectorEntityIndex = -1;
		return;
	}

	Vector normal;
	int iIndex = -1;
	if (tr.DidHitNonWorldEntity())
	{
		iIndex = tr.m_pEnt->entindex();
		iSelectorEntityIndex = iIndex;
	}
	else
		iSelectorEntityIndex = -1;

	normal.x = tr.plane.normal.x;
	normal.y = tr.plane.normal.y;
	normal.z = tr.plane.normal.z;

	edict_t *pClient = engine->PEntityOfEntIndex(UTIL_GetCommandClient()->entindex());
	engine->ClientCommand(pClient, UTIL_VarArgs("selector_value %i %f %f %f %f %f %f;\n", iIndex, tr.endpos.x, tr.endpos.y, tr.endpos.z, normal.x, normal.y, normal.z));

	/*
	CBaseEntity* pEntity = gEntList.FindEntityByClassnameWithin(pStartingEntity, "prop_hotlink", tr.endpos, 200);
	if (pEntity)
	{
		edict_t *pClient = engine->PEntityOfEntIndex(UTIL_GetCommandClient()->entindex());
		engine->ClientCommand(pClient, UTIL_VarArgs("slyselect %i;\n", pEntity->entindex()));
	}
	else {
		pEntity = gEntList.FindEntityByClassnameWithin(NULL, "prop_hotlink", tr.endpos, 200);

		if (pEntity)
		{
			edict_t *pClient = engine->PEntityOfEntIndex(UTIL_GetCommandClient()->entindex());
			engine->ClientCommand(pClient, UTIL_VarArgs("slyselect %i;\n", pEntity->entindex()));
		}
	}*/
}
ConCommand selectortrace("selector_trace", SelectorTrace, "Usegae: Find out what the local user is looking at.", FCVAR_HIDDEN);

void ServerTesterJoint(const CCommand &args)
{
	Msg("Testing...\n");
	Msg("Server values for:\n\tMAX_EDICT_BITS\t%i\n\tMAX_EDICTS\t%i\n\tNUM_ENT_ENTRY_BITS\t%i\n\tNUM_ENT_ENTRIES\t%i\n", MAX_EDICT_BITS, MAX_EDICTS, NUM_ENT_ENTRY_BITS, NUM_ENT_ENTRIES);
}
ConCommand server_testerjoint("server_testerjoint", ServerTesterJoint, "Usegae: Server tester joint.", FCVAR_NONE);

void ShowHubsMenu(const CCommand &args)
{
	//Msg("Nodes are currently disabled in Redux.  They will be re-enabled when I finish coding support for them.\n");
	//return;

	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();

	/*
	CBaseEntity* pPlayerBaseEntity = UTIL_GetCommandClient();
	CHL2MP_Player* pMPPlayer = dynamic_cast<CHL2MP_Player*>(pPlayerBaseEntity);
	*/

	// We are given the button entity, who's parent is the node_info entity.
	CBaseEntity* pButtonEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	CBaseEntity* pNodeInfoEntity = pButtonEntity->GetParent();

	// Find the node volume
	CTriggerMultiple* pTrigger = NULL;
	CBaseEntity* pTriggerEntity = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "trigger_multiple");
	while (pTriggerEntity)
	{
		pTrigger = dynamic_cast<CTriggerMultiple*>(pTriggerEntity);
		if (pTrigger && pTrigger->GetEnabled() && Q_strcmp(STRING(pTrigger->GetEntityName()), "") && !Q_stricmp(STRING(pTrigger->m_iFilterName), "filterHubEntities") && pTrigger->PointIsWithin(pNodeInfoEntity->GetAbsOrigin()) ) //(!pNodeInfoEntity && pTrigger->PointIsWithin(pMPPlayer->GetAbsOrigin())) || (pAltEntity && pTrigger->PointIsWithin(pAltEntity->GetAbsOrigin())))
		{
			// Node volume found
			break;
		}
		else
		{
			pTrigger = NULL;
			pTriggerEntity = gEntList.FindEntityByClassname(pTriggerEntity, "trigger_multiple");
		}
	}

	if (!pTrigger)
	{
		DevMsg("ERROR: Could not find node volume!\n");
		return;
	}

	// We now have the node_info entity and the trigger volume entity
	// Let's create the KV that will be saved out to a file for the client-side code to load.
	KeyValues* pNodeKV = new KeyValues("node");
	KeyValues* pNodeSetupKV = pNodeKV->FindKey("setup", true);
	pNodeSetupKV->SetString("style", pTriggerEntity->GetEntityName().ToCStr());

	Vector origin = pNodeInfoEntity->GetAbsOrigin();
	QAngle angles = pNodeInfoEntity->GetAbsAngles();

	char buf[512];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pNodeSetupKV->SetString("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pNodeSetupKV->SetString("angles", buf);

	// these 3 are only useful server-side!
	pNodeSetupKV->SetInt("buttonIndex", pButtonEntity->entindex());
	pNodeSetupKV->SetInt("nodeInfoIndex", pNodeInfoEntity->entindex());
	pNodeSetupKV->SetInt("triggerIndex", pTrigger->entindex());

	// Next, determine if the trigger volume is empty.
	// If it isn't, push all of the entity indexes into the KV file for the client-side code to load.
	KeyValues* pNodeObjectsKV = pNodeSetupKV->FindKey("objects", true);
	KeyValues* pNodeObjectKV;
	CPropShortcutEntity* pShortcut;
	CBaseEntity* testEntity = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "prop_shortcut");
	while (testEntity)
	{
		if (pTrigger->PointIsWithin(testEntity->GetAbsOrigin()))
		{
			pShortcut = dynamic_cast<CPropShortcutEntity*>(testEntity);
			if (pShortcut)
			{
				//bEmptyVolume = false;
				pNodeObjectKV = pNodeObjectsKV->CreateNewKey();
				pNodeObjectKV->SetName("prop_shortcut");
				pNodeObjectKV->SetInt(NULL, pShortcut->entindex());
				//shortcuts.push_back(pShortcut);
			}
		}

		testEntity = gEntList.FindEntityByClassname(testEntity, "prop_shortcut");
	}

	pNodeKV->SaveToFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"); 

	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	engine->ClientCommand( pClient, "showhubsmenuclient\n" );
}
ConCommand showhubsmenu("showhubsmenu", ShowHubsMenu, "Shows the HUBs menu.", FCVAR_HIDDEN);

void ShowHubSaveMenu(const CCommand &args)
{
	KeyValues* pNodeInfoKV = new KeyValues("node");
	if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Could not load nodevolume.txt!\n");
		return;
	}

	// Find the info shortcut for the node.
	//KeyValues* pInstanceKV = NULL;
	CBaseEntity* pInfoBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pInfoBaseEntity)
	{
		DevMsg("ERROR: Could not find info object within node command.\n");
		return;
	}

	CPropShortcutEntity* pInfoShortcut = dynamic_cast<CPropShortcutEntity*>(pInfoBaseEntity);
	if (!pInfoShortcut)
	{
		DevMsg("ERROR: Could not cast base entity to shortcut entity for node.\n");
		return;
	}

	CBaseEntity* pBaseEntity;
	CPropShortcutEntity* pPropShortcutEntity;
	for (KeyValues *sub = pNodeInfoKV->FindKey("setup/objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = CBaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		// don't try to parent ourselves to ourselves.
		if (pPropShortcutEntity == pInfoShortcut)
			continue;

		// these entities already exist.  parent them to the info object, then tell the client-side code when we're done.
		pPropShortcutEntity->SetParent(pInfoShortcut);
	}

	pNodeInfoKV->deleteThis();

	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	engine->ClientCommand(pClient, UTIL_VarArgs("showhubsavemenuclient %i;\n", pInfoShortcut->entindex()));
}
ConCommand showhubsavemenu("showhubsavemenu", ShowHubSaveMenu, "Shows the HUB save menu.", FCVAR_HIDDEN);

/*
// WE are given: [1]itemfile [2]model [3]origin [4]angles [5]nophysics [6]allowplacement
void Create_Hotlink(const CCommand &args)
{
	// arg 1: item
	// arg 2: model
	bool bAbsoluteLocation = false;

	if (args.ArgC() > 3)
	{
		bAbsoluteLocation = true;
	}

	char modelFile[256];
	if (args.ArgC() > 2 && Q_strcmp(args.Arg(2), ""))
		strcpy(modelFile, args.Arg(2));
	else if (args.ArgC() > 1 && Q_strcmp(args.Arg(1), ""))
		strcpy(modelFile, args.Arg(1));
	else
		strcpy(modelFile, "models/cabinets/two_player_arcade.mdl");

	if (!g_pFullFileSystem->FileExists(modelFile))
		strcpy(modelFile, "models/icons/missing.mdl");

	if (!engine->IsModelPrecached(modelFile))
		engine->PrecacheModel(modelFile, true);

	if (!bAbsoluteLocation)
	{
		// Figure out where to place it
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		Vector forward;
		pPlayer->EyeVectors(&forward);

		trace_t tr;
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_NPCSOLID,
			pPlayer, COLLISION_GROUP_NONE, &tr);

		// No hit? We're done.
		if (tr.fraction == 1.0)
			return;

		VMatrix entToWorld;
		Vector xaxis;
		Vector yaxis;

		if (tr.plane.normal.z == 0.0f)
		{
			yaxis = Vector(0.0f, 0.0f, 1.0f);
			CrossProduct(yaxis, tr.plane.normal, xaxis);
			entToWorld.SetBasisVectors(tr.plane.normal, xaxis, yaxis);
		}
		else
		{
			Vector ItemToPlayer;
			VectorSubtract(pPlayer->GetAbsOrigin(), Vector(tr.endpos.x, tr.endpos.y, tr.endpos.z), ItemToPlayer);

			xaxis = Vector(ItemToPlayer.x, ItemToPlayer.y, ItemToPlayer.z);

			CrossProduct(tr.plane.normal, xaxis, yaxis);
			if (VectorNormalize(yaxis) < 1e-3)
			{
				xaxis.Init(0.0f, 0.0f, 1.0f);
				CrossProduct(tr.plane.normal, xaxis, yaxis);
				VectorNormalize(yaxis);
			}
			CrossProduct(yaxis, tr.plane.normal, xaxis);
			VectorNormalize(xaxis);

			entToWorld.SetBasisVectors(xaxis, yaxis, tr.plane.normal);
		}

		QAngle angles;
		MatrixToAngles(entToWorld, angles);

		// Now spawn it
		CPropHotlinkEntity *pProp = dynamic_cast<CPropHotlinkEntity*>(CreateEntityByName("prop_hotlink"));

		CArcadeResources* pServerArcadeResources = CArcadeResources::GetSelf();

		// Pass in standard key values
		char buf[512];
		// Pass in standard key values
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", tr.endpos.x, tr.endpos.y, tr.endpos.z);
		pProp->KeyValue("origin", buf);
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
		pProp->KeyValue("angles", buf);

		pProp->KeyValue("model", modelFile);
		//pProp->KeyValue("solid", "6");		// for 	V_PHYSICS
		pProp->KeyValue("solid", "0");
		pProp->KeyValue("fademindist", "-1");
		pProp->KeyValue("fademaxdist", "0");
		pProp->KeyValue("fadescale", "1");
		pProp->KeyValue("MinAnimTime", "5");
		pProp->KeyValue("MaxAnimTime", "10");
		pProp->KeyValue("spawnflags", "8");
		pProp->KeyValue("IndexOfKV", UTIL_VarArgs("%i", pServerArcadeResources->GetHotlinkCount()));

		DispatchSpawn(pProp);

		pServerArcadeResources->IncrementHotlinkCount();
	}
	else
	{
		// Now spawn it
		CPropHotlinkEntity *pProp = dynamic_cast<CPropHotlinkEntity*>(CreateEntityByName("prop_hotlink"));

		CArcadeResources* pServerArcadeResources = CArcadeResources::GetSelf();

		// Pass in standard key values
		pProp->KeyValue("origin", args[3]);
		pProp->KeyValue("angles", args[4]);
		pProp->KeyValue("model", modelFile);
		//		pProp->KeyValue("solid", "6");		// for 	V_PHYSICS
		pProp->KeyValue("solid", "0");
		pProp->KeyValue("fademindist", "-1");
		pProp->KeyValue("fademaxdist", "0");
		pProp->KeyValue("fadescale", "1");
		pProp->KeyValue("MinAnimTime", "5");
		pProp->KeyValue("MaxAnimTime", "10");

		//pProp->KeyValue("spawnflags", "8");

		if (Q_atoi(args[5]) == 1)
			pProp->KeyValue("spawnflags", "8");

		pProp->KeyValue("IndexOfKV", UTIL_VarArgs("%i", pServerArcadeResources->GetHotlinkCount()));

		DispatchSpawn(pProp);

		pServerArcadeResources->IncrementHotlinkCount();

		if (Q_atoi(args[6]) == 0)
		{
			pProp->VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);

			IPhysicsObject* pPhysics = pProp->VPhysicsGetObject();
			if (pPhysics)
			{
				if (pProp->GetSpawnFlags() == 8)
					pPhysics->EnableMotion(false);
				else
					pPhysics->EnableMotion(true);
			}

			//pProp->SetCollisionGroup(COLLISION_GROUP_NONE);

			//////////////////
			pProp->SetSolid(SOLID_VPHYSICS);

			IPhysicsObject* pPhysics = pProp->VPhysicsGetObject();
			if (!pPhysics && pProp->CreateVPhysics())
			pPhysics = pProp->VPhysicsGetObject();

			if (pPhysics)
			{
			if (pProp->GetSpawnFlags() == 8)
			pPhysics->EnableMotion(false);
			else
			pPhysics->EnableMotion(true);
			}
			/////////////////////////
		}
	}

	//	if( args.ArgC() > 2 )
	//	{
	//		edict_t *pClient = engine->PEntityOfEntIndex( pPlayer->entindex() );
	//		engine->ClientCommand( pClient, UTIL_VarArgs( "new_item \"%s\" \"%s\"\n", args.Arg(1), args.Arg(2) ) );
	//	}
	//	else if( args.ArgC() > 1 )
	//	{
	//		edict_t *pClient = engine->PEntityOfEntIndex( pPlayer->entindex() );
	//		engine->ClientCommand( pClient, UTIL_VarArgs( "new_item \"%s\"\n", args.Arg(1) ) );
	//	}
}

ConCommand prop_hotlink_create("prop_hotlink_create", Create_Hotlink, "Create a dynamic item under your crosshairs.");
*/

void SetPlayerModel(const CCommand &args)
{
	CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
	std::string model = args[1];

	int notCached = 0;
	if (!engine->IsModelPrecached(model.c_str()))
		notCached = pRequestingPlayer->PrecacheModel(model.c_str());

	if ( notCached >= 0)
		UTIL_SetModel(pRequestingPlayer, model.c_str());
}

ConCommand setplayermodel("setplayermodel", SetPlayerModel, "For internal use only.");

#include "../hl2/npc_citizen17.h"
void DoNPCMove(const CCommand &args)
{
	CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
	//pRequestingPlayer->ClearActiveWeapon();////>SetActiveWeapon(NULL);
	//return;

	Vector targetPosition = Vector(Q_atof(args[1]), Q_atof(args[2]), Q_atof(args[3]));
	//Vector targetPosition = pRequestingPlayer->GetAbsOrigin();

	//Vector forward;
	//Vector right;
	//Vector up;
	//pRequestingPlayer->GetVectors(&forward, &right, &up);
	//targetPosition += (forward * 100.0);

	/*
	// First, grab this NPC's AA target path_corner
	CBaseEntity* pTargetEntity = gEntList.FindEntityByName(NULL, "aa_target_path_corner__");
	if (!pTargetEntity)
	{
		// It doesn't exist yet.  Create it right now.
		KeyValues* pEntityKV = new KeyValues("entity");
		pEntityKV->SetString("classname", "path_corner");
		pEntityKV->SetString("targetname", "aa_target_path_corner__");	// own name
		pEntityKV->SetString("spawnflags", "0");
		pEntityKV->SetString("speed", "0");
		pEntityKV->SetString("wait", "0");
		pEntityKV->SetString("yaw_speed", "0");
		pEntityKV->SetString("target", "");	// next stop name

		pTargetEntity = CreateEntityByName(pEntityKV->GetString("classname"));

		char buf[512];
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", targetPosition.x, targetPosition.y, targetPosition.z);
		pTargetEntity->KeyValue("origin", buf);

		pTargetEntity->KeyValue("angles", "0 0 0");

		pTargetEntity->Precache();
		DispatchSpawn(pTargetEntity);
	}
	*/

	//std::vector<CBaseEntity*> victimEntities;
	CBaseEntity* pVictimEntity = gEntList.FindEntityByClassname(NULL, "npc_citizen");
	while (pVictimEntity)
	{
		//victimEntities.push_back(pVictimEntity);
		CNPC_Citizen* pCitizen = dynamic_cast<CNPC_Citizen*>(pVictimEntity);
		if (pCitizen)
		{
			//pCitizen->ForceSelectedGoRandom();
			//pCitizen->KeyValue("target", "aa_target_path_corner__");
			//pCitizen->SetTarget(pTargetEntity);
			//pCitizen->ClearCommandGoal();
			//pCitizen->SetGoalEnt(pTargetEntity);
			//pCitizen->SetCommandGoal()
			//pCitizen->UpdateTargetPos(
			//pCitizen->TaskInterrupt();
			//pCitizen->SetState(NPC_STATE_IDLE);
			//pCitizen->SetTarget(pRequestingPlayer);
			//pCitizen->ScheduledMoveToGoalEntity(SCHED_IDLE_WALK, pRequestingPlayer, ACT_WALK);
			//pCitizen->SetGoalEnt(pRequestingPlayer);
			//pCitizen->Goal
			//pCitizen->SetActivity(ACT_RESET);
			//pCitizen->SetState(NPC_STATE_IDLE);// NPC_STATE_NONE);
			//pCitizen->SetActivity(ACT_IDLE);
			//pCitizen->SetActivity(ACT_WALK);
			//pCitizen->SetIdealActivity(ACT_WALK);
			//pCitizen->SetIdealActivity(ACT_WALK);
			//pCitizen->SetIdealState(NPC_STATE_IDLE);
			//pCitizen->TaskComplete(true);

			///*
			//pCitizen->SetTarget(pRequestingPlayer);

			//pCitizen->ClearAllSchedules();
			//pCitizen->ScheduledMoveToGoalEntity(SCHED_IDLE_WALK, pRequestingPlayer, ACT_WALK);

			//pCitizen->ResetActivity();

			//pCitizen->SetNavIgnore();





			CAI_BaseNPC* pYeah = pCitizen->CreateCustomTarget(targetPosition, 10.0f);
			pCitizen->SetTarget(pYeah);
			pCitizen->ScheduledMoveToGoalEntity(0, pYeah, ACT_WALK);




			//pCitizen->VelocityPunch(forward * 100.0f);
			//pCitizen->AccumulateIdealYaw(pCitizen->VecToYaw(forward), 1.0f);
			//pCitizen->AddLookTarget(targetPosition, 10.0f, 1.0f);
			//pCitizen->MoveOrder(targetPosition, NULL, 0);











			//pCitizen->AutoMovement(pYeah);

			//pCitizen->TaskInterrupt();
			/*Task_t* pMyTask = new Task_t();
			pMyTask->flTaskData = 0;
			//pMyTask->iTask = TASK_GET_PATH_TO_PLAYER;
			//pMyTask->iTask = TASK_SCRIPT_WALK_TO_TARGET;
			pMyTask->iTask = TASK_SCRIPT_WALK_TO_TARGET;
			pCitizen->StartTask(pMyTask);
			//pCitizen->RunTask(pMyTask);
			delete pMyTask;*/


			//pCitizen->SetTarget(pRequestingPlayer);
			//pCitizen->SetGoalEnt(pRequestingPlayer);
			//pCitizen->Goal
			//pCitizen->SetActivity(ACT_RESET);
			//pCitizen->SetState(NPC_STATE_IDLE);// NPC_STATE_NONE);
			//pCitizen->SetActivity(ACT_WALK);

			//pCitizen->TaskInterrupt();
			//pCitizen->KeyValue("target", "aa_target_path_corner__");
			//pCitizen->AddFlag(65536);
			//pCitizen->Move
			//pCitizen->SetTarget(pTargetEntity);
			//pCitizen->SetTarget(pRequestingPlayer);
			//pCitizen->FollowEntity(pRequestingPlayer);
		}

		pVictimEntity = gEntList.FindEntityByClassname(pVictimEntity, "npc_citizen");
	}
}
ConCommand donpcmove("donpcmove", DoNPCMove, "For internal use only.  Tells all nearby NPC's to move to the specified coordinates.");

void SetEntityModel(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pEntity)
		return;

	std::string model = args[2];

	int notCached = 0;
	if (!engine->IsModelPrecached(model.c_str()))
		notCached = pEntity->PrecacheModel(model.c_str());

	if (notCached >= 0)
		UTIL_SetModel(pEntity, model.c_str());
}
ConCommand setentitymodel("setentitymodel", SetEntityModel, "For internal use only.");

void SetIfPointInsideNode(const CCommand &args)
{
	std::string nodeStyle = args.Arg(1);
	if (nodeStyle.length() < 5)
		return;

	nodeStyle = nodeStyle.substr(5);

	//DevMsg("Nodestye: %s\n", nodeStyle.c_str());
	Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	//CBaseEntity* pBaseEntity = CBaseEntity::Instance(Q_atoi(args[2]));
	//Vector origin = pBaseEntity->GetAbsOrigin();


	//CBaseEntity* pButtonEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	CBaseEntity* pNodeInfoEntity;// = pButtonEntity->GetParent();

	// Find the node volume
	CTriggerMultiple* pTrigger = NULL;
	CBaseEntity* pTriggerEntity = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "trigger_multiple");
	while (pTriggerEntity)
	{
		pTrigger = dynamic_cast<CTriggerMultiple*>(pTriggerEntity);
	//	DevMsg("NOde %s: %s - %s/n", nodeStyle.c_str(), pTrigger->GetEntityName().ToCStr(), STRING(pTrigger->m_iFilterName));
		if (pTrigger && pTrigger->GetEnabled() && Q_strcmp(STRING(pTrigger->GetEntityName()), "") && !Q_stricmp(STRING(pTrigger->m_iFilterName), "filterHubEntities") && pTrigger->PointIsWithin(origin) && nodeStyle == pTrigger->GetEntityName().ToCStr()) //(!pNodeInfoEntity && pTrigger->PointIsWithin(pMPPlayer->GetAbsOrigin())) || (pAltEntity && pTrigger->PointIsWithin(pAltEntity->GetAbsOrigin())))
		{
			// Node volume found
			//DevMsg("NOde In SPace: %s/n", pTrigger->GetEntityName().ToCStr());

			// FIXME: FInd OUR node_info.
			pNodeInfoEntity = dynamic_cast<CNodeInfoEntity*>(gEntList.FindEntityByClassname(gEntList.FirstEnt(), "node_info"));
			while (pNodeInfoEntity)
			{
				if (pTrigger->PointIsWithin(pNodeInfoEntity->GetAbsOrigin()))
				{
					pointWithinNode.SetValue(pNodeInfoEntity->entindex());
					return;
				}

				pNodeInfoEntity = gEntList.FindEntityByClassname(pNodeInfoEntity, "node_info");
			}
		}
		else
		{
			pTrigger = NULL;
			pTriggerEntity = gEntList.FindEntityByClassname(pTriggerEntity, "trigger_multiple");
		}
	}
	pointWithinNode.SetValue(-1);
}
ConCommand setifpointinsidenode("setifpointnode", SetIfPointInsideNode, "For internal use only.");

void SetCabPos(const CCommand &args)
{
	int TheEntity = Q_atoi(args.Arg(1));

	edict_t *pEdict = INDEXENT(TheEntity);
	if (pEdict && !pEdict->IsFree())
	{
		CBaseEntity* pEntity = GetContainingEntity(pEdict);
		//CPropHotlinkEntity* pEntity = (CPropHotlinkEntity*)GetContainingEntity(pHotlinkEdict);
		Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
		QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

		UTIL_SetOrigin(pEntity, origin, true);
		pEntity->SetAbsAngles(angles);

		Vector vel = Vector(0, 0, 0);
		pEntity->Teleport(&origin, &angles, &vel);

		//pEntity->SetCollisionGroup(COLLISION_GROUP_PLAYER);//>SetSolid(SOLID_NONE);
	}
}

ConCommand setcabpos("setcabpos", SetCabPos, "For internal use only.");

void SetCabAngles(const CCommand &args)
{
	int TheEntity = Q_atoi(args.Arg(1));

	edict_t *pEdict = INDEXENT(TheEntity);
	if (pEdict && !pEdict->IsFree())
	{
		CBaseEntity* pEntity = GetContainingEntity(pEdict);
		QAngle angles = QAngle(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));

		//UTIL_SetOrigin(pEntity, origin, true);
		pEntity->SetAbsAngles(angles);

		//Vector vel = Vector(0, 0, 0);
		//pEntity->Teleport(&origin, &angles, &vel);
	}
}

ConCommand setcabangles("setcabangles", SetCabAngles, "For internal use only.");

/*
void Peak(const CCommand &args)
{
	//m_fAudioPeak
	float fAudioPeak = Q_atof(args[1]);
	DevMsg("Peak is: %f\n", fAudioPeak);
}
ConCommand peak("peak", Peak, "For internal use only.");
*/

void PrecacheModel(const CCommand &args)
{
	const char* modelFile = args[1];

	if (!engine->IsModelPrecached(modelFile))
	{
		CBaseEntity::PrecacheModel(modelFile);

		CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();

		if (!pRequestingPlayer)
			return;

		edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
		engine->ClientCommand(pClient, "create_model_preview \"%s\";", modelFile);
	}
}
ConCommand precachemodel("precachemodel", PrecacheModel, "For internal use only.");

void SwitchModel(const CCommand &args)
{
	const char* modelId = args[1];
	const char *TheModel = args[2];
	int TheEntity = Q_atoi(args.Arg(3));

	edict_t *pHotlinkEdict = INDEXENT(TheEntity);
	if (pHotlinkEdict && !pHotlinkEdict->IsFree())
	{
		//CPropHotlinkEntity* pHotlink = (CPropHotlinkEntity*)GetContainingEntity(pHotlinkEdict);
		CBaseEntity* pEntity = GetContainingEntity(pHotlinkEdict);
		CPropShortcutEntity* pHotlink = dynamic_cast<CPropShortcutEntity*>(pEntity);

		//CDynamicProp* pEntity = dynamic_cast<CDynamicProp*>(pHotlink);

		if (!engine->IsModelPrecached(TheModel))
		{
			int result = pEntity->PrecacheModel(TheModel, false);
			//DevMsg("Cache result for %s is: %i\n", TheModel, result);

			//			IMaterial* pMaterial;
			//		modelinfo->GetModelMaterials(modelinfo->FindOrLoadModel(TheModel), 1, &pMaterial);
		}

		if (pHotlink)
		{
			pHotlink->SetModelId(std::string(modelId));

			if (pHotlink->VPhysicsGetObject())
				pHotlink->VPhysicsDestroyObject();

			//pHotlink->CleanupCameraEntities();
			//pHotlink->CleanupSequenceEntities();


			//if (!engine->IsModelPrecached(TheModel))
			//int result = pHotlink->PrecacheModel(TheModel);
			//DevMsg("Result: %i\n", result);
			/*
			if (g_pFullFileSystem->FileExists(TheModel, "GAME"))
				DevMsg("File exists: %s\n", TheModel);
			else
				DevMsg("File does NOT exist: %s\n", TheModel);
			*/

			//if (g_pFullFileSystem->FileExists(TheModel, "GAME"))
			//	DevMsg("Maybe now is the time to set animation on guests?\n");

			//int result = engine->PrecacheModel(TheModel);
			//DevMsg("Result: %i\n", result);

			UTIL_SetModel(pHotlink, TheModel);
			pHotlink->SetModel(TheModel);
			pHotlink->SetSolid(SOLID_NONE);
			//pHotlink->SetSize(pHotlink->WorldAlignMins(), pHotlink->WorldAlignMaxs());

			/*
			if (pHotlink->CreateVPhysics())
			{
				IPhysicsObject *pPhysics = pHotlink->VPhysicsGetObject();
				if (pPhysics)
					pPhysics->EnableMotion(false);
			}
			*/

			if (args.ArgC() > 4 && Q_atoi(args[4]) == 1)
			{
				/*
				pEntity->SetSolid(SOLID_NONE);
				pEntity->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
				//SetRenderMode(kRenderTransTexture);
				pEntity->SetRenderMode(kRenderTransColor);
				pEntity->SetRenderColorA(160);
				*/

				pHotlink->SetRenderMode(kRenderTransColor);
				pHotlink->SetRenderColorA(160);
				//engine->ServerCommand(UTIL_VarArgs("makeghost %i;\n", pEntity->entindex()));	// lazy way to make transparent & stuff
			}

			if (args.ArgC() > 5 && Q_atoi(args[5]) == 1)
			{
				//pHotlink->SetMoveType(MOVETYPE_NONE);
				pHotlink->SetSolid(SOLID_VPHYSICS);
				/*
				IPhysicsObject *pPhysics = pHotlink->VPhysicsGetObject();
				if (pPhysics || pHotlink->CreateVPhysics())
				{
					pPhysics = pHotlink->VPhysicsGetObject();
					pHotlink->SetMoveType(MOVETYPE_NONE);

					if (pPhysics)
					{
						pPhysics->EnableMotion(false);

						float fScale = pHotlink->GetBaseAnimating()->GetModelScale();
						if (scale_collisions.GetBool())// && fScale != 1.0f)
						{
							UTIL_CreateScaledPhysObject(pHotlink->GetBaseAnimating(), fScale);
							pHotlink->CollisionProp()->RefreshScaledCollisionBounds();
							pHotlink->GetBaseAnimating()->RefreshCollisionBounds();
						}
					}
				}*/
			}

			pEntity->NetworkStateChanged();
		}
	}
}

ConCommand switchmodel("switchmodel", SwitchModel, "For internal use only.");

/*
void SwitchModel(const CCommand &args)
{
	const char *TheModel = args[1];
	int TheEntity = Q_atoi(args.Arg(2));
	std::string itemValue = args.Arg(3);
	int itemValueLength = itemValue.length();
	char bufItemValue[1024];
	Q_strcpy(bufItemValue, itemValue.c_str());


	CURL* easyhandle = curl_easy_init();
	std::string commands = curl_easy_escape(easyhandle, bufItemValue, itemValueLength);
	curl_easy_cleanup(easyhandle);


	edict_t *pHotlinkEdict = INDEXENT(TheEntity);
	if (pHotlinkEdict && !pHotlinkEdict->IsFree())
	{
		CPropHotlinkEntity* pHotlink = (CPropHotlinkEntity*)GetContainingEntity(pHotlinkEdict);

		//CDynamicProp* pEntity = dynamic_cast<CDynamicProp*>(pHotlink);

		if (!engine->IsModelPrecached(TheModel))
		{
			int result = pHotlink->PrecacheModel(TheModel);
			DevMsg("Cache result for %s is: %i\n\n\n", TheModel, result);

			//			IMaterial* pMaterial;
			//		modelinfo->GetModelMaterials(modelinfo->FindOrLoadModel(TheModel), 1, &pMaterial);
		}

		ConVar* pMPModeVar = cvar->FindVar("mp_mode");

		if (!pMPModeVar->GetBool())
		{
			pHotlink->CleanupCameraEntities();
			pHotlink->CleanupSequenceEntities();
		}

		UTIL_SetModel(pHotlink, TheModel);
		pHotlink->SetModel(TheModel);

		//pHotlink->ResetSequenceInfo();

		std::string realSequenceName = "activated";
		int index = pHotlink->LookupSequence(realSequenceName.c_str());

		if (index == ACT_INVALID)
		{
			realSequenceName = "activeidle";
			index = pHotlink->LookupSequence(realSequenceName.c_str());
		}

		if (index != ACT_INVALID)
		{
			CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();

			if (!pRequestingPlayer)
				return;

			edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());

			pHotlink->ResetSequenceInfo();

			//engine->ClientCommand(pClient, "nextsequenceready %i \"activated\" 1\n", pHotlink->entindex());
			engine->ClientCommand(pClient, "nextsequenceready %i \"%s\" 1;\n", pHotlink->entindex(), realSequenceName.c_str());
		}
	}
}

ConCommand switchmodel("switchmodel", SwitchModel, "For internal use only.");
*/






/*
void SetCabPos(const CCommand &args)
{
	int TheEntity = Q_atoi(args.Arg(1));

	edict_t *pHotlinkEdict = INDEXENT(TheEntity);
	if (pHotlinkEdict && !pHotlinkEdict->IsFree())
	{
		CPropHotlinkEntity* pEntity = (CPropHotlinkEntity*)GetContainingEntity(pHotlinkEdict);
		Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
		QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

		CArcadeResources* pArcadeResources = CArcadeResources::GetSelf();

		// Cycle through all trigger_multiple and test if our point is inside of them...
		CTriggerMultiple* pTriggerMultiple;
		CNodeInfoEntity* pNodeInfo;

		// Check if we have a most-likely trigger to test first...
		CBaseEntity* pBaseMostRecentTrigger = pArcadeResources->GetMostRecentHubTrigger();
		CBaseEntity* pBaseMostRecentNodeInfo = pArcadeResources->GetMostRecentHubNodeInfo();
		if (pBaseMostRecentTrigger)
		{
			// If we have a most recent trigger, then test against it.  If we are, in fact, inside of it, then just return.
			pTriggerMultiple = dynamic_cast<CTriggerMultiple*>(pBaseMostRecentTrigger);
			if (pTriggerMultiple != NULL && pTriggerMultiple->GetEnabled() && pTriggerMultiple->PointIsWithin(Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)))))
			{
				pNodeInfo = dynamic_cast<CNodeInfoEntity*>(pBaseMostRecentNodeInfo);

				if (pNodeInfo && pEntity->GetAbsOrigin() == pNodeInfo->GetAbsOrigin())
					return;
			}
			else
			{
				// If we aren't in that volume anymore, clear it.
				pArcadeResources->SetMostRecentHubTrigger(NULL);
				pArcadeResources->SetMostRecentHubNodeInfo(NULL);
			}

			// Otherwise, fall through.
		}

		bool bVolumeOccupied = false;	// We don't want to spawn 2 hubs in the same volume!
		bool bVolumeIsMatch = false;
		bool bMatchIsLearnVolume = false;

		std::string nameBuf;

		CBaseEntity* pBaseTriggerMultiple = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "trigger_multiple");
		while (pBaseTriggerMultiple)
		{
			pTriggerMultiple = dynamic_cast<CTriggerMultiple*>(pBaseTriggerMultiple);
			if (pTriggerMultiple != NULL && pTriggerMultiple->GetEnabled() && pTriggerMultiple->PointIsWithin(Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)))))
			{
				// Check if the name of this entity is the match to our nodestyle.
				nameBuf = UTIL_VarArgs("%s", pTriggerMultiple->GetEntityName());

				if (!Q_stricmp(UTIL_VarArgs("%s", pTriggerMultiple->GetEntityName()), args.Arg(8)) || nameBuf.find("snap") == 0 || nameBuf.find("autospawn") != std::string::npos)
				{
					// Cycle through all node_info and test if they are inside of this trigger.
					CBaseEntity* pBaseNodeInfo = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "node_info");
					while (pBaseNodeInfo)
					{
						pNodeInfo = dynamic_cast<CNodeInfoEntity*>(pBaseNodeInfo);
						//if (pNodeInfo && pTriggerMultiple->IsTouching(pNodeInfo))
						if (pNodeInfo && pTriggerMultiple->PointIsWithin(pNodeInfo->GetAbsOrigin()))
						{
							if (nameBuf.find("autospawn") != std::string::npos)
								bMatchIsLearnVolume = true;

							bVolumeIsMatch = true;

							// Cycle through all prop_hotlinks and test if any are at the exact same origin and angles as the NodeInfo...
							CBaseEntity* pBaseHotlink = gEntList.FindEntityByClassname(gEntList.FirstEnt(), "prop_hotlink");
							while (pBaseHotlink)
							{
								CPropHotlinkEntity* pHotlink = dynamic_cast<CPropHotlinkEntity*>(pBaseHotlink);
								//if (pHotlink && pTriggerMultiple->IsTouching(pHotlink) && pHotlink->GetAbsOrigin() == pNodeInfo->GetAbsOrigin())	// This is assuming that IsTouching is less expensive than comparing origins.
								if (pHotlink && pHotlink->GetAbsOrigin() == pNodeInfo->GetAbsOrigin())	// This is assuming that IsTouching is less expensive than comparing origins.
								{
									bVolumeOccupied = true;
									break;
								}

								pBaseHotlink = gEntList.FindEntityByClassname(pBaseHotlink, "prop_hotlink");
							}

							break;
						}

						pBaseNodeInfo = gEntList.FindEntityByClassname(pBaseNodeInfo, "node_info");
					}

					if (bVolumeIsMatch)
						break;
				}
			}

			pBaseTriggerMultiple = gEntList.FindEntityByClassname(pBaseTriggerMultiple, "trigger_multiple");
		}

		if (bVolumeOccupied || !bVolumeIsMatch)
		{
			// Otherwise, proceed as usual...
			//		pEntity->SetLocalOrigin(origin);
			//			pEntity->SetAbsOrigin(origin);

			UTIL_SetOrigin(pEntity, origin, true);
			pEntity->SetAbsAngles(angles);

			Vector vel = Vector(0, 0, 0);
			pEntity->Teleport(&origin, &angles, &vel);
		}
		else
		{
			// Otherwise, do some work with pTriggerMultiple, pNodeInfo, and other stuff.
			pArcadeResources->SetMostRecentHubTrigger((CBaseEntity*)pTriggerMultiple);
			pArcadeResources->SetMostRecentHubNodeInfo((CBaseEntity*)pNodeInfo);

			UTIL_SetOrigin(pEntity, pNodeInfo->GetAbsOrigin(), true);
			pEntity->SetAbsAngles(pNodeInfo->GetAbsAngles());

			Vector vel2 = Vector(0, 0, 0);
			Vector origin2 = pNodeInfo->GetAbsOrigin();
			QAngle angle2 = pNodeInfo->GetAbsAngles();

			pEntity->Teleport(&origin2, &angle2, &vel2);

			if (bMatchIsLearnVolume)
			{
				DevMsg("SHOULD AUTO SPAWN!!\n");

				CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();

				edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
				engine->ClientCommand(pClient, "doautospawn;\n");
			}
		}
	}
}

ConCommand setcabpos("setcabpos", SetCabPos, "For internal use only.");
*/

void RemoveObject(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	//CPropShortcutEntity
	//CDynamicProp* pProp = NULL;
	//pProp = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(Q_atoi(args[1])));

	int iIndex = Q_atoi(args[1]);

	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(iIndex));
	if (pShortcut)
	{
		pShortcut->SetSolid(SOLID_NONE);
		pShortcut->VPhysicsDestroyObject();

		pShortcut->CleanupCameraEntities();
		pShortcut->CleanupSequenceEntities();

		inputdata_t emptyDummy;
		pShortcut->InputKillHierarchy(emptyDummy);
	}
	else
	{
		CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(iIndex));
		if (pProp)
		{
			pProp->SetSolid(SOLID_NONE);
			pProp->VPhysicsDestroyObject();

			inputdata_t emptyDummy;
			pProp->InputKillHierarchy(emptyDummy);
			return;
		}

		CBaseEntity* pBaseEntity = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(iIndex));
		if (!pBaseEntity)
		{
			DevMsg("Invalid entindex specified for \"remove\" command: %i\n", iIndex);
			return;
		}

		pBaseEntity->SetSolid(SOLID_NONE);
		pBaseEntity->VPhysicsDestroyObject();

		inputdata_t emptyDummy;
		pBaseEntity->InputKillHierarchy(emptyDummy);
		return;
	}
}
ConCommand removeobject("removeobject", RemoveObject, "Deletes an object from the game.");

void BulkRemovedObject(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	int iInfoShortcutIndex = Q_atoi(args[1]);
	CPropShortcutEntity* pInfoShortcut = (iInfoShortcutIndex >= 0) ? dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1]))) : NULL;

	KeyValues* pNodeInfoKV = new KeyValues("node");
	if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Could not load nodevolume.txt!\n");
		return;
	}

	// Find the info shortcut for the node.
	//KeyValues* pInstanceKV = NULL;
	CBaseEntity* pInfoBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pInfoBaseEntity)
	{
		DevMsg("ERROR: Could not find info object within node command.\n");
		return;
	}

	CBaseEntity* pBaseEntity;
	CPropShortcutEntity* pPropShortcutEntity;
	for (KeyValues *sub = pNodeInfoKV->FindKey("setup/objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = CBaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		// don't try to parent ourselves to ourselves.
		if (pInfoShortcut && (pPropShortcutEntity == pInfoShortcut || pPropShortcutEntity->GetMoveParent() == pInfoShortcut->GetBaseEntity()))
			continue;

		inputdata_t emptyDummy;
		pPropShortcutEntity->InputKillHierarchy(emptyDummy);
	}
	pNodeInfoKV->deleteThis();

	if (pInfoShortcut)
	{
		inputdata_t emptyDummy;
		pInfoShortcut->InputKillHierarchy(emptyDummy);
	}

	//CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	//edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	//engine->ClientCommand(pClient, UTIL_VarArgs("showhubclearedmenuclient %i;\n", pInfoShortcut->entindex()));
}
ConCommand bulkremovedobject("bulkremovedobjects", BulkRemovedObject, "Deletes lots of objects from the game.");

void SetObjectIds(const CCommand &args)
{
	if (args.ArgC() < 3)
		return;

	//CPropShortcutEntity
	//CDynamicProp* pProp = NULL;
	//pProp = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(Q_atoi(args[1])));
	std::string itemId = args[2];
	std::string modelId = args[3];
	std::string modelFile = args[4];
	CPropShortcutEntity* pProp = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (!pProp)
	{
		DevMsg("Invalid entindex specified for \"setobjectitemid\" command!\n");
		return;
	}

	if (pProp->VPhysicsGetObject())
		pProp->VPhysicsDestroyObject();

	pProp->PrecacheModel(modelFile.c_str());
	pProp->SetModel(modelFile.c_str());	// This might need to be done server-side (maybe in addition)
	// does physics need to be adjusted for the new model??
	pProp->SetItemId(itemId);
	pProp->SetModelId(modelId);

	if (args.ArgC() > 5 && Q_atoi(args[5]) != 0)
	{
		/*
		pProp->SetSolid(SOLID_NONE);
		pProp->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		//SetRenderMode(kRenderTransTexture);
		pProp->SetRenderMode(kRenderTransColor);
		pProp->SetRenderColorA(160);
		*/

		pProp->SetRenderMode(kRenderTransColor);
		pProp->SetRenderColorA(160);
		
		//engine->ServerCommand(UTIL_VarArgs("makeghost %i;\n", pProp->entindex()));	// lazy way to make transparent & stuff
	}
		//engine->ServerCommand(UTIL_VarArgs("makeghost %i 0;\n", pShortcut->entindex()));	// lazy way to make transparent & stuff

	pProp->NetworkStateChanged();
}
ConCommand setobjectids("setobjectids", SetObjectIds, "");

void SetSlave(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	int iEntityIndex = Q_atoi(args[1]);
	bool bVal = Q_atoi(args[2]);

	CPropShortcutEntity* pProp = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(iEntityIndex));
	if (!pProp)
	{
		DevMsg("Invalid entindex specified for \"setslave\" command!\n");
		return;
	}

	pProp->SetSlave(bVal);
	pProp->NetworkStateChanged();
}
ConCommand setslave("setslave", SetSlave, "");

void SetRenderMode(const CCommand &args)
{
	int iEntityIndex = Q_atoi(args.Arg(1));
	edict_t *pEntityEdict = INDEXENT(iEntityIndex);
	if (pEntityEdict && !pEntityEdict->IsFree())
	{
		CDynamicProp* pEntity = (CDynamicProp*)GetContainingEntity(pEntityEdict);
		if (Q_atoi(args.Arg(2)) == 0)
			pEntity->SetRenderMode(kRenderNone);
		else
			pEntity->SetRenderMode(kRenderTransColor);
		pEntity->NetworkStateChanged();
	}
}
ConCommand set_render_mode("set_render_mode", SetRenderMode, "For internal use only.");

void SetObjectPos(const CCommand &args)
{
	int iEntityIndex = Q_atoi(args.Arg(1));

	edict_t *pEntityEdict = INDEXENT(iEntityIndex);
	if (pEntityEdict && !pEntityEdict->IsFree())
	{
		CDynamicProp* pEntity = (CDynamicProp*)GetContainingEntity(pEntityEdict);
		Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));// + 10.0
		QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));
		float flSpeedFactor = (args.ArgC() > 8) ? Q_atof(args.Arg(8)) : 100.0f;

		// initialize the values we'll spline between
		///*
		Vector vStartPos = pEntity->GetAbsOrigin();
		//float flInterpStartTime = gpGlobals->curtime;
		pEntity->SetAbsVelocity(vec3_origin);
		pEntity->SetLerpSync(origin, angles, flSpeedFactor);
		//*/

		/*
		UTIL_SetOrigin(pEntity, origin, true);
		pEntity->SetAbsAngles(angles);

		Vector vel = Vector(0, 0, 0);
		pEntity->Teleport(&origin, &angles, &vel);
		*/
	}
}
ConCommand set_object_pos("set_object_pos", SetObjectPos, "For internal use only.");

void SnapObjectPos(const CCommand &args)
{
	int iEntityIndex = Q_atoi(args.Arg(1));

	edict_t *pEntityEdict = INDEXENT(iEntityIndex);
	if (pEntityEdict && !pEntityEdict->IsFree())
	{
		CDynamicProp* pEntity = (CDynamicProp*)GetContainingEntity(pEntityEdict);
		Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));// + 10.0
		QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

		// initialize the values we'll spline between
		/*
		Vector vStartPos = pEntity->GetAbsOrigin();
		pEntity->SetAbsVelocity(vec3_origin);
		pEntity->SetLerpSync(origin, angles, 1000.0);
		*/

		///*
		UTIL_SetOrigin(pEntity, origin, true);
		pEntity->SetAbsAngles(angles);

		Vector vel = Vector(0, 0, 0);
		pEntity->Teleport(&origin, &angles, &vel);
		//*/
	}
}
ConCommand snap_object_pos("snap_object_pos", SnapObjectPos, "For internal use only.");

#include "../../game/server/spotlightend.h"
#include "../../game/shared/beam_shared.h"
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

void JumpObjectPos(const CCommand &args)
{
	int iEvenOdd = Q_atoi(args.Arg(1));
	int iEntityIndex = Q_atoi(args.Arg(2));
	int iEntityIndex2 = Q_atoi(args.Arg(3));

	edict_t *pEntityEdict = INDEXENT(iEntityIndex);
	edict_t *pEntityEdict2 = INDEXENT(iEntityIndex2);
	if (pEntityEdict && !pEntityEdict->IsFree() && pEntityEdict2 && !pEntityEdict2->IsFree())
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(iEntityIndex);
		CBaseEntity* pEntity2 = CBaseEntity::Instance(iEntityIndex2);
		if (pEntity && pEntity2)
		{
			CPointSpotlight* pSpotLight = dynamic_cast<CPointSpotlight*>(pEntity);
			CPointSpotlight* pSpotLight2 = dynamic_cast<CPointSpotlight*>(pEntity2);

			pSpotLight->VRSpazzFixPulse();
			pSpotLight2->VRSpazzFixPulse();
			//CDynamicProp* pEntity = (CDynamicProp*)GetContainingEntity(pEntityEdict);
			//Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));// + 10.0
			//QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));
			//CBaseEntity* pParentEntity = pEntity->GetMoveParent();
			/*if (pParentEntity && pParentEntity != pEntity)
			{
				pEntity->SetParent(pEntity);
				pEntity->SetAbsAngles(angles);
				//UTIL_SetOrigin(pEntity, origin);
				pEntity->SetAbsOrigin(origin);
				pEntity->SetParent(pParentEntity);
			}*/

			//pEntity->SetParent(pEntity);

			//pEntity->SetMoveType(MOVETYPE_NOCLIP);
			//pEntity->SetAbsAngles(angles);
			//UTIL_SetOrigin(pEntity, origin);
			//pEntity->SetAbsOrigin(origin);
			//pEntity->SetParent(pParentEntity);


			// initialize the values we'll spline between
			///*
			//Vector vStartPos = pEntity->GetAbsOrigin();
			//pEntity->SetAbsVelocity(vec3_origin);
			//pEntity->SetLerpSync(origin, angles, 1000.0);
			//*/

			/*
			UTIL_SetOrigin(pEntity, origin, true);
			pEntity->SetAbsAngles(angles);

			Vector vel = Vector(0, 0, 0);
			pEntity->Teleport(&origin, &angles, &vel);
			*/
		}
	}
}
ConCommand jump_object_pos("jump_object_pos", JumpObjectPos, "For internal use only.");	// This is specifically about the VR spazz fix?

void CreateVRSpazzFix(const CCommand &args)
{
	int iEntityIndex = (args.ArgC() > 1) ? Q_atoi(args.Arg(1)) : -1;

	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	CBaseEntity* pParentEntity = (iEntityIndex < 0) ? pPlayerEntity : CBaseEntity::Instance(iEntityIndex);
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());

	KeyValues* pEntityKV = new KeyValues("entity");
	pEntityKV->SetString("classname", "point_spotlight");
	pEntityKV->SetString("disablereceiveshadows", "0");
	pEntityKV->SetString("HDRColorScale", "1");
	pEntityKV->SetString("maxdxlevel", "0");
	pEntityKV->SetString("mindxlevel", "0");
	pEntityKV->SetString("renderamt", "0.01");
	pEntityKV->SetString("rendercolor", "255 255 255");
	pEntityKV->SetString("renderfx", "0");
	pEntityKV->SetString("rendermode", "0");
	pEntityKV->SetString("spawnflags", "3");//7
	pEntityKV->SetString("spotlightlength", "150");
	pEntityKV->SetString("spotlightwidth", "50");

	CBaseEntity* pBaseEntity = CreateEntityByName(pEntityKV->GetString("classname"));

	for (KeyValues *pPropertyKV = pEntityKV->GetFirstSubKey(); pPropertyKV; pPropertyKV = pPropertyKV->GetNextKey())
	{
		if (pPropertyKV->GetFirstSubKey() || !Q_stricmp(pPropertyKV->GetName(), "angles") || !Q_stricmp(pPropertyKV->GetName(), "origin"))
			continue;

		pBaseEntity->KeyValue(pPropertyKV->GetName(), pPropertyKV->GetString());
	}

	Vector vecAttachmentPos = pPlayerEntity->GetAbsOrigin();
	vecAttachmentPos.z += 12.0;
	QAngle vecAttachmentAngles = pPlayerEntity->GetAbsAngles();
	vecAttachmentAngles.x = 90;
	vecAttachmentAngles.y = 90;
	vecAttachmentAngles.z = 0;

	char buf[512];

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentPos.x, vecAttachmentPos.y, vecAttachmentPos.z);
	pBaseEntity->KeyValue("origin", buf);

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentAngles.x, vecAttachmentAngles.y, vecAttachmentAngles.z);
	pBaseEntity->KeyValue("angles", buf);

	pBaseEntity->Precache();

	/*Vector min;
	min.x = -MAX_COORD_FLOAT;
	min.y = -MAX_COORD_FLOAT;
	min.z = -MAX_COORD_FLOAT;

	Vector max;
	max.x = MAX_COORD_FLOAT;
	max.y = MAX_COORD_FLOAT;
	max.z = MAX_COORD_FLOAT;

	UTIL_SetSize(pBaseEntity, min, max);*/

	pBaseEntity->SetParent(pParentEntity);
	if (DispatchSpawn(pBaseEntity) > -1)
	{
		pBaseEntity->SetParent(pParentEntity);

		int iFirstIndex = pBaseEntity->entindex();




		pPlayerEntity = UTIL_GetCommandClient();
		pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());

		pEntityKV = new KeyValues("entity");
		pEntityKV->SetString("classname", "point_spotlight");
		pEntityKV->SetString("disablereceiveshadows", "0");
		pEntityKV->SetString("HDRColorScale", "1");
		pEntityKV->SetString("maxdxlevel", "0");
		pEntityKV->SetString("mindxlevel", "0");
		pEntityKV->SetString("renderamt", "0.01");
		pEntityKV->SetString("rendercolor", "255 255 255");
		pEntityKV->SetString("renderfx", "0");
		pEntityKV->SetString("rendermode", "0");
		pEntityKV->SetString("spawnflags", "3");//7
		pEntityKV->SetString("spotlightlength", "150");
		pEntityKV->SetString("spotlightwidth", "50");

		pBaseEntity = CreateEntityByName(pEntityKV->GetString("classname"));

		for (KeyValues *pPropertyKV = pEntityKV->GetFirstSubKey(); pPropertyKV; pPropertyKV = pPropertyKV->GetNextKey())
		{
			if (pPropertyKV->GetFirstSubKey() || !Q_stricmp(pPropertyKV->GetName(), "angles") || !Q_stricmp(pPropertyKV->GetName(), "origin"))
				continue;

			pBaseEntity->KeyValue(pPropertyKV->GetName(), pPropertyKV->GetString());
		}

		vecAttachmentPos = pPlayerEntity->GetAbsOrigin();
		vecAttachmentPos.z += 12.0;
		vecAttachmentAngles = pPlayerEntity->GetAbsAngles();
		vecAttachmentAngles.x = 90;
		vecAttachmentAngles.y = 90;
		vecAttachmentAngles.z = 0;

		//char buf[512];

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentPos.x, vecAttachmentPos.y, vecAttachmentPos.z);
		pBaseEntity->KeyValue("origin", buf);

		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentAngles.x, vecAttachmentAngles.y, vecAttachmentAngles.z);
		pBaseEntity->KeyValue("angles", buf);

		pBaseEntity->Precache();

		pBaseEntity->SetParent(pParentEntity);
		if (DispatchSpawn(pBaseEntity) > -1)
		{
			pBaseEntity->SetParent(pParentEntity);

			CPointSpotlight* pSpotLight = dynamic_cast<CPointSpotlight*>(pBaseEntity);
			pSpotLight->VRSpazzFixPulse();
			//pSpotLight->VRSpazzFixPulse();
			//pSpotLight->SetTransmitState(FL_EDICT_ALWAYS);

			engine->ClientCommand(pClient, UTIL_VarArgs("vr_spazz_fixed %i %i", iFirstIndex, pBaseEntity->entindex()));
		}
	}
}
ConCommand create_vr_spazz_fix("create_vr_spazz_fix", CreateVRSpazzFix, "Fixes the spazz that happens on some maps w/ VR mode.");

void MakeGhost(const CCommand &args)
{
	CBaseEntity* pShortcut = CBaseEntity::Instance(Q_atoi(args[1]));
	//pShortcut->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));	// FIXME: This should be dynamic!!

	pShortcut->SetSolid(SOLID_NONE);	// ALWAYS make non-solid
	if (pShortcut->VPhysicsGetObject())
		pShortcut->VPhysicsDestroyObject();

	bool bShouldGhost = (Q_atoi(args[2]) != 0);
	if ( bShouldGhost )
	{
		pShortcut->SetRenderMode(kRenderTransColor);
		pShortcut->SetRenderColorA(160);
	}
	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		pChild->SetSolid(SOLID_NONE);	// ALWAYS make non-solid
		//if (pChild->VPhysicsGetObject())
		//	pChild->VPhysicsDestroyObject();

		/*
		bool bShouldGhost = (Q_atoi(args[2]) != 0);
		if (bShouldGhost)
		{
			pChild->SetRenderMode(kRenderTransColor);
			pChild->SetRenderColorA(160);
		}
		pChild->NetworkStateChanged();
		*/

		pChild = pChild->NextMovePeer();
	}
}
ConCommand makeghost("makeghost", MakeGhost, "Interal use only.", FCVAR_HIDDEN);

void AutoCameraSelect(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (!pShortcut)
		return;

	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());

	pShortcut->SetWantsCamera(true);
	engine->ClientCommand(pClient, UTIL_VarArgs("select %i 1", pShortcut->entindex()));
}
ConCommand auto_camera_select("autocameraselect", AutoCameraSelect, "Interal use only.", FCVAR_NONE);

void SetAttractModeTransform(const CCommand &args)
{
	Vector origin = Vector(Q_atof(args.Arg(1)), Q_atof(args.Arg(2)), Q_atof(args.Arg(3)));
	QAngle angles = QAngle(Q_atof(args.Arg(4)), Q_atof(args.Arg(5)), Q_atof(args.Arg(6)));


	// Transition Types
	// 0 Auto
	// 1 Interpolate
	// 2 Cut
	int iTransitionType = (args.ArgC() > 7) ? Q_atoi(args.Arg(7)) : 0;

	g_pAnarchyManager->SetAttractModeTransform(origin, angles, iTransitionType);
}
ConCommand set_attract_mode_transform("set_attract_mode_transform", SetAttractModeTransform, "Interal use only.", FCVAR_HIDDEN);

void EndAttractMode(const CCommand &args)
{
	g_pAnarchyManager->EndAttractMode();
}
ConCommand end_attract_mode("end_attract_mode", EndAttractMode, "Interal use only.", FCVAR_HIDDEN);

void TogglePhysics(const CCommand &args)
{
	CBaseEntity* pBaseEntity = (args.ArgC() > 1) ? CBaseEntity::Instance(Q_atoi(args[1])) : NULL;
	if (!pBaseEntity && iSelectorEntityIndex >= 0)
		pBaseEntity = CBaseEntity::Instance(iSelectorEntityIndex);
	//CPhysicsProp* pPhysicsProp = dynamic_cast<CPhysicsProp*>(pBaseEntity);
	if (pBaseEntity)
	{
		CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
		IPhysicsObject* pPhysics = pBaseEntity->VPhysicsGetObject();
		bool bPhysicsWasAlreadyOn = (pPhysics && pPhysics->IsMotionEnabled() && pBaseEntity->GetMoveType() == MOVETYPE_VPHYSICS);
		if (bPhysicsWasAlreadyOn)
		{
			if (pPhysics)
			{
				pPhysics->EnableMotion(false);
				pBaseEntity->VPhysicsDestroyObject();
				pBaseEntity->SetMoveType(MOVETYPE_NONE);
			}

			// reset us too
			CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
			edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
			engine->ClientCommand(pClient, UTIL_VarArgs("resetphysics %i", pShortcut->entindex()));
		}
		else
		{
			if (pPhysics)
				pBaseEntity->VPhysicsDestroyObject();

			bool bPhysicsWasCreated = (pShortcut) ? pShortcut->CreateVPhysicsMotion() : pBaseEntity->CreateVPhysics();
			if (bPhysicsWasCreated)
			{
				pPhysics = pBaseEntity->VPhysicsGetObject();
				if (pPhysics)
				{
					if (pShortcut && scale_collisions.GetBool())
					{
						float fScale = pShortcut->GetModelScale();
						if (fScale != 1.0f)
						{
							UTIL_CreateScaledPhysObject(pShortcut->GetBaseAnimating(), fScale);
							pShortcut->SetSize(pShortcut->WorldAlignMins(), pShortcut->WorldAlignMaxs());
							pShortcut->CollisionProp()->RefreshScaledCollisionBounds();
							pShortcut->NetworkStateChanged();
						}
					}

					pBaseEntity->SetMoveType(MOVETYPE_VPHYSICS);
					pPhysics->EnableMotion(true);
					pPhysics->Wake();
				}
			}
		}
	}
}
ConCommand toggle_physics("togglephysics", TogglePhysics, "Toggles physics on/off for the object under your crosshair.", FCVAR_NONE);

void MakeRagdollNow(const CCommand &args)
{
	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(CBaseEntity::Instance(Q_atoi(args[1])));
	if (!pShortcut)
		return;

	IPhysicsObject* pPhysics = pShortcut->VPhysicsGetObject();
	bool bPhysicsWasAlreadyOn = (pPhysics && pPhysics->IsMotionEnabled() && pShortcut->GetMoveType() == MOVETYPE_VPHYSICS);
	if (bPhysicsWasAlreadyOn)
	{
		DevMsg("Ragdoll is already enabled on this model!\n");

		/*
		if (pPhysics)
		{
			pPhysics->EnableMotion(false);
			pShortcut->VPhysicsDestroyObject();
			pShortcut->SetMoveType(MOVETYPE_NONE);
		}

		// reset us too
		CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
		edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
		engine->ClientCommand(pClient, UTIL_VarArgs("resetphysics %i", pShortcut->entindex()));
		*/
	}
	else
	{
		if (pPhysics)
			pShortcut->VPhysicsDestroyObject();

		//bool bPhysicsWasCreated = (pShortcut) ? pShortcut->CreateVPhysicsMotion() : pBaseEntity->CreateVPhysics();
		bool bPhysicsWasCreated = pShortcut->CreateVPhysicsMotion();
		if (bPhysicsWasCreated)
		{
			pPhysics = pShortcut->VPhysicsGetObject();
			if (pPhysics)
			{
				if (pShortcut && scale_collisions.GetBool())
				{
					float fScale = pShortcut->GetModelScale();
					if (fScale != 1.0f)
					{
						UTIL_CreateScaledPhysObject(pShortcut->GetBaseAnimating(), fScale);
						pShortcut->SetSize(pShortcut->WorldAlignMins(), pShortcut->WorldAlignMaxs());
						pShortcut->CollisionProp()->RefreshScaledCollisionBounds();
						pShortcut->NetworkStateChanged();
					}
				}

				pShortcut->SetMoveType(MOVETYPE_VPHYSICS);
				pPhysics->EnableMotion(true);
				pPhysics->Wake();
				if (pShortcut->CanBecomeRagdoll())
				{
					//inputdata_t myInputData;
					//pShortcut->InputBecomeRagdoll(myInputData);
					pShortcut->BecomeRagdollOnClient(Vector(0, 0, 0));

					/*
					int iHeadBone = pShortcut->LookupBone("ValveBiped.Bip01_Head1");

					//pShortcut->Bone
					DevMsg("Bone number: %i\n", iHeadBone);

					if (iHeadBone < 0)// || !pShortcut->IsRagdoll()
						return;

					Vector pos;
					matrix3x4_t headBoneMatrix;

					headBoneMatrix = pShortcut->GetBoneForWrite(iHeadBone);
					headBoneMatrix = pShortcut->LookupBone(
					MatrixPosition(headBoneMatrix, pos);
					DevMsg("Bone 0 Pos: (%f/%f/%f)\n", pos.x, pos.y, pos.z);

					CStudioHdr* hdr = pShortcut->GetModelPtr();
					int boneControllerCount = hdr->numbonecontrollers();

					//m_iv_flEncodedController.SetMaxCount(boneControllerCount);

					mstudiobone_t* pBone = hdr->pBone(iHeadBone);
					for (int i = 0; i < boneControllerCount; i++)
					{
						bool loop = (hdr->pBonecontroller(i)->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) != 0;
						//m_iv_flEncodedController.SetLooping(loop, i);
						pShortcut->SetBoneController(i, 0.0);
					}
					*/
				}
			}
		}

		CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
		edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
		engine->ClientCommand(pClient, UTIL_VarArgs("ragdollinfo %i", pShortcut->entindex()));
	}
}
ConCommand makeragdollnow("makeragdollnow", MakeRagdollNow, "Internal use only.", FCVAR_HIDDEN);

void QuestHideEntity(const CCommand &args)
{
	CBaseEntity* pBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pBaseEntity)
		return;

	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
	if (!pShortcut)
		return;

	// we want to HIDE and be NON-COLLIDE
	pShortcut->SetSolid(SOLID_NONE);
	pShortcut->SetRenderMode(kRenderNone);
	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		//pChild->SetSolid(SOLID_NONE);
		pChild->SetRenderMode(kRenderNone);
		pChild = pChild->NextMovePeer();
	}

	pShortcut->SetMoveType(MOVETYPE_NONE);
}
ConCommand questhideentity("quest_hide_entity", QuestHideEntity, "Internal use only.", FCVAR_HIDDEN);

void QuestUnhideEntity(const CCommand &args)
{
	CBaseEntity* pBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pBaseEntity)
		return;

	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
	if (!pShortcut)
		return;

	// we want to HIDE and be NON-COLLIDE
	pShortcut->SetSolid(SOLID_VPHYSICS);
	pShortcut->SetRenderMode(kRenderNormal);
	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		//pChild->SetSolid(SOLID_VPHYSICS);
		pChild->SetRenderMode(kRenderNormal);
		pChild = pChild->NextMovePeer();
	}

	pShortcut->SetMoveType(MOVETYPE_NONE);
}
ConCommand questunhideentity("quest_unhide_entity", QuestUnhideEntity, "Internal use only.", FCVAR_HIDDEN);

void QuestMarkCollectibleEntity(const CCommand &args)
{
	CBaseEntity* pBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pBaseEntity)
		return;

	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
	if (!pShortcut)
		return;

	//pShortcut->SetTransmitState(FL_EDICT_ALWAYS);	// FIXME: We need to store whatever our original value was for transmite state so we can restore to it after we unmark this entity as collectible.

	bool bCollectiblesCollide = (args.ArgC() > 6) ? (Q_atoi(args[6]) != 0) : false;

	pShortcut->SetMoveType(MOVETYPE_NOCLIP);

	if (!bCollectiblesCollide)
		pShortcut->SetSolid(SOLID_NONE);
	else
		pShortcut->SetSolid(SOLID_VPHYSICS);

	// UNHIDE US IF WE ARE HIDDEN
	pShortcut->SetRenderMode(kRenderNormal);
	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		pChild->SetRenderMode(kRenderNormal);
		pChild = pChild->NextMovePeer();
	}

	float flAnglesX = (args.ArgC() > 3) ? Q_atoi(args[3]) : 0;
	float flAnglesY = (args.ArgC() > 4) ? Q_atoi(args[4]) : 0;
	float flAnglesZ = (args.ArgC() > 5) ? Q_atoi(args[5]) : 0;

	QAngle angles;
	angles.x = flAnglesX;
	angles.y = flAnglesY;
	angles.z = flAnglesZ;
	pShortcut->SetAbsAngles(angles);

	// Should we rotate?
	int iShouldRotate = (args.ArgC() > 2) ? Q_atoi(args[2]) : 0;
	if (iShouldRotate != 0)
	{
		QAngle rotAngle;
		rotAngle.x = 0.0f;
		rotAngle.y = 180.0f;
		rotAngle.z = 0.0f;
		pShortcut->SetLocalAngularVelocity(rotAngle);
	}
	else
	{
		QAngle rotAngle;
		rotAngle.Init();

		pShortcut->SetLocalAngularVelocity(rotAngle);
	}
}
ConCommand questMarkCollectibleEntity("quest_mark_collectible_entity", QuestMarkCollectibleEntity, "Internal use only.", FCVAR_HIDDEN);

void QuestUnmarkCollectibleEntity(const CCommand &args)
{
	CBaseEntity* pBaseEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (!pBaseEntity)
		return;

	CPropShortcutEntity* pShortcut = dynamic_cast<CPropShortcutEntity*>(pBaseEntity);
	if (!pShortcut)
		return;

	bool bCollectiblesCollide = (args.ArgC() > 6) ? (Q_atoi(args[6]) != 0) : false;

	QAngle rotAngle;
	rotAngle.Init();
	pShortcut->SetLocalAngularVelocity(rotAngle);
	pShortcut->SetRenderMode(kRenderNormal);

	//if (!bCollectiblesCollide)
		pShortcut->SetSolid(SOLID_VPHYSICS);

	float flAnglesX = (args.ArgC() > 3) ? Q_atoi(args[3]) : 0;
	float flAnglesY = (args.ArgC() > 4) ? Q_atoi(args[4]) : 0;
	float flAnglesZ = (args.ArgC() > 5) ? Q_atoi(args[5]) : 0;
	QAngle angles;
	angles.x = flAnglesX;
	angles.y = flAnglesY;
	angles.z = flAnglesZ;
	pShortcut->SetAbsAngles(angles);

	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		//pChild->SetSolid(SOLID_VPHYSICS);
		pChild->SetRenderMode(kRenderNormal);
		pChild = pChild->NextMovePeer();
	}

	pShortcut->SetMoveType(MOVETYPE_NONE);
}
ConCommand questUnmarkCollectibleEntity("quest_unmark_collectible_entity", QuestUnmarkCollectibleEntity, "Internal use only.", FCVAR_HIDDEN);

void MakeNonGhost(const CCommand &args)
{
	CBaseEntity* pShortcut = CBaseEntity::Instance(Q_atoi(args[1]));

	bool bShouldGhost = (Q_atoi(args[2]) != 0);
	if (bShouldGhost)
	{
		pShortcut->SetRenderColorA(255);
		pShortcut->SetRenderMode(kRenderNormal);
	}

	// make the prop solid
	//pShortcut->SetSolid(SOLID_VPHYSICS);
	pShortcut->SetSize(pShortcut->WorldAlignMins(), pShortcut->WorldAlignMaxs());
	//pShortcut->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));

	//pShortcut->SetSize(-Vector(200, 200, 200), Vector(200, 200, 200));
	
	pShortcut->SetMoveType(MOVETYPE_NONE);
	//pShortcut->SetMoveType(MOVETYPE_VPHYSICS);
	///*
//	if (pShortcut->VPhysicsGetObject())
	//{
		//VPhysicsGetObject()->EnableCollisions(false);
	//	pShortcut->VPhysicsDestroyObject();
	//}
//	*/
	pShortcut->SetSolid(SOLID_VPHYSICS);

	IPhysicsObject *pPhysics = pShortcut->VPhysicsGetObject();
	if (pPhysics || pShortcut->CreateVPhysics())
	{
		pPhysics = pShortcut->VPhysicsGetObject();

		pShortcut->SetMoveType(MOVETYPE_NONE);

		if (pPhysics)
		{
			pPhysics->EnableMotion(false);

			float fScale = pShortcut->GetBaseAnimating()->GetModelScale();
			if (scale_collisions.GetBool())// && fScale != 1.0f)
			{
				UTIL_CreateScaledPhysObject(pShortcut->GetBaseAnimating(), fScale);
				pShortcut->CollisionProp()->RefreshScaledCollisionBounds();
			}
		}
	}
	pShortcut->NetworkStateChanged();

	CBaseEntity* pChild = pShortcut->FirstMoveChild();
	while (pChild)
	{
		pChild->SetMoveType(MOVETYPE_NONE);
		pChild->SetSolid(SOLID_VPHYSICS);	// ALWAYS make non-solid

		IPhysicsObject *pPhysics = pChild->VPhysicsGetObject();
		if (pPhysics || pChild->CreateVPhysics())
		{
			pPhysics = pChild->VPhysicsGetObject();

			pChild->SetMoveType(MOVETYPE_NONE);

			if (pPhysics)
			{
				pPhysics->EnableMotion(false);

				float fScale = pChild->GetBaseAnimating()->GetModelScale();
				if (scale_collisions.GetBool())// && fScale != 1.0f)
				{
					UTIL_CreateScaledPhysObject(pChild->GetBaseAnimating(), fScale);
					pChild->CollisionProp()->RefreshScaledCollisionBounds();
				}
			}
		}

		pChild = pChild->NextMovePeer();
	}
}
ConCommand makenonghost("makenonghost", MakeNonGhost, "Interal use only.", FCVAR_HIDDEN);

//#include <vector>
CBaseEntity* GetEntSpawnPoint(void)
{
		// NOT THE ACTUAL FUNCTION USED IN HL2MP_PLAYER.CPP.  THIS FUNCTION IS REFERENCED NOWHERE AND THE COMPILER DROPS IT.

	// Added for Anarchy Arcade BEGIN
	CBaseEntity *pSpot = NULL;

	bool bNeedsSpawnPoint = true;
	// check if we have a player_spawn_override value to use...
	if (Q_strcmp(player_spawn_override.GetString(), ""))
	{
		// find the info_target with this name
		std::vector<CBaseEntity*> targetEntities;
		CBaseEntity* pTargetEntity = gEntList.FindEntityByName(NULL, MAKE_STRING(player_spawn_override.GetString()));
		while (pTargetEntity)
		{
			targetEntities.push_back(pTargetEntity);
			pTargetEntity = gEntList.FindEntityByName(pTargetEntity, MAKE_STRING(player_spawn_override.GetString()));
		}

		// just use the FIRST one with this name for now.
		// later on, it might need to get all target entities with this name & randomly select from them so people don't all spawn at the exact same spot.

		if (targetEntities.size() > 0)
		{
			pSpot = targetEntities[0];
			bNeedsSpawnPoint = false;
		}
	}

	if ( bNeedsSpawnPoint )
	{
		//ConVar* SpawnPositionVar = cvar->FindVar("spawn_position");
		//int SpawnPosition = SpawnPositionVar->GetInt();
		int SpawnPosition = 420;

		std::vector<CBaseEntity*> potentials;

		int count = 0;

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_start");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_start");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_counterterrorist");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_counterterrorist");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_terrorist");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_terrorist");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_deathmatch");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_deathmatch");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_allies");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_allies");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_axis");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_axis");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_fof");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_fof");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_coop");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_coop");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_teamspawn");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_teamspawn");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_rebel");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_rebel");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_combine");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_combine");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_nmrih");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_nmrih");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_american");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_american");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_british");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_british");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "emp_imp_commander");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "emp_imp_commander");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "emp_nf_commander");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "emp_nf_commander");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_es_spawn");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_es_spawn");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_hidden_spawn");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_hidden_spawn");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_marine_spawn");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_marine_spawn");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_mi6");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_mi6");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_janus");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_janus");
		}

		pSpot = gEntList.FindEntityByClassname(NULL, "ins_spawnpoint");
		while (pSpot && count < SpawnPosition)
		{
			potentials.push_back(pSpot);
			count++;

			pSpot = gEntList.FindEntityByClassname(pSpot, "ins_spawnpoint");
		}

		if (count > 0)
			pSpot = potentials[potentials.size() - 1];
		else
		{
			if (!pSpot)
			{
				Msg("ERROR: NO SPAWN POINTS COULD BE FOUND!\n");
				return NULL;
			}

			/*
			pSpot = gEntList.FindEntityByClassname(pSpot, "prop_physics");

			if ( !pSpot )
			pSpot = gEntList.FindEntityByClassname(pSpot, "prop_dynamioc");

			if (!pSpot)
			pSpot = gEntList.FindEntityByClassname(pSpot, "prop_static");

			if (!pSpot)
			{
			Msg("ERROR: NO SPAWN POINTS COULD BE FOUND!\n");
			return NULL;
			}
			*/
		}
	}

	//g_pLastSpawn = pSpot;
	//m_flSlamProtectTime = gpGlobals->curtime + 0.5;

	return pSpot;
}

void CreateAPIObject(const CCommand &args)
{
	//engine->ClientCmd(VarArgs("create_api_object %i \"%s\" \"%s\" \"%f %f %f\" \"%f %f %f\"", this->GetOriginalEntIndex(), objectId, modelFile, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z));

	//CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	//if (!pSafeSpawnEntity)
	//	return;

	std::string sessionId = args[1];

	int iParentEntityIndex = Q_atoi(args[2]);
	CDynamicProp* pParentEntity = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(iParentEntityIndex));//pParentEntity = CBaseEntity::Instance(iParentEntityIndex);
	if (!pParentEntity)
		return;

	std::string objectId = args[3];
	std::string modelFile = args[4];// "models\\sithlord\\alphabet_capital.mdl";
	std::string origin = args[5];
	std::string rotation = args[6];

	float flScale = Q_atof(args[7]);// pParentEntity->GetModelScale();// 1.0f;//Q_atof(args[7]);

	/*
	Vector goodOrigin;
	goodOrigin.Init();
	UTIL_StringToVector(goodOrigin.Base(), origin.c_str());

	Vector scaleVec;
	scaleVec.Init();
	scaleVec.x = flScale;
	scaleVec.y = flScale;
	scaleVec.z = flScale;

	goodOrigin *= scaleVec;
	*/

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	// Pass in standard key values
	pProp->KeyValue("origin", origin.c_str());
	
	//char buf[512];
	//Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", goodOrigin.x, goodOrigin.y, goodOrigin.z);
	//pProp->KeyValue("origin", buf);

	pProp->KeyValue("angles", rotation.c_str());

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1.0");// UTIL_VarArgs("%f", pParentEntity->GetModelScale()));
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "1");
	pProp->KeyValue("disableshadows", "1");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile.c_str());
	pProp->SetParent(pParentEntity, -1);

	//pProp->SetTransmitState(FL_EDICT_ALWAYS);
	if (DispatchSpawn(pProp) > -1)
	{
		pProp->SetModelScale(flScale, 0.0f);

		// finish spawning
		//pProp->SetCollisionGroup(COLLISION_GROUP_NONE);
		pProp->SetParent(pParentEntity, -1);

		CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
		edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
		engine->ClientCommand(pClient, "api_object_created \"%s\" \"%s\" %i %i;\n", sessionId.c_str(), objectId.c_str(), pProp->entindex(), iParentEntityIndex);
	}
}
ConCommand createapiobject("create_api_object", CreateAPIObject, "Interal use only.", FCVAR_HIDDEN);

void UpdateAPIObjectTransform(const CCommand &args)
{
	std::string sessionId = args[1];
	std::string objectId = args[2];

	int iEntityIndex = Q_atoi(args[3]);
	CDynamicProp* pEntity = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(iEntityIndex));
	if (!pEntity)
		return;

	int iParentEntityIndex = Q_atoi(args[4]);
	CDynamicProp* pParentEntity = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(iParentEntityIndex));
	if (!pParentEntity)
		return;

	std::string modelFile = args[5];
	std::string origin = args[6];
	std::string rotation = args[7];
	float flScale = Q_atof(args[8]);
	float flBaseScale = pParentEntity->GetModelScale() * 0.75;
	float flGoodScale = flScale * flBaseScale;

	pEntity->SetModelScale(flScale, flGoodScale);

	Vector goodOrigin;
	goodOrigin.Init();
	UTIL_StringToVector(goodOrigin.Base(), origin.c_str());

	QAngle goodAngle;
	goodAngle.Init();
	UTIL_StringToVector(goodAngle.Base(), rotation.c_str());

	pEntity->SetAbsOrigin(goodOrigin);
	pEntity->SetAbsAngles(goodAngle);

	Vector vel = Vector(0, 0, 0);
	pEntity->Teleport(&goodOrigin, &goodAngle, &vel);
}
ConCommand updateapiobjecttransform("update_api_object_transform", UpdateAPIObjectTransform, "Interal use only.", FCVAR_HIDDEN);

void CreateLetter(char c, CDynamicProp* pParentEntity, Vector origin, QAngle angles, float flScale)
{
	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pSafeSpawnEntity)
		return;

	std::string modelFile = "models\\sithlord\\alphabet_capital.mdl";

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	// Pass in standard key values
	char buf[512];

	//Vector safeOrigin = pSafeSpawnEntity->GetAbsOrigin();
	//QAngle safeAngles = pSafeSpawnEntity->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1.0");// UTIL_VarArgs("%f", pParentEntity->GetModelScale()));
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "1");
	pProp->KeyValue("disableshadows", "1");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile.c_str());
	pProp->SetParent(pParentEntity, -1);

	if (DispatchSpawn(pProp) > -1)
	{
		if ((int)c >= 65 && (int)c <= 90)
			pProp->SetBodygroup(0, (int)c - 65);
		else
			pProp->SetBodygroup(0, 26);

		pProp->SetModelScale(flScale, 0.0f);

		// finish spawning
		pProp->SetCollisionGroup(COLLISION_GROUP_NONE);
		pProp->SetParent(pParentEntity, -1);
	}
}

std::string ReplaceString(std::string subject, const std::string& search,
	const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

void SplitString(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiter)
{
	std::string safeStr = str;
	//std::transform(safeStr.begin(), safeStr.end(), safeStr.begin(), ::tolower);

	// find the first match
	size_t found = safeStr.find(delimiter);
	while (found != std::string::npos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(safeStr.substr(0, found));
		safeStr = safeStr.substr(found + delimiter.length());
		found = safeStr.find(delimiter);
	}
	tokens.push_back(safeStr);
}

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
void CreateVoxel(CDynamicProp* pParentEntity, Vector origin, QAngle angles, float flBaseScale, Color color)
{
	float flScale = flBaseScale;

	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pSafeSpawnEntity)
		return;

	std::string modelFile = "models\\hunter\\blocks\\cube025x025x025.mdl";

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	// Pass in standard key values
	char buf[512];

	//Vector safeOrigin = pSafeSpawnEntity->GetAbsOrigin();
	//QAngle safeAngles = pSafeSpawnEntity->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", origin.x, origin.y, origin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1.0");// UTIL_VarArgs("%f", pParentEntity->GetModelScale()));
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", UTIL_VarArgs("%i %i %i", color.r(), color.g(), color.b()));//"255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "1");
	pProp->KeyValue("disableshadows", "1");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile.c_str());
	pProp->SetParent(pParentEntity, -1);

	if (DispatchSpawn(pProp) > -1)
	{
		pProp->SetModelScale(flScale, 0.0f);

		// finish spawning
		pProp->SetCollisionGroup(COLLISION_GROUP_NONE);
		pProp->SetParent(pParentEntity, -1);
	}
}

void ReallySpawnVoxels(CBaseEntity* pParentEntity, std::string voxfile)
{
	float flVoxelSize = 12.0f;
	float flBaseScale = 0.25f;

	// LOAD THE VOX INTO MEMORY BUFFER
	FileHandle_t fh = filesystem->Open(voxfile.c_str(), "rb", "GAME");
	if (!fh)
		return;

	int file_len = filesystem->Size(fh);

	/* Changed this known working unsigned char* code to be uint8_t* to match the OGT Vox code.
	unsigned char* pVoxelData = new unsigned char[file_len + 1];
	filesystem->Read((void*)pVoxelData, file_len, fh);
	*/

	uint8_t* buffer = new uint8_t[file_len + 1];
	filesystem->Read((void*)buffer, file_len, fh);

	buffer[file_len] = 0; // null terminator
	filesystem->Close(fh);

	// PARSE THE MEMORY BUFFER INTO A VOX OBJECT
	const ogt_vox_scene* scene = ogt_vox_read_scene(buffer, file_len);
	// the buffer can be safely deleted once the scene is instantiated.
	delete[] buffer;

	if (!scene)
		return;

	DevMsg("# models: %u\n", scene->num_models);
	DevMsg("# instances: %u\n", scene->num_instances);
	DevMsg("# of layers: %u\n", scene->num_layers);
	DevMsg("# of groups: %u\n", scene->num_groups);

	// TODO: work...

	// PROCESS A MODEL
	// Then, time to display a set of voxels somewhere...
	if (scene->num_models < 1)
		return;

	const ogt_vox_model* pVoxModel = scene->models[0];
	DevMsg("\tModel Size: %ux%ux%u\n", pVoxModel->size_x, pVoxModel->size_y, pVoxModel->size_z);

	// PROCESS A MODEL'S DATA
	// Step through the planes of the voxel grid, grabbing info about each voxel.

	Vector voxelOrigin;
	QAngle voxelAngles;
	voxelAngles.Init();

	int iHalfX, iHalfY, iHalfZ;

	iHalfX = pVoxModel->size_x / 2;
	iHalfY = pVoxModel->size_y / 2;
	iHalfZ = pVoxModel->size_z / 2;

	unsigned int iVoxelIndex;
	uint8_t color_index;
	ogt_vox_rgba color;
	int x, y, z;
	for (z = 0; z < pVoxModel->size_z; ++z)
	{
		for (y = 0; y < pVoxModel->size_y; ++y)
		{
			for (x = 0; x < pVoxModel->size_x; ++x)
			{
				iVoxelIndex = x + (y * pVoxModel->size_x) + (z * pVoxModel->size_x * pVoxModel->size_y);
				color_index = pVoxModel->voxel_data[iVoxelIndex];

				if (color_index != 0)
				{
					color = scene->palette.color[color_index];
					DevMsg("Voxel Color: %u %u %u\n", color.r, color.g, color.b, color.a);

					// spawn a voxel.
					// FIXME: Move this into the mesh builder, so that each VOX cluster is only 1 entity instead of many.
					// TODO: figure out this voxel's origin relative to its hierarchy - including AArcade-parent-object-space and AArcade-world-space.
					//voxelOrigin = ?;

					// The grid gets centered on all 3 axes.
					voxelOrigin.Init();
					voxelOrigin.x = (x - iHalfX) * flVoxelSize;
					voxelOrigin.y = (y - iHalfY) * flVoxelSize;
					voxelOrigin.z = (z - iHalfZ) * flVoxelSize;

					voxelOrigin *= flBaseScale;

					voxelAngles.Init();
					VMatrix childMatrix;
					childMatrix.SetupMatrixOrgAngles(voxelOrigin, voxelAngles);

					VMatrix composedMatrix;
					composedMatrix.SetupMatrixOrgAngles(pParentEntity->GetAbsOrigin(), pParentEntity->GetAbsAngles());
					composedMatrix = composedMatrix * childMatrix;

					// back to vecs & angles
					MatrixAngles(composedMatrix.As3x4(), voxelAngles, voxelOrigin);

					CreateVoxel(dynamic_cast<CDynamicProp*>(pParentEntity), voxelOrigin, voxelAngles, flBaseScale, Color(color.r, color.g, color.b, color.a));
				}
			}
		}
	}

	ogt_vox_destroy_scene(scene);

	DevMsg("fin\n");
}

void SpawnVoxels(const CCommand &args)
{
	// Just snag the local player for now.  In the future, pParentEntity will actually be passed to us, along with a VOX file name.
	CBaseEntity *pPlayer = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	std::string voxfile = "chr_knight.vox";
	if (args.ArgC() > 1)
	{
		voxfile = args[1];
		if (voxfile.find(".vox") == std::string::npos && voxfile.find(".VOX") == std::string::npos)
			voxfile += ".vox";
	}

	ReallySpawnVoxels(pPlayer, voxfile);
}
ConCommand spawn_voxels("spawn_voxels", SpawnVoxels, "For internal use only.");

void ReallySetText(std::string text_in, int pParentEntityIndex, float flExtraScale = 1.0f)
{
	//std::string text = args[2];
	std::string text = text_in;

	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	};

	CDynamicProp* pParentEntity = dynamic_cast<CDynamicProp*>(CBaseEntity::Instance(pParentEntityIndex));

	// remove any letters the parent already has
	std::vector<CBaseEntity*> victims;
	CBaseEntity* pChild = pParentEntity->FirstMoveChild();
	while (pChild)
	{
		victims.push_back(pChild);
		pChild = pChild->NextMovePeer();
	}

	for (unsigned int i = 0; i < victims.size(); i++)
	{
		inputdata_t emptyDummy;
		victims[i]->InputKillHierarchy(emptyDummy);
	}

	if (text == "")
		return;

	float flParentScale = pParentEntity->GetModelScale();
	float flScale = flParentScale * flExtraScale;

	Vector childOrigin;
	childOrigin.Init();

	QAngle childAngles;
	childAngles.Init();

	float flBaseCharWidth = 20.0f;
	float flLineHeight = 26.0f;

	int iLineNumber = 0;
	float flBottom;
	float flLeft;
	int iCharIndex;

	text = ReplaceString(text, "%20", "_");
	std::vector<std::string> textLines;
	SplitString(text, textLines, "%0A");

	std::string textLine;
	for (unsigned int i = 0; i < textLines.size(); i++)
	{
		textLine = textLines[i];

		//VMatrix childMatrix;
		//VMatrix parentMatrix;
		//parentMatrix.SetupMatrixOrgAngles(pParentEntity->GetAbsOrigin(), pParentEntity->GetAbsAngles());
		//VMatrix parentMatrixInverse = parentMatrix.InverseTR();

		//float flMiddleY = textLine.length() / -2.0f;
		flLeft = ((textLine.length() / -2.0) * flBaseCharWidth) + (flBaseCharWidth / 2.0f);
		flBottom = (textLines.size() - 1 - iLineNumber) * flLineHeight;

		iCharIndex = 0;
		for (char& c : textLine)
		{
			childOrigin.Init();
			childOrigin.y = flLeft + (iCharIndex * flBaseCharWidth);// +(flBaseCharWidth / 2.0f);		//flYOffset;
			childOrigin.z = flBottom;

			childAngles.Init();
			//childAngles = TransformAnglesToLocalSpace(childAngles, parentMatrix.As3x4());
			//childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
			//childMatrix = childMatrix * parentMatrixInverse;
			//childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
			//childOrigin += pParentEntity->GetAbsOrigin();

			//flParentScale = 1.0f;
			//flScale = 1.0f;

			childOrigin *= flScale;

			VMatrix childMatrix;
			childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);

			VMatrix composedMatrix;
			composedMatrix.SetupMatrixOrgAngles(pParentEntity->GetAbsOrigin(), pParentEntity->GetAbsAngles());
			composedMatrix = composedMatrix * childMatrix;

			// back to vecs & angles
			MatrixAngles(composedMatrix.As3x4(), childAngles, childOrigin);

			//flScale *= flParentScale;

			CreateLetter(c, pParentEntity, childOrigin, childAngles, flScale);
			iCharIndex++;
		}

		iLineNumber++;
	}


	/*
	VMatrix childMatrix;
	VMatrix parentMatrix;
	parentMatrix.SetupMatrixOrgAngles(pParentEntity->GetAbsOrigin(), pParentEntity->GetAbsAngles());
	VMatrix parentMatrixInverse = parentMatrix.InverseTR();

	Vector childOrigin;
	childOrigin.x = 0;
	childOrigin.y = 0;
	childOrigin.z = 0;

	QAngle childAngles;
	childAngles.x = 0;
	childAngles.y = 0;
	childAngles.z = 0;

	childAngles = TransformAnglesToLocalSpace(childAngles, parentMatrix.As3x4());

	childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
	childMatrix = childMatrix * parentMatrixInverse;

	// Finally convert back to origin+angles.
	childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);

	char buf1[512];
	char buf2[512];

	// position
	Vector localPosition = childOrigin;// pPropShortcutEntity->GetLocalOrigin();
	Q_snprintf(buf1, sizeof(buf1), "%.10f %.10f %.10f", localPosition.x, localPosition.y, localPosition.z);
	std::string position = buf1;

	// rotation
	QAngle localAngles = childAngles;// pPropShortcutEntity->GetLocalAngles();
	Q_snprintf(buf2, sizeof(buf2), "%.10f %.10f %.10f", localAngles.x, localAngles.y, localAngles.z);
	std::string rotation = buf2;
	*/










}

void SetText(const CCommand &args)
{
	std::string text = args[2];
	ReallySetText(text, Q_atoi(args[1]));
}
ConCommand set_text("set_text", SetText, "For internal use only.");

/*
void CreateNavNodeServer(const CCommand &args)
{
	Vector v;
	v.x = Q_atof(args[1]);
	v.y = Q_atof(args[2]);
	v.z = Q_atof(args[3]);

	CBaseEntity* pBaseEntity = CreateEntityByName("info_node");

	char buf[512];

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", v.x, v.y, v.z);
	pBaseEntity->KeyValue("origin", buf);

	//Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", vecAttachmentAngles.x, vecAttachmentAngles.y, vecAttachmentAngles.z);
	pBaseEntity->KeyValue("spawnflags", "0");
	pBaseEntity->KeyValue("nodeid", "1");
	pBaseEntity->KeyValue("angles", "0 0 0");
	pBaseEntity->Precache();
	if (DispatchSpawn(pBaseEntity) != 0)
	{
		Msg("Error spawning info_node entity on server-side code!\n");
	}
}
ConCommand create_nav_node_server("create_nav_node_server", CreateNavNodeServer, "For internal use only.");
*/

void SpawnChatBall(const CCommand &args)
{
	//CBaseEntity* pSafeSpawnEntity = GetEntSpawnPoint();

	std::string text = (args.ArgC() > 1) ? args[1] : "TEXT";

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer->GetHealth() <= 0)
		return;


	//std::string models = junkmodel.GetString();
	//if (models == "")
	//	models = "models\\de_halloween\\jacklight.mdl";

	//std::vector<std::string> junkModels;
	//SplitString(models, junkModels, "::");

	//int iRand = rand() % (junkModels.size() - 1) + 0;	// between SIZE-1 and 0
	std::string model = "models\\de_halloween\\jacklight.mdl";// "models\\sithlord\\giftbox.mdl"; junkModels[iRand];

	Vector eyePosition;
	Vector forward;
	Vector right;
	Vector up;
	QAngle eyeAngles = pPlayer->EyeAngles();
	pPlayer->EyePositionAndVectors(&eyePosition, &forward, &right, &up);

	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(pPlayer);
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	}

	/*
	1 - modelFile
	2 - origin X
	3 - origin Y
	4 - origin Z
	5 - angles P
	6 - angles Y
	7 - angles R
	8 - userId
	*/

	////char id[256];
	////strcpy(id, args.Arg(8));

	//Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	//QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

	//char modelFile[256];
	//strcpy(modelFile, "models\\de_halloween\\jacklight.mdl");

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	float flScale = 1.0;

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = eyePosition;// pSafeSpawnEntity->GetAbsOrigin();
	QAngle safeAngles = eyeAngles;// pSafeSpawnEntity->GetAbsAngles();
	safeAngles.y += 180.0f;

	safeOrigin += forward * 24.0;

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "6");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "1");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", model.c_str());// modelFile);

	if (DispatchSpawn(pProp) > -1)
	{
		pProp->SetRenderMode(kRenderNone);

		pProp->SetCollisionGroup(COLLISION_GROUP_PLAYER);

		pProp->SetSolid(SOLID_VPHYSICS);
		pProp->CreateVPhysics();

		pProp->SetMoveType(MOVETYPE_VPHYSICS);
		pProp->VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);
		//pProp->SetAbsVelocity((forward * 2.0));// +(up * 0.2));

		float flBaseScale = 0.3;
		float flLengthScale = 8.0 / (text.length() * 1.0);
		float flScale = flBaseScale * flLengthScale;
		ReallySetText(text, pProp->entindex(), flScale);

		g_pAnarchyManager->AddJunkEntity(pProp);

		//pProp->IgniteLifetime(3.0);
	}
}
ConCommand chat_ball("chat_ball", SpawnChatBall, "For internal use only. (For now, until I make it more user-command-console friendly.");

void SpawnPet_Server(const CCommand &args)
{
	std::string model = (args.ArgC() > 1) ? args.Arg(1) : "";
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer->GetHealth() <= 0)
		return;

	Vector eyePosition;
	Vector forward;
	Vector right;
	Vector up;
	QAngle eyeAngles = pPlayer->EyeAngles();
	pPlayer->EyePositionAndVectors(&eyePosition, &forward, &right, &up);

	trace_t tr;
	Vector vForward;
	pPlayer->EyeVectors(&vForward);
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + vForward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);


	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(pPlayer);
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	}

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_shortcut"));
	float flScale = (args.ArgC() > 2) ? Q_atof(args.Arg(2)) : 1.0;

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = eyePosition;
	safeOrigin = tr.endpos;
	QAngle safeAngles = QAngle(0, 0, 0);

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "6");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "0");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", model.c_str());

	if (DispatchSpawn(pProp) > -1)
	{
		pProp->SetMoveType(MOVETYPE_NONE);
		pProp->SetCollisionGroup(COLLISION_GROUP_NONE);
		pProp->SetSolid(SOLID_NONE);
		if (pProp->VPhysicsGetObject())
			pProp->VPhysicsDestroyObject();

		CBaseEntity* pChild = pProp->FirstMoveChild();
		while (pChild)
		{
			pChild->SetSolid(SOLID_NONE);
			pChild->SetCollisionGroup(COLLISION_GROUP_NONE);
			pChild = pChild->NextMovePeer();
		}

		pProp->NetworkStateChanged();

		CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
		edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
		engine->ClientCommand(pClient, "pet_created %i;\n", pProp->entindex());
	}
}
ConCommand spawn_pet_server("spawn_pet_server", SpawnPet_Server, "For internal use only.", FCVAR_HIDDEN);

void SpawnJunk(const CCommand &args)
{
	//CBaseEntity* pSafeSpawnEntity = GetEntSpawnPoint();

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer->GetHealth() <= 0)
		return;


	std::string models = junkmodel.GetString();
	if (models == "")
		models = "models\\de_halloween\\jacklight.mdl";

	std::vector<std::string> junkModels;
	SplitString(models, junkModels, "::");

	if (junkModels.size() == 0)
		return;

	int iRand = (junkModels.size() == 1) ? 0 : rand() % (junkModels.size() - 1) + 0;	// between SIZE-1 and 0
	std::string model = junkModels[iRand];

	Vector eyePosition;
	Vector forward;
	Vector right;
	Vector up;
	QAngle eyeAngles = pPlayer->EyeAngles();
	pPlayer->EyePositionAndVectors(&eyePosition, &forward, &right, &up);

	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(pPlayer);
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	}

	/*
	1 - modelFile
	2 - origin X
	3 - origin Y
	4 - origin Z
	5 - angles P
	6 - angles Y
	7 - angles R
	8 - userId
	*/

	////char id[256];
	////strcpy(id, args.Arg(8));

	//Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	//QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

	//char modelFile[256];
	//strcpy(modelFile, "models\\de_halloween\\jacklight.mdl");

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	float flScale = 1.0;

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = eyePosition;// pSafeSpawnEntity->GetAbsOrigin();
	QAngle safeAngles = eyeAngles;// pSafeSpawnEntity->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", UTIL_VarArgs("%f", flScale));
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "6");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "0");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", model.c_str());// modelFile);

	if (DispatchSpawn(pProp) > -1)
	{
		pProp->SetCollisionGroup(COLLISION_GROUP_PLAYER);

		pProp->SetSolid(SOLID_VPHYSICS);
		pProp->CreateVPhysics();

		pProp->SetMoveType(MOVETYPE_VPHYSICS);
		pProp->VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);
		pProp->SetAbsVelocity((forward * 2.0) * up);

		g_pAnarchyManager->AddJunkEntity(pProp);
	}
}
ConCommand spawn_junk("spawn_junk", SpawnJunk, "For internal use only.");

void CreateLocalAvatarObject(const CCommand &args)
{
	//CBaseEntity* pSafeSpawnEntity = GetEntSpawnPoint();
	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	}

	/*
	1 - modelFile
	2 - origin X
	3 - origin Y
	4 - origin Z
	5 - angles P
	6 - angles Y
	7 - angles R
	8 - userId
	*/

	//char id[256];
	//strcpy(id, args.Arg(8));

	Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

	char modelFile[256];
	strcpy(modelFile, args.Arg(1));

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = pSafeSpawnEntity->GetAbsOrigin();
	QAngle safeAngles = pSafeSpawnEntity->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", "0.5");
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "0");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile);

	if (DispatchSpawn(pProp) > -1)
	{
		// finish spawning
		//pProp->VPhysicsInitNormal(SOLID_NONE, 0, false);
		pProp->SetCollisionGroup(COLLISION_GROUP_NONE);

		CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();

		edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());

		engine->ClientCommand(pClient, "local_avatar_object_created %i;\n", pProp->entindex());// , id); \"%s\"
	}
}
ConCommand create_local_avatar_object("create_local_avatar_object", CreateLocalAvatarObject, "For internal use only.");

void CreateAvatarObject(const CCommand &args)
{
	//CBaseEntity* pSafeSpawnEntity = GetEntSpawnPoint();
	CBaseEntity *pSafeSpawnEntity = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pSafeSpawnEntity)
	{
		DevMsg("ERROR: No spawn positions found!\n");
		return;
	}

	/*
		1 - modelFile
		2 - origin X
		3 - origin Y
		4 - origin Z
		5 - angles P
		6 - angles Y
		7 - angles R
		8 - userId
	*/

	char id[256];
	//char avatarName[256];
	strcpy(id, args.Arg(8));
	//strcpy(avatarName, args.Arg(9));

	Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

	char modelFile[256];
	strcpy(modelFile, args.Arg(1));

	//CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	//Vector playerOrigin = pPlayer->GetAbsOrigin();

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));
	//pProp->KeyValue("targetname", entityName.c_str());

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = pSafeSpawnEntity->GetAbsOrigin();
	QAngle safeAngles = pSafeSpawnEntity->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", "1.0");
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "0");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile);
	//pProp->SetAvatarName(avatarName);

	if (DispatchSpawn(pProp) > -1)
	{
		// snap it to position
		/*
		UTIL_SetOrigin(pProp, origin, true);
		pProp->SetAbsAngles(angles);
		Vector vel = Vector(0, 0, 0);
		pProp->Teleport(&origin, &angles, &vel);

		Vector vStartPos = pProp->GetAbsOrigin();
		pProp->SetAbsVelocity(vec3_origin);
		pProp->SetLerpSync(origin, angles);
		*/

		// finish spawning
		pProp->VPhysicsInitNormal(SOLID_NONE, 0, false);
		pProp->SetCollisionGroup(COLLISION_GROUP_NONE);

		CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();

		edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());

		engine->ClientCommand(pClient, "avatar_object_created %i \"%s\";\n", pProp->entindex(), id);
	}
}
ConCommand create_avatar_object("create_avatar_object", CreateAvatarObject, "For internal use only.");

void CreateVRHands(const CCommand &args)
{
	CBaseEntity *pPlayer = dynamic_cast<CBaseEntity*>(UTIL_GetLocalPlayer());
	if (!pPlayer)
	{
		DevMsg("ERROR: No player found!\n");
		return;
	}

	/*
	1 - modelFile
	2 - side
	//2 - origin X
	//3 - origin Y
	//4 - origin Z
	//5 - angles P
	//6 - angles Y
	//7 - angles R
	*/

	char modelFile[256];
	strcpy(modelFile, args.Arg(1));
	//strcpy(modelFile, "models/players/hands/rift_cv1_right.mdl");

	int iSide = Q_atoi(args.Arg(2));

	//Vector origin = Vector(Q_atof(args.Arg(2)), Q_atof(args.Arg(3)), Q_atof(args.Arg(4)));
	//QAngle angles = QAngle(Q_atof(args.Arg(5)), Q_atof(args.Arg(6)), Q_atof(args.Arg(7)));

	// Now spawn it
	CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

	// Pass in standard key values
	char buf[512];

	Vector safeOrigin = pPlayer->GetAbsOrigin();
	QAngle safeAngles;
	safeAngles.Init();// = pPlayer->GetAbsAngles();

	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
	pProp->KeyValue("angles", buf);

	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("MaxAnimTime", "10");
	pProp->KeyValue("MinAnimTime", "5");
	pProp->KeyValue("modelscale", "1.0");
	pProp->KeyValue("renderamt", "255");
	pProp->KeyValue("rendercolor", "255 255 255");
	pProp->KeyValue("solid", "0");
	pProp->KeyValue("DisableBoneFollowers", "0");
	pProp->KeyValue("disablereceiveshadows", "0");
	pProp->KeyValue("disableshadows", "0");
	pProp->KeyValue("ExplodeDamage", "0");
	pProp->KeyValue("skin", "0");
	pProp->KeyValue("ExplodeRadius", "0");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("maxdxlevel", "0");
	pProp->KeyValue("mindxlevel", "0");

	pProp->KeyValue("PerformanceMode", "0");
	pProp->KeyValue("pressuredelay", "0");
	pProp->KeyValue("spawnflags", "0");
	pProp->KeyValue("RandomAnimation", "0");
	pProp->KeyValue("renderfx", "0");
	pProp->KeyValue("rendermode", "0");
	pProp->KeyValue("SetBodyGroup", "0");
	pProp->KeyValue("StartDisabled", "0");

	pProp->KeyValue("model", modelFile);
	//pProp->SetAvatarName(avatarName);

	if (DispatchSpawn(pProp) > -1)
	{
		// finish spawning
		//pProp->VPhysicsInitNormal(SOLID_NONE, 0, false);
		//pProp->SetCollisionGroup(COLLISION_GROUP_NONE);
		//pProp->SetSolid(SOLID_NONE);

		if (iSide < 1)
		{
			CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
			edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
			engine->ClientCommand(pClient, "vr_hand_created %i %i;\n", pProp->entindex(), iSide);
		}
		else
		{
			// CREATE THE POINTER
			CDynamicProp* pHandProp = pProp;
			CDynamicProp* pProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

			// Pass in standard key values
			char buf[512];

			//safeAngles = pProp->GetAbsAngles();
			safeAngles.x += 45.0f;

			// Pass in standard key values
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
			pProp->KeyValue("origin", buf);
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
			pProp->KeyValue("angles", buf);

			pProp->KeyValue("fademindist", "-1");
			pProp->KeyValue("fadescale", "1");
			pProp->KeyValue("MaxAnimTime", "10");
			pProp->KeyValue("MinAnimTime", "5");
			pProp->KeyValue("modelscale", "1.0");
			pProp->KeyValue("renderamt", "255");
			pProp->KeyValue("rendercolor", "255 255 255");
			pProp->KeyValue("solid", "0");
			pProp->KeyValue("DisableBoneFollowers", "0");
			pProp->KeyValue("disablereceiveshadows", "1");
			pProp->KeyValue("disableshadows", "1");
			pProp->KeyValue("ExplodeDamage", "0");
			pProp->KeyValue("skin", "0");
			pProp->KeyValue("ExplodeRadius", "0");
			pProp->KeyValue("fademaxdist", "0");
			pProp->KeyValue("maxdxlevel", "0");
			pProp->KeyValue("mindxlevel", "0");

			pProp->KeyValue("PerformanceMode", "0");
			pProp->KeyValue("pressuredelay", "0");
			pProp->KeyValue("spawnflags", "0");
			pProp->KeyValue("RandomAnimation", "0");
			pProp->KeyValue("renderfx", "0");
			pProp->KeyValue("rendermode", "0");
			pProp->KeyValue("SetBodyGroup", "0");
			pProp->KeyValue("StartDisabled", "0");

			pProp->KeyValue("model", "models/players/hands/pointer.mdl");
			pProp->SetRenderMode(kRenderNone);
			pProp->SetRenderColorA(160);
			pProp->SetParent(pHandProp, -1);


			// CREATE THE TELEPORT MARKER
			CDynamicProp* pTeleportProp = dynamic_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic"));

			//safeAngles = pTeleportProp->GetAbsAngles();
			safeAngles.x += 45.0f;

			// Pass in standard key values
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeOrigin.x, safeOrigin.y, safeOrigin.z);
			pTeleportProp->KeyValue("origin", buf);
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", safeAngles.x, safeAngles.y, safeAngles.z);
			pTeleportProp->KeyValue("angles", buf);

			pTeleportProp->KeyValue("fademindist", "-1");
			pTeleportProp->KeyValue("fadescale", "1");
			pTeleportProp->KeyValue("MaxAnimTime", "10");
			pTeleportProp->KeyValue("MinAnimTime", "5");
			pTeleportProp->KeyValue("modelscale", "1.0");
			pTeleportProp->KeyValue("renderamt", "255");
			pTeleportProp->KeyValue("rendercolor", "255 255 255");
			pTeleportProp->KeyValue("solid", "0");
			pTeleportProp->KeyValue("DisableBoneFollowers", "0");
			pTeleportProp->KeyValue("disablereceiveshadows", "1");
			pTeleportProp->KeyValue("disableshadows", "1");
			pTeleportProp->KeyValue("ExplodeDamage", "0");
			pTeleportProp->KeyValue("skin", "0");
			pTeleportProp->KeyValue("ExplodeRadius", "0");
			pTeleportProp->KeyValue("fademaxdist", "0");
			pTeleportProp->KeyValue("maxdxlevel", "0");
			pTeleportProp->KeyValue("mindxlevel", "0");

			pTeleportProp->KeyValue("PerformanceMode", "0");
			pTeleportProp->KeyValue("pressuredelay", "0");
			pTeleportProp->KeyValue("spawnflags", "0");
			pTeleportProp->KeyValue("RandomAnimation", "0");
			pTeleportProp->KeyValue("renderfx", "0");
			pTeleportProp->KeyValue("rendermode", "0");
			pTeleportProp->KeyValue("SetBodyGroup", "0");
			pTeleportProp->KeyValue("StartDisabled", "0");

			pTeleportProp->KeyValue("model", "models/players/hands/vrteleport.mdl");
			pTeleportProp->SetRenderMode(kRenderNone);
			pTeleportProp->SetRenderColorA(160);

			if (DispatchSpawn(pHandProp) > -1)
			{
				if (DispatchSpawn(pTeleportProp) > -1)
				{
					if (DispatchSpawn(pProp) > -1)
					{
						pProp->SetParent(pHandProp, -1);

						CBasePlayer* pRequestingPlayer = UTIL_GetCommandClient();
						edict_t *pClient = engine->PEntityOfEntIndex(pRequestingPlayer->entindex());
						engine->ClientCommand(pClient, "vr_hand_created %i %i %i %i;\n", pHandProp->entindex(), iSide, pProp->entindex(), pTeleportProp->entindex());
					}
				}
			}
		}
	}
}
ConCommand create_vr_hands("create_vr_hands", CreateVRHands, "For internal use only.");

void SetScale(const CCommand &args)
{
	float flScale = Q_atof(args[2]);

	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
	{
		CBaseAnimating* pAnimatingEntity = dynamic_cast<CBaseAnimating*>(pEntity);
		if (pAnimatingEntity)
		{
			//CPropShortcutEntity* pShortcutEntity = dynamic_cast<CPropShortcutEntity*>(pEntity);
			//pShortcutEntity->SetModelScale(flScale, 0.0f);
			if (scale_collisions.GetBool() && pAnimatingEntity->VPhysicsGetObject())
			{






				UTIL_CreateScaledPhysObject(pAnimatingEntity, flScale);
				pAnimatingEntity->CollisionProp()->RefreshScaledCollisionBounds();
			}
			else
				pAnimatingEntity->SetModelScale(flScale, 0.0f);
		}
	}
}
ConCommand setscale("setscale", SetScale, "Interal use only.", FCVAR_HIDDEN);

void GetColMinMax(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
	{
		CPropShortcutEntity* pShortcutEntity = dynamic_cast<CPropShortcutEntity*>(pEntity);
		if (pShortcutEntity->VPhysicsGetObject())
		{
			/*
			//UTIL_CreateScaledPhysObject(pShortcutEntity->GetBaseAnimating(), flScale);
			int iWorked = (UTIL_CreateScaledPhysObjectVerts(pShortcutEntity->GetBaseAnimating())) ? 1 : 0;

			CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
			edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
			engine->ClientCommand(pClient, UTIL_VarArgs("colminmaxgotten %i %i;\n", Q_atoi(args[1]), iWorked));
			*/
		}
	}
}
ConCommand getcolminmax("getcolminmax", GetColMinMax, "Interal use only.", FCVAR_HIDDEN);

void GetTempPinnedCam(const CCommand &args)
{
	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	CBaseEntity* pEntity = gEntList.FindEntityByTarget(NULL, "aatmpcamdrop");
	if (pEntity)
	{
		engine->ClientCommand(pClient, UTIL_VarArgs("gottemppinnedcam %i;\n", pEntity->entindex()));
	}
	else {
		engine->ClientCommand(pClient, "gottemppinnedcam -1;\n");
	}
}
ConCommand gettemppinnedcam("gettemppinnedcam", GetTempPinnedCam, "Interal use only.", FCVAR_HIDDEN);

void SetAngles(const CCommand &args)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(Q_atoi(args[1]));
	if (pEntity)
	{
		QAngle angles;
		angles.x = Q_atoi(args[2]);
		angles.y = Q_atoi(args[3]);
		angles.z = Q_atoi(args[4]);
		pEntity->SetAbsAngles(angles);
	}
}
ConCommand setangles("setangles", SetAngles, "Internal use only.", FCVAR_HIDDEN);

void GetValid2DBBoxes(const CCommand &args)
{
	KeyValues* pVertKV;
	CBaseEntity* pEntity;
	KeyValues* pKV = new KeyValues("entities");
	if (pKV->LoadFromFile(g_pFullFileSystem, "temp_entities.txt", "DEFAULT_WRITE_PATH", true))
	{
		for (KeyValues *pEntityEntryKV = pKV->GetFirstSubKey(); pEntityEntryKV; pEntityEntryKV = pEntityEntryKV->GetNextKey())
		{
			pEntity = CBaseEntity::Instance(pEntityEntryKV->GetInt());

			if (pEntity)
			{
				CPropShortcutEntity* pShortcutEntity = dynamic_cast<CPropShortcutEntity*>(pEntity);
				if (pShortcutEntity && pShortcutEntity->VPhysicsGetObject())
				{
					CBaseAnimating* pInstance = pEntity->GetBaseAnimating();

					// Get our object
					IPhysicsObject *pObject = pInstance->VPhysicsGetObject();
					if (pObject)
					{
						vcollide_t *pCollide = modelinfo->GetVCollide(pInstance->GetModelIndex());
						if (pCollide && pCollide->solidCount > 0)
						{
							// Create a query to get more information from the collision object
							for (unsigned short m = 0; m < 1; m++)// pCollide->solidCount; m++)
							{
								ICollisionQuery *pQuery = physcollision->CreateQueryModel(pCollide->solids[m]);	// FIXME: This should iterate over all solids!
								if (pQuery)
								{
									// Create a container to hold all the convexes we'll create
									const int nNumConvex = pQuery->ConvexCount();
									float flScale = pShortcutEntity->GetModelScale();// 1.0;

									pEntityEntryKV->Clear();

									// For each convex, collect the verts and create a convex from it we'll retain for later
									for (int i = 0; i < nNumConvex; i++)
									{
										int nNumTris = pQuery->TriangleCount(i);
										int nNumVerts = nNumTris * 3;
										// FIXME: Really?  stackalloc?
										Vector *pVerts = (Vector *)stackalloc(sizeof(Vector) * nNumVerts);
										Vector **ppVerts = (Vector **)stackalloc(sizeof(Vector *) * nNumVerts);
										for (int j = 0; j < nNumTris; j++)
										{
											// Get all the verts for this triangle and scale them up
											pQuery->GetTriangleVerts(i, j, pVerts + (j * 3));
											*(pVerts + (j * 3)) *= flScale;
											*(pVerts + (j * 3) + 1) *= flScale;
											*(pVerts + (j * 3) + 2) *= flScale;

											// Setup our pointers (blech!)
											*(ppVerts + (j * 3)) = pVerts + (j * 3);
											*(ppVerts + (j * 3) + 1) = pVerts + (j * 3) + 1;
											*(ppVerts + (j * 3) + 2) = pVerts + (j * 3) + 2;

											for (int l = 0; l < 3; l++)
											{
												pVertKV = pEntityEntryKV->CreateNewKey();
												pVertKV->SetString("", UTIL_VarArgs("%.02f %.02f %.02f", ppVerts[j + l]->x, ppVerts[j + l]->y, ppVerts[j + l]->z));
											}
										}
									}

									// Clean up
									physcollision->DestroyQueryModel(pQuery);
								}
							}
						}
					}
				}
			}
		}
	}

	pKV->SaveToFile(g_pFullFileSystem, "temp_entities.txt", "DEFAULT_WRITE_PATH");
	pKV->deleteThis();

	CBaseEntity* pPlayerEntity = UTIL_GetCommandClient();
	edict_t *pClient = engine->PEntityOfEntIndex(pPlayerEntity->entindex());
	engine->ClientCommand(pClient, "got_valid_2d_bboxes;\n");
}
ConCommand getvalid2dbboxes("get_valid_2d_bboxes", GetValid2DBBoxes, "Interal use only.", FCVAR_HIDDEN);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CreateCar(const CCommand &args)
{
	std::string modelName = (args.ArgC() > 1 ) ? args[1] : "models/buggy.mdl";
	std::string scriptName = (args.ArgC() > 2) ? args[2] : "scripts/vehicles/jeep_test.txt";

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors(pPlayer->EyeAngles(), &vecForward);
	CBaseEntity *pCar = (CBaseEntity *)CreateEntityByName("prop_vehicle_jeep");
	if (pCar)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0, 0, 64);
		QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);
		pCar->SetAbsOrigin(vecOrigin);
		pCar->SetAbsAngles(vecAngles);
		pCar->KeyValue("model", modelName.c_str());
		pCar->KeyValue("solid", "6");
		pCar->KeyValue("targetname", "spawnedcar");
		pCar->KeyValue("vehiclescript", scriptName.c_str());
		DispatchSpawn(pCar);
		pCar->Activate();
		pCar->Teleport(&vecOrigin, &vecAngles, NULL);
	}

	/*
	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors(pPlayer->EyeAngles(), &vecForward);
	CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName("prop_vehicle_jeep");
	if (pJeep)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0, 0, 64);
		QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);
		pJeep->SetAbsOrigin(vecOrigin);
		pJeep->SetAbsAngles(vecAngles);
		pJeep->KeyValue("model", "models/vehicle.mdl");
		pJeep->KeyValue("solid", "6");
		pJeep->KeyValue("targetname", "jeep");
		pJeep->KeyValue("vehiclescript", "scripts/vehicles/jalopy.txt");
		DispatchSpawn(pJeep);
		pJeep->Activate();
		pJeep->Teleport(&vecOrigin, &vecAngles, NULL);
	}*/
}

ConCommand createcar("createcar", CreateCar, "Spawn car in front of the player. Can pass in model name & vehicle script name too (optional.) Default values are models/buggy.mdl and scripts/vehicles/jeep_test.txt", FCVAR_NONE);