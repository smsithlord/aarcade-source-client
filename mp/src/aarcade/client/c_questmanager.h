#ifndef C_QUEST_MANAGER_H
#define C_QUEST_MANAGER_H

//#include "c_quest.h"
#include <map>
#include <vector>
#include <queue>
#include <string>

enum eQuestStatus {
	QUEST_STATUS_INACTIVE = 0,
	QUEST_STATUS_ACTIVE = 1,
	QUEST_STATUS_COMPLETE = 2,
	QUEST_STATUS_FAIL = 3
};

/*
enum eQuestVisibility {
	QUEST_VISIBILITY_DEFAULT = 0,	// invisible at init.  visible after begin.
	QUEST_VISIBILITY_NEVER = 1,
	QUEST_VISIBILITY_UNTIL_SUCCESS = 2,
	QUEST_VISIBILITY_AFTER_SUCCESS = 3
};
*/

enum eQuestCollectMethod {
	QUEST_COLLECT_METHOD_USE = 0,
	QUEST_COLLECT_METHOD_TOUCH = 1
};

enum eQuestClueType {
	QUEST_CLUE_TYPE_ALERT = 0,
	QUEST_CLUE_TYPE_COMIC = 1,
	QUEST_CLUE_TYPE_AWARD = 2
};

struct quest_t {
	std::string name;
	std::string objective;
	std::string id;
	eQuestStatus eStatus;
	bool bHidden;
	eQuestCollectMethod eCollectMethod;
	float flCollectRadius;
	std::string collectionSound;
	bool bCollectiblesSpin;
	bool bCollectiblesCollide;
	bool bCollectOnce;
	float flStartTime;
	float flMaxDuration;
	std::vector<std::string> collectibleObjects;
	std::vector<std::string> collectedObjects;
	std::vector<std::string> hiddenObjects;
	std::string initQuestEventId;
	std::string beginQuestEventId;
	std::string collectQuestEventId;
	std::string successQuestEventId;
	std::string failureQuestEventId;
	std::string _template;
	std::string _file;
};

struct questClue_t {
	std::string name;
	std::string id;
	std::string objectId;
	eQuestClueType eType;
	std::vector<std::string> positiveResultQuestEventIds;
	std::vector<std::string> negativeResultQuestEventIds;
	std::string dialogue;
};

struct questEvent_t {
	std::string name;
	std::string id;
	std::vector<std::string> objectsToHide;
	std::vector<std::string> objectsToUnhide;
	std::vector<std::string> objectsToMarkCollectible;
	std::vector<std::string> questsToBegin;
	std::vector<std::string> questsToEnd;
	std::vector<std::string> questsToHide;
	std::vector<std::string> questsToUnhide;
	std::string questClueId;
};

// In order to detect if an objectID is going to be used with an EZ quest tha tis not yet initialized, we must store EZ quests internally for this superficial reason.
struct EZQuest_t {
	std::string _id;
	std::string _templateName;
	std::string _fileName;
	std::string title;
	std::string objective;
	std::string success;
	std::string spin;
	std::string collectsound;
	std::string model;
	std::string object;
	std::string type;
	std::string interact;
	std::string presence;
	std::string visibility;
	std::string initial;
	std::string nextQuests;
	std::string collidable;
};

// We must queue any quest actions so we don't flood the server w/ commands.
struct QueuedQuestAction_t {
	std::string action;
	std::string questId;
	std::string objectId;
};

class C_QuestManager
{
public:
	C_QuestManager();
	~C_QuestManager();

	void Init();
	void Update();

	void LoadEZQuests(std::string instanceId);
	void ShutdownWorldQuests();

	// these actually just queue things
	//void CollectObjectForQuest(std::string questId, std::string objectId);
	void HideObjectForQuest(std::string questId, std::string objectId);
	void UnhideObjectForQuest(std::string questId, std::string objectId);
	void MarkCollectibleObjectForQuest(std::string questId, std::string objectId);
	void UnmarkCollectibleObjectForQuest(std::string questId, std::string objectId);

	// these actually DO the actions
	void CollectObjectForQuest(std::string questId, std::string objectId);
	void DoHideObjectForQuest(std::string questId, std::string objectId);
	void DoUnhideObjectForQuest(std::string questId, std::string objectId);
	void DoMarkCollectibleObjectForQuest(std::string questId, std::string objectId);
	void DoUnmarkCollectibleObjectForQuest(std::string questId, std::string objectId);

	void UpdateQueue();

	void QuestDialogueEvent(std::string questId, std::string questClueId, std::string eventName);

	void QuestInit(std::string questId);
	void QuestInitHandleBeginQuests(std::string questId);
	void QuestBegin(std::string questId);
	void QuestCollect(std::string questId, std::string objectId);
	void QuestSuccess(std::string questId);

	bool IsObjectUsedByAnyEZQuests(object_t* object);

	void SetInitialValues(questClue_t* pQuestClue);
	void SetInitialValues(quest_t* pQuest);
	void SetInitialValues(questEvent_t* pQuestEvent);

	quest_t* GetQuest(std::string questId);
	questClue_t* GetQuestClue(std::string questClueId);
	questEvent_t* GetQuestEvent(std::string questEventId);

	void GetAllQuests(std::vector<quest_t*>& questsResponse);
	void ResetQuest(std::string questId);

	void GetAllQuestsForObject(std::string objectId, std::vector<quest_t*>& questsResponse);

	void RestartQuestSystem();

	bool OnPlayerUse();

	void InitializeAndBeginAllQuests();
	void InitializeQuest(std::string questId);
	void CreateEZQuest(std::string templateName, std::vector<std::string> arguments, bool bPreLoadOnly, std::string questId, std::string fileName);
	void DeleteEZQuest(std::string questId);

	// bookkeeping
	void AddQuest(quest_t* pQuest);
	void AddQuestClue(questClue_t* pQuestClue);
	void AddQuestEvent(questEvent_t* pQuestEvent);

	// accessors
	bool AreQuestsInitializing() { return m_bQuestsInitlaizing; }

	// mutators
	void SetQuestsInitializing(bool bValue) { m_bQuestsInitlaizing = bValue; }

private:
	bool m_bQuestsInitlaizing;
	std::queue<QueuedQuestAction_t*> m_queuedQuestActions;
	std::vector<EZQuest_t*> m_EZQuests;
	std::map<std::string, quest_t*> m_quests;
	std::map<std::string, questClue_t*> m_questClues;
	std::map<std::string, questEvent_t*> m_questEvents;
};

#endif