#include "cbase.h"
#include "c_accountant.h"
#include "c_anarchymanager.h"
#include <vector>
#include "filesystem.h"
#include "clientsteamcontext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_Accountant::C_Accountant()
{
	m_bReady = false;
	//m_bIsInitializing = false;
}

C_Accountant::~C_Accountant()
{
}

void C_Accountant::Init()
{
	//m_bIsInitializing = true;

	// First load the stats & achievements schema
	KeyValues* pSchemaKV = new KeyValues("schema");
	if (!pSchemaKV->LoadFromFile(g_pFullFileSystem, "resource/aa_achievement_schema.txt", "MOD"))
	{
		Msg("ERROR: Failed to load resource/aa_achievement_schema.txt!\n");
		return;
	}

	m_flLastStoredStatsTime = 0.0f;

	int iStatType;
	aaStat* pStat;
	ConVar* pConVar;
	KeyValues* pStatsKV = pSchemaKV->FindKey("stats");
	for (KeyValues *pStatKV = pStatsKV->GetFirstSubKey(); pStatKV; pStatKV = pStatKV->GetNextKey())
	{
		pStat = new aaStat();
		pStat->name = pStatKV->GetString("name", "");

		pConVar = cvar->FindVar(VarArgs("data_%s", pStat->name.c_str()));
		pStat->data = (pConVar) ? pConVar->GetString() : "";

		std::string statType = pStatKV->GetString("type", "");
		if (statType == "int")
		{
			pStat->type = AASTAT_TYPE_INT;
			pStat->iDefaultValue = pStatKV->GetInt("defaultValue", -1);
			pStat->iValue = pStat->iDefaultValue;
			pStat->iMaxChange = pStatKV->GetInt("maxChange", -1);
			pStat->iMinChange = pStatKV->GetInt("minChange", -1);
			pStat->iMaxValue = pStatKV->GetInt("maxValue", -1);
		}
		else if (statType == "float")
		{
			pStat->type = AASTAT_TYPE_FLOAT;
			pStat->flDefaultValue = pStatKV->GetFloat("defaultValue", -1);
			pStat->flValue = pStat->flDefaultValue;
			pStat->flMaxChange = pStatKV->GetFloat("maxChange", -1);
			pStat->flMinChange = pStatKV->GetFloat("minChange", -1);
			pStat->flMaxValue = pStatKV->GetFloat("maxValue", 1);
		}
		//else if (statType == "avg")
		//{
			//	pStat->type = AASTAT_TYPE_AVG;
		//}
		else
		{
			pStat->type = AASTAT_TYPE_NONE;
			delete pStat;
			pStat = null;
			continue;
		}

		//pStat->flWindow = pStatKV->GetFloat("window", -1);
		pStat->aggregated = pStatKV->GetBool("aggregated", false);
		pStat->incrementOnly = pStatKV->GetBool("incrementOnly", false);
		pStat->displayName = pStatKV->GetString("displayName", "Unnamed");
		pStat->xp = pStatKV->GetInt("xp", 0);
		//pStat->id = pStatKV->GetInt("id", -1);

		m_stats[pStat->name] = pStat;
	}

	std::string progresses;
	aaAchievement* pAchievement;
	KeyValues* pAchievementsKV = pSchemaKV->FindKey("achievements");
	for (KeyValues *pAchievementKV = pAchievementsKV->GetFirstSubKey(); pAchievementKV; pAchievementKV = pAchievementKV->GetNextKey())
	{
		pAchievement = new aaAchievement();
		pAchievement->achieved = false;
		pAchievement->unlockTime = 0;

		pAchievement->name = pAchievementKV->GetString("name", "");
		pAchievement->description = pAchievementKV->GetString("description", "");
		pAchievement->displayName = pAchievementKV->GetString("displayName", "");
		//pAchievement->id = pAchievementKV->GetInt("id", 0);
		pAchievement->stat = pAchievementKV->GetString("stat", "");
		pAchievement->statMin = pAchievementKV->GetInt("statMin", 0);
		pAchievement->statMax = pAchievementKV->GetInt("statMax", 1);
		pAchievement->xp = pAchievementKV->GetInt("xp", 0);
		pAchievement->hidden = pAchievementKV->GetBool("hidden", false);

		progresses = pAchievementKV->GetString("progresses", "");

		std::vector<std::string> progressesTokens;
		g_pAnarchyManager->Tokenize(progresses, progressesTokens, ", ");
		for (unsigned int i = 0; i < progressesTokens.size(); i++)
			pAchievement->progresses.push_back(Q_atoi(progressesTokens[i].c_str()));

		m_achievements[pAchievement->name] = pAchievement;
	}

	//steamapicontext->SteamUserStats()->RequestCurrentStats();
	// Now grab the user's actual stat & achievement values from the Steam servers...
	//if (!steamapicontext->SteamUserStats()->RequestCurrentStats())
	//{
	//	Msg("ERROR: Could not retrieve stat info.\n");
	//	return;
	//}
	
	UserStatsReceived_t* pUserStatsReceived = new UserStatsReceived_t();
	pUserStatsReceived->m_eResult = k_EResultOK;
	pUserStatsReceived->m_steamIDUser = steamapicontext->SteamUser()->GetSteamID();
	this->OnUserStatsReceived(pUserStatsReceived);
	delete pUserStatsReceived;
}

void C_Accountant::OnUserStatsReceived(UserStatsReceived_t *pCallback)
{
	if (pCallback->m_eResult != k_EResultOK)
	{
		DevMsg("ERROR: Failed to receive user stats.\n");
		return;
	}

	if (!m_bReady && pCallback->m_steamIDUser == steamapicontext->SteamUser()->GetSteamID())
	{
		// Now iterate through the stats and put the user's real data into the local data structs
		auto stat = m_stats.begin();
		while (stat != m_stats.end())
		{
			if (stat->second->type == AASTAT_TYPE_INT)
			{
				int iValue;
				if (steamapicontext->SteamUserStats()->GetStat(stat->second->name.c_str(), &iValue))
					stat->second->iValue = iValue;
			}
			else if (stat->second->type == AASTAT_TYPE_FLOAT)
			{
				float flValue;
				if (steamapicontext->SteamUserStats()->GetStat(stat->second->name.c_str(), &flValue))
					stat->second->flValue = flValue;
			}
			stat++;
		}

		// Do the same for the achievements too
		auto it = m_achievements.begin();
		while (it != m_achievements.end())
		{
			bool bAchieved;
			unsigned int unUnlockTime;
			if (steamapicontext->SteamUserStats()->GetAchievementAndUnlockTime(it->second->name.c_str(), &bAchieved, &unUnlockTime))
			{
				it->second->achieved = bAchieved;
				it->second->unlockTime = (uint64)(unUnlockTime);
				// NOTE: Maybe use GetAchievementDisplayAttribute to get the actual description & stuff from the server.
			}
			it++;
		}

		m_bReady = true;
		g_pAnarchyManager->OnAccountantReady();
	}
}


void C_Accountant::GetGlobalStatHistory(std::string statName, int iDays, std::vector<int64>& results)
{
	if (iDays > 60)	// 60 is API max
		return;
	
	//DevMsg("Yadda: %s %i\n", statName.c_str(), iDays);
	//uint32 size = 3;
	//int64 data[3];
	int64* data = new int64[iDays];
	//int32 iCount = steamapicontext->SteamUserStats()->GetGlobalStatHistory(statName.c_str(), &data[0], sizeof(int64) * iDays);
	int32 iCount = steamapicontext->SteamUserStats()->GetGlobalStatHistory(statName.c_str(), data, sizeof(int64) * iDays);
	//DevMsg("Count vs size is: %i vs %i\n", iCount, iDays);
	for (int32 i = 0; i < iCount; i++)
	{
		int64 val = data[i];
		results.push_back(val);
	}

	delete[] data;
}

int64 C_Accountant::GetGlobalStat(std::string statName)
{
	int64 data;
	if (!steamapicontext->SteamUserStats()->GetGlobalStat(statName.c_str(), &data))
		return 0;
	
	return data;
}


aaStat* C_Accountant::GetStat(std::string statName)
{
	auto it = m_stats.find(statName);
	if (it != m_stats.end())
		return it->second;
	return null;
}

void C_Accountant::StoreStats()
{
	if (!m_bReady)
		return;

	if (!steamapicontext->SteamUserStats()->StoreStats())
		Msg("ERROR: Could not save stats.\n");
	else
		DevMsg("Storing stats...\n");

	m_flLastStoredStatsTime = engine->Time();
}

void C_Accountant::OnStatsStored()
{
	if (!m_bReady)
		return;

	DevMsg("Stats stored successfully.\n");
}

void C_Accountant::ResetStats(bool bAchievementsToo)
{
	g_pAnarchyManager->ResetStatTimers();

	ConVar* pConVar = cvar->FindVar("data_aa_npcs_used");
	if (pConVar)
		pConVar->SetValue("");
	pConVar = cvar->FindVar("data_aa_maps_discovered");
	if (pConVar)
		pConVar->SetValue("");
	pConVar = cvar->FindVar("data_aa_props_used");
	if (pConVar)
		pConVar->SetValue("");
	pConVar = cvar->FindVar("data_aa_letters_used");
	if (pConVar)
		pConVar->SetValue("");

	aaStat* pStat;
	auto it = m_stats.begin();
	while (it != m_stats.end())
	{
		pStat = it->second;
		pStat->flValue = 0;
		pStat->iValue = 0;
		pStat->data = "";

		if ( pStat->type == AASTAT_TYPE_INT)
			steamapicontext->SteamUserStats()->SetStat(pStat->name.c_str(), pStat->iDefaultValue);
		else if (pStat->type == AASTAT_TYPE_FLOAT)
			steamapicontext->SteamUserStats()->SetStat(pStat->name.c_str(), pStat->flDefaultValue);

		it++;
	}

	if (bAchievementsToo)
	{
		aaAchievement* pAchievement;
		auto it = m_achievements.begin();
		while (it != m_achievements.end())
		{
			pAchievement = it->second;
			pAchievement->achieved = false;
			pAchievement->unlockTime = 0;
			steamapicontext->SteamUserStats()->ClearAchievement(pAchievement->name.c_str());

			it++;
		}

		g_pAnarchyManager->AddToastMessage("All Stats & Achievements Reset");
	}
	else
		g_pAnarchyManager->AddToastMessage("All Stats Reset");

	//if (!steamapicontext->SteamUserStats()->ResetAllStats(bAchievementsToo)){
	//	DevMsg("ERROR resetting stats.\n");
}

void C_Accountant::Action(std::string name, int iDeltaValue, std::string data)
{
	if (g_pAnarchyManager->GetState() != AASTATE_RUN)
		return;

	auto it = m_stats.find(name);
	if (it != m_stats.end())
	{
		aaStat* pStat = it->second;
		if (data != "" && pStat->data != "")
		{
			std::vector<std::string> dataTokens;
			g_pAnarchyManager->Tokenize(pStat->data, dataTokens, "::");
			for (unsigned int i = 0; i < dataTokens.size(); i++)
			{
				if (dataTokens[i] == data)
					return;
			}
		}
		
		if ((pStat->iMaxChange == -1 || iDeltaValue <= pStat->iMaxChange) && (pStat->iMinChange == -1 || iDeltaValue >= pStat->iMinChange) && (pStat->iMaxValue == -1 || pStat->iValue + iDeltaValue <= pStat->iMaxValue) && (!pStat->incrementOnly || pStat->iValue + iDeltaValue > pStat->iValue))
		{
			pStat->iValue += iDeltaValue;

			if (data != "")
			{
				if (pStat->data != "")
					pStat->data += "::";
				pStat->data += data;
			}

			steamapicontext->SteamUserStats()->SetStat(pStat->name.c_str(), pStat->iValue);

			// find any achievement based on this stat
			aaAchievement* pAchievement;
			auto it = m_achievements.begin();
			while (it != m_achievements.end())
			{
				if (it->second->stat == pStat->name)
				{
					pAchievement = it->second;

					if (!pAchievement->achieved && pStat->iValue >= pAchievement->statMax)
					{
						pAchievement->achieved = true;
						steamapicontext->SteamUserStats()->SetAchievement(pAchievement->name.c_str());
						this->StoreStats();
						pAchievement->unlockTime = g_pAnarchyManager->GetTimeNumber();
					}
					else
					{
						for (unsigned int i = 0; i < pAchievement->progresses.size(); i++)
						{
							if (pAchievement->progresses[i] == pStat->iValue)
							{
								//DevMsg("\tStat: %s\n\tName: %s\n\tProgress: %i\n\tValue: %i\n", it->second->stat.c_str(), pStat->name.c_str(), pAchievement->progresses[i], pStat->iValue);
								steamapicontext->SteamUserStats()->IndicateAchievementProgress(pAchievement->name.c_str(), pStat->iValue, pAchievement->statMax);
								break;
							}
						}
					}
				}

				it++;
			}

			if (pStat->name != "aa_xp" && pStat->xp > 0)
				this->Action(std::string("aa_xp"), pStat->xp);
		}
	}

	this->AutoStoreStats();
}

void C_Accountant::AutoStoreStats()
{
	if (g_pAnarchyManager->IsPaused())
		return;

	float flTime = engine->Time();
	if (m_flLastStoredStatsTime != 0.0f && flTime - m_flLastStoredStatsTime < 120.0f)
		return;

	this->StoreStats();	// m_flLastStoredStatsTime is updated in here.
}

void C_Accountant::Action(std::string name, float flDeltaValue, std::string data)
{
	if (g_pAnarchyManager->GetState() != AASTATE_RUN)
		return;

	auto it = m_stats.find(name);
	if (it != m_stats.end())
	{
		aaStat* pStat = it->second;
		if (data != "" && pStat->data != "")
		{
			std::vector<std::string> dataTokens;
			g_pAnarchyManager->Tokenize(pStat->data, dataTokens, "::");
			for (unsigned int i = 0; i < dataTokens.size(); i++)
			{
				if (dataTokens[i] == data)
					return;
			}
		}

		if ((pStat->flMaxChange == -1 || flDeltaValue <= pStat->flMaxChange) && (pStat->flMinChange == -1 || flDeltaValue >= pStat->flMinChange) && (pStat->flMaxValue == -1 || pStat->flValue + flDeltaValue <= pStat->flMaxValue) && (!pStat->incrementOnly || pStat->flValue + flDeltaValue > pStat->flValue))
		{
			pStat->flValue += flDeltaValue;

			if (data != "")
			{
				if (pStat->data != "")
					pStat->data += "::";
				pStat->data += data;
			}

			steamapicontext->SteamUserStats()->SetStat(pStat->name.c_str(), pStat->flValue);

			if (pStat->name != "aa_xp" && pStat->xp > 0)
				this->Action(std::string("aa_xp"), pStat->xp);

			// now locally check if we reached an achievement so that we can give XP accordingly
			// TODO: work
		}
	}
}