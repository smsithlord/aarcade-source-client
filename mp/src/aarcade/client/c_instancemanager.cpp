#include "cbase.h"

#include "aa_globals.h"
#include "c_instancemanager.h"
#include "c_prop_shortcut.h"
#include "filesystem.h"
#include "icliententitylist.h"
#include "c_anarchymanager.h"
#include "../../game/client/view.h"	// for MainViewOrigin
#include <algorithm> // for sort
//#include <vector>       // std::vector
bool sortPlayerDistB(object_t* pObjectA, object_t* pObjectB)
{
	return (pObjectA->origin.DistTo(C_BasePlayer::GetLocalPlayer()->GetAbsOrigin())>pObjectB->origin.DistTo(C_BasePlayer::GetLocalPlayer()->GetAbsOrigin()));
}

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_InstanceManager::C_InstanceManager()
{
	DevMsg("WebManager: Constructor\n");
	m_bHadDebugText = false;
	m_fLastSpawnActionPressed = 0;
	m_uNextFlashedObject = 0;
	m_pInstanceKV = null;
	m_pActiveMaterialMods = null;

	m_pTransform = new transform_t();
	m_pTransform->offX = 0;
	m_pTransform->offY = 0;
	m_pTransform->offZ = 0;
	m_pTransform->rotP = 0;
	m_pTransform->rotY = 0;
	m_pTransform->rotR = 0;
	m_pTransform->scale = 1.0;
	m_pTransform->pseudo = false;

	m_pRecentModelIdConVar = cvar->FindVar("recent_model_id");//null;
	m_pDebugObjectSpawnConVar = cvar->FindVar("debug_object_spawn");
	m_pSkipObjectsConVar = cvar->FindVar("skip_objects");
	m_pReShadeConVar = cvar->FindVar("reshade");
	m_pReShadeDepthConVar = cvar->FindVar("reshadedepth");
	m_pVGUIConVar = cvar->FindVar("r_drawvgui");
	m_pNearestSpawnDistConVar = cvar->FindVar("spawn_dist");
	m_fNearestSpawnDist = m_pNearestSpawnDistConVar->GetFloat();	//0.0f;
	m_pSpawnObjectsDoubleTapConVar = cvar->FindVar("spawn_objects_double_tap");
	m_pSpawnObjectsWithinViewOnlyConVar = cvar->FindVar("spawn_objects_within_view_only");

	m_incomingNodeId = "";

	m_iUnspawnedWithinRangeEstimate = 0;

	SetDefLessFunc(m_instances);
}

C_InstanceManager::~C_InstanceManager()
{
	DevMsg("ShortcutManager: Destructor\n");
	//m_instances.clear();
	m_instances.RemoveAll();	// NOTE: Might need to do a Purge&Delete instead!!
}

void C_InstanceManager::ResetObjectChanges(C_PropShortcutEntity* pShortcut)
{
	KeyValues* pEntryKV = m_pInstanceKV->FindKey(VarArgs("objects/%s", pShortcut->GetObjectId().c_str()));

	// check if this object has been saved to the KV yet
	if (pEntryKV)
	{
		// yes, it already exists and has values to revert to
		object_t* pObject = this->GetInstanceObject(pShortcut->GetObjectId());
		// revert
		// TODO: everything should be condensed into a single server-command

		// FOR NOW: start by setting object ID's
		std::string modelId = pObject->modelId;// pShortcut->GetModelId();
		std::string modelFile;

		KeyValues* entryModel = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId);
		KeyValues* activeModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(entryModel);
		if (activeModel)
			modelFile = activeModel->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID), "models\\cabinets\\two_player_arcade.mdl");	// uses default model if key value read fails
		else
			modelFile = "models\\cabinets\\two_player_arcade.mdl";

		pShortcut->PrecacheModel(modelFile.c_str());	// not needed.  handled server-side?
		pShortcut->SetModel(modelFile.c_str());	// not needed.  handled server-side?
		//engine->ServerCmd(VarArgs("setobjectids %i \"%s\" \"%s\" \"%s\";\n", pShortcut->entindex(), modelId.c_str(), modelId.c_str(), modelFile.c_str()), false);

		// FOR NOW: then do position, rotation, and scale
		//Vector origin = pShortcut->GetAbsOrigin();
		//QAngle angles = pShortcut->GetAbsAngles();
		//engine->ServerCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z), false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.

		// FOR NOW: lastly, do scale
//		engine->ServerCmd(VarArgs("switchmodel \"%s\" \"%s\" %i;\n", modelId.c_str(), modelFile.c_str(), pShortcut->entindex()));	// TODO: This shouldn't be required cu setobjectids is supposed to do everything this does...
		engine->ClientCmd(VarArgs("setobjectids %i \"%s\" \"%s\" \"%s\";\n", pShortcut->entindex(), pObject->itemId.c_str(), modelId.c_str(), modelFile.c_str()));	// servercmdfix , false);
		engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z));	// servercmdfix , false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.
		engine->ClientCmd(VarArgs("setscale %i %f;\n", pShortcut->entindex(), pObject->scale));	// servercmdfix , false);
		engine->ClientCmd(VarArgs("setbody %i %i;\n", pShortcut->entindex(), pObject->body));
		engine->ClientCmd(VarArgs("setskin %i %i;\n", pShortcut->entindex(), pObject->skin));

		if (pObject->child)
			engine->ClientCmd(VarArgs("setparent %i %i;\n", pShortcut->entindex(), pObject->parentEntityIndex));
		//engine->ServerCmd(VarArgs("setobjectids %i \"%s\" \"%s\" \"%s\"; setcabpos %i %f %f %f %f %f %f; setscale %i %f;", pShortcut->entindex(), pObject->itemId.c_str(), modelId.c_str(), modelFile.c_str(), pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z, pShortcut->entindex(), pObject->scale), false);
		
		// undo build mode FX
		engine->ClientCmd(VarArgs("makenonghost %i %i;\n", pShortcut->entindex(), g_pAnarchyManager->UseBuildGhosts()));	// servercmdfix , false);

		if (pObject->anim != "")
			pShortcut->PlaySequenceRegular(pObject->anim.c_str());

		// if this item is a 3d text item, it needs to resize its children.
		std::string itemId = pObject->itemId;
		if (itemId != modelId)
		{
			KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId));
			if (pItemKV)
			{
				// check file field for being a text item w/ ?&m=1
				std::string file = pItemKV->GetString("file");
				if (file.find("http://text.txt/") == 0 && (file.find("&m=") != std::string::npos || file.find("?m=") != std::string::npos))
				{
					// it is a text item
					size_t foundAt = file.find("?txt=");
					if (foundAt == std::string::npos)
						foundAt = file.find("&txt=");
					if (foundAt != std::string::npos)
					{
						std::string paramValue = file.substr(foundAt + 5);
						foundAt = paramValue.find_first_of("&#");
						if (foundAt != std::string::npos)
							paramValue = paramValue.substr(0, foundAt);

						if (paramValue != "")
						{
							std::transform(paramValue.begin(), paramValue.end(), paramValue.begin(), ::toupper);
							//DevMsg("ParamValue: %s\n", paramValue.c_str());

							engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", pShortcut->entindex(), paramValue.c_str()));
						}
					}
				}
			}
		}

		/*
		pShortcut->SetRenderColorA(255);
		pShortcut->SetRenderMode(kRenderNormal);

		// make the prop solid
		pShortcut->SetSolid(SOLID_VPHYSICS);
		pShortcut->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		pShortcut->SetMoveType(MOVETYPE_VPHYSICS);

		if (pShortcut->CreateVPhysics())
		{
			IPhysicsObject *pPhysics = pShortcut->VPhysicsGetObject();
			if (pPhysics)
			{
				pPhysics->EnableMotion(false);
			}
		}
		*/
	}
	else
	{
		// nothing to revert to, just remove it.
		this->RemoveEntity(pShortcut);
	}
}

void C_InstanceManager::ModelFileChanged(std::string modelId)
{
	// 1. find every object using this model
	//std::map<std::string, object_t*> m_objects;
	std::string modelFile;
	KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId));
	if (pModelKV)
		modelFile = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));

	if (modelFile != "")
	{
		C_BaseEntity* pBaseEntity;
		C_PropShortcutEntity* pShortcut;
		object_t* pObject;
		int entityIndex;
		auto it = m_objects.begin();
		while (it != m_objects.end())
		{
			pObject = it->second;

			if (pObject->modelId == modelId)
			{
				entityIndex = pObject->entityIndex;

				if (entityIndex >= 0)
				{
					pBaseEntity = C_BaseEntity::Instance(entityIndex);
					if (pBaseEntity)
					{
						pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
						this->ChangeModel(pBaseEntity, modelId, modelFile, false);// g_pAnarchyManager->UseBuildGhosts());
					}
				}
			}

			it++;
		}
	}

		// 2. re-initialize that object so it re-reads the model file name from it's model.
		// 3. fin
}

void C_InstanceManager::SetMaterialMod(std::string material, std::string materialVarName, std::string materialVarValue, bool bDoSave)
{
	if (!this->GetCurrentInstance())
		return;

	KeyValues* pOverridesKV = this->GetCurrentInstanceKV();
	if (!pOverridesKV)
		return;

	// In case this material already is modified, unapply it's current modifications.
	//this->UnapplyMaterialMod(material);

	std::string materialId = g_pAnarchyManager->GenerateLegacyHash(material.c_str());
	KeyValues* pMaterialModKV = pOverridesKV->FindKey(VarArgs("overrides/materials/%s", materialId.c_str()), true);
	pMaterialModKV->SetString("material", material.c_str());
	pMaterialModKV->SetString(materialVarName.c_str(), materialVarValue.c_str());

	// save the changes
	if (bDoSave && (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost))
		this->SaveActiveInstance();

	// now that the mod is set & saved, actually apply it.
	this->ApplyMaterialMod(material, materialVarName, materialVarValue);
}

void C_InstanceManager::ClearMaterialMod(std::string material, bool bDoSave)
{
	if (!this->GetCurrentInstance())
		return;

	KeyValues* pOverridesKV = this->GetCurrentInstanceKV();
	if (pOverridesKV)
	{
		// In case this material already is modified, unapply it's current modifications.
		this->UnapplyMaterialMod(material);

		std::string materialId = g_pAnarchyManager->GenerateLegacyHash(material.c_str());
		KeyValues* pMaterialModsKV = pOverridesKV->FindKey("overrides/materials", true);
		KeyValues* pMaterialModKV = pMaterialModsKV->FindKey(materialId.c_str());
		if (pMaterialModKV)
			pMaterialModsKV->RemoveSubKey(pMaterialModKV);
	}

	// save the changes
	if (bDoSave && (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost))
		this->SaveActiveInstance();
}

void C_InstanceManager::ClearAllMaterialMods()
{
	if (!this->GetCurrentInstance())
		return;

	KeyValues* pOverridesKV = this->GetCurrentInstanceKV();
	if (!pOverridesKV)
		return;

	pOverridesKV = pOverridesKV->FindKey("overrides", true);

	KeyValues* pMaterialModsKV = pOverridesKV->FindKey("materials");
	if (pMaterialModsKV)
	{
		std::string material;
		KeyValues* pMaterialModKV = null;
		for (KeyValues *sub = pMaterialModsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			material = sub->GetString("material");

			pMaterialModKV = m_pActiveMaterialMods->FindKey(sub->GetName());
			// if it's already active, we should unapply it first.
			if (pMaterialModKV)
				this->UnapplyMaterialMod(material);
		}

		pOverridesKV->RemoveSubKey(pMaterialModsKV);

		// save the changes
		if (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost)
			this->SaveActiveInstance();
	}
}

void C_InstanceManager::ApplyMaterialMod(std::string material, std::string materialVarName, std::string materialVarValue)
{
	//KeyValues* pMaterialModKV = m_pActiveMaterialMods->FindKey(materialId.c_str());
	std::string oldValue = g_pAnarchyManager->ModifyMaterial(material, materialVarName, materialVarValue);

	// add it for bookkeeping
	std::string materialId = g_pAnarchyManager->GenerateLegacyHash(material.c_str());
	KeyValues* pMaterialModKV = m_pActiveMaterialMods->FindKey(materialId.c_str());// , true);

	if (!pMaterialModKV)
	{
		pMaterialModKV = m_pActiveMaterialMods->FindKey(materialId.c_str(), true);
		pMaterialModKV->SetString("material", material.c_str());
		pMaterialModKV->SetString(materialVarName.c_str(), oldValue.c_str());
	}
}

void C_InstanceManager::UnapplyMaterialMod(std::string material)
{
	std::string materialId = g_pAnarchyManager->GenerateLegacyHash(material.c_str());
	KeyValues* pMaterialModKV = m_pActiveMaterialMods->FindKey(materialId.c_str());
	if (!pMaterialModKV)
		return;

	///* OBSOLETE: Just remove ALL material mods instead by refreshing the material.
	for (KeyValues *sub = pMaterialModKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (!Q_strcmp(sub->GetName(), "material"))
			continue;

		g_pAnarchyManager->ModifyMaterial(material, sub->GetName(), sub->GetString());
	}

	m_pActiveMaterialMods->RemoveSubKey(pMaterialModKV);
	
	/*
	IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(material.c_str(), TEXTURE_GROUP_WORLD);
	if (!pMaterial || pMaterial->IsErrorMaterial())
		pMaterial = g_pMaterialSystem->FindMaterial(material.c_str(), TEXTURE_GROUP_MODEL);

	if (pMaterial && !pMaterial->IsErrorMaterial())
		pMaterial->Refresh();	// this also fixes lighting issues
	*/
}

void C_InstanceManager::ApplyAllMaterialMods()
{
	if (!this->GetCurrentInstance())
		return;

	KeyValues* pOverridesKV = this->GetCurrentInstanceKV();
	if (!pOverridesKV)
		return;

	KeyValues* pMaterialModsKV = pOverridesKV->FindKey("overrides/materials", true);

	std::string material;
	KeyValues* pMaterialModKV = null;
	for (KeyValues *sub = pMaterialModsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		material = sub->GetString("material");

		pMaterialModKV = m_pActiveMaterialMods->FindKey(sub->GetName(), true);
		pMaterialModKV->SetString("material", material.c_str());
		for (KeyValues *sub2 = sub->GetFirstSubKey(); sub2; sub2 = sub2->GetNextKey())
		{
			if (!Q_strcmp(sub2->GetName(), "material"))
				continue;

			std::string oldValue = g_pAnarchyManager->ModifyMaterial(material, sub2->GetName(), sub2->GetString());
			pMaterialModKV->SetString(sub2->GetName(), oldValue.c_str());
		}
	}
}

void C_InstanceManager::ResetAllMaterialMods()
{
	if (m_pActiveMaterialMods)
	{
		std::string material;
		for (KeyValues *sub = m_pActiveMaterialMods->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			material = sub->GetString("material");
			for (KeyValues *sub2 = sub->GetFirstSubKey(); sub2; sub2 = sub2->GetNextKey())
			{
				if (!Q_strcmp(sub2->GetName(), "material"))
					continue;

				g_pAnarchyManager->ModifyMaterial(material, sub2->GetName(), sub2->GetString());
			}
		}

		m_pActiveMaterialMods->deleteThis();
	}

	m_pActiveMaterialMods = new KeyValues("overrides");
}

void C_InstanceManager::ResetTransform()
{
	m_pTransform->offX = 0;
	m_pTransform->offY = 0;
	m_pTransform->offZ = 0;
	m_pTransform->rotP = 0;
	m_pTransform->rotY = 0;
	m_pTransform->rotR = 0;
	m_pTransform->scale = 1.0;
	m_pTransform->pseudo = false;
}

void C_InstanceManager::AdjustObjectOffset(float x, float y, float z)
{
	m_pTransform->offX = x;
	m_pTransform->offY = y;
	m_pTransform->offZ = z;
	//m_pTransform->pseudo = false;
}

void C_InstanceManager::AdjustObjectRot(float p, float y, float r)
{
	m_pTransform->rotP = p;
	m_pTransform->rotY = y;
	m_pTransform->rotR = r;
	//m_pTransform->pseudo = false;
}

void C_InstanceManager::AdjustObjectScale(float scale)
{
	if (scale < 0.05)
		m_pTransform->scale = -0.05;
	else
		m_pTransform->scale = scale;
}

void C_InstanceManager::AdjustTransformPseudo(bool bValue)
{
	m_pTransform->pseudo = bValue;
}

void C_InstanceManager::SaveActiveInstance(KeyValues* pInstanceKV, bool bForceNoSave)
{
	if (!pInstanceKV)
		pInstanceKV = m_pInstanceKV;

	std::string instanceId = pInstanceKV->GetString("info/local/id");

	//DevMsg("Saving instance ID %s\n", instanceId.c_str());
	// THIS is where you'd put code for some sort of save indicator icon in the HUD.

	// now save out to the SQL
	g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", instanceId.c_str(), pInstanceKV, false, bForceNoSave);
}

void C_InstanceManager::ApplyChanges(C_PropShortcutEntity* pShortcut, bool bShouldSave, std::string objectIdOverride, std::string itemIdOverride)
{
	if (!m_pInstanceKV)
	{
		//DevMsg("WARNING: This is the 1st time this instance has been modified by the local user.  It's KeyValues structure must be generated first, before changes can be saved.\n");

		DevMsg("ERROR: No KEY for this instance!\n");
		return;
	}

	// untag as legacy, because any instance that gets saved out gets saved out in REDUX format
	instance_t* instance = this->GetCurrentInstance();
	if (instance && instance->legacy)
	{
		//KeyValues* pLegacyKV = m_pInstanceKV->FindKey("legacy");

		//if (pLegacyKV)
			//pLegacyKV->SetInt("", 0);
//			m_pInstanceKV->RemoveSubKey(pLegacyKV);
		m_pInstanceKV->SetInt("legacy", 0);
		instance->legacy = 0;
		instance->file = "";
	}

	std::string objectId = (pShortcut) ? pShortcut->GetObjectId() : objectIdOverride;

	// update this object's object_t
	object_t* object = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!object)
		DevMsg("FATAL ERROR: This shortcut has no object data struct!\n");

	if (pShortcut)
	{
		object->scale = pShortcut->GetModelScale();
		object->itemId = (itemIdOverride != "") ? itemIdOverride : pShortcut->GetItemId();
		object->modelId = pShortcut->GetModelId();

		int sequence = pShortcut->GetSequence();
		if (sequence != ACT_INVALID)
		{
			std::string sequenceName = pShortcut->GetSequenceName(sequence);
			if (sequenceName == "inactiveidle" || sequenceName == "activated" || sequenceName == "activeidle" || sequenceName == "deactivated")
				object->anim = "";
			else
				object->anim = sequenceName;
		}

		object->body = pShortcut->GetBody();
		object->skin = pShortcut->GetSkin();

		/*
		object->origin = (object->child) ? pShortcut->GetLocalOrigin() : pShortcut->GetAbsOrigin();
		object->angles = (object->child) ? pShortcut->GetLocalAngles() : pShortcut->GetAbsAngles();
		*/
		object->origin = pShortcut->GetAbsOrigin();
		object->angles = pShortcut->GetAbsAngles();
		object->slave = pShortcut->GetSlave();
	}
	// else, the object is already updated with the values we want prior to us being called

	KeyValues* pObjectKV = m_pInstanceKV->FindKey(VarArgs("objects/%s", objectId.c_str()), true);

	// position
	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->origin.x, object->origin.y, object->origin.z);
	std::string position = buf;

	// rotation
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->angles.x, object->angles.y, object->angles.z);
	std::string rotation = buf;

	int slave = (object->slave) ? 1 : 0;
	int child = (object->child) ? 1 : 0;

	this->CreateObject(pObjectKV, objectId, object->itemId, object->modelId, position, rotation, object->scale, object->anim, slave, child, object->body, object->skin);

	if (bShouldSave)
	{
		this->SaveActiveInstance();
		//DevMsg("Saving instance ID %s vs %s\n", m_pInstanceKV->GetString("info/local/id"), instance->id.c_str());

		// now save out to the SQL
		//g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", m_pInstanceKV->GetString("info/local/id"), m_pInstanceKV);
	}
}

void C_InstanceManager::CacheAllInstanceModels()
{
	// obsolete!
	/*
	if (!m_pInstanceKV)
		return;

	KeyValues* pPrecacheModelsKV = new KeyValues("models");

	KeyValues* pModelKV;
	std::string model;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(it->second->modelId));
		model = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));

		pPrecacheModelsKV->SetString(it->second->modelId.c_str(), model.c_str());
		//C_BaseEntity::PrecacheModel(model.c_str());
		//DevMsg("Caching %s\n", model.c_str());
		it++;
	}

	pPrecacheModelsKV->SaveToFile(g_pFullFileSystem, "precache_models.txt", "DEFAULT_WRITE_PATH");
	*/
}

void C_InstanceManager::SetObjectEntity(std::string objectId, C_BaseEntity* pEntity)
{
	object_t* pObject = this->GetInstanceObject(objectId);
	pObject->entityIndex = pEntity->entindex();
}

C_BaseEntity* C_InstanceManager::GetObjectEntity(std::string objectId)
{
	object_t* pObject = this->GetInstanceObject(objectId);
	if (pObject && pObject->entityIndex != -1)
		return C_BaseEntity::Instance(pObject->entityIndex);
	else
		return null;
}

void C_InstanceManager::CreateObject(KeyValues* pObjectKV, std::string objectId, std::string itemId, std::string modelId, std::string position, std::string rotation, float scale, std::string anim, int slave, int child, int body, int skin)
{
	////bool slave = (sub->GetInt("local/slave") > 0);	// FIXME: slave mode needs to be determined somehow
	//bool physics = (sub->GetInt("slave") > 0); // FIXME: TODO: Use the physics stuff too!

	// assume "local" user key.
	pObjectKV->SetString("local/info/id", objectId.c_str());
	pObjectKV->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	pObjectKV->SetString("local/info/owner", "local");
	pObjectKV->SetString("local/item", itemId.c_str());
	pObjectKV->SetString("local/model", modelId.c_str());
	pObjectKV->SetString("local/anim", anim.c_str());
	pObjectKV->SetString("local/position", position.c_str());
	pObjectKV->SetString("local/rotation", rotation.c_str());
	pObjectKV->SetFloat("local/scale", scale);
	pObjectKV->SetInt("local/slave", slave);
	pObjectKV->SetInt("local/child", child);
	pObjectKV->SetInt("local/body", body);
	pObjectKV->SetInt("local/skin", skin);
}

// NOTE: This is a loose search, for now.  It's meant specifically to find URLs that match, but might not have the same https http www prefix.
std::string C_InstanceManager::GetObjectWithFile(std::string file)
{
	std::string testFile;
	KeyValues* pItemKV;
	std::string objectId = "";
	auto it = m_objects.begin();
	while (it != m_objects.end())
	{
		if (it->second->spawned)
		{
			pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(it->second->itemId));
			if (pItemKV )//&& !Q_stricmp(pItemKV->GetString("file"), file.c_str()))
			{
				testFile = pItemKV->GetString("file");
				if (testFile.find(file) != std::string::npos)
				{
					objectId = it->first;
					break;
				}
			}
		}

		it++;
	}

	return objectId;
}

std::string C_InstanceManager::GetObjectWithModelId(std::string modelId, bool bOnlySpawned)
{
	auto it = m_objects.begin();
	while (it != m_objects.end())
	{
		if ((!bOnlySpawned || it->second->spawned) && it->second->modelId == modelId)
		{
			return it->second->objectId;
		}
		it++;
	}

	return "";
}

std::string C_InstanceManager::GetObjectWithItemId(std::string itemId, bool bOnlySpawned)
{
	auto it = m_objects.begin();
	while (it != m_objects.end())
	{
		if ((!bOnlySpawned || it->second->spawned) && it->second->itemId == itemId)
		{
			return it->second->objectId;
		}
		it++;
	}

	return "";
}

std::string C_InstanceManager::CreateBlankInstance(int iLegacy, KeyValues* pInstanceKV, std::string instanceId, std::string mapId, std::string title, std::string file, std::string workshopIds, std::string mountIds, std::string style, C_Backpack* pBackpack)
{
	// DO WORK
	// (create a new instance, save it to the db, then overwrite the instanceId with the newly created id.)

	if (instanceId == "")
		instanceId = g_pAnarchyManager->GenerateUniqueId();

	if (title == "")
	{
		if (workshopIds != "")
		{
			SteamWorkshopDetails_t* pDetails = g_pAnarchyManager->GetWorkshopManager()->GetWorkshopSubscription((PublishedFileId_t)Q_atoui64(workshopIds.c_str()));
			if (pDetails)
				title = "Unnamed (" + pDetails->title + ")";
		}

		if (title == "")
			title = "Unnamed (" + instanceId + ")";
	}

	bool bNeedCleanup = false;
	if (!pInstanceKV)
	{
		// If we are creating the KV here, then save it out & clean it up right away as well.
		bNeedCleanup = true;
		pInstanceKV = new KeyValues("instance");
	}
	else
		pInstanceKV->Clear();	// Clear out the KV if we are given one (because we are making it a BLANK instance KV)

	// set workshop & legacy versioning
	pInstanceKV->SetInt("generation", 3);
	pInstanceKV->SetInt("legacy", iLegacy);

	// set local 
	KeyValues* pInstanceInfoKV = pInstanceKV->FindKey("info/local", true);
	pInstanceInfoKV->SetString("id", instanceId.c_str());
	pInstanceInfoKV->SetString("created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	pInstanceInfoKV->SetString("owner", "local");
	pInstanceInfoKV->SetString("title", title.c_str());
	pInstanceInfoKV->SetString("map", mapId.c_str());
	pInstanceInfoKV->SetString("style", style.c_str());	// for nodes
	pInstanceInfoKV->SetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID), workshopIds.c_str());
	pInstanceInfoKV->SetString(VarArgs("platforms/%s/mountIds", AA_PLATFORM_ID), mountIds.c_str());
	//pInstanceInfoKV->SetString(VarArgs("platforms/%s/backpackIds", AA_PLATFORM_ID), backpackId.c_str());

	if (!pBackpack)
		g_pAnarchyManager->GetInstanceManager()->AddInstance(iLegacy, instanceId, mapId, title, file, workshopIds, mountIds, "", style);

	if (bNeedCleanup)
	{
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", instanceId.c_str(), pInstanceKV, true, false);	// force this.  unfortunately, it *must* exist in the DB so it can be loaded after connecting to the server hosting the instance.
		pInstanceKV->deleteThis();
		pInstanceKV = null;
	}

	return instanceId;
}

// Note that this bIsAutoplayObject is more like "Spawn as FL_EDICT_ALWAYS". It should really be renamed so it's not confused with the real autoplay objects.
void C_InstanceManager::SpawnObject(object_t* object, bool bIsAutoplayObject)
{
	auto it = std::find(m_unspawnedObjects.begin(), m_unspawnedObjects.end(), object);
	if (it != m_unspawnedObjects.end())
		m_unspawnedObjects.erase(it);
	else
		return;
	
	if (m_pDebugObjectSpawnConVar->GetBool())
	{
		DevMsg("Spawning object: %s\n", object->objectId.c_str());

		std::string skipObjects = m_pSkipObjectsConVar->GetString();
		if (skipObjects.find(object->objectId) != std::string::npos)
		{
			object->spawned = true;
			return;
		}
	}

	object->spawned = true;	// FIXME: This really shouldn't be set to true until after it exists on the client. there should be a different state for waiting to spawn.
	//DevMsg("Here it is: %s vs %s\n", object->modelId.c_str(), object->itemId.c_str());
	std::string goodModelId = (object->modelId != "") ? object->modelId : object->itemId;

	// is this a node?
	if (object->modelId != object->itemId && object->itemId != "")
	{
		KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", object->itemId));
		if (pItemKV )
		{
			KeyValues* pTypeKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("types", pItemKV->GetString("type")));
			if (pTypeKV && !Q_stricmp(pTypeKV->GetString("title"), "node"))//!Q_stricmp(pTypeKV->GetString("title"), "node"))
			{
				goodModelId = g_pAnarchyManager->GetMetaverseManager()->GetSpecialModelId("node");
				object->modelId = goodModelId;// "models\\cabinets\\node.mdl";
			}
		}
	}

	std::string modelFile;
	KeyValues* model = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(goodModelId);
	if (!model)
	{
		//if (goodModelId == "1e258e3d")
		//	DevMsg("Tester output.\n");

		DevMsg("WARNING: Model not found in library with given ID! Using default cabinet instead. %s\n", goodModelId.c_str());
		//goodModelId = g_pAnarchyManager->GenerateLegacyHash("models/cabinets/two_player_arcade.mdl");
		//goodModelId = g_pAnarchyManager->GenerateLegacyHash("models/icons/missing.mdl");
		//model = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(goodModelId);
		modelFile = "models/icons/missing.mdl";
	}
	else
	{
		KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(model);
		modelFile = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));

		if (!g_pFullFileSystem->FileExists(modelFile.c_str(), "GAME"))
			modelFile = "models/icons/missing.mdl";
	}

	//int ghost = (bShouldGhost) ? 1 : 0;
	bool bShouldGhost = g_pAnarchyManager->UseBuildGhosts();
	bool bIsNewObject = (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject() == object);

	int iIsSlaveObject = (object->slave) ? 1 : 0;
	//DevMsg("Yar: %i\n", iIsSlaveObject);

	bool bShouldBeAutoplayObject = (this->GetCurrentInstance() && this->GetCurrentInstance()->autoplayId != "" && this->GetCurrentInstance()->autoplayId == object->objectId) ? 1 : 0;
	if (bShouldBeAutoplayObject && !cvar->FindVar("autoplay_enabled")->GetBool())
		bShouldBeAutoplayObject = false;

	int iGoodIsAutoplayObject = (bShouldBeAutoplayObject || bIsAutoplayObject) ? 1 : 0;

	std::string msg = VarArgs("spawnshortcut \"%s\" \"%s\" \"%s\" \"%s\" %.10f %.10f %.10f %.10f %.10f %.10f %.10f %i %i %i %i %i %i %i\n", object->objectId.c_str(), object->itemId.c_str(), goodModelId.c_str(), modelFile.c_str(), object->origin.x, object->origin.y, object->origin.z, object->angles.x, object->angles.y, object->angles.z, object->scale, iIsSlaveObject, object->body, object->skin, object->parentEntityIndex, bShouldGhost, bIsNewObject, iGoodIsAutoplayObject);
	engine->ExecuteClientCmd(msg.c_str());	// servercmdfix , false);ClientCmd

//	if (m_unspawnedObjects.empty())	// doing this msg here is probably spamming it.
	//{
		// update rich presence objectcount
	//	steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", this->GetInstanceObjectCount()));
	//}
}

//#include <iostream>     // std::cout
//#include <algorithm>    // std::sort
//#include <vector>       // std::vector
//bool sortPlayerDist(object_t* pObjectA, object_t* pObjectB)
//{
//	return (pObjectA->origin.DistTo(C_BasePlayer::GetLocalPlayer()->GetAbsOrigin())>pObjectB->origin.DistTo(C_BasePlayer::GetLocalPlayer()->GetAbsOrigin()));
//}

void C_InstanceManager::GetObjectsInSight(std::vector<object_t*> &objects)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseEntity* pEntity;

	Vector startPos = pPlayer->GetAbsOrigin();

	object_t* pTestObject = null;
	float fMaxDist = -1;
	float fTestDist;

	trace_t tr;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pTestObject = it->second;
		fTestDist = pTestObject->origin.DistTo(startPos);

		//UTIL_TraceLine(pPlayer->EyePosition(), pTestObject->origin, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
		//CTraceFilterSkipClassname traceFilter(pPlayer, "prop_shortcut", COLLISION_GROUP_NONE);
		//UTIL_TraceLine(pPlayer->EyePosition(), pTestObject->origin, MASK_SOLID, &traceFilter, &tr);
		//if (tr.m_pEnt && (fMaxDist == -1 || fTestDist < fMaxDist) && pPlayer->IsInFieldOfView(pTestObject->origin) && tr.fraction != 1.0 && tr.DidHitNonWorldEntity() && tr.m_pEnt->entindex() == pTestObject->entityIndex)
		//	objects.push_back(pTestObject);

		if( (fMaxDist == -1 || fTestDist < fMaxDist) && pPlayer->IsInFieldOfView(pTestObject->origin) )
			objects.push_back(pTestObject);

		it++;
	}

	std::sort(objects.begin(), objects.end(), sortPlayerDistB);
}

object_t* C_InstanceManager::GetNearestObjectToObject(std::string mode, object_t* pOriginalObject, object_t* pCurrentObject, bool bYouTubeOnly, float flMaxRange)
{
	// NOTE: flMaxRange not actually used here.

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer || pPlayer->GetHealth() <= 0)
		return false;

	//object_t* pPreviousNearObject = null;
	object_t* pNearObject = null;
	object_t* pTestObject = null;
	float fMinDist = (pCurrentObject && pCurrentObject != pOriginalObject) ? pOriginalObject->origin.DistTo(pCurrentObject->origin) : 0;
	float fTestDist;

	bool bIsNextMode = (mode == "next");
	float fBestDist = -1;// (bIsNextMode) ? -1 : MAX_COORD_RANGE;

	bool bIsTubeId = false;
	KeyValues* pItemKV = null;
	//bool bIsPastCurrentObject = (pCurrentObject) ? false : true;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pTestObject = it->second;
		/*
		if (bIsNextMode && !bIsPastCurrentObject)
		{
			if (pTestObject == pCurrentObject)
				bIsPastCurrentObject = true;
		}
		else */
		if ((pTestObject != pOriginalObject || !bIsNextMode) && pTestObject != pCurrentObject)
		{
			fTestDist = pTestObject->origin.DistTo(pOriginalObject->origin);
			if ((bIsNextMode && fTestDist >= fMinDist) || (!bIsNextMode && (fMinDist == 0 || fTestDist <= fMinDist)))
			{
				//if (fBestDist == -1 || fTestDist < fBestDist)
				if (fBestDist == -1 || (bIsNextMode && fTestDist <= fBestDist) || (!bIsNextMode && fTestDist >= fBestDist))
				{
					if (bYouTubeOnly)
					{
						// make sure we have a YouTube URL to use
						pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pTestObject->itemId));
						if (pItemKV && (g_pAnarchyManager->IsYouTube(pItemKV->GetString("file")) || g_pAnarchyManager->IsYouTube(pItemKV->GetString("preview")) || g_pAnarchyManager->IsYouTube(pItemKV->GetString("stream"))))
							bIsTubeId = true;
					}

					if (!bYouTubeOnly || bIsTubeId)
					{
						fBestDist = fTestDist;
						//pPreviousNearObject = pNearObject;
						pNearObject = pTestObject;
						bIsTubeId = false;
					}
				}
			}
		}

		it++;
	}

	//if (bIsNextMode)
	return pNearObject;
	//else
	//	return pPreviousNearObject;
}

object_t* C_InstanceManager::GetNearestObjectToPlayerLook(object_t* pStartingObject, float flMaxRange)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	if (pPlayer->GetHealth() <= 0)
		return false;

	/*
	// fire a trace line
	trace_t tr;
	g_pAnarchyManager->SelectorTraceLine(tr);
	//Vector forward;
	//pPlayer->EyeVectors(&forward);
	//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	Vector startPos = tr.endpos;
	*/

	Vector startPos = g_pAnarchyManager->GetSelectorTraceVector();

	object_t* pNearObject = null;
	object_t* pTestObject = null;
	float fMinDist = flMaxRange;	// the textDist must be LESS THAN this to be included
	float fTestDist;

	float fStartingDist = 0.0f;
	if (pStartingObject)
		fStartingDist = pStartingObject->origin.DistTo(startPos);

	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pTestObject = it->second;
		fTestDist = pTestObject->origin.DistTo(startPos);
		if (fTestDist > fStartingDist && (fMinDist == -1 || fTestDist < fMinDist))
		{
			fMinDist = fTestDist;
			pNearObject = pTestObject;
		}

		it++;
	}

	return pNearObject;
}

object_t* C_InstanceManager::AddObject(std::string objectId, std::string itemId, std::string modelId, Vector origin, QAngle angles, float scale, std::string anim, bool slave, int body, int skin, unsigned int created, std::string owner, unsigned int removed, std::string remover, unsigned int modified, std::string modifier, bool isChild, int iParentEntityIndex)
{
	std::string goodObjectId = (objectId != "") ? objectId : g_pAnarchyManager->GenerateUniqueId();

	//DevMsg("Object ID here is: %s\n", goodObjectId.c_str());

	object_t* pObject = new object_t();
	pObject->objectId = goodObjectId;
	pObject->created = created;
	pObject->owner = owner;
	pObject->removed = removed;
	pObject->remover = remover;
	pObject->modified = modified;
	pObject->modifier = modifier;
	pObject->itemId = itemId;
	pObject->modelId = modelId;
	pObject->origin.Init(origin.x, origin.y, origin.z);
	pObject->angles.Init(angles.x, angles.y, angles.z);
	//pObject->child = (iParentEntityIndex >= 0) ? true : isChild;
	pObject->child = isChild;
	pObject->spawned = false;
	pObject->scale = scale;
	pObject->slave = slave;
	pObject->entityIndex = -1;
	pObject->parentEntityIndex = iParentEntityIndex;
	pObject->anim = anim;
	pObject->body = body;
	pObject->skin = skin;

	m_objects[goodObjectId] = pObject;

	m_unspawnedObjects.push_back(pObject);
	return pObject;
}

unsigned int C_InstanceManager::GetInstanceObjectCount()
{
	/*
	unsigned int count = 0;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		count++;
		it++;
	}

	return count;
	*/

	return m_objects.size();
}

object_t* C_InstanceManager::GetInstanceObject(std::string objectId)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end())
		return it->second;
	
	return null;
}

void C_InstanceManager::RemoveEntity(C_PropShortcutEntity* pShortcutEntity, bool bBulkRemove, std::string objectOverrideId)
{
	int iEntityIndex = (pShortcutEntity) ? pShortcutEntity->entindex() : 0;
	C_EmbeddedInstance* pTesterInstance = null;

	// 1. build a vector of this object, & all its children
	std::vector<object_t*> victims;
	if (!pShortcutEntity && objectOverrideId != "")
	{
		// NOTE: never call this special objectOverrideId when there's an actual entity to use instead!!
		// NOTE 2: for nodes, we'll probably want to remove child objects too, even if they aren't spawned yet.
		auto masterIt = m_objects.find(objectOverrideId);
		if (masterIt != m_objects.end())
			victims.push_back(masterIt->second);
	}
	else
	{
		auto masterIt = m_objects.begin();
		while (masterIt != m_objects.end())
		{
			if (masterIt->second->entityIndex == iEntityIndex || masterIt->second->parentEntityIndex == iEntityIndex)
			{
				victims.push_back(masterIt->second);

				pTesterInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstanceByEntityIndex(iEntityIndex);
				if (pTesterInstance)
					g_pAnarchyManager->GetSteamBrowserManager()->DestroySteamBrowserInstance(dynamic_cast<C_SteamBrowserInstance*>(pTesterInstance));

				pTesterInstance = g_pAnarchyManager->GetLibretroManager()->FindLibretroInstanceByEntityIndex(iEntityIndex);
				if (pTesterInstance)
					g_pAnarchyManager->GetLibretroManager()->DestroyLibretroInstance(dynamic_cast<C_LibretroInstance*>(pTesterInstance));

				pTesterInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstanceByEntityIndex(iEntityIndex);
				if (pTesterInstance)
					g_pAnarchyManager->GetAwesomiumBrowserManager()->DestroyAwesomiumBrowserInstance(dynamic_cast<C_AwesomiumBrowserInstance*>(pTesterInstance));
			}

			masterIt++;
		}
	}

	// 2. loop through the vector, removing the entities in it.
	KeyValues* pObjectsKV = m_pInstanceKV->FindKey("objects", true);

	C_PropShortcutEntity* pVictimEntity;
	for (unsigned int i = 0; i < victims.size(); i++)
	{
		if (victims[i]->spawned)
		{
			pVictimEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(victims[i]->entityIndex));
			if (pVictimEntity)
			{
				// Entry should be removed from the instance KV also, or at least flag it as "removed" so deleted objects can be undone.
				if (pObjectsKV)
				{
					KeyValues* pEntryKV = pObjectsKV->FindKey(pVictimEntity->GetObjectId().c_str());
					if (pEntryKV)
					{
						pObjectsKV->RemoveSubKey(pEntryKV);

						// save out the instance KV
						if (!bBulkRemove)
							g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", m_pInstanceKV->GetString("info/local/id"), m_pInstanceKV);
					}
				}

				// FIXME: What if the object is still unspawned?  Could this ever happen?  If it could, then it'd need to be removed from the unspawned objects vector too.
				auto it = m_objects.find(pVictimEntity->GetObjectId());
				if (it != m_objects.end())
				{
					g_pAnarchyManager->GetMetaverseManager()->SendObjectRemoved(it->second);
					delete it->second;
					m_objects.erase(it);
				}
			}
		}
	}
	victims.clear();

	if (!bBulkRemove && pShortcutEntity)
		engine->ClientCmd(VarArgs("removeobject %i;\n", iEntityIndex));	// servercmdfix , false);

	// update rich presence objectcount
	steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", this->GetInstanceObjectCount()));
}

void C_InstanceManager::SetNearestSpawnDistFast(float flMaxDist)
{
	m_fNearestSpawnDist = flMaxDist;
	m_pNearestSpawnDistConVar->SetValue(m_fNearestSpawnDist);
}

int C_InstanceManager::SetTempNearestSpawnDist(double maxDist)
{
	m_fNearestSpawnDist = (float)maxDist;

	// FIXME: Below is the exact same code as in SetNearestSpawnDist, almost.
	//Vector playerEyePos = C_BasePlayer::GetLocalPlayer()->EyePosition();

	int count = 0;
	// only do distance calculations if we need to
	if (m_fNearestSpawnDist <= 0)
		count = (int)m_unspawnedObjects.size();
	else
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		Vector playerPos = pPlayer->GetAbsOrigin();

		// figure out how many unspawned objects are within that range
		object_t* pTestObject = null;

		bool bNeedsVisiblityTest = (m_fNearestSpawnDist > 0 && m_pSpawnObjectsWithinViewOnlyConVar->GetBool());
		bool bPassesVisibilityTest;

		unsigned int uNumObjects = m_unspawnedObjects.size();
		float fTestDist;
		unsigned int i;
		for (i = 0; i < uNumObjects; i++)
		{
			pTestObject = m_unspawnedObjects[i];
			fTestDist = pTestObject->origin.DistTo(playerPos);
			if (fTestDist < m_fNearestSpawnDist)
			{
				// Check if the player has a line-of-sight to the player.
				if (!pTestObject->child && bNeedsVisiblityTest)
				{
					//trace_t tr;
					//UTIL_TraceLine(playerEyePos, pTestObject->origin, CONTENTS_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
					//if (tr.fraction >= 0.9)
					//	bPassesVisibilityTest = true;
					//else
					//{
					if (engine->IsBoxInViewCluster(pTestObject->origin + Vector(1, 1, 1), pTestObject->origin + Vector(-1, -1, -1)))
						bPassesVisibilityTest = true;
					else
						bPassesVisibilityTest = false;
					//}
				}

				if (pTestObject->child || !bNeedsVisiblityTest || bPassesVisibilityTest)
				{
					count++;
				}
			}
		}
	}

	m_iUnspawnedWithinRangeEstimate = count;
	return count;
}

int C_InstanceManager::SetNearestSpawnDist(double maxDist)
{
	m_fNearestSpawnDist = (float)maxDist;
	m_pNearestSpawnDistConVar->SetValue(m_fNearestSpawnDist);
	
	//Vector playerEyePos = C_BasePlayer::GetLocalPlayer()->EyePosition();

	int count = 0;
	// only do distance calculations if we need to
	if (m_fNearestSpawnDist <= 0)
		count = (int)m_unspawnedObjects.size();
	else
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		Vector playerPos = pPlayer->GetAbsOrigin();

		// figure out how many unspawned objects are within that range
		object_t* pTestObject = null;

		bool bNeedsVisiblityTest = (m_fNearestSpawnDist > 0 && m_pSpawnObjectsWithinViewOnlyConVar->GetBool());
		bool bPassesVisibilityTest;

		unsigned int uNumObjects = m_unspawnedObjects.size();
		float fTestDist;
		unsigned int i;
		for (i = 0; i < uNumObjects; i++)
		{
			pTestObject = m_unspawnedObjects[i];
			fTestDist = pTestObject->origin.DistTo(playerPos);
			if (fTestDist < m_fNearestSpawnDist)
			{
				// Check if the player has a line-of-sight to the player.
				if (!pTestObject->child && bNeedsVisiblityTest)
				{
					//trace_t tr;
					//UTIL_TraceLine(playerEyePos, pTestObject->origin, CONTENTS_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
					//if (tr.fraction >= 0.9)
					//	bPassesVisibilityTest = true;
					//else
					//{
						if (engine->IsBoxInViewCluster(pTestObject->origin + Vector(1, 1, 1), pTestObject->origin + Vector(-1, -1, -1)))
							bPassesVisibilityTest = true;
						else
							bPassesVisibilityTest = false;
					//}
				}

				if (pTestObject->child || !bNeedsVisiblityTest || bPassesVisibilityTest)
				{
					count++;
				}
			}
		}
	}

	m_iUnspawnedWithinRangeEstimate = count;
	return count;
}

bool C_InstanceManager::SpawnNearestObject()
{
	bool bNeedSpawnOverride;	// If the object is used by a quest OR it is the autoplay object, it should really spawn RIGHT NOW instead.
	object_t* pTestObject = null;

	if (g_pAnarchyManager->ShouldBatchObjectSpawn() && m_fNearestSpawnDist <= 0)	// just spawn everything
	{
		//int iEstimate = g_pAnarchyManager->GetInstanceManager()->GetUnspawnedWithinRangeEstimate();

		std::vector<object_t*>::iterator itt = m_unspawnedObjects.begin();
		while (itt != m_unspawnedObjects.end())
		{
			pTestObject = *itt;

			bNeedSpawnOverride = (this->GetCurrentInstance()->autoplayId == pTestObject->objectId);
			if (!bNeedSpawnOverride)
				bNeedSpawnOverride = g_pAnarchyManager->GetQuestManager()->IsObjectUsedByAnyEZQuests(pTestObject);

			//if (pTestObject->spawned)
				//bNeedSpawnOverride = false;

			this->SpawnObject(pTestObject, bNeedSpawnOverride);//(*itt)
			itt = m_unspawnedObjects.begin();	// this must mean that the object we spawn gets insta-removed from the m_unspawnedObjects during our loop.
			//itt++;
		}

		// IMPORTANT: Doing this here now that objects are multi-spawned.
		// update rich presence objectcount
		steamapicontext->SteamFriends()->SetRichPresence("objectcount", VarArgs("%u", this->GetInstanceObjectCount()));

		// If we are done spawning objects in this batch, restore our spawn dist to the cvar value.
		// (This is because we may ahve overrode the spawn dist by holding down the SPANW OBJECTS button for 1 second.)
		m_fNearestSpawnDist = m_pNearestSpawnDistConVar->GetFloat();

		//if (iEstimate == 0)
		return false;
		//else
		//	return true;
	}

	// Disabled (unless ShouldBatchObjectSpawn is FALSE) on January 9th for the model precache update, as a result of dynamic WebSurfaceProxies now doing nothing in ReleaseStuff - causing their proxy's initialized msg not to be sent again on the next map.
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	Vector playerPos = pPlayer->GetAbsOrigin();
	//Vector playerEyePos = pPlayer->EyePosition();

	bool bNeedsVisiblityTest = (m_fNearestSpawnDist > 0 && m_pSpawnObjectsWithinViewOnlyConVar->GetBool());
	bool bPassesVisibilityTest;

	bool bNearestNeedsSpawnOverride = false;

	object_t* pNearObject = null;
	float fMinDist = -1;
	float fTestDist;
	std::vector<object_t*>::iterator nearestIt;
	std::vector<object_t*>::iterator it = m_unspawnedObjects.begin();
	while (it != m_unspawnedObjects.end())
	{
		pTestObject = *it;
		fTestDist = pTestObject->origin.DistTo(playerPos);

		bNeedSpawnOverride = (this->GetCurrentInstance()->autoplayId == pTestObject->objectId);
		if (!bNeedSpawnOverride)
			bNeedSpawnOverride = g_pAnarchyManager->GetQuestManager()->IsObjectUsedByAnyEZQuests(pTestObject);

		if (pTestObject->spawned)
			bNeedSpawnOverride = false;

		if (bNeedSpawnOverride || ((m_fNearestSpawnDist <= 0 || fTestDist < m_fNearestSpawnDist) && (fMinDist == -1 || fTestDist < fMinDist)))
		{

			// Check if the player has a line-of-sight to the player.
			if (!bNeedSpawnOverride && (!pTestObject->child && bNeedsVisiblityTest))
			{
				//trace_t tr;
				//UTIL_TraceLine(playerEyePos, pTestObject->origin, CONTENTS_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
				//if (tr.fraction >= 0.9)
					//bPassesVisibilityTest = true;
				//else
				//{
					if (engine->IsBoxInViewCluster(pTestObject->origin + Vector(1, 1, 1), pTestObject->origin + Vector(-1, -1, -1)))
						bPassesVisibilityTest = true;
					else
						bPassesVisibilityTest = false;
				//}
			}

			if (bNeedSpawnOverride || pTestObject->child || !bNeedsVisiblityTest || bPassesVisibilityTest)
			{
				bNearestNeedsSpawnOverride = bNeedSpawnOverride;
				fMinDist = fTestDist;
				pNearObject = pTestObject;
				nearestIt = it;
			}
		}

		it++;
	}
	/*
	unsigned int uNumObjects = m_unspawnedObjects.size();
	unsigned int i;
	for (i = 0; i < uNumObjects; i++)
	{
		pTestObject = m_unspawnedObjects[i];
		fTestDist = pTestObject->origin.DistTo(playerPos);
		if (fMinDist == -1 || fTestDist < fMinDist)
		{
			fMinDist = fTestDist;
			pNearObject = pTestObject;
		}
	}
	*/

	if (pNearObject)
	{
		this->SpawnObject(pNearObject, bNearestNeedsSpawnOverride);
		return true;
	}

	// If we are done spawning objects in this batch, restore our spawn dist to the cvar value.
	// (This is because we may ahve overrode the spawn dist by holding down the SPANW OBJECTS button for 1 second.)
	m_fNearestSpawnDist = m_pNearestSpawnDistConVar->GetFloat();

	return false;
}

void C_InstanceManager::AddInstance(int iLegacy, std::string instanceId, std::string mapId, std::string title, std::string file, std::string workshopIds, std::string mountIds, std::string autoplayId, std::string style)
{
	/*
	auto it = m_instances.find(instanceId);
	if (it != m_instances.end())
	{
		DevMsg("WARNING: Instance already exists with id %s, aborting.\n", instanceId.c_str());
		return;
	}
	*/

	//DevMsg("Added instance w/ ID %s\n", instanceId.c_str());

	instance_t* pInstance;
	int index = m_instances.Find(instanceId);
	if (index != m_instances.InvalidIndex())
		pInstance = m_instances.Element(index);
	else
		pInstance = new instance_t();

	//DevMsg("Adding instance for:\n\tID: %s\n\tMapID: %s\n\tTitle: %s\n\tStyle: %s\n\tFile: %s\n\tWorkshopIds: %s\n\tMountIds: %s\n\tLegacy: %i\n", instanceId.c_str(), mapId.c_str(), title.c_str(), style.c_str(), file.c_str(), workshopIds.c_str(), mountIds.c_str(), iLegacy);
	//instance_t* pInstance = new instance_t();
	pInstance->id = instanceId;
	pInstance->mapId = mapId;
	pInstance->title = title;
	pInstance->style = style;
	pInstance->file = file;
	pInstance->workshopIds = workshopIds;
	pInstance->mountIds = mountIds;
	pInstance->autoplayId = autoplayId;
	//pInstance->backpackId = backpackId;
	pInstance->legacy = iLegacy;

	//m_instances[instanceId] = pInstance;
	m_instances.InsertOrReplace(instanceId, pInstance);
}

instance_t* C_InstanceManager::GetCurrentInstance()
{
	int iInstancesMapIndex = m_instances.Find(g_pAnarchyManager->GetInstanceId());
	if (iInstancesMapIndex != m_instances.InvalidIndex())
		return m_instances.Element(iInstancesMapIndex);

	return null;

	// OLD STD::MAP STUFF HERE
	/*
	std::map<std::string, instance_t*>::iterator it = m_instances.find(g_pAnarchyManager->GetInstanceId());
	if (it != m_instances.end())
		return it->second;

	return null;
	*/
}

instance_t* C_InstanceManager::GetInstance(std::string id)
{
	int iInstancesMapIndex = m_instances.Find(id);
	if (iInstancesMapIndex != m_instances.InvalidIndex())
		return m_instances.Element(iInstancesMapIndex);

	return null;
	/*
	std::map<std::string, instance_t*>::iterator it = m_instances.find(id);
	if (it != m_instances.end())
		return it->second;
	
	return null;
	*/
}

instance_t* C_InstanceManager::FindInstance(std::string instanceId)
{
	instance_t* pInstance;
	for (int iInstancesMapIndex = m_instances.FirstInorder(); iInstancesMapIndex != m_instances.InvalidIndex(); iInstancesMapIndex = m_instances.NextInorder(iInstancesMapIndex))
	{
		pInstance = m_instances.Element(iInstancesMapIndex);
		if (!Q_stricmp(pInstance->id.c_str(), instanceId.c_str()))
			return pInstance;
	}

	return null;
}

void C_InstanceManager::FindAllInstances(std::string mapId, std::vector<instance_t*> &instances)
{
	instance_t* pInstance;
	for (int iInstancesMapIndex = m_instances.FirstInorder(); iInstancesMapIndex != m_instances.InvalidIndex(); iInstancesMapIndex = m_instances.NextInorder(iInstancesMapIndex))
	{
		pInstance = m_instances.Element(iInstancesMapIndex);
		if (pInstance->mapId == mapId)
			instances.push_back(pInstance);
	}
	/*
	std::map<std::string, instance_t*>::iterator it = m_instances.begin();
	while (it != m_instances.end())
	{
		//DevMsg("Map iiiiiiiiiid: %s\n", it->second->mapId.c_str());
		//if (it->second->mapId == mapId)
		//if (!Q_stricmp(it->second->mapId.c_str(), mapId.c_str()))
		if (it->second->mapId == mapId)
			instances.push_back(it->second);

		it++; 
	}
	*/
}

void C_InstanceManager::LegacyMapIdFix(std::string legacyMapName, std::string mapId)
{
	instance_t* pInstance;
	for (int iInstancesMapIndex = m_instances.FirstInorder(); iInstancesMapIndex != m_instances.InvalidIndex(); iInstancesMapIndex = m_instances.NextInorder(iInstancesMapIndex))
	{
		pInstance = m_instances.Element(iInstancesMapIndex);

		if (!Q_stricmp(pInstance->mapId.c_str(), legacyMapName.c_str()))
		{
			//DevMsg("LegacyMapIdFixing: %s to %s\n", pInstance->mapId.c_str(), mapId.c_str());
			pInstance->mapId = mapId;
		}
	}

	/*
	std::map<std::string, instance_t*>::iterator it = m_instances.begin();
	while (it != m_instances.end())
	{
		//if (it->second->mapId == legacyMapName)
		if (!Q_stricmp(it->second->mapId.c_str(), legacyMapName.c_str()))
		{
			DevMsg("LegacyMapIdFixing: %s to %s\n", it->second->mapId.c_str(), mapId.c_str());
			it->second->mapId = mapId;
		}

		it++;
	}
	*/
}

// TODO: This will replace LoadLegacyInstance, because legacy instances will be consumed before loaded.
bool C_InstanceManager::ConsumeLegacyInstance(std::string instanceId, std::string filename, std::string path, std::string searchPath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack)
{
	//DevMsg("Consuming legacy instance from %s%s\n", path.c_str(), filename.c_str());

	/* These fields must be determined
	pInstance->id = instanceId; (already given)
	pInstance->workshopIds = workshopIds; (already given)
	pInstance->mountIds = mountIds; (already given)
	TODO: pInstance->backpackId = mountIds; (should be already given)
	pInstance->file = file;
	pInstance->title = title;
	pInstance->style = style;
	pInstance->mapId = mapId;	(but uses legacyMapId instead, and blank if just a node.)
	*/

	// file
	std::string file = path + filename;

	// Load up the SET file, it has useful info.
	KeyValues* pLegacyInstanceKV = new KeyValues("instance");
	if (!pLegacyInstanceKV->LoadFromFile(g_pFullFileSystem, file.c_str(), searchPath.c_str()))
	{
		DevMsg("ERROR: Failed to load legacy instance for consumption: %s\n", file.c_str());
		return false;
	}

	// title
	std::string title = pLegacyInstanceKV->GetString("title");	// gen2 nodes (there were no gen1 nodes) have titles in their SET file
	if (title == "")
		title = filename;

	// style
	bool bIsNode = false;
	std::string style = pLegacyInstanceKV->GetString("map");
	if (style.find("node_") == 0)
		bIsNode = true;
	else
		style = "";

	// mapId
	std::string legacyMapId;
	std::string goodMapName;
	if (!bIsNode)
	{
		goodMapName = filename.substr(0, filename.find("."));
		legacyMapId = goodMapName;	// map ID's can only be found after all maps have been detected.  So use a legacyMapId right now, which gets fixed when Redux detects all BSP files.
	}

	// PRAISE TOM CRUISE!  There exists a function that gets called later on that fixes all the instance ID's of shit, outside of the KeyValues.
	// The function gets called as part of "detecting all maps", which is scanning for BSP files, then adjusts the ID's of any instances that just blantantly reference the map file name (no extension).
//	this->LegacyMapIdFix()

	// Build a GEN3 instance KeyValues structure here and save it out
	KeyValues* pInstanceKV = new KeyValues("instance");

	// TODO:
	//	1. Make THESE legacy functions the STANDARD for EXPORTING legacy stuff.
	//	2. Improve/refactor the LoadLegacyInstance function (where ever it may be) to resolve these kinds of things when it loads, and it only needs to work with things w/ legacy = 1
	//	3. Remember that mapId gets updated upon map detection.

	// Note: We CANNOT assume this is a legacy GEN2 workshop item in the future.  Each GEN of workshop addon will likely need its own constumption method actually.
	g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(1, pInstanceKV, instanceId, legacyMapId, title, file, workshopIds, mountIds, style);

	KeyValues* pObjectsKV = pInstanceKV->FindKey("objects", true);
	KeyValues* pLegacyObjectsKV = pLegacyInstanceKV->FindKey("objects", true);
	
	std::string legacyItemId;
	std::string legacyModelId;
	std::string objectId;
	std::string position;
	std::string rotation;
	float scale;
	bool slave;
	bool child;
	KeyValues* pObjectKV;
	for (KeyValues *pLegacyObjectKV = pLegacyObjectsKV->GetFirstSubKey(); pLegacyObjectKV; pLegacyObjectKV = pLegacyObjectKV->GetNextKey())
	{
		legacyItemId = pLegacyObjectKV->GetString("itemfile");

		/* THIS SHOULD BE DONE UPON LOADING THE INSTANCE NOW, when legacy = 1 is detected in its KV!
		itemId = g_pAnarchyManager->ExtractLegacyId(pLegacyObjectKV->GetString("itemfile"));
		if (itemId == "")
		{
			// don't generate item id's for legacy prop objects
			testBuf = pLegacyObjectKV->GetString("itemfile");
			size_t foundExt = testBuf.find(".mdl");
			if (foundExt != testBuf.length() - 4)
			{
				KeyValues* pSearchInfo = new KeyValues("search");
				pSearchInfo->SetString("file", pInstanceKV->GetString("itemfile"));

				KeyValues* foundActive = null;
				KeyValues* foundItem = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryItem(pSearchInfo));
				if (foundItem)
					itemId = foundActive->GetString("info/id");
				else
				{
					DevMsg("Instance failed to resolve item with itemfile: %s\n", pInstanceKV->GetString("itemfile"));
					itemId = g_pAnarchyManager->GenerateLegacyHash(pInstanceKV->GetString("itemfile"));
				}
			}
		}
		*/

		legacyModelId = pLegacyObjectKV->GetString("model");

		/* This should be handled just like itemId, and resolved with nearly the same logic
		modelId = g_pAnarchyManager->GenerateLegacyHash(pLegacyObjectKV->GetString("model"));
		*/

		scale = pLegacyObjectKV->GetFloat("scale", 1.0f);
		slave = (pLegacyObjectKV->GetInt("slave") > 0);
		child = pLegacyObjectKV->GetBool("isChild");

		objectId = g_pAnarchyManager->GenerateUniqueId();

		position = pLegacyObjectKV->GetString("origin", "0 0 0");
		rotation = pLegacyObjectKV->GetString("angles", "0 0 0");

		// create an object in the KV
		KeyValues* objectKV = pObjectsKV->FindKey(objectId.c_str(), true);
		g_pAnarchyManager->GetInstanceManager()->CreateObject(objectKV, objectId, legacyItemId, legacyModelId, position, rotation, scale, "", slave, child, 0, 0);
	}

	// FIXME: This really should NOT be saved into the local library so early!!
	// Saving the workshop instance should not happen until the user makes a change to it, however, until
	// GEN2 workshop addons can be CONSUMED properly, we'll continue to instantly save workshop instances out
	// to the user library to avoid any weird systems that will become obsolete once consuming GEN2 workshop items properly is finished.
	// The only side-effect to doing it this way is that instances get saved to the user's library IMMEDIATELY upon consuming a workshop instance,
	// instead of not until the user makes changes to them, which is how it will be eventually.

	sqlite3** pDb = (pBackpack && pBackpack->GetSQLDbPointer()) ? pBackpack->GetSQLDbPointer() : null;
	g_pAnarchyManager->GetMetaverseManager()->SaveSQL(pDb, "instances", instanceId.c_str(), pInstanceKV);

	char fullFilePath[AA_MAX_STRING];
	PathTypeQuery_t pathTypeQuery;
	g_pFullFileSystem->RelativePathToFullPath(file.c_str(), searchPath.c_str(), fullFilePath, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);
	std::string fullpath = fullFilePath;

	g_pAnarchyManager->GetInstanceManager()->AddInstance(true, instanceId, legacyMapId, title, fullpath, workshopIds, mountIds, "", style);
	pLegacyInstanceKV->deleteThis();
	pLegacyInstanceKV = null;
	pInstanceKV->deleteThis();
	pInstanceKV = null;
}

void C_InstanceManager::ClearNodeSpace()
{
	DevMsg("And here, we gotta clear that node space buddy guy pal!\n");

	KeyValues* pNodeInfoKV = new KeyValues("node");
	if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Could not load nodevolume.txt!\n");
		return;
	}

	// Find the info shortcut for the node.
	KeyValues* pInstanceKV = null;
	C_PropShortcutEntity* pInfoShortcut = null;
	C_BaseEntity* pBaseEntity;
	C_PropShortcutEntity* pPropShortcutEntity;
	for (KeyValues *sub = pNodeInfoKV->FindKey("setup/objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pPropShortcutEntity->GetItemId()));
		if (pItemKV)
		{
			KeyValues* pTypeKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryType(pItemKV->GetString("type")));
			if (pTypeKV)
			{
				if (!Q_strcmp(pTypeKV->GetString("info/id"), g_pAnarchyManager->GetMetaverseManager()->GetSpecialTypeId("node").c_str()))
				{
					pInfoShortcut = pPropShortcutEntity;
					break;
				}
			}
		}
	}

	KeyValues* pNodeInfoSetupKV = pNodeInfoKV->FindKey("setup", true);

	KeyValues* pNodeObjectsKV = pNodeInfoSetupKV->FindKey("objects", true);
	for (KeyValues *sub = pNodeObjectsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// loop through it adding all the info to the response object.
		pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
		if (!pBaseEntity)
			continue;

		pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pPropShortcutEntity)
			continue;

		if (pInfoShortcut && (pPropShortcutEntity == pInfoShortcut || pPropShortcutEntity->GetMoveParent() == pInfoShortcut->GetBaseEntity()))
			continue;

		this->RemoveEntity(pPropShortcutEntity, true);
	}

	if (pInfoShortcut)
		this->RemoveEntity(pInfoShortcut, true);

	// finished bulk removing...
	int iInfoShortcutIndex = (pInfoShortcut) ? pInfoShortcut->entindex() : -1;
	this->SaveActiveInstance();
	engine->ClientCmd(VarArgs("bulkremovedobjects %i;\n", iInfoShortcutIndex));	// servercmdfix , false);
}

object_t* C_InstanceManager::GetObjectUnderPlayerAim()
{
	C_BaseEntity* pEntity = null;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->GetHealth() > 0)
	{
		/*
		// fire a trace line
		trace_t tr;
		g_pAnarchyManager->SelectorTraceLine(tr);
		//Vector forward;
		//pPlayer->EyeVectors(&forward);
		//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
			pEntity = tr.m_pEnt;
			*/

		pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());

		// only allow prop shortcuts
		if (pEntity)
		{
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
			if (pShortcut)
				return this->GetInstanceObject(pShortcut->GetObjectId());
		}
	}

	return null;
}

void C_InstanceManager::AssignObjectItem(std::string objectId, std::string itemId)
{
	bool bDidReplace = false;
	C_PropShortcutEntity* pShortcut = null;
	object_t* pObject = this->GetInstanceObject(objectId);
	KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId));
	if (pObject && pItemKV)
	{
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
		if (pShortcut)
		{
			pObject->itemId = itemId;

			std::string modelFile;
			KeyValues* entryModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pObject->modelId));
			if (entryModel)
				modelFile = entryModel->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID), "models\\cabinets\\two_player_arcade.mdl");	// uses default model if key value read fails
			else
				modelFile = "models\\cabinets\\two_player_arcade.mdl";

			engine->ClientCmd(VarArgs("setobjectids %i \"%s\" \"%s\" \"%s\";\n", pShortcut->entindex(), pObject->itemId.c_str(), pShortcut->GetModelId().c_str(), modelFile.c_str()));	// servercmdfix , false);
			bDidReplace = true;
			g_pAnarchyManager->AddToastMessage("Replaced Object's Item");
		}
	}

	//if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
	//	g_pAnarchyManager->DeactivateObjectPlacementMode(false);

	if (bDidReplace)// && pShortcut && pObject
		this->ApplyChanges(pShortcut, true, "", pObject->itemId);
		//this->SaveActiveInstance();
}

/*
//-----------------------------------------------------------------------------
// Creates euler angles from a matrix 
//-----------------------------------------------------------------------------
void AAMatrixToAngles(const VMatrix& src, QAngle& vAngles)
{
	float forward[3];
	float left[3];
	float up[3];

	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	forward[0] = src[0][0];
	forward[1] = src[1][0];
	forward[2] = src[2][0];
	left[0] = src[0][1];
	left[1] = src[1][1];
	left[2] = src[2][1];
	up[2] = src[2][2];

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		vAngles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// redone.	// Added for Anarchy Arcade
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		vAngles[0] = -RAD2DEG(atan2f(-forward[2], xyDist));

		// (roll)	z = ATAN( left.z, up.z );
		vAngles[2] = RAD2DEG(atan2f(left[2], up[2]));
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		vAngles[1] = RAD2DEG(atan2f(-left[0], left[1]));

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// redone.	// Added for Anarchy Arcade
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		vAngles[0] = -RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		vAngles[2] = 0;
	}
}
*/

void C_InstanceManager::CreateNewNode(std::string nodeName, C_PropShortcutEntity* pNodeEntity, std::string nodeItemId)
{
	KeyValues* pNodeInfoKV = new KeyValues("node");
	if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
	{
		DevMsg("ERROR: Could not load nodevolume.txt!\n");
		return;
	}

	// if we are creating a node from a pNodeEntity, then set the player's camera to the right place & take the screenshot for the node.
	// screenshots should save to screenshots/nodes/[nodeId].jpg.
	// some legacy nodes saved to library/nodes/screens/[hash_of_nodeId].tbn

	KeyValues* pNodeItemKV = (pNodeEntity) ? g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", nodeItemId)) : null;

	// Find the info shortcut for the node.
	KeyValues* pNodeInfoSetupKV = pNodeInfoKV->FindKey("setup");

	std::string nodeTypeId = g_pAnarchyManager->GetMetaverseManager()->GetSpecialTypeId("node");
	std::string nodeModelId = g_pAnarchyManager->GetMetaverseManager()->GetSpecialModelId("node");// "models/cabinets/node.mdl";

	std::string itemId = (!pNodeItemKV) ? g_pAnarchyManager->GenerateUniqueId() : nodeItemId;
	std::string nodeId = (!pNodeItemKV) ? g_pAnarchyManager->GenerateUniqueId() : pNodeItemKV->GetString("file");

	std::string title = nodeName;
	std::string description = "";
	std::string file = nodeId;
	std::string type = nodeTypeId;
	std::string app = "";
	std::string reference = "";
	std::string preview = "";
	std::string download = "";
	std::string stream = "";
	std::string screen = "";
	std::string marquee = "";
	std::string model = nodeModelId;

	KeyValues* pItemKV = (!pNodeItemKV) ? new KeyValues("item") : pNodeItemKV;
	if(!pNodeItemKV)
		g_pAnarchyManager->GetMetaverseManager()->CreateItem(0, itemId, pItemKV, title, description, file, type, app, reference, preview, download, stream, screen, marquee, model, "");

	if (pItemKV)
	{
		if (!pNodeItemKV)
		{
			// push this onto the active library & save it
			g_pAnarchyManager->GetMetaverseManager()->AddItem(pItemKV);
			//g_pAnarchyManager->GetMetaverseManager()->SaveItem(pItemKV);
			g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "items", itemId.c_str(), pItemKV);

			// Remember this itemID so that the item can identify itself while its spawning as a "currently building" node item.
			m_incomingNodeId = itemId;

			// Add this object to the instance
			Vector origin;
			UTIL_StringToVector(origin.Base(), pNodeInfoSetupKV->GetString("origin", "0 0 0"));

			QAngle angles;
			UTIL_StringToVector(angles.Base(), pNodeInfoSetupKV->GetString("angles", "0 0 0"));

			//std::string modelId = g_pAnarchyManager->GenerateLegacyHash("models/cabinets/two_player_arcade.mdl");
			object_t* pObject = this->AddObject("", itemId, nodeModelId, origin, angles, 1.0f, "", false, 0, 0);
			this->ApplyChanges(null, true, pObject->objectId, itemId);
			//this->SaveActiveInstance();
			this->SpawnObject(pObject, false);
			//this->SpawnNearestObject();
			//object_t* pObject = this->AddObject("", itemId, modelId, origin, angles, 1.0f, false, 0, 0, 0, "", 0, "", true);
			//g_pAnarchyManager->GetMetaverseManager()->SetSpawningObject(pObject);
			//this->SpawnObject(pObject);

			// Spawn the item. At THAT point, the node will parent all children to itself (instead of spawning its children) and CREATE the node instance (instead of LOADING the node instance).
			// TODO: work this->ApplyChanges([the_entity]);

			// we are finished from here.  the rest of the logic happens as the node item spawns itself in.
		}
		else
		{
			// we are UPDATING an EXISTING node, so we have to immediately do work.

			// create the actual node instance
			KeyValues* pNodeInstanceKV = new KeyValues("instance");
			std::string nodeStyle = "node_" + std::string(pNodeInfoKV->GetString("setup/style"));
			std::string nodeInstanceId = g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(0, pNodeInstanceKV, pItemKV->GetString("file"), "", pItemKV->GetString("title"), "", "", "", nodeStyle, null);

			object_t* pInstanceObject;

			VMatrix childMatrix;
			VMatrix parentMatrix;
			parentMatrix.SetupMatrixOrgAngles(pNodeEntity->GetAbsOrigin(), pNodeEntity->GetAbsAngles());
			VMatrix parentMatrixInverse = parentMatrix.InverseTR();

			//Vector dummy;
			Vector childOrigin;
			Vector childAnglesRaw;
			QAngle childAngles;

			KeyValues* pObjectKV;
			C_BaseEntity* pBaseEntity;
			C_PropShortcutEntity* pPropShortcutEntity;
			//object_t* pObject;
			KeyValues* pObjectInfo;
			KeyValues* pItemInfo;
			KeyValues* pModelInfo;
			KeyValues* pNodeObjectsKV = pNodeInfoKV->FindKey("setup/objects", true);

			VMatrix composedMatrix;
			float flParentScale = pNodeEntity->GetModelScale();

			int arrayIndex = 0;
			bool bFoundDuplicate;
			for (KeyValues *sub = pNodeObjectsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				// loop through it adding all to the instance KV
				pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
				if (!pBaseEntity)
					continue;

				pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
				if (!pPropShortcutEntity)
					continue;

				pInstanceObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pPropShortcutEntity->GetObjectId());

				childOrigin = pInstanceObject->origin;// pPropShortcutEntity->GetAbsOrigin();
				childAngles = TransformAnglesToLocalSpace(pInstanceObject->angles, parentMatrix.As3x4()); //pInstanceObject->angles;// pPropShortcutEntity->GetAbsAngles();				

				childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
				childMatrix = childMatrix * parentMatrixInverse;

				// Finally convert back to origin+angles.
				childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
				//MatrixToAngles(childMatrix.As3x4(), childAngles);



				/*
				// At this point, childOrigin and childAngles should hold valid values.
				// However, there is an orientation bug that causes certain objects to be incorrectly oriented afer being restored.
				// This bug is reversable - as saving the node again in this errored state corrects the errors.
				// So the code below simulates doing the math twice, in order to avoid ever saving the error.

				// Reapply to induce orientation errors.
				childMatrix.SetupMatrixOrgAngles(childOrigin * flParentScale, childAngles);
				composedMatrix.SetupMatrixOrgAngles(pNodeEntity->GetAbsOrigin(), pNodeEntity->GetAbsAngles());
				composedMatrix = composedMatrix * childMatrix;
				MatrixAngles(composedMatrix.As3x4(), childAngles, childOrigin);

				// And reapply a 3rd time to correct those orientation errors.
				childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
				childMatrix = childMatrix * parentMatrixInverse;
				childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
				MatrixToAngles(childMatrix.As3x4(), childAngles);
				*/



				char buf1[AA_MAX_STRING];
				char buf2[AA_MAX_STRING];

				// position
				Vector localPosition = childOrigin;// pPropShortcutEntity->GetLocalOrigin();
				Q_snprintf(buf1, sizeof(buf1), "%.10f %.10f %.10f", localPosition.x, localPosition.y, localPosition.z);
				std::string position = buf1;

				// rotation
				QAngle localAngles = childAngles;// pPropShortcutEntity->GetLocalAngles();
				Q_snprintf(buf2, sizeof(buf2), "%.10f %.10f %.10f", localAngles.x, localAngles.y, localAngles.z);
				std::string rotation = buf2;

				// is this a node itself?  prevent node-ception.
				KeyValues* pSubItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", pPropShortcutEntity->GetItemId()));
				if (pSubItemKV)
				{
					KeyValues* pSubTypeKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("types", pSubItemKV->GetString("type")));
					if (pSubTypeKV && !Q_stricmp(pSubTypeKV->GetString("title"), "node"))
						continue;
				}

				// now make sure that there's no other objects at this exact same XYZ to fix the case where multiple things save.
				bFoundDuplicate = false;
				for (KeyValues *testSub = pNodeInstanceKV->FindKey("objects", true)->GetFirstSubKey(); testSub; testSub = testSub->GetNextKey())
				{
					if (!Q_strcmp(testSub->GetString("origin"), buf1) && !Q_strcmp(testSub->GetString("angles"), buf2))
					{
						DevMsg("Node Duplicate Object Skipped\n");
						bFoundDuplicate = true;
						break;
					}
				}
				if (bFoundDuplicate)
					continue;


				//if (pPropShortcutEntity->GetMoveParent() != pPropShortcutEntity)
				//{
					// flag it as a child object in the active instance too, & save the instance.  Cuz child objects do NOT spawn automatically, their nodes control their spawning.
					//pInstanceObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pPropShortcutEntity->GetObjectId());
					//pObjectKV = g_pAnarchyManager->GetInstanceManager()->GetCurrentInstanceKV()->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()));

					std::string sequenceName = pInstanceObject->anim;//pObject->anim //pPropShortcutEntity->GetSequenceName(pPropShortcutEntity->GetSequence())
					//if (pObjectKV)
					//{
						KeyValues* pNodeObjectKV = pNodeInstanceKV->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()), true);

						// position
						//Vector localPosition = childOrigin;// pPropShortcutEntity->GetLocalOrigin();
						//Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", localPosition.x, localPosition.y, localPosition.z);
						//std::string position = buf;

						// rotation
						//QAngle localAngles = childAngles;// pPropShortcutEntity->GetLocalAngles();
						//Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", localAngles.x, localAngles.y, localAngles.z);
						//std::string rotation = buf;

						float scale = pPropShortcutEntity->GetModelScale();
						int slave = (pInstanceObject->slave) ? 1 : 0;
						int child = 1;	// Objects inside of nodes themselves cannot have children.

						pObjectKV = pNodeInstanceKV->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()), true);
						g_pAnarchyManager->GetInstanceManager()->CreateObject(pObjectKV, pPropShortcutEntity->GetObjectId(), pPropShortcutEntity->GetItemId(), pPropShortcutEntity->GetModelId(), position, rotation, scale, sequenceName, slave, child, pInstanceObject->body, pInstanceObject->skin);





						/*std::string childOriginString = VarArgs("%f %f %f", childOrigin.x, childOrigin.y, childOrigin.z);
						std::string childAnglesString = VarArgs("%f %f %f", childAngles.x, childAngles.y, childAngles.z);
						pNodeObjectKV->SetString("local/origin", childOriginString.c_str());
						pNodeObjectKV->SetString("local/angles", childAnglesString.c_str());
						pNodeObjectKV->SetString("local/anim", sequenceName.c_str());
						pNodeObjectKV->SetFloat("local/scale", pPropShortcutEntity->GetModelScale());

						pNodeObjectKV->SetInt("local/child", 1);*/
					//}

						if (pPropShortcutEntity->GetMoveParent() != pNodeEntity)
						{
							pInstanceObject->parentEntityIndex = pNodeEntity->entindex();
							pInstanceObject->child = true;
							engine->ClientCmd(VarArgs("setparent %i %i\n", pPropShortcutEntity->entindex(), pNodeEntity->entindex()));
						}
				//}
			}

			//sqlite3** pDb = (pBackpack && pBackpack->GetSQLDbPointer()) ? pBackpack->GetSQLDbPointer() : null;
			sqlite3** pDb = null;
			//DevMsg("Instance ID: %s vs %s\n", nodeInstanceId.c_str(), pNodeInstanceKV->GetString("info/local/id"));
			g_pAnarchyManager->GetMetaverseManager()->SaveSQL(pDb, "instances", nodeInstanceId.c_str(), pNodeInstanceKV, true);
			pNodeInstanceKV->deleteThis();
			pNodeInstanceKV = null;

			g_pAnarchyManager->GetInstanceManager()->SaveActiveInstance(); // to save our flagging of objects as child
		}
	}
}

//#include "../../../public/engine/ivdebugoverlay.h"
#include "../../../game/shared/debugoverlay_shared.h"
void C_InstanceManager::SpawnActionPressed(bool bForceSpawnAll)
{
	if (!g_pAnarchyManager->IsInitialized())
		return;

	/* WORKING SPAWNING CODE
	std::string loadingScreenshotId = g_pAnarchyManager->GetMetaverseManager()->GetLoadingScreenshotId();
	//if ( this->GetInstanceObjectCount() > 0)
	//{
	std::string screenshotPostfix = (loadingScreenshotId != "") ? "&screenshot=" + loadingScreenshotId : "";
	std::string uri = "asset://ui/spawnItems.html?max=" + std::string(VarArgs("%f", m_fNearestSpawnDist)) + screenshotPostfix;//"99999999999.9"

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pHudBrowserInstance);
	pHudBrowserInstance->SetUrl(uri);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
	return;
	*/

	// end of shit





	// re-enable the double-tap spawn method.

	float fMinDist = (m_fNearestSpawnDist <= 0 ) ? -1 : m_fNearestSpawnDist;// 420.0;
	float fDuration = 4.0f;
	//DevMsg("Time is: %f\n", gpGlobals->realtime - m_fLastSpawnActionPressed);

	// determine if there is anything to spawn
	//Vector testPlayerPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	Vector playerPos = pPlayer->GetAbsOrigin();
	Vector playerEyePos = pPlayer->EyePosition();

	bool bNeedsVisiblityTest = (m_fNearestSpawnDist > 0 && m_pSpawnObjectsWithinViewOnlyConVar->GetBool());
	bool bPassesVisibilityTest;
	//if (true)	// turn off double-tap loading if unspawned item title previews are on while walking around
	if (bForceSpawnAll || m_fNearestSpawnDist <= 0 || !m_pSpawnObjectsDoubleTapConVar->GetBool() || (m_fLastSpawnActionPressed != 0 && gpGlobals->realtime - m_fLastSpawnActionPressed < fDuration))
	{
		m_fLastSpawnActionPressed = 0.0f;// gpGlobals->realtime;

		// Only spawn stuff if there is something to spawn.
		if (m_unspawnedObjects.empty())
			return;

		bool bHasOne = false;
		object_t* pTestTestObject = null;
		float fTestTestDist;
		std::vector<object_t*>::iterator testIt = m_unspawnedObjects.begin();
		trace_t tr;
		while (testIt != m_unspawnedObjects.end())
		{
			pTestTestObject = *testIt;
			fTestTestDist = pTestTestObject->origin.DistTo(playerPos);
			if (bForceSpawnAll || pTestTestObject->child || fMinDist == -1 || fTestTestDist < fMinDist)
			{
				// Check if the player has a line-of-sight to the player.
				if (!bForceSpawnAll && !pTestTestObject->child && bNeedsVisiblityTest)
				{
					//UTIL_TraceLine(playerEyePos, pTestTestObject->origin, CONTENTS_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
					//if (tr.fraction >= 0.9)
					//	bPassesVisibilityTest = true;
					//else
					//{
						if (engine->IsBoxInViewCluster(pTestTestObject->origin + Vector(1, 1, 1), pTestTestObject->origin + Vector(-1, -1, -1)))
							bPassesVisibilityTest = true;
						else
							bPassesVisibilityTest = false;
					//}
				}

				if (bForceSpawnAll || pTestTestObject->child || !bNeedsVisiblityTest || bPassesVisibilityTest)
				{
					bHasOne = true;
					break;
				}
			}

			testIt++;
		}

		if (!bHasOne)
			return;
		
		std::string loadingScreenshotId = g_pAnarchyManager->GetMetaverseManager()->GetLoadingScreenshotId();
		//if ( this->GetInstanceObjectCount() > 0)
		//{
		std::string screenshotPostfix = (loadingScreenshotId != "") ? "&screenshot=" + loadingScreenshotId : "";
		std::string maxDist = (bForceSpawnAll) ? "0" : VarArgs("%f", m_fNearestSpawnDist);
		std::string uri = "asset://ui/spawnItems.html?max=" + maxDist + screenshotPostfix;//"99999999999.9"

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pHudBrowserInstance);
		pHudBrowserInstance->SetUrl(uri);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
	}
	else
	{
		if (m_unspawnedObjects.empty())
			return;

		m_fLastSpawnActionPressed = gpGlobals->realtime;

		int iNumItemsFound = 0;
		int iNumModelsFound = 0;

		//object_t* pNearObject = null;
		object_t* pTestObject = null;
		float fTestDist;
		//std::vector<object_t*>::iterator nearestIt;
		std::vector<object_t*>::iterator it = m_unspawnedObjects.begin();
		trace_t tr;
		KeyValues* active;
		KeyValues* item;
		KeyValues* model;
		while (it != m_unspawnedObjects.end())
		{
			pTestObject = *it;
			fTestDist = pTestObject->origin.DistTo(playerPos);
			//if (fTestDist < m_fNearestSpawnDist && (fMinDist == -1 || fTestDist < fMinDist))
			if (fMinDist == -1 || fTestDist < fMinDist)
			{
				// Check if the player has a line-of-sight to the player.
				if (!pTestObject->child && bNeedsVisiblityTest)
				{
					//UTIL_TraceLine(playerEyePos, pTestObject->origin, CONTENTS_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
					//if (tr.fraction >= 0.9)
					//	bPassesVisibilityTest = true;
					//else
					//{
						if (engine->IsBoxInViewCluster(pTestObject->origin + Vector(1, 1, 1), pTestObject->origin + Vector(-1, -1, -1)))
							bPassesVisibilityTest = true;
						else
							bPassesVisibilityTest = false;
					//}
				}

				if (pTestObject->child || !bNeedsVisiblityTest || bPassesVisibilityTest)
				{
					item = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pTestObject->itemId.c_str());
					if (item)
					{
						iNumItemsFound++;
						active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(item);
						NDebugOverlay::Text(pTestObject->origin, active->GetString("title"), false, fDuration);
						if (m_pReShadeConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
							cvar->FindVar("r_drawvgui")->SetValue(1);
					}
					else
					{
						model = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pTestObject->modelId.c_str());
						if (model)
						{
							iNumModelsFound++;
							active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(model);
							NDebugOverlay::Text(pTestObject->origin, active->GetString("title"), false, fDuration);
							if (m_pReShadeConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
								cvar->FindVar("r_drawvgui")->SetValue(1);
						}
					}
				}
			}

			it++;
		}

		if (iNumItemsFound <= 0 && iNumModelsFound <= 0)
			m_fLastSpawnActionPressed = 0.0f;
	}
}

void C_InstanceManager::ChangeModel(C_BaseEntity* pEntity, std::string modelId, std::string in_modelFile, bool bMakeGhost, bool bReinit)
{
	std::string modelFile = in_modelFile;
	//DevMsg("Yadda: %s\n", modelFile.c_str());

	if (!g_pFullFileSystem->FileExists(modelFile.c_str()))
		modelFile = "models/icons/missing.mdl";

	//DevMsg("Here the model id is: %s\n", modelId.c_str());

	m_pRecentModelIdConVar->SetValue(modelId.c_str());

	if (!g_pFullFileSystem->FileExists(modelFile.c_str(), "GAME"))
		modelFile = "models/icons/missing.mdl";

	//int iMakeGhost = (bMakeGhost) ? 1 : 0;
	engine->ClientCmd(VarArgs("switchmodel \"%s\" \"%s\" %i %i %i;\n", modelId.c_str(), modelFile.c_str(), pEntity->entindex(), bMakeGhost, bReinit));	// servercmdfix
}

bool C_InstanceManager::ModelAssetReadyFirstMorph(std::string fileHash, std::string file, bool& bSkippedTarget)
{
	bool bDidSkipTarget = false;
	if (engine->IsInGame())
	{
		C_BaseEntity* pEntity;

		// Find any object that uses this model
		auto it = m_objects.begin();
		while (it != m_objects.end())
		{
			if (it->second->spawned && it->second->modelId == fileHash)
			{
				pEntity = C_BaseEntity::Instance(it->second->entityIndex);
				if (!pEntity )//|| !pEntity->IsVisible())
				{
					//DevMsg("ERROR: Entity does not exist!\n");
					bDidSkipTarget = true;
					it++;
					continue;
				}

				const model_t* TheModel = pEntity->GetModel();
				if (TheModel)
				{
					std::string modelName = modelinfo->GetModelName(TheModel);
					if (modelName == "models/icons/missing.mdl")
					{
						pEntity->SetRenderMode(kRenderNormal, true);
						this->ChangeModel(pEntity, fileHash, file, false, true);
						/*
						if (it->second->anim != "")
						{
						C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
						if ( pShortcut )
						pShortcut->PlaySequenceRegular(it->second->anim.c_str(), true);
						}*/

						bSkippedTarget = bDidSkipTarget;
						//bSkippedTarget = false;
						return true;
					}
				}
			}
			it++;
		}
	}

	bSkippedTarget = bDidSkipTarget;
	return false;
}

void C_InstanceManager::OnModelAssetReady(std::string fileHash, std::string file)
{
	/*
	if (engine->IsInGame())
	{
		C_BaseEntity* pEntity;
		unsigned int count = 0;

		// Find any object that uses this model
		auto it = m_objects.begin();
		while (it != m_objects.end())
		{
			if (it->second->spawned && it->second->modelId == fileHash)
			{
				pEntity = C_BaseEntity::Instance(it->second->entityIndex);
				pEntity->SetRenderMode(kRenderNormal, true);
				this->ChangeModel(pEntity, fileHash, file, false);

				//if (it->second->anim != "")
				//{
				//	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				//	if ( pShortcut )
				//		pShortcut->PlaySequenceRegular(it->second->anim.c_str(), true);
				//}

				count++;
			}
			it++;
		}

		// FIXME: Is the model in question used on the current spawning object perhaps?
		// TODO: Handle it.
	}
	*/
}

void C_InstanceManager::RemoveInstance(instance_t* pInstance)
{
	int it = m_instances.Find(pInstance->id);
	if (it != m_instances.InvalidIndex())
		m_instances.RemoveAt(it);

	delete pInstance;
}

void C_InstanceManager::SelectNext(C_PropShortcutEntity* pSelectedEntity, C_PropShortcutEntity* pOriginalEntity)
{
	//g_pAnarchyManager->ClearAttractMode();
	if (cvar->FindVar("attract_mode_active")->GetBool() || cvar->FindVar("camcut_attract_mode_active")->GetBool())
		return;

	//* pOriginalObject = m_pInstanceManager->GetObjectEntity(pOriginalEntity->GetObjectId());
	//auto originalIt = m_objects.find(pOriginalEntity->GetObjectId());
	//if (originalIt == m_objects.end())
	//	return;

	//	g_pAnarchyManager->SetSelectOriginal(this->entindex());

	object_t* pSelectedObject = (pSelectedEntity) ? m_objects[pSelectedEntity->GetObjectId()] : null;
	object_t* pOriginalObject = (pOriginalEntity) ? m_objects[pOriginalEntity->GetObjectId()] : null;

	Vector originalPos;
	if (pOriginalEntity)
		originalPos = pOriginalEntity->GetAbsOrigin();
	else
		originalPos = MainViewOrigin();
		//originalPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		//originalPos.Init();

	float flCutoff = (pSelectedObject && pOriginalEntity) ? pSelectedObject->origin.DistTo(originalPos) : -1;

	C_BaseEntity* pBaseEntity;
	int iAttachmentIndex;
	object_t* pNearObject = null;
	object_t* pTestObject = null;
	float fMinDist = -1.0f;
	float fTestDist;
	std::map<std::string, object_t*>::iterator nearestIt;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pTestObject = it->second;
		if (!pTestObject->spawned || pTestObject == pSelectedObject || pTestObject == pOriginalObject)
		{
			it++;
			continue;
		}

		fTestDist = pTestObject->origin.DistTo(originalPos);

		if (flCutoff < fTestDist && (fMinDist == -1.0f || fTestDist < fMinDist))
		{
			pBaseEntity = C_BaseEntity::Instance(pTestObject->entityIndex);
			if (pBaseEntity)
			{
				iAttachmentIndex = pBaseEntity->LookupAttachment("aacam");
				if (iAttachmentIndex > 0)
				{
					fMinDist = fTestDist;
					pNearObject = pTestObject;
					nearestIt = it;

					//if (!pSelectedEntity)
					//	break;
				}
			}
		}

		it++;
	}

	if (pOriginalObject && !pNearObject && pSelectedObject != pOriginalObject)
		pNearObject = pOriginalObject;

	if (pNearObject)
	{
		C_PropShortcutEntity* pNextEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pNearObject->entityIndex));
		if (pNextEntity)
		{
			//g_pAnarchyManager->DeselectEntity();
			//g_pAnarchyManager->SelectEntity(pNextEntity);

			//g_pAnarchyManager->SetSelectOriginal(pNextEntity->entindex());
			engine->ClientCmd(VarArgs("autocameraselect %i", pNextEntity->entindex()));	// servercmdfix , false);
		}
	}
}

void C_InstanceManager::SelectPrev(C_PropShortcutEntity* pSelectedEntity, C_PropShortcutEntity* pOriginalEntity)
{
	//g_pAnarchyManager->ClearAttractMode();
	if (cvar->FindVar("attract_mode_active")->GetBool() || cvar->FindVar("camcut_attract_mode_active")->GetBool())
		return;

	//* pOriginalObject = m_pInstanceManager->GetObjectEntity(pOriginalEntity->GetObjectId());
	//auto originalIt = m_objects.find(pOriginalEntity->GetObjectId());
	//if (originalIt == m_objects.end())
	//	return;

	object_t* pSelectedObject = (pSelectedEntity) ? m_objects[pSelectedEntity->GetObjectId()] : null;
	object_t* pOriginalObject = (pOriginalEntity) ? m_objects[pOriginalEntity->GetObjectId()] : null;

	Vector originalPos;
	if (pOriginalEntity)
		originalPos = pOriginalEntity->GetAbsOrigin();
	else
		originalPos = MainViewOrigin();
		//originalPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		//originalPos.Init();

	float flCutoff = (pSelectedObject && pOriginalEntity) ? pSelectedObject->origin.DistTo(originalPos) : -1;

	C_BaseEntity* pBaseEntity;
	int iAttachmentIndex;
	object_t* pNearObject = null;
	object_t* pFarObject = null;
	object_t* pTestObject = null;
	float fMinDist = -1.0f;
	float fMaxDist = -1.0f;
	float fTestDist;
	std::map<std::string, object_t*>::iterator nearestIt;
	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		pTestObject = it->second;
		if (!pTestObject->spawned || pTestObject == pSelectedObject || pTestObject == pOriginalObject)
		{
			it++;
			continue;
		}

		fTestDist = pTestObject->origin.DistTo(originalPos);
		if (flCutoff > fTestDist && (fMinDist == -1.0f || fTestDist > fMinDist))
		{
			pBaseEntity = C_BaseEntity::Instance(pTestObject->entityIndex);
			if (pBaseEntity)
			{
				iAttachmentIndex = pBaseEntity->LookupAttachment("aacam");
				if (iAttachmentIndex > 0)
				{
					fMinDist = fTestDist;
					pNearObject = pTestObject;
					nearestIt = it;
				}
			}
		}
		else
		{
			pBaseEntity = C_BaseEntity::Instance(pTestObject->entityIndex);
			if (pBaseEntity)
			{
				iAttachmentIndex = pBaseEntity->LookupAttachment("aacam");
				if (iAttachmentIndex > 0)
				{
					if (fTestDist > fMaxDist)
					{
						fMaxDist = fTestDist;
						pFarObject = pTestObject;

					//if (!pSelectedEntity)
					//	break;
					}
				}
			}
		}

		it++;
	}

	if (pOriginalObject && !pNearObject && pSelectedObject != pOriginalObject)
		pNearObject = pOriginalObject;

	if (!pNearObject && pFarObject && pFarObject != pSelectedObject)
		pNearObject = pFarObject;

	if (pNearObject)
	{
		C_PropShortcutEntity* pNextEntity = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pNearObject->entityIndex));
		if (pNextEntity)
		{
			//g_pAnarchyManager->DeselectEntity();
			//g_pAnarchyManager->SelectEntity(pNextEntity);

			//g_pAnarchyManager->SetSelectOriginal(pNextEntity->entindex());
			engine->ClientCmd(VarArgs("autocameraselect %i", pNextEntity->entindex()));	// servercmdfix , false);
		}
	}
}

void C_InstanceManager::ResetPhysicsOnObject(C_PropShortcutEntity* pShortcut)
{
	KeyValues* pEntryKV = m_pInstanceKV->FindKey(VarArgs("objects/%s", pShortcut->GetObjectId().c_str()));

	// check if this object has been saved to the KV yet
	if (pEntryKV)
	{
		// yes, it already exists and has values to revert to
		object_t* pObject = this->GetInstanceObject(pShortcut->GetObjectId());
		// revert
		// TODO: everything should be condensed into a single server-command

		// FOR NOW: start by setting object ID's
		std::string modelId = pObject->modelId;// pShortcut->GetModelId();
		std::string modelFile;

		KeyValues* entryModel = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId);
		KeyValues* activeModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(entryModel);
		if (activeModel)
			modelFile = activeModel->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID), "models\\cabinets\\missing.mdl");	// uses default model if key value read fails
		else
			modelFile = "models\\cabinets\\missing.mdl";

		pShortcut->PrecacheModel(modelFile.c_str());	// not needed.  handled server-side?
		pShortcut->SetModel(modelFile.c_str());	// not needed.  handled server-side?

		// FOR NOW: lastly, do scale
		engine->ClientCmd(VarArgs("setobjectids %i \"%s\" \"%s\" \"%s\";\n", pShortcut->entindex(), pObject->itemId.c_str(), modelId.c_str(), modelFile.c_str()));	// servercmdfix , false);
		engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z));	// servercmdfix , false);	// FIXME: Other calls to setcabpos in client code may have an additional unused blank string param at the end that the server-side code doesn't ask for.  Fix that.  No extra param should be sent.
		engine->ClientCmd(VarArgs("setscale %i %f;\n", pShortcut->entindex(), pObject->scale));

		if (pObject->child)
			engine->ClientCmd(VarArgs("setparent %i %i;\n", pShortcut->entindex(), pObject->parentEntityIndex));

		// undo build mode FX
		engine->ClientCmd(VarArgs("makenonghost %i %i;\n", pShortcut->entindex(), g_pAnarchyManager->UseBuildGhosts()));

		if (pObject->anim != "")
			pShortcut->PlaySequenceRegular(pObject->anim.c_str());

		// if this item is a 3d text item, it needs to resize its children.
		std::string itemId = pObject->itemId;
		if (itemId != modelId)
		{
			KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId));
			if (pItemKV)
			{
				// check file field for being a text item w/ ?&m=1
				std::string file = pItemKV->GetString("file");
				if (file.find("http://text.txt/") == 0 && (file.find("&m=") != std::string::npos || file.find("?m=") != std::string::npos))
				{
					// it is a text item
					size_t foundAt = file.find("?txt=");
					if (foundAt == std::string::npos)
						foundAt = file.find("&txt=");
					if (foundAt != std::string::npos)
					{
						std::string paramValue = file.substr(foundAt + 5);
						foundAt = paramValue.find_first_of("&#");
						if (foundAt != std::string::npos)
							paramValue = paramValue.substr(0, foundAt);

						if (paramValue != "")
						{
							std::transform(paramValue.begin(), paramValue.end(), paramValue.begin(), ::toupper);
							//DevMsg("ParamValue: %s\n", paramValue.c_str());

							engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", pShortcut->entindex(), paramValue.c_str()));
						}
					}
				}
			}
		}
	}
}
void C_InstanceManager::PlayNearestGIF()
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->GetHealth() <= 0)
		return;

	object_t* pStartingObject = null;
	float flMaxRange = 1000.0f;
	Vector startPos = g_pAnarchyManager->GetSelectorTraceVector();

	KeyValues* pNearBaseItemKV = null;
	object_t* pNearObject = null;
	object_t* pTestObject = null;
	float fMinDist = flMaxRange;	// the textDist must be LESS THAN this to be included
	float fTestDist;
	float fStartingDist = 0.0f;
	if (pStartingObject)
		fStartingDist = pStartingObject->origin.DistTo(startPos);

	// Grab the nearest object to player look
	size_t found;
	KeyValues* pBaseItemKV;
	KeyValues* pItemKV;
	std::string testString;
	C_SteamBrowserInstance* pSteamBrowserInstance;
	std::string testTaskId;

	std::map<std::string, object_t*>::iterator it = m_objects.begin();
	while (it != m_objects.end())
	{
		if (!it->second->spawned || it->second->entityIndex < 0)
		{
			it++;
			continue;
		}

		pTestObject = it->second;
		fTestDist = pTestObject->origin.DistTo(startPos);
		if (fTestDist > fStartingDist && (fMinDist == -1 || fTestDist < fMinDist))
		{
			// ONLY if it has a GIF in its item.
			pBaseItemKV = g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", pTestObject->itemId);
			pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pBaseItemKV);
			if (pItemKV)
			{
				testString = pItemKV->GetString("file");
				std::transform(testString.begin(), testString.end(), testString.begin(), ::tolower);

				found = testString.find_first_of("?#");
				if (found != std::string::npos)
					testString = testString.substr(0, found);

				if (testString.find(".gif") == testString.length() - 4 || testString.find(".webp") == testString.length() - 5)
				{
					// and ONLY if it isn't already active
					testTaskId = std::string("auto") + std::string(pItemKV->GetString("info/id"));
					pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance(testTaskId);

					if (!pSteamBrowserInstance)
					{
						pNearBaseItemKV = pBaseItemKV;
						fMinDist = fTestDist;
						pNearObject = pTestObject;
					}
				}
			}
		}

		it++;
	}

	if (pNearObject)
	{
		//g_pAnarchyManager->QuickRemember(pNearObject->entityIndex);
		//g_pAnarchyManager->GetAccountant()->Action("aa_objects_autoplayed", 1);

		// If the currently selected input is ALSO a steamworks browser, we need to make sure it retains focus.
		/*
		C_EmbeddedInstance* pInputEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		C_SteamBrowserInstance* pInputSteamInstance = dynamic_cast<C_SteamBrowserInstance*>(pInputEmbeddedInstance);

		C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->AutoInspect(pBaseItemKV, "", pNearObject->entityIndex);

		if (pInputSteamInstance)
			g_pAnarchyManager->GetSteamBrowserManager()->SelectSteamBrowserInstance(pInputSteamInstance);
		else
		{
			C_AwesomiumBrowserInstance* pInputAwesomiumInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pInputEmbeddedInstance);
			g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pInputAwesomiumInstance);
		}
		*/

		//g_pAnarchyManager->QuickRemember(pNearObject->entityIndex);
		C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->AutoInspect(pNearBaseItemKV, "", pNearObject->entityIndex, "1");
		//g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(pEmbeddedInstance);
	}
}

void C_InstanceManager::CloneInstance(std::string instanceId)
{
	// First, get the instance
	instance_t* pInstance = this->GetInstance(instanceId);
	if (!pInstance)
	{
		g_pAnarchyManager->AddToastMessage("Failed To Clone Instance");
		return;
	}

	// Now, get everything ready for the clone instance.
	int iLegacy = pInstance->legacy;
	std::string cloneInstanceId = g_pAnarchyManager->GenerateUniqueId();
	std::string mapId = pInstance->mapId;
	std::string title = "(CLONE) " + pInstance->title;
	std::string file = pInstance->file;
	std::string workshopIds = pInstance->workshopIds;
	std::string mountIds = pInstance->mountIds;
	std::string autoplayId = pInstance->autoplayId;
	std::string style = pInstance->style;

	if (file != "" || iLegacy)
	{
		DevMsg("Only tested on non-legacy save instances so far! Aborting.\n");
		g_pAnarchyManager->AddToastMessage("Failed To Clone Instance");
		// Probably just need to completely break any Legacy references to support legacy saves - if it is even an issue.
		return;
	}

	this->AddInstance(iLegacy, cloneInstanceId, mapId, title, file, workshopIds, mountIds, autoplayId, style);
	instance_t* pCloneInstance = this->GetInstance(cloneInstanceId);

	// now clone the KV

	C_Backpack* pBackpack = null;
	KeyValues* pInstanceKV = new KeyValues("instance");
	if (!g_pAnarchyManager->GetMetaverseManager()->LoadSQLKevValues("instances", pInstance->id.c_str(), pInstanceKV))
	{
		// if this wasn't in our library, try other librarys.
		// check all backpacks...
		pBackpack = g_pAnarchyManager->GetBackpackManager()->FindBackpackWithInstanceId(pInstance->id);
		if (pBackpack)
		{
			// we found the backpack containing this instance ID
			DevMsg("Loading from instance backpack w/ ID %s...\n", pBackpack->GetId().c_str());
			pBackpack->OpenDb();
			sqlite3* pDb = pBackpack->GetSQLDb();
			if (!pDb || !g_pAnarchyManager->GetMetaverseManager()->LoadSQLKevValues("instances", pInstance->id.c_str(), pInstanceKV, pDb))
			{
				DevMsg("CRITICAL ERROR: Failed to load instance from library!\n");
				pBackpack->CloseDb();
				pBackpack = null;
			}
			else
				pBackpack->CloseDb();
		}

		if (!pBackpack)
		{
			DevMsg("WARNING: Could not load instance!");// Attempting to load as legacy instance...\n");
			pInstanceKV->deleteThis();
			pInstanceKV = null;
		}
	}

	if (pInstanceKV)
	{
		pInstanceKV->SetString("info/local/id", cloneInstanceId.c_str());
		pInstanceKV->SetString("info/local/title", pCloneInstance->title.c_str());
		pInstanceKV->SetString("info/local/map", pCloneInstance->mapId.c_str());
		pInstanceKV->SetString("info/local/autoplay", pCloneInstance->autoplayId.c_str());

		if (!pBackpack)
			g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", cloneInstanceId.c_str(), pInstanceKV);
		else
		{
			DevMsg("Weird backpack detected! Don't know how to handle it! Fix it!\n");
			//g_pAnarchyManager->GetMetaverseManager()->SaveSQL(*(pBackpack->GetSQLDb()), "instances", pInstance->id.c_str(), pInstanceKV);
		}

		pInstanceKV->deleteThis();
		pInstanceKV = null;

		g_pAnarchyManager->AddToastMessage("Instance Has Been Cloned");
	}
	else
		g_pAnarchyManager->AddToastMessage("Failed To Clone Instance");
}

void C_InstanceManager::CloneObject(C_PropShortcutEntity* pShortcut)
{
	KeyValues* pEntryKV = m_pInstanceKV->FindKey(VarArgs("objects/%s", pShortcut->GetObjectId().c_str()));

	// check if this object has been saved to the KV yet
	if (pEntryKV)
	{
		// yes, it already exists and has values to revert to
		object_t* pOldObject = this->GetInstanceObject(pShortcut->GetObjectId());
		// revert
		// TODO: everything should be condensed into a single server-command

		// FOR NOW: start by setting object ID's
		std::string modelId = pOldObject->modelId;// pShortcut->GetModelId();
		std::string modelFile;

		KeyValues* entryModel = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId);
		KeyValues* activeModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(entryModel);
		if (activeModel)
			modelFile = activeModel->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID), "models\\cabinets\\missing.mdl");	// uses default model if key value read fails
		else
			modelFile = "models\\cabinets\\missing.mdl";

		std::string category = "items";
		if (pOldObject->itemId == pOldObject->modelId || pOldObject->modelId == "")
			category = "models";

		g_pAnarchyManager->GetMetaverseManager()->SetLibraryBrowserContext(category, pOldObject->itemId, "", "");

		// get the point that the local player is looking at
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

		Vector normal = g_pAnarchyManager->GetSelectorTraceNormal();
		Vector endpos = g_pAnarchyManager->GetSelectorTraceVector();

		VMatrix entToWorld;
		Vector xaxis;
		Vector yaxis;

		if (normal.z == 0.0f)
		{
			yaxis = Vector(0.0f, 0.0f, 1.0f);
			CrossProduct(yaxis, normal, xaxis);
			entToWorld.SetBasisVectors(normal, xaxis, yaxis);
		}
		else
		{
			Vector objectToPlayer;
			VectorSubtract(pPlayer->GetAbsOrigin(), Vector(endpos.x, endpos.y, endpos.z), objectToPlayer);

			xaxis = Vector(objectToPlayer.x, objectToPlayer.y, objectToPlayer.z);

			CrossProduct(normal, xaxis, yaxis);
			if (VectorNormalize(yaxis) < 1e-3)
			{
				xaxis.Init(0.0f, 0.0f, 1.0f);
				CrossProduct(normal, xaxis, yaxis);
				VectorNormalize(yaxis);
			}
			CrossProduct(yaxis, normal, xaxis);
			VectorNormalize(xaxis);

			entToWorld.SetBasisVectors(xaxis, yaxis, normal);
		}

		QAngle angles;
		MatrixToAngles(entToWorld, angles);

		// position in tr.endpos
		// orientation in angles

		if (modelId == "")
		{
			if (category == "items")
			{
				std::string testerModelId = cvar->FindVar("recent_model_id")->GetString();
				if (testerModelId != "")
					modelId = testerModelId;
				else
					modelId = g_pAnarchyManager->GenerateLegacyHash("models/cabinets/missing.mdl");	// TODO: Get the actual default model to use from the item itself, or intellegently figure out which one the user probably wants otherwise.
			}
			else if (category == "models")
			{
				// an empty modelId means the entry IS a model.
				modelId = "";
			}
		}

		g_pAnarchyManager->GetInstanceManager()->AdjustObjectRot(0, 0, 0);
		g_pAnarchyManager->GetInstanceManager()->AdjustObjectOffset(0, 0, 0);
		g_pAnarchyManager->GetInstanceManager()->AdjustObjectScale(pOldObject->scale);

		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->AddObject("", pOldObject->itemId, modelId, endpos, angles, pOldObject->scale, pOldObject->anim, false, pOldObject->body, pOldObject->skin);
		g_pAnarchyManager->GetMetaverseManager()->SetSpawningObject(pObject);
		g_pAnarchyManager->GetInstanceManager()->SpawnObject(pObject, false);
	}
}

void C_InstanceManager::LevelShutdownPostEntity()
{
	DevMsg("FIXME: C_InstanceManager::LevelShutdownPostEntity needs to actually clear shit out (or does it?)!!\n");

	m_bHadDebugText = false;

	m_fNearestSpawnDist = m_pNearestSpawnDistConVar->GetFloat();
	m_uNextFlashedObject = 0;
	m_fLastSpawnActionPressed = 0;
	//m_fNearestSpawnDist = 0;
	m_iUnspawnedWithinRangeEstimate = 0;

	// clear out objects and unspanwed objects
	auto it = m_objects.begin();
	while (it != m_objects.end())
	{
		delete it->second;
		it++;
	}

	m_objects.clear();
	m_unspawnedObjects.clear();

	if (m_pInstanceKV)
	{
		m_pInstanceKV->deleteThis();
		m_pInstanceKV = null;
	}

	m_incomingNodeId = "";
}

void C_InstanceManager::LevelShutdownPreEntity()
{
	this->ResetAllMaterialMods();
}

//#include "debugoverlay_shared.h"
void C_InstanceManager::Update()
{
	if (engine->IsInGame() && m_pReShadeConVar->GetBool() && m_pReShadeDepthConVar->GetBool())
	{
		bool bHasDebugText = (debugoverlay->GetFirst()) ? true : false;
		if (bHasDebugText && !m_bHadDebugText)
		{
			if (!m_pVGUIConVar->GetBool())
				m_pVGUIConVar->SetValue(1);
		}
		else if (!bHasDebugText && m_bHadDebugText)
		{
			if (!g_pAnarchyManager->GetInputManager()->GetFullscreenMode() && m_pVGUIConVar->GetBool())
				m_pVGUIConVar->SetValue(0);
		}
		
		m_bHadDebugText = bHasDebugText;
	}

	return;	// FIXME: Re-enable later.  Disabled for stablity of public beta.

	if (engine->IsInGame() && !g_pAnarchyManager->GetSuspendEmbedded() && g_pAnarchyManager->GetInstanceId() != "")
	{
	//	DevMsg("Letters callback\n");

		// grab an iterator to the last flashed object and increment it, otherwise just use .begin()
		unsigned int max = m_unspawnedObjects.size();
		if (m_uNextFlashedObject >= max)
			m_uNextFlashedObject = 0;

		//m_uNextFlashedObject++;
		//if (m_uNextFlashedObject >= max)
//			m_uNextFlashedObject = 0;

		if (max > 0)
		{
			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			Vector playerPos = pPlayer->GetAbsOrigin();

			object_t* pTestObject = null;
			float fDuration = 3.0f;
			float fMinDist = 420.0;
			float fTestDist;
			//std::vector<object_t*>::iterator nearestIt;
			//std::vector<object_t*>::iterator it = m_unspawnedObjects.begin();
			while (m_uNextFlashedObject < max)
			{
				pTestObject = m_unspawnedObjects[m_uNextFlashedObject];
				fTestDist = pTestObject->origin.DistTo(playerPos);
				//if (fTestDist < m_fNearestSpawnDist && (fMinDist == -1 || fTestDist < fMinDist))
				if (fMinDist == -1 || fTestDist < fMinDist)
				{
					//fMinDist = fTestDist;
					//pNearObject = pTestObject;
					//nearestIt = it;

					KeyValues* kv = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pTestObject->itemId);
					if (!kv)	// load the model instead if no item exists
						kv = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pTestObject->modelId);

					if (kv)
					{
						KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(kv);
						NDebugOverlay::Text(pTestObject->origin, active->GetString("title"), true, fDuration);
						break;
						//debugoverlay->AddTextOverlay(pTestObject->origin, 3.0, "%s", active->GetString("title"));
					}
				}

				break;
				//m_uNextFlashedObject++;
			}

			m_uNextFlashedObject++;
			if (m_uNextFlashedObject >= max)
				m_uNextFlashedObject = 0;
		}
	}
}

// TODO: Generalize this function so that nodes can use it to spawn in their objects. :)
/*
	x. read this method carefully and re-learn its logic.
	x. re-read this method again, mentally separating the logic that is required for loading an instance vs loading a node.
	x. re-read this method again, figuring in the logic that would follow the calls to this method in the TWO cases it is used from.
	x. make an algo for what the new version of this method will actually do, including any other sub-routines it'll be broken up into.
	5. dowork.
*/
KeyValues* C_InstanceManager::LoadInstance(C_PropShortcutEntity* pParentNodeEntity, std::string instanceId, std::string position, std::string rotation, bool bDoNotSpawn)
{
	// Legacy saves un-legacy themselves after they are loaded for the 1st time!!
	// Legacy saves are detected, converted, and saved, all in 1 swoop when being loaded for use.

	DevMsg("Load the instance!!!\n");
	instance_t* pInstance = this->GetInstance(instanceId);
	
	//DevMsg("Attempting to load instance w/ ID: %s\n", instanceId.c_str());

	KeyValues* instanceKV = new KeyValues("instance");
	if ( !g_pAnarchyManager->GetMetaverseManager()->LoadSQLKevValues("instances", pInstance->id.c_str(), instanceKV))
	{
		// if this wasn't in our library, try other librarys.
		// check all backpacks...
		C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->FindBackpackWithInstanceId(pInstance->id);
		if (pBackpack)
		{
			// we found the backpack containing this instance ID
			DevMsg("Loading from instance backpack w/ ID %s...\n", pBackpack->GetId().c_str());
			pBackpack->OpenDb();
			sqlite3* pDb = pBackpack->GetSQLDb();
			if (!pDb || !g_pAnarchyManager->GetMetaverseManager()->LoadSQLKevValues("instances", pInstance->id.c_str(), instanceKV, pDb))
			{
				DevMsg("CRITICAL ERROR: Failed to load instance from library!\n");
				pBackpack->CloseDb();
				pBackpack = null;
			}
			else
				pBackpack->CloseDb();
		}

		//instanceKV->deleteThis();

		if ( !pBackpack )
		{
			DevMsg("WARNING: Could not load instance!");// Attempting to load as legacy instance...\n");
			instanceKV->deleteThis();
			instanceKV = null;

			//if (pInstance->file != "")
				//this->LoadLegacyInstance(instanceId, instanceKV);

			//m_pInstanceKV = instanceKV;	// FIXME: if this fails to load the instance, the KeyValues instanceKV is NOT deleted or cleaned up anywhere.
		}
	}

	if (instanceKV)
	{
		C_BaseEntity* pEntity = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();
		bool bIsSoleSpawn = (pEntity == pParentNodeEntity);

		std::string objectId;
		std::string itemId;
		std::string modelId;
		std::string testItemId;
		std::string testModelId;
		std::vector<KeyValues*> badObjectKeys;
		bool bWasDuplicate;
		bool bFoundSelf;
		Vector testOrigin;
		Vector testOrigin2;
		for (KeyValues *sub = instanceKV->FindKey("objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			objectId = sub->GetString("local/info/id");

			if (objectId == "" || (!pParentNodeEntity && sub->GetBool("local/child") && (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost)))
			{
				badObjectKeys.push_back(sub);
				continue;
			}

			// test for duplicate objects...
			if (pParentNodeEntity)
			{
				UTIL_StringToVector(testOrigin.Base(), sub->GetString("local/position", "0 0 0"));
				bWasDuplicate = false;
				bFoundSelf = false;
				for (KeyValues *sub2 = instanceKV->FindKey("objects", true)->GetFirstSubKey(); sub2; sub2 = sub2->GetNextKey())
				{
					if (sub2 == sub)
					{
						bFoundSelf = true;
						continue;
					}
					else if (!bFoundSelf)
						continue;

					UTIL_StringToVector(testOrigin2.Base(), sub2->GetString("local/position", "0 0 0"));
					if (testOrigin.DistTo(testOrigin2) < 0.1)
					{
						bWasDuplicate = true;
						break;
					}
				}

				if (bWasDuplicate)
				{
					badObjectKeys.push_back(sub);
					continue;
				}
			}

			//DevMsg("Checkpoint A\n");
			// if this is a node object, each object needs a unique id.
			if (pParentNodeEntity)
				objectId = g_pAnarchyManager->GenerateUniqueId();
			//else if (!bDoNotSpawn && sub->GetBool("local/child") )//&& (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost))	// don't spawn child objects if we are the world instance & we are NOT the server host.
			//{
			//	DevMsg("Skipped spawning child object.\n");
			//	continue;
			//}

			itemId = sub->GetString("local/item");

			// is this a node itself?  prevent node-ception.
			if (pParentNodeEntity)
			{
				KeyValues* pSubItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", itemId));
				if (pSubItemKV)
				{
					KeyValues* pSubTypeKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("types", pSubItemKV->GetString("type")));
					if (pSubTypeKV && !Q_stricmp(pSubTypeKV->GetString("title"), "node"))
						continue;
				}
			}

			if (pInstance->legacy)
			{
				testItemId = g_pAnarchyManager->ExtractLegacyId(itemId);

				if (testItemId == "")
				{
					// don't generate item id's for legacy prop objects
					testItemId = itemId;
					size_t foundExt = testItemId.find(".mdl");
					if (foundExt != testItemId.length() - 4)
					{
						KeyValues* pSearchInfo = new KeyValues("search");
						pSearchInfo->SetString("file", itemId.c_str());

						KeyValues* foundItem = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryItem(pSearchInfo));
						if (foundItem)
							testItemId = foundItem->GetString("info/id");
						else
						{
							DevMsg("Instance failed to resolve item with itemfile: %s\n", itemId.c_str());
							testItemId = g_pAnarchyManager->GenerateLegacyHash(itemId.c_str());
						}

						if (testItemId != "" && testItemId != "ffffffff" )
							itemId = testItemId;
					}
				}
				else
					itemId = testItemId;

				// write the resolved ID to the KV.  (don't forget to un-legacy tag it when saving out the instance!)
				sub->SetString("local/item", itemId.c_str());
			}

			modelId = sub->GetString("local/model");
			if (pInstance->legacy)
			{
				// model gets resolved on-the-fly for legacy saves
				//modelId = g_pAnarchyManager->GenerateLegacyHash(modelId.c_str());

				testModelId = g_pAnarchyManager->ExtractLegacyId(modelId);

				if (testModelId == "")
				{
					// don't generate model id's for legacy prop objects
					testModelId = modelId;
					size_t foundExt = testModelId.find(".mdl");
					if (foundExt == testModelId.length() - 4)
					{
						KeyValues* pSearchInfo = new KeyValues("search");
						pSearchInfo->SetString("file", modelId.c_str());

						KeyValues* foundModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryModel(pSearchInfo));
						if (foundModel)
							testModelId = foundModel->GetString("info/id");
						else
						{
							// If no model exists for this MDL, but this MDL DOES exist, then create an MDL entry for it!

							// TODO: DO THIS SHIT NEXT TIME!! AS DESCRIBED ABOVE!!!!!!!!!!!!!!!!
							DevMsg("Instance failed to resolve model with modelfile: %s\n", modelId.c_str());
							testModelId = g_pAnarchyManager->GenerateLegacyHash(modelId.c_str());
						}

						if (testModelId != "" && testModelId != "ffffffff")
							modelId = testModelId;
					}
				}
				else
					modelId = testModelId;

				// write the resolved ID to the KV.  (don't forget to un-legacy tag it when saving out the instance!)
				sub->SetString("local/model", modelId.c_str());
			}

			// alright, spawn this object
			if (!bDoNotSpawn)
			{
				Vector origin;
				UTIL_StringToVector(origin.Base(), sub->GetString("local/position", "0 0 0"));	// FIXME: TODO: Failing to properly read a position at this point might indicate a broken item KV.  should try and detect it instead of having a fallback of "0 0 0"

				QAngle angles;
				UTIL_StringToVector(angles.Base(), sub->GetString("local/rotation", "0 0 0"));

				float scale = sub->GetFloat("local/scale", 1.0f);

				if (pParentNodeEntity)
				{
					float flParentScale = pParentNodeEntity->GetModelScale();
					origin *= flParentScale;

					VMatrix childMatrix;
					childMatrix.SetupMatrixOrgAngles(origin, angles);

					VMatrix composedMatrix;
					composedMatrix.SetupMatrixOrgAngles(pParentNodeEntity->GetAbsOrigin(), pParentNodeEntity->GetAbsAngles());
					composedMatrix = composedMatrix * childMatrix;

					// back to vecs & angles
					MatrixAngles(composedMatrix.As3x4(), angles, origin);

					scale *= flParentScale;
					//pParentNodeEntity->
				}

				// DO MAGIC:
				// Here's a couple of possible ways to relatively attach to the parent.
				// Keep in mind, Legacy had to use a weird way to make it work, so the obvious way is likely going to fail.
				// A: Attach the entity to the parent with a magic built-in Source engine attach method that takes a relative offset. (Maybe a UTIL.)
				// B: Zero-out the parentEntity's matrix (except use 1.0 for scale) and attach the child objects to the parentEntity,
				//		then restore the parentEntity's original matrix.  (This is how Legacy spawned nodes.)

				// FIXME: TODO: OBSOLETE: this isn't needed after the exporter in legacy is fixed!
				if (scale == 0)
					scale = 1.0f;

				bool slave = (sub->GetInt("local/slave") > 0);
				//bool physics = (sub->GetInt("slave") > 0); // FIXME: TODO: Use the physics stuff too!

				std::string anim = sub->GetString("local/anim");

				int iParentEntityIndex = (pParentNodeEntity) ? pParentNodeEntity->entindex() : -1;
				bool child = (pParentNodeEntity != null);// (iParentEntityIndex < 0) ? false : true;// sub->GetBool("local/child");

				//DevMsg("IDs here are: %s %s %s\n", objectId.c_str(), itemId.c_str(), modelId.c_str());
				object_t* ourObject = this->AddObject(objectId, itemId, modelId, origin, angles, scale, anim, slave, sub->GetInt("local/body", 0), sub->GetInt("local/skin", 0), 0, "", 0, "", 0, "", child, iParentEntityIndex);

				// if this is a child of a node, we gotta manually add it to the ACTIVE KV too...
				if (pParentNodeEntity)
				{
					KeyValues* pActiveInstanceObjectKV = m_pInstanceKV->FindKey(VarArgs("objects/%s", objectId.c_str()), true);
					pActiveInstanceObjectKV->SetString("local/info/id", ourObject->objectId.c_str());
					pActiveInstanceObjectKV->SetString("local/item", ourObject->itemId.c_str());
					pActiveInstanceObjectKV->SetString("local/model", ourObject->modelId.c_str());
					pActiveInstanceObjectKV->SetString("local/position", VarArgs("%f %f %f", origin.x, origin.y, origin.z));
					pActiveInstanceObjectKV->SetString("local/rotation", VarArgs("%f %f %f", angles.x, angles.y, angles.z));
					pActiveInstanceObjectKV->SetFloat("local/scale", ourObject->scale);
					pActiveInstanceObjectKV->SetInt("local/slave", ourObject->slave);
					pActiveInstanceObjectKV->SetInt("local/child", ourObject->child);
					pActiveInstanceObjectKV->SetInt("local/body", ourObject->body);
					pActiveInstanceObjectKV->SetInt("local/skin", ourObject->skin);
				}

				if (pInstance->style != "" && bIsSoleSpawn)
					g_pAnarchyManager->GetInstanceManager()->SpawnObject(ourObject, false);
			}
			//DevMsg("Checkpoint B\n");
		}

		unsigned int max = badObjectKeys.size();
		KeyValues* pObjectsKey = instanceKV->FindKey("objects", true);
		for (unsigned int i = 0; i < max; i++)
			pObjectsKey->RemoveSubKey(badObjectKeys[i]);

		if (max > 0)
			badObjectKeys.clear();

		if (!pParentNodeEntity)
		{
			if (!bDoNotSpawn)
			{
				m_pInstanceKV = instanceKV;

				m_pActiveMaterialMods = new KeyValues("overrides");
				this->ResetAllMaterialMods();
				this->ApplyAllMaterialMods();

				std::string loadingScreenshotId = g_pAnarchyManager->GetMetaverseManager()->GetLoadingScreenshotId();

				// check if we should teleport
				if (position != "" && rotation != "")
				{
					C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

					// try to grab a camera position from a screenshot instead of including camera position / rotation as regular params like body pos & rot
					std::string cameraRotation = "0 0 0";
					if (loadingScreenshotId != "")
					{
						KeyValues* pScreenshotKV = g_pAnarchyManager->GetMetaverseManager()->GetScreenshot(loadingScreenshotId);
						if (pScreenshotKV && !Q_strcmp(pScreenshotKV->GetString("body/position"), position.c_str()))
							cameraRotation = pScreenshotKV->GetString("camera/rotation", "0 0 0");
					}

					if (cameraRotation == "0 0 0")
						engine->ClientCmd(VarArgs("teleport_player %i %s %s\n", pPlayer->entindex(), position.c_str(), rotation.c_str()));	// servercmdfix , true);
					else
						engine->ClientCmd(VarArgs("teleport_player %i %s %s %s\n", pPlayer->entindex(), position.c_str(), rotation.c_str(), cameraRotation.c_str()));	// servercmdfix , true);
				}


				//if ( this->GetInstanceObjectCount() > 0)
				//{
				std::string screenshotPostfix = (loadingScreenshotId != "") ? "&screenshot=" + loadingScreenshotId : "";

				std::string uri;
				if (g_pAnarchyManager->GetCurrentLobby() == "")
				{
					//float fl_minDist = cvar->FindVar("aapropfademin")->GetFloat();
					//float fl_maxDist = cvar->FindVar("aapropfademax")->GetFloat();
					//std::string maxDist = (fl_minDist >= 0 && fl_maxDist > 0) ? VarArgs("%f", fl_maxDist) : "99999999999.9";


					std::string maxDist = VarArgs("%f", this->GetNearestSpawnDist());
					uri = "asset://ui/spawnItems.html?max=" + maxDist + screenshotPostfix;//std::string("99999999999.9") + screenshotPostfix;
				}
				else
				{
					if (g_pAnarchyManager->GetCurrentLobbyTitle() != "")
					{
						pInstance->title = g_pAnarchyManager->GetCurrentLobbyTitle();
						m_pInstanceKV->SetString("info/local/title", g_pAnarchyManager->GetCurrentLobbyTitle().c_str());
					}

					std::string maxDist = VarArgs("%f", this->GetNearestSpawnDist());
					//uri = "asset://ui/sync.html?max=" + std::string("99999999999.9") + screenshotPostfix;
					uri = "asset://ui/sync.html?max=" + maxDist + screenshotPostfix;
				}

				C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
				g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pHudBrowserInstance);
				pHudBrowserInstance->SetUrl(uri);
				//g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
				//}
				//else
				//	g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

				g_pAnarchyManager->GetMetaverseManager()->SetLoadingScreenshotId("");
			}
		}
	}

	//DevMsg("Checkpoint C\n");
	return instanceKV;	// Note: Nodes who pass in a pParentNodeEntity MUST delete this returned KV manually!
}

void C_InstanceManager::LoadLegacyInstance(std::string instanceId, KeyValues* instanceKV)
{
	instance_t* pInstance = this->GetInstance(instanceId);
	DevMsg("Load the LEGACYinstance!!! %s\n", pInstance->file.c_str());
	
	bool spawnedOne = false;
	KeyValues* item;
	KeyValues* activeItem;
	//KeyValues* modelSearchInfo = new KeyValues("search");
	std::string modelId;
	KeyValues* model;
	KeyValues* activeModel;
	//std::string modelFile;
	std::string itemId;
	std::string fileName = pInstance->file;
	KeyValues* legacyKv = new KeyValues("instance");
	std::string testBuf;
	if (legacyKv->LoadFromFile(g_pFullFileSystem, fileName.c_str(), ""))
	{
		DevMsg("Creating KV for changes to this instance.\n");

		//KeyValues* kv = new KeyValues("instance");
		instanceKV->SetInt("generation", 3);

		instance_t* pInstance = this->GetInstance(g_pAnarchyManager->GetInstanceId());

		KeyValues* info = instanceKV->FindKey("info", true);
		info->SetString("id", pInstance->id.c_str());
		info->SetString("title", pInstance->title.c_str());
		info->SetString("map", pInstance->mapId.c_str());
		info->SetString("style", pInstance->style.c_str());	// for nodes
		//info->SetString("created", "0");	// store unsigned ints as strings, then %llu them
		info->SetString("owner", "local");


		for (KeyValues *sub = legacyKv->FindKey("objects", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			itemId = g_pAnarchyManager->ExtractLegacyId(sub->GetString("itemfile"));

			if (itemId == "")
			{
				// don't generate item id's for legacy prop objects
				testBuf = sub->GetString("itemfile");
				size_t foundExt = testBuf.find(".mdl");
				if (foundExt != testBuf.length() - 4)
				{
					KeyValues* pSearchInfo = new KeyValues("search");
					pSearchInfo->SetString("file", sub->GetString("itemfile"));

					KeyValues* foundActive = null;
					KeyValues* foundItem = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryItem(pSearchInfo));
					if (foundItem)
						itemId = foundActive->GetString("info/id");
					else
					{
						DevMsg("Instance failed to resolve item with itemfile: %s\n", sub->GetString("itemfile"));
						itemId = g_pAnarchyManager->GenerateLegacyHash(sub->GetString("itemfile"));
					}
				//	else
				//		DevMsg("Resolved it to: %s\n", itemId.c_str());
				}
			}

			modelId = g_pAnarchyManager->GenerateLegacyHash(sub->GetString("model"));
			//DevMsg("Item title: %s w/ %s\n", activeItem->GetString("title"), modelId.c_str());

			// alright, spawn this object
			Vector origin;
			UTIL_StringToVector(origin.Base(), sub->GetString("origin", "0 0 0"));

			QAngle angles;
			UTIL_StringToVector(angles.Base(), sub->GetString("angles", "0 0 0"));

			float scale = sub->GetFloat("scale", 1.0f);

			bool slave = (sub->GetInt("slave") > 0);
			bool child = sub->GetBool("isChild");

			object_t* pObject = this->AddObject("", itemId, modelId, origin, angles, scale, "", slave, 0, 0, 0, "", 0, "", 0, "", child);	// isChild cuz this is a LEGACY instance that is BEING imported, so it's using gen 2 shit.

			// create an object in the KV
			KeyValues* objectKV = instanceKV->FindKey(VarArgs("objects/%s", pObject->objectId.c_str()), true);
			objectKV->SetString("local/info/id", pObject->objectId.c_str());
			objectKV->SetString("local/item", pObject->itemId.c_str());
			objectKV->SetString("local/model", pObject->modelId.c_str());
			objectKV->SetString("local/position", sub->GetString("origin", "0 0 0"));
			objectKV->SetString("local/rotation", sub->GetString("angles", "0 0 0"));
			objectKV->SetFloat("local/scale", pObject->scale);
			objectKV->SetInt("local/slave", pObject->slave);
			objectKV->SetInt("local/child", pObject->child);
						
			/*
			item = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId);
			if (item)
			{
				activeItem = item->FindKey("current");
				if (!activeItem)
					activeItem = item->FindKey("local", true);
			}
			*/
		}
	}
	legacyKv->deleteThis();
	//modelSearchInfo->deleteThis();
}