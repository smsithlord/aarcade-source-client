#include "cbase.h"

#include "c_prop_shortcut.h"
//#include "vgui/ISystem.h"
//#include "boost/regex.h"
//#include "baseviewport.h"
//#include "ienginevgui.h"
//#include "inetchannelinfo.h"
//#include "ICreateHotlink.h"
//#include "ienginevgui.h"
//#include <cassert>
//#include <string>
#include <algorithm>

#include "c_anarchymanager.h"
#include "materialsystem/imaterialsystem.h"

#include <KeyValues.h>
#include "Filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_PropShortcutEntity, DT_PropShortcutEntity, CPropShortcutEntity)
	RecvPropBool(RECVINFO(m_bSlave)),
	RecvPropString(RECVINFO(m_objectId)),
	RecvPropString(RECVINFO(m_itemId)),
	RecvPropString(RECVINFO(m_modelId)),
END_RECV_TABLE()

C_PropShortcutEntity::C_PropShortcutEntity()
{
	m_bInitialized = false;
	m_bAlreadySetObjectEntity = false;
	m_pCloudAssetsDownload = null;
	m_pSequenceBlacklistConVar = null;
	m_pCloudAssetsDownload = null;
	m_pFadeDistMinConVar = null;
	m_pFadeDistMaxConVar = null;
	m_bDrawForeground = false;
	m_pForegroundMaterial = null;

	this->SetHotlink(true);
}

C_PropShortcutEntity::~C_PropShortcutEntity()
{
}

void C_PropShortcutEntity::Initialize()
{
	if (m_bInitialized)
		return;

	Vector absOrigin = this->GetAbsOrigin();
	object_t* pSpawningObject = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject();
	//if (pSpawningObject)
	//	DevMsg("Abs origin (dist %f) is: %f %f %f vs %f %f %f\n", pSpawningObject->origin.DistTo(absOrigin), absOrigin.x, absOrigin.y, absOrigin.z, absOrigin.x, absOrigin.y, absOrigin.z);

	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity() == null && pSpawningObject && pSpawningObject->origin.DistTo(absOrigin) < 1.0)// GEEZ! not even position is set yet.  defer //&& !Q_strcmp(pSpawningObject->itemId.c_str(), m_itemId) )	// FIXME: accounts for rounding errors, but can produce false-positives!! // FIXME2: m_itemId isn't set until the 1st data update, der.  so this check is probably very unqualified.  too many false positive posibilities.
	{
		// THIS IS THE OBJECT THE CURRENT USER IS SPAWNING!!
		//Precache();
		//SetModel("models\\cabinets\\two_player_arcade.mdl");// VarArgs("%s", this->GetModelName()));

		/*
		std::string modelId = (pSpawningObject->modelId != "") ? pSpawningObject->modelId : pSpawningObject->itemId;// g_pAnarchyManager->GenerateLegacyHash("models/cabinets/two_player_arcade.mdl");
		std::string modelFile;

		KeyValues* entryModel = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(modelId);
		KeyValues* activeModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(entryModel);
		if (activeModel)
			modelFile = activeModel->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID), "models\\cabinets\\two_player_arcade.mdl");	// uses default model if key value read fails
		else
			modelFile = "models\\icons\\missing.mdl";

		SetModel(modelFile.c_str());

		SetSolid(SOLID_NONE);
		SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		//SetRenderMode(kRenderTransTexture);
		SetRenderMode(kRenderTransColor);
		SetRenderColorA(160);
		*/
		g_pAnarchyManager->ActivateObjectPlacementMode(this);
	}
	else
	{
		//this->SetCollisionBounds(Vector(-1000, -1000, -1000), Vector(1000, 1000, 1000));
		//this->RefreshCollisionBounds();
		//this->RefreshCollisionBounds();
		//this->CollisionProp()->RefreshScaledCollisionBounds();
		//SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		/*
		// This is a regular object that already existed or somebody else spawned
		//Precache();
		SetModel("models\\icons\\missing.mdl");// VarArgs("%s", this->GetModelName()));
		//SetSolid(SOLID_NONE);

		SetSolid(SOLID_VPHYSICS);
		SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		SetMoveType(MOVETYPE_VPHYSICS);

		SetRenderColorA(255);
		SetRenderMode(kRenderNormal);

		if (CreateVPhysics())
		{
			IPhysicsObject *pPhysics = this->VPhysicsGetObject();
			if (pPhysics)
			{
				pPhysics->EnableMotion(false);
			}
		}

		*/

		//this->SetModelScale(0.5, 0);
		//this->CreateVPhysics();
		//vcollide_t *pCollide = modelinfo->GetVCollide(this->GetModelIndex());
		//pCollide->S
		//UTIL_CreateScaledPhysObject(pShortcut->GetBaseAnimating(), fScale);
	}

	//this->ConfirmNotError();

	m_bInitialized = true;
}

void C_PropShortcutEntity::ConfirmNotError()
{
	// Check if the model's material is an error
	const model_t* TheModel = this->GetModel();
	if (TheModel)
	{
		std::string modelName = modelinfo->GetModelName(TheModel);
		//DevMsg("Checking %s\n", modelName.c_str());
		if( m_oldModel != modelName)
		{
			/*
			IMaterial* pMaterial;
			modelinfo->GetModelMaterials(TheModel, 1, &pMaterial);

			//bool bIsMultiplayer = false;
			//bool bIsMultiplayerHost = false;
			if (!Q_stricmp(pMaterial->GetName(), "models/error/new light1"))// && (!bIsMultiplayer || bIsMultiplayerHost == -1))
			{
				DevMsg("WARNING: The following model failed to load. Reverting to placeholder: %s\n", modelName.c_str());

				//std::string modelId = g_pAnarchyManager->GetMetaverseManager()->ResolveLegacyModel("models/icons/missing.mdl");
				//std::string modelId = g_pAnarchyManager->GetMetaverseManager()->GetMissingModelId();
				//if (modelId != "")
				engine->ServerCmd(VarArgs("switchmodel \"%s\" \"models/icons/missing.mdl\" %i;\n", m_modelId, this->entindex()));// modelId.c_str(), this->entindex()));

				g_pAnarchyManager->GetMetaverseManager()->RequestAsset(modelName);
			}
			*/
			//DevMsg("Model Name: %s\n", modelName.c_str());//modelFile
			//object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(m_model)


			if (modelName == "models/icons/missing.mdl")
			{
				if (!m_pCloudAssetsDownload)
					m_pCloudAssetsDownload = cvar->FindVar("cloud_assets_download");

				if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && !g_pAnarchyManager->GetConnectedUniverse()->isHost && m_pCloudAssetsDownload->GetBool())
				{
					this->SetRenderMode(kRenderNone, true);
					KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(m_modelId));
					if (pModelKV)
						g_pAnarchyManager->GetMetaverseManager()->RequestAsset(pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
				}
			}
			else if (m_oldModel == "models/icons/missing.mdl")// && g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && !g_pAnarchyManager->GetConnectedUniverse()->isHost)
			{
				object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(m_objectId);
				if (pObject)
				{
					int iSequence = this->LookupSequence(pObject->anim.c_str());
					if (iSequence != ACT_INVALID)
						this->PlaySequenceRegular(pObject->anim.c_str());
				}
			}

			m_oldModel = modelName;
		}
	}
}


void C_PropShortcutEntity::PlaySequenceRegular(const char* sequenceTitle, bool bHandleModelChange)
{
	if (!m_pSequenceBlacklistConVar)
		m_pSequenceBlacklistConVar = cvar->FindVar("sequence_blacklist");
	std::string sequenceBlacklist = m_pSequenceBlacklistConVar->GetString();
	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(sequenceBlacklist, tokens, " ");
	unsigned int size = tokens.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (tokens[i] == sequenceTitle)
			return;
	}

	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");
	//if (pMPModeVar->GetBool())
	//	return;

	std::string realSequenceTitle = sequenceTitle;

	int index = this->LookupSequence(realSequenceTitle.c_str());

	if (index == ACT_INVALID)
	{
		if (!Q_strcmp(sequenceTitle, "activated"))
		{
			realSequenceTitle = "activeidle";
			index = this->LookupSequence(realSequenceTitle.c_str());
		}
		else if (!Q_strcmp(sequenceTitle, "deactivated"))
		{
			realSequenceTitle = "inactiveidle";
			index = this->LookupSequence(realSequenceTitle.c_str());
		}
	}

	if (index != ACT_INVALID)
	{
		//		DevMsg("Playing sequence %s on the client.\n", sequenceTitle);

		//		this->SetModelScale(2.0);	// WORKING WORKING WORKING ITEM SCALING!  JUST LEAVE LAST PARAM AS ZERO!
		//		DevMsg("here sequenceis: %i with index %i\n", this->GetSequence(), index);

		if (this->GetSequence() > 0 || bHandleModelChange)
			this->SetSequence(index);

		this->SetCycle(0.0f);

		engine->ClientCmd(VarArgs("playsequence %i \"%s\";\n", this->entindex(), realSequenceTitle.c_str()));	// servercmdfix , false);
		//engine->ServerCmd(VarArgs("playsequence %i \"%s\";\n", this->entindex(), realSequenceTitle.c_str()), false);
		//engine->ServerCmd(VarArgs("playsequence %i \"%s\";\n", this->entindex(), realSequenceTitle.c_str()), true);
	}
	else
	{
		DevMsg("NOTE: Setting animation back to default is not yet supported! :S\n");
	}
}

void C_PropShortcutEntity::OnPreDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnPreDataChanged(updateType);
}

/*
	inline Vector LocalToWorld(const Vector &vVec) const
	{
		return VMul4x3(vVec);
	}

	inline Vector WorldToLocal(const Vector &vVec) const
	{
		return VMul4x3Transpose(vVec);
	}

	inline Vector LocalToWorldRotation(const Vector &vVec) const
	{
		return VMul3x3(vVec);
	}

	inline Vector WorldToLocalRotation(const Vector &vVec) const
	{
		return VMul3x3Transpose(vVec);
	}
	*/

void C_PropShortcutEntity::OnDataChanged(DataUpdateType_t updateType)
{
	//DevMsg("PreDataChangedUpdate: ")//DATA_UPDATE_DATATABLE_CHANGED
	if (!m_bInitialized )
		this->Initialize();
	else
	{
		// do nada
	}

	//this->ConfirmNotError();	// (semi)EXPENSIVE to do this EVERY data change!! (cuz it loops through model materials looking for error mat, but it actually only checks the 1st material)

	if (!m_bAlreadySetObjectEntity && Q_strcmp(m_objectId, ""))
	{
		// associated this entity with this object
		g_pAnarchyManager->GetInstanceManager()->SetObjectEntity(std::string(m_objectId), this);
		m_bAlreadySetObjectEntity = true;

		// handle nodes
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(m_objectId);
		if (pObject)
		{
			KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pObject->itemId));
			if (pItemKV)
			{
				if (!Q_strcmp(pItemKV->GetString("info/id"), ""))
				{
					DevMsg("Skipping item with no ID.\n");
				}
				else
				{
					//DevMsg("tester file field: %s\n", pItemKV->GetString("file"));
					std::string fileValue = pItemKV->GetString("file");
					if (fileValue.find_first_of("/\\") == std::string::npos && fileValue.find(".") == std::string::npos)
					{
						// is this the node item we're expecting?
						std::string incomingNodeId = g_pAnarchyManager->GetInstanceManager()->GetIncomingNodeId();
						if (incomingNodeId != "" && !Q_strcmp(incomingNodeId.c_str(), pItemKV->GetString("info/id")))
						{
							// we are CREATING a node instance
							// ================================

							// (this should actually all be done on the SERVER side. for correct parenting.) (???? maybe not anymore.)
							// TODO: work (maybe?)

							// attach all entities listed in the nodevolume to us.
							// TODO: work

							KeyValues* pNodeInfoKV = new KeyValues("node");
							if (!pNodeInfoKV->LoadFromFile(g_pFullFileSystem, "nodevolume.txt", "DEFAULT_WRITE_PATH"))
							{
								DevMsg("ERROR: Could not load nodevolume.txt!\n");
							}
							else
							{
								//DevMsg("Node Title: %s\n", pItemKV->GetString("title"));
								
								// create the actual node instance
								KeyValues* pNodeInstanceKV = new KeyValues("instance");
								std::string nodeStyle = "node_" + std::string(pNodeInfoKV->GetString("setup/style"));
								std::string nodeInstanceId = g_pAnarchyManager->GetInstanceManager()->CreateBlankInstance(0, pNodeInstanceKV, pItemKV->GetString("file"), "", pItemKV->GetString("title"), "", "", "", nodeStyle, null);
								//instance_t* pNodeInstance = g_pAnarchyManager->GetInstanceManager()->GetInstance(nodeInstanceId);
								



								//DevMsg("Node ID: %s\n", nodeInstanceId.c_str());
								object_t* pInstanceObject;

								VMatrix childMatrix;
								VMatrix parentMatrix;
								parentMatrix.SetupMatrixOrgAngles(this->GetAbsOrigin(), this->GetAbsAngles());
								VMatrix parentMatrixInverse = parentMatrix.InverseTR();
								
								Vector dummy;
								Vector childOrigin;
								Vector childAnglesRaw;
								QAngle childAngles;

								KeyValues* pObjectKV;
								C_BaseEntity* pBaseEntity;
								C_PropShortcutEntity* pPropShortcutEntity;
								object_t* pObject;
								KeyValues* pObjectInfo;
								KeyValues* pItemInfo;
								KeyValues* pModelInfo;
								KeyValues* pNodeObjectsKV = pNodeInfoKV->FindKey("setup/objects", true);
								int arrayIndex = 0;

								VMatrix composedMatrix;
								float flParentScale = this->GetModelScale();

								for (KeyValues *sub = pNodeObjectsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
								{
									// loop through it adding all to the instance KV
									pBaseEntity = C_BaseEntity::Instance(sub->GetInt());
									if (!pBaseEntity)
										continue;

									pPropShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
									if (!pPropShortcutEntity)
										continue;

									// is this a node itself?  prevent node-ception.
									KeyValues* pSubItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("items", pPropShortcutEntity->GetItemId()));
									if (pSubItemKV)
									{
										KeyValues* pSubTypeKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("types", pSubItemKV->GetString("type")));
										if (pSubTypeKV && !Q_stricmp(pSubTypeKV->GetString("title"), "node"))
											continue;
									}

									/*
									childOrigin = pPropShortcutEntity->GetAbsOrigin();
									childAngles = pPropShortcutEntity->GetAbsAngles();

									childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
									childMatrix = childMatrix * parentMatrixInverse;

									// Finally convert back to origin+angles.
									childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
									MatrixAngles(childMatrix.As3x4(), childAngles, dummy);
									*/

									pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pPropShortcutEntity->GetObjectId());

									childOrigin = pObject->origin;// pPropShortcutEntity->GetAbsOrigin();
									childAngles = TransformAnglesToLocalSpace(pObject->angles, parentMatrix.As3x4()); //pObject->angles;// pPropShortcutEntity->GetAbsAngles();

									childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
									childMatrix = childMatrix * parentMatrixInverse;

									// Finally convert back to origin+angles.
									childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
									//MatrixAngles(childMatrix.As3x4(), childAngles, dummy);



									/*
									// At this point, childOrigin and childAngles should hold valid values.
									// However, there is an orientation bug that causes certain objects to be incorrectly oriented afer being restored.
									// This bug is reversable - as saving the node again in this errored state corrects the errors.
									// So the code below simulates doing the math twice, in order to avoid ever saving the error.

									// Reapply to induce orientation errors.
									childMatrix.SetupMatrixOrgAngles(childOrigin * flParentScale, childAngles);
									composedMatrix.SetupMatrixOrgAngles(this->GetAbsOrigin(), this->GetAbsAngles());
									composedMatrix = composedMatrix * childMatrix;
									MatrixAngles(composedMatrix.As3x4(), childAngles, childOrigin);

									// And reapply a 3rd time to correct those orientation errors.
									childMatrix.SetupMatrixOrgAngles(childOrigin, childAngles);
									childMatrix = childMatrix * parentMatrixInverse;
									childOrigin = parentMatrix.VMul4x3Transpose(childOrigin);
									MatrixToAngles(childMatrix.As3x4(), childAngles);
									*/



									//pObjectKV = new KeyValues("object");
									pObjectKV = pNodeInstanceKV->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()), true);
									//std::string sequenceName = pObject->anim;// //pPropShortcutEntity->GetSequenceName(pPropShortcutEntity->GetSequence())
									std::string childOriginString = VarArgs("%f %f %f", childOrigin.x, childOrigin.y, childOrigin.z);
									std::string childAnglesString = VarArgs("%f %f %f", childAngles.x, childAngles.y, childAngles.z);
									float flChildScale = pPropShortcutEntity->GetModelScale();
									//int iIsChild = true;// (pPropShortcutEntity->GetSlave()) ? 1 : 0;
									int iIsSlave = (pPropShortcutEntity->GetSlave()) ? 1 : 0;
									//pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pPropShortcutEntity->GetObjectId());

									g_pAnarchyManager->GetInstanceManager()->CreateObject(pObjectKV, pPropShortcutEntity->GetObjectId(), pPropShortcutEntity->GetItemId(), pPropShortcutEntity->GetModelId(), childOriginString, childAnglesString, flChildScale, pObject->anim, iIsSlave, true, pObject->body, pObject->skin);

									// create an object in the KV
									/*KeyValues* objectKV = pNodeInstanceKV->FindKey(VarArgs("objects/%s", pObject->objectId.c_str()), true);
									objectKV->SetString("local/info/id", pObject->objectId.c_str());
									objectKV->SetString("local/item", pObject->itemId.c_str());
									objectKV->SetString("local/model", pObject->modelId.c_str());
									objectKV->SetString("local/position", sub->GetString("origin", "0 0 0"));
									objectKV->SetString("local/rotation", sub->GetString("angles", "0 0 0"));
									objectKV->SetFloat("local/scale", pObject->scale);
									objectKV->SetString("local/anim", sequenceName.c_str());
									objectKV->SetInt("local/slave", pObject->slave);
									objectKV->SetInt("local/child", pObject->child);*/

									// flag it as a child object in the active instance too, & save the instance.  Cuz child objects do NOT spawn automatically, their nodes control their spawning.
									pInstanceObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pPropShortcutEntity->GetObjectId());
									pObjectKV = g_pAnarchyManager->GetInstanceManager()->GetCurrentInstanceKV()->FindKey(VarArgs("objects/%s", pPropShortcutEntity->GetObjectId().c_str()));
									if ( pObjectKV )
										pObjectKV->SetInt("local/child", 1);

									pInstanceObject->parentEntityIndex = this->entindex();
									pInstanceObject->child = true;


									//pPropShortcutEntity->SetParent(this);
									//pPropShortcutEntity->SetAbsOrigin(childOrigin);
									//pPropShortcutEntity->SetAbsAngles(childAngles);
									//g_pAnarchyManager->GetInstanceManager()->ApplyChanges(pPropShortcutEntity);
									engine->ClientCmd(VarArgs("setparent %i %i\n", pPropShortcutEntity->entindex(), this->entindex()));
								}

								//sqlite3** pDb = (pBackpack && pBackpack->GetSQLDbPointer()) ? pBackpack->GetSQLDbPointer() : null;
								sqlite3** pDb = null;
								g_pAnarchyManager->GetMetaverseManager()->SaveSQL(pDb, "instances", nodeInstanceId.c_str(), pNodeInstanceKV);
								pNodeInstanceKV->deleteThis();
								pNodeInstanceKV = null;

								g_pAnarchyManager->GetInstanceManager()->SaveActiveInstance(); // to save our flagging of objects as child
							}

							/*std::string instanceId = pInstanceKV->GetString("info/local/id");

							//DevMsg("Saving instance ID %s\n", instanceId.c_str());
							// THIS is where you'd put code for some sort of save indicator icon in the HUD.

							// now save out to the SQL
							g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", instanceId.c_str(), pInstanceKV, false, bForceNoSave);
							*/

							// create a node instance using the RELATIVE origin & angles of all children
							// TODO: work

							// give it the correct nodestyle too!
							// TODO: work

							// save it with out w/ the correct Id
							// TODO: work

							// now continue loading it & adding all children objects like normal
							// TODO: work

							// finally, make sure they all spawn right away.
							// TODO: work

							// reset the incoming node id
							g_pAnarchyManager->GetInstanceManager()->SetIncomingNodeId("");
							//engine->ServerCmd(VarArgs("showhubsavemenu %i;", this->entindex()));	// DISABLED FOR NOW
						}
						else if(!pObject->child)// if (this->GetMoveParent() == this)
						{
							// potentially a node instance ID
							instance_t* pNodeInstance = g_pAnarchyManager->GetInstanceManager()->FindInstance(fileValue);
							if (pNodeInstance)
							{
								//DevMsg("Bengo! (However, node spawning is disabled for right now.)\n");	// From here, all the objects get added (knowing their parent entity index), but still use the regular arse object spawning system...
								g_pAnarchyManager->GetInstanceManager()->LoadInstance(this, fileValue);
								//g_pAnarchyManager->GetInstanceManager()->SpawnNearestObject();
								//g_pAnarchyManager->GetInstanceManager()->SpawnNearestObject();
								// spawn all the objects too, if needed...
								// FIXME: do it.
								// TODO: work
							}
						}
					}
					else if (fileValue.find("http://text.txt/?") == 0 && (fileValue.find("&m=") != std::string::npos || fileValue.find("?m=") != std::string::npos))
					{
						// it is a text item
						size_t foundAt = fileValue.find("?txt=");
						if (foundAt == std::string::npos)
							foundAt = fileValue.find("&txt=");
						if (foundAt != std::string::npos)
						{
							std::string paramValue = fileValue.substr(foundAt + 5);
							foundAt = paramValue.find_first_of("&#");
							if (foundAt != std::string::npos)
								paramValue = paramValue.substr(0, foundAt);

							if (paramValue != "")
							{
								std::transform(paramValue.begin(), paramValue.end(), paramValue.begin(), ::toupper);
								//DevMsg("ParamValue: %s\n", paramValue.c_str());

								engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", this->entindex(), paramValue.c_str()));
							}
						}
					}
				}
			}
		}

		if (pObject->anim != "" && pObject->anim != "Idle" && pObject->anim != "idle" && pObject->anim != "ragdoll")
		{
			//DevMsg("Anim: %s\n", pObject->anim.c_str());
			int iSequence = this->LookupSequence(pObject->anim.c_str());
			if (iSequence != ACT_INVALID)
				this->PlaySequenceRegular(pObject->anim.c_str());
		}

		if (g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance() && !Q_stricmp(g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance()->autoplayId.c_str(), m_objectId) && cvar->FindVar("autoplay_enabled")->GetBool())
			g_pAnarchyManager->QuickRemember(this->entindex());
	}

	this->ConfirmNotError();	// (semi)EXPENSIVE to do this EVERY data change!! (cuz it loops through model materials looking for error mat, but it actually only checks the 1st material)

	BaseClass::OnDataChanged(updateType);
}

void C_PropShortcutEntity::Release()
{
	//g_pAnarchyManager->ClearLookAtObjects();
	g_pAnarchyManager->RemoveAlwaysLookObject(this);
	BaseClass::Release();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the fade scale of the entity in question
// Output : unsigned char - 0 - 255 alpha value
//-----------------------------------------------------------------------------
unsigned char C_PropShortcutEntity::GetClientSideFade(void)
{
	// disabled the "spawning object" check because what happens in server-code overrode it.
	//if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity() == this || (this->GetMoveParent() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity() == this->GetMoveParent()))
	//	return UTIL_ComputeEntityFade(this, m_fadeMinDist, m_fadeMaxDist, m_flFadeScale);
	//else
	//{
		if (!m_pFadeDistMinConVar)
		{
			m_pFadeDistMinConVar = cvar->FindVar("aapropfademin");
			m_pFadeDistMaxConVar = cvar->FindVar("aapropfademax");
		}

		return UTIL_ComputeEntityFade(this, m_pFadeDistMinConVar->GetFloat(), m_pFadeDistMaxConVar->GetFloat(), m_flFadeScale);
	//}
}

void C_PropShortcutEntity::Precache(void)
{
	PrecacheModel("models\\cabinets\\two_player_arcade.mdl");	// FIXME: Probably can precache the correct model here.
	BaseClass::Precache();
}

/*
int C_PropShortcutEntity::DrawModel(int flags)
{
	if (!m_pNoDrawShortcutsConVar)
		m_pNoDrawShortcutsConVar = cvar->FindVar("nodraw_shortcuts");

	if (m_pNoDrawShortcutsConVar->GetBool())
		return false;

	return BaseClass::DrawModel(flags);
}
*/

/*
bool C_PropShortcutEntity::ShouldDraw()
{
	if (!m_pNoDrawShortcutsConVar)
		m_pNoDrawShortcutsConVar = cvar->FindVar("nodraw_shortcuts");

	if (m_pNoDrawShortcutsConVar->GetBool())
		return false;
	
	return BaseClass::ShouldDraw();
}
*/

void C_PropShortcutEntity::SetDrawForeground(bool bValue)
{
	m_bDrawForeground = bValue;
}

int C_PropShortcutEntity::DrawModel(int flags)	// Added for Anarchy Arcade
{
	//if ((g_pAnarchyManager->GetNoDrawShortcutsValue() == 1 || (g_pAnarchyManager->GetNoDrawShortcutsValue() == 2 && g_pAnarchyManager->UseSBSRendering() && g_pAnarchyManager->GetEye() != ISourceVirtualReality::VREye::VREye_Left)))
	//	return 0;

	if (m_bDrawForeground)
	{
		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->DepthRange(0.0f, 0.1f);
		int result = BaseClass::DrawModel(flags);

		//float depthmin = 0.0f;
		//float depthmax = 1.0f;
		pRenderContext->DepthRange(0.0f, 1.0f);
		return result;

		/*
		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->OverrideDepthEnable(true, true);

		// Call the original DrawModel function
		int result = BaseClass::DrawModel(flags);

		// Restore the old depth range
		pRenderContext->OverrideDepthEnable(false, false);
		//pRenderContext->PopRenderTargetAndViewport();
		return result;
		*/

		// get the override material (if needed)
		/*
		if (!m_pForegroundMaterial)
		{
			//m_pForegroundMaterial = materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true);
			//KeyValues* pMaterialKV = new KeyValues("UnlitGeneric");
			//pMaterialKV->SetString("$basetexture", "vgui/canvas");
			//IMaterial* pMaterial = materials->CreateMaterial("projectorview", pMaterialKV);
			//pMaterial->Refresh();
			//DevMsg("Projector Material Name: %s\n", pMaterial->GetName());
			//m_pForegroundMaterial = pMaterial;
		}

		// override
		if (m_pForegroundMaterial)
		{
			g_pStudioRender->ForcedMaterialOverride(m_pForegroundMaterial);
		}

		int result = BaseClass::DrawModel(flags);
		g_pStudioRender->ForcedMaterialOverride(NULL);
		//pRenderContext->PopRenderTargetAndViewport();
		return result;
		*/

		/*C_BaseEntity *pAttachment = this->FirstMoveChild();
		while (pAttachment != NULL)
		{
			if (pAttachment->ShouldDraw())
			{
				pAttachment->DrawModel(flags);
			}
			pAttachment = pAttachment->NextMovePeer();
		}*/
	}
	else
	{
		// Call the original DrawModel function
		return BaseClass::DrawModel(flags);
	}
}


void C_PropShortcutEntity::OnUsed()
{
	g_pAnarchyManager->SetSelectOriginal(this->entindex());

	/*
	KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(this->GetItemId()));
	if (pItemKV)
	{
		std::string file = pItemKV->GetString("file");
		size_t found = file.find("travel.html?");
		if (found == 0)
		{
			KeyValues* pParams = new KeyValues("params");
			std::string params = file.substr(found+12);

			//std::transform(mapfile.begin(), mapfile.end(), mapfile.begin(), ::tolower);

			std::vector<std::string> pairs;
			g_pAnarchyManager->Tokenize(params, pairs, "&");

			std::vector<std::string> token;
			std::string pair;
			for (unsigned int i = 0; i < pairs.size(); i++)
			{
				token.clear();
				pair = pairs[i];
				g_pAnarchyManager->Tokenize(pair, token, "=");

				if (token.size() == 2)
					pParams->SetString(token[0].c_str(), token[1].c_str());
			}

			std::string screenshot = pParams->GetString("screenshot");
			std::string lobby = pParams->GetString("lobby");
			std::string lobbytitle = pParams->GetString("lobbytitle");
			std::string instance = pParams->GetString("instance");
			std::string mapfile = pParams->GetString("mapfile");
			std::string pos = pParams->GetString("pos");
			std::string rot = pParams->GetString("rot");
			pParams->deleteThis();

			g_pAnarchyManager->MapTransition(mapfile, "", screenshot, lobby, lobbytitle, instance, pos, rot);
		}
	}
	*/
}

void C_PropShortcutEntity::Spawn()
{
	m_bInitialized = false;
	Precache();
	//SetModel("models\\cabinets\\two_player_arcade.mdl");// VarArgs("%s", this->GetModelName()));
	//if (engine->IsInGame() && Q_strcmp(g_pAnarchyManager->MapName(), ""))

	object_t* pSpawningObject = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject();//!pSpawningObject ||
	if (!pSpawningObject || g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity() != null)	// FIXME: Simply checking if the local user is spawning an entity is NOT enough to determine if THIS entity is the one he is spawning...
		this->Initialize();

	//	DevMsg("SPAWNING!!\n");
	/*
	object_t* pSpawningObject = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject();
	if (false && pSpawningObject->origin.DistTo(this->GetAbsOrigin()) < 1.0 )// GEEZ! not even position is set yet.  defer //&& !Q_strcmp(pSpawningObject->itemId.c_str(), m_itemId) )	// FIXME: accounts for rounding errors, but can produce false-positives!! // FIXME2: m_itemId isn't set until the 1st data update, der.  so this check is probably very unqualified.  too many false positive posibilities.
	{
		// THIS IS THE OBJECT THE CURRENT USER IS SPAWNING!!
		Precache();
		SetModel("models\\cabinets\\two_player_arcade.mdl");// VarArgs("%s", this->GetModelName()));
		SetSolid(SOLID_NONE);
		SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		SetRenderMode(kRenderTransTexture);
		g_pAnarchyManager->ActivateObjectPlacementMode(this);
		BaseClass::Spawn();
	}
	else
	{
		// This is a regular object that already existed or somebody else spawned
		Precache();
		SetModel("models\\cabinets\\two_player_arcade.mdl");// VarArgs("%s", this->GetModelName()));
		//SetSolid(SOLID_NONE);
		SetSolid(SOLID_VPHYSICS);
		SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
		SetMoveType(MOVETYPE_VPHYSICS);

		if (CreateVPhysics())
		{
			IPhysicsObject *pPhysics = this->VPhysicsGetObject();
			if (pPhysics)
			{
				pPhysics->EnableMotion(false);
			}
		}
	}
	*/
	//	SetRenderMode(kRenderNormal);
	//SetUse(&CPropHotlinkEntity::UseFunc);

	//this->CollisionProp()->SetSolid(SOLID_VPHYSICS);
	//this->CollisionProp()->SetSurroundingBoundsType(USE_BEST_COLLISION_BOUNDS);
	//this->CollisionProp()->SetSize(-Vector(100, 100, 100), Vector(100, 100, 100));
	//this->RefreshCollisionBounds();
	BaseClass::Spawn();
}

std::string C_PropShortcutEntity::GetItemId()
{
	return std::string(m_itemId);
}

std::string C_PropShortcutEntity::GetObjectId()
{
	return std::string(m_objectId);
}

std::string C_PropShortcutEntity::GetModelId()
{
	return std::string(m_modelId);
}

bool C_PropShortcutEntity::GetSlave()
{
	//DevMsg("Returning %i\n", m_bSlave);
	return m_bSlave;
}

bool C_PropShortcutEntity::TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr)
{
	if (ray.m_IsRay && IsSolidFlagSet(FSOLID_CUSTOMRAYTEST))
	{
		if (!TestHitboxes(ray, fContentsMask, tr))
			return true;

		return tr.DidHit();
	}

	if (!ray.m_IsRay && IsSolidFlagSet(FSOLID_CUSTOMBOXTEST))
	{
		if (!TestHitboxes(ray, fContentsMask, tr))
			return true;

		return true;
	}

	// We shouldn't get here.
	Assert(0);
	return false;
}

void C_PropShortcutEntity::GetEmbeddedInstances(std::vector<C_EmbeddedInstance*>& embeddedInstances)
{
	// DETECT DYNAMIC TEXTURES
	const model_t* model = this->GetModel();

	IMaterial* pMaterials[32];
	for (int x = 0; x < 32; x++)
		pMaterials[x] = NULL;

	modelinfo->GetModelMaterials(model, 32, &pMaterials[0]);

	//auto it;
	std::vector<C_EmbeddedInstance*>::iterator it;
	IMaterial* pMaterial;
	C_EmbeddedInstance* pEmbeddedInstance;
	for (int x = 0; x < 32; x++)
	{
		if (pMaterials[x] && pMaterials[x]->HasProxy())
		{
			pMaterial = pMaterials[x];
			pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(pMaterial);

			if (pEmbeddedInstance)
			{
				// make sure it isn't already on the list
				it = std::find(embeddedInstances.begin(), embeddedInstances.end(), pEmbeddedInstance);
				if (it == embeddedInstances.end())
					embeddedInstances.push_back(pEmbeddedInstance);
			}
		}
		//	Material: vgui/websurfacealt2
		//	Material: vgui/websurfacealt
		//	Material: vgui/websurfacealt5
		//	Material: vgui/websurfacealt5
		//	Material: vgui/websurfacealt7
	}
}