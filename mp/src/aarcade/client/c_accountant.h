#ifndef C_ACCOUNTANT_H
#define C_ACCOUNTANT_H

#include <map>
#include <string>
#include <vector>
#include "../public/steam/steam_api.h"

enum aaStatType
{
	AASTAT_TYPE_NONE = 0,
	AASTAT_TYPE_INT = 1,
	AASTAT_TYPE_FLOAT = 2//,
	//AASTAT_TYPE_AVG = 3
};

struct aaStat {
	//int id;
	int xp;
	std::string name;
	std::string displayName;
	aaStatType type;
	bool incrementOnly;
	bool aggregated;

	int iValue;
	int iMaxChange;
	int iMinChange;
	int iMaxValue;
	int iDefaultValue;

	float flValue;
	float flMaxChange;
	float flMinChange;
	float flMaxValue;
	float flDefaultValue;

	//float flWindow;
	std::string data;
};


struct aaAchievement {
	//int id;
	int xp;
	std::string name;
	std::string stat;
	int statMin;
	int statMax;
	std::string displayName;
	std::string description;
	bool hidden;
	bool achieved;
	uint64 unlockTime;
	std::vector<int> progresses;
};

class C_Accountant
{
public:
	C_Accountant();
	~C_Accountant();

	void Init();
	//void OnUserStatsReceived(UserStatsReceived_t *pUserStatsReceived);
	void StoreStats();
	void OnStatsStored();

	void ResetStats(bool bAchievementsToo);
	
	void AutoStoreStats();

	void Action(std::string name, int iDeltaValue, std::string data = "");
	void Action(std::string name, float flDeltaValue, std::string data = "");
	void OnUserStatsReceived(UserStatsReceived_t* pCallback);

	aaStat* GetStat(std::string statName);
	void GetGlobalStatHistory(std::string statName, int iDays, std::vector<int64>& results);
	int64 GetGlobalStat(std::string statName);

	// accessors
	std::map<std::string, aaStat*> GetStats() { return m_stats; }
	std::map<std::string, aaAchievement*> GetAchievements() { return m_achievements; }
	//bool IsInitializing() { return m_bIsInitializing; }

	// mutators

private:
	float m_flLastStoredStatsTime;
	bool m_bReady;
	//bool m_bIsInitializing;
	std::map<std::string, aaStat*> m_stats;
	std::map<std::string, aaAchievement*> m_achievements;
	//STEAM_CALLBACK(C_Accountant, OnUserStatsReceived, UserStatsReceived_t);
};

#endif