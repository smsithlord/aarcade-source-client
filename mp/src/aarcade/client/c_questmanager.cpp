#include "cbase.h"
#include "c_anarchymanager.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_QuestManager::C_QuestManager()
{
	DevMsg("C_QuestManager: Constructor\n");
	m_bQuestsInitlaizing = false;
}

C_QuestManager::~C_QuestManager()
{
	DevMsg("C_QuestManager: Destructor\n");
}

void C_QuestManager::Init()
{
	DevMsg("C_QuestManager: Init\n");
}

void C_QuestManager::Update()
{
	//DevMsg("C_QuestManager: Update\n");

	// Find all quests that have un-collected collectibles AND use the TOUCH collect method.
	bool bAlreadyCollected;
	std::string collectibleObjectId;
	unsigned int uNumCollectibleObjects;
	object_t* pCollectibleObject;

	C_BasePlayer* pPlayer = null;
	Vector playerOrigin;

	quest_t* pQuest;
	auto it = m_quests.begin();
	while (it != m_quests.end())
	{
		pQuest = it->second;
		if (pQuest->eStatus == QUEST_STATUS_ACTIVE && pQuest->eCollectMethod == QUEST_COLLECT_METHOD_TOUCH)	// NOTE: This should really only happen if the quest has collectOnce also, to avoid touch spam.
		{
			if (!pPlayer)
			{
				pPlayer = C_BasePlayer::GetLocalPlayer();
				playerOrigin = pPlayer->GetAbsOrigin();
			}

			// check if there are un-collected collectibles
			uNumCollectibleObjects = pQuest->collectibleObjects.size();
			if (pQuest->collectedObjects.size() < uNumCollectibleObjects && uNumCollectibleObjects > 0)
			{
				// See if we are within range of any of these un-collected collectibles.
				for (unsigned int i = 0; i < uNumCollectibleObjects; i++)
				{
					collectibleObjectId = pQuest->collectibleObjects[i];

					// make sure we are not already collected.
					bAlreadyCollected = false;
					for (unsigned int j = 0; j < pQuest->collectedObjects.size(); j++)
					{
						if (pQuest->collectedObjects[j] == collectibleObjectId)
						{
							bAlreadyCollected = true;
							break;
						}
					}

					if (bAlreadyCollected)
						continue;

					// we are an un-collected collectible.
					// Check if the object is available still.
					pCollectibleObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(collectibleObjectId);

					if (!pCollectibleObject)
					{
						//DevMsg("ERROR: Cannot get collectible object that should exist!\n");
						continue;
					}

					// Check the distance of the object to the player.
					if (pCollectibleObject->entityIndex >= 0 && playerOrigin.DistTo(pCollectibleObject->origin) < pQuest->flCollectRadius)
					{
						// collect the object for the quest.

						// hide the object, only if we are supposed to.
						//this->HideObjectForQuest(pQuest->id, collectibleObjectId);

						// collect it
						//this->QuestCollect(questId, objectId);
						this->CollectObjectForQuest(pQuest->id, collectibleObjectId);
						//break;	// only collect one at a time per quest
						return;
					}
				}
			}
		}

		it++;
	}
}

/*void C_QuestManager::CollectObjectForQuest(std::string questId, std::string objectId)
{
	QueuedQuestAction_t* pQuestAction = new QueuedQuestAction_t();
	pQuestAction->action = "collect";
	pQuestAction->objectId = objectId;
	pQuestAction->questId = questId;
	m_queuedQuestActions.push_back(pQuestAction);
}*/

void C_QuestManager::CollectObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	if (pQuest->bCollectOnce)
		pQuest->collectedObjects.push_back(objectId);

	std::string soundFx = pQuest->collectionSound;
	if( soundFx != "" )
		g_pAnarchyManager->PlaySound(soundFx);

	// does this quest have a collect handler?
	questEvent_t* pCollectQuestEvent = this->GetQuestEvent(pQuest->collectQuestEventId);
	if (pCollectQuestEvent)
	{
		// This quest DOES have a handler for collect.  Utilize its attributes.
		this->QuestCollect(questId, objectId);
	}

	// If there is nothing left to collect, we have won.
	if (pQuest->collectedObjects.size() == pQuest->collectibleObjects.size())
		this->QuestSuccess(questId);
}

void C_QuestManager::HideObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	// Make sure we are not already hidden.
	for (unsigned int i = 0; i < pQuest->hiddenObjects.size(); i++)
	{
		if (pQuest->hiddenObjects[i] == objectId)
			return;
	}

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}
	pQuest->hiddenObjects.push_back(objectId);

	// add us to the action queue
	QueuedQuestAction_t* pQuestAction = new QueuedQuestAction_t();
	pQuestAction->action = "hide";
	pQuestAction->objectId = objectId;
	pQuestAction->questId = questId;
	m_queuedQuestActions.push(pQuestAction);
}

void C_QuestManager::DoHideObjectForQuest(std::string questId, std::string objectId)
{
	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	// we have an object, and we really want to hide it now.
	// "quest hide" means make non-collide & invisible.
	engine->ClientCmd(VarArgs("quest_hide_entity %i;", pObject->entityIndex));

	C_BaseEntity* pEntity = C_BaseEntity::Instance(pObject->entityIndex);
	if (pEntity)
	{
		pEntity->SetRenderMode(kRenderNone);
		pEntity->SetMoveType(MOVETYPE_NOCLIP);
		pEntity->SetSolid(SOLID_NONE);
	}
}

void C_QuestManager::UnhideObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	// Make sure we are already hidden.
	bool bFoundSelf = false;
	unsigned int uSelfIndex = -1;	// save this for later so we can easily remove ourselves from the hidden objects vector.
	for (unsigned int i = 0; i < pQuest->hiddenObjects.size(); i++)
	{
		if (pQuest->hiddenObjects[i] == objectId)
		{
			bFoundSelf = true;
			uSelfIndex = i;
			break;
		}
	}

	//if (!bFoundSelf)
	//return;

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	if (bFoundSelf)
		pQuest->hiddenObjects.erase(pQuest->hiddenObjects.begin() + uSelfIndex);

	QueuedQuestAction_t* pQuestAction = new QueuedQuestAction_t();
	pQuestAction->action = "unhide";
	pQuestAction->objectId = objectId;
	pQuestAction->questId = questId;
	m_queuedQuestActions.push(pQuestAction);
}

void C_QuestManager::DoUnhideObjectForQuest(std::string questId, std::string objectId)
{
	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	// we have an object, and we really want to hide it now.
	// "quest hide" means make non-collide & invisible.
	engine->ClientCmd(VarArgs("quest_unhide_entity %i;", pObject->entityIndex));

	C_BaseEntity* pEntity = C_BaseEntity::Instance(pObject->entityIndex);
	if (pEntity)
	{
		pEntity->SetRenderMode(kRenderNormal);
		pEntity->SetMoveType(MOVETYPE_NONE);
		pEntity->SetSolid(SOLID_VPHYSICS);
	}
}

void C_QuestManager::MarkCollectibleObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	// Make sure we are not already makred as collectible.
	/*
	bool bFoundSelf = false;
	for (unsigned int i = 0; i < pQuest->collectibleObjects.size(); i++)
	{
	if (pQuest->collectibleObjects[i] == objectId)
	{
	bFoundSelf = true;
	break;
	}
	}

	if (bFoundSelf)
	return;
	*/

	// Make sure we are already hidden.
	bool bFoundSelf = false;
	unsigned int uSelfIndex = -1;	// save this for later so we can easily remove ourselves from the hidden objects vector.
	for (unsigned int i = 0; i < pQuest->hiddenObjects.size(); i++)
	{
		if (pQuest->hiddenObjects[i] == objectId)
		{
			bFoundSelf = true;
			uSelfIndex = i;
			break;
		}
	}

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	if (bFoundSelf)
		pQuest->hiddenObjects.erase(pQuest->hiddenObjects.begin() + uSelfIndex);

	QueuedQuestAction_t* pQuestAction = new QueuedQuestAction_t();
	pQuestAction->action = "mark";
	pQuestAction->objectId = objectId;
	pQuestAction->questId = questId;
	m_queuedQuestActions.push(pQuestAction);
}

void C_QuestManager::DoMarkCollectibleObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	// we have an object, and we really want to hide it now.
	// "quest hide" means make non-collide & invisible.
	int iShouldSpin = (pQuest->bCollectiblesSpin) ? 1 : 0;
	int iCollectiblesCollide = (pQuest->bCollectiblesCollide) ? 1 : 0;
	engine->ClientCmd(VarArgs("quest_mark_collectible_entity %i %i %.10g %.10g %.10g %i;", pObject->entityIndex, iShouldSpin, pObject->angles.x, pObject->angles.y, pObject->angles.z, iCollectiblesCollide));

	C_BaseEntity* pEntity = C_BaseEntity::Instance(pObject->entityIndex);
	if (pEntity)
	{
		pEntity->SetRenderMode(kRenderNormal);
		pEntity->SetMoveType(MOVETYPE_NOCLIP);

		if ( pQuest->bCollectiblesCollide )
			pEntity->SetSolid(SOLID_VPHYSICS);
		else
			pEntity->SetSolid(SOLID_NONE);
	}
}

void C_QuestManager::UnmarkCollectibleObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	// Make sure we are already makred as collectible.
	bool bFoundSelf = false;
	for (unsigned int i = 0; i < pQuest->collectibleObjects.size(); i++)
	{
		if (pQuest->collectibleObjects[i] == objectId)
		{
			bFoundSelf = true;
			break;
		}
	}

	if (!bFoundSelf)
		return;

	// Check if we are already hidden.
	bFoundSelf = false;
	unsigned int uSelfIndex = -1;	// save this for later so we can easily remove ourselves from the hidden objects vector.
	for (unsigned int i = 0; i < pQuest->hiddenObjects.size(); i++)
	{
		if (pQuest->hiddenObjects[i] == objectId)
		{
			bFoundSelf = true;
			uSelfIndex = i;
			break;
		}
	}

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	if (bFoundSelf)
		pQuest->hiddenObjects.erase(pQuest->hiddenObjects.begin() + uSelfIndex);

	// If we are already collected, we want to remove ourselves from there too.
	for (unsigned int i = 0; i < pQuest->collectedObjects.size(); i++)
	{
		if (pQuest->collectedObjects[i] == objectId)
		{
			pQuest->collectedObjects.erase(pQuest->collectedObjects.begin() + i);
			break;
		}
	}

	QueuedQuestAction_t* pQuestAction = new QueuedQuestAction_t();
	pQuestAction->action = "unmark";
	pQuestAction->objectId = objectId;
	pQuestAction->questId = questId;
	m_queuedQuestActions.push(pQuestAction);
}

void C_QuestManager::DoUnmarkCollectibleObjectForQuest(std::string questId, std::string objectId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Could not find object to hide for quest: %s\n", objectId.c_str());
		return;
	}

	if (!pObject->spawned || pObject->entityIndex < 0)
	{
		DevMsg("ERROR: Quest object has not yet been spawned yet: %s\n", objectId.c_str());
		return;
	}

	// we have an object, and we really want to hide it now.
	// "quest hide" means make non-collide & invisible.
	int iShouldSpin = (pQuest->bCollectiblesSpin) ? 1 : 0;
	int iCollectiblesCollide = (pQuest->bCollectiblesCollide) ? 1 : 0;
	engine->ClientCmd(VarArgs("quest_unmark_collectible_entity %i %.10g %.10g %.10g %i;", pObject->entityIndex, pObject->angles.x, pObject->angles.y, pObject->angles.z, iCollectiblesCollide));

	C_BaseEntity* pEntity = C_BaseEntity::Instance(pObject->entityIndex);
	if (pEntity)
	{
		pEntity->SetRenderMode(kRenderNormal);
		pEntity->SetMoveType(MOVETYPE_NONE);

		//if (!pQuest->bCollectiblesCollide)
			pEntity->SetSolid(SOLID_VPHYSICS);
	}
}

void C_QuestManager::QuestInit(std::string questId)
{
	DevMsg("C_QuestManager: QuestInit\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	questEvent_t* pInitQuestEvent = this->GetQuestEvent(pQuest->initQuestEventId);
	if (pInitQuestEvent)
	{
		// This quest has an event handler for this event. Use its attributes.

		// FOR NOW: Support enough to do the coin collect quest. So we'll need: HIDE (w/ "!collectibles" & "!self" support), BEGIN, MARKCOLLECTIBLE (w/ "!collectibles" support), and QuestClue support.
		// TODO: Support the rest of the functionality diagramed in the DIA.

		// HIDE
		// Supports:
		//	!collectibles (All objects the quest has as "collectibles".)
		////	!self (The object that sent a "collect" event.)
		unsigned int uNumObjectsToHide = pInitQuestEvent->objectsToHide.size();
		if (uNumObjectsToHide > 0)
		{
			for (unsigned int i = 0; i < uNumObjectsToHide; i++)
			{
				if (pInitQuestEvent->objectsToHide[i] == "!collectibles")
				{
					for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
						this->HideObjectForQuest(questId, pQuest->collectibleObjects[j]);
				}
				else
					this->HideObjectForQuest(questId, pInitQuestEvent->objectsToHide[i]);
			}
		}

		// UNHIDE
		// Supports:
		//	!collectibles (All objects the quest has as "collectibles".)
		////	!self (The object that sent a "collect" event.)
		unsigned int uNumObjectsToUnhide = pInitQuestEvent->objectsToUnhide.size();
		if (uNumObjectsToUnhide > 0)
		{
			for (unsigned int i = 0; i < uNumObjectsToUnhide; i++)
			{
				if (pInitQuestEvent->objectsToUnhide[i] == "!collectibles")
				{
					for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
						this->UnhideObjectForQuest(questId, pQuest->collectibleObjects[j]);
				}
				else
					this->UnhideObjectForQuest(questId, pInitQuestEvent->objectsToUnhide[i]);
			}
		}
	}
}

void C_QuestManager::QuestInitHandleBeginQuests(std::string questId)
{
	DevMsg("C_QuestManager: QuestInitHandleBegin\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	questEvent_t* pInitQuestEvent = this->GetQuestEvent(pQuest->initQuestEventId);
	if (pInitQuestEvent)
	{
		// BEGIN
		unsigned int uNumQuestsToBegin = pInitQuestEvent->questsToBegin.size();
		if (uNumQuestsToBegin > 0)
		{
			for (unsigned int i = 0; i < uNumQuestsToBegin; i++)
				this->QuestBegin(questId);
		}
	}
}

void C_QuestManager::QuestBegin(std::string questId)
{
	DevMsg("C_QuestManager: QuestBegin\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	// MARK COLLECTIBLE
	unsigned int uNumObjectsToMarkCollectible = pQuest->collectibleObjects.size();
	if (uNumObjectsToMarkCollectible > 0)
	{
		// Automatically UNHIDE them if they need to be.
		//for (unsigned int i = 0; i < uNumObjectsToMarkCollectible; i++)
		//	this->UnhideObjectForQuest(questId, pQuest->collectibleObjects[i]);

		for (unsigned int i = 0; i < uNumObjectsToMarkCollectible; i++)
			this->MarkCollectibleObjectForQuest(questId, pQuest->collectibleObjects[i]);
	}

	pQuest->eStatus = QUEST_STATUS_ACTIVE;

	questEvent_t* pQuestEvent = this->GetQuestEvent(pQuest->beginQuestEventId);
	if (!pQuestEvent)
		return;

	//	QUESTS TO HIDE
	for (unsigned int j = 0; j < pQuestEvent->questsToHide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pQuestEvent->questsToHide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = true;
	}

	//	QUESTS TO UNHIDE
	for (unsigned int j = 0; j < pQuestEvent->questsToUnhide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pQuestEvent->questsToUnhide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = false;
	}
}

void C_QuestManager::QuestSuccess(std::string questId)
{
	DevMsg("C_QuestManager: QuestSuccess\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	if (pQuest->eStatus == QUEST_STATUS_COMPLETE)
	{
		DevMsg("WARNING: Quest w/ ID %s received another Quest Success event after it was already in completed status. Ignoring.\n", pQuest->id.c_str());
		return;
	}

	pQuest->eStatus = QUEST_STATUS_COMPLETE;

	questEvent_t* pSuccessQuestEvent = this->GetQuestEvent(pQuest->successQuestEventId);
	if (!pSuccessQuestEvent)
		return;

	// HIDE
	// Supports:
	//	!collectibles (All objects the quest has as "collectibles".)
	//	!self (The object that sent a "collect" event.)
	/*
	unsigned int uNumObjectsToHide = pCollectQuestEvent->objectsToHide.size();
	if (uNumObjectsToHide > 0)
	{
	for (unsigned int i = 0; i < uNumObjectsToHide; i++)
	{
	if (pCollectQuestEvent->objectsToHide[i] == "!collectibles")
	{
	for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
	this->HideObjectForQuest(questId, pQuest->collectibleObjects[j]);
	}
	else if (pCollectQuestEvent->objectsToHide[i] == "!self")
	this->HideObjectForQuest(questId, objectId);
	else
	this->HideObjectForQuest(questId, pCollectQuestEvent->objectsToHide[i]);
	}
	}*/

	// QUESTCLUE
	questClue_t* pQuestClue = this->GetQuestClue(pSuccessQuestEvent->questClueId);
	if (pQuestClue)
	{
		if (enginevgui->IsGameUIVisible() || g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
			g_pAnarchyManager->HandleUiToggle();
		std::string questClueUrl = VarArgs("asset://ui/questClue.html?quest=%s&id=%s", pQuest->id.c_str(), pQuestClue->id.c_str());//&dialogue=%s, g_pAnarchyManager->encodeURIComponent(pQuestClue->dialogue).c_str());

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl(questClueUrl.c_str());
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
	}

	//	QUESTS TO END
	for (unsigned int j = 0; j < pSuccessQuestEvent->questsToEnd.size(); j++)
	{
		//quest_t* pVictimQuest = this->GetQuest(pSuccessQuestEvent->questsToEnd[j]);
		//if (pVictimQuest)
		this->QuestSuccess(pSuccessQuestEvent->questsToEnd[j]);
	}

	//	QUESTS TO BEGIN
	for (unsigned int j = 0; j < pSuccessQuestEvent->questsToBegin.size(); j++)
	{
		//quest_t* pVictimQuest = this->GetQuest(pSuccessQuestEvent->questsToBegin[j]);
		//if (pVictimQuest)
			this->QuestBegin(pSuccessQuestEvent->questsToBegin[j]);
			//this->InitializeQuest(pVictimQuest->id);
	}

	//	QUESTS TO HIDE
	for (unsigned int j = 0; j < pSuccessQuestEvent->questsToHide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pSuccessQuestEvent->questsToHide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = true;
	}

	//	QUESTS TO UNHIDE
	for (unsigned int j = 0; j < pSuccessQuestEvent->questsToUnhide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pSuccessQuestEvent->questsToUnhide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = false;
	}
}

#include <algorithm>
bool C_QuestManager::IsObjectUsedByAnyEZQuests(object_t* object)
{
	if(m_EZQuests.empty())
	{
		return false;
	}

	// Alright. This function is going to be a bit of a cluster fuck.
	// First of all, EZ Quests are not the native, strictly defined internal format of quests.
	// EZ Quests are more like quest generators. Instead of saying what object IDs are associated, they often just say a MDL to use instead.
	// Second is that EZ Quests cannot really initialize until AFTER objects are spawned in.
	// However, this method gets called WHILE objects are being spawned in.
	// EZ quests must be loaded immediately, but NOT initialized. At least enough for us to check against EZ quests in this method.
	// This method should be EZ quest specific - as the initialized quests have a much more specific & simple way of checking which objects are used by quests.

	KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(object->modelId));
	std::string modelFile = (pModelKV) ? pModelKV->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file") : "";

	//int iAAMaxString = modelFile.length() + 1;
	int iAAMaxString = AA_MAX_STRING;
	char* modelFileFixed = new char[iAAMaxString];

	Q_strncpy(modelFileFixed, modelFile.c_str(), iAAMaxString);
	V_FixSlashes(modelFileFixed);
	V_FixDoubleSlashes(modelFileFixed);
	modelFile = modelFileFixed;
	std::transform(modelFile.begin(), modelFile.end(), modelFile.begin(), ::tolower);

	std::string testModelFile;
	EZQuest_t* pEZQuest;
	for (unsigned int i = 0; i < m_EZQuests.size(); i++)
	{
		pEZQuest = m_EZQuests[i];
		
		if (pEZQuest->model != "")
		{
			testModelFile = pEZQuest->model;

			Q_strncpy(modelFileFixed, testModelFile.c_str(), iAAMaxString);
			V_FixSlashes(modelFileFixed);
			V_FixDoubleSlashes(modelFileFixed);
			testModelFile = modelFileFixed;
			std::transform(testModelFile.begin(), testModelFile.end(), testModelFile.begin(), ::tolower);

			if (testModelFile == modelFile)
				return true;
		}

		if (pEZQuest->object != "" && pEZQuest->object == object->objectId)
			return true;
	}

	return false;
}

void C_QuestManager::QuestCollect(std::string questId, std::string objectId)
{
	//DevMsg("C_QuestManager: QuestCollect\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	questEvent_t* pCollectQuestEvent = this->GetQuestEvent(pQuest->collectQuestEventId);
	if (!pCollectQuestEvent)
		return;

	// HIDE
	// Supports:
	//	!collectibles (All objects the quest has as "collectibles".)
	//	!self (The object that sent a "collect" event.)
	unsigned int uNumObjectsToHide = pCollectQuestEvent->objectsToHide.size();
	if (uNumObjectsToHide > 0)
	{
		for (unsigned int i = 0; i < uNumObjectsToHide; i++)
		{
			if (pCollectQuestEvent->objectsToHide[i] == "!collectibles")
			{
				for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
					this->HideObjectForQuest(questId, pQuest->collectibleObjects[j]);
			}
			else if (pCollectQuestEvent->objectsToHide[i] == "!self")
				this->HideObjectForQuest(questId, objectId);
			else
				this->HideObjectForQuest(questId, pCollectQuestEvent->objectsToHide[i]);
		}
	}

	// QUESTCLUE
	questClue_t* pQuestClue = this->GetQuestClue(pCollectQuestEvent->questClueId);
	if (pQuestClue)
	{
		if (enginevgui->IsGameUIVisible() || g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
			g_pAnarchyManager->HandleUiToggle();
		std::string questClueUrl = VarArgs("asset://ui/questClue.html?quest=%s&id=%s", pQuest->id.c_str(), pQuestClue->id.c_str());

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->SetUrl(questClueUrl.c_str());
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
	}

	//	QUESTS TO END
	for (unsigned int j = 0; j < pCollectQuestEvent->questsToEnd.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pCollectQuestEvent->questsToEnd[j]);
		if (pVictimQuest)
			this->QuestSuccess(pVictimQuest->id);
	}

	//	QUESTS TO HIDE
	for (unsigned int j = 0; j < pCollectQuestEvent->questsToHide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pCollectQuestEvent->questsToHide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = true;
	}

	//	QUESTS TO UNHIDE
	for (unsigned int j = 0; j < pCollectQuestEvent->questsToUnhide.size(); j++)
	{
		quest_t* pVictimQuest = this->GetQuest(pCollectQuestEvent->questsToUnhide[j]);
		if (pVictimQuest)
			pVictimQuest->bHidden = false;
	}
}

void C_QuestManager::SetInitialValues(questClue_t* pQuestClue)
{
	pQuestClue->dialogue = "No Dialogue.";
	pQuestClue->id = g_pAnarchyManager->GenerateUniqueId();
	pQuestClue->objectId = "";
	pQuestClue->eType = QUEST_CLUE_TYPE_ALERT;
	pQuestClue->name = pQuestClue->id;
}

void C_QuestManager::SetInitialValues(quest_t* pQuest)
{
	pQuest->bCollectOnce = true;
	pQuest->bHidden = true;
	pQuest->flMaxDuration = -1.0f;
	pQuest->flStartTime = -1.0f;
	pQuest->eCollectMethod = QUEST_COLLECT_METHOD_USE;
	pQuest->flCollectRadius = 120.0f;
	pQuest->bCollectiblesSpin = false;
	pQuest->bCollectiblesCollide = false;
	pQuest->id = g_pAnarchyManager->GenerateUniqueId();
	pQuest->eStatus = QUEST_STATUS_INACTIVE;
	pQuest->name = pQuest->id;
}

void C_QuestManager::SetInitialValues(questEvent_t* pQuestEvent)
{
	pQuestEvent->id = g_pAnarchyManager->GenerateUniqueId();
	pQuestEvent->name = pQuestEvent->id;
}

void C_QuestManager::AddQuest(quest_t* pQuest)
{
	m_quests[pQuest->id] = pQuest;
}

void C_QuestManager::AddQuestClue(questClue_t* pQuestClue)
{
	m_questClues[pQuestClue->id] = pQuestClue;
}

void C_QuestManager::AddQuestEvent(questEvent_t* pQuestEvent)
{
	m_questEvents[pQuestEvent->id] = pQuestEvent;
}

void C_QuestManager::UpdateQueue()
{
	if (!m_queuedQuestActions.empty())
	{
		QueuedQuestAction_t* pQuestAction = m_queuedQuestActions.front();// m_queuedQuestActions[m_queuedQuestActions.size() - 1];
		if (pQuestAction->action == "hide")
			this->DoHideObjectForQuest(pQuestAction->questId, pQuestAction->objectId);
		else if (pQuestAction->action == "unhide")
			this->DoUnhideObjectForQuest(pQuestAction->questId, pQuestAction->objectId);
		else if (pQuestAction->action == "mark")
			this->DoMarkCollectibleObjectForQuest(pQuestAction->questId, pQuestAction->objectId);
		else if (pQuestAction->action == "unmark")
			this->DoUnmarkCollectibleObjectForQuest(pQuestAction->questId, pQuestAction->objectId);

		m_queuedQuestActions.pop();
	}
}

void C_QuestManager::QuestDialogueEvent(std::string questId, std::string questClueId, std::string eventName)
{
	DevMsg("C_QuestManager: QuestDialogueEvent\n");

	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
	{
		DevMsg("ERROR: Could not find quest w/ ID: %s\n", questId.c_str());
		return;
	}

	questClue_t* pQuestClue = this->GetQuestClue(questClueId);
	if (!pQuestClue)
	{
		DevMsg("ERROR: Could not find questClue w/ ID: %s\n", questClueId.c_str());
		return;
	}

	if (eventName == "success")
	{
		for (unsigned int i = 0; i < pQuestClue->positiveResultQuestEventIds.size(); i++)
		{
			questEvent_t* pEvent = this->GetQuestEvent(pQuestClue->positiveResultQuestEventIds[i]);
			if (pEvent)
			{
				// HIDE
				// Supports:
				//	!collectibles (All objects the quest has as "collectibles".)
				//	!self (The object that sent a "collect" event.)
				unsigned int uNumObjectsToHide = pEvent->objectsToHide.size();
				if (uNumObjectsToHide > 0)
				{
					for (unsigned int i = 0; i < uNumObjectsToHide; i++)
					{
						if (pEvent->objectsToHide[i] == "!collectibles")
						{
							for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
								this->HideObjectForQuest(questId, pQuest->collectibleObjects[j]);
						}
						//else if (pEvent->objectsToHide[i] == "!self")
						//	this->HideObjectForQuest(questId, objectId);
						else
							this->HideObjectForQuest(questId, pEvent->objectsToHide[i]);
					}
				}

				// QUESTCLUE
				questClue_t* pQuestClue = this->GetQuestClue(pEvent->questClueId);
				if (pQuestClue)
				{
					if (enginevgui->IsGameUIVisible() || g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
						g_pAnarchyManager->HandleUiToggle();
					std::string questClueUrl = VarArgs("asset://ui/questClue.html?quest=%s&id=%s", pQuest->id.c_str(), pQuestClue->id.c_str());

					C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
					pHudBrowserInstance->SetUrl(questClueUrl.c_str());
					g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
				}

				//	QUESTS TO END
				for (unsigned int j = 0; j < pEvent->questsToEnd.size(); j++)
				{
					quest_t* pVictimQuest = this->GetQuest(pEvent->questsToEnd[j]);
					if (pVictimQuest)
						this->QuestSuccess(pVictimQuest->id);
				}
			}
		}
	}
	/*else if (eventName == "failure")
	{
		for (unsigned int i = 0; i < pQuestClue->negativeResultQuestEventIds.size(); i++)
		{
			questEvent_t* pEvent = this->GetQuestEvent(pQuestClue->negativeResultQuestEventIds[i]);
			if (pEvent)
			{
				// HIDE
				// Supports:
				//	!collectibles (All objects the quest has as "collectibles".)
				//	!self (The object that sent a "collect" event.)
				unsigned int uNumObjectsToHide = pEvent->objectsToHide.size();
				if (uNumObjectsToHide > 0)
				{
					for (unsigned int i = 0; i < uNumObjectsToHide; i++)
					{
						if (pEvent->objectsToHide[i] == "!collectibles")
						{
							for (unsigned int j = 0; j < pQuest->collectibleObjects.size(); j++)
								this->HideObjectForQuest(questId, pQuest->collectibleObjects[j]);
						}
						//else if (pEvent->objectsToHide[i] == "!self")
						//	this->HideObjectForQuest(questId, objectId);
						else
							this->HideObjectForQuest(questId, pEvent->objectsToHide[i]);
					}
				}

				// QUESTCLUE
				questClue_t* pQuestClue = this->GetQuestClue(pEvent->questClueId);
				if (pQuestClue)
				{
		if (enginevgui->IsGameUIVisible() || g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
			g_pAnarchyManager->HandleUiToggle();
					std::string questClueUrl = VarArgs("asset://ui/questClue.html?quest=%s&id=%s", pQuest->id.c_str(), pQuestClue->id.c_str());

					C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
					pHudBrowserInstance->SetUrl(questClueUrl.c_str());
					g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);
				}
			}
		}
	}*/
}

quest_t* C_QuestManager::GetQuest(std::string questId)
{
	auto it = m_quests.find(questId);
	if (it != m_quests.end())
		return it->second;

	return null;
}

questClue_t* C_QuestManager::GetQuestClue(std::string questClueId)
{
	auto it = m_questClues.find(questClueId);
	if (it != m_questClues.end())
		return it->second;

	return null;
}

questEvent_t* C_QuestManager::GetQuestEvent(std::string questEventId)
{
	auto it = m_questEvents.find(questEventId);
	if (it != m_questEvents.end())
		return it->second;

	return null;
}

void C_QuestManager::DeleteEZQuest(std::string questId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
		return;

	if (pQuest->_file == "" || !g_pFullFileSystem->FileExists(pQuest->_file.c_str(), "DEFAULT_WRITE_PATH"))
		return;

	g_pFullFileSystem->RemoveFile(pQuest->_file.c_str(), "DEFAULT_WRITE_PATH");
	this->RestartQuestSystem();
}

void C_QuestManager::CreateEZQuest(std::string templateName, std::vector<std::string> arguments, bool bPreLoadOnly, std::string questId, std::string fileName)
{
	quest_t* pGivenQuest = (questId != "") ? this->GetQuest(questId) : null;
	std::string saveFile = fileName;
	if (pGivenQuest && saveFile == "")
		saveFile = pGivenQuest->_file;

	if (templateName == "gossip")
	{
		quest_t* pQuest = new quest_t();
		this->SetInitialValues(pQuest);
			
		if (questId != "")
			pQuest->id = questId;

		// gossip params: [title, objective, dialogue, spin, collectSound, modelFile, clueType, interact, presence, hidden]
		if (!bPreLoadOnly)
		{
			// TODO: If we are given a questId, grab the SAVE FILE NAME from the Quest C++ object.  Then just restart the entire quest system & return.
			// Otherwise, just create a new name right now like regular.

			// save out the EZ Quest form info so it can be re-loaded next time we visit this instance
			g_pFullFileSystem->CreateDirHierarchy("ezquests", "DEFAULT_WRITE_PATH");

			if (saveFile == "")
			{
				std::string saveFileNameBase = VarArgs("quest_%s", g_pAnarchyManager->GetInstanceId().c_str());
				int iFileNumber = 0;
				std::string potentialFileName = VarArgs("ezquests\\%s.txt", saveFileNameBase.c_str());
				bool fileExists = g_pFullFileSystem->FileExists(potentialFileName.c_str(), "DEFAULT_WRITE_PATH");
				while (fileExists)
				{
					iFileNumber++;
					potentialFileName = VarArgs("ezquests\\%s_%i.txt", saveFileNameBase.c_str(), iFileNumber);
					fileExists = g_pFullFileSystem->FileExists(potentialFileName.c_str(), "DEFAULT_WRITE_PATH");
				}
				saveFile = potentialFileName;
			}

			unsigned int numArgs = arguments.size();
			KeyValues* pEZQuestKV = new KeyValues("ezquest");
			pEZQuestKV->SetString("id", pQuest->id.c_str());
			pEZQuestKV->SetString("_template", templateName.c_str());
			pEZQuestKV->SetString("title", (numArgs > 0) ? arguments[0].c_str() : "EZ Quest");
			pEZQuestKV->SetString("objective", (numArgs > 1) ? arguments[1].c_str() : "");
			pEZQuestKV->SetString("success", (numArgs > 2) ? arguments[2].c_str() : "");
			pEZQuestKV->SetString("spin", (numArgs > 3) ? arguments[3].c_str() : "0");
			pEZQuestKV->SetString("collectsound", (numArgs > 4) ? arguments[4].c_str() : "");
			pEZQuestKV->SetString("object", (numArgs > 5) ? arguments[5].c_str(): "");
			pEZQuestKV->SetString("type", (numArgs > 6) ? arguments[6].c_str() : "");
			pEZQuestKV->SetString("interact", (numArgs > 7) ? arguments[7].c_str() : "");
			pEZQuestKV->SetString("presence", (numArgs > 8) ? arguments[8].c_str() : "");
			pEZQuestKV->SetString("visibility", (numArgs > 9) ? arguments[9].c_str() : "default");
			pEZQuestKV->SetString("initial", (numArgs > 10) ? arguments[10].c_str() : "active");
			pEZQuestKV->SetString("nextQuests", (numArgs > 11) ? arguments[11].c_str() : "");
			pEZQuestKV->SetString("collidable", (numArgs > 12) ? arguments[12].c_str() : "0");

			if (!pEZQuestKV->SaveToFile(g_pFullFileSystem, saveFile.c_str(), "DEFAULT_WRITE_PATH"))
				DevMsg("ERROR: Failed to save quest file: %s\n", saveFile.c_str());

			EZQuest_t* pEZQuest = new EZQuest_t();
			pEZQuest->_id = pEZQuestKV->GetString("id");
			pEZQuest->_templateName = templateName;
			pEZQuest->_fileName = fileName;
			pEZQuest->title = pEZQuestKV->GetString("title");
			pEZQuest->objective = pEZQuestKV->GetString("objective");
			pEZQuest->success = pEZQuestKV->GetString("success");
			pEZQuest->spin = pEZQuestKV->GetString("spin", "0");
			pEZQuest->collectsound = pEZQuestKV->GetString("collectsound");
			pEZQuest->model = pEZQuestKV->GetString("model");
			pEZQuest->object = pEZQuestKV->GetString("object");
			pEZQuest->type = pEZQuestKV->GetString("type");
			pEZQuest->interact = pEZQuestKV->GetString("interact");
			pEZQuest->presence = pEZQuestKV->GetString("presence");
			pEZQuest->visibility = pEZQuestKV->GetString("visibility", "default");
			pEZQuest->initial = pEZQuestKV->GetString("initial", "active");
			pEZQuest->nextQuests = pEZQuestKV->GetString("nextQuests");
			pEZQuest->collidable = pEZQuestKV->GetString("collidable", "0");
			m_EZQuests.push_back(pEZQuest);

			pEZQuestKV->deleteThis();

			if (pGivenQuest)
			{
				this->RestartQuestSystem();
				return;
			}
		}

		std::string title = arguments[0];
		if (title == "")
			return;

		std::string objectiveText = arguments[1];
		if (objectiveText == "")
			return;

		std::string successText = arguments[2];
		if (successText == "")
			return;

		bool bSpinCollectibles = (arguments[3] != "0");

		std::string questCollectSound = arguments[4];

		std::string objectId_in = arguments[5];	// we need to confirm we can find the model file in the library still.

		std::string questClueType = arguments[6];	// alert, award, comic
		std::string questInteract = arguments[7];	// useOnce, touchOnce, useRepeatable
		std::string questPresence = arguments[8];	// untilCollected, always, untilSuccess, untilFailure, afterBegun
		std::string questVisibility = arguments[9];
		std::string questInitial = arguments[10];
		std::string questNextQuests = arguments[11];
		bool bCollectiblesCollide = (arguments[12] != "0");
		pQuest->bCollectiblesCollide = bCollectiblesCollide;

		// We have everything we need to create a NEW coin collect quest...

		if (!g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId_in))
		{
			DevMsg("ERROR: Cannot find instance object w/ ID: %s\n", objectId_in.c_str());
			return;
		}

		std::string objectId = objectId_in;

		// 1. Create a QuestClue w/ the user's victory Dialogue File.
		questClue_t* pVictoryClue = new questClue_t();
		this->SetInitialValues(pVictoryClue);
		pVictoryClue->dialogue = successText;
		if (questClueType == "comic")
			pVictoryClue->eType = QUEST_CLUE_TYPE_COMIC;
		else if (questClueType == "award")
			pVictoryClue->eType = QUEST_CLUE_TYPE_AWARD;
		else
			pVictoryClue->eType = QUEST_CLUE_TYPE_ALERT;
		pVictoryClue->objectId = objectId;
		this->AddQuestClue(pVictoryClue);

		// 2. Create a Quest w/ specified objectId as collectibles & the specified name/id. The interact method specified.  24 inch collect radius.  The given collectibles spin option & given collect sound file.
		pQuest->_template = templateName;
		pQuest->_file = saveFile;
		pQuest->name = title;
		pQuest->objective = objectiveText;
		if (questInteract == "useOnce")
			pQuest->eCollectMethod = QUEST_COLLECT_METHOD_USE;
		else if (questInteract == "useRepeatable")
		{
			pQuest->eCollectMethod = QUEST_COLLECT_METHOD_USE;
			pQuest->bCollectOnce = false;
		}
		else// if (questInteract == "touchOnce")
		{
			pQuest->eCollectMethod = QUEST_COLLECT_METHOD_TOUCH;
			pQuest->flCollectRadius = 60.0f;
		}
		pQuest->collectionSound = questCollectSound;
		pQuest->bCollectiblesSpin = bSpinCollectibles;
		// don't add this quest until the end.

		pQuest->collectibleObjects.push_back(objectId);



		// Create a positive dialogue result QuestEvent that hides the object.
		//if (questPresence == "untilSuccess")
		//{
			questEvent_t* pQuestEvent = new questEvent_t();
			this->SetInitialValues(pQuestEvent);
			if( questPresence == "untilSuccess" )
				pQuestEvent->objectsToHide.push_back("!collectibles");
			pQuestEvent->questsToEnd.push_back(pQuest->id);
			this->AddQuestEvent(pQuestEvent);

			pVictoryClue->positiveResultQuestEventIds.push_back(pQuestEvent->id);
		//}

		// Bind a Success QuestEvent if we want to hide our quest after it's over.
		if (questVisibility == "untilSuccess" || questVisibility == "afterSuccess" || questNextQuests != "")
		{
			questEvent_t* pQuestEventSuccess = new questEvent_t();
			this->SetInitialValues(pQuestEventSuccess);
			if( questVisibility == "untilSuccess" )
				pQuestEventSuccess->questsToHide.push_back(pQuest->id);
			else if (questVisibility == "afterSuccess")
				pQuestEventSuccess->questsToUnhide.push_back(pQuest->id);

			if (questNextQuests != "")
			{
				std::vector<std::string> tokenizedQuestsToBegin;
				g_pAnarchyManager->Tokenize(questNextQuests, tokenizedQuestsToBegin, ", ");

				for (unsigned int i = 0; i < tokenizedQuestsToBegin.size(); i++)
					pQuestEventSuccess->questsToBegin.push_back(tokenizedQuestsToBegin[i]);
			}

			pQuest->successQuestEventId = pQuestEventSuccess->id;
			this->AddQuestEvent(pQuestEventSuccess);
		}

		// 3. Bind an Init QuestEvent that hides the collectibles & begins the quest.
		questEvent_t* pQuestEventInit = new questEvent_t();
		this->SetInitialValues(pQuestEventInit);
		if (questPresence == "always")
			pQuestEventInit->objectsToUnhide.push_back("!collectibles");
		else
			pQuestEventInit->objectsToHide.push_back("!collectibles");
		if (questInitial != "inactive")
			pQuestEventInit->questsToBegin.push_back(pQuest->id);// pQuestEventInit->id);
		pQuest->initQuestEventId = pQuestEventInit->id;
		this->AddQuestEvent(pQuestEventInit);

		// 4. Bind a Begin QuestEvent that marks the collectibles as collectible. (Auto - unhides them when you do this.)
		questEvent_t* pQuestEventBegin = new questEvent_t();
		this->SetInitialValues(pQuestEventBegin);
		pQuestEventBegin->objectsToMarkCollectible.push_back("!collectibles");
		if (questVisibility != "never" && questVisibility != "afterSuccess")
			pQuestEventBegin->questsToUnhide.push_back(pQuest->id);
		pQuest->beginQuestEventId = pQuestEventBegin->id;
		this->AddQuestEvent(pQuestEventBegin);

		// 5. Bind a Collect QuestEvent that hides the collected object (if desired) and displays the dialogue.
		questEvent_t* pQuestEventCollect = new questEvent_t();
		this->SetInitialValues(pQuestEventCollect);
		if (questPresence == "untilCollected")
		{
			pQuestEventCollect->objectsToHide.push_back("!collectibles");
			pQuestEventCollect->questsToEnd.push_back(pQuest->id);
		}
		pQuestEventCollect->questClueId = pVictoryClue->id;
		pQuest->collectQuestEventId = pQuestEventCollect->id;
		this->AddQuestEvent(pQuestEventCollect);

		this->AddQuest(pQuest);

		// now initialize it.
		if (!bPreLoadOnly)
			this->InitializeQuest(pQuest->id);
	}
	else if (templateName == "coinCollect")
	{
		// coinCollect params: [title, objectiveText, successText, questSpinCoins, questCollectSound, modelFile, type, interact, persence, hidden]
		quest_t* pQuest = new quest_t();
		this->SetInitialValues(pQuest);
		if (questId != "")
			pQuest->id = questId;

		if (!bPreLoadOnly)
		{
			// save out the EZ Quest form info so it can be re-loaded next time we visit this instance
			g_pFullFileSystem->CreateDirHierarchy("ezquests", "DEFAULT_WRITE_PATH");

			if (saveFile == "")
			{
				std::string saveFileNameBase = VarArgs("quest_%s", g_pAnarchyManager->GetInstanceId().c_str());
				int iFileNumber = 0;
				std::string potentialFileName = VarArgs("ezquests\\%s.txt", saveFileNameBase.c_str());
				bool fileExists = g_pFullFileSystem->FileExists(potentialFileName.c_str(), "DEFAULT_WRITE_PATH");
				while (fileExists)
				{
					iFileNumber++;
					potentialFileName = VarArgs("ezquests\\%s_%i.txt", saveFileNameBase.c_str(), iFileNumber);
					fileExists = g_pFullFileSystem->FileExists(potentialFileName.c_str(), "DEFAULT_WRITE_PATH");
				}
				saveFile = potentialFileName;
			}

			unsigned int numArgs = arguments.size();
			KeyValues* pEZQuestKV = new KeyValues("ezquest");
			pEZQuestKV->SetString("id", pQuest->id.c_str());
			pEZQuestKV->SetString("_template", templateName.c_str());
			pEZQuestKV->SetString("title", (numArgs > 0) ? arguments[0].c_str() : "EZ Quest");
			pEZQuestKV->SetString("objective", (numArgs > 1) ? arguments[1].c_str() : "");
			pEZQuestKV->SetString("success", (numArgs > 2) ? arguments[2].c_str() : "");
			pEZQuestKV->SetString("spin", (numArgs > 3) ? arguments[3].c_str() : "0");
			pEZQuestKV->SetString("collectsound", (numArgs > 4) ? arguments[4].c_str() : "");
			pEZQuestKV->SetString("model", (numArgs > 5) ? arguments[5].c_str() : "");
			pEZQuestKV->SetString("type", (numArgs > 6) ? arguments[6].c_str() : "");
			pEZQuestKV->SetString("interact", (numArgs > 7) ? arguments[7].c_str() : "");
			pEZQuestKV->SetString("presence", (numArgs > 8) ? arguments[8].c_str() : "");
			pEZQuestKV->SetString("visibility", (numArgs > 9) ? arguments[9].c_str() : "default");
			pEZQuestKV->SetString("initial", (numArgs > 10) ? arguments[10].c_str() : "active");
			pEZQuestKV->SetString("nextQuests", (numArgs > 11) ? arguments[11].c_str() : "");
			pEZQuestKV->SetString("collidable", (numArgs > 12) ? arguments[12].c_str() : "0");

			if (!pEZQuestKV->SaveToFile(g_pFullFileSystem, saveFile.c_str(), "DEFAULT_WRITE_PATH"))
				DevMsg("ERROR: Failed to save quest file: %s\n", saveFile.c_str());

			EZQuest_t* pEZQuest = new EZQuest_t();
			pEZQuest->_id = pEZQuestKV->GetString("id");
			pEZQuest->_templateName = templateName;
			pEZQuest->_fileName = fileName;
			pEZQuest->title = pEZQuestKV->GetString("title");
			pEZQuest->objective = pEZQuestKV->GetString("objective");
			pEZQuest->success = pEZQuestKV->GetString("success");
			pEZQuest->spin = pEZQuestKV->GetString("spin", "0");
			pEZQuest->collectsound = pEZQuestKV->GetString("collectsound");
			pEZQuest->model = pEZQuestKV->GetString("model");
			pEZQuest->object = pEZQuestKV->GetString("object");
			pEZQuest->type = pEZQuestKV->GetString("type");
			pEZQuest->interact = pEZQuestKV->GetString("interact");
			pEZQuest->presence = pEZQuestKV->GetString("presence");
			pEZQuest->visibility = pEZQuestKV->GetString("visibility", "default");
			pEZQuest->initial = pEZQuestKV->GetString("initial", "active");
			pEZQuest->nextQuests = pEZQuestKV->GetString("nextQuests");
			pEZQuest->collidable = pEZQuestKV->GetString("collidable", "0");
			m_EZQuests.push_back(pEZQuest);

			pEZQuestKV->deleteThis();

			if (pGivenQuest)
			{
				this->RestartQuestSystem();
				return;
			}
		}
		
		std::string title = arguments[0];
		if (title == "")
			return;

		std::string objectiveText = arguments[1];
		if (objectiveText == "")
			return;

		std::string successText = arguments[2];
		if (successText == "")
			return;

		bool bSpinCollectibles = (arguments[3] != "0");

		std::string questCollectSound = arguments[4];

		std::string modelFile_in = arguments[5];	// we need to confirm we can find the model file in the library still.

		std::string questClueType = arguments[6];	// alert, award, comic
		std::string questInteract = arguments[7];	// useOnce, touchOnce
		std::string questPresence = arguments[8];	// untilCollected, always
		std::string questVisibility = arguments[9];
		std::string questInitial = arguments[10];
		std::string questNextQuests = arguments[11];
		bool bCollectiblesCollide = (arguments[12] != "0");
		pQuest->bCollectiblesCollide = bCollectiblesCollide;

		// We have everything we need to create a NEW coin collect quest...

		// Find every object that uses this model and add them to the collectibles list...
		KeyValues* pSearchInfoKV = new KeyValues("search");
		pSearchInfoKV->SetString("file", modelFile_in.c_str());

		// Find the model in the library
		KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryModel(pSearchInfoKV));
		if (!pModelKV)
		{
			DevMsg("ERROR: Could not find the model \"%s\" in library.\n", modelFile_in.c_str());
			return;
		}

		std::string modelFile = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
		if (modelFile == "")
		{
			DevMsg("ERROR: Model has empty FILE value.\n");
			return;
		}

		std::string modelId = pModelKV->GetString("info/id");

		// 1. Create a QuestClue w/ the user's victory Dialogue File.
		questClue_t* pVictoryClue = new questClue_t();
		this->SetInitialValues(pVictoryClue);
		pVictoryClue->dialogue = successText;
		if (questClueType == "comic")
			pVictoryClue->eType = QUEST_CLUE_TYPE_COMIC;
		else if (questClueType == "award")
			pVictoryClue->eType = QUEST_CLUE_TYPE_AWARD;
		else
			pVictoryClue->eType = QUEST_CLUE_TYPE_ALERT;
		this->AddQuestClue(pVictoryClue);

		// 2. Create a Quest w/ all objects that use the specified model as collectibles & the specified name/id. Touch to collect, 24 inch collect radius.  Collectibles spin & given collect sound file.
		pQuest->_template = templateName;
		pQuest->_file = saveFile;
		//pQuest->bHidden = bQuestHidden;
		pQuest->name = title;
		pQuest->objective = objectiveText;
		if (questInteract == "useOnce")
			pQuest->eCollectMethod = QUEST_COLLECT_METHOD_USE;
		else// if (questInteract == "touchOnce")
		{
			pQuest->eCollectMethod = QUEST_COLLECT_METHOD_TOUCH;
			pQuest->flCollectRadius = 60.0f;
		}
		pQuest->collectionSound = questCollectSound;
		pQuest->bCollectiblesSpin = bSpinCollectibles;
		// don't add this quest until the end.

		// Add all object IDs that use this model to the collectible list.
		std::map<std::string, object_t*> objects = g_pAnarchyManager->GetInstanceManager()->GetObjectsMap();
		object_t* pObject;
		auto it = objects.begin();
		while (it != objects.end())
		{
			pObject = it->second;
			if (pObject->modelId == modelId)
				pQuest->collectibleObjects.push_back(pObject->objectId);

			it++;
		}

		// 3. Bind an Init QuestEvent that hides the collectibles & begins the quest.
		questEvent_t* pQuestEventInit = new questEvent_t();
		this->SetInitialValues(pQuestEventInit);
		if (questPresence == "always")
			pQuestEventInit->objectsToUnhide.push_back("!collectibles");
		else
			pQuestEventInit->objectsToHide.push_back("!collectibles");
		if (questInitial != "inactive")
			pQuestEventInit->questsToBegin.push_back(pQuest->id);//pQuestEventInit->id);
		pQuest->initQuestEventId = pQuestEventInit->id;
		this->AddQuestEvent(pQuestEventInit);

		// 4. Bind a Begin QuestEvent that marks the collectibles as collectible. (Auto - unhides them when you do this.)
		questEvent_t* pQuestEventBegin = new questEvent_t();
		this->SetInitialValues(pQuestEventBegin);
		pQuestEventBegin->objectsToMarkCollectible.push_back("!collectibles");
		if (questVisibility != "never" && questVisibility != "afterSuccess")
			pQuestEventBegin->questsToUnhide.push_back(pQuest->id);
		pQuest->beginQuestEventId = pQuestEventBegin->id;
		this->AddQuestEvent(pQuestEventBegin);

		// 5. Bind a Collect QuestEvent that hides the collected object.
		questEvent_t* pQuestEventCollect = new questEvent_t();
		this->SetInitialValues(pQuestEventCollect);
		if (questPresence == "untilCollected")
			pQuestEventCollect->objectsToHide.push_back("!self");
		pQuest->collectQuestEventId = pQuestEventCollect->id;
		this->AddQuestEvent(pQuestEventCollect);

		// 6. Bind a Success QuestEvent that displays the victory QuestClue.
		questEvent_t* pQuestEventSuccess = new questEvent_t();
		this->SetInitialValues(pQuestEventSuccess);
		pQuestEventSuccess->questClueId = pVictoryClue->id;
		if (questVisibility == "untilSuccess")
			pQuestEventSuccess->questsToHide.push_back(pQuest->id);
		else if (questVisibility == "afterSuccess")
			pQuestEventSuccess->questsToUnhide.push_back(pQuest->id);

		if (questNextQuests != "")
		{
			std::vector<std::string> tokenizedQuestsToBegin;
			g_pAnarchyManager->Tokenize(questNextQuests, tokenizedQuestsToBegin, ", ");

			for (unsigned int i = 0; i < tokenizedQuestsToBegin.size(); i++)
				pQuestEventSuccess->questsToBegin.push_back(tokenizedQuestsToBegin[i]);
		}

		pQuest->successQuestEventId = pQuestEventSuccess->id;
		this->AddQuestEvent(pQuestEventSuccess);

		this->AddQuest(pQuest);

		// now initialize it.
		if( !bPreLoadOnly )
			this->InitializeQuest(pQuest->id);
	}
	else
		DevMsg("ERROR: Unknown template name: %s\n", templateName.c_str());
}

void C_QuestManager::InitializeQuest(std::string questId)
{
	this->QuestInit(questId);
	this->QuestInitHandleBeginQuests(questId);
}

void C_QuestManager::GetAllQuests(std::vector<quest_t*>& questsResponse)
{
	auto it = m_quests.begin();
	while (it != m_quests.end())
	{
		questsResponse.push_back(it->second);
		it++;
	}
}

void C_QuestManager::ResetQuest(std::string questId)
{
	quest_t* pQuest = this->GetQuest(questId);
	if (!pQuest)
		return;

	// stop the quest
	pQuest->eStatus = QUEST_STATUS_INACTIVE;
	
	// unmark all objects as collectible
	unsigned int uNumCollectibleObjects = pQuest->collectibleObjects.size();
	for (unsigned int i = 0; i < uNumCollectibleObjects; i++)
		this->UnmarkCollectibleObjectForQuest(questId, pQuest->collectibleObjects[i]);

	// unhide all its objects
	unsigned int uNumHiddenObjects = pQuest->hiddenObjects.size();
	for (unsigned int i = 0; i < uNumHiddenObjects; i++)
		this->UnhideObjectForQuest(questId, pQuest->hiddenObjects[i]);

	// uncollect any that are already collected.
	pQuest->collectedObjects.clear();
	pQuest->hiddenObjects.clear();

	// now re-initialize it.
	this->InitializeQuest(questId);
}

void C_QuestManager::GetAllQuestsForObject(std::string objectId, std::vector<quest_t*>& questsResponse)
{
	unsigned int max;
	auto it = m_quests.begin();
	while (it != m_quests.end())
	{
		max = it->second->collectibleObjects.size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (it->second->collectibleObjects[i] == objectId)
			{
				questsResponse.push_back(it->second);
				break;
			}
		}
		it++;
	}
}

void C_QuestManager::RestartQuestSystem()
{
	g_pAnarchyManager->GetQuestManager()->ShutdownWorldQuests();
	//g_pAnarchyManager->GetQuestManager()->LoadAndInitWorldQuests();
	g_pAnarchyManager->GetQuestManager()->LoadEZQuests(g_pAnarchyManager->GetInstanceId());
	g_pAnarchyManager->GetQuestManager()->InitializeAndBeginAllQuests();
}

bool C_QuestManager::OnPlayerUse()
{
	bool bAlreadyCollected;
	std::string collectibleObjectId;
	unsigned int uNumCollectibleObjects;
	object_t* pCollectibleObject;

	C_BasePlayer* pPlayer = null;
	Vector playerOrigin;
	Vector testOrigin;

	std::vector<object_t*> validObjects;
	std::vector<quest_t*> validQuests;


	quest_t* pQuest;
	auto it = m_quests.begin();
	while (it != m_quests.end())
	{
		pQuest = it->second;
		if (pQuest->eStatus == QUEST_STATUS_ACTIVE && pQuest->eCollectMethod == QUEST_COLLECT_METHOD_USE)
		{
			if (!pPlayer)
			{
				pPlayer = C_BasePlayer::GetLocalPlayer();
				playerOrigin = pPlayer->GetAbsOrigin();
			}

			// check if there are un-collected collectibles
			uNumCollectibleObjects = pQuest->collectibleObjects.size();
			if (pQuest->collectedObjects.size() < uNumCollectibleObjects && uNumCollectibleObjects > 0)
			{
				// See if we are within range of any of these un-collected collectibles.
				for (unsigned int i = 0; i < uNumCollectibleObjects; i++)
				{
					collectibleObjectId = pQuest->collectibleObjects[i];

					// make sure we are not already collected.
					bAlreadyCollected = false;
					for (unsigned int j = 0; j < pQuest->collectedObjects.size(); j++)
					{
						if (pQuest->collectedObjects[j] == collectibleObjectId)
						{
							bAlreadyCollected = true;
							break;
						}
					}

					if (bAlreadyCollected)
						continue;

					// we are an un-collected collectible.
					// Check if the object is available still.
					pCollectibleObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(collectibleObjectId);

					if (!pCollectibleObject)
					{
						//DevMsg("ERROR: Cannot get collectible object that should exist!\n");
						continue;
					}

					// Check the distance of the object to the player.
					testOrigin = pCollectibleObject->origin;

					// Does the object exist? If so, use it's current pos instead of object pos.
					if (pCollectibleObject->entityIndex >= 0) {
						C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(pCollectibleObject->entityIndex);
						if (pBaseEntity) {
							testOrigin = pBaseEntity->GetAbsOrigin();
						}
					}

					if (pCollectibleObject->entityIndex >= 0 && playerOrigin.DistTo(testOrigin) < pQuest->flCollectRadius)	// Might be useful if quests could work even w/o their objects spawned - to avoid the need to spawn all quest-related objects prior to running quests.
					{
						// collect the object for the quest.

						// hide the object, only if we are supposed to.
						//this->HideObjectForQuest(pQuest->id, collectibleObjectId);

						// collect it
						////this->QuestCollect(questId, objectId);
						//this->CollectObjectForQuest(pQuest->id, collectibleObjectId);
						//return true;	// only collect one at a time
						validObjects.push_back(pCollectibleObject);
						validQuests.push_back(pQuest);
					}
				}
			}
		}

		it++;
	}

	if (!pPlayer)
		return false;

	// validObjects and validQuests are filled with only objects within range that can be selected.
	// Find the best one.
	Vector eyePosition = pPlayer->EyePosition();
	QAngle eyeAngles = pPlayer->EyeAngles();

	float flBestDot = 999.9f;	// view dot is between -1 and 1. So this is as good as INFINITY.
	quest_t* pBestQuest = null;
	object_t* pBestObject = null;
	float flTestDot;
	Vector toTarget;
	Vector forward;
	for (unsigned int i = 0; i < validObjects.size(); i++)
	{
		toTarget = validObjects[i]->origin - eyePosition;
		toTarget.NormalizeInPlace();
		AngleVectors(eyeAngles, &forward);
		flTestDot = DotProduct(forward, toTarget);
		if (!pBestQuest || flTestDot > flBestDot)
		{
			flBestDot = flTestDot;
			pBestQuest = validQuests[i];
			pBestObject = validObjects[i];
		}
	}

	if (pBestQuest)
	{
		this->CollectObjectForQuest(pBestQuest->id, pBestObject->objectId);
		return true;
	}

	return false;
}

void C_QuestManager::InitializeAndBeginAllQuests()
{
	if (m_EZQuests.empty())
	{
		return;
	}

	// Load our EZQuests into real quests.
	EZQuest_t* pEZQuest;
	std::vector<std::string> arguments;
	for (unsigned int i = 0; i < m_EZQuests.size(); i++)
	{
		pEZQuest = m_EZQuests[i];

		arguments.clear();

		// Load the file into a KV
		arguments.push_back(pEZQuest->title);
		arguments.push_back(pEZQuest->objective);
		arguments.push_back(pEZQuest->success);
		arguments.push_back(pEZQuest->spin);
		arguments.push_back(pEZQuest->collectsound);
		if (pEZQuest->_templateName == "coinCollect")
			arguments.push_back(pEZQuest->model);
		else if (pEZQuest->_templateName == "gossip")
			arguments.push_back(pEZQuest->object);
		arguments.push_back(pEZQuest->type);
		arguments.push_back(pEZQuest->interact);
		arguments.push_back(pEZQuest->presence);
		arguments.push_back(pEZQuest->visibility);
		arguments.push_back(pEZQuest->initial);
		arguments.push_back(pEZQuest->nextQuests);
		arguments.push_back(pEZQuest->collidable);
		this->CreateEZQuest(pEZQuest->_templateName, arguments, true, pEZQuest->_id, pEZQuest->_fileName);
	}

	// Alright, EZ quests are "loaded" now.
	// We don't have any regular quests to load yet because right now only EZ quests are all we can save / load. So moving on...

	// Let's initialize them now. The trick here is that we don't want to "begin" anything until after ALL quests have had a chance to initialize.
	auto questsIterator = m_quests.begin();
	while (questsIterator != m_quests.end())
	{
		this->QuestInit(questsIterator->first);
		questsIterator++;
	}

	// NOW handle begin quests - now that ALL have been initialized.
	questsIterator = m_quests.begin();
	while (questsIterator != m_quests.end())
	{
		this->QuestInitHandleBeginQuests(questsIterator->first);
		questsIterator++;
	}
}

void C_QuestManager::LoadEZQuests(std::string instanceId)
{
	DevMsg("C_QuestManager: LoadEZQuests\n");

	// Find any .txt files in "ezquests" that match our instance ID and try to load them.
	std::string loadFileNameBase = VarArgs("quest_%s", instanceId.c_str());//g_pAnarchyManager->GetInstanceId()

	std::string templateName;
	std::string fileName;
	KeyValues* pEZQuestKV;
	EZQuest_t* pEZQuest;
	FileFindHandle_t findFileHandle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("ezquests\\%s*.txt", loadFileNameBase.c_str()), "DEFAULT_WRITE_PATH", &findFileHandle);
	while (pFilename != NULL)
	{
		// Load the file into a KV
		fileName = VarArgs("ezquests\\%s", pFilename);
		pEZQuestKV = new KeyValues("ezquest");
		if (!pEZQuestKV->LoadFromFile(g_pFullFileSystem, fileName.c_str(), "DEFAULT_WRITE_PATH"))
		{
			DevMsg("ERROR: Could not load file: %s\n", fileName.c_str());

			// Clean up the KV
			pEZQuestKV->deleteThis();
			pEZQuestKV = null;

			pFilename = g_pFullFileSystem->FindNext(findFileHandle);
			continue;
		}


		// use the KV
		templateName = pEZQuestKV->GetString("_template");
		
		if (templateName == "coinCollect" || templateName == "gossip")
		{
			pEZQuest = new EZQuest_t();
			pEZQuest->_id = pEZQuestKV->GetString("id");
			pEZQuest->_templateName = templateName;
			pEZQuest->_fileName = fileName;
			pEZQuest->title = pEZQuestKV->GetString("title");
			pEZQuest->objective = pEZQuestKV->GetString("objective");
			pEZQuest->success = pEZQuestKV->GetString("success");
			pEZQuest->spin = pEZQuestKV->GetString("spin", "0");
			pEZQuest->collectsound = pEZQuestKV->GetString("collectsound");
			pEZQuest->model = pEZQuestKV->GetString("model");
			pEZQuest->object = pEZQuestKV->GetString("object");
			pEZQuest->type = pEZQuestKV->GetString("type");
			pEZQuest->interact = pEZQuestKV->GetString("interact");
			pEZQuest->presence = pEZQuestKV->GetString("presence");
			pEZQuest->visibility = pEZQuestKV->GetString("visibility", "default");
			pEZQuest->initial = pEZQuestKV->GetString("initial", "active");
			pEZQuest->nextQuests = pEZQuestKV->GetString("nextQuests");
			pEZQuest->collidable = pEZQuestKV->GetString("collidable", "0");
			m_EZQuests.push_back(pEZQuest);
		}
		else
			DevMsg("ERROR: Unsupported EZ mode template name.\n");

		// Clean up the KV
		pEZQuestKV->deleteThis();
		pEZQuestKV = null;

		pFilename = g_pFullFileSystem->FindNext(findFileHandle);
	}
	g_pFullFileSystem->FindClose(findFileHandle);

	DevMsg("Finished Loading EZ Quests\n");
}

/*
OLD VERSION. Depreciated because now we must init quests earlier to make sure their objects spawn in.
void C_QuestManager::LoadAndInitWorldQuests()
{
	DevMsg("C_QuestManager: LoadAndInitWorldQuests\n");

	// Find any .txt files in "ezquests" that match our instance ID and try to load them.
	std::string loadFileNameBase = VarArgs("quest_%s", g_pAnarchyManager->GetInstanceId().c_str());

	std::vector<std::string> arguments;
	std::string templateName;
	std::string fileName;
	KeyValues* pEZQuestKV;
	FileFindHandle_t findFileHandle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx(VarArgs("ezquests\\%s*.txt", loadFileNameBase.c_str()), "DEFAULT_WRITE_PATH", &findFileHandle);
	while (pFilename != NULL)
	{
		arguments.clear();

		// Load the file into a KV
		fileName = VarArgs("ezquests\\%s", pFilename);
		pEZQuestKV = new KeyValues("ezquest");
		if (!pEZQuestKV->LoadFromFile(g_pFullFileSystem, fileName.c_str(), "DEFAULT_WRITE_PATH"))
			DevMsg("ERROR: Could not load file: %s\n", fileName.c_str());

		// use the KV
		templateName = pEZQuestKV->GetString("_template");

		if (templateName == "coinCollect")
		{
			// coinCollect params: [title, objectiveText, successText, questSpinCoins, questCollectSound, modelFile]
			arguments.push_back(pEZQuestKV->GetString("title"));
			arguments.push_back(pEZQuestKV->GetString("objective"));
			arguments.push_back(pEZQuestKV->GetString("success"));
			arguments.push_back(pEZQuestKV->GetString("spin", "0"));
			arguments.push_back(pEZQuestKV->GetString("collectsound"));
			arguments.push_back(pEZQuestKV->GetString("model"));
			arguments.push_back(pEZQuestKV->GetString("type"));
			arguments.push_back(pEZQuestKV->GetString("interact"));
			arguments.push_back(pEZQuestKV->GetString("presence"));
			arguments.push_back(pEZQuestKV->GetString("visibility", "default"));
			arguments.push_back(pEZQuestKV->GetString("initial", "active"));
			arguments.push_back(pEZQuestKV->GetString("nextQuests", ""));
			arguments.push_back(pEZQuestKV->GetString("collidable", "0"));
			this->CreateEZQuest(templateName, arguments, true, pEZQuestKV->GetString("id"), fileName);
		}
		else if (templateName == "gossip")
		{
			// coinCollect params: [title, objectiveText, successText, questSpinCoins, questCollectSound, modelFile]
			arguments.push_back(pEZQuestKV->GetString("title"));
			arguments.push_back(pEZQuestKV->GetString("objective"));
			arguments.push_back(pEZQuestKV->GetString("success"));
			arguments.push_back(pEZQuestKV->GetString("spin", "0"));
			arguments.push_back(pEZQuestKV->GetString("collectsound"));
			arguments.push_back(pEZQuestKV->GetString("object"));
			arguments.push_back(pEZQuestKV->GetString("type"));
			arguments.push_back(pEZQuestKV->GetString("interact"));
			arguments.push_back(pEZQuestKV->GetString("presence"));
			arguments.push_back(pEZQuestKV->GetString("visibility", "default"));
			arguments.push_back(pEZQuestKV->GetString("initial", "active"));
			arguments.push_back(pEZQuestKV->GetString("nextQuests", ""));
			arguments.push_back(pEZQuestKV->GetString("collidable", "0"));
			this->CreateEZQuest(templateName, arguments, true, pEZQuestKV->GetString("id"), fileName);
		}
		else
			DevMsg("ERROR: Unsupported EZ mode template name.\n");

		// Clean up the KV
		pEZQuestKV->deleteThis();
		pEZQuestKV = null;

		pFilename = g_pFullFileSystem->FindNext(findFileHandle);
	}
	g_pFullFileSystem->FindClose(findFileHandle);

	// Alright, quests are "loaded" now.
	// Let's initialize them now. The trick here is that we don't want to "begin" anything until after ALL quests have had a chance to initialize.
	auto questsIterator = m_quests.begin();
	while (questsIterator != m_quests.end())
	{
		this->QuestInit(questsIterator->first);
		questsIterator++;
	}

	// NOW handle begin quests - now that ALL have been initialized.
	questsIterator = m_quests.begin();
	while (questsIterator != m_quests.end())
	{
		this->QuestInitHandleBeginQuests(questsIterator->first);
		questsIterator++;
	}

	DevMsg("Finished Loading & Initializing Quests\n");
}
*/

void C_QuestManager::ShutdownWorldQuests()
{
	DevMsg("C_QuestManager: ShutdownWorldQuests\n");

	while (!m_questClues.empty())
	{
		delete m_questClues.begin()->second;
		m_questClues.erase(m_questClues.begin());
	}

	while (!m_questEvents.empty())
	{
		delete m_questEvents.begin()->second;
		m_questEvents.erase(m_questEvents.begin());
	}

	while (!m_quests.empty())
	{
		delete m_quests.begin()->second;
		m_quests.erase(m_quests.begin());
	}

	while (!m_EZQuests.empty())
		m_EZQuests.pop_back();

	while (!m_queuedQuestActions.empty())
		m_queuedQuestActions.pop();

	m_bQuestsInitlaizing = false;
}