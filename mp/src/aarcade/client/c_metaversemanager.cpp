#include "cbase.h"

#include "c_anarchymanager.h"
#include "aa_globals.h"
#include "aa_apikeys.h"
#include "c_metaversemanager.h"
#include "filesystem.h"
#include "vgui/IInput.h"
#include <algorithm>
#include "../../public/zip/XZip.h"
#include "../../public/zip/XUnzip.h"
//#include "zip_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void AddSubKeys2(KeyValues* kv, JSObject& object, bool bConvertPropertyNamesToLowercase = false)
{
	if (!kv)
		return;

	std::string lowerBuf;
	//bConvertPropertyNamesToLowercase
	for (KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (sub->GetFirstSubKey())
		{
			JSObject subObject;
			AddSubKeys2(sub, subObject, bConvertPropertyNamesToLowercase);

			if (bConvertPropertyNamesToLowercase && Q_strcmp(sub->GetName(), AA_PLATFORM_ID))	// don't lower-case the platform ID node!
			{
				lowerBuf = sub->GetName();
				std::transform(lowerBuf.begin(), lowerBuf.end(), lowerBuf.begin(), ::tolower);
				object.SetProperty(WSLit(lowerBuf.c_str()), subObject);
			}
			else
				object.SetProperty(WSLit(sub->GetName()), subObject);
		}
		else
		{
			if (bConvertPropertyNamesToLowercase)
			{
				lowerBuf = sub->GetName();
				std::transform(lowerBuf.begin(), lowerBuf.end(), lowerBuf.begin(), ::tolower);
				object.SetProperty(WSLit(lowerBuf.c_str()), WSLit(sub->GetString()));
			}
			else
				object.SetProperty(WSLit(sub->GetName()), WSLit(sub->GetString()));
		}
	}
}

C_MetaverseManager::C_MetaverseManager()
{
	DevMsg("MetaverseManager: Constructor\n");
	//m_pWebTab = null;
	m_pPreviousSearchInfo = null;
	m_pPreviousModelSearchInfo = null;
	m_pPreviousAppSearchInfo = null;
	m_pSpawningObject = null;
	m_pSpawningObjectEntity = null;
	m_iSpawningRotationAxis = 1;
	m_spawningAngles.x = 0;
	m_spawningAngles.y = 0;
	m_spawningAngles.z = 0;
	//m_bTwitchBotEnabled = cvar->FindVar("twitch_bot_enabled")->GetBool();//false;
	m_pTwitchBotEnabledConVar = cvar->FindVar("twitch_bot_enabled");
	m_pNodeModelConVar = cvar->FindVar("node_model");
	m_bTwitchChannelLive = false;

	m_bHasDisconnected = false;

	m_pPointWithinNodeConVar = null;

	m_bHostSessionNow = false;
	m_pLocalUser = null;

	m_fPresenceLastSynced = 0;

	m_pImportSteamGamesKV = null;
	m_pImportSteamGamesSubKV = null;

	m_pUseGlobalRotationConVar = cvar->FindVar("use_global_rotation");

	m_pUploadsLogKV = new KeyValues("uploads");

	m_libraryBrowserContextCategory = "items";
	m_libraryBrowserContextId = "";
	m_libraryBrowserContextSearch = "";
	m_libraryBrowserContextFilter = "";
	
	m_pPreviousLoadLocalItemLegacyBackpack = null;

	m_pDebugDisableMPModelsConVar = null;
	m_pDebugDisableMPPlayersConVar = null;
	m_pDebugDisableMPItemsConVar = null;

	m_defaultFields.push_back("title");
	m_defaultFields.push_back("description");
	m_defaultFields.push_back("file");
	m_defaultFields.push_back("type");
	m_defaultFields.push_back("app");
	m_defaultFields.push_back("reference");
	m_defaultFields.push_back("preview");
	m_defaultFields.push_back("download");
	m_defaultFields.push_back("stream");
	m_defaultFields.push_back("screen");
	m_defaultFields.push_back("marquee");
	m_defaultFields.push_back("model");

	m_pVolatileSavesKV = null;

	//m_uProcessBatchSize = 1;
	//m_uProcessCurrentCycle = 0;

	m_uNumSteamGamesToImport = 0;
	m_uNumSteamGamesToImported = 0;
	
	m_db = null;
	SetDefLessFunc(m_uploadBatches);

	m_blacklistedTitleMatches = "unknown, untitled, default, sample, image, video, steamuserimages-a, maxresdefault, giphy";
}

C_MetaverseManager::~C_MetaverseManager()
{
	DevMsg("MetaverseManager: Destructor\n");
	//m_pUploadsLogKV->deleteThis();	// FIXME: would this cause a crash?  its lingering around, so SeemsGood to clean it up here.

	m_mapScreenshots.clear();

	if (m_previousLoadLocalAppFilePath != "")
	{
		g_pFullFileSystem->FindClose(m_previousLoadLocalAppFileHandle);
		m_previousLoadLocalAppFilePath = "";
	}

	if (m_pPreviousSearchInfo)
		m_pPreviousSearchInfo->deleteThis();

	if (m_pPreviousModelSearchInfo)
		m_pPreviousModelSearchInfo->deleteThis();

	if (m_pPreviousAppSearchInfo)
		m_pPreviousAppSearchInfo->deleteThis();

	// m_apps
	while (!m_apps.empty())
	{
		m_apps.begin()->second->deleteThis();
		m_apps.erase(m_apps.begin());
	}

	// m_models
	while (!m_models.empty())
	{
		m_models.begin()->second->deleteThis();
		m_models.erase(m_models.begin());
	}

	// m_items
	while (!m_items.empty())
	{
		m_items.begin()->second->deleteThis();
		m_items.erase(m_items.begin());
	}

	// m_types
	while (!m_types.empty())
	{
		m_types.begin()->second->deleteThis();
		m_types.erase(m_types.begin());
	}

	// screenshots
	while (!m_mapScreenshots.empty())
	{
		m_mapScreenshots.begin()->second->deleteThis();
		m_mapScreenshots.erase(m_mapScreenshots.begin());
	}

	// upload batches
	this->DestroyAllUploadBatches();

	if (m_pImportSteamGamesKV)
	{
		m_pImportSteamGamesKV->deleteThis();
		m_pImportSteamGamesKV = null;
		m_pImportSteamGamesSubKV = null;
	}

	sqlite3_close(m_db);
	// TODO: error objects for sqlite must also be cleaned up
}

bool C_MetaverseManager::CreateDb(std::string libraryFile, sqlite3** pDb)
{
	// create or open the library.db
	//DevMsg("Opening (or creating) SQL DB at: %s\n", libraryFile.c_str());

	sqlite3* db;
	int rc = sqlite3_open(libraryFile.c_str(), &db);
	if (!db)
	{
		DevMsg("Critical error opening the specified SQLite3 database!\n");
		return false;
	}

	*pDb = db;
	return true;
}

bool C_MetaverseManager::IsEmptyDb(sqlite3** pDb)
{
	if (!pDb)
		pDb = &m_db;

	// no maps means empty library
	// TODO: Improve this check
	// confirm that default stuff exists
	bool bNeedsDefault = true;

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(*pDb, "SELECT * from  types", -1, &stmt, NULL);
	if (rc == SQLITE_OK)
	{
		bNeedsDefault = false;
		/*
		int length;
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			length = sqlite3_column_bytes(stmt, 1);

			if (length == 0)
			{
				DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
				continue;
			}

			//KeyValues* pMap = new KeyValues("map");

			//CUtlBuffer buf(0, length, 0);
			//buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
			//pMap->ReadAsBinary(buf);

			// TODO: Look up any alias here first!! (or maybe later.  later is probably OK.  later is probably better. way later.  later during library functions.)
			//pMap = this->GetActiveKeyValues(pMap);
			//if (pMap)
			//	bNeedsDefault = false;

			break;
		}
		*/
		sqlite3_finalize(stmt);
	}

	return bNeedsDefault;
}

void C_MetaverseManager::BeginTransaction()
{
	char *error;
	const char *sqlBeginTransaction = "BEGIN TRANSACTION;";
	int rc1 = sqlite3_exec(m_db, sqlBeginTransaction, NULL, NULL, &error);
	if (rc1 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(m_db));
		sqlite3_free(error);
	}
}

void C_MetaverseManager::CommitTransaction()
{
	char *error;
	const char *sqlBeginTransaction = "COMMIT;";
	int rc2 = sqlite3_exec(m_db, sqlBeginTransaction, NULL, NULL, &error);
	if (rc2 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(m_db));
		sqlite3_free(error);
	}
}

void C_MetaverseManager::AddDefaultTables(sqlite3** pDb)
{
	if (!pDb)
		pDb = &m_db;

	char *error;
	const char *sqlBeginTransaction = "BEGIN TRANSACTION;";
	int rc1 = sqlite3_exec(*pDb, sqlBeginTransaction, NULL, NULL, &error);
	if (rc1 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	// create the tables
	//char *error;
	const char *sqlCreateAppsTable = "CREATE TABLE apps (id TEXT PRIMARY KEY, value BLOB);";
	int rc = sqlite3_exec(*pDb, sqlCreateAppsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateItemsTable = "CREATE TABLE items (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreateItemsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateMapsTable = "CREATE TABLE maps (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreateMapsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateModelsTable = "CREATE TABLE models (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreateModelsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreatePlatformsTable = "CREATE TABLE platforms (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreatePlatformsTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateTypesTable = "CREATE TABLE types (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreateTypesTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateInstancesTable = "CREATE TABLE instances (id TEXT PRIMARY KEY, value BLOB);";
	rc = sqlite3_exec(*pDb, sqlCreateInstancesTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	const char *sqlCreateVersionTable = "CREATE TABLE version (id INTEGER PRIMARY KEY, value INTEGER);";
	rc = sqlite3_exec(*pDb, sqlCreateVersionTable, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	// now save our AA_LIBRARY_VERSION number to the DB for future proofing
	sqlite3_stmt *stmt = NULL;
	rc = sqlite3_prepare(*pDb, VarArgs("INSERT INTO version (id, value) VALUES(0, %i)", AA_LIBRARY_VERSION), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(*pDb));

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
		DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(*pDb));

	sqlite3_finalize(stmt);

	const char *sqlCommitTransaction = "COMMIT;";
	int rc2 = sqlite3_exec(*pDb, sqlCommitTransaction, NULL, NULL, &error);
	if (rc2 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}
}

addDefaultLibraryContext_t* C_MetaverseManager::GetAddDefaultLibraryContext()
{
	auto it = m_addDefaultLibraryContexts.begin();
	if( it != m_addDefaultLibraryContexts.end() )
		return it->first;
	
	return null;
}

void C_MetaverseManager::SetAddDefaultLibraryToDbIterativeContext(addDefaultLibraryContext_t* pContext)
{
	auto it = m_addDefaultLibraryContexts.find(pContext);
	if (it == m_addDefaultLibraryContexts.end())
		m_addDefaultLibraryContexts[pContext] = true;
}

bool C_MetaverseManager::DeleteAddDefaultLibraryContext(addDefaultLibraryContext_t* pContext)
{
	auto it = m_addDefaultLibraryContexts.find(pContext);
	if (it != m_addDefaultLibraryContexts.end())
	{
		m_addDefaultLibraryContexts.erase(it);
		delete pContext;
		return true;
	}

	return false;
}

/* STATES
	0 - uninitialized
	1 - finished
	2 - adding apps
	3 - adding cabinets
	4 - adding maps
	5 - adding models
	6 - adding types
*/
void C_MetaverseManager::AddDefaultLibraryToDbIterative(addDefaultLibraryContext_t* pContext)
{
	sqlite3** pDb = (pContext->pDb) ? pContext->pDb : &m_db;

	// get ready for apps
	if (pContext->state == 0)
	{
		pContext->pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\apps\\*.txt", "MOD", &pContext->handle);
		pContext->state = 2;
	}

	if (pContext->state == 2)
	{
		// 2 APPS
		while (pContext->pFilename != NULL && g_pFullFileSystem->FindIsDirectory(pContext->handle))
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);

		if (pContext->pFilename != NULL)
		{
			pContext->kv = new KeyValues("app");
			if (pContext->kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\apps\\%s", pContext->pFilename), "MOD"))
			{
				KeyValues* active = this->GetActiveKeyValues(pContext->kv);
				this->SaveSQL(pDb, "apps", active->GetString("info/id"), pContext->kv);
			}
			pContext->numApps++;
			pContext->kv->deleteThis();
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);
		}
		else
		{
			g_pFullFileSystem->FindClose(pContext->handle);

			// get ready for cabinets
			pContext->pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\cabinets\\*.txt", "MOD", &pContext->handle);
			pContext->state = 3;
		}
	}

	if (pContext->state == 3)
	{
		// 3 CABINETS
		while (pContext->pFilename != NULL && g_pFullFileSystem->FindIsDirectory(pContext->handle))
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);

		if (pContext->pFilename != NULL)
		{
			pContext->kv = new KeyValues("model");
			if (pContext->kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\cabinets\\%s", pContext->pFilename), "MOD"))
			{
				KeyValues* active = this->GetActiveKeyValues(pContext->kv);

				// generate an ID on the file name instead of trying to load the one saved in the txt file cuz of how shiddy KV's are at loading from txt files.
				std::string id = g_pAnarchyManager->GenerateLegacyHash(active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
				active->SetString("info/id", id.c_str());

				this->SaveSQL(pDb, "models", id.c_str(), pContext->kv);
			}
			pContext->numCabinets++;
			pContext->kv->deleteThis();
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);
		}
		else
		{
			g_pFullFileSystem->FindClose(pContext->handle);

			// get ready for maps
			pContext->pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\maps\\*.txt", "MOD", &pContext->handle);
			pContext->state = 4;
		}
	}

	if (pContext->state == 4)
	{
		// 4 MAPS
		while (pContext->pFilename != NULL && g_pFullFileSystem->FindIsDirectory(pContext->handle))
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);

		if (pContext->pFilename != NULL)
		{
			pContext->kv = new KeyValues("map");
			if (pContext->kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\maps\\%s", pContext->pFilename), "MOD"))
			{
				KeyValues* active = this->GetActiveKeyValues(pContext->kv);

				// generate an ID on the file name instead of trying to load the one saved in the txt file cuz of how shiddy KV's are at loading from txt files.
				std::string id = g_pAnarchyManager->GenerateLegacyHash(active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
				active->SetString("info/id", id.c_str());

				this->SaveSQL(pDb, "maps", active->GetString("info/id"), pContext->kv);
			}
			pContext->numMaps++;
			pContext->kv->deleteThis();
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);
		}
		else
		{
			g_pFullFileSystem->FindClose(pContext->handle);

			// get ready for models
			pContext->pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\models\\*.txt", "MOD", &pContext->handle);
			pContext->state = 5;
		}
	}

	if (pContext->state == 5)
	{
		// 5 MODELS
		while (pContext->pFilename != NULL && g_pFullFileSystem->FindIsDirectory(pContext->handle))
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);

		if (pContext->pFilename != NULL)
		{
			pContext->kv = new KeyValues("model");
			if (pContext->kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\models\\%s", pContext->pFilename), "MOD"))
			{
				KeyValues* active = this->GetActiveKeyValues(pContext->kv);

				// generate an ID on the file name instead of trying to load the one saved in the txt file cuz of how shiddy KV's are at loading from txt files.
				std::string id = g_pAnarchyManager->GenerateLegacyHash(active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
				active->SetString("info/id", id.c_str());

				this->SaveSQL(pDb, "models", active->GetString("info/id"), pContext->kv);
			}
			pContext->numModels++;
			pContext->kv->deleteThis();
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);
		}
		else
		{
			g_pFullFileSystem->FindClose(pContext->handle);

			// get ready for types
			pContext->pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\types\\*.txt", "MOD", &pContext->handle);
			pContext->state = 6;
		}
	}

	if (pContext->state == 6)
	{
		// 6 TYPES
		while (pContext->pFilename != NULL && g_pFullFileSystem->FindIsDirectory(pContext->handle))
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);

		if (pContext->pFilename != NULL)
		{
			pContext->kv = new KeyValues("type");
			if (pContext->kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\types\\%s", pContext->pFilename), "MOD"))
			{
				KeyValues* active = this->GetActiveKeyValues(pContext->kv);
				this->SaveSQL(pDb, "types", active->GetString("info/id"), pContext->kv);
			}
			pContext->numTypes++;
			pContext->kv->deleteThis();
			pContext->pFilename = g_pFullFileSystem->FindNext(pContext->handle);
		}
		else
		{
			g_pFullFileSystem->FindClose(pContext->handle);

			// finished
			pContext->state = 1;
		}
	}
}

void C_MetaverseManager::AddDefaultLibraryToDb(unsigned int& numApps, unsigned int& numCabinets, unsigned int& numModels, unsigned int& numTypes, sqlite3** pDb)
{
	// OBSOLETE!!
	if (!pDb)
		pDb = &m_db;
	
	char *error;
	const char *sqlBeginTransaction = "BEGIN TRANSACTION;";
	int rc = sqlite3_exec(*pDb, sqlBeginTransaction, NULL, NULL, &error);
	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}

	// NOW LOAD IN ALL DEFAULTLIBRARY KEYVALUES & ADD THEM TO THE LIBRARY (and copy scraper.js files into the right folder)
	FileFindHandle_t handle;
	const char *pFilename;
	KeyValues* kv;

	// APPS
	pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\apps\\*.txt", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		kv = new KeyValues("app");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\apps\\%s", pFilename), "MOD"))
		{
			KeyValues* active = this->GetActiveKeyValues(kv);
			this->SaveSQL(pDb, "apps", active->GetString("info/id"), kv);
			numApps++;
		}
		kv->deleteThis();

		pFilename = g_pFullFileSystem->FindNext(handle);
	}
	g_pFullFileSystem->FindClose(handle);

	// CABINETS
	pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\cabinets\\*.txt", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		kv = new KeyValues("model");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\cabinets\\%s", pFilename), "MOD"))
		{
			KeyValues* active = this->GetActiveKeyValues(kv);
			DevMsg("Saving cabinet w/ title %s and id %s\n", active->GetString("title"), active->GetString("info/id"));
			this->SaveSQL(pDb, "models", active->GetString("info/id"), kv);
			numCabinets++;
		}
		kv->deleteThis();

		pFilename = g_pFullFileSystem->FindNext(handle);
	}
	g_pFullFileSystem->FindClose(handle);

	// MAPS
	pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\maps\\*.txt", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		kv = new KeyValues("map");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\maps\\%s", pFilename), "MOD"))
		{
			KeyValues* active = this->GetActiveKeyValues(kv);
			this->SaveSQL(pDb, "maps", active->GetString("info/id"), kv);
		}
		kv->deleteThis();

		pFilename = g_pFullFileSystem->FindNext(handle);
	}
	g_pFullFileSystem->FindClose(handle);

	// MODELS
	pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\models\\*.txt", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		kv = new KeyValues("model");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\models\\%s", pFilename), "MOD"))
		{
			KeyValues* active = this->GetActiveKeyValues(kv);
			this->SaveSQL(pDb, "models", active->GetString("info/id"), kv);
			numModels++;
		}
		kv->deleteThis();

		pFilename = g_pFullFileSystem->FindNext(handle);
	}
	g_pFullFileSystem->FindClose(handle);

	// TYPES
	pFilename = g_pFullFileSystem->FindFirstEx("defaultLibrary\\types\\*.txt", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		kv = new KeyValues("type");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\types\\%s", pFilename), "MOD"))
		{
			KeyValues* active = this->GetActiveKeyValues(kv);
			this->SaveSQL(pDb, "types", active->GetString("info/id"), kv);
			numTypes++;
		}
		kv->deleteThis();

		pFilename = g_pFullFileSystem->FindNext(handle);
	}
	g_pFullFileSystem->FindClose(handle);

	// SCRAPERS
	// TODO: work

	const char *sqlCommitTransaction = "COMMIT;";
	int rc2 = sqlite3_exec(*pDb, sqlCommitTransaction, NULL, NULL, &error);
	if (rc2 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}
}

void C_MetaverseManager::Init()
{
	m_pCloudAssetsUploadConVar = cvar->FindVar("cloud_assets_upload");
	m_pPointWithinNodeConVar = cvar->FindVar("point_within_node");
	m_pDebugDisableMPModelsConVar = cvar->FindVar("debug_disable_mp_models");
	m_pDebugDisableMPPlayersConVar = cvar->FindVar("debug_disable_mp_players");
	m_pDebugDisableMPItemsConVar = cvar->FindVar("debug_disable_mp_items");

	m_pVolatileSavesKV = new KeyValues("volatile");
	this->CreateDb("aarcade_user/library.db", &m_db);

	this->LoadTwitchConfig();
}

int C_MetaverseManager::ExtractLibraryVersion(sqlite3** pDb)
{
	if (!pDb)
		pDb = &m_db;

	int value = -1;

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(*pDb, "SELECT * from version WHERE id = 0", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(*pDb));
	else if (sqlite3_step(stmt) == SQLITE_ROW)
		value = sqlite3_column_int(stmt, 1);

	sqlite3_finalize(stmt);
	return value;
}

/*
void C_MetaverseManager::OnWebTabCreated(C_WebTab* pWebTab)
{
	m_pWebTab = pWebTab;
}
*/

unsigned int C_MetaverseManager::GetLibraryDbSize()
{
	return g_pFullFileSystem->Size("library.db", "DEFAULT_WRITE_PATH");
}

bool C_MetaverseManager::CreateLibraryBackup()
{
	g_pAnarchyManager->AddToastMessage("Libary Backup Created");

	// make sure the backups folder exists
	g_pFullFileSystem->CreateDirHierarchy("backups", "DEFAULT_WRITE_PATH");

	// find a suitable filename
	unsigned int backupNumber = 0;
	std::string backupFile = "backups/library-auto" + std::string(VarArgs("%u", backupNumber)) + ".db";

	while (g_pFullFileSystem->FileExists(backupFile.c_str(), "DEFAULT_WRITE_PATH"))
	{
		backupNumber++;
		backupFile = "backups/library-auto" + std::string(VarArgs("%u", backupNumber)) + ".db";
	}

	// copy the library.db file to it
	CUtlBuffer buf;
	if (g_pFullFileSystem->ReadFile("library.db", "DEFAULT_WRITE_PATH", buf))
		g_pFullFileSystem->WriteFile(backupFile.c_str(), "DEFAULT_WRITE_PATH", buf);
	buf.Purge();

	return true;
}

bool C_MetaverseManager::ConvertLibraryVersion(unsigned int uOld, unsigned int uTarget)
{
	// 1. Make a backup of library.db to aarcade_user/backups/library-auto0.db
	// 2. The library is already loaded, so it just needs to be converted right now.
	// 3. The oldest conversion needed right now is from version -1/0 to version 1.

	// always create a backup of library.db

	this->CreateLibraryBackup();

	// now determine which conversion to do
	if (uOld == 0 && uTarget == 1)
	{
		// VERSION 0 TO VERSION 1:
		/*
		- Instances need to re-structure their info node to be info/local/FIELD
		- The platforms key is now stored under the info/local key as well, it is no longer a sibling to generation.
		- Instances converted from Version 0 to Version 1 are no longer considered or tagged as Legacy because Version 0 stuff tried to resolve item & model ID's upon addon consumption rather than on instance load.  Their info needs to be restructured still, but leave their resolved item & model ID's alone.
		- Library conversion should be done IMMEDIATELY upon any outdated library.db that gets loaded.
		- library.db files NEED to have their Library Version saved to their header, or be assumed Version 0.
		- Library Version should NOT change EVER, but ESPECIALLY after Redux is out of beta.
		- Conversion between Library Versions should be ABNORMAL behavior and only really needed by beta testers who don't want to lose their saves.
		*/

		// Alright, a backup has been made.  Time to start converting.
		std::vector<std::string> badInstanceIds;
		std::vector<std::string> instanceIds;
		sqlite3* pDb = m_db;
		//unsigned int count = 0;

		sqlite3_stmt *stmtSelAll = NULL;
		int rc = sqlite3_prepare(pDb, "SELECT * from instances", -1, &stmtSelAll, NULL);
		if (rc != SQLITE_OK)
			DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

		while (sqlite3_step(stmtSelAll) == SQLITE_ROW)
			instanceIds.push_back(std::string((const char*)sqlite3_column_text(stmtSelAll, 0)));
		sqlite3_finalize(stmtSelAll);

		for (unsigned int i = 0; i < instanceIds.size(); i++)
		{
			sqlite3_stmt *stmtSel = NULL;
			int rc = sqlite3_prepare(pDb, VarArgs("SELECT * from instances WHERE id = \"%s\"", instanceIds[i].c_str()), -1, &stmtSel, NULL);
			if (rc != SQLITE_OK)
			{
				DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));
				sqlite3_finalize(stmtSel);
				continue;
			}

			if (sqlite3_step(stmtSel) != SQLITE_ROW)
			{
				DevMsg("warning: did now find a row. skipping.\n");
				sqlite3_finalize(stmtSel);
				continue;
			}

			std::string rowId = std::string((const char*)sqlite3_column_text(stmtSel, 0));
			//		DevMsg("Row ID is: %s\n", rowId.c_str());
			if (rowId == "")
			{
				sqlite3_finalize(stmtSel);
				continue;
			}

			int length = sqlite3_column_bytes(stmtSel, 1);
			if (length == 0)
			{
				DevMsg("WARNING: Zero-byte KeyValues detected as bad.\n");
				badInstanceIds.push_back(rowId);
				sqlite3_finalize(stmtSel);
				continue;
			}

			KeyValues* pInstance = new KeyValues("instance");

			CUtlBuffer buf(0, length, 0);
			buf.CopyBuffer(sqlite3_column_blob(stmtSel, 1), length);
			pInstance->ReadAsBinary(buf);

			// done with the select statement now
			sqlite3_finalize(stmtSel);
			buf.Purge();

			// instance is loaded & ready to convert
			KeyValues* oldInfoKV = pInstance->FindKey("info");
			if (!oldInfoKV)
			{
				// bogus save detected, clear it out... later
				badInstanceIds.push_back(rowId);
				pInstance->deleteThis();
				continue;
			}

			//KeyValues* infoKV = pInstance->FindKey("info/local", true);

			// copy subkeys from oldInfo to info
			for (KeyValues *sub = oldInfoKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (sub->GetFirstSubKey())
					continue;	// don't copy some garbage values that show up.

				pInstance->SetString(VarArgs("info/local/%s", sub->GetName()), sub->GetString());
			}

			/*
			generation
			info (parent)
			title
			map
			style
			creator
			id
			platforms (parent) (sometimes)
			objects (parent) (sometimes)
			*/

			DevMsg("Converted instance w/ id %s\n", rowId.c_str());// and info : \n", rowId.c_str());
			//for (KeyValues *sub = pInstance->FindKey("info/local")->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			//	DevMsg("\t%s: %s\n", sub->GetName(), sub->GetString());

			//// Remove the platforms tab cuz it shouldn't be there.
			// Copy workshopId and mountId from the platform keys, if needed
			KeyValues* platformsKV = pInstance->FindKey(VarArgs("platforms/%s", AA_PLATFORM_ID));
			if (platformsKV)
			{
				for (KeyValues *sub = platformsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					pInstance->SetString(VarArgs("info/local/platforms/%s/%s", AA_PLATFORM_ID, sub->GetName()), sub->GetString());
				}
			}

			std::vector<KeyValues*> targetSubKeys;
			// remove everything in the info key besides "local"
			for (KeyValues *sub = pInstance->FindKey("info")->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (Q_strcmp(sub->GetName(), "local"))
					targetSubKeys.push_back(sub);
			}

			KeyValues* pOldInfoKV = pInstance->FindKey("info");
			for (unsigned int j = 0; j < targetSubKeys.size(); j++)
			{
				pOldInfoKV->RemoveSubKey(targetSubKeys[j]);
				//pInstance->SetString("info", "");
			}
			targetSubKeys.clear();

			// then remove the old platforms key
			if (pInstance->FindKey("platforms"))
			{
				pInstance->RemoveSubKey(pInstance->FindKey("platforms"));
				//pInstance->SetString("platforms", "");
				platformsKV = null;
			}

			// done removing stuff.

			//DevMsg("And now...\n");
			//for (KeyValues *sub = pInstance->FindKey("info/local")->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			//	DevMsg("\t%s: %s\n", sub->GetName(), sub->GetString());

			// Now save the instance back out to the library.db
			sqlite3_stmt *stmtInst = NULL;
			rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO instances VALUES(\"%s\", ?)", rowId.c_str()), -1, &stmtInst, NULL);
			if (rc != SQLITE_OK)
			{
				DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));
				sqlite3_finalize(stmtInst);
			}
			else
			{
				// SQLITE_STATIC because the statement is finalized before the buffer is freed:
				CUtlBuffer instBuf;
				pInstance->WriteAsBinary(instBuf);

				int size = instBuf.Size();
				rc = sqlite3_bind_blob(stmtInst, 1, instBuf.Base(), size, SQLITE_STATIC);
				if (rc != SQLITE_OK)
					DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
				else
				{
					rc = sqlite3_step(stmtInst);
					if (rc != SQLITE_DONE)
					{
						if (rc == SQLITE_ERROR)
							DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
						else
							DevMsg("Weird other error!!\n");
					}
				}
				sqlite3_finalize(stmtInst);
				instBuf.Purge();
			}
			pInstance->deleteThis();
		}
		instanceIds.clear();

		// remove bad instances
		DevMsg("Removing %u bogus instances from old library.\n", badInstanceIds.size());
		for (unsigned int i = 0; i < badInstanceIds.size(); i++)
		{
			/*
			DELETE
			FROM
			artists_backup
			WHERE
			artistid = 1;
			*/

			sqlite3_stmt *stmtDel = NULL;
			rc = sqlite3_prepare(pDb, VarArgs("DELETE FROM instances WHERE id = \"%s\"", badInstanceIds[i].c_str()), -1, &stmtDel, NULL);
			if (rc != SQLITE_OK)
				DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

			rc = sqlite3_step(stmtDel);
			if (rc != SQLITE_DONE)
				DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));

			sqlite3_finalize(stmtDel);
		}
		badInstanceIds.clear();

		//DevMsg("ERROR: Could not find old info key!  Contents include:\n");
		//for (KeyValues *sub = pInstance->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		//	DevMsg("\t%s: %s\n", sub->GetName(), sub->GetString());

		// Now add the version table and set it to 1
		// now save our AA_LIBRARY_VERSION number to the DB for future proofing
		char *error;
		const char *sqlCreateVersionTable = "CREATE TABLE version (id INTEGER PRIMARY KEY, value INTEGER);";
		rc = sqlite3_exec(pDb, sqlCreateVersionTable, NULL, NULL, &error);
		if (rc != SQLITE_OK)
		{
			DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
			sqlite3_free(error);
			return false;
		}

		sqlite3_stmt *stmt3 = NULL;
		rc = sqlite3_prepare(pDb, "INSERT INTO version (id, value) VALUES(0, 1)", -1, &stmt3, NULL);
		if (rc != SQLITE_OK)
		{
			DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));
			return false;
		}

		rc = sqlite3_step(stmt3);
		if (rc != SQLITE_DONE)
		{
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
			return false;
		}

		sqlite3_finalize(stmt3);

		return true;
	}
	else if (uOld == 1 && uTarget == 2)
	{
		// VERSION 1 TO VERSION 2:
		/*
		- All tables that previously had their id field as datatype STRING should alter their datatype to TEXT: apps, items, maps, models, platforms, types, instances

		Note that this change was made because datatype STRING in SQLite would evaluate to INF for anything with ###e#### in its ID.
		*/

		// a backup has been made already, so let's get to work.

		// SQLite does **not** support changing data types on existing tables, so we have to:
		// - Create a NEW table w/ a temporary name.
		// - Copy everything into it from the old table.
		// - Clear out the old table.
		// - Drop the old table.
		// - Rename the NEW table to match the old table's name.
		// - Update the Library version number.

		sqlite3* pDb = m_db;

		std::vector<std::string> tableNames;
		tableNames.push_back("apps");
		tableNames.push_back("items");
		tableNames.push_back("maps");
		tableNames.push_back("models");
		tableNames.push_back("platforms");
		tableNames.push_back("types");
		tableNames.push_back("instances");

		char *error;
		int rc;
		for (unsigned int i = 0; i < tableNames.size(); i++)
		{
			// Create NEW table
			const char *sqlCreateTable = VarArgs("CREATE TABLE IF NOT EXISTS %s_TEMP (id TEXT PRIMARY KEY, value BLOB);", tableNames[i].c_str());
			rc = sqlite3_exec(pDb, sqlCreateTable, NULL, NULL, &error);
			if (rc != SQLITE_OK)
			{
				DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
				sqlite3_free(error);
				return false;
			}

			// Copy everything into it from the old table.
			const char *sqlCopyTable = VarArgs("REPLACE INTO %s_TEMP SELECT * FROM %s;", tableNames[i].c_str(), tableNames[i].c_str());
			//REPLACE INTO
			rc = sqlite3_exec(pDb, sqlCopyTable, NULL, NULL, &error);
			if (rc != SQLITE_OK)
			{
				DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
				sqlite3_free(error);
				return false;
			}

			/*
			// - Clear out the old table.
			sqlite3_stmt *stmtClearTable = NULL;
			rc = sqlite3_prepare(pDb, VarArgs("DELETE * FROM %s where 1=1", tableNames[i].c_str()), -1, &stmtClearTable, NULL);
			//DELETE * FROM table_name where 1=1;
			//DELETE * FROM
			if (rc != SQLITE_OK)
			{
				DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));
				return false;
			}

			rc = sqlite3_step(stmtClearTable);
			if (rc != SQLITE_DONE)
			{
				DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
				return false;
			}
			sqlite3_finalize(stmtClearTable);*/

			/*const char *sqlClearTable = VarArgs("DELETE * FROM %s;", tableNames[i].c_str());
			rc = sqlite3_exec(pDb, sqlClearTable, NULL, NULL, &error);
			if (rc != SQLITE_OK)
			{
				DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
				sqlite3_free(error);
				return false;
			}*/

			// - Drop the old table.
			const char *sqlDropTable = VarArgs("DROP TABLE %s;", tableNames[i].c_str());
			rc = sqlite3_exec(pDb, sqlDropTable, NULL, NULL, &error);
			if (rc != SQLITE_OK)
			{
				DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
				sqlite3_free(error);
				return false;
			}

			/*sqlite3_stmt *stmtDropTable = NULL;
			rc = sqlite3_prepare(pDb, VarArgs("DROP TABLE %s", tableNames[i].c_str()), -1, &stmtDropTable, NULL);
			//DELETE * FROM table_name where 1=1;
			//DELETE * FROM
			if (rc != SQLITE_OK)
			{
				DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));
				return false;
			}

			rc = sqlite3_step(stmtDropTable);
			if (rc != SQLITE_DONE)
			{
				DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
				return false;
			}
			sqlite3_finalize(stmtDropTable);*/

			// - Rename the NEW table to match the old table's name.
			const char *sqlRenameTable = VarArgs("ALTER TABLE %s_TEMP RENAME TO %s;", tableNames[i].c_str(), tableNames[i].c_str());
			rc = sqlite3_exec(pDb, sqlRenameTable, NULL, NULL, &error);
			if (rc != SQLITE_OK)
			{
				DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(pDb));
				sqlite3_free(error);
				return false;
			}
		}

		// - Update the Library version number.
		sqlite3_stmt *stmtLibraryVersion = NULL;
		rc = sqlite3_prepare(pDb, "REPLACE INTO version (id, value) VALUES(0, 2)", -1, &stmtLibraryVersion, NULL);
		if (rc != SQLITE_OK)
		{
			DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));
			return false;
		}

		rc = sqlite3_step(stmtLibraryVersion);
		if (rc != SQLITE_DONE)
		{
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
			return false;
		}
		sqlite3_finalize(stmtLibraryVersion);

		// and now we are done.
		return true;
	}
						// NOTE: Is this outdated info here?  This library version was never implemented at all, and version 2 instead went along with the implementation above instead.
						// I think they were the ORIGINAL notes, and I was presuming the OLD version would be 1 (but it turned out to be 0.)  So most likely all obsolete comments.
						// (Plus it seems to be a weird mashup of what version 0 to version 1 as defined as addressing!)
						//else if (uOld == 1 && uTarget == 2)
						//{
							// VERSION 1 TO VERSION 2:
							/*
							- There is now a default NODE type
							- The platforms key is now stored under the info/local key as well, it is no longer a sibling to generation.
							- Instances converted from Version 0 to Version 1 are no longer considered or tagged as Legacy because Version 0 stuff tried to resolve item & model ID's upon addon consumption rather than on instance load.  Their info needs to be restructured still, but leave their resolved item & model ID's alone.
							- Library conversion should be done IMMEDIATELY upon any outdated library.db that gets loaded.
							- library.db files NEED to have their Library Version saved to their header, or be assumed Version 0.
							- Library Version should NOT change EVER, but ESPECIALLY after Redux is out of beta.
							- Conversion between Library Versions should be ABNORMAL behavior and only really needed by beta testers who don't want to lose their saves.
							*/
						//	return true;
						//}
	else
		DevMsg("ERROR: Unknown library conversion values!\n");

	return false;
}

void C_MetaverseManager::Update()
{
	if (m_bHasDisconnected && !g_pAnarchyManager->IsPaused() && !engine->IsPaused())
	{
		this->RestartNetwork();
		return;
	}

	bool bNetworkUpdateAvailable = true;
	// handle any pending user updates
	auto it = m_pendingUserUpdates.begin();
	if (it != m_pendingUserUpdates.end())
	{
		if (this->ProcessUserSessionUpdate(it->second))
		{
			m_pendingUserUpdates.erase(it);
			bNetworkUpdateAvailable = false;
		}
	}

	if (bNetworkUpdateAvailable)
	{
		// handle any pending object updates
		auto it = m_pendingObjectUpdates.begin();
		if (it != m_pendingObjectUpdates.end())
		{
			if (this->ProcessObjectUpdate(it->second))
			{
				m_pendingObjectUpdates.erase(it);
				bNetworkUpdateAvailable = false;
			}
		}
	}

	if (m_bHostSessionNow)
		this->HostSessionNow();

	// thats all we do if we're paused.
	if (g_pAnarchyManager->IsPaused() || engine->IsPaused() )
		return;

	// FIXME: all this spawning object stuff should be done in instance manager, not here....
	if (m_pSpawningObjectEntity)
	{
		// Figure out where to place it
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		//Vector forward;
		//pPlayer->EyeVectors(&forward);

		transform_t* pTransform = g_pAnarchyManager->GetInstanceManager()->GetTransform();
		Vector origin;
		QAngle angles;
		if (!g_pAnarchyManager->IsVRActive() || !g_pAnarchyManager->IsHandTrackingActive())
		{
			/*
			trace_t tr;
			g_pAnarchyManager->SelectorTraceLine(tr);
			//UTIL_TraceLine(pPlayer->EyePosition(),
			//	pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_NPCSOLID,
			//	pPlayer, COLLISION_GROUP_NONE, &tr);

			// No hit? We're done.
			if (tr.fraction == 1.0)
				return;
			*/

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
				Vector ItemToPlayer;
				VectorSubtract(pPlayer->GetAbsOrigin(), Vector(endpos.x, endpos.y, endpos.z), ItemToPlayer);

				xaxis = Vector(ItemToPlayer.x, ItemToPlayer.y, ItemToPlayer.z);

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

			MatrixToAngles(entToWorld, angles);

			//		UTIL_SetOrigin(m_pSpawningObjectEntity, tr.endpos);
			//		m_pSpawningObjectEntity->SetAbsAngles(angles);

			// WORKING!! Rotate based on how much player has turned since last frame
			/* disabled for now cuz of transform menu
			if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
			{
			if (vgui::input()->IsKeyDown(KEY_E))
			{
			int x, y;
			vgui::input()->GetCursorPos(x, y);

			switch (m_iSpawningRotationAxis)
			{
			case 0:
			m_spawningAngles.x += x - ScreenWidth() / 2;
			break;

			case 1:
			m_spawningAngles.y += x - ScreenWidth() / 2;
			break;

			case 2:
			m_spawningAngles.z += x - ScreenWidth() / 2;
			break;
			}
			}

			angles.x += m_spawningAngles.x;
			angles.y += m_spawningAngles.y;
			angles.z += m_spawningAngles.z;
			}
			*/

			origin = endpos;
		}

		if (g_pAnarchyManager->IsVRActive() && g_pAnarchyManager->IsHandTrackingActive() && g_pAnarchyManager->GetVRPointer(1))
		{
			//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			//Vector playerOrigin = pPlayer->GetAbsOrigin();
			//QAngle playerAngles = pPlayer->GetAbsAngles();
			//VMatrix playerMatrix;
			//playerMatrix.SetupMatrixOrgAngles(playerOrigin, playerAngles);

			Vector handOrigin = g_pAnarchyManager->GetVRPointer(1)->GetAbsOrigin();
			QAngle handAngles = g_pAnarchyManager->GetVRPointer(1)->GetAbsAngles();
			VMatrix handMatrix;
			handMatrix.SetupMatrixOrgAngles(handOrigin, handAngles);

			VMatrix offsetMatrix;
			offsetMatrix.SetupMatrixOrgAngles(Vector(pTransform->offX, pTransform->offY, pTransform->offZ), QAngle(pTransform->rotP, pTransform->rotY, pTransform->rotR));

			handMatrix = handMatrix * offsetMatrix;

			QAngle goodAngles;
			MatrixAngles(handMatrix.As3x4(), goodAngles);
			Vector goodOffset = handMatrix.GetTranslation();

			// add in the current transform
			origin = goodOffset;
			angles = goodAngles;
		}
		else
		{
			// add in the current transform
			origin.x += pTransform->offX;
			origin.y += pTransform->offY;
			origin.z += pTransform->offZ;

			if (m_pUseGlobalRotationConVar->GetBool())
			{
				angles.x = pTransform->rotP;
				angles.y = pTransform->rotY;
				angles.z = pTransform->rotR;
			}
			else
			{
				angles.x += pTransform->rotP;
				angles.y += pTransform->rotY;
				angles.z += pTransform->rotR;
			}
		}

		// check for nodes
		C_BaseEntity* pNodeInfoEntity = null;
		KeyValues* pItemKV = this->GetActiveKeyValues(this->GetLibraryEntry("items", m_pSpawningObjectEntity->GetItemId()));
		if (pItemKV)
		{
			instance_t* pNodeInstance = g_pAnarchyManager->GetInstanceManager()->FindInstance(pItemKV->GetString("file"));
			if (pNodeInstance)
			{
				engine->ClientCmd(VarArgs("setifpointnode \"%s\" %f %f %f", pNodeInstance->style.c_str(), origin.x, origin.y, origin.z));//m_pSpawningObjectEntity->entindex()));	// this causes point_within_node to be set to -1 or an entindex.

				if (m_pPointWithinNodeConVar->GetInt() >= 0)
				{
					pNodeInfoEntity = C_BaseEntity::Instance(m_pPointWithinNodeConVar->GetInt());
					if (pNodeInfoEntity)
					{
						// make sure there's no object already existing at this position.
						object_t* pOriginalObject = new object_t();//g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(m_pSpawningObjectEntity->GetObjectId());
						pOriginalObject->angles = pNodeInfoEntity->GetAbsAngles();
						pOriginalObject->origin = pNodeInfoEntity->GetAbsOrigin();
						pOriginalObject->child = false;
						pOriginalObject->created = 0;
						pOriginalObject->entityIndex = -1;
						pOriginalObject->modified = 0;
						pOriginalObject->parentEntityIndex = 0;
						pOriginalObject->removed = 0;
						pOriginalObject->scale = 1.0f;
						pOriginalObject->slave = false;
						pOriginalObject->body = 0;
						pOriginalObject->skin = 0;
						pOriginalObject->spawned = false;
						pOriginalObject->state = 0;

						object_t* pNearestObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToObject("next", pOriginalObject, pOriginalObject);
						if (pNearestObject && pNearestObject->origin.DistTo(pOriginalObject->origin) < 0.1 && pNearestObject != g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(m_pSpawningObjectEntity->GetObjectId()))
							pNodeInfoEntity = null;

						delete pOriginalObject;
					}
				}
			}
		}
		
		if (!pNodeInfoEntity)
		{
			// FIXME: that stuff should be done server-side so collision boxes are updated properly... HUH? wut stuff?  Bad comment award.  Hopefully just obsolete. (i think i was talking about scale)


			//m_pSpawningObjectEntity->SetCollisionGroup(COLLISION_GROUP_NONE);//>SetSolid(SOLID_NONE);
			//m_pSpawningObjectEntity->SetSolid(SOLID_NONE);
			engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f \"%s\";\n", m_pSpawningObjectEntity->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z, ""));	// servercmdfix , false);
		}
		else
		{
			// snap to nodeinfoentity instead
			origin = pNodeInfoEntity->GetAbsOrigin();
			angles = pNodeInfoEntity->GetAbsAngles();
			engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f \"%s\";\n", m_pSpawningObjectEntity->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z, ""));	// servercmdfix , false);
		}

		float currentScale = m_pSpawningObjectEntity->GetModelScale();
		if (abs(pTransform->scale - currentScale) > 0.04)
			g_pAnarchyManager->GetMetaverseManager()->SetObjectScale(g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity(), pTransform->scale);
	}

	//C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected) // && pNetworkBrowserInstance
	{
		//uint64 currentTime = g_pAnarchyManager->GetTimeNumber();
		//uint64 presenceSyncInterval = 300;

		float fCurrentTime = engine->Time();
		float fPresenceSyncInterval = 0.5;

		if (m_fPresenceLastSynced == 0 || fCurrentTime - m_fPresenceLastSynced >= fPresenceSyncInterval)
		{
			m_fPresenceLastSynced = fCurrentTime;
			//g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
			this->PerformLocalPlayerUpdate();
		}
	}

	if (m_avatarDeathList.size() > 0)
	{
		C_DynamicProp* pVictimProp = m_avatarDeathList.back();
		m_avatarDeathList.pop_back();

		engine->ClientCmd(VarArgs("removeobject %i;\n", pVictimProp->entindex()));	// servercmdfix , false);
	}

	if (m_nextUploadBatchAddMap != "")
	{
		this->AddSingleMapToUploadBatch(m_nextUploadBatchAddMap, "");
		m_nextUploadBatchAddMap = "";
	}
	else if (m_nextUploadBatchAddObject != "")
	{
		this->AddSingleObjectToUploadBatch(m_nextUploadBatchAddObject, "");
		m_nextUploadBatchAddObject = "";
	}

	if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && !g_pAnarchyManager->GetConnectedUniverse()->isHost && m_nowReadyAssets.size() > 0)
	{
		bool bSkippedTarget;
		for (unsigned int i = 0; i < m_nowReadyAssets.size(); i++)
		{
			model_asset_ready_t* pAssetReady = m_nowReadyAssets[i];
			bool bFoundTarget = g_pAnarchyManager->GetInstanceManager()->ModelAssetReadyFirstMorph(pAssetReady->fileHash, pAssetReady->file, bSkippedTarget);
			if (bFoundTarget)
				break;
			else if (!bSkippedTarget)
			{
				delete pAssetReady;
				m_nowReadyAssets.erase(m_nowReadyAssets.begin() + i);
				break;
			}
		}
	}
}

void C_MetaverseManager::PerformLocalPlayerUpdate()
{
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	/*
	0 - instance
	1 - say
	2 - bodyOrigin
	3 - bodyAngles
	4 - headOrigin
	5 - headAngles
	6 - item
	7 - object
	8 - mouseX
	9 - mouseY
	10 - webURL
	11 - state
	12 - launched
	13 - twitchChannel
	14 - twitchLive
	*/

	//user_t* pUser = this->GetInstanceUser(cvar->FindVar("aamp_client_id"));

	if (!m_pLocalUser)
		return;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	QAngle playerBodyAngles = pPlayer->GetAbsAngles();
	Vector playerBodyOrigin = pPlayer->GetAbsOrigin();
	QAngle playerHeadAngles = pPlayer->EyeAngles();
	Vector playerHeadOrigin;
	pPlayer->EyeVectors(&playerHeadOrigin);

	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerBodyOrigin.x, playerBodyOrigin.y, playerBodyOrigin.z);
	std::string bodyOrigin = buf;

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerBodyAngles.x, playerBodyAngles.y, playerBodyAngles.z);
	std::string bodyAngles = buf;

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerHeadOrigin.x, playerHeadOrigin.y, playerHeadOrigin.z);
	std::string headOrigin = buf;

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerHeadAngles.x, playerHeadAngles.y, playerHeadAngles.z);
	std::string headAngles = buf;

	std::string object;
	std::string item;
	std::string model;
	std::string webUrl;
	std::string mouseX = "0";
	std::string mouseY = "0";
	std::string state = (g_pAnarchyManager->IsPaused()) ? "1" : "0";
	std::string launched = g_pAnarchyManager->GetLastLaunchedItemID();
	std::string twitchChannel = g_pAnarchyManager->GetMetaverseManager()->GetTwitchChannel();
	std::string twitchLive = (g_pAnarchyManager->GetMetaverseManager()->GetTwitchChannelLive()) ? "1" : "0";

	C_PropShortcutEntity* pSelectedShortcut = null;
	C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();//g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay()
	if (pEmbeddedInstance)
		pSelectedShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pEmbeddedInstance->GetOriginalEntIndex()));

	if (!pSelectedShortcut)
	{
		// If no shortcut determined from display instance, try the selected entity
		pSelectedShortcut = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
	}

	if (pSelectedShortcut)
	{
		if (!pEmbeddedInstance)
			pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance("auto" + pSelectedShortcut->GetItemId());


		object = pSelectedShortcut->GetObjectId();
		item = pSelectedShortcut->GetItemId();
		model = pSelectedShortcut->GetModelId();

		if (pEmbeddedInstance)
		{
			webUrl = pEmbeddedInstance->GetURL();

			float fMouseX;
			float fMouseY;
			pEmbeddedInstance->GetLastMouse(fMouseX, fMouseY);

			mouseX = VarArgs("%.10f", fMouseX);
			mouseY = VarArgs("%.10f", fMouseY);
		}
		else
		{
			webUrl = "";
			mouseX = "0";
			mouseY = "0";
		}

	}

	std::vector<std::string> args;
	args.push_back(g_pAnarchyManager->GetInstanceId());
	args.push_back(m_say);
	args.push_back(bodyOrigin);
	args.push_back(bodyAngles);
	args.push_back(headOrigin);
	args.push_back(headAngles);
	args.push_back(item);
	args.push_back(object);
	args.push_back(mouseX);
	args.push_back(mouseY);
	args.push_back(webUrl);
	args.push_back(state);
	args.push_back(launched);
	args.push_back(twitchChannel);
	args.push_back(twitchLive);

	// update our local user object too, so we can be treated just like all other users in the menus...
	m_pLocalUser->say = m_say;
	m_pLocalUser->bodyAngles = playerBodyAngles;
	m_pLocalUser->bodyOrigin = playerBodyOrigin;
	m_pLocalUser->headAngles = playerHeadAngles;
	m_pLocalUser->headOrigin = playerHeadOrigin;
	m_pLocalUser->itemId = item;
	m_pLocalUser->sessionId = g_pAnarchyManager->GetConnectedUniverse()->session;
	m_pLocalUser->objectId = object;
	m_pLocalUser->mouseX = mouseX;
	m_pLocalUser->mouseY = mouseY;
	m_pLocalUser->webUrl = webUrl;
	m_pLocalUser->state = state;
	m_pLocalUser->launched = launched;
	m_pLocalUser->twitchChannel = twitchChannel;
	m_pLocalUser->twitchLive = twitchLive;

	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "localUserUpdate", args);
}

//void C_MetaverseManager::ImportSteamGame(std::string name, std::string appid)
void C_MetaverseManager::ImportSteamGames(KeyValues* kv)
{
	// 1st save this out
	kv->SaveToFile(g_pFullFileSystem, "steamGames.key", "DEFAULT_WRITE_PATH");
	m_pImportSteamGamesKV = kv;
}

void C_MetaverseManager::StartImportSteamGames()
{
	unsigned int count = 0;
	for (KeyValues *sub = m_pImportSteamGamesKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		count++;

	m_uNumSteamGamesToImport = count;
	m_uNumSteamGamesToImported = 0;

	m_pImportSteamGamesSubKV = m_pImportSteamGamesKV->GetFirstSubKey();

	this->BeginTransaction();
	this->ImportNextSteamGame();
}

void C_MetaverseManager::ImportNextSteamGame()
{
	// now loop through it and add any missing games to the user library
	std::string appid;
	std::string name;
	std::string detailsurl;
	std::string screenurl;
	std::string marqueeurl;

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	//for (KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	if ( m_pImportSteamGamesSubKV )
	{
		appid = m_pImportSteamGamesSubKV->GetName();
		appid = appid.substr(2);
		name = m_pImportSteamGamesSubKV->GetString();
		//DevMsg("Adding game %s w/ appid %s\n", name.c_str(), appid.c_str());

		// 1. try to find an item with this fileid
		// 2. if not found, create an item for it
		// 3. victory bowl

		// build the search info
		KeyValues* pSearchInfo = new KeyValues("search");	// this gets deleted by the metaverse manager!!
		pSearchInfo->SetString("file", appid.c_str());

		KeyValues* item = this->FindLibraryItem(pSearchInfo);
		if (!item)
		{
			//DevMsg("Adding Steam game w/ id %s - %s\n", appid.c_str(), name.c_str());
			item = new KeyValues("item");
			item->SetInt("generation", 3);
			item->SetInt("local/info/created", 0);
			item->SetString("local/info/owner", "local");
			item->SetInt("local/info/removed", 0);
			item->SetString("local/info/remover", "");
			item->SetString("local/info/alias", "");
			item->SetString("local/info/id", g_pAnarchyManager->GenerateUniqueId());
			item->SetString("local/title", name.c_str());
			item->SetString("local/description", "");
			item->SetString("local/file", appid.c_str());

			item->SetString("local/type", this->ResolveLegacyType("pc").c_str());
			item->SetString("local/app", this->ResolveLegacyApp("Steam").c_str());

			detailsurl = "http://store.steampowered.com/app/" + appid;
			item->SetString("local/reference", detailsurl.c_str());

			item->SetString("local/preview", "");
			item->SetString("local/download", "");
			item->SetString("local/stream", "");

			screenurl = "http://cdn.akamai.steamstatic.com/steam/apps/" + appid + "/page_bg_generated.jpg";	// not the best or most reliable url for a screenshot, but is a standard format.
			item->SetString("local/screen", screenurl.c_str());

			marqueeurl = "http://cdn.akamai.steamstatic.com/steam/apps/" + appid + "/header.jpg";
			item->SetString("local/marquee", marqueeurl.c_str());

			this->AddItem(item);

			// JUST SAVE THE ITEM OUT NOW.  this way steamGames.key doesn't have to be iterated through every time AArcade starts.
			//g_pAnarchyManager->GetMetaverseManager()->SaveItem(item);
			g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "items", item->GetString("local/info/id"), item);

			g_pAnarchyManager->GetAccountant()->Action("aa_items_added", 1);

			m_uNumSteamGamesToImported++;
			m_pImportSteamGamesSubKV = m_pImportSteamGamesSubKV->GetNextKey();
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Steam Games", "importsteamgames", "0", VarArgs("%u", m_uNumSteamGamesToImport), "+", "importNextSteamGameCallback");
		}
		else
		{
			m_pImportSteamGamesSubKV = m_pImportSteamGamesSubKV->GetNextKey();
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Steam Games", "importsteamgames", "0", VarArgs("%u", m_uNumSteamGamesToImport), "+", "importNextSteamGameCallback");
		}
	}
	else
	{
		m_pImportSteamGamesKV->deleteThis();
		m_pImportSteamGamesKV = null;
		m_pImportSteamGamesSubKV = null;

		//pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Processing Steam Games", "importsteamgames", "0", VarArgs("%u", m_uNumSteamGamesToImport), "+0", "");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Finished Importing Steam Games", "importsteamgamescomplete", "", "", VarArgs("%u", m_uNumSteamGamesToImported), "");
		g_pAnarchyManager->AddToastMessage(VarArgs("Steam Games Imported (%u)", m_uNumSteamGamesToImported));

		this->CommitTransaction();

		std::vector<std::string> args;
		pHudBrowserInstance->DispatchJavaScriptMethod("eventListener", "doneImportingSteamGames", args);
	}
}

void C_MetaverseManager::AddType(KeyValues* pType, std::map<std::string, KeyValues*>* pResponseMap)
{
	KeyValues* active = this->GetActiveKeyValues(pType);
	if (active)
	{
		std::string id = VarArgs("%s", active->GetString("info/id"));
		if (pResponseMap)
			(*pResponseMap)[id] = pType;
		else
			m_types[id] = pType;
	}
	/*
	std::map<std::string, KeyValues*> responseMap = (pResponseMap) ? (*pResponseMap) : m_types;

	KeyValues* active = this->GetActiveKeyValues(pType);
	if (active)
	{
		std::string id = VarArgs("%s", active->GetString("info/id"));
		responseMap[id] = pType;
	}
	*/
}

void C_MetaverseManager::AddItem(KeyValues* pItem)
{
	KeyValues* active = this->GetActiveKeyValues(pItem);
	if (active)
	{
		std::string id = VarArgs("%s", active->GetString("info/id"));
		m_items[id] = pItem;
	}
}

void C_MetaverseManager::AddModel(KeyValues* pModel)
{
	KeyValues* active = this->GetActiveKeyValues(pModel);
	if (active)
	{
		std::string id = VarArgs("%s", active->GetString("info/id"));
		m_models[id] = pModel;
	}
}

void C_MetaverseManager::DeleteSQL(sqlite3** pDb, const char* tableName, const char* id)
{
	if (!pDb)
		pDb = &m_db;

	char *error;
	//sqlite3_stmt *stmt = NULL;
	const char *sqlDeleteInstance = VarArgs("DELETE from %s where id=\"%s\";", tableName, id);
	int rc = sqlite3_exec(*pDb, sqlDeleteInstance, NULL, NULL, &error);

	if (rc != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(*pDb));
		sqlite3_free(error);
	}
}

/*
void C_MetaverseManager::PurgeVolatileLocally()
{
	m_pVolatileSavesKV->deleteThis();
	m_pVolatileSavesKV = new KeyValues("volatile");
}
*/

void C_MetaverseManager::SaveVolatileLocally(bool bNewOnly)
{
	char *error;
	const char *sqlBeginTransaction = "BEGIN TRANSACTION;";
	int rc1 = sqlite3_exec(m_db, sqlBeginTransaction, NULL, NULL, &error);
	if (rc1 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(m_db));
		sqlite3_free(error);
	}

	std::string tableName;
	std::string id;
	KeyValues* kv;
	for (KeyValues *table = m_pVolatileSavesKV->GetFirstSubKey(); table; table = table->GetNextKey())
	{
		for (KeyValues *entry = table->GetFirstSubKey(); entry; entry = entry->GetNextKey())
		{
			kv = null;

			if (!Q_strcmp(table->GetName(), "instances"))
			{
				//aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
				//if (!pConnection || pConnection->isHost )	// OBSOLETE COMMENT (we DO save instances on clients now) - don't bother saving instances on clients.
					kv = g_pAnarchyManager->GetInstanceManager()->GetCurrentInstanceKV();
					//std::string instanceId = kv->GetString("info/local/id");
					//std::string mapId = kv->GetString("info/local/map");
					//std::string title = kv->GetString("info/local/title");
					//if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->ti)
					//DevMsg("Title: %s (%s): %s\n", title.c_str(), mapId.c_str(), entry->GetName());//, instanceId.c_str()
					//g_pAnarchyManager->GetInstanceManager()->SaveActiveInstance();
					this->SaveSQL((sqlite3**)entry->GetPtr(), table->GetName(), entry->GetName(), kv, true);
			}
			else
			{
				aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
				if (pConnection && pConnection->connected)
				{
					tableName = table->GetName();
					id = entry->GetName();
					//DevMsg("Table Name: %s %s\n", tableName.c_str(), id.c_str());
					if (tableName == "items")
						this->SaveRemoteItemChanges(id, true);
					else if (tableName == "models" || tableName == "Models")	// for some reason, the KV wants to store this with a capitol M
						this->SaveRemoteModelChanges(id, true);
					else if (tableName == "types")
						this->SaveRemoteTypeChanges(id, true);
					else if (tableName == "apps")
						this->SaveRemoteAppChanges(id, true);
				}
				else
				{
					kv = this->GetLibraryEntry(table->GetName(), entry->GetName());
					this->SaveSQL((sqlite3**)entry->GetPtr(), table->GetName(), entry->GetName(), kv, true);
				}
			}

			//if ( kv )
			//	this->SaveSQL((sqlite3**)entry->GetPtr(), table->GetName(), entry->GetName(), kv, true);
		}
	}
	
	m_pVolatileSavesKV->deleteThis();
	m_pVolatileSavesKV = new KeyValues("volatile");

	const char *sqlCommitTransaction = "COMMIT;";
	int rc2 = sqlite3_exec(m_db, sqlCommitTransaction, NULL, NULL, &error);
	if (rc2 != SQLITE_OK)
	{
		DevMsg("Error executing SQLite3 statement: %s\n", sqlite3_errmsg(m_db));
		sqlite3_free(error);
	}
}

bool C_MetaverseManager::GetNeedsSave()
{
	bool bNeedsIt = false;
	for (KeyValues *table = m_pVolatileSavesKV->GetFirstSubKey(); table; table = table->GetNextKey())
	{
		bNeedsIt = true;
		break;
	}

	return bNeedsIt;
}

// Remove volatile NEW items from the active library.  (Called during volatile dumping.)
void C_MetaverseManager::UnloadEntry(std::string category, std::string id)
{
	//DevMsg("NOTICE: Need to un-load %s %s\n", category.c_str(), id.c_str());
	std::map<std::string, KeyValues*>::iterator it;
	if (category == "items")
	{
		it = m_items.find(id);
		if (it != m_items.end())
		{
			it->second->deleteThis();
			m_items.erase(it);
		}
	}
	else if (category == "apps")
	{
		it = m_apps.find(id);
		if (it != m_apps.end())
		{
			it->second->deleteThis();
			m_apps.erase(it);
		}
	}
	else if (category == "models")
	{
		it = m_models.find(id);
		if (it != m_models.end())
		{
			it->second->deleteThis();
			m_models.erase(it);
		}
	}
	else if (category == "maps")
	{
		it = m_maps.find(id);
		if (it != m_maps.end())
		{
			it->second->deleteThis();
			m_maps.erase(it);
		}
	}
	else if (category == "types")
	{
		it = m_types.find(id);
		if (it != m_types.end())
		{
			it->second->deleteThis();
			m_types.erase(it);
		}
	}
}

void C_MetaverseManager::DiscardVolatileLocally()
{
	//std::map<std::string, KeyValues*>* pCategoryEntries;
	std::string category;
	std::string id;
	KeyValues* kv;
	//KeyValues* entry;
	KeyValues* victim;
	for (KeyValues *table = m_pVolatileSavesKV->GetFirstSubKey(); table; table = table->GetNextKey())
	{
		category = table->GetName();

		if (!Q_strcmp(category.c_str(), "instances"))	// instances will already go back to their original state naturally
			continue;

		for (KeyValues *entry = table->GetFirstSubKey(); entry; entry = entry->GetNextKey())
		{
			id = entry->GetName();
			victim = this->GetLibraryEntry(category, id);
			if (!victim)
				continue;
				
			kv = new KeyValues(category.substr(0, category.length() - 1).c_str());
			if (this->LoadSQLKevValues(category.c_str(), id.c_str(), kv, *((sqlite3**)entry->GetPtr())) )	// this *should* account for backpacks too, because it uses a sqldb pointer
			{
				victim->deleteThis();
				g_pAnarchyManager->GetMetaverseManager()->AddItem(kv);
			}
			else
			{
				kv->deleteThis();
				this->UnloadEntry(category, id);
			}
		}
	}

	m_pVolatileSavesKV->deleteThis();
	m_pVolatileSavesKV = new KeyValues("volatile");

	// Now dump out all remote-only entries (if any exist)
	// ITEMS
	auto remoteItemsIt = m_networkRemoteOnlyItems.begin();
	while (remoteItemsIt != m_networkRemoteOnlyItems.end())
	{
		this->UnloadEntry("items", remoteItemsIt->first);
		/*
		auto localItemsIt = m_items.find(remoteItemsIt->first);
		if (localItemsIt != m_items.end())
		{
			localItemsIt->second->deleteThis();
			m_items.erase(localItemsIt);
		}
		*/

		remoteItemsIt++;
	}
	m_networkRemoteOnlyItems.clear();

	// MODELS
	auto remoteModelsIt = m_networkRemoteOnlyModels.begin();
	while (remoteModelsIt != m_networkRemoteOnlyModels.end())
	{
		this->UnloadEntry("models", remoteModelsIt->first);
		/*
		// remove the model from (1) the m_models map, and (2) delete it's KV.
		auto localModelsIt = m_models.find(remoteModelsIt->first);
		if (localModelsIt != m_models.end())
		{
			localModelsIt->second->deleteThis();
			m_models.erase(localModelsIt);
		}
		*/

		remoteModelsIt++;
	}
	m_networkRemoteOnlyModels.clear();

	// TYPES
	auto remoteTypesIt = m_networkRemoteOnlyTypes.begin();
	while (remoteTypesIt != m_networkRemoteOnlyTypes.end())
	{
		this->UnloadEntry("types", remoteTypesIt->first);
		/*
		// remove the type from (1) the m_types map, and (2) delete it's KV.
		auto localTypesIt = m_types.find(remoteTypesIt->first);
		if (localTypesIt != m_types.end())
		{
			localTypesIt->second->deleteThis();
			m_types.erase(localTypesIt);
		}
		*/

		remoteTypesIt++;
	}
	m_networkRemoteOnlyTypes.clear();

	// APPS
	auto remoteAppsIt = m_networkRemoteOnlyApps.begin();
	while (remoteAppsIt != m_networkRemoteOnlyApps.end())
	{
		this->UnloadEntry("apps", remoteAppsIt->first);
		/*
		// remove the app from (1) the m_apps map, and (2) delete it's KV.
		auto localAppsIt = m_apps.find(remoteAppsIt->first);
		if (localAppsIt != m_apps.end())
		{
			localAppsIt->second->deleteThis();
			m_apps.erase(localAppsIt);
		}
		*/

		remoteAppsIt++;
	}
	m_networkRemoteOnlyApps.clear();
}

void C_MetaverseManager::SaveSQL(sqlite3** pDb, const char* tableName, const char* id, KeyValues* kv, bool bForceSave, bool bForceNoSave)
{
	if (!pDb)
		pDb = &m_db;

	//if (!Q_strcmp(tableName, "models"))
	//	DevMsg("Write Model w/ ID: %s\n", id);

	if (!Q_strcmp(tableName, "items") || !Q_strcmp(tableName, "apps") || !Q_strcmp(tableName, "models") || !Q_strcmp(tableName, "types"))
	{
		KeyValues* active = this->GetActiveKeyValues(kv);
		active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	}

	if (bForceNoSave || (!bForceSave && !g_pAnarchyManager->GetAutoSave()))
	{
		m_pVolatileSavesKV->SetPtr(VarArgs("%s/%s", tableName, id), pDb);
	}
	else
	{
		sqlite3_stmt *stmt = NULL;
		int rc = sqlite3_prepare(*pDb, VarArgs("REPLACE INTO %s VALUES(\"%s\", ?)", tableName, id), -1, &stmt, NULL);
		//int rc = sqlite3_prepare(*pDb, VarArgs("REPLACE INTO %s (id, value) VALUES(\"%s\", ?)", tableName, id), -1, &stmt, NULL);
		if (rc != SQLITE_OK)
			DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(*pDb));
		// SQLITE_STATIC because the statement is finalized before the buffer is freed:
		CUtlBuffer buf;
		kv->WriteAsBinary(buf);

		int size = buf.Size();
		rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
		if (rc != SQLITE_OK)
			DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(*pDb));
		else
		{
			rc = sqlite3_step(stmt);
			if (rc != SQLITE_DONE)
				DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(*pDb));
		}
		sqlite3_finalize(stmt);
		buf.Purge();
	}
}

/*
void C_MetaverseManager::SaveItem(KeyValues* pItem, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	KeyValues* active = this->GetActiveKeyValues(pItem);

	// FIXME: ALWAYS SAVING TO ACTIVE, BUT WHEN OTHER KEY SLOTS GET USED, WILL HAVE TO SAVE TO USE LOGIC TO DETERMINE WHICH SUB-KEY WE'RE SAVING.
	// NOTE: We're only using the item to get the ID and to update its modified time.
	active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));	// save as string because KeyValue's have no uint64 data type.

	// FIXME: SAVING TO .KEY FILES DISABLED FOR MYSQL MIGRATION!!
	// And dodged a bullet there, because when KV's get saved & loaded from disk, they forget their types, and ID's that start with 0 get screwed.
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO items VALUES(\"%s\", ?)", active->GetString("info/id")), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

	// SQLITE_STATIC because the statement is finalized before the buffer is freed:
	CUtlBuffer buf;
	pItem->WriteAsBinary(buf);

	int size = buf.Size();
	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
	}
	sqlite3_finalize(stmt);
	buf.Purge();
	//return rc;
	//pItem->SaveToFile(g_pFullFileSystem, VarArgs("library/items/%s.key", active->GetString("info/id")), "DEFAULT_WRITE_PATH");
	//DevMsg("Saved item %s to library/items/%s.key\n", active->GetString("title"), active->GetString("info/id"));
}
*/

/*
void C_MetaverseManager::SaveApp(KeyValues* pApp, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	KeyValues* active = this->GetActiveKeyValues(pApp);

	// FIXME: ALWAYS SAVING TO ACTIVE, BUT WHEN OTHER KEY SLOTS GET USED, WILL HAVE TO SAVE TO USE LOGIC TO DETERMINE WHICH SUB-KEY WE'RE SAVING.
	// NOTE: We're only using the app to get the ID and to update its modified time.
	active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));	// save as string because KeyValue's have no uint64 data type.

	// FIXME: SAVING TO .KEY FILES DISABLED FOR MYSQL MIGRATION!!
	// And dodged a bullet there, because when KV's get saved & loaded from disk, they forget their types, and ID's that start with 0 get screwed.
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO apps VALUES(\"%s\", ?)", active->GetString("info/id")), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

	// SQLITE_STATIC because the statement is finalized before the buffer is freed:
	CUtlBuffer buf;
	pApp->WriteAsBinary(buf);

	int size = buf.Size();
	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
	}
	sqlite3_finalize(stmt);
	buf.Purge();
}
*/

/*
void C_MetaverseManager::SaveModel(KeyValues* pItem, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	KeyValues* active = this->GetActiveKeyValues(pItem);

	// FIXME: ALWAYS SAVING TO ACTIVE, BUT WHEN OTHER KEY SLOTS GET USED, WILL HAVE TO SAVE TO USE LOGIC TO DETERMINE WHICH SUB-KEY WE'RE SAVING.
	// NOTE: We're only using the item to get the ID and to update its modified time.
	active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));	// save as string because KeyValue's have no uint64 data type.

	// FIXME: SAVING TO .KEY FILES DISABLED FOR MYSQL MIGRATION!!
	// And dodged a bullet there, because when KV's get saved & loaded from disk, they forget their types, and ID's that start with 0 get screwed.
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO models VALUES(\"%s\", ?)", active->GetString("info/id")), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

	// SQLITE_STATIC because the statement is finalized before the buffer is freed:
	CUtlBuffer buf;
	pItem->WriteAsBinary(buf);

	int size = buf.Size();
	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
	}
	sqlite3_finalize(stmt);
	buf.Purge();
}
*/

/*
void C_MetaverseManager::SaveType(KeyValues* pType, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	KeyValues* active = this->GetActiveKeyValues(pType);

	// FIXME: ALWAYS SAVING TO ACTIVE, BUT WHEN OTHER KEY SLOTS GET USED, WILL HAVE TO SAVE TO USE LOGIC TO DETERMINE WHICH SUB-KEY WE'RE SAVING.
	// NOTE: We're only using the item to get the ID and to update its modified time.
	active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));	// save as string because KeyValue's have no uint64 data type.

	// FIXME: SAVING TO .KEY FILES DISABLED FOR MYSQL MIGRATION!!
	// And dodged a bullet there, because when KV's get saved & loaded from disk, they forget their types, and ID's that start with 0 get screwed.
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO types VALUES(\"%s\", ?)", active->GetString("info/id")), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

	// SQLITE_STATIC because the statement is finalized before the buffer is freed:
	CUtlBuffer buf;
	pType->WriteAsBinary(buf);

	int size = buf.Size();
	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
	}
	sqlite3_finalize(stmt);
	buf.Purge();
}
*/

/*
void C_MetaverseManager::SaveMap(KeyValues* pMap, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	KeyValues* active = this->GetActiveKeyValues(pType);

	// FIXME: ALWAYS SAVING TO ACTIVE, BUT WHEN OTHER KEY SLOTS GET USED, WILL HAVE TO SAVE TO USE LOGIC TO DETERMINE WHICH SUB-KEY WE'RE SAVING.
	// NOTE: We're only using the item to get the ID and to update its modified time.
	active->SetString("info/modified", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));	// save as string because KeyValue's have no uint64 data type.

	// FIXME: SAVING TO .KEY FILES DISABLED FOR MYSQL MIGRATION!!
	// And dodged a bullet there, because when KV's get saved & loaded from disk, they forget their types, and ID's that start with 0 get screwed.
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, VarArgs("REPLACE INTO types VALUES(\"%s\", ?)", active->GetString("info/id")), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: prepare failed: %s\n", sqlite3_errmsg(pDb));

	// SQLITE_STATIC because the statement is finalized before the buffer is freed:
	CUtlBuffer buf;
	pType->WriteAsBinary(buf);

	int size = buf.Size();
	rc = sqlite3_bind_blob(stmt, 1, buf.Base(), size, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		DevMsg("FATAL ERROR: bind failed: %s\n", sqlite3_errmsg(pDb));
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
			DevMsg("FATAL ERROR: execution failed: %s\n", sqlite3_errmsg(pDb));
	}
	sqlite3_finalize(stmt);
	buf.Purge();
}
*/

// Create the item and return it, but don't save it or add it to the library.
bool C_MetaverseManager::CreateItem(int iLegacy, std::string itemId, KeyValues* pItemKV, std::string title, std::string description, std::string file, std::string type, std::string app, std::string reference, std::string preview, std::string download, std::string stream, std::string screen, std::string marquee, std::string model, std::string keywords)
{
	// TODO NEXT TIME:
	// MAKE USE OF THIS CREATE ITEM METHOD EVERYWHERE POSSIBLE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (!pItemKV)
	{
		DevMsg("No item given to CreateItem! Aborting.\n");
		return false;
	}
	
	if (itemId == "")
		itemId = g_pAnarchyManager->GenerateUniqueId();

	pItemKV->SetInt("generation", 3);
	pItemKV->SetInt("legacy", iLegacy);

	KeyValues* pActiveKV = this->GetActiveKeyValues(pItemKV);

	// add standard info
	pActiveKV->SetString("info/id", itemId.c_str());
	pActiveKV->SetString("info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	pActiveKV->SetString("info/owner", "local");
	//pActiveKV->SetString("info/removed", "");
	//pActiveKV->SetString("info/remover", "");
	//pActiveKV->SetString("info/alias", "");

	pActiveKV->SetString("title", title.c_str());
	pActiveKV->SetString("description", description.c_str());
	pActiveKV->SetString("file", file.c_str());
	pActiveKV->SetString("type", type.c_str());	//AA_DEFAULT_TYPEID
	pActiveKV->SetString("app", app.c_str());
	pActiveKV->SetString("reference", reference.c_str());
	pActiveKV->SetString("preview", preview.c_str());
	pActiveKV->SetString("download", download.c_str());
	pActiveKV->SetString("stream", stream.c_str());
	pActiveKV->SetString("screen", screen.c_str());
	pActiveKV->SetString("marquee", marquee.c_str());
	pActiveKV->SetString("model", model.c_str());
	pActiveKV->SetString("keywords", keywords.c_str());

	if( !g_pAnarchyManager->GetAutoSave() )	// TODO: is this the appropriate place for this?  What if the user starts to create the item, but cancels before actually creating it.
		m_pVolatileSavesKV->SetPtr(VarArgs("%s/%s", "items", itemId.c_str()), m_db);	// Add this new item to the temporary list, incase the user chooses NOT to save changes.

	return pItemKV;
}

bool C_MetaverseManager::DeleteApp(std::string appId)
{
	bool bSuccess = false;
	KeyValues* pAppKV = g_pAnarchyManager->GetMetaverseManager()->GetLibraryApp(appId);
	if (pAppKV)
	{
		auto it = m_apps.find(appId);
		m_apps.erase(it);
		this->DeleteSQL(null, "apps", appId.c_str());

		g_pAnarchyManager->AddToastMessage("Open-With App Profile Deleted");
	}

	return bSuccess;
}

void C_MetaverseManager::SmartMergItemKVs(KeyValues* pItemA, KeyValues* pItemB, bool bPreferFirst)
{
	KeyValues* pKeeperItem = (bPreferFirst) ? pItemA : pItemB;
	KeyValues* pMergerItem = (bPreferFirst) ? pItemB : pItemA;

	KeyValues* pActiveKeeper = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pKeeperItem);
	KeyValues* pActiveMerger = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pMergerItem);

	if (Q_strcmp(pActiveKeeper->GetString("info/id"), pActiveMerger->GetString("info/id")))
		DevMsg("WARNING: SmargMerg-ing 2 items with different IDs: %s %s\n", pActiveKeeper->GetString("info/id"), pActiveMerger->GetString("info/id"));

	// pKeeperItem
	// - Contains the most user-customized versions of fields.
	// - Fields with local file paths are probably important.

	// Compare "app values.
	// If the Primary uses an app, keep it.  Otherwise, use the one from Secondary.
	if (!Q_strcmp(pActiveKeeper->GetString("app"), "") && Q_strcmp(pActiveMerger->GetString("app"), ""))
		pActiveKeeper->SetString("app", pActiveMerger->GetString("app"));

	// Compare "file" values.
	// If Primary uses a non-internet location, keep it.
	bool bSecondaryFileLocationIsGood = true;
	if (!Q_strcmp(pActiveMerger->GetString("file"), "") || !Q_strcmp(pActiveMerger->GetString("file"), pActiveMerger->GetString("title")) || !Q_stricmp(pActiveMerger->GetString("file"), pActiveKeeper->GetString("file")))
		bSecondaryFileLocationIsGood = false;

	if (bSecondaryFileLocationIsGood)
	{
		std::string primaryFileLocation = pActiveKeeper->GetString("file");

		bool bPrimaryFileLocationIsGood = true;
		if (!Q_strcmp(pActiveKeeper->GetString("file"), "") || !Q_strcmp(pActiveKeeper->GetString("file"), pActiveKeeper->GetString("title")) || primaryFileLocation.find("http://") == 0 || primaryFileLocation.find("https://") == 0 || primaryFileLocation.find("www.") == 0)
			bPrimaryFileLocationIsGood = false;

		if (!bPrimaryFileLocationIsGood)
		{
			bool bSecondaryFileLocationIsAWebImage = false;

			if (Q_strcmp(pActiveKeeper->GetString("file"), ""))
			{
				std::string imageTypes = "|jpg|jpeg|gif|png|bmp|tga|tbn|";

				// Find out if Secondary's "file" is a web image.
				std::string secondaryFileLocationExtension = pActiveMerger->GetString("file");
				if (secondaryFileLocationExtension.find("http://") == 0 || secondaryFileLocationExtension.find("https://") == 0 || secondaryFileLocationExtension.find("www.") == 0)
				{
					size_t found = secondaryFileLocationExtension.find_last_of(".");
					if (found != std::string::npos)
					{
						secondaryFileLocationExtension = secondaryFileLocationExtension.substr(found + 1);

						if (imageTypes.find(VarArgs("|%s|", secondaryFileLocationExtension.c_str())) != std::string::npos)
						{
							bSecondaryFileLocationIsAWebImage = true;
						}
					}
				}
			}

			if (bSecondaryFileLocationIsAWebImage)
			{
				// So, we have a "file" property about to be replaced with a web image.  We need to determine if we should replace the screen and marquee URLs now along with it.

				bool bPrimaryScreensLocationIsALocalImage = false;
				std::string primaryScreensLocation = pActiveKeeper->GetString("screen");

				if (primaryScreensLocation.find(":") == 1)
					bPrimaryScreensLocationIsALocalImage = true;

				if (!bPrimaryScreensLocationIsALocalImage)
				{
					if (!Q_strcmp(pActiveKeeper->GetString("screen"), "") || !Q_strcmp(pActiveKeeper->GetString("screen"), pActiveKeeper->GetString("file")))
					{
						pActiveKeeper->SetString("screen", pActiveMerger->GetString("file"));

						// Clear Primary's "screenslocation" as well so the image refreshes next time.
						// First,  delete the cached version of the image so it can be re-downloaded.
						/*
						if (Q_strcmp(pActiveKeeper->GetString("screen"), "") && g_pFullFileSystem->FileExists(pActiveKeeper->GetString("screen"), USE_GAME_PATH))
						{
							if (pClientArcadeResources->GetReallyDoneStarting())
							{
								pClientArcadeResources->DeleteLocalFile(pActiveKeeper->GetString("screenslocation"));
								pActiveKeeper->SetString("screenslocation", "");	// Only clear this if we are really gonna re-download it, for speed up 3/30/2015
							}
						}
						*/

						//						pPrimaryItemKV->SetString("screenslocation", "");	// Move this higher to try to improve speedup
					}
				}

				bool bPrimaryMarqueesLocationIsALocalImage = false;
				std::string primaryMarqueesLocation = pActiveKeeper->GetString("marquee");

				if (primaryMarqueesLocation.find(":") == 1)
					bPrimaryMarqueesLocationIsALocalImage = true;

				if (!bPrimaryMarqueesLocationIsALocalImage)
				{
					if (!Q_strcmp(pActiveKeeper->GetString("marquee"), "") || !Q_strcmp(pActiveKeeper->GetString("marquee"), pActiveKeeper->GetString("file")))
					{
						pActiveKeeper->SetString("marquee", pActiveMerger->GetString("file"));

						// Clear Primary's "marqueeslocation" as well so the image refreshes next time.
						// First,  delete the cached version of the image so it can be re-downloaded.
						/*
						if (Q_strcmp(pActiveKeeper->GetString("marqueeslocation"), "") && g_pFullFileSystem->FileExists(pActiveKeeper->GetString("marqueeslocation"), USE_GAME_PATH))
						{
							if (pClientArcadeResources->GetReallyDoneStarting())
							{
								pClientArcadeResources->DeleteLocalFile(pActiveKeeper->GetString("marqueeslocation"));

								pActiveKeeper->SetString("marqueeslocation", "");
							}
						}
						*/
					}
				}
			}

			// The "file" property is going to change, and if Primary has no "trailerurl", then Primary's "noembed" property should change along with it.
//			if (!Q_strcmp(pActiveKeeper->GetString("preview"), ""))
	//			pActiveKeeper->SetInt("noembed", pActiveMerger->GetInt("noembed"));


			// Now replace the "file" property.
			pActiveKeeper->SetString("file", pActiveMerger->GetString("file"));
		}
	}

	// Compare "screens/low" values.
	// If Primary's screenslocation is not a local image, use Secondary's.
	bool bSecondaryScreensURLIsGood = true;
	if (!Q_strcmp(pActiveMerger->GetString("screen"), "") || !Q_stricmp(pActiveMerger->GetString("screen"), pActiveKeeper->GetString("screen")))
		bSecondaryScreensURLIsGood = false;

	if (bSecondaryScreensURLIsGood)
	{
		bool bPrimaryScreensLocationIsALocalImage = false;
		std::string primaryScreensLocation = pActiveKeeper->GetString("screen");

		if (primaryScreensLocation.find(":") == 1)
			bPrimaryScreensLocationIsALocalImage = true;

		if (!bPrimaryScreensLocationIsALocalImage)
		{
			// So Secondary's "screens/low" is good, and there is no local image used on Primary's "screenslocation".
			pActiveKeeper->SetString("screen", pActiveMerger->GetString("screen"));

			// Clear Primary's "screenslocation" as well so the image refreshes next time.
			// First,  delete the cached version of the image so it can be re-downloaded.
			/*
			if (Q_strcmp(pActiveKeeper->GetString("screenslocation"), "") && g_pFullFileSystem->FileExists(pActiveKeeper->GetString("screenslocation"), USE_GAME_PATH))
			{
				if (pClientArcadeResources->GetReallyDoneStarting())
				{
					pClientArcadeResources->DeleteLocalFile(pActiveKeeper->GetString("screenslocation"));
					pActiveKeeper->SetString("screenslocation", "");
				}
			}
			*/
		}
	}

	// Compare "marquees/low" values.
	// If Primary's marqueeslocation is not a local image, use Secondary's.
	bool bSecondaryMarqueesURLIsGood = true;
	if (!Q_strcmp(pActiveMerger->GetString("marquee"), "") || !Q_stricmp(pActiveMerger->GetString("marquee"), pActiveKeeper->GetString("marquee")))
		bSecondaryMarqueesURLIsGood = false;

	if (bSecondaryMarqueesURLIsGood)
	{
		bool bPrimaryMarqueesLocationIsALocalImage = false;
		std::string primaryMarqueesLocation = pActiveKeeper->GetString("marquee");

		if (primaryMarqueesLocation.find(":") == 1)
			bPrimaryMarqueesLocationIsALocalImage = true;

		if (!bPrimaryMarqueesLocationIsALocalImage)
		{
			// So Secondary's "marquees/low" is good, and there is no local image used on Primary's "marqueeslocation".
			pActiveKeeper->SetString("marquee", pActiveMerger->GetString("marquee"));

			// Clear Primary's "marqueeslocation" as well so the image refreshes next time.
			// First,  delete the cached version of the image so it can be re-downloaded.
			/*
			if (Q_strcmp(pActiveKeeper->GetString("marqueeslocation"), "") && g_pFullFileSystem->FileExists(pActiveKeeper->GetString("marqueeslocation"), USE_GAME_PATH))
			{
				if (pClientArcadeResources->GetReallyDoneStarting())
				{
					pClientArcadeResources->DeleteLocalFile(pActiveKeeper->GetString("marqueeslocation"));
					pActiveKeeper->SetString("marqueeslocation", "");
				}
			}
			*/
		}
	}

	// Compare "trailerurl" values
	if (Q_strcmp(pActiveMerger->GetString("preview"), ""))
		pActiveKeeper->SetString("preview", pActiveMerger->GetString("preview"));

	/*
	// Compare "publishedfileid" values
	if (Q_strcmp(pActiveMerger->GetString("publishedfileid"), ""))
		pActiveKeeper->SetString("publishedfileid", pActiveMerger->GetString("publishedfileid"));
	*/

	// Compare "model" values
	if (Q_strcmp(pActiveMerger->GetString("model"), ""))
		pActiveKeeper->SetString("model", pActiveMerger->GetString("model"));

	// Compare "type" values
	// Always use Secondary's "type" (unless maybe we have a m_pKeepMyItem"Names"ConVar?)
	pActiveKeeper->SetString("type", pActiveMerger->GetString("type"));

	// If there was a m_bKeepMyItemNamesConvar, this is where it should be used.
	/*
	if (bFullMerg)
	{
		// Compare "title" values
		if (Q_strcmp(pActiveMerger->GetString("title"), ""))
			pActiveKeeper->SetString("title", pActiveMerger->GetString("title"));

		// Compare "description" values
		if (Q_strcmp(pActiveMerger->GetString("description"), ""))
			pActiveKeeper->SetString("description", pActiveMerger->GetString("description"));

		// Compare "downloadurl" values
		if (Q_strcmp(pActiveMerger->GetString("downloadurl"), ""))
			pActiveKeeper->SetString("downloadurl", pActiveMerger->GetString("downloadurl"));

		// Compare "detailsurl" values
		if (Q_strcmp(pActiveMerger->GetString("detailsurl"), ""))
			pActiveKeeper->SetString("detailsurl", pActiveMerger->GetString("detailsurl"));

		// Compare "comments" values
		if (Q_strcmp(pActiveMerger->GetString("comments"), ""))
			pActiveKeeper->SetString("comments", pActiveMerger->GetString("comments"));

		// Compare "instructions" values
		if (Q_strcmp(pActiveMerger->GetString("instructions"), ""))
			pActiveKeeper->SetString("instructions", pActiveMerger->GetString("instructions"));

		// Compare "group" values
		if (Q_strcmp(pActiveMerger->GetString("group"), ""))
			pActiveKeeper->SetString("group", pActiveMerger->GetString("group"));

		// Compare "groupurl" values
		if (Q_strcmp(pActiveMerger->GetString("groupurl"), ""))
			pActiveKeeper->SetString("groupurl", pActiveMerger->GetString("groupurl"));

		// Compare "addon" values
		if (Q_strcmp(pActiveMerger->GetString("addon"), ""))
			pActiveKeeper->SetString("addon", pActiveMerger->GetString("addon"));
	}
	*/
}

std::vector<std::string>* C_MetaverseManager::GetDefaultFields()
{
	return &m_defaultFields;
}

void C_MetaverseManager::UpdateScrapersJS()
{
	std::vector<std::string> scraperList;

	// get a list of all the .js files in the folder
	FileFindHandle_t handle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx("resource\\ui\\html\\scrapers\\*.js", "MOD", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		scraperList.push_back(pFilename);
		pFilename = g_pFullFileSystem->FindNext(handle);
	}

	// generate code
	std::string code = "";//"if( !!arcadeHud) { ";
	unsigned int max = scraperList.size();
	for (unsigned int i = 0; i < max; i++)
		code += "arcadeHud.loadHeadScript(\"scrapers/" + scraperList[i] + "\");\n";
	//code += " } else console.log('ERROR: arcadeHud object was NOT ready to receive the scraper list.');";

	// open up scrapers.js and replace it.  this path is greated when AArcade is started.
	FileHandle_t fh = filesystem->Open("resource\\ui\\html\\scrapers.js", "w", "DEFAULT_WRITE_PATH");
	if (fh)
	{
		//filesystem->FPrintf(fh, "%s", code.c_str());
		filesystem->Write(code.c_str(), V_strlen(code.c_str()), fh);
		filesystem->Close(fh);
	}
}

void C_MetaverseManager::AdoptModel(std::string modelId, KeyValues* pAdoptedFilesKV, std::string modelFileOVerride, std::string customSubFolder, bool bDoNotReallyAdopt)
{
	KeyValues* pModelKV = (modelFileOVerride == "") ? this->GetActiveKeyValues(this->GetLibraryModel(modelId)) : null;
	if (!pModelKV && modelFileOVerride == "")
		return;

	std::string modelFile = (modelFileOVerride == "") ? pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)) : modelFileOVerride;
	if (modelFile == "")
		return;

	std::string goodCustomSubFolder = customSubFolder;
	if (goodCustomSubFolder == "")
		goodCustomSubFolder = "adopted";

	// Detect if the aarcade_user / custom / adopted folder already exists.
	std::string basePath = "custom\\adopted";
	/*if (pAdoptedFilesKV)
		pAdoptedFilesKV->SetBool("readyForUse", g_pFullFileSystem->IsDirectory(basePath.c_str()));*/

	// Create the aarcade_user/custom/adopted folder.
	if (!bDoNotReallyAdopt)
		g_pFullFileSystem->CreateDirHierarchy(basePath.c_str(), "DEFAULT_WRITE_PATH");

	// Scan & copy the assets into the folder.


	//if ( pAdoptedFilesKV )

	//char modelFileFixed[AA_MAX_STRING];
	int iAAMaxString = modelFile.length() + 1;
	char* modelFileFixed = new char[iAAMaxString];
	Q_strncpy(modelFileFixed, modelFile.c_str(), iAAMaxString);
	V_FixSlashes(modelFileFixed);
	V_FixDoubleSlashes(modelFileFixed);

	//this->AddFileToPackage(pAdoptedFilesKV, modelFileFixed);

	// Scan the model, add all its assets.
	std::string modelName = modelFileFixed;

	auto found2 = modelName.find(".\\");
	while (found2 != std::string::npos)
	{
		modelName.erase(found2, 2);
		found2 = modelName.find(".\\");
	}

	found2 = modelName.find("..\\");
	while (found2 != std::string::npos)
	{
		modelName.erase(found2, 3);
		found2 = modelName.find("..\\");
	}

	size_t found = modelName.find_last_of(".");

	if (found == std::string::npos)
		return;

	modelName = modelName.substr(0, found);

	//char res_string[512];
	//KeyValues* pBufKV;
	std::string buf;

	//char* packingPath = USE_GAME_PATH;
	char* packingPath = "GAME";

	// Add this .mdl file
	buf = VarArgs("%s.mdl", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this dx80.vtx file
	buf = VarArgs("%s.dx80.vtx", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this dx90.vtx file
	buf = VarArgs("%s.dx90.vtx", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this phy file
	buf = VarArgs("%s.phy", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this sw.vtx file
	buf = VarArgs("%s.sw.vtx", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this vvd file
	buf = VarArgs("%s.vvd", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Add this xbox.vtx file
	buf = VarArgs("%s.xbox.vtx", modelName.c_str());
	if (filesystem->FileExists(buf.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	// Load the model and find all of its materials (actually only the first 1024 used on it)
	const model_t* TheModel = modelinfo->FindOrLoadModel(modelFileFixed);

	IMaterial* pMaterials[1024];
	for (int x = 0; x < 1024; x++)
		pMaterials[x] = NULL;

	modelinfo->GetModelMaterials(TheModel, 1024, &pMaterials[0]);

	for (int x = 0; x < 1024; x++)
	{
		if (pMaterials[x])
			this->AdoptMaterial(pAdoptedFilesKV, pMaterials[x], null, customSubFolder, bDoNotReallyAdopt);
	}

	delete[] modelFileFixed;
}

void C_MetaverseManager::AdoptMaterialDependency(KeyValues* pAdoptedFilesKV, std::string attributeName, IMaterial* pMaterial, KeyValues* pMaterialKV, std::string customSubFolder, bool bDoNotReallyAdopt)
{
	bool bUseLoadedMaterialValues = false;
	char* packingPath = "GAME";
	bool foundVar = false;
	IMaterialVar* pMaterialTextureVar = NULL;

	if (attributeName == "replace")	// for patch materials
	{
		KeyValues* pReplaceKV = pMaterialKV->FindKey("replace");
		if (pReplaceKV)
		{
			// Cycle through the different material vars
			std::vector<std::string> textureAttributeNames;
			textureAttributeNames.push_back("$basetexture");
			textureAttributeNames.push_back("$texture2");
			textureAttributeNames.push_back("$bumpmap");
			textureAttributeNames.push_back("$normalmap");
			textureAttributeNames.push_back("$normalmap2");
			textureAttributeNames.push_back("$envmap");
			textureAttributeNames.push_back("$envmapmask");
			textureAttributeNames.push_back("$hdrbasetexture");
			textureAttributeNames.push_back("$hdrcompressedtexture");
			textureAttributeNames.push_back("$iris");
			textureAttributeNames.push_back("$corneatexture");
			textureAttributeNames.push_back("$ambientoccltexture");
			textureAttributeNames.push_back("$phongexponenttexture");
			textureAttributeNames.push_back("$lightwarptexture");
			textureAttributeNames.push_back("$selfillummask");
			textureAttributeNames.push_back("$detail");

			for (unsigned int i = 0; i < textureAttributeNames.size(); i++)
			{
				if (bUseLoadedMaterialValues)
					this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], pMaterial, pReplaceKV, customSubFolder, bDoNotReallyAdopt);
				else
					this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], null, pReplaceKV, customSubFolder, bDoNotReallyAdopt);
			}
		}
	}
	else if (attributeName == "include")	// for patch materials
	{
		std::string includeFileName = "";	// a VMT but w/ the .vmt at the end too.
		IMaterial* pIncludeMaterial = null;	// might remain null if the material isn't loaded but the file DOES exist.

		// 1 - Get the includeFileName from the given material or material name.
		if (pMaterial)
		{
			pMaterialTextureVar = pMaterial->FindVar(attributeName.c_str(), &foundVar, false);
			if (foundVar && pMaterialTextureVar->IsDefined())
				includeFileName = VarArgs("%s", pMaterialTextureVar->GetStringValue());
		}
		
		if (includeFileName == "" && pMaterialKV)
			includeFileName = pMaterialKV->GetString(attributeName.c_str());

		char* fixIncludeFileName = VarArgs("%s", includeFileName.c_str());
		V_FixSlashes(fixIncludeFileName);
		V_FixDoubleSlashes(fixIncludeFileName);
		includeFileName = fixIncludeFileName;

		auto found = includeFileName.find(".\\");
		while (found != std::string::npos)
		{
			includeFileName.erase(found, 2);
			found = includeFileName.find(".\\");
		}

		found = includeFileName.find("..\\");
		while (found != std::string::npos)
		{
			includeFileName.erase(found, 3);
			found = includeFileName.find("..\\");
		}

		//DevMsg("Include File Name: %s\n", includeFileName.c_str());

		if (includeFileName != "" && filesystem->FileExists(includeFileName.c_str(), packingPath))
		{
			std::string includeMaterialName = "";	// this must always be filled out, even if the material is not loaded yet.
			std::string testIncludeMaterialName = includeFileName;	// This should be the same as includeFileName, but w/o the .vmt at the end.
			size_t foundExtension = testIncludeMaterialName.find(".vmt");
			if (foundExtension == std::string::npos)
				foundExtension = testIncludeMaterialName.find(".VMT");

			if (foundExtension != std::string::npos)
				testIncludeMaterialName = testIncludeMaterialName.substr(0, foundExtension);

			char* fixTestIncludeMaterialName = VarArgs("%s", testIncludeMaterialName.c_str());
			V_FixSlashes(fixTestIncludeMaterialName);
			V_FixDoubleSlashes(fixTestIncludeMaterialName);
			testIncludeMaterialName = fixTestIncludeMaterialName;

			auto found = testIncludeMaterialName.find(".\\");
			while (found != std::string::npos)
			{
				testIncludeMaterialName.erase(found, 2);
				found = testIncludeMaterialName.find(".\\");
			}

			found = testIncludeMaterialName.find("..\\");
			while (found != std::string::npos)
			{
				testIncludeMaterialName.erase(found, 3);
				found = testIncludeMaterialName.find("..\\");
			}

			// Remove the materials/ prefix.

			size_t foundPrefix = testIncludeMaterialName.find("materials\\");
			if (foundPrefix == 0)
				testIncludeMaterialName = testIncludeMaterialName.substr(foundPrefix + 10);

			includeMaterialName = testIncludeMaterialName;

			// 2 - Attempt to get a material pointer, if the material is loaded.  But it's fine if we can't find the material pointer, because the VMT DOES exist.
			if (includeMaterialName != "")
			{
				IMaterial* pTestMaterial = null;

				if (bUseLoadedMaterialValues)
				{
					pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_WORLD);

					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_MODEL);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_VGUI);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_PARTICLE);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_DECAL);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_SKYBOX);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_OTHER);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_PRECACHED);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_UNACCOUNTED);
				}

				if (pTestMaterial && !pTestMaterial->IsErrorMaterial())
					pIncludeMaterial = pTestMaterial;

				// 3 - Adopt the material that is referenced
				this->AdoptMaterial(pAdoptedFilesKV, pIncludeMaterial, includeMaterialName.c_str(), customSubFolder, bDoNotReallyAdopt);
			}
		}
	}
	else
	{
		// Add the [attributeName] VTF to the list
		std::string testFileName = "";
		if (pMaterial)
		{
			pMaterialTextureVar = pMaterial->FindVar(attributeName.c_str(), &foundVar, false);
			if (foundVar && pMaterialTextureVar->IsDefined())
				testFileName = VarArgs("materials/%s.vtf", pMaterialTextureVar->GetStringValue());
		}
		
		if (testFileName == "" && pMaterialKV)
		{
			testFileName = pMaterialKV->GetString(attributeName.c_str());

			if (testFileName != "")
				testFileName = VarArgs("materials/%s.vtf", testFileName.c_str());
		}

		if (testFileName != "")
		{
			char* fixTestFileName = VarArgs("%s", testFileName.c_str());
			V_FixSlashes(fixTestFileName);
			V_FixDoubleSlashes(fixTestFileName);
			testFileName = fixTestFileName;

			auto found = testFileName.find(".\\");
			while (found != std::string::npos)
			{
				testFileName.erase(found, 2);
				found = testFileName.find(".\\");
			}

			found = testFileName.find("..\\");
			while (found != std::string::npos)
			{
				testFileName.erase(found, 3);
				found = testFileName.find("..\\");
			}
		}

		if (testFileName != "" && (attributeName != "$envmap" || Q_stricmp(testFileName.c_str(), "materials/env_cubemap.vtf")))
		{
			if (filesystem->FileExists(testFileName.c_str(), packingPath))
				this->AdoptFile(pAdoptedFilesKV, testFileName.c_str(), customSubFolder, bDoNotReallyAdopt);
		}
	}
}

void C_MetaverseManager::AdoptMaterial(KeyValues* pAdoptedFilesKV, IMaterial* pMaterial, const char* materialName, std::string customSubFolder, bool bDoNotReallyAdopt)
{
	KeyValues* materialKV = new KeyValues("material");

	std::string testFileName = "";
	std::string valueBuf = "";

	char* packingPath = "GAME";

	bool bUseLoadedMaterialValues = false;

	std::string goodMaterialName = (pMaterial) ? pMaterial->GetName() : materialName;

	char* fixGoodMaterialName = VarArgs("%s", goodMaterialName.c_str());
	V_FixSlashes(fixGoodMaterialName);
	V_FixDoubleSlashes(fixGoodMaterialName);
	goodMaterialName = fixGoodMaterialName;

	auto found = goodMaterialName.find(".\\");
	while (found != std::string::npos)
	{
		goodMaterialName.erase(found, 2);
		found = goodMaterialName.find(".\\");
	}

	found = goodMaterialName.find("..\\");
	while (found != std::string::npos)
	{
		goodMaterialName.erase(found, 3);
		found = goodMaterialName.find("..\\");
	}

	if (!bUseLoadedMaterialValues || !pMaterial)
	{
		testFileName = "materials/";
		testFileName += goodMaterialName;
		testFileName += ".vmt";

		// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
		if (!materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), packingPath))
		{
			materialKV->deleteThis();
			return;
		}
	}

	//KeyValues* pBufKV;
	std::string buf;

	// Add this VMT to the download list
	valueBuf = goodMaterialName;// (bUseLoadedMaterialValues && pMaterial) ? pMaterial->GetName() : materialName;
	testFileName = VarArgs("materials/%s.vmt", valueBuf.c_str());

	if (filesystem->FileExists(testFileName.c_str(), packingPath))
		this->AdoptFile(pAdoptedFilesKV, testFileName.c_str(), customSubFolder, bDoNotReallyAdopt);

	// Cycle through the different material vars
	std::vector<std::string> textureAttributeNames;
	textureAttributeNames.push_back("$basetexture");
	textureAttributeNames.push_back("$texture2");
	textureAttributeNames.push_back("$bumpmap");
	textureAttributeNames.push_back("$normalmap");
	textureAttributeNames.push_back("$normalmap2");
	textureAttributeNames.push_back("$envmap");
	textureAttributeNames.push_back("$envmapmask");
	textureAttributeNames.push_back("$hdrbasetexture");
	textureAttributeNames.push_back("$hdrcompressedtexture");
	textureAttributeNames.push_back("$iris");
	textureAttributeNames.push_back("$corneatexture");
	textureAttributeNames.push_back("$ambientoccltexture");
	textureAttributeNames.push_back("$phongexponenttexture");
	textureAttributeNames.push_back("$lightwarptexture");
	textureAttributeNames.push_back("$selfillummask");
	textureAttributeNames.push_back("$detail");
	textureAttributeNames.push_back("include");
	textureAttributeNames.push_back("replace");

	for (unsigned int i = 0; i < textureAttributeNames.size(); i++)
	{
		if (bUseLoadedMaterialValues)
			this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], pMaterial, materialKV, customSubFolder, bDoNotReallyAdopt);
		else
			this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], null, materialKV, customSubFolder, bDoNotReallyAdopt);
	}

	materialKV->deleteThis();
}

bool C_MetaverseManager::AdoptFile(KeyValues* pAdoptedFilesKV, std::string file_in, std::string customSubFolder, bool bDoNotReallyAdopt)//(KeyValues* objectKV, const char* file, bool bCheckMountContent, bool bAddDependencies, bool bOwnPublishedAddonsMatter)
{
	//	char* packingPath = USE_GAME_PATH;
	char* packingPath = "GAME";

	//char fileFixed[AA_MAX_STRING];
	int iAAMaxString = file_in.length() + 1;
	char* fileFixed = new char[iAAMaxString];
	Q_strncpy(fileFixed, file_in.c_str(), iAAMaxString);
	V_FixSlashes(fileFixed);
	V_FixDoubleSlashes(fileFixed);

	/*std::string buf = fileFixed;
	size_t foundIssue = buf.find("\\\\");
	while (foundIssue != std::string::npos)
	{
		buf.replace(foundIssue, 2, "\\");
		foundIssue = buf.find("\\\\");
	}

	Q_strcpy(fileFixed, buf.c_str());*/

	std::string goodCustomSubFolder = customSubFolder;
	if (goodCustomSubFolder == "")
		goodCustomSubFolder = "adopted";

	std::string file = fileFixed;

	auto found = file.find(".\\");
	while (found != std::string::npos)
	{
		file.erase(found, 2);
		found = file.find(".\\");
	}

	found = file.find("..\\");
	while (found != std::string::npos)
	{
		file.erase(found, 3);
		found = file.find("..\\");
	}

	std::string basePath = std::string("custom\\") + goodCustomSubFolder;

	//size_t lastSlash = file.find_last_of("/\\");
	//if (lastSlash == std::string::npos)
		//return false;

	size_t lastSlash;
	std::string targetPath;
	std::string targetFile;
	CUtlBuffer utilBuf;
	if (g_pFullFileSystem->ReadFile(file.c_str(), packingPath, utilBuf))
	{
		targetFile = VarArgs("%s\\%s", basePath.c_str(), file.c_str());

		lastSlash = targetFile.find_last_of("/\\");
		if (lastSlash == std::string::npos)
		{
			delete[] fileFixed;
			return false;
		}
		targetPath = targetFile.substr(0, lastSlash);

		if (!bDoNotReallyAdopt)
			g_pFullFileSystem->CreateDirHierarchy(targetPath.c_str(), "DEFAULT_WRITE_PATH");

		//DevMsg("(%s) %s copy to %s\n", targetPath.c_str(), file.c_str(), targetFile.c_str());
		if (!bDoNotReallyAdopt)
		{
			g_pFullFileSystem->CreateDirHierarchy(targetPath.c_str());
			g_pFullFileSystem->WriteFile(targetFile.c_str(), "DEFAULT_WRITE_PATH", utilBuf);
		}

		if (pAdoptedFilesKV)
		{
			KeyValues* entryKV = pAdoptedFilesKV->CreateNewKey();
			entryKV->SetString("", fileFixed);
		}

		delete[] fileFixed;
		return true;
	}

	delete[] fileFixed;
	return false;
}

bool C_MetaverseManager::LoadSQLKevValues(const char* tableName, const char* id, KeyValues* kv, sqlite3* pDb)
{
	if (!pDb)
		pDb = m_db;

	sqlite3_stmt *stmt = NULL;
	//DevMsg("loading from table name: %s _id %s\n", tableName, id);
	int rc = sqlite3_prepare(pDb, VarArgs("SELECT * from %s WHERE id = \"%s\"", tableName, id), -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

	bool bSuccess = false;
	int length;
	if (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		if (length > 0)
		{
			CUtlBuffer buf(0, length, 0);
			buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
			if (kv->ReadAsBinary(buf))
				bSuccess = true;
			buf.Purge();
		}
		else
			bSuccess = false;
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.

	return bSuccess;
}

KeyValues* C_MetaverseManager::AdoptNode(std::string nodeInstanceId, std::string customFolder, bool bDoNotReallyAdopt)
{
	instance_t* pInstance = g_pAnarchyManager->GetInstanceManager()->FindInstance(nodeInstanceId);
	if (pInstance)
	{
		//KeyValues* pInstanceKV = g_pAnarchyManager->GetInstanceManager()->LoadInstance(null, pInstance->id, "", "", true);

		KeyValues* pInstanceKV = new KeyValues("instance");
		if (!g_pAnarchyManager->GetMetaverseManager()->LoadSQLKevValues("instances", pInstance->id.c_str(), pInstanceKV))
		{
			DevMsg("Failed to load node instance KV.\n");
			return null;
		}

		KeyValues* pAdoptedFilesKV = new KeyValues("adopted");
		KeyValues* pAdoptedFilesModelsKV = pAdoptedFilesKV->FindKey("models", true);
		KeyValues* pAdoptedFileKV;
		KeyValues* pNodeObjectsKV = pInstanceKV->FindKey("objects", true);
		std::string modelId;
		for (KeyValues *sub = pNodeObjectsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			modelId = sub->GetString("local/model");

			pAdoptedFileKV = pAdoptedFilesModelsKV->FindKey(modelId.c_str(), true);
			this->AdoptModel(modelId, pAdoptedFileKV, "", customFolder, true);
		}

		pInstanceKV->deleteThis();
		//pAdoptedFilesKV->deleteThis();
		return pAdoptedFilesKV;
	}

	return null;
}

void C_MetaverseManager::SaveInstanceInfo(instance_t* pInstance)
{
	C_Backpack* pBackpack = null;
	KeyValues* pInstanceKV = new KeyValues("instance");
	if (!this->LoadSQLKevValues("instances", pInstance->id.c_str(), pInstanceKV))
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
		pInstanceKV->SetString("info/local/title", pInstance->title.c_str());
		pInstanceKV->SetString("info/local/map", pInstance->mapId.c_str());
		pInstanceKV->SetString("info/local/autoplay", pInstance->autoplayId.c_str());

		if (!pBackpack)
			g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "instances", pInstance->id.c_str(), pInstanceKV);
		else
		{
			DevMsg("Weird backpack detected! Don't know how to handle it! Fix it!\n");
			//g_pAnarchyManager->GetMetaverseManager()->SaveSQL(*(pBackpack->GetSQLDb()), "instances", pInstance->id.c_str(), pInstanceKV);
		}

		pInstanceKV->deleteThis();
		pInstanceKV = null;
	}
}

void C_MetaverseManager::DeleteInstance(instance_t* pInstance)
{
	// can't delete the currently loaded instance
	if (pInstance->id == g_pAnarchyManager->GetInstanceId())
		return;

	this->DeleteSQL(null, "instances", pInstance->id.c_str());

	g_pAnarchyManager->GetInstanceManager()->RemoveInstance(pInstance);
}

void C_MetaverseManager::OnAssetUploadBatchReady()
{
	std::vector<std::string> args;

	std::string batchId;
	KeyValues* pBatchKV = null;
	int index = m_uploadBatches.FirstInorder();// .Find(instanceId);
	if (index != m_uploadBatches.InvalidIndex())
	{
		pBatchKV = m_uploadBatches.Element(index);
		args.push_back(m_uploadBatches.Key(index).c_str());
		//batchId = m_uploadBatches.Key(index);
	}

	if (!pBatchKV)
		return;


	//std::string fullPath;
	//std::string fullPathHash;
	KeyValues* pMapKV;
	KeyValues* pMapMaterialsKV;
	KeyValues* pMapModelsKV;
	KeyValues* pMapModelMaterialsKV;
	KeyValues* pMapsKV;

	KeyValues* pModelKV;
	KeyValues* pModelsKV;
	KeyValues* pModelMaterialsKV;
	//KeyValues* pModelMaterialKV;
	KeyValues* pMaterialKV;
	KeyValues* pOtherParentKV;
	int iFileCount = 0;
	std::string otherHash;
	KeyValues* pUniqueKV = pBatchKV->FindKey("unique");
	if (pUniqueKV)
	{
		for (KeyValues *pFileKV = pUniqueKV->GetFirstSubKey(); pFileKV; pFileKV = pFileKV->GetNextKey())
		{
			std::string fullPath = pFileKV->GetString("");
			std::string fullPathHash = pFileKV->GetName();
			if (fullPathHash.length() < 3)
			{
				DevMsg("ERROR: Fullpathhash is length less than 3! Invalid!\n");
				return;
			}

			args.push_back(fullPath);
			args.push_back(fullPathHash.substr(2));

			//DevMsg("Debug: %s vs %s\n", fullPathHash.c_str(), fullPathHash.substr(2).c_str());

			std::string other = "";
			// is this a model (MDL) or a model material (VMT)? (If so, then it may have dependencies.)
			// first, check if it's a BSP.
			if (fullPath.find(".bsp") == fullPath.length() - 4 || fullPath.find(".BSP") == fullPath.length() - 4)
			{
				pMapKV = pBatchKV->FindKey(VarArgs("maps/%s", fullPathHash.c_str()));
				if (pMapKV)
				{
					// the map can have subkeys: materials, models, models/materials

					// MATERIALS USED IN THE MAP
					pMapMaterialsKV = pMapKV->FindKey("materials");
					if (pMapMaterialsKV)
					{
						for (KeyValues* pMapMaterialKV = pMapMaterialsKV->GetFirstSubKey(); pMapMaterialKV; pMapMaterialKV = pMapMaterialKV->GetNextKey())
						{
							if (other != "")
								other += "::";

							otherHash = pMapMaterialKV->GetName();
							if (otherHash.length() < 3)
							{
								DevMsg("ERROR: otherHash is length less than 3! Invalid!\n");
								return;
							}

							other += otherHash.substr(2);
							other += "::";
							other += pMapMaterialKV->GetString("file");

							pOtherParentKV = pMapMaterialKV->FindKey("other", true);
							for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
							{
								if (other != "")
									other += "::";

								otherHash = pOtherKV->GetName();
								if (otherHash.length() < 3)
								{
									DevMsg("ERROR: Another otherHash is length less than 3! Invalid!\n");
									return;
								}
								other += otherHash.substr(2);
								other += "::";
								other += pOtherKV->GetString("");
							}
						}
					}

					// MODELS USED IN THE MAP
					pMapModelsKV = pMapKV->FindKey("models");
					if (pMapModelsKV)
					{
						for (KeyValues* pMapModelKV = pMapModelsKV->GetFirstSubKey(); pMapModelKV; pMapModelKV = pMapModelKV->GetNextKey())
						{
							if (other != "")
								other += "::";

							otherHash = pMapModelKV->GetName();
							if (otherHash.length() < 3)
							{
								DevMsg("ERROR: otherHash is length less than 3! Invalid!\n");
								return;
							}

							other += otherHash.substr(2);
							other += "::";
							other += pMapModelKV->GetString("file");

							pMapModelMaterialsKV = pMapModelKV->FindKey("materials");
							if (pMapModelMaterialsKV)
							{
								for (KeyValues* pMapModelMaterialKV = pMapModelMaterialsKV->GetFirstSubKey(); pMapModelMaterialKV; pMapModelMaterialKV = pMapModelMaterialKV->GetNextKey())
								{
									if (other != "")
										other += "::";

									otherHash = pMapModelMaterialKV->GetName();
									if (otherHash.length() < 3)
									{
										DevMsg("ERROR: otherHash is length less than 3! Invalid!\n");
										return;
									}

									other += otherHash.substr(2);
									other += "::";
									other += pMapModelMaterialKV->GetString("file");

									pOtherParentKV = pMapModelMaterialKV->FindKey("other", true);
									for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
									{
										if (other != "")
											other += "::";

										otherHash = pOtherKV->GetName();
										if (otherHash.length() < 3)
										{
											DevMsg("ERROR: Another otherHash is length less than 3! Invalid!\n");
											return;
										}
										other += otherHash.substr(2);
										other += "::";
										other += pOtherKV->GetString("");
									}
								}
							}

							pOtherParentKV = pMapModelKV->FindKey("other", true);
							for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
							{
								if (other != "")
									other += "::";

								otherHash = pOtherKV->GetName();
								if (otherHash.length() < 3)
								{
									DevMsg("ERROR: otherHash (alpha beta) is length less than 3! Invalid!\n");
									return;
								}
								other += otherHash.substr(2);
								other += "::";
								other += pOtherKV->GetString("");
							}
						}
					}
				}
			}
			else if (fullPath.find(".mdl") == fullPath.length() - 4 || fullPath.find(".MDL") == fullPath.length() - 4)
			{
				pModelKV = pBatchKV->FindKey(VarArgs("models/%s", fullPathHash.c_str()));
				if (pModelKV)
				{
					pModelMaterialsKV = pModelKV->FindKey("materials");
					if (pModelMaterialsKV)
					{
						for (KeyValues* pModelMaterialKV = pModelMaterialsKV->GetFirstSubKey(); pModelMaterialKV; pModelMaterialKV = pModelMaterialKV->GetNextKey())
						{
							if (other != "")
								other += "::";

							otherHash = pModelMaterialKV->GetName();
							if (otherHash.length() < 3)
							{
								DevMsg("ERROR: otherHash is length less than 3! Invalid!\n");
								return;
							}

							other += otherHash.substr(2);
							other += "::";
							other += pModelMaterialKV->GetString("file");

							pOtherParentKV = pModelMaterialKV->FindKey("other", true);
							for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
							{
								if (other != "")
									other += "::";

								otherHash = pOtherKV->GetName();
								if (otherHash.length() < 3)
								{
									DevMsg("ERROR: Another otherHash is length less than 3! Invalid!\n");
									return;
								}
								other += otherHash.substr(2);
								other += "::";
								other += pOtherKV->GetString("");
							}
						}
					}

					pOtherParentKV = pModelKV->FindKey("other", true);
					for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
					{
						if (other != "")
							other += "::";

						otherHash = pOtherKV->GetName();
						if (otherHash.length() < 3)
						{
							DevMsg("ERROR: otherHash (alpha beta) is length less than 3! Invalid!\n");
							return;
						}
						other += otherHash.substr(2);
						other += "::";
						other += pOtherKV->GetString("");
					}
				}
			}
			else if (fullPath.find(".vmt") == fullPath.length() - 4 || fullPath.find(".VMT") == fullPath.length() - 4)
			{

				// Find this VMT referenced somewhere, in order to get it's dependencies.
				bool bFoundAlready = false;

				// First, check under model materials...
				pModelsKV = pBatchKV->FindKey("models", true);
				for (KeyValues *pModelKV = pModelsKV->GetFirstSubKey(); pModelKV; pModelKV = pModelKV->GetNextKey())
				{
					pMaterialKV = pModelKV->FindKey(VarArgs("materials/%s", fullPathHash.c_str()));
					if (pMaterialKV)
					{
						pOtherParentKV = pMaterialKV->FindKey("other", true);
						for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
						{
							if (other != "")
								other += "::";

							otherHash = pOtherKV->GetName();
							other += otherHash.substr(2);
							if (otherHash.length() < 3)
							{
								DevMsg("ERROR: otherHash (delta charlie) is length less than 3! Invalid!\n");
								return;
							}
							other += "::";
							other += pOtherKV->GetString("");
						}
						bFoundAlready = true;
						break;
					}
				}

				if (!bFoundAlready)
				{
					// Second, check under map materials...
					pMapsKV = pBatchKV->FindKey("maps", true);
					for (KeyValues *pMapKV = pMapsKV->GetFirstSubKey(); pMapKV; pMapKV = pMapKV->GetNextKey())
					{
						pMaterialKV = pMapKV->FindKey(VarArgs("materials/%s", fullPathHash.c_str()));
						if (pMaterialKV)
						{
							pOtherParentKV = pMaterialKV->FindKey("other", true);
							for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
							{
								if (other != "")
									other += "::";

								otherHash = pOtherKV->GetName();
								other += otherHash.substr(2);
								if (otherHash.length() < 3)
								{
									DevMsg("ERROR: otherHash (delta charlie) is length less than 3! Invalid!\n");
									return;
								}
								other += "::";
								other += pOtherKV->GetString("");
							}
							bFoundAlready = true;
							break;
						}
					}
				}

				if (!bFoundAlready)
				{
					// Third, check under map model materials...
					pMapsKV = pBatchKV->FindKey("maps", true);
					for (KeyValues *pMapKV = pMapsKV->GetFirstSubKey(); pMapKV; pMapKV = pMapKV->GetNextKey())
					{
						pMapModelsKV = pMapKV->FindKey("models", true);
						for (KeyValues *pMapModelKV = pMapModelsKV->GetFirstSubKey(); pMapModelKV; pMapModelKV = pMapModelKV->GetNextKey())
						{
							pMaterialKV = pMapModelKV->FindKey(VarArgs("materials/%s", fullPathHash.c_str()));
							if (pMaterialKV)
							{
								pOtherParentKV = pMaterialKV->FindKey("other", true);
								for (KeyValues *pOtherKV = pOtherParentKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
								{
									if (other != "")
										other += "::";

									otherHash = pOtherKV->GetName();
									other += otherHash.substr(2);
									if (otherHash.length() < 3)
									{
										DevMsg("ERROR: otherHash (delta charlie) is length less than 3! Invalid!\n");
										return;
									}
									other += "::";
									other += pOtherKV->GetString("");
								}
								bFoundAlready = true;
								break;
							}
						}
					}
				}
			}

			//DevMsg("Other Upload Batch: %s\n", other.c_str());
			args.push_back(other);
			iFileCount++;
		}
	}

	/*
	args.push_back(g_pAnarchyManager->GenerateUniqueId());
	std::vector<std::string> stims;
	stims.push_back("sound\\anarch5Jerec.png");
	stims.push_back("models\\Crash\\crate_basic.mdl");
	stims.push_back("sound\\art.jpg");
	stims.push_back("sound\\badart.jpg");
	for (unsigned int i = 0; i < stims.size(); i++)
	{
		args.push_back(stims[i]);
	}
	*/

	pBatchKV->SaveToFile(g_pFullFileSystem, "upload_batch_log.txt", "DEFAULT_WRITE_PATH");

	// callback
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
	pNetworkInstance->DispatchJavaScriptMethod("assetManager", "onUploadBatchReady", args);

	//g_pAnarchyManager->AddToastMessage(VarArgs("Syncing %i asset file(s) to the cloud...", iFileCount));
}

void C_MetaverseManager::GetAssetUploadBatch(Awesomium::WebView* pWebView, std::string batchId)
{
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
	if (!pNetworkInstance || pNetworkInstance->GetWebView() != pWebView)
		return;

	pWebView->InjectMouseMove(511, 511);
	pWebView->InjectMouseMove(2, 2);
	pWebView->InjectMouseDown(Awesomium::MouseButton::kMouseButton_Left);
	pWebView->InjectMouseUp(Awesomium::MouseButton::kMouseButton_Left);
}

KeyValues* C_MetaverseManager::GetUploadBatch(std::string batchId)
{
	int index = m_uploadBatches.Find(batchId);
	if (index != m_uploadBatches.InvalidIndex())
		return m_uploadBatches.Element(index);
	else
		return null;

	/*
	auto it = m_uploadBatches.find(batchId);
	if (it != m_uploadBatches.end())
		return it->second;
	else
		return null;
		*/
}

void C_MetaverseManager::DestroyUploadBatch(std::string batchId)
{
	int index = m_uploadBatches.Find(batchId);
	if (index != m_uploadBatches.InvalidIndex())
	{
		KeyValues* pKV = m_uploadBatches.Element(index);
		if (pKV)
			pKV->deleteThis();

		m_uploadBatches.RemoveAt(index);
	}

	/*
	auto it = m_uploadBatches.find(batchId);
	if (it != m_uploadBatches.end())
	{
		it->second->deleteThis();
		m_uploadBatches.erase(it);
	}
	*/
}

void C_MetaverseManager::AddMaterialDependencyToUploadBatch(std::string batchId, KeyValues* pBatchParentKV, std::string parentFileHash, std::string fileHash, std::string attributeName, IMaterial* pMaterial, KeyValues* pMaterialKV)	// used by AddMaterialToUploadBatch
{
	bool bUseLoadedMaterialValues = false;
	char* packingPath = "GAME";
	bool foundVar = false;
	IMaterialVar* pMaterialTextureVar = NULL;
	std::string mashupHash = "";

	if (attributeName == "replace")
	{
		KeyValues* pReplaceKV = pMaterialKV->FindKey("replace");
		if (pReplaceKV)
		{
			// Cycle through the different material vars
			std::vector<std::string> textureAttributeNames;
			textureAttributeNames.push_back("$basetexture");
			textureAttributeNames.push_back("$texture2");
			textureAttributeNames.push_back("$bumpmap");
			textureAttributeNames.push_back("$normalmap");
			textureAttributeNames.push_back("$normalmap2");
			textureAttributeNames.push_back("$envmap");
			textureAttributeNames.push_back("$envmapmask");
			textureAttributeNames.push_back("$hdrbasetexture");
			textureAttributeNames.push_back("$hdrcompressedtexture");
			textureAttributeNames.push_back("$iris");
			textureAttributeNames.push_back("$corneatexture");
			textureAttributeNames.push_back("$ambientoccltexture");
			textureAttributeNames.push_back("$phongexponenttexture");
			textureAttributeNames.push_back("$lightwarptexture");
			textureAttributeNames.push_back("$selfillummask");
			textureAttributeNames.push_back("$detail");

			for (unsigned int i = 0; i < textureAttributeNames.size(); i++)
			{
				if (bUseLoadedMaterialValues)
				{
					//this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], pMaterial, pReplaceKV, customSubFolder);
					std::string goodParentFileHash = (parentFileHash != "") ? parentFileHash : fileHash;
					this->AddMaterialDependencyToUploadBatch(batchId, pBatchParentKV, goodParentFileHash, fileHash, textureAttributeNames[i], pMaterial, pReplaceKV);// pMaterialKV);
				}
				else
				{
					//this->AdoptMaterialDependency(pAdoptedFilesKV, textureAttributeNames[i], null, pReplaceKV, customSubFolder);

					std::string goodParentFileHash = (parentFileHash != "") ? parentFileHash : fileHash;
					this->AddMaterialDependencyToUploadBatch(batchId, pBatchParentKV, goodParentFileHash, fileHash, textureAttributeNames[i], null, pReplaceKV);// pMaterialKV);
				}
			}
		}
	}
	else if (attributeName == "include")
	{
		std::string includeFileName = "";	// a VMT but w/ the .vmt at the end too.
		IMaterial* pIncludeMaterial = null;	// might remain null if the material isn't loaded but the file DOES exist.

		// 1 - Get the includeFileName from the given material or material name.
		if (pMaterial)
		{
			pMaterialTextureVar = pMaterial->FindVar(attributeName.c_str(), &foundVar, false);
			if (foundVar && pMaterialTextureVar->IsDefined())
				includeFileName = VarArgs("%s", pMaterialTextureVar->GetStringValue());
		}

		if (includeFileName == "" && pMaterialKV)
			includeFileName = pMaterialKV->GetString(attributeName.c_str());

		char* fixIncludeFileName = VarArgs("%s", includeFileName.c_str());
		V_FixSlashes(fixIncludeFileName);
		V_FixDoubleSlashes(fixIncludeFileName);
		includeFileName = fixIncludeFileName;

		auto found = includeFileName.find(".\\");
		while (found != std::string::npos)
		{
			includeFileName.erase(found, 2);
			found = includeFileName.find(".\\");
		}

		found = includeFileName.find("..\\");
		while (found != std::string::npos)
		{
			includeFileName.erase(found, 3);
			found = includeFileName.find("..\\");
		}

		//DevMsg("Include File Name: %s\n", includeFileName.c_str());

		if (includeFileName != "" && filesystem->FileExists(includeFileName.c_str(), packingPath))
		{
			std::string includeMaterialName = "";	// this must always be filled out, even if the material is not loaded yet.
			std::string testIncludeMaterialName = includeFileName;	// This should be the same as includeFileName, but w/o the .vmt at the end.
			size_t foundExtension = testIncludeMaterialName.find(".vmt");
			if (foundExtension == std::string::npos)
				foundExtension = testIncludeMaterialName.find(".VMT");

			if (foundExtension != std::string::npos)
				testIncludeMaterialName = testIncludeMaterialName.substr(0, foundExtension);

			char* fixTestIncludeMaterialName = VarArgs("%s", testIncludeMaterialName.c_str());
			V_FixSlashes(fixTestIncludeMaterialName);
			V_FixDoubleSlashes(fixTestIncludeMaterialName);
			testIncludeMaterialName = fixTestIncludeMaterialName;

			auto found = testIncludeMaterialName.find(".\\");
			while (found != std::string::npos)
			{
				testIncludeMaterialName.erase(found, 2);
				found = testIncludeMaterialName.find(".\\");
			}

			found = testIncludeMaterialName.find("..\\");
			while (found != std::string::npos)
			{
				testIncludeMaterialName.erase(found, 3);
				found = testIncludeMaterialName.find("..\\");
			}

			// Remove the materials/ prefix.

			size_t foundPrefix = testIncludeMaterialName.find("materials\\");
			if (foundPrefix == 0)
				testIncludeMaterialName = testIncludeMaterialName.substr(foundPrefix + 10);

			includeMaterialName = testIncludeMaterialName;

			// 2 - Attempt to get a material pointer, if the material is loaded.  But it's fine if we can't find the material pointer, because the VMT DOES exist.
			if (includeMaterialName != "")
			{
				IMaterial* pTestMaterial = null;

				if (bUseLoadedMaterialValues)
				{
					pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_WORLD);

					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_MODEL);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_VGUI);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_PARTICLE);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_DECAL);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_SKYBOX);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_OTHER);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_PRECACHED);
					if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
						pTestMaterial = g_pMaterialSystem->FindMaterial(includeMaterialName.c_str(), TEXTURE_GROUP_UNACCOUNTED);
				}

				if (pTestMaterial && !pTestMaterial->IsErrorMaterial())
					pIncludeMaterial = pTestMaterial;

				// 3 - Add it to the batch
				//this->AdoptMaterial(pAdoptedFilesKV, pIncludeMaterial, includeMaterialName.c_str(), customSubFolder);
				this->AddMaterialToUploadBatch(null, batchId, pBatchParentKV, includeMaterialName, parentFileHash);
			}
		}
	}
	else
	{
		// Add the [attributeName] VTF to the list
		std::string testFileName = "";
		if (pMaterial)
		{
			pMaterialTextureVar = pMaterial->FindVar(attributeName.c_str(), &foundVar, false);
			if (foundVar && pMaterialTextureVar->IsDefined())
				testFileName = VarArgs("materials/%s.vtf", pMaterialTextureVar->GetStringValue());
		}

		if (testFileName == "" && pMaterialKV)
		{
			testFileName = pMaterialKV->GetString(attributeName.c_str());

			if (testFileName != "")
				testFileName = VarArgs("materials/%s.vtf", testFileName.c_str());
		}

		if (testFileName != "" && (attributeName != "$envmap" || Q_stricmp(testFileName.c_str(), "materials/env_cubemap.vtf")))
		{
			char* fixTestFileName = VarArgs("%s", testFileName.c_str());
			V_FixSlashes(fixTestFileName);
			V_FixDoubleSlashes(fixTestFileName);

			testFileName = fixTestFileName;

			auto found = testFileName.find(".\\");
			while (found != std::string::npos)
			{
				testFileName.erase(found, 2);
				found = testFileName.find(".\\");
			}

			found = testFileName.find("..\\");
			while (found != std::string::npos)
			{
				testFileName.erase(found, 3);
				found = testFileName.find("..\\");
			}

			std::replace(testFileName.begin(), testFileName.end(), '\\', '/');

			// VTF file
			// other/id[MASHUP_HASH]
			if (filesystem->FileExists(testFileName.c_str(), "GAME"))
			{
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(testFileName.c_str());
				pBatchParentKV->SetString(VarArgs("id%s/other/id%s", parentFileHash.c_str(), mashupHash.c_str()), testFileName.c_str());
			}
		}
	}
}

std::string C_MetaverseManager::AddMaterialToUploadBatch(IMaterial* pMaterial, std::string batchId, KeyValues* pBatchParentKV, std::string materialNameOverride, std::string parentFileHash)
{
	bool bIsNewBatch = false;
	if (batchId == "")
	{
		batchId = this->CreateUploadBatch();
		bIsNewBatch = true;
	}

	if (!pBatchParentKV)
		return batchId;

	KeyValues* materialKV = new KeyValues("material");

	std::string testFileName = "";
	std::string valueBuf = "";

	std::string materialName = "";	// always blank, for now.
	if (!pMaterial && materialNameOverride != "")
	{
		testFileName = "materials/";
		testFileName += materialNameOverride;
		testFileName += ".vmt";

		// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
		if (!materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), "GAME"))
		{
			materialKV->deleteThis();
			return batchId;
		}

		materialName = materialNameOverride;
	}

	std::string buf;

	// Add this VMT to the download list
	std::string materialFilenameShort = (pMaterial) ? pMaterial->GetName() : materialName;

	char* fixMaterialFilenameShort = VarArgs("%s", materialFilenameShort.c_str());
	V_FixSlashes(fixMaterialFilenameShort);
	V_FixDoubleSlashes(fixMaterialFilenameShort);
	materialFilenameShort = fixMaterialFilenameShort;

	auto found = materialFilenameShort.find(".\\");
	while (found != std::string::npos)
	{
		materialFilenameShort.erase(found, 2);
		found = materialFilenameShort.find(".\\");
	}

	found = materialFilenameShort.find("..\\");
	while (found != std::string::npos)
	{
		materialFilenameShort.erase(found, 3);
		found = materialFilenameShort.find("..\\");
	}

	std::string materialFile = VarArgs("materials/%s.vmt", materialFilenameShort.c_str());

	std::replace(materialFile.begin(), materialFile.end(), '\\', '/');

	std::string fileHash = g_pAnarchyManager->GenerateLegacyHash(materialFile.c_str());

	// VMT file
	// file
	if (filesystem->FileExists(materialFile.c_str(), "GAME"))
	{
		//DevMsg("Parent File Hash Is: %s\n", parentFileHash.c_str());
		if (parentFileHash != "")
			pBatchParentKV->SetString(VarArgs("id%s/other/id%s", parentFileHash.c_str(), fileHash.c_str()), materialFile.c_str());
		else
			pBatchParentKV->SetString(VarArgs("id%s/file", fileHash.c_str()), materialFile.c_str());
	}

	// Cycle through the different material vars
	std::string mashupHash;
	bool foundVar = false;
	IMaterialVar* pTexture = NULL;
	std::vector<std::string> textureAttributeNames;
	textureAttributeNames.push_back("$basetexture");
	textureAttributeNames.push_back("$texture2");
	textureAttributeNames.push_back("$bumpmap");
	textureAttributeNames.push_back("$normalmap");
	textureAttributeNames.push_back("$normalmap2");
	textureAttributeNames.push_back("$envmap");
	textureAttributeNames.push_back("$envmapmask");
	textureAttributeNames.push_back("$hdrbasetexture");
	textureAttributeNames.push_back("$hdrcompressedtexture");
	textureAttributeNames.push_back("$iris");
	textureAttributeNames.push_back("$corneatexture");
	textureAttributeNames.push_back("$ambientoccltexture");
	textureAttributeNames.push_back("$phongexponenttexture");
	textureAttributeNames.push_back("$lightwarptexture");
	textureAttributeNames.push_back("$selfillummask");
	textureAttributeNames.push_back("$detail");
	textureAttributeNames.push_back("include");
	textureAttributeNames.push_back("replace");

	std::string goodParentFileHash = (parentFileHash != "") ? parentFileHash : fileHash;

	for (unsigned int i = 0; i < textureAttributeNames.size(); i++)
		this->AddMaterialDependencyToUploadBatch(batchId, pBatchParentKV, goodParentFileHash, fileHash, textureAttributeNames[i], pMaterial, materialKV);

	materialKV->deleteThis();
	return batchId;
}

std::string C_MetaverseManager::AddModelToUploadBatch(std::string modelId, std::string batchId, std::string modelFileOverride, KeyValues* pBatchParentKV)
{
	bool bIsNewBatch = false;
	if (batchId == "")
	{
		batchId = this->CreateUploadBatch();
		bIsNewBatch = true;
	}

	KeyValues* pBatchKV = (pBatchParentKV) ? pBatchParentKV : this->GetUploadBatch(batchId);
	if (!pBatchKV)
		return batchId;
	
	KeyValues* pModelKV = null;
	if (modelId != "")
		pModelKV = this->GetActiveKeyValues(this->GetLibraryModel(modelId));

	if (!pModelKV && modelFileOverride == "")
		return batchId;

	std::string modelFile = (pModelKV) ? pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)) : modelFileOverride;
	if (modelFile == "")
		return batchId;

	std::replace(modelFile.begin(), modelFile.end(), '\\', '/');

	auto found = modelFile.find("./");
	while (found != std::string::npos)
	{
		modelFile.erase(found, 2);
		found = modelFile.find("./");
	}

	found = modelFile.find("../");
	while (found != std::string::npos)
	{
		modelFile.erase(found, 3);
		found = modelFile.find("../");
	}

	std::string fileHash = g_pAnarchyManager->GenerateLegacyHash(modelFile.c_str());
	std::string modelFileBaseName;
	found = modelFile.find_last_of(".");
	if (found != std::string::npos)
		modelFileBaseName = modelFile.substr(0, found);

	//PathTypeQuery_t pathTypeQuery;
	//char* path = new char[AA_MAX_STRING];

	// MDL file
	// models/id[FILE_HASH]/file
	if (filesystem->FileExists(modelFile.c_str(), "GAME"))
	{
		//g_pFullFileSystem->RelativePathToFullPath(modelFile.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
		//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			pBatchKV->SetString(VarArgs("models/id%s/file", fileHash.c_str()), modelFile.c_str());
	}

	std::string mashupName;
	std::string mashupHash;
	if (modelFileBaseName != "")
	{
		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".dx80.vtx";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".dx90.vtx";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".phy";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".sw.vtx";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".vvd";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// models/id[FILE_HASH]/other
		mashupName = modelFileBaseName + ".xbox.vtx";
		if (filesystem->FileExists(mashupName.c_str(), "GAME"))
		{
			//g_pFullFileSystem->RelativePathToFullPath(mashupName.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
			//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)
			//{
				//std::replace(mashupName.begin(), mashupName.end(), '\\', '/');
				mashupHash = g_pAnarchyManager->GenerateLegacyHash(mashupName.c_str());
				//DevMsg("Mashup (%s): %s\n", mashupHash.c_str(), mashupName.c_str());
				pBatchKV->SetString(VarArgs("models/id%s/other/id%s", fileHash.c_str(), mashupHash.c_str()), mashupName.c_str());
			//}
		}

		// Load the model and find all of its materials (actually only the first 32 used on it)
		const model_t* TheModel = modelinfo->FindOrLoadModel(modelFile.c_str());
		int iNumMaterials = 32;//modelinfo->GetModelMaterialCount(TheModel);
		IMaterial** pMaterials = new IMaterial*[iNumMaterials];
		for (unsigned int i = 0; i < iNumMaterials; i++)
			pMaterials[i] = null;
		modelinfo->GetModelMaterials(TheModel, iNumMaterials, pMaterials);

		bool bUseLoadedMaterialValues = false;

		std::string buf;
		KeyValues* pModelBatchParentKV = pBatchKV->FindKey(VarArgs("models/id%s/materials", fileHash.c_str()), true);
		for (int x = 0; x < iNumMaterials && pMaterials[x]; x++)
		{
			if (pMaterials[x])
			{
				buf = pMaterials[x]->GetName();
				//if (buf.find("models\\smarcade\\") != 0 && buf.find("models/smarcade/") != 0 )

				if ( bUseLoadedMaterialValues )
					this->AddMaterialToUploadBatch(pMaterials[x], batchId, pModelBatchParentKV);
				else
				{
					std::string goodMaterialName = "";	// always blank, for now.
					std::string testMaterialName = pMaterials[x]->GetName();
					//DevMsg("testMaterialName: %s\n", testMaterialName.c_str());
					if (testMaterialName != "")
					{
						std::string testFileName = "materials/";
						testFileName += testMaterialName;
						testFileName += ".vmt";

						KeyValues* materialKV = new KeyValues("material");
						// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
						if (materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), "GAME"))
							goodMaterialName = testMaterialName;
						materialKV->deleteThis();

						if (goodMaterialName != "")
						{
							//KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
							this->AddMaterialToUploadBatch(null, batchId, pModelBatchParentKV, goodMaterialName);
						}
					}
				}
			}
		}
		
		delete[] pMaterials;
	}

	return batchId;
}

void C_MetaverseManager::SendBatch(std::string batchId)
{
	KeyValues* pBatchKV = this->GetUploadBatch(batchId);
	if (!pBatchKV)
		return;

	// generate a unique list
	std::string buf;
	std::string bufHash;
	KeyValues* pUniqueKV = pBatchKV->FindKey("unique", true);

	KeyValues* pBatchMapsKV = pBatchKV->FindKey("maps", true);
	KeyValues* pBatchModelsKV = pBatchKV->FindKey("models", true);

	KeyValues* pMapMaterialsKV;
	KeyValues* pMapMaterialKV;
	KeyValues* pMapModelsKV;
	KeyValues* pMapModelKV;

	KeyValues* pModelOtherKV;
	KeyValues* pOtherKV;
	KeyValues* pModelMaterialsKV;
	KeyValues* pModelMaterialKV;
	KeyValues* pMaterialOtherKV;
	for (KeyValues *pBatchModelKV = pBatchModelsKV->GetFirstSubKey(); pBatchModelKV; pBatchModelKV = pBatchModelKV->GetNextKey())
	{
		// MDL
		//DevMsg("Model: %s\n", pBatchModelKV->GetString("file"));
		buf = pBatchModelKV->GetString("file");
		pUniqueKV->SetString(pBatchModelKV->GetName(), buf.c_str());

		// OTHER
		pModelOtherKV = pBatchModelKV->FindKey("other", true);
		for (pOtherKV = pModelOtherKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
		{
			//DevMsg("\tFile (%s): %s\n", pOtherKV->GetName(), pOtherKV->GetString(""));
			buf = pOtherKV->GetString("");
			pUniqueKV->SetString(pOtherKV->GetName(), buf.c_str());
		}

		// MATERIALS
		pModelMaterialsKV = pBatchModelKV->FindKey("materials", true);
		for (pModelMaterialKV = pModelMaterialsKV->GetFirstSubKey(); pModelMaterialKV; pModelMaterialKV = pModelMaterialKV->GetNextKey())
		{
			// VMT
			//DevMsg("\tMaterial: %s\n", pModelMaterialKV->GetString("file"));
			buf = pModelMaterialKV->GetString("file");
			pUniqueKV->SetString(pModelMaterialKV->GetName(), buf.c_str());

			// OTHER
			pMaterialOtherKV = pModelMaterialKV->FindKey("other", true);
			for (pOtherKV = pMaterialOtherKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
			{
				//DevMsg("\t\tFile: %s\n", pOtherKV->GetString());
				buf = pOtherKV->GetString("");
				pUniqueKV->SetString(pOtherKV->GetName(), buf.c_str());
			}
		}
	}

	for (KeyValues *pBatchMapKV = pBatchMapsKV->GetFirstSubKey(); pBatchMapKV; pBatchMapKV = pBatchMapKV->GetNextKey())
	{
		// BSP
		//DevMsg("Map: %s\n", pBatchMapKV->GetString("file"));
		buf = pBatchMapKV->GetString("file");// VarArgs("maps/%s", pBatchMapKV->GetString("file"));
		pUniqueKV->SetString(pBatchMapKV->GetName(), buf.c_str());

		// MAP MATERIALS
		pMapMaterialsKV = pBatchMapKV->FindKey("materials", true);
		for (pMapMaterialKV = pMapMaterialsKV->GetFirstSubKey(); pMapMaterialKV; pMapMaterialKV = pMapMaterialKV->GetNextKey())
		{
			// VMT
			//DevMsg("\tMaterial: %s\n", pModelMaterialKV->GetString("file"));
			buf = pMapMaterialKV->GetString("file");
			pUniqueKV->SetString(pMapMaterialKV->GetName(), buf.c_str());

			// OTHER
			pMaterialOtherKV = pMapMaterialKV->FindKey("other", true);
			for (pOtherKV = pMaterialOtherKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
			{
				//DevMsg("\t\tFile: %s\n", pOtherKV->GetString());
				buf = pOtherKV->GetString("");
				pUniqueKV->SetString(pOtherKV->GetName(), buf.c_str());
			}
		}

		// MAP MODELS
		pMapModelsKV = pBatchMapKV->FindKey("models", true);
		for (pMapModelKV = pMapModelsKV->GetFirstSubKey(); pMapModelKV; pMapModelKV = pMapModelKV->GetNextKey())
		{
			// MDL
			//DevMsg("Model: %s\n", pMapModelKV->GetString("file"));
			buf = pMapModelKV->GetString("file");
			pUniqueKV->SetString(pMapModelKV->GetName(), buf.c_str());

			// OTHER
			pModelOtherKV = pMapModelKV->FindKey("other", true);
			for (pOtherKV = pModelOtherKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
			{
				//DevMsg("\tFile (%s): %s\n", pOtherKV->GetName(), pOtherKV->GetString(""));
				buf = pOtherKV->GetString("");
				pUniqueKV->SetString(pOtherKV->GetName(), buf.c_str());
			}

			// MATERIALS
			pModelMaterialsKV = pMapModelKV->FindKey("materials", true);
			for (pModelMaterialKV = pModelMaterialsKV->GetFirstSubKey(); pModelMaterialKV; pModelMaterialKV = pModelMaterialKV->GetNextKey())
			{
				// VMT
				//DevMsg("\tMaterial: %s\n", pModelMaterialKV->GetString("file"));
				buf = pModelMaterialKV->GetString("file");
				pUniqueKV->SetString(pModelMaterialKV->GetName(), buf.c_str());

				// OTHER
				pMaterialOtherKV = pModelMaterialKV->FindKey("other", true);
				for (pOtherKV = pMaterialOtherKV->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
				{
					//DevMsg("\t\tFile: %s\n", pOtherKV->GetString());
					buf = pOtherKV->GetString("");
					pUniqueKV->SetString(pOtherKV->GetName(), buf.c_str());
				}
			}
		}
	}

	this->OnAssetUploadBatchReady();
}

std::string C_MetaverseManager::AddSingleMaterialToUploadBatch(std::string materialName, std::string batchId)
{
	batchId = this->AddMaterialToUploadBatch(null, batchId, null, materialName);
	return batchId;
}

std::string C_MetaverseManager::AddSingleObjectToUploadBatch(std::string objectId, std::string batchId)
{
	batchId = this->AddObjectToUploadBatch(objectId, batchId);
	this->SendBatch(batchId);
	return batchId;
}

bool C_MetaverseManager::IsOwnProjectMapByFileName(std::string fileName)
{
	std::string searchFileName = fileName;
	std::transform(searchFileName.begin(), searchFileName.end(), searchFileName.begin(), ::tolower);
	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));
	std::string ownedFileEnding = VarArgs("%s.bsp", userHash.c_str());

	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(searchFileName, tokens, "_");
	if (tokens.size() > 0)
	{
		if (tokens.at(tokens.size() - 1) == ownedFileEnding)
			return true;
	}

	return false;
}

bool C_MetaverseManager::IsOwnProjectMap(std::string mapId, bool bIgnoreCloudMode)
{
	std::string goodMapId = mapId;
	if (goodMapId == "")
	{
		if (!bIgnoreCloudMode && (!engine->IsInGame() || !g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || !g_pAnarchyManager->GetConnectedUniverse()->isHost || !cvar->FindVar("cloud_assets_upload")->GetBool()))
			return false;

		instance_t* instance = g_pAnarchyManager->GetInstanceManager()->GetInstance(g_pAnarchyManager->GetInstanceId());
		if (!instance)
			return false;

		goodMapId = instance->mapId;
	}

	KeyValues* pMapKV = this->GetActiveKeyValues(this->GetLibraryMap(goodMapId));

	std::string bspFile = pMapKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
	if (bspFile == "")
		return false;

	std::string searchFileName = bspFile;
	std::transform(searchFileName.begin(), searchFileName.end(), searchFileName.begin(), ::tolower);
	std::string userHash = g_pAnarchyManager->GenerateLegacyHash(VarArgs("%llu", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64()));
	std::string ownedFileEnding = VarArgs("%s.bsp", userHash.c_str());

	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(searchFileName, tokens, "_");
	if (tokens.size() > 0)
	{
		if (tokens.at(tokens.size() - 1) == ownedFileEnding)
			return true;
	}

	return false;
}

void C_MetaverseManager::RequestAddSingleMapToUploadBatch(std::string mapId)
{
	//KeyValues* pMapKV = this->GetActiveKeyValues(this->GetLibraryMap(mapId));
	//if (!pMapKV)
	//{
	//	DevMsg("ERROR: Could not find map KeyValue entry.\n");
	//	return;
	//}

	//std::string bspFile = pMapKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
	//if (bspFile == "")
	//	return;

	//if (this->IsOwnProjectMap(bspFile))
	if (this->IsOwnProjectMap(mapId))
	{
		g_pAnarchyManager->GetMetaverseManager()->SetNextUploadBatchAddMap(mapId);
		//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		return;
	}

	DevMsg("Only maps managed from Main Menu - Maps - Level Designer can be synced.\n");
}

std::string C_MetaverseManager::AddSingleMapToUploadBatch(std::string mapId, std::string batchId)
{
	batchId = this->AddMapToUploadBatch(mapId, batchId);
	this->SendBatch(batchId);
	return batchId;
}

std::string C_MetaverseManager::AddObjectToUploadBatch(std::string objectId, std::string batchId)
{
	bool bIsNewBatch = false;
	if (batchId == "")
	{
		batchId = this->CreateUploadBatch();
		bIsNewBatch = true;
	}

	KeyValues* pBatchKV = this->GetUploadBatch(batchId);
	if (!pBatchKV)
		return batchId;

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!pObject)
		return batchId;

	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
	if (!pBaseEntity)
		DevMsg("WARNING: Object %s has not yet spawned. Asset files may fail to detect.\n");

	// we'll be gettin info about the model & the item
	KeyValues* pItemKV = this->GetActiveKeyValues(this->GetLibraryItem(pObject->itemId));
	KeyValues* pModelKV = this->GetActiveKeyValues(this->GetLibraryModel(pObject->modelId));
	if (!pModelKV && pItemKV)	// TODO: Standardized the check to obtain the modelKV of objects into a subroutine.
		pModelKV = pItemKV;

	// MODEL
	std::string modelName = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
	if (modelName != "")
	{
		//DevMsg("Add model %s to the batch!\n", modelName.c_str());
		this->AddModelToUploadBatch(pModelKV->GetString("info/id"), batchId);
	}

	// ITEM
	// TODO: support syncing local images as object textures (at least for server host)

	return batchId;
}

std::string C_MetaverseManager::AddMapToUploadBatch(std::string mapId, std::string batchId)
{
	bool bUseLoadedMaterialValues = false;

	bool bIsNewBatch = false;
	if (batchId == "")
	{
		batchId = this->CreateUploadBatch();
		bIsNewBatch = true;
	}

	KeyValues* pBatchKV = this->GetUploadBatch(batchId);
	if (!pBatchKV)
		return batchId;

	;
	// we'll be gettin info about the model & the item
	KeyValues* pMapKV = this->GetActiveKeyValues(this->GetLibraryMap(mapId));

	std::string bspFile = pMapKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
	if (bspFile == "")
		return batchId;

	bspFile = "maps/" + bspFile;

	std::replace(bspFile.begin(), bspFile.end(), '\\', '/');

	auto found = bspFile.find("./");
	while (found != std::string::npos)
	{
		bspFile.erase(found, 2);
		found = bspFile.find("./");
	}

	found = bspFile.find("../");
	while (found != std::string::npos)
	{
		bspFile.erase(found, 3);
		found = bspFile.find("../");
	}

	std::string fileHash = g_pAnarchyManager->GenerateLegacyHash(bspFile.c_str());
	//std::string bspFileBaseName;
	//size_t found = bspFile.find_last_of(".");
	//if (found != std::string::npos)
	//	bspFileBaseName = bspFile.substr(0, found);

	//PathTypeQuery_t pathTypeQuery;
	//char* path = new char[AA_MAX_STRING];

	// BSP file
	// maps/id[FILE_HASH]/file
	if (filesystem->FileExists(bspFile.c_str(), "GAME")) //(VarArgs("maps/%s", bspFile.c_str()), "GAME"))
	{
		//g_pFullFileSystem->RelativePathToFullPath(modelFile.c_str(), "GAME", path, AA_MAX_STRING, FILTER_CULLPACK, &pathTypeQuery);
		//if (pathTypeQuery == PATH_IS_NORMAL && strstr(path, ".vpk\\") == null && strstr(path, ".vpk/") == null && strstr(path, ".VPK\\") == null && strstr(path, ".VPK/") == null)

		pBatchKV->SetString(VarArgs("maps/id%s/file", fileHash.c_str()), bspFile.c_str());

		// now attempt to load up the map's VMF file from Anarchy Arcade/aarcade_user/mapsrc
		// scan it for every world material & model referneced from it and add them to the batch
		// TODO: There is 1 additional place in the code where these "other" files must be processed.
		// They'll actually have a models & materials subkey too, just like objects do.
		// fin

		// There are TWO ways to scan a map for models & materials:
		// A: If possible, load the original VMF from aarcade_user/mapsrc/[shortMapFile].vmf and scan it for asset references - as it is highly accurate & the map doesn't need to be currently loaded.
		// B: If no VMF is available AND the BSP is currently loaded, the currently loaded world can be scanned instead.

		// Check if VMF is available...
		std::string shortFileName = g_pAnarchyManager->MapName();

		std::string projectName = shortFileName;
		auto found = projectName.find_last_of("_");
		if (found != std::string::npos)
			projectName = projectName.substr(0, found);

		std::string assetFile;
		KeyValues* pDetectedAssets = new KeyValues("assets");

		bool bDidLoadVMF = false;
		if (g_pFullFileSystem->FileExists(VarArgs("mapsrc/%s/%s.vmf", projectName.c_str(), shortFileName.c_str()), "DEFAULT_WRITE_PATH"))
		{
			KeyValues* vmf = new KeyValues("vmf");
			if (vmf->LoadFromFile(g_pFullFileSystem, VarArgs("mapsrc/%s/%s.vmf", projectName.c_str(), shortFileName.c_str()), "DEFAULT_WRITE_PATH"))
			{
				bDidLoadVMF = true;

				KeyValues* skyRef;
				KeyValues* modelRef;
				KeyValues* solidMaterialRef;
				std::vector<std::string> skyboxNameVariations;
				skyboxNameVariations.push_back("bk");
				skyboxNameVariations.push_back("dn");
				skyboxNameVariations.push_back("ft");
				skyboxNameVariations.push_back("lf");
				skyboxNameVariations.push_back("rt");
				skyboxNameVariations.push_back("up");
				skyboxNameVariations.push_back("_hdrbk");
				skyboxNameVariations.push_back("_hdrdn");
				skyboxNameVariations.push_back("_hdrft");
				skyboxNameVariations.push_back("_hdrlf");
				skyboxNameVariations.push_back("_hdrrt");
				skyboxNameVariations.push_back("_hdrup");

				// We only care about parent keys who are:
				// - world
				// - entity*
				for (KeyValues* kv = vmf; kv; kv = kv->GetNextKey())
				{
					if (!Q_stricmp(kv->GetName(), "world"))
					{
						// TODO: work

						// We only care about these keys:
						// - skyname
						// - solid*

						skyRef = kv->FindKey("skyname");
						if (skyRef)
						{
							std::string skyboxName = skyRef->GetString();
							if (skyboxName != "")
							{
								for (unsigned int i = 0; i < skyboxNameVariations.size(); i++)
								{
									assetFile = "skybox/" + skyboxName + skyboxNameVariations[i];
									//if (assetFile.find("models/smarcade/") == 0)	// only dynamic textures live in here, so don't ever distribute them (yet) until there is a better blacklist implemented that excludes vanilla files.
									//	continue;

									std::replace(assetFile.begin(), assetFile.end(), '\\', '/');
									std::transform(assetFile.begin(), assetFile.end(), assetFile.begin(), ::tolower);

									auto found = assetFile.find("./");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 2);
										found = assetFile.find("./");
									}

									found = assetFile.find("../");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 3);
										found = assetFile.find("../");
									}

									bool bNeedsAdd = true;
									for (KeyValues* tsub = pDetectedAssets->GetFirstSubKey(); tsub; tsub = tsub->GetNextKey())
									{
										if (!Q_strcmp(tsub->GetString(), assetFile.c_str()))
										{
											bNeedsAdd = false;
											break;
										}
									}

									if (bNeedsAdd)
									{
										//std::string materialNameOverride;

										// pack this material
										IMaterial* pMaterial = null;
										if (bUseLoadedMaterialValues)
										{
											g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_WORLD);
											if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_MODEL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_VGUI);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_PARTICLE);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_DECAL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_SKYBOX);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_CLIENT_EFFECTS);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_OTHER);
											//else if (g_pFullFileSystem->FileExists(VarArgs("materials/skybox/%s.vmt", assetFile.c_str(), "GAME")))
											//	materialNameOverride = assetFile;
										}

										if (pMaterial && !pMaterial->IsErrorMaterial())
										{
											KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
											this->AddMaterialToUploadBatch(pMaterial, batchId, pBatchParentKV);

											// materials
											KeyValues* key = pDetectedAssets->CreateNewKey();	// NOTE: This bookkeeping might best be more aggressive to avoid checking of the SAME material over & over w/ fail
											key->SetString("", assetFile.c_str());
										}
										else
										{
											std::string materialName = "";	// always blank, for now.
											if (assetFile != "")
											{
												std::string testFileName = "materials/";
												testFileName += assetFile;
												testFileName += ".vmt";

												KeyValues* materialKV = new KeyValues("material");
												// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
												if (materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), "GAME"))
													materialName = assetFile;
												materialKV->deleteThis();

												if (materialName != "")
												{
													KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
													this->AddMaterialToUploadBatch(null, batchId, pBatchParentKV, materialName);

													// materials
													KeyValues* key = pDetectedAssets->CreateNewKey();	// NOTE: This bookkeeping might best be more aggressive to avoid checking of the SAME material over & over w/ fail
													key->SetString("", assetFile.c_str());
												}
											}
										}
									}
								}
							}
						}

						for (KeyValues* sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey())
						{
							if (Q_stricmp(sub->GetName(), "solid"))
								continue;

							// We only care about these keys:
							// - side*
							for (KeyValues* side = sub->GetFirstSubKey(); side; side = side->GetNextKey())
							{
								if (Q_stricmp(side->GetName(), "side"))
									continue;

								// We only care about these keys:
								// - material
								solidMaterialRef = side->FindKey("material");
								if (solidMaterialRef)
								{
									assetFile = solidMaterialRef->GetString();
									//if (assetFile.find("models/smarcade/") == 0)	// only dynamic textures live in here, so don't ever distribute them (yet) until there is a better blacklist implemented that excludes vanilla files.
									//	continue;

									std::replace(assetFile.begin(), assetFile.end(), '\\', '/');
									std::transform(assetFile.begin(), assetFile.end(), assetFile.begin(), ::tolower);

									auto found = assetFile.find("./");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 2);
										found = assetFile.find("./");
									}

									found = assetFile.find("../");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 3);
										found = assetFile.find("../");
									}

									bool bNeedsAdd = true;
									for (KeyValues* tsub = pDetectedAssets->GetFirstSubKey(); tsub; tsub = tsub->GetNextKey())
									{
										if (!Q_strcmp(tsub->GetString(), assetFile.c_str()))
										{
											bNeedsAdd = false;
											break;
										}
									}

									if (bNeedsAdd)
									{
										// pack this material
										IMaterial* pMaterial = null;
										if (bUseLoadedMaterialValues)
										{
											pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_WORLD);
											if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_MODEL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_VGUI);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_PARTICLE);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_DECAL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_SKYBOX);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_CLIENT_EFFECTS);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_OTHER);
										}

										if (pMaterial && !pMaterial->IsErrorMaterial())
										{
											KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
											this->AddMaterialToUploadBatch(pMaterial, batchId, pBatchParentKV);

											// materials
											KeyValues* key = pDetectedAssets->CreateNewKey();
											key->SetString("", assetFile.c_str());
										}
										else
										{
											std::string materialName = "";	// always blank, for now.
											if (assetFile != "")
											{
												std::string testFileName = "materials/";
												testFileName += assetFile;
												testFileName += ".vmt";

												KeyValues* materialKV = new KeyValues("material");
												// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
												if (materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), "GAME"))
													materialName = assetFile;
												materialKV->deleteThis();

												if (materialName != "")
												{
													KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
													this->AddMaterialToUploadBatch(null, batchId, pBatchParentKV, materialName);

													// materials
													KeyValues* key = pDetectedAssets->CreateNewKey();	// NOTE: This bookkeeping might best be more aggressive to avoid checking of the SAME material over & over w/ fail
													key->SetString("", assetFile.c_str());
												}
											}
										}
									}
								}
							}
						}
					}
					else if (!Q_stricmp(kv->GetName(), "entity"))
					{
						// We only care about these keys:
						// - model
						// - solid
						modelRef = kv->FindKey("model");
						if (modelRef)
						{
							assetFile = modelRef->GetString();

							std::replace(assetFile.begin(), assetFile.end(), '\\', '/');
							std::transform(assetFile.begin(), assetFile.end(), assetFile.begin(), ::tolower);

							auto found = assetFile.find("./");
							while (found != std::string::npos)
							{
								assetFile.erase(found, 2);
								found = assetFile.find("./");
							}

							found = assetFile.find("../");
							while (found != std::string::npos)
							{
								assetFile.erase(found, 3);
								found = assetFile.find("../");
							}

							DevMsg("Asset file is: %s\n", assetFile.c_str());

							bool bNeedsAdd = true;
							if (!g_pFullFileSystem->FileExists(assetFile.c_str(), "GAME"))
								bNeedsAdd = false;
							else
							{
								for (KeyValues* tsub = pDetectedAssets->GetFirstSubKey(); tsub; tsub = tsub->GetNextKey())
								{
									if (!Q_strcmp(tsub->GetString(), assetFile.c_str()))
									{
										bNeedsAdd = false;
										break;
									}
								}
							}

							if (bNeedsAdd)
							{
								KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s", fileHash.c_str()), true);

								std::string modelHash = g_pAnarchyManager->GenerateLegacyHash(assetFile.c_str());
								this->AddModelToUploadBatch(modelHash, batchId, assetFile, pBatchParentKV);

								// models
								KeyValues* key = pDetectedAssets->CreateNewKey();
								key->SetString("", assetFile.c_str());
							}
						}

						for (KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey())
						{
							if (Q_stricmp(sub->GetName(), "solid"))
								continue;

							// We only care about these keys:
							// - side*
							for (KeyValues *side = sub->GetFirstSubKey(); side; side = side->GetNextKey())
							{
								if (Q_stricmp(side->GetName(), "side"))
									continue;

								// We only care about these keys:
								// - material
								solidMaterialRef = side->FindKey("material");
								if (solidMaterialRef)
								{
									assetFile = solidMaterialRef->GetString();

									std::replace(assetFile.begin(), assetFile.end(), '\\', '/');
									std::transform(assetFile.begin(), assetFile.end(), assetFile.begin(), ::tolower);

									auto found = assetFile.find("./");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 2);
										found = assetFile.find("./");
									}

									found = assetFile.find("../");
									while (found != std::string::npos)
									{
										assetFile.erase(found, 3);
										found = assetFile.find("../");
									}

									bool bNeedsAdd = true;
									for (KeyValues* tsub = pDetectedAssets->GetFirstSubKey(); tsub; tsub = tsub->GetNextKey())
									{
										if (!Q_strcmp(tsub->GetString(), assetFile.c_str()))
										{
											bNeedsAdd = false;
											break;
										}
									}

									if (bNeedsAdd)
									{
										// pack this material
										IMaterial* pMaterial = null;
										if (bUseLoadedMaterialValues)
										{
											g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_WORLD);
											if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_MODEL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_VGUI);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_PARTICLE);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_DECAL);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_SKYBOX);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_CLIENT_EFFECTS);
											else if (!pMaterial || pMaterial->IsErrorMaterial())
												pMaterial = g_pMaterialSystem->FindMaterial(assetFile.c_str(), TEXTURE_GROUP_OTHER);
										}

										if (pMaterial && !pMaterial->IsErrorMaterial())
										{
											KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
											this->AddMaterialToUploadBatch(pMaterial, batchId, pBatchParentKV);

											// materials
											KeyValues* key = pDetectedAssets->CreateNewKey();
											key->SetString("", assetFile.c_str());
										}
										else
										{
											std::string materialName = "";	// always blank, for now.
											if (assetFile != "")
											{
												std::string testFileName = "materials/";
												testFileName += assetFile;
												testFileName += ".vmt";

												KeyValues* materialKV = new KeyValues("material");
												// A KeyValues for the VMT *must* be loaded to proceed if no material is passed to us.
												if (materialKV->LoadFromFile(g_pFullFileSystem, testFileName.c_str(), "GAME"))
													materialName = assetFile;
												materialKV->deleteThis();

												if (materialName != "")
												{
													KeyValues* pBatchParentKV = pBatchKV->FindKey(VarArgs("maps/id%s/materials", fileHash.c_str()), true);
													this->AddMaterialToUploadBatch(null, batchId, pBatchParentKV, materialName);

													// materials
													KeyValues* key = pDetectedAssets->CreateNewKey();	// NOTE: This bookkeeping might best be more aggressive to avoid checking of the SAME material over & over w/ fail
													key->SetString("", assetFile.c_str());
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				vmf->deleteThis();
				return batchId;
			}

			vmf->deleteThis();
		}
		else
			return batchId;

		//for (KeyValues *sub = pDetectedAssets->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		//{
		//	DevMsg("File: %s\n", sub->GetString());
		//}
		pDetectedAssets->deleteThis();
	}

	pBatchKV->SaveToFile(g_pFullFileSystem, "map_upload_manifest.txt", "DEFAULT_WRITE_PATH");
	//return batchId;

	this->OnUploadStatusAdded(batchId, bspFile);

	// TODO: Load up the VMF file from the aarcade_user/mapsrc folder and detect all referenced models & materials, then add those to the batch too.
	// (This will allow people to use any texture or model when creating their custom maps in Hammer, without having to worry about what is vanilla content and what isn't.)

	return batchId;
}

void C_MetaverseManager::OnUploadStatusSuccess(std::string batchId, int iBatchSize, int iNumCompleted, std::string batchFile, std::string file, int iSize)
{
	KeyValues* pUploadKV = m_pUploadsLogKV->FindKey(batchId.c_str(), true);
	pUploadKV->SetInt("total", iBatchSize);
	pUploadKV->SetInt("current", iNumCompleted);
	pUploadKV->SetString("file", batchFile.c_str());
	pUploadKV->SetInt("size", pUploadKV->GetInt("size") + iSize);
	pUploadKV->SetString("id", batchId.c_str());

	if (iBatchSize == iNumCompleted)
		pUploadKV->SetString("status", "uploaded");
	else
		pUploadKV->SetString("status", "uploading");

	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		JSObject batch;
		AddSubKeys2(pUploadKV, batch, true);
		arguments.Push(batch);

		object.InvokeAsync(WSLit("onUploadStatusSuccess"), arguments);
	}
}

void C_MetaverseManager::OnUploadStatusFailure(std::string batchId, int iBatchSize, int iNumCompleted, std::string batchFile, std::string file)
{
	KeyValues* pUploadKV = m_pUploadsLogKV->FindKey(batchId.c_str(), true);
	pUploadKV->SetInt("total", iBatchSize);
	pUploadKV->SetInt("current", iNumCompleted);
	pUploadKV->SetString("file", batchFile.c_str());
	pUploadKV->SetString("id", batchId.c_str());

	if (iBatchSize == iNumCompleted)
		pUploadKV->SetString("status", "uploaded");
	else
		pUploadKV->SetString("status", "uploading");

	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		JSObject batch;
		AddSubKeys2(pUploadKV, batch, true);
		arguments.Push(batch);

		object.InvokeAsync(WSLit("onUploadStatusFailure"), arguments);
	}
}

void C_MetaverseManager::OnUploadStatusSkipped(std::string batchId, int iBatchSize, int iNumCompleted, std::string batchFile, std::string file)
{
	KeyValues* pUploadKV = m_pUploadsLogKV->FindKey(batchId.c_str(), true);
	pUploadKV->SetInt("total", iBatchSize);
	pUploadKV->SetInt("current", iNumCompleted);
	pUploadKV->SetString("file", batchFile.c_str());
	pUploadKV->SetString("id", batchId.c_str());

	if (iBatchSize == iNumCompleted)
		pUploadKV->SetString("status", "uploaded");
	else
		pUploadKV->SetString("status", "uploading");

	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		JSObject batch;
		AddSubKeys2(pUploadKV, batch, true);
		arguments.Push(batch);

		object.InvokeAsync(WSLit("onUploadStatusSkipped"), arguments);
	}
}

void C_MetaverseManager::OnUploadStatusAdded(std::string batchId, std::string batchFile)
{
	KeyValues* pUploadKV = m_pUploadsLogKV->FindKey(batchId.c_str(), true);
	pUploadKV->SetString("file", batchFile.c_str());
	pUploadKV->SetString("status", "pending");
	pUploadKV->SetString("id", batchId.c_str());

	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		JSObject batch;
		AddSubKeys2(pUploadKV, batch, true);
		arguments.Push(batch);

		object.InvokeAsync(WSLit("onUploadStatusAdded"), arguments);
	}
}

void C_MetaverseManager::ClearUploadsLog()
{
	m_pUploadsLogKV->deleteThis();
	m_pUploadsLogKV = new KeyValues("uploads");
}

std::string C_MetaverseManager::CreateUploadBatch()
{
	std::string batchId = g_pAnarchyManager->GenerateUniqueId();

	KeyValues* pBatchKV = new KeyValues("batch");
	m_uploadBatches.InsertOrReplace(batchId, pBatchKV);
	//m_uploadBatches[batchId] = pBatchKV;
	return batchId;
}

void C_MetaverseManager::DestroyAllUploadBatches()
{
	// upload batches
	KeyValues* pKV;
	for (int iIndex = m_uploadBatches.FirstInorder(); iIndex != m_uploadBatches.InvalidIndex(); iIndex = m_uploadBatches.NextInorder(iIndex))
	{
		pKV = m_uploadBatches.Element(iIndex);
		if (pKV)
			pKV->deleteThis();
	}
	m_uploadBatches.RemoveAll();
	/*
	while (!m_uploadBatches.empty())
	{
		m_uploadBatches.begin()->second->deleteThis();
		m_uploadBatches.erase(m_uploadBatches.begin());
	}
	*/
}

void C_MetaverseManager::HandleAssetUploadBatch(std::string batchId)
{
	KeyValues* pBatchKV = this->GetUploadBatch(batchId);
	if (!pBatchKV)
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");
	if (!pNetworkInstance)
		return;

	WebStringArray myResponse;

	/*
	std::vector<std::string> stims;
	stims.push_back("sound\\anarch5Jerec.png");
	stims.push_back("models\\Crash\\crate_basic.mdl");
	stims.push_back("sound\\art.jpg");
	stims.push_back("sound\\badart.jpg");
	*/

	//int iFileCount = 0;
	char* path = new char[AA_MAX_STRING];
	PathTypeQuery_t pathTypeQuery;
	std::string fullPath;
	KeyValues* pUniqueKV = pBatchKV->FindKey("unique");
	if (pUniqueKV)
	{
		bool bValid;

		// If we turn out to be valid, we'll want to do some bookkeeping so the user can monitor upload progress on the UI.
		// So let's keep track of the the file sizes.
		//int iTotalZippedSize = 0;

		std::string relativeFile;;
		for (KeyValues *pFileKV = pUniqueKV->GetFirstSubKey(); pFileKV; pFileKV = pFileKV->GetNextKey())
		{
			std::string fullZip;
			bValid = true;

			relativeFile = pFileKV->GetString("");
			std::replace(relativeFile.begin(), relativeFile.end(), '\\', '/');

			g_pFullFileSystem->RelativePathToFullPath(relativeFile.c_str(), "GAME", path, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);
			if (pathTypeQuery != PATH_IS_NORMAL || strstr(path, ".vpk\\") != null || strstr(path, ".vpk/") != null || strstr(path, ".VPK\\") != null || strstr(path, ".VPK/") != null)
			{
				bool bCopied = false;
				CUtlBuffer buf;
				if (g_pFullFileSystem->ReadFile(relativeFile.c_str(), "GAME", buf))
				{
					std::string writePathOnly = VarArgs("upload/%s", relativeFile.c_str());
					writePathOnly = writePathOnly.substr(0, writePathOnly.find_last_of("/\\"));
					g_pFullFileSystem->CreateDirHierarchy(writePathOnly.c_str(), "DEFAULT_WRITE_PATH");
					if (g_pFullFileSystem->WriteFile(VarArgs("upload/%s", relativeFile.c_str()), "DEFAULT_WRITE_PATH", buf))
					{
						fullPath = g_pAnarchyManager->GetAArcadeUserFolder();
						std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
						fullPath += "/upload/";
						fullPath += relativeFile;
						if (g_pFullFileSystem->FileExists(fullPath.c_str()))
							bCopied = true;
					}
				}

				if (!bCopied)
				{
					DevMsg("Skipping file %s\n", relativeFile.c_str());
					bValid = false;
				}
			}
			else
				fullPath = path;

			/*
			if (IS_PACKFILE(pathTypeQuery))
			*/
			
			if (bValid && g_pFullFileSystem->FileExists(fullPath.c_str())) // && completePath[1] == ':' 
			{
				// The actual file is ready.  But is the ZIP version ready?
				std::string fileOnly = relativeFile.substr(relativeFile.find_last_of("/") + 1);
				std::string pathOnly = relativeFile.substr(0, relativeFile.find_last_of("/") + 1);

				std::string relativeZip = VarArgs("upload/%s.zip", relativeFile.c_str());
				fullZip = g_pAnarchyManager->GetAArcadeUserFolder();
				std::replace(fullZip.begin(), fullZip.end(), '\\', '/');
				fullZip += "/";
				fullZip += relativeZip;
				std::string fullZipPathOnly = fullZip.substr(0, fullZip.find_last_of("/") + 1);

				//std::string zipFileOnly = relativeZip.substr(relativeZip.find_last_of("/") + 1);
				std::string zipPathOnly = relativeZip.substr(0, relativeZip.find_last_of("/") + 1);

				// Create a ZIP
				// FIXME: The local ZIPs are created BEFORE the cloud checks which files it actually needs.  That means ZIPs could be created even if it turns out NOTHING needs to be uploaded. Optimize this!!
				bool bZipAlreadyExists = g_pFullFileSystem->FileExists(relativeZip.c_str(), "DEFAULT_WRITE_PATH");
				if (bZipAlreadyExists && (relativeFile.find(".bsp") == relativeFile.length() - 4 || relativeFile.find(".BSP") == relativeFile.length() - 4))
				{
					g_pFullFileSystem->RemoveFile(relativeZip.c_str(), "DEFAULT_WRITE_PATH");
					bZipAlreadyExists = false;
				}

				if (!bZipAlreadyExists)
				{
					g_pFullFileSystem->CreateDirHierarchy(zipPathOnly.c_str(), "DEFAULT_WRITE_PATH");
					HZIP hz = CreateZip(VarArgs("%s", fullZip.c_str()), 0, ZIP_FILENAME);

					if (!hz)
					{
						// FIX ME FIXME Should probably just re-try to ZIP the file if it fails the 1st time.
						Msg("Error - Failed to create ZIP file %s\n", fullZip.c_str());
						bValid = false;
					}
					else
					{
						//DevMsg("Zipped %s from %s\n", relativeFile.c_str(), fullPath.c_str());
						ZRESULT result = ZipAdd(hz, relativeFile.c_str(), VarArgs("%s", fullPath.c_str()), 0, ZIP_FILENAME);
						if (result != ZR_OK)
						{
							Msg("Error - Failed to add file %s to the ZIP file.\n", fullPath.c_str());
							g_pFullFileSystem->RemoveFile(fullZip.c_str());
							bValid = false;
						}
						CloseZip(hz);

						//iTotalZippedSize +=
					}
				}
			}

			if (bValid)
				myResponse.Push(WSLit(fullZip.c_str()));
			else
				myResponse.Push(WSLit(""));
			//iFileCount++;
		}
	}

	pNetworkInstance->GetWebView()->DidChooseFiles(myResponse, false);

	// we are now finished with this batch.
	this->DestroyUploadBatch(batchId);
	delete[] path;
}

KeyValues* C_MetaverseManager::LoadLocalItemLegacy(bool& bIsModel, bool& bWasAlreadyLoaded, std::string file, std::string filePath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack, std::string searchPath, bool bShouldAddToActiveLibrary)
{
	KeyValues* pItem = new KeyValues("item");
	bool bLoaded;
	
	bool bWasLoaded = false;

	std::string fullFile = filePath + file;
	if (searchPath != "")
	{
		bLoaded = pItem->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), searchPath.c_str());
	}
	else if (filePath != "")
		bLoaded = pItem->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), "");
	else
		bLoaded = pItem->LoadFromFile(g_pFullFileSystem, file.c_str(), "MOD");

	bool bResponseIsModel;
	if ( !bLoaded )
	{
		pItem->deleteThis();
		pItem = null;
		bResponseIsModel = false;
	}
	else
	{
		// flag us as being loaded from legacy
		pItem->SetBool("loadedFromLegacy", true);

		// determine the generation of this item
		KeyValues* pGeneration = pItem->FindKey("generation");
		if (!pGeneration)
		{
			// update us to 3rd generation
			pItem->SetInt("generation", 3);

			// add standard info (except for id)
			//pItem->SetInt("local/info/created", 0);
			pItem->SetString("local/info/owner", "local");
			//pItem->SetInt("local/info/removed", 0);
			//pItem->SetString("local/info/remover", "");
			//pItem->SetString("local/info/alias", "");

			// determine if this is a model or not
			std::string itemFile = pItem->GetString("filelocation");
			std::string modelFile = pItem->GetString("filelocation");
			//if (modelFile.find("cabinets") >= 0)
				//DevMsg("Harrrr: %s\n", modelFile.c_str());
			size_t foundExt = modelFile.find(".mdl");
			if (foundExt == modelFile.length() - 4)
			{
				bResponseIsModel = true;
				std::string itemId = g_pAnarchyManager->GenerateLegacyHash(itemFile.c_str());
				
				pItem->SetString("local/info/id", itemId.c_str());
				pItem->SetString("local/title", pItem->GetString("title"));
				pItem->SetString("local/preview", pItem->GetString("trailerurl"));

				if (Q_strcmp(pItem->GetString("group"), ""))
					pItem->SetString("local/keywords", pItem->GetString("group"));
				//pItem->SetInt("local/dynamic", 0);
				pItem->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
				pItem->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), modelFile.c_str());
				pItem->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), "");

				pItem->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), workshopIds.c_str());
				pItem->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), mountIds.c_str());
//				pItem->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), backpackId.c_str());

				// models can be loaded right away because they don't depend on anything else, like items do. (items depend on models)
				//DevMsg("Loading model with ID %s and model %s\n", itemId.c_str(), modelFile.c_str());

				auto it = m_models.find(itemId);
				if (it != m_models.end())
				{
					pItem->deleteThis();
					pItem = it->second;
					bWasLoaded = true;
				}
				else if (bShouldAddToActiveLibrary)
					m_models[itemId] = pItem;
			}
			else
			{
				bResponseIsModel = false;

				std::string nodeId;

				// NEEDS RESOLVING!!
				std::string legacyType = pItem->GetString("type");
				std::string resolvedType = (pBackpack) ? this->ResolveLegacyType(legacyType, pBackpack->GetSQLDb(), pBackpack->GetTypesResponseMapPointer()) : this->ResolveLegacyType(legacyType);
				pItem->SetString("local/type", resolvedType.c_str());

				if (legacyType == "node" || resolvedType == this->GetSpecialTypeId("node") )
				{
					// Try to extract a legacy ID from the relative .set file path, cuz redux doesn't save shit that way anymore.
					nodeId = g_pAnarchyManager->ExtractLegacyId(itemFile);

					if (nodeId != "")
					{
						DevMsg("Legacy node consumed from workshop with extracted id: %s\n", nodeId.c_str());

						// Replace the local/file field with the nodeId instead.
						itemFile = nodeId;
					}
				}

				std::string itemId = g_pAnarchyManager->ExtractLegacyId(file, pItem);
				pItem->SetString("local/info/id", itemId.c_str());
				pItem->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), workshopIds.c_str());
				pItem->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), mountIds.c_str());
				//pItem->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), backpackId.c_str());

				pItem->SetString("local/title", pItem->GetString("title"));

				if (Q_strcmp(pItem->GetString("group"), ""))
					pItem->SetString("local/keywords", pItem->GetString("group"));

				pItem->SetString("local/description", pItem->GetString("description"));
				pItem->SetString("local/file", itemFile.c_str());
				//pItem->SetString("local/file", pItem->GetString("filelocation"));

				// NEEDS RESOLVING!!
				std::string legacyApp = pItem->GetString("app");
				std::string resolvedApp = this->ResolveLegacyApp(legacyApp);
				pItem->SetString("local/app", resolvedApp.c_str());
				pItem->SetString("local/reference", pItem->GetString("detailsurl"));
				pItem->SetString("local/preview", pItem->GetString("trailerurl"));
				pItem->SetString("local/download", pItem->GetString("downloadurl"));

				// NEEDS RESOLVING!!
				std::string resolvedScreen = pItem->GetString("screens/low");
				if (resolvedScreen == "")
				{
					resolvedScreen = pItem->GetString("screenslocation");
					if (resolvedScreen.find(":") != 1 || !g_pFullFileSystem->FileExists(resolvedScreen.c_str(), ""))
						resolvedScreen = "";
				}
				pItem->SetString("local/screen", resolvedScreen.c_str());


				/*
				// CACHE NEEDS RESOLVING!!
				//std::string legacyPath = "A:\\SteamLibrary\\steamapps\\common\\Anarchy Arcade\\aarcade\\";
				char fullPathBuf[AA_MAX_STRING];
				std::string resolvedScreenCached = pItem->GetString("screenslocation");
				//std::string resolvedScreenCached = VarArgs("%s\\%s\\screens\\%s.tbn", library_type.c_str(), newType.c_str(), this->GenerateHashX(itemfile_ref.c_str();
				//BuildItemKV->SetString("marqueeslocation", UTIL_VarArgs("%s\\%s\\marquees\\%s.tbn", library_type.c_str(), newType.c_str(), this->GenerateHashX(itemfile_ref.c_str())));
				//BuildItemKV->SetString("screenslocation", UTIL_VarArgs("%s\\%s\\screens\\%s.tbn", library_type.c_str(), newType.c_str(), this->GenerateHashX(itemfile_ref.c_str())));

				if (resolvedScreenCached != "" && resolvedScreenCached.find(":") != 1)
				{
					g_pFullFileSystem->RelativePathToFullPath(resolvedScreenCached.c_str(), "MOD", fullPathBuf, AA_MAX_STRING);
					resolvedScreenCached = fullPathBuf;

					if (resolvedScreenCached.find(":") != 1)
					{
						DevMsg("Testing sample path: %s\n", file.c_str());
						resolvedScreenCached = VarArgs("%s\\%s\\screens\\%s.tbn", "library", pItem->GetString("type"), g_pAnarchyManager->GenerateLegacyHash(file.c_str()));
						g_pFullFileSystem->RelativePathToFullPath(resolvedScreenCached.c_str(), "MOD", fullPathBuf, AA_MAX_STRING);
						resolvedScreenCached = fullPathBuf;

						if (resolvedScreenCached.find(":") != 1)
						{
							DevMsg("Testing sample path: %s\n", file.c_str());
							resolvedScreenCached = VarArgs("%s\\%s\\screens\\%s.tbn", "library_cache", pItem->GetString("type"), g_pAnarchyManager->GenerateLegacyHash(file.c_str()));
							g_pFullFileSystem->RelativePathToFullPath(resolvedScreenCached.c_str(), "MOD", fullPathBuf, AA_MAX_STRING);
							resolvedScreenCached = fullPathBuf;

							if (resolvedScreenCached.find(":") != 1)
								resolvedScreenCached = "";
						}
					}

					DevMsg("Arbys full path here is: %s\n", resolvedScreenCached.c_str());
				}
				else
					resolvedScreenCached = "";
				pItem->SetString("local/screencached", resolvedScreenCached.c_str());
				*/



				// NEEDS RESOLVING!!
				std::string resolvedMarquee = pItem->GetString("marquees/low");
				if (resolvedMarquee == "")
				{
					resolvedMarquee = pItem->GetString("marqueeslocation");
					if (resolvedMarquee.find(":") != 1 || !g_pFullFileSystem->FileExists(resolvedMarquee.c_str(), ""))
						resolvedMarquee = "";
				}
				pItem->SetString("local/marquee", resolvedMarquee.c_str());



				/*
				// CACHE NEEDS RESOLVING!!
				//std::string legacyPath = "A:\\SteamLibrary\\steamapps\\common\\Anarchy Arcade\\aarcade\\";
				//char fullPathBuf[AA_MAX_STRING];
				std::string resolvedMarqueeCached = pItem->GetString("marqueeslocation");
				if (resolvedMarqueeCached != "" && resolvedMarqueeCached.find(":") != 1)
				{
					g_pFullFileSystem->RelativePathToFullPath(resolvedMarqueeCached.c_str(), "MOD", fullPathBuf, AA_MAX_STRING);
					resolvedMarqueeCached = fullPathBuf;
					DevMsg("Arbys full path here is: %s\n", resolvedMarqueeCached.c_str());
				}
				else
					resolvedMarqueeCached = "";
				pItem->SetString("local/marqueecached", resolvedMarqueeCached.c_str());
				*/

				/*
				auto it = m_items.find(itemId);
				if (it != m_items.end())
				{
					pItem->deleteThis();
					pItem = it->second;
					bWasLoaded = true;
				}
				else if (bShouldAddToActiveLibrary)
					m_items[itemId] = pItem;
				*/

				if ( bShouldAddToActiveLibrary)
					m_previousLoadLocalItemsLegacyBuffer.push_back(pItem);
			}
		}
		else
			bResponseIsModel = false;
	}

	bIsModel = bResponseIsModel;
	bWasAlreadyLoaded = bWasLoaded;
	return pItem;
}

void C_MetaverseManager::LoadFirstLocalItemLegacy(bool bFastMode, std::string filePath, std::string workshopIds, std::string mountIds, C_Backpack* pBackpack)
{
	// only need to do this on the first one.
	//bool bBadFastMode = true;

	if (m_previousLoadLocalItemLegacyFilePath != "")
		this->LoadLocalItemLegacyClose();

	// start it
	m_previousLoadLocalItemLegacyFastMode = bFastMode;
	m_previousLoadLocalItemLegacyFilePath = filePath;
	m_previousLoadLocalItemLegacyWorkshopIds = workshopIds;
	m_previousLoadLocalItemLegacyMountIds = mountIds;
	m_pPreviousLoadLocalItemLegacyBackpack = pBackpack;

	unsigned int uMountWorkshopNumModels = 0;
	unsigned int uMountWorkshopNumItems = 0;

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	KeyValues* pItem;
	std::string fullPath = m_previousLoadLocalItemLegacyFilePath + "library\\*";
	const char *pFoldername = g_pFullFileSystem->FindFirst(fullPath.c_str(), &m_previousLoadLocalItemLegacyFolderHandle);
	while (pFoldername != NULL)
	{
		if (!Q_strcmp(pFoldername, ".") || !Q_strcmp(pFoldername, ".."))
		{
			pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
			continue;
		}

		m_previousLoadLocalItemLegacyFolderPath = VarArgs("library\\%s", pFoldername);
		if (!g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalItemLegacyFolderHandle))
		{
			DevMsg("All items files must be within a subfolder!\n");
			pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
			continue;
		}

		const char *pFilename = g_pFullFileSystem->FindFirst(VarArgs("%s%s\\*.itm", m_previousLoadLocalItemLegacyFilePath.c_str(), m_previousLoadLocalItemLegacyFolderPath.c_str()), &m_previousLoadLocalItemLegacyFileHandle);
		while (pFilename != NULL)
		{
			if (g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalItemLegacyFileHandle))
			{
				pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
				continue;
			}

			// WE'VE FOUND A FILE TO ATTEMPT TO LOAD!!!
			std::string foundName = VarArgs("%s\\%s", m_previousLoadLocalItemLegacyFolderPath.c_str(), pFilename);
			//pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);

			// MAKE THE FILE PATH NICE
			//char path_buffer[AA_MAX_STRING];
			int iAAMaxString = foundName.length() + 1;
			char* path_buffer = new char[iAAMaxString];
			Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
			V_FixSlashes(path_buffer);
			foundName = path_buffer;
			delete[] path_buffer;
			// FINISHED MAKING THE FILE PATH NICE

			bool bIsModel;
			bool bWasAlreadyLoaded;
			pItem = this->LoadLocalItemLegacy(bIsModel, bWasAlreadyLoaded, foundName, m_previousLoadLocalItemLegacyFilePath, m_previousLoadLocalItemLegacyWorkshopIds, m_previousLoadLocalItemLegacyMountIds, m_pPreviousLoadLocalItemLegacyBackpack);
			if (pItem)
			{
				if (bIsModel)
					uMountWorkshopNumModels++;
				else
					uMountWorkshopNumItems++;

				if (!bFastMode)
				{
					if (bIsModel)
					{
						if (m_previousLoadLocalItemLegacyWorkshopIds != "")
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
						else
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Models", "oldlibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
					}
					else
					{
						if (m_previousLoadLocalItemLegacyWorkshopIds != "")
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
						else
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Items", "oldlibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
					}
					return;
				}
			}

			pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
		}

		g_pFullFileSystem->FindClose(m_previousLoadLocalItemLegacyFileHandle);
		pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
		//break;
	}

	if (bFastMode)
	{
		if (m_previousLoadLocalItemLegacyWorkshopIds != "")
		{
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", std::string(VarArgs("+%u", uMountWorkshopNumModels)));
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", std::string(VarArgs("+%u", uMountWorkshopNumItems)));
		}
		else
		{
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Models", "oldlibrarymodels", "", "", std::string(VarArgs("+%u", uMountWorkshopNumModels)));
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Items", "oldlibraryitems", "", "", std::string(VarArgs("+%u", uMountWorkshopNumItems)));
		}
	}

	g_pAnarchyManager->GetWorkshopManager()->OnMountWorkshopSucceed();	
	this->LoadLocalItemLegacyClose();
	return;
}

void C_MetaverseManager::LoadNextLocalItemLegacy()
{
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	unsigned int uMountWorkshopNumModels = 0;
	unsigned int uMountWorkshopNumItems = 0;

	KeyValues* pItem;
	const char *pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalItemLegacyFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
			continue;
		}

		// WE'VE FOUND A FILE TO ATTEMPT TO LOAD!!!
		std::string foundName = VarArgs("%s\\%s", m_previousLoadLocalItemLegacyFolderPath.c_str(), pFilename);
		//pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);

		// MAKE THE FILE PATH NICE
		//char path_buffer[AA_MAX_STRING];
		int iAAMaxString = foundName.length() + 1;
		char* path_buffer = new char[iAAMaxString];
		Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
		V_FixSlashes(path_buffer);
		foundName = path_buffer;
		delete[] path_buffer;
		// FINISHED MAKING THE FILE PATH NICE

		bool bIsModel;
		bool bWasAlreadyLoaded;
		pItem = this->LoadLocalItemLegacy(bIsModel, bWasAlreadyLoaded, foundName, m_previousLoadLocalItemLegacyFilePath, m_previousLoadLocalItemLegacyWorkshopIds, m_previousLoadLocalItemLegacyMountIds, m_pPreviousLoadLocalItemLegacyBackpack);
		if (pItem)
		{
			if (bIsModel)
				uMountWorkshopNumModels++;
			else
				uMountWorkshopNumItems++;

			if (!m_previousLoadLocalItemLegacyFastMode)
			{
				if (bIsModel)
				{
					if (m_previousLoadLocalItemLegacyWorkshopIds != "")
						pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
					else
						pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Models", "oldlibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
				}
				else
				{
					if (m_previousLoadLocalItemLegacyWorkshopIds != "")
						pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
					else
						pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Items", "oldlibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
				}
				return;
			}
		}

		pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
		//break;
	}

	g_pFullFileSystem->FindClose(m_previousLoadLocalItemLegacyFileHandle);

	// done searching a folder, continue on to next folder
	std::string fullPath = m_previousLoadLocalItemLegacyFilePath + "library\\*";
	const char *pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
	while (pFoldername != NULL)
	{
		if (!Q_strcmp(pFoldername, ".") || !Q_strcmp(pFoldername, ".."))
		{
			pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
			continue;
		}

		m_previousLoadLocalItemLegacyFolderPath = VarArgs("library\\%s", pFoldername);
		if (!g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalItemLegacyFolderHandle))
		{
			DevMsg("All items files must be within a subfolder!\n");
			pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
			continue;
		}

		const char *pFilename = g_pFullFileSystem->FindFirst(VarArgs("%s%s\\*.itm", m_previousLoadLocalItemLegacyFilePath.c_str(), m_previousLoadLocalItemLegacyFolderPath.c_str()), &m_previousLoadLocalItemLegacyFileHandle);
		while (pFilename != NULL)
		{
			if (g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalItemLegacyFileHandle))
			{
				pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
				continue;
			}

			// WE'VE FOUND A FILE TO ATTEMPT TO LOAD!!!
			std::string foundName = VarArgs("%s\\%s", m_previousLoadLocalItemLegacyFolderPath.c_str(), pFilename);

			// MAKE THE FILE PATH NICE
			//char path_buffer[AA_MAX_STRING];
			int iAAMaxString = foundName.length() + 1;
			char* path_buffer = new char[iAAMaxString];
			Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
			V_FixSlashes(path_buffer);
			foundName = path_buffer;
			delete[] path_buffer;
			// FINISHED MAKING THE FILE PATH NICE

			bool bIsModel;
			bool bWasAlreadyLoaded;
			pItem = this->LoadLocalItemLegacy(bIsModel, bWasAlreadyLoaded, foundName, m_previousLoadLocalItemLegacyFilePath, m_previousLoadLocalItemLegacyWorkshopIds, m_previousLoadLocalItemLegacyMountIds, m_pPreviousLoadLocalItemLegacyBackpack);
			if (pItem)
			{
				if (bIsModel)
					uMountWorkshopNumModels++;
				else
					uMountWorkshopNumItems++;

				if (!m_previousLoadLocalItemLegacyFastMode)
				{
					if (bIsModel)
					{
						if (m_previousLoadLocalItemLegacyWorkshopIds != "")
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
						else
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Models", "oldlibrarymodels", "", "", "+", "loadNextLocalItemLegacyCallback");
					}
					else
					{
						if (m_previousLoadLocalItemLegacyWorkshopIds != "")
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
						else
							pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Items", "oldlibraryitems", "", "", "+", "loadNextLocalItemLegacyCallback");
					}
					return;
				}
			}

			pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFileHandle);
		}

		g_pFullFileSystem->FindClose(m_previousLoadLocalItemLegacyFileHandle);
		pFoldername = g_pFullFileSystem->FindNext(m_previousLoadLocalItemLegacyFolderHandle);
		//break;
	}

	if (m_previousLoadLocalItemLegacyFastMode)
	{
		if (m_previousLoadLocalItemLegacyWorkshopIds != "")
		{
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", std::string(VarArgs("+%u", uMountWorkshopNumModels)));
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", std::string(VarArgs("+%u", uMountWorkshopNumItems)));
		}
		else
		{
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Models", "oldlibrarymodels", "", "", std::string(VarArgs("+%u", uMountWorkshopNumModels)));
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Importing Old AArcade Items", "oldlibraryitems", "", "", std::string(VarArgs("+%u", uMountWorkshopNumItems)));
		}
	}

	g_pAnarchyManager->GetWorkshopManager()->OnMountWorkshopSucceed();
	//g_pAnarchyManager->GetMetaverseManager()->OnMountWorkshopSucceed();
	this->LoadLocalItemLegacyClose();
	return;
}

void C_MetaverseManager::LoadLocalItemLegacyClose()
{
	if (m_previousLoadLocalItemLegacyFilePath != "")
	{
//		g_pFullFileSystem->FindClose(m_previousLoadLocalItemLegacyFolderHandle);

//		if (m_previousLoadLocalItemLegacyFolderPath != "")
//			g_pFullFileSystem->FindClose(m_previousLoadLocalItemLegacyFileHandle);
		m_previousLoadLocalItemLegacyFastMode = false;
		m_previousLoadLocalItemLegacyFilePath = "";
		m_previousLoadLocalItemLegacyFolderPath = "";
		m_previousLoadLocalItemLegacyWorkshopIds = "";
		m_previousLoadLocalItemLegacyMountIds = "";
		m_pPreviousLoadLocalItemLegacyBackpack = null;
//		m_previousLoadLocalItemsLegacyBuffer.clear();
	}
}

void C_MetaverseManager::AddApp(KeyValues* pAppKV)
{
	KeyValues* pActiveKV = this->GetActiveKeyValues(pAppKV);
	std::string id = pActiveKV->GetString("info/id");
	m_apps[id] = pAppKV;
}

void C_MetaverseManager::ResolveLoadLocalItemLegacyBuffer()
{
	// FIXME This is a huge bottleneck

	// This usually gets done after all  workshop mounts are done, but since this is an after-the-fact one, gotta do it again manually here
	std::string legacyModelId;
	unsigned int numResponses = m_previousLoadLocalItemsLegacyBuffer.size();
	unsigned int i;
	unsigned int j;
	KeyValues* pCompoundItemKV;
	//KeyValues* active;
	unsigned int numVictims;
	std::vector<KeyValues*> victims;
	//KeyValues* model;
	//KeyValues* modelActive;
	for (i = 0; i < numResponses; i++)
	{
		pCompoundItemKV = m_previousLoadLocalItemsLegacyBuffer[i];

		// OSOLETE! Legacy models & items are no longer pre-resolved like this!
		//active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pCompoundItemKV);
		//std::string resolvedModel = g_pAnarchyManager->GenerateLegacyHash(pCompoundItemKV->GetString("lastmodel"));
		//active->SetString("model", resolvedModel.c_str());

		// legacy model (gets resolved upon map load)
		legacyModelId = pCompoundItemKV->GetString("lastmodel");

		// remove everything not in local or current or generation
		for (KeyValues *sub = pCompoundItemKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (Q_strcmp(sub->GetName(), "local") && Q_strcmp(sub->GetName(), "generation"))
				victims.push_back(sub);
		}

		numVictims = victims.size();
		for (j = 0; j < numVictims; j++)
			pCompoundItemKV->RemoveSubKey(victims[j]);

		if (numVictims > 0)
			victims.clear();

		// the itemKV is no longer compound!  It has been stripped down to a GEN3 item now!
		KeyValues* pItemKV = pCompoundItemKV;
		KeyValues* pActiveKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItemKV);

		//for (KeyValues *sub = pActiveKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		//	DevMsg("Key & Value: %s & %s\n", sub->GetName(), sub->GetString());

		std::string id = VarArgs("%s", pActiveKV->GetString("info/id"));
		DevMsg("Processing %s\n", id.c_str());
		KeyValues* pTestItemKV;
		KeyValues* pTestActiveKV;
		auto it = m_items.find(id);
		if (it != m_items.end())
		{
			pTestItemKV = it->second;
			pTestActiveKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pTestItemKV);

			// this is a legacy item, so give the right-of-way to any generation 3 item that was already found.
			if (pTestItemKV->GetInt("legacy") == 1)
			{
				// merg this legacy item with the other legacy item
				this->SmartMergItemKVs(pTestItemKV, pItemKV);
				pItemKV->deleteThis();
				pItemKV = it->second;

				//// FIXME: For now, just delete the old one and let this one overpower it.
				//it->second->deleteThis();
				//m_items.erase(it);
				//m_items[id] = pItem;
			}
			else
			{
				// let the generation 3 item overpower us
				pItemKV->deleteThis();
				pItemKV = it->second;
			}
		}
		else
			m_items[id] = pItemKV;
	}

	m_previousLoadLocalItemsLegacyBuffer.clear();
}

KeyValues* C_MetaverseManager::LoadLocalType(std::string file, std::string filePath)
{
	// TODO: IS THIS OBSOLETE?
	KeyValues* pType = new KeyValues("type");
	bool bLoaded;

	if (filePath != "")
	{
		std::string fullFile = filePath + file;
		bLoaded = pType->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), "");
	}
	else
		bLoaded = pType->LoadFromFile(g_pFullFileSystem, file.c_str(), "MOD");

	if (!bLoaded)
	{
		pType->deleteThis();
		pType = null;
	}
	else
	{
		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pType);
		std::string id = pActive->GetString("info/id");
		m_types[id] = pType;
	}

	return pType;
}

unsigned int C_MetaverseManager::LoadAllLocalTypes(sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;
	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, "SELECT * from types", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

	int length;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		if (length == 0)
		{
			DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
			continue;
		}

		KeyValues* pType = new KeyValues("type");

		CUtlBuffer buf(0, length, 0);
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pType->ReadAsBinary(buf);
		buf.Purge();

		// TODO: Look up any alias here first!!
		KeyValues* pActive = this->GetActiveKeyValues(pType);
		std::string id = pActive->GetString("info/id");
		if (pResponseMap)
			(*pResponseMap)[id] = pType;
		else
			m_types[id] = pType;
		
		//DevMsg("Loaded type %s from Redux library\n", pActive->GetString("title"));
		count++;
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.

	//DevMsg("Finished loading local types.\n");
	return count;

	/*
	unsigned int count = 0;
	FileFindHandle_t testFileHandle;
	const char *pFilename = g_pFullFileSystem->FindFirst(VarArgs("%slibrary\\types\\*.key", filePath.c_str()), &testFileHandle);

	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(testFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(testFileHandle);
			continue;
		}

		std::string foundName = pFilename;
		foundName = VarArgs("library\\types\\%s", pFilename);
		pFilename = g_pFullFileSystem->FindNext(testFileHandle);

		// MAKE THE FILE PATH NICE
		char path_buffer[AA_MAX_STRING];
		Q_strcpy(path_buffer, foundName.c_str());
		V_FixSlashes(path_buffer);

		for (int i = 0; path_buffer[i] != '\0'; i++)
			path_buffer[i] = tolower(path_buffer[i]);
		// FINISHED MAKING THE FILE PATH NICE

		foundName = path_buffer;
		if (this->LoadLocalType(foundName, filePath))
			count++;
	}

	g_pFullFileSystem->FindClose(testFileHandle);
	return count;
	*/
}

std::string C_MetaverseManager::ResolveLegacyType(std::string legacyType, sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;

	if (legacyType == "")
		return AA_DEFAULT_TYPEID;

	std::map<std::string, KeyValues*> responseMap = (pResponseMap) ? (*pResponseMap) : m_types;

	auto foundIt = responseMap.find(legacyType);
	if (foundIt != responseMap.end())
		return legacyType;

	// iterate through the types
	KeyValues* active;
	for (std::map<std::string, KeyValues*>::iterator it = responseMap.begin(); it != responseMap.end(); ++it)
	{
		active = this->GetActiveKeyValues(it->second);

		if (!Q_stricmp(active->GetString("title"), legacyType.c_str()))
			return it->first;
	}

	if (legacyType == "other" )
		return AA_DEFAULT_TYPEID;

	///*
	// If no type is found for this legacy type, create one!
	//DevMsg("Creating a type w/ title %s because it didn't exist yet.\n", legacyType.c_str());
	KeyValues* pTypeKV = new KeyValues("type");

	// update us to 3rd generation
	pTypeKV->SetInt("generation", 3);

	// add standard info (except for id)
	pTypeKV->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	pTypeKV->SetString("local/info/owner", "local");
	//pCurrent->SetInt("local/info/removed", 0);
	//pCurrent->SetString("local/info/remover", "");
	//pCurrent->SetString("local/info/alias", "");

	std::string typeId;
	// First check if there is an existing type ID for this type in the user's library.
	if (pResponseMap && responseMap != m_types)
	{
		auto foundIt = m_types.find(legacyType);
		if (foundIt != m_types.end())
			typeId = foundIt->first;

		if (typeId == "")
		{
			for (std::map<std::string, KeyValues*>::iterator it = m_types.begin(); it != m_types.end(); ++it)
			{
				active = this->GetActiveKeyValues(it->second);

				if (!Q_stricmp(active->GetString("title"), legacyType.c_str()))
				{
					typeId = it->first;
					break;
				}
			}
		}
	}

	if (typeId == "")
		typeId = g_pAnarchyManager->GenerateUniqueId();

	pTypeKV->SetString("local/info/id", typeId.c_str());

	//pCurrent->SetString("fileformat", "/.+$/i");
	//pCurrent->SetString("fileformat", "<AUTO>");	// will be processed after the apps are loaded so that file extensions can be added.
	//pCurrent->FindKey("fileformat", true);
	pTypeKV->SetString("local/titleformat", "/.+$/i");
	pTypeKV->SetString("local/title", legacyType.c_str());
	pTypeKV->SetInt("local/priority", 1);
	this->AddType(pTypeKV, pResponseMap);
	///////this->SaveType(pTypeKV);	// Save to the user database, so AArcade doesnt' get confused over types when the user unsubscribes from this addon. (if they created other items using the type from this addon, for example.)




	g_pAnarchyManager->GetMetaverseManager()->SaveSQL(&pDb, "types", typeId.c_str(), pTypeKV);
	
	return typeId;
	//*/
}

KeyValues* C_MetaverseManager::LoadLocalApp(std::string file, std::string filePath, std::string searchPath)
{
	KeyValues* pApp = new KeyValues("app");
	bool bLoaded;

//	if (filePath != "")
	//{
		std::string fullFile = filePath + file;
		bLoaded = pApp->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), searchPath.c_str());
	//}
	//else
		//bLoaded = pApp->LoadFromFile(g_pFullFileSystem, file.c_str(), "MOD");

	if (!bLoaded)
	{
		pApp->deleteThis();
		pApp = null;
	}
	else
	{
		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pApp);

		std::string id = pActive->GetString("info/id");
		m_apps[id] = pApp;
	}

	return pApp;
}

unsigned int C_MetaverseManager::LoadAllLocalInstances(sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;
	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, "SELECT * from instances", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));
	//DevMsg("C_MetaverseManager::LoadAllLocalInstances\n");
	int length;
	std::string rowId;

	int iNumSkippedZeroByte = 0;
	int iNumSkippedBlankIds = 0;
	int iNumSkippedConflictedId = 0;
	int iNumSkippedNoId = 0;
	int iNumSkippedEmpty = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		rowId = std::string((const char*)sqlite3_column_text(stmt, 0));
		length = sqlite3_column_bytes(stmt, 1);

		if (length == 0)
		{
			//DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
			iNumSkippedZeroByte++;
			continue;
		}

		KeyValues* pInstance = new KeyValues("instance");

		CUtlBuffer buf(0, length, 0);
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pInstance->ReadAsBinary(buf);
		buf.Purge();

		//for (KeyValues *sub = pInstance->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		//	DevMsg("%s is %s\n", sub->GetName(), sub->GetString());

		std::string instanceId = pInstance->GetString("info/local/id");
		if (instanceId == "")
		{
			//DevMsg("Skipping instance w/ blank ID.\n");
			iNumSkippedBlankIds++;
			continue;
		}

		if (instanceId != rowId)
		{
			//DevMsg("Skipping ID conflicted instance: rowid(%s) vs instanceid(%s)\n", rowId.c_str(), instanceId.c_str());
			iNumSkippedConflictedId++;
			continue;
		}
		//DevMsg("Instance id is: %s vs %s\n", instanceId.c_str(), rowId.c_str());
		if (instanceId == "")
		{
			//DevMsg("Skipping id-less instance...\n");
			iNumSkippedNoId++;
			continue;
		}

		if (!pInstance->FindKey("objects", true)->GetFirstSubKey())
		{
			//DevMsg("Skipping empty instance...\n");
			iNumSkippedEmpty++;
			continue;
		}

		if (pResponseMap)
			(*pResponseMap)[instanceId] = pInstance;	// FIXME: Make sure these get cleaned up for ALL LoadAllLocalInstances calls.
		else
		{
			//int generation = pInstance->GetInt("generation", 3);
			int iLegacy = pInstance->GetInt("legacy", 0);
			//DevMsg("But the other is: %s\n", pInstance->GetString("info/id"));
			KeyValues* pInstanceInfoKV = pInstance->FindKey("info/local", true);
			//std::string instanceId = pInstanceInfoKV->GetString("id");
			std::string mapId = pInstanceInfoKV->GetString("map");
			std::string title = pInstanceInfoKV->GetString("title");
			if (title == "")
				title = "Unnamed (" + instanceId + ")";
			std::string file = "";
			std::string workshopIds = pInstanceInfoKV->GetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID));
			std::string mountIds = pInstanceInfoKV->GetString(VarArgs("platforms/%s/mountIds", AA_PLATFORM_ID));
			//std::string backpackId = pInstanceInfoKV->GetString(VarArgs("platforms/%s/backpackIds", AA_PLATFORM_ID));
			std::string style = pInstanceInfoKV->GetString("style");
			std::string autoplayId = pInstanceInfoKV->GetString("autoplay");
			pInstance->deleteThis();
			pInstance = null;
			//DevMsg("Adding node of style: %s\n", style.c_str());
			g_pAnarchyManager->GetInstanceManager()->AddInstance(iLegacy, instanceId, mapId, title, file, workshopIds, mountIds, autoplayId, style);

			//m_instances[instanceId] = pInstance; //DevMsg("WARNING: No response map was given to LocalAllLocalInstances!\n");
		}
		count++;
	}

	//DevMsg("Loaded Instances: %i\n", count);
	if (iNumSkippedZeroByte > 0 || iNumSkippedBlankIds > 0 || iNumSkippedConflictedId > 0 || iNumSkippedNoId > 0 || iNumSkippedEmpty > 0)
		DevMsg("Skipped Loading Instances:\n\t%i\tZeroByte\n\t%i\tBlankIds\n\t%i\tConflictedIds\n\t%i\tNoId\n\t%i\tEmpty\n", iNumSkippedZeroByte, iNumSkippedBlankIds, iNumSkippedConflictedId, iNumSkippedNoId, iNumSkippedEmpty);

	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.
	return count;
}

unsigned int C_MetaverseManager::LoadAllLocalApps(sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;
	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, "SELECT * from apps", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

	int length;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		if (length == 0)
		{
			DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
			continue;
		}

		KeyValues* pApp = new KeyValues("app");

		CUtlBuffer buf(0, length, 0);
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pApp->ReadAsBinary(buf);
		buf.Purge();

		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pApp);
		std::string id = pActive->GetString("info/id");

		if (pResponseMap)
			(*pResponseMap)[id] = pApp;
		else
			m_apps[id] = pApp;

		count++;
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.
	return count;
}

std::string C_MetaverseManager::ResolveLegacyApp(std::string legacyApp)
{
	if (legacyApp == "")
		return "";

	// iterate through the types
	KeyValues* active;
	for (std::map<std::string, KeyValues*>::iterator it = m_apps.begin(); it != m_apps.end(); ++it)
	{
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		if (!Q_stricmp(active->GetString("title"), legacyApp.c_str()))
			return it->first;
	}

	return "";
}

KeyValues* C_MetaverseManager::LoadFirstLocalApp(std::string filePath)
{
	if (m_previousLoadLocalAppFilePath != "")
		this->LoadLocalAppClose();

	m_previousLoadLocalAppFilePath = filePath;
	const char *pFilename = g_pFullFileSystem->FindFirstEx("library\\apps\\*.key", filePath.c_str(), &m_previousLoadLocalAppFileHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalAppFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalAppFileHandle);
			continue;
		}

		std::string foundName = pFilename;
		foundName = VarArgs("library\\apps\\%s", pFilename);
		
		// MAKE THE FILE PATH NICE
		//char path_buffer[AA_MAX_STRING];
		int iAAMaxString = foundName.length() + 1;
		char* path_buffer = new char[iAAMaxString];
		Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
		V_FixSlashes(path_buffer);

		for (int i = 0; path_buffer[i] != '\0'; i++)
			path_buffer[i] = tolower(path_buffer[i]);
		// FINISHED MAKING THE FILE PATH NICE

		foundName = path_buffer;
		delete[] path_buffer;
		return this->LoadLocalApp(foundName, "", m_previousLoadLocalAppFilePath);
	}

	return null;
}

KeyValues* C_MetaverseManager::LoadNextLocalApp()
{
//	if ( != "")
//		g_pFullFileSystem->FindClose(m_previousLoadLocalAppFileHandle);

	const char *pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalAppFileHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(m_previousLoadLocalAppFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(m_previousLoadLocalAppFileHandle);
			continue;
		}

		std::string foundName = pFilename;
		foundName = VarArgs("library\\apps\\%s", pFilename);

		// MAKE THE FILE PATH NICE
		//char path_buffer[AA_MAX_STRING];
		int iAAMaxString = foundName.length() + 1;
		char* path_buffer = new char[iAAMaxString];
		Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
		V_FixSlashes(path_buffer);

		for (int i = 0; path_buffer[i] != '\0'; i++)
			path_buffer[i] = tolower(path_buffer[i]);
		// FINISHED MAKING THE FILE PATH NICE

		foundName = path_buffer;
		delete[] path_buffer;
		return this->LoadLocalApp(foundName, "", m_previousLoadLocalAppFilePath);
	}

	//g_pFullFileSystem->FindClose(m_previousLoadLocalAppFileHandle);
	//m_previousLoadLocalAppFilePath = "";
	return null;
}

void C_MetaverseManager::LoadLocalAppClose()
{
	if (m_previousLoadLocalAppFilePath != "")
	{
		g_pFullFileSystem->FindClose(m_previousLoadLocalAppFileHandle);
		m_previousLoadLocalAppFilePath = "";
	}
}

KeyValues* C_MetaverseManager::GetFirstLibraryType()
{
	m_previousGetTypeIterator = m_types.begin();
	if (m_previousGetTypeIterator != m_types.end())
		return m_previousGetTypeIterator->second;

	return null;
}

KeyValues* C_MetaverseManager::GetNextLibraryType()
{
	if (m_previousGetTypeIterator != m_types.end())
	{
		m_previousGetTypeIterator++;
		
		if (m_previousGetTypeIterator != m_types.end())
			return m_previousGetTypeIterator->second;
	}

	return null;
}
/*
KeyValues* C_MetaverseManager::GetFirstLibraryApp()
{
	m_previousGetAppIterator = m_apps.begin();
	if (m_previousGetAppIterator != m_apps.end())
		return m_previousGetAppIterator->second;

	return null;
}

KeyValues* C_MetaverseManager::GetNextLibraryApp()
{
	if (m_previousGetAppIterator != m_apps.end())
	{
		m_previousGetAppIterator++;

		if (m_previousGetAppIterator != m_apps.end())
			return m_previousGetAppIterator->second;
	}

	return null;
}
*/

/*
KeyValues* C_MetaverseManager::FindLibraryType(std::string term)
{
	// note: only finds exact matches.  IS case sensi
	auto it = m_types.begin();
	while (it != m_types.end())
	{
		if (it->first == term)
			return it->second;

		it++;
	}

	return null;
}
*/

KeyValues* C_MetaverseManager::GetLibraryType(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_types.find(id);
	if (it != m_types.end())
		return it->second;
	else
		return null;
}

std::string C_MetaverseManager::GetSpecialTypeId(std::string typeTitle)
{
	KeyValues* pTypeKV;
	std::map<std::string, KeyValues*>::iterator it = m_types.begin();// find(id);
	while (it != m_types.end())
	{
		pTypeKV = this->GetActiveKeyValues(it->second);
		if (pTypeKV && std::string(pTypeKV->GetString("title")) == typeTitle)
			return std::string(pTypeKV->GetString("info/id"));

		it++;
	}

	// FUTURE-PROOF WAY OF FINDING IMPORTANT SHIT:
	// If we can't find it, add it to the library.

	pTypeKV = new KeyValues("type");
	if (pTypeKV->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\types\\%s.txt", typeTitle.c_str()), "MOD"))
	{
		KeyValues* active = this->GetActiveKeyValues(pTypeKV);
		this->SaveSQL(null, "types", active->GetString("info/id"), pTypeKV);
		this->AddType(pTypeKV);

		return std::string(active->GetString("info/id"));
	}

	DevMsg("WARNING: Could NOT find special type with title %s\n", typeTitle.c_str());
	return "";
}

std::string C_MetaverseManager::GetSpecialModelId(std::string modelType)
{
	std::string file = "";
	if (modelType == "node")
	{
		if (Q_strcmp(m_pNodeModelConVar->GetString(), ""))
			file = m_pNodeModelConVar->GetString();
		else
			file = "models\\cabinets\\node.mdl";
	}

	KeyValues* pSearchInfo = new KeyValues("search");
	pSearchInfo->SetString("file", file.c_str());

	KeyValues* pModelKV = this->GetActiveKeyValues(this->FindLibraryModel(pSearchInfo));
	if (pModelKV)
		return std::string(pModelKV->GetString("info/id"));


	// FUTURE-PROOF WAY OF FINDING IMPORTANT SHIT:
	// If we can't find it, add it to the library.

	KeyValues* pCabinetKV = new KeyValues("model");
	if (pCabinetKV->LoadFromFile(g_pFullFileSystem, VarArgs("defaultLibrary\\cabinets\\%s.txt", modelType.c_str()), "MOD"))
	{
		KeyValues* active = this->GetActiveKeyValues(pCabinetKV);
		this->SaveSQL(null, "models", active->GetString("info/id"), pCabinetKV);
		this->AddModel(pCabinetKV);

		return std::string(active->GetString("info/id"));
	}

	DevMsg("WARNING: Could NOT find special model with file %s\n", file.c_str());
	return "";
}

std::string C_MetaverseManager::GetFirstLibraryEntry(KeyValues*& response, const char* category)//const char* 
{
	//char queryId[20];
	//const char* queryId = g_pAnarchyManager->GenerateUniqueId();
	//g_pAnarchyManager->GenerateUniqueId(queryId);

	//const char*
	std::string queryId = g_pAnarchyManager->GenerateUniqueId();
	DevMsg("Here id is: %s\n", queryId.c_str());

	//std::string idBuf = std::string(queryId);

	// TODO: use queryId to organize N search queries & store the category info for each.
	//*
	if (!Q_strcmp(category, "items"))
	{
		m_previousGetItemIterator = m_items.begin();
		response = (m_previousGetItemIterator != m_items.end()) ? m_previousGetItemIterator->second : null;
	}
	else if(!Q_strcmp(category, "models"))
	{
		m_previousGetModelIterator = m_models.begin();
		response = (m_previousGetModelIterator != m_models.end()) ? m_previousGetModelIterator->second : null;
	}
	else if(!Q_strcmp(category, "apps"))
	{
		m_previousGetAppIterator = m_apps.begin();
		response = (m_previousGetAppIterator != m_apps.end()) ? m_previousGetAppIterator->second : null;
	}
	//*/

	/*
	auto categoryEntries = (!Q_strcmp(category, "items")) ? m_items : m_models;
	auto it = (!Q_strcmp(category, "items")) ? m_previousGetItemIterator : m_previousGetModelIterator;

	it = categoryEntries.begin();

	if (it != categoryEntries.end())
		response = it->second;
	else
		response = null;
	*/

	return queryId;
}
KeyValues* C_MetaverseManager::GetNextLibraryEntry(const char* queryId, const char* category)
{
	///*
	//DevMsg("AAaaannnd here it is: %s and %s\n", queryId, category);
	if (!Q_strcmp(category, "items"))
	{
		//DevMsg("Has it: %i\n", m_previousGetItemIterator);
		if (m_previousGetItemIterator != m_items.end())
			m_previousGetItemIterator++;

		return (m_previousGetItemIterator != m_items.end()) ? m_previousGetItemIterator->second : null;
	}
	else if(!Q_strcmp(category, "models"))
	{
		if (m_previousGetModelIterator != m_models.end())
			m_previousGetModelIterator++;

		return (m_previousGetModelIterator != m_models.end()) ? m_previousGetModelIterator->second : null;
	}
	else if (!Q_strcmp(category, "apps"))
	{
		if (m_previousGetAppIterator != m_models.end())
			m_previousGetAppIterator++;

		return (m_previousGetAppIterator != m_apps.end()) ? m_previousGetAppIterator->second : null;
	}
	//*/

	/*
	// TODO: use queryId to organize N search queries & store the category info for each.
	auto categoryEntries = (!Q_strcmp(category, "items")) ? m_items : m_models;
	auto it = (!Q_strcmp(category, "items")) ? m_previousGetItemIterator : m_previousGetModelIterator;

	if (it != categoryEntries.end())
	{
		it++;

		if (it != categoryEntries.end())
			return it->second;
	}
	*/

	return null;
}

// LEGACY! OBSOLETE!!
KeyValues* C_MetaverseManager::GetFirstLibraryItem()
{
	m_previousGetItemIterator = m_items.begin();
	if (m_previousGetItemIterator != m_items.end())
		return m_previousGetItemIterator->second;

	return null;
}

KeyValues* C_MetaverseManager::GetNextLibraryItem()
{
	if (m_previousGetItemIterator != m_items.end())
	{
		m_previousGetItemIterator++;

		if (m_previousGetItemIterator != m_items.end())
			return m_previousGetItemIterator->second;
	}

	return null;
}

KeyValues* C_MetaverseManager::GetFirstLibraryModel()
{
	m_previousGetModelIterator = m_models.begin();
	if (m_previousGetModelIterator != m_models.end())
		return m_previousGetModelIterator->second;

	return null;
}

KeyValues* C_MetaverseManager::GetNextLibraryModel()
{
	if (m_previousGetModelIterator != m_models.end())
	{
		m_previousGetModelIterator++;

		if (m_previousGetModelIterator != m_models.end())
			return m_previousGetModelIterator->second;
	}

	return null;
}

KeyValues* C_MetaverseManager::GetFirstLibraryApp()
{
	m_previousGetAppIterator = m_apps.begin();
	if (m_previousGetAppIterator != m_apps.end())
		return m_previousGetAppIterator->second;

	return null;
}

KeyValues* C_MetaverseManager::GetNextLibraryApp()
{
	if (m_previousGetAppIterator != m_apps.end())
	{
		m_previousGetAppIterator++;

		if (m_previousGetAppIterator != m_apps.end())
			return m_previousGetAppIterator->second;
	}

	return null;
}

KeyValues* C_MetaverseManager::GetLibraryEntry(std::string category, std::string id)
{
	std::map<std::string, KeyValues*>* pCategoryEntries;
	if (category == "items")
		pCategoryEntries = &m_items;
	else if (category == "models")
		pCategoryEntries = &m_models;
	else if (category == "apps")
		pCategoryEntries = &m_apps;
	else if (category == "maps")
		pCategoryEntries = &m_maps;
	else if (category == "types")
		pCategoryEntries = &m_types;
	else
	{
		DevMsg("Warning: Unknown library category %s referenced.\n", category.c_str());
		pCategoryEntries = &m_items;
	}

	std::map<std::string, KeyValues*>::iterator it = pCategoryEntries->find(id);// m_items.find(id);
	if (it != pCategoryEntries->end())// m_items.end())
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::GetLibraryItem(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_items.find(id);
	if (it != m_items.end())
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::GetLibraryModel(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_models.find(id);
	if (it != m_models.end())
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryModel(KeyValues* pSearchInfo, std::map<std::string, KeyValues*>::iterator& it)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i, numTokens;
	bool bFoundMatch = false;
	while (it != m_models.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential model data
		// pSearchInfo has the search criteria
		bool bGood = false;
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				if (!Q_strcmp(searchField->GetString(), ""))
					bGood = true;
				else
				{
					potentialBuf = active->GetString("title");
					std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

					searchBuf = searchField->GetString();
					std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

					/*
					searchTokens.clear();
					g_pAnarchyManager->Tokenize(searchBuf, searchTokens, " ");
					numTokens = searchTokens.size();

					for (i = 0; i < numTokens; i++)
					{
					if (searchTokens[i].length() < 2)
					continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
					bFoundMatch = true;
					break;
					}
					}
					*/

					if (potentialBuf.find(searchBuf) != std::string::npos)
						bGood = true;
					else
						bGood = false;
				}
			}
			else if (fieldName == "dynamic")
			{
				if (!Q_strcmp(searchField->GetString(), "") || !Q_strcmp(active->GetString("dynamic"), searchField->GetString()))
					bGood = true;
				else
					bGood = false;
			}

			if (!bGood)
				break;
		}

		if (bGood)
		{
			bFoundMatch = true;
			break;
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryModel(KeyValues* pSearchInfo)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	unsigned int i, numTokens;
	bool bFoundMatch = false;
	std::map<std::string, KeyValues*>::iterator it = m_models.begin();
	while (it != m_models.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential model data
		// pSearchInfo has the search criteria
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				potentialBuf = searchField->GetString();	// FIXME: Does this work? seems to work... but how?
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = pSearchInfo->GetString(fieldName.c_str());
				searchTokens.clear();
				g_pAnarchyManager->Tokenize(searchBuf, searchTokens, ", ");
				numTokens = searchTokens.size();

				for (i = 0; i < numTokens; i++)
				{
					if (searchTokens[i].length() < 2)
						continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
						bFoundMatch = true;
						break;
					}
				}

				if (bFoundMatch)
					break;
			}
			else if (fieldName == "file")
			{
				potentialBuf = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);
				std::replace(potentialBuf.begin(), potentialBuf.end(), '/', '\\'); // replace all '/' to '\'

				searchBuf = searchField->GetString();

				// TODO: Make these changes be required to the value prior to calling find.
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);
				std::replace(searchBuf.begin(), searchBuf.end(), '/', '\\'); // replace all '/' to '\'

				if (potentialBuf == searchBuf)
				{
					//	DevMsg("Found match with %s = %s\n", potentialBuf.c_str(), searchBuf.c_str());
					bFoundMatch = true;
					break;
				}
			}
			else
			{
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
				{
					//	DevMsg("Found match with %s = %s\n", potentialBuf.c_str(), searchBuf.c_str());
					bFoundMatch = true;
					break;
				}
			}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	// Do this here because we MUST handle the deletion for findNext and findFirst, so this usage should match!!
	pSearchInfo->deleteThis();

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryApp(KeyValues* pSearchInfo, std::map<std::string, KeyValues*>::iterator& it)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i, numTokens;
	bool bFoundMatch = false;
	while (it != m_apps.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential app data
		// pSearchInfo has the search criteria
		bool bGood = false;
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				if (!Q_strcmp(searchField->GetString(), ""))
					bGood = true;
				else
				{
					potentialBuf = active->GetString("title");
					std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

					searchBuf = searchField->GetString();
					std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

					/*
					searchTokens.clear();
					g_pAnarchyManager->Tokenize(searchBuf, searchTokens, " ");
					numTokens = searchTokens.size();

					for (i = 0; i < numTokens; i++)
					{
					if (searchTokens[i].length() < 2)
					continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
					bFoundMatch = true;
					break;
					}
					}
					*/

					if (potentialBuf.find(searchBuf) != std::string::npos)
						bGood = true;
					else
						bGood = false;
				}
			}
			else
			{
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
					bGood = true;
				else
					bGood = false;
			}
			/*
			else if (fieldName == "type")
			{
			if (!Q_strcmp(searchField->GetString(), "") || !Q_strcmp(active->GetString("type"), searchField->GetString()))
			bGood = true;
			else
			bGood = false;
			}
			*/

			if (!bGood)
				break;
		}

		if (bGood)
		{
			bFoundMatch = true;
			break;
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryApp(KeyValues* pSearchInfo)
{
	//DevMsg("C_MetaverseManager: FindLibraryApp with ONLY pSearchinfo!!\n");
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	unsigned int numTokens;//i
	bool bFoundMatch = false;
	std::map<std::string, KeyValues*>::iterator it = m_apps.begin();
	while (it != m_apps.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential app data
		// pSearchInfo has the search criteria
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			/*
			if (fieldName == "title")
			{
				potentialBuf = searchField->GetString();	// FIXME: Does this work? seems to work... but how?
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = pSearchInfo->GetString(fieldName.c_str());
				searchTokens.clear();
				g_pAnarchyManager->Tokenize(searchBuf, searchTokens, ", ");
				numTokens = searchTokens.size();

				for (i = 0; i < numTokens; i++)
				{
					if (searchTokens[i].length() < 2)
						continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
						bFoundMatch = true;
						break;
					}
				}

				if (bFoundMatch)
					break;
			}
			else
			{*/
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
				{
					//	DevMsg("Found match with %s = %s\n", potentialBuf.c_str(), searchBuf.c_str());
					bFoundMatch = true;
					break;
				}
			//}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	// Do this here because we MUST handle the deletion for findNext and findFirst, so this usage should match!!
	pSearchInfo->deleteThis();

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryType(KeyValues* pSearchInfo, std::map<std::string, KeyValues*>::iterator& it)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i, numTokens;
	bool bFoundMatch = false;
	while (it != m_types.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);

		// active has the potential type data
		// pSearchInfo has the search criteria
		bool bGood = false;
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				if (!Q_strcmp(searchField->GetString(), ""))
					bGood = true;
				else
				{
					potentialBuf = active->GetString("title");
					std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

					searchBuf = searchField->GetString();
					std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

					/*
					searchTokens.clear();
					g_pAnarchyManager->Tokenize(searchBuf, searchTokens, " ");
					numTokens = searchTokens.size();

					for (i = 0; i < numTokens; i++)
					{
					if (searchTokens[i].length() < 2)
					continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
					bFoundMatch = true;
					break;
					}
					}
					*/

					if (potentialBuf.find(searchBuf) != std::string::npos)
						bGood = true;
					else
						bGood = false;
				}
			}
			else
			{
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
					bGood = true;
				else
					bGood = false;
			}

			if (!bGood)
				break;
		}

		if (bGood)
		{
			bFoundMatch = true;
			break;
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryType(KeyValues* pSearchInfo)
{
	//DevMsg("C_MetaverseManager: FindLibraryType with ONLY pSearchinfo!!\n");
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i;// , numTokens;
	bool bFoundMatch = false;
	std::map<std::string, KeyValues*>::iterator it = m_types.begin();
	while (it != m_types.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential type data
		// pSearchInfo has the search criteria
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			/*
			if (fieldName == "title")
			{
			potentialBuf = searchField->GetString();	// FIXME: Does this work? seems to work... but how?
			std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

			searchBuf = pSearchInfo->GetString(fieldName.c_str());
			searchTokens.clear();
			g_pAnarchyManager->Tokenize(searchBuf, searchTokens, ", ");
			numTokens = searchTokens.size();

			for (i = 0; i < numTokens; i++)
			{
			if (searchTokens[i].length() < 2)
			continue;

			if (potentialBuf.find(searchTokens[i]) != std::string::npos)
			{
			bFoundMatch = true;
			break;
			}
			}

			if (bFoundMatch)
			break;
			}
			else
			{*/
			potentialBuf = active->GetString(fieldName.c_str());
			std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

			searchBuf = searchField->GetString();
			std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

			if (potentialBuf == searchBuf)
			{
				//	DevMsg("Found match with %s = %s\n", potentialBuf.c_str(), searchBuf.c_str());
				bFoundMatch = true;
				break;
			}
			//}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	// Do this here because we MUST handle the deletion for findNext and findFirst, so this usage should match!!
	pSearchInfo->deleteThis();

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindFirstLibraryModel(KeyValues* pSearchInfo)
{
	// remember this search query
	if (!m_pPreviousModelSearchInfo)
		m_pPreviousModelSearchInfo = pSearchInfo;// new KeyValues("search");
	else if (m_pPreviousModelSearchInfo != pSearchInfo)	// this should never be called!!!
	{
		m_pPreviousModelSearchInfo->deleteThis();
		m_pPreviousModelSearchInfo = pSearchInfo;
	}

	m_previousFindModelIterator = m_models.begin();

	// start the search
	KeyValues* response = this->FindLibraryModel(m_pPreviousModelSearchInfo, m_previousFindModelIterator);
	return response;
}

KeyValues* C_MetaverseManager::FindFirstLibraryApp(KeyValues* pSearchInfo)
{
	// remember this search query
	if (!m_pPreviousAppSearchInfo)
		m_pPreviousAppSearchInfo = pSearchInfo;// new KeyValues("search");
	else if (m_pPreviousAppSearchInfo != pSearchInfo)	// this should never be called!!!
	{
		m_pPreviousAppSearchInfo->deleteThis();
		m_pPreviousAppSearchInfo = pSearchInfo;
	}

	m_previousFindAppIterator = m_apps.begin();

	// start the search
	KeyValues* response = this->FindLibraryApp(m_pPreviousAppSearchInfo, m_previousFindAppIterator);
	return response;
}

KeyValues* C_MetaverseManager::FindNextLibraryModel()
{
	// continue the search
	KeyValues* response = null;
	m_previousFindModelIterator++;
	if (m_previousFindModelIterator != m_models.end())
		response = this->FindLibraryModel(m_pPreviousModelSearchInfo, m_previousFindModelIterator);
	return response;
}

KeyValues* C_MetaverseManager::FindNextLibraryApp()
{
	// continue the search
	KeyValues* response = null;
	m_previousFindAppIterator++;
	if (m_previousFindAppIterator != m_apps.end())
		response = this->FindLibraryApp(m_pPreviousAppSearchInfo, m_previousFindAppIterator);
	return response;
}

/*
KeyValues* C_MetaverseManager::FindLibraryModel(KeyValues* pSearchInfo, bool bExactOnly)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	unsigned int i, numTokens;
	bool bFoundMatch;
	std::map<std::string, KeyValues*>::iterator it = m_models.begin();
	char slashBuffer[AA_MAX_STRING];
	while (it != m_models.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = potential->FindKey("current");
		if (!active)
			active = potential->FindKey("local");
		if (active)
		{
			// active has the potential model data
			// pSearchInfo has the search criteria
			for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
			{
				fieldName = searchField->GetName();
				if (fieldName == "lastmodel")
				{
					if (!bExactOnly)
					{
						potentialBuf = searchField->GetString();
						std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

						searchBuf = pSearchInfo->GetString(fieldName.c_str());
						searchTokens.clear();
						g_pAnarchyManager->Tokenize(searchBuf, searchTokens, ", ");
						numTokens = searchTokens.size();

						for (i = 0; i < numTokens; i++)
						{
							if (searchTokens[i].length() < 2)
								continue;

							if (potentialBuf.find(searchTokens[i]) != std::string::npos)
							{
								bFoundMatch = true;
								break;
							}
						}
					}
					else
					{
						// fix slashes to reduce duplicates
						Q_strcpy(slashBuffer, searchField->GetString());
						V_FixSlashes(slashBuffer);
						potentialBuf = slashBuffer;

						Q_strcpy(slashBuffer, pSearchInfo->GetString(fieldName.c_str()));
						V_FixSlashes(slashBuffer);
						searchBuf = slashBuffer;

						if (potentialBuf == searchBuf)
							bFoundMatch = true;
					}

					if (bFoundMatch)
						break;

				}
			}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	// Do this here because we MUST handle the deletion for findNext and findFirst, so this usage should match!!
	pSearchInfo->deleteThis();

	if (bFoundMatch)
		return it->second;
	else
		return null;
}
*/

KeyValues* C_MetaverseManager::FindLibraryEntry(const char* category, KeyValues* pSearchInfo, std::map<std::string, KeyValues*>::iterator& it, bool bTightMatchOnly, std::string tightMatchValue)
{
	//KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i, numTokens;
	bool bFoundMatch = false;

	// Determine if we are dealing with type = nodes
	bool bIsNodeTypeSearch = false;
	KeyValues* pTypeKV = this->GetActiveKeyValues(this->GetLibraryType(pSearchInfo->GetString("type")));
	//DevMsg("Type: %s\n", pSearchInfo->GetString("type"));
	//if (pTypeKV)
	//	DevMsg("Yadda: %s\n", pTypeKV->GetString("title"));
	if (pTypeKV && !Q_strcmp(pTypeKV->GetString("title"), "node"))
	{
		pSearchInfo->SetString("type", this->GetSpecialTypeId("node").c_str());
		bIsNodeTypeSearch = true;
	}

	instance_t* pNodeInstance;
	std::string nodeInstanceId;
	std::string testNodeStyle = pSearchInfo->GetString("nodestyle");

	std::map<std::string, KeyValues*>* pCategoryEntries;// = (!Q_strcmp(category, "items")) ? &m_items : &m_models;
	if (!Q_strcmp(category, "items"))
		pCategoryEntries = &m_items;
	else if (!Q_strcmp(category, "models"))
		pCategoryEntries = &m_models;
	else if (!Q_strcmp(category, "apps"))
		pCategoryEntries = &m_apps;
	else if (!Q_strcmp(category, "types"))
		pCategoryEntries = &m_types;
	else
	{
		DevMsg("Warning: Unknown library category %s referenced.\n", category);
		pCategoryEntries = &m_items;
	}

	std::string typeMatchText;
	KeyValues* pTypeMatchKV;
	KeyValues* pSubKey;
	KeyValues* pSubKeyTest;
	std::string subKeyPath;
	
	while (it != pCategoryEntries->end())
	{
		bFoundMatch = false;
		active = this->GetActiveKeyValues(it->second);
		if (active)
		{
			// active has the potential item data
			// pSearchInfo has the search criteria
			bool bGood = false;
			for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
			{
				fieldName = searchField->GetName();

				subKeyPath = "";
				pSubKey = null;
				pSubKeyTest = searchField->GetFirstSubKey();
				while (pSubKeyTest)
				{
					if (subKeyPath == "")
						subKeyPath = searchField->GetName();

					subKeyPath += std::string("/") + pSubKeyTest->GetName();

					pSubKey = pSubKeyTest;
					pSubKeyTest = pSubKey->GetFirstSubKey();
				}

				if (fieldName == "nodestyle")	// skip this psuedo search field
					continue;
				else if ((!bTightMatchOnly || tightMatchValue != "") && fieldName == "title")
				{
					if (!Q_strcmp(searchField->GetString(), ""))
						bGood = true;
					else
					{
						potentialBuf = active->GetString("title");
						std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

						searchBuf = searchField->GetString();
						std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);	// FIX ME: this is supposed to be done prior to calling us!!!!

						if (potentialBuf == searchBuf)
							bGood = true;
						else
						{
							/*
							searchTokens.clear();
							g_pAnarchyManager->Tokenize(searchBuf, searchTokens, " ");
							numTokens = searchTokens.size();

							for (i = 0; i < numTokens; i++)
							{
							if (searchTokens[i].length() < 2)
							continue;

							if (potentialBuf.find(searchTokens[i]) != std::string::npos)
							{
							bFoundMatch = true;
							break;
							}
							}
							*/

							if (bTightMatchOnly)
							{
								pTypeMatchKV = this->GetActiveKeyValues(this->GetLibraryType(active->GetString("type")));
								if (pTypeMatchKV)
								{
									typeMatchText = pTypeMatchKV->GetString("title");
									std::transform(typeMatchText.begin(), typeMatchText.end(), typeMatchText.begin(), ::tolower);

									if (typeMatchText == tightMatchValue && potentialBuf.find(searchBuf) != std::string::npos)
										bGood = true;
									else
										bGood = false;
								}
								else
									bGood = false;
							}
							else if (!bTightMatchOnly && potentialBuf.find(searchBuf) != std::string::npos)
								bGood = true;
							else
								bGood = false;
						}
					}
				}
				/*
				else if (categoryEntries == m_models && fieldName == "dynamic")
				{
					if (!Q_strcmp(searchField->GetString(), "") || !Q_strcmp(active->GetString("dynamic"), searchField->GetString()))
						bGood = true;
					else
						bGood = false;
				}*/
				else if (!pSubKey)
				{
					searchBuf = searchField->GetString();

					if (pCategoryEntries == &m_models && fieldName == "file")
					{
						potentialBuf = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
						std::replace(potentialBuf.begin(), potentialBuf.end(), '/', '\\');

						//std::replace(searchBuf.begin(), searchBuf.end(), '/', '\\');	// now done prior to calling us. (fixed.)	// FIXME: This really only needs to be done once! (It's doing it EVERY compare right now.)
					}
					else
						potentialBuf = active->GetString(fieldName.c_str());

					//std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);
					//std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);	// done prior to calling us

					//DevMsg("Test %s vs %s\n", potentialBuf.c_str(), searchBuf.c_str());

					if (potentialBuf == searchBuf)
						bGood = true;
					else
						bGood = false;
				}
				else// if( pSubKey)
				{
					searchBuf = pSubKey->GetString();

					//if (pCategoryEntries == &m_models && fieldName == "file")
					//{
					//	potentialBuf = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
					//	std::replace(potentialBuf.begin(), potentialBuf.end(), '/', '\\');
					//}
					//else
					potentialBuf = active->GetString(subKeyPath.c_str());

					std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);
					//std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);	// done prior to calling us

					if (potentialBuf == searchBuf)
						bGood = true;
					else
						bGood = false;
				}

				if (!bGood)
					break;
			}


			// validate node search results
			if (bIsNodeTypeSearch && bGood)
			{
				if (testNodeStyle == "")
				{
					// if we are NOT given a nodestyle, then we are ONLY good if we are of node_walls, node_floors, or node_loft
					nodeInstanceId = active->GetString("file");
					//DevMsg("Node isntance id is: ")
					pNodeInstance = g_pAnarchyManager->GetInstanceManager()->GetInstance(nodeInstanceId);
					if (!pNodeInstance)
						bGood = false;
					else
					{
						//testNodeStyle = pNodeInstance->style;

						//if (!pNodeInstance || pNodeInstance->style != testNodeStyle)
						if (!pNodeInstance || (pNodeInstance->style != "node_smallwall" && pNodeInstance->style != "node_floor3x4" && pNodeInstance->style != "node_loft3x4"))
							bGood = false;
						else
							bGood = true;
					}
				}
				else
				{
					// confirm our match is also the right nodestyle
					nodeInstanceId = active->GetString("file");
					//DevMsg("Node ID: %s\n", nodeInstanceId);
					pNodeInstance = g_pAnarchyManager->GetInstanceManager()->GetInstance(nodeInstanceId);
					//if (pNodeInstance)
					//	DevMsg("Node style: %s\n", pNodeInstance->style.c_str());

					if (!pNodeInstance || pNodeInstance->style != testNodeStyle)
						bGood = false;
					else
					{
						//DevMsg("Found node: %s\n", pNodeInstance->style.c_str());
						bGood = true;
					}
				}
			}


			//if (bGood)
			//{
				if (bGood)
				{
					bFoundMatch = true;
					break;
				}
			//}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	if (bFoundMatch)
	{
		if (it == pCategoryEntries->end())
		{
			DevMsg("WARNING: Bad serach result attempted to return! Aborting.\n");
			return null;
		}
		else
			return it->second;
	}
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryItem(KeyValues* pSearchInfo, std::map<std::string, KeyValues*>::iterator& it)
{
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	//unsigned int i, numTokens;
	bool bFoundMatch = false;
	while (it != m_items.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential item data
		// pSearchInfo has the search criteria
		bool bGood = false;
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				if (!Q_strcmp(searchField->GetString(), ""))
					bGood = true;
				else
				{
					potentialBuf = active->GetString("title");
					std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

					searchBuf = searchField->GetString();
					std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

					/*
					searchTokens.clear();
					g_pAnarchyManager->Tokenize(searchBuf, searchTokens, " ");
					numTokens = searchTokens.size();

					for (i = 0; i < numTokens; i++)
					{
					if (searchTokens[i].length() < 2)
					continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
					bFoundMatch = true;
					break;
					}
					}
					*/

					if (potentialBuf.find(searchBuf) != std::string::npos)
						bGood = true;
					else
						bGood = false;
				}
			}
			else
			{
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
					bGood = true;
				else
					bGood = false;
			}
			/*
			else if (fieldName == "type")
			{
				if (!Q_strcmp(searchField->GetString(), "") || !Q_strcmp(active->GetString("type"), searchField->GetString()))
					bGood = true;
				else
					bGood = false;
			}
			*/

			if (!bGood)
				break;
		}

		if (bGood)
		{
			bFoundMatch = true;
			break;
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::FindLibraryItem(KeyValues* pSearchInfo)
{
	//DevMsg("C_MetaverseManager: FindLibraryItem with ONLY pSearchinfo!!\n");
	KeyValues* potential;
	KeyValues* active;
	KeyValues* searchField;
	std::string fieldName, potentialBuf, searchBuf;
	//char charBuf[AA_MAX_STRING];
	std::vector<std::string> searchTokens;
	unsigned int i, numTokens;
	bool bFoundMatch = false;
	std::map<std::string, KeyValues*>::iterator it = m_items.begin();
	while (it != m_items.end())
	{
		bFoundMatch = false;
		potential = it->second;
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(potential);
		// active has the potential item data
		// pSearchInfo has the search criteria
		for (searchField = pSearchInfo->GetFirstSubKey(); searchField; searchField = searchField->GetNextKey())
		{
			fieldName = searchField->GetName();
			if (fieldName == "title")
			{
				potentialBuf = searchField->GetString();	// FIXME: Does this work? seems to work... but how?
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = pSearchInfo->GetString(fieldName.c_str());
				searchTokens.clear();
				g_pAnarchyManager->Tokenize(searchBuf, searchTokens, ", ");
				numTokens = searchTokens.size();

				for (i = 0; i < numTokens; i++)
				{
					if (searchTokens[i].length() < 2)
						continue;

					if (potentialBuf.find(searchTokens[i]) != std::string::npos)
					{
						bFoundMatch = true;
						break;
					}
				}

				if (bFoundMatch)
					break;
			}
			else
			{
				potentialBuf = active->GetString(fieldName.c_str());
				std::transform(potentialBuf.begin(), potentialBuf.end(), potentialBuf.begin(), ::tolower);

				searchBuf = searchField->GetString();
				std::transform(searchBuf.begin(), searchBuf.end(), searchBuf.begin(), ::tolower);

				if (potentialBuf == searchBuf)
				{
				//	DevMsg("Found match with %s = %s\n", potentialBuf.c_str(), searchBuf.c_str());
					bFoundMatch = true;
					break;
				}
			}
		}

		if (bFoundMatch)
			break;
		else
			it++;
	}

	// Do this here because we MUST handle the deletion for findNext and findFirst, so this usage should match!!
	pSearchInfo->deleteThis();

	if (bFoundMatch)
		return it->second;
	else
		return null;
}

std::string C_MetaverseManager::FindFirstLibraryEntry(KeyValues*& response, const char* category, KeyValues* pSearchInfo)
{
	//std::string categoryBuf = std::string(category);

	//if (categoryBuf == "")
	//	categoryBuf = "items";

	std::string queryId = g_pAnarchyManager->GenerateUniqueId();
	//DevMsg("Query ID point A: %s\n", queryId.c_str());
	//std::string idBuf = std::string(queryId);

	// TODO: use queryId to organize N search queries & store the category info for each.
	KeyValues* pEntry;
	KeyValues* pPreviousSearchInfo = null;
	std::map<std::string, KeyValues*>::iterator* it;
	std::map<std::string, KeyValues*>* categoryEntries;
	if (!Q_strcmp(category, "items"))
	{
		categoryEntries = &m_items;

		if (m_pPreviousSearchInfo)
		{
			//DevMsg("Cleaning up a library query context that was left open...\n");	// FIXME: This entire block should be handled differently as soon as concurrent library queries are supported, but right now we're actually limited to 1 query per category.
			pPreviousSearchInfo->deleteThis();
		}

		m_pPreviousSearchInfo = pSearchInfo;
		it = &m_previousFindItemIterator;
	}
	else if (!Q_strcmp(category, "models")) // if (!Q_strcmp(category, "items"))
	{
		categoryEntries = &m_models;

		if (m_pPreviousModelSearchInfo)
		{
			//DevMsg("Cleaning up a library query context that was left open...\n");	// FIXME: This entire block should be handled differently as soon as concurrent library queries are supported, but right now we're actually limited to 1 query per category.
			m_pPreviousModelSearchInfo->deleteThis();
		}

		m_pPreviousModelSearchInfo = pSearchInfo;
		it = &m_previousFindModelIterator;
	}
	else if (!Q_strcmp(category, "apps"))
	{
		categoryEntries = &m_apps;

		if (m_pPreviousAppSearchInfo)
		{
			//DevMsg("Cleaning up a library query context that was left open...\n");	// FIXME: This entire block should be handled differently as soon as concurrent library queries are supported, but right now we're actually limited to 1 query per category.
			m_pPreviousAppSearchInfo->deleteThis();
		}

		m_pPreviousAppSearchInfo = pSearchInfo;
		it = &m_previousFindAppIterator;
	}
	else
	{
		DevMsg("Warning: Unknown library category %s referenced.\n", category);
		categoryEntries = &m_items;
	}
	
	// start the search
	//m_previousFindItemIterator = categoryEntries->begin();
	*it = categoryEntries->begin();
	pEntry = this->FindLibraryEntry(category, pSearchInfo, *it);

	response = pEntry;
	//std::string idBuf = std::string(queryId);
	return queryId.c_str();
}

KeyValues* C_MetaverseManager::FindNextLibraryEntry(const char* queryId, const char* category)
{
	KeyValues* response = null;
	if (!Q_strcmp(category, "items"))
	{
		// continue the search
		m_previousFindItemIterator++;
		if (m_previousFindItemIterator != m_items.end())
			response = this->FindLibraryEntry(category, m_pPreviousSearchInfo, m_previousFindItemIterator);
	}
	else if (!Q_strcmp(category, "models"))
	{
		// continue the search
		m_previousFindModelIterator++;
		if (m_previousFindModelIterator != m_models.end())
			response = this->FindLibraryEntry(category, m_pPreviousModelSearchInfo, m_previousFindModelIterator);
	}
	else if (!Q_strcmp(category, "apps"))
	{
		// continue the search
		m_previousFindAppIterator++;
		if (m_previousFindAppIterator != m_apps.end())
			response = this->FindLibraryEntry(category, m_pPreviousAppSearchInfo, m_previousFindAppIterator);
	}

	return response;
}

KeyValues* C_MetaverseManager::FindFirstLibraryItem(KeyValues* pSearchInfo)
{
	// remember this search query
	if (!m_pPreviousSearchInfo)
		m_pPreviousSearchInfo = pSearchInfo;// new KeyValues("search");
	else if (m_pPreviousSearchInfo != pSearchInfo)	// this should never be called!!!
	{
		m_pPreviousSearchInfo->deleteThis();
		m_pPreviousSearchInfo = pSearchInfo;
	}

	m_previousFindItemIterator = m_items.begin();

	// start the search
	KeyValues* response = this->FindLibraryItem(m_pPreviousSearchInfo, m_previousFindItemIterator);
	return response;
}

KeyValues* C_MetaverseManager::FindNextLibraryItem()
{
	// continue the search
	KeyValues* response = null;
	m_previousFindItemIterator++;
	if (m_previousFindItemIterator != m_items.end())
		response = this->FindLibraryItem(m_pPreviousSearchInfo, m_previousFindItemIterator);
	return response;
}

KeyValues* C_MetaverseManager::GetScreenshot(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_mapScreenshots.find(id);
	if (it != m_mapScreenshots.end())
		return it->second;
	else
		return null;
}

void C_MetaverseManager::AddScreenshot(KeyValues* pScreenshotKV)
{
	std::string id = pScreenshotKV->GetString("id");
	m_mapScreenshots[id] = pScreenshotKV;
}

void C_MetaverseManager::DeleteScreenshot(std::string id)
{
	std::string filename;
	KeyValues* pScreenshotKV = this->GetScreenshot(id);
	if (pScreenshotKV)
	{
		// 1. Delete [ID].tga and [ID].txt from the shots folder.
		filename = "screenshots\\" + id + ".tga";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		// support when shots was used instead of screenshots
		filename = "shots\\" + id + ".tga";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		filename = "screenshots\\" + id + ".txt";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		// support when shots was used instead of screenshots
		filename = "shots\\" + id + ".txt";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		filename = "screenshots\\" + id + ".jpg";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		filename = "screenshots\\" + id + ".js";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		filename = "screenshots\\" + id + ".html";
		if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

		// is there a subfolder for this screenshot?
		std::string foldername = "screenshots\\" + id;
		if (g_pFullFileSystem->IsDirectory(foldername.c_str(), "DEFAULT_WRITE_PATH"))
		{
			filename = "screenshots\\" + id + "\\index.html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

			filename = "screenshots\\" + id + "\\index.jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

			filename = "screenshots\\" + id + "\\index.js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

			filename = "screenshots\\" + id + "\\readme.txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

			// front
			std::string side = "front";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// back
			side = "back";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// left
			side = "left";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// right
			side = "right";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// bottom
			side = "bottom";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// top
			side = "top";
			filename = "screenshots\\" + id + "\\" + side + ".jpg";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			/*
			filename = "screenshots\\" + id + "\\" + side + ".txt";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\" + side + ".html";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			*/

			// pano folder
			filename = "screenshots\\" + id + "\\pano\\aalogo.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\close.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\eye.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\launchicon.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\OBJLoader.js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\roller.css";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\sceneManager.js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\skipbackicon.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\skipnexticon.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\stopicon.png";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");
			filename = "screenshots\\" + id + "\\pano\\three.min.js";
			if (g_pFullFileSystem->FileExists(filename.c_str(), "DEFAULT_WRITE_PATH"))
				g_pFullFileSystem->RemoveFile(filename.c_str(), "DEFAULT_WRITE_PATH");

			std::string fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
			fullFile += "\\screenshots\\" + id + "\\pano";
			if (!RemoveDirectory(fullFile.c_str()))
				Msg("WARNING: COULD NOT REMOVE FOLDER %s\n", fullFile.c_str());

			fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
			fullFile += "\\screenshots\\";
			fullFile += id;
			if (!RemoveDirectory(fullFile.c_str()))
				Msg("WARNING: COULD NOT REMOVE FOLDER %s\n", fullFile.c_str());
		}

		// 2. Remove the entry from m_mapScreenshots & delete the KeyValues.
		pScreenshotKV->deleteThis();
		m_mapScreenshots.erase(id);
	}
}

KeyValues* C_MetaverseManager::FindMostRecentScreenshot(std::string mapId, instance_t* pInstance)
{
	std::string instanceId = (pInstance) ? pInstance->id : "";

	KeyValues* pMapKV = null;
	std::string mapFileName = "";
	std::string goodMapId = "";
	if (mapId != "")
	{
		goodMapId = mapId;
		pMapKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(this->GetMap(goodMapId));
		if (pMapKV)
		{
			goodMapId = pMapKV->GetString("info/id");
			mapFileName = pMapKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
		}
	}
	else
	{
		mapFileName = std::string(g_pAnarchyManager->MapName()) + std::string(".bsp");
		pMapKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(this->FindMap(mapFileName.c_str()));
		if (pMapKV)
			goodMapId = pMapKV->GetString("info/id");
	}

	if (goodMapId == "" || mapFileName == "" )
		return null;

	//std::string testId;
	std::vector<KeyValues*> screenshots;
	std::map<std::string, KeyValues*>::iterator it = m_mapScreenshots.begin();
	while (it != m_mapScreenshots.end())
	{
		//testId = it->second->GetString("map/id");	// SOMEHOW COULD BE 1.#INF00 when printed as a string, even though it was fine in the KV text. Like for ID 545e1841
		//testId = (instanceId == "") ? it->second->GetString("instance/mapId") : it->second->GetString("instance/id");
		//DevMsg("IDs: %s vs %s\n", testId.c_str(), goodMapId.c_str());
		//if ((instanceId == "" && testId == goodMapId) || (instanceId != "" && testId == instanceId))
		if (!Q_strcmp(it->second->GetString("map/file"), mapFileName.c_str()))
			screenshots.push_back(it->second);

		it++;
	}

	// now screenshots holds all applicable screenshots.  find the most recent one.
	KeyValues* pBestScreenshot = null;
	uint64 bestValue = 0;
	uint64 testValue;
	unsigned int max = screenshots.size();
	for (unsigned int i = 0; i < max; i++)
	{
		testValue = Q_atoui64(screenshots[i]->GetString("created"));
		if (testValue > bestValue)
		{
			bestValue = testValue;
			pBestScreenshot = screenshots[i];
		}
	}

	return pBestScreenshot;
}

KeyValues* C_MetaverseManager::GetLibraryApp(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_apps.find(id);
	if (it != m_apps.end())
		return it->second;
	else
		return null;
}

KeyValues* C_MetaverseManager::GetLibraryMap(std::string id)
{
	std::map<std::string, KeyValues*>::iterator it = m_maps.find(id);
	if (it != m_maps.end())
		return it->second;
	else
		return null;
}

std::map<std::string, KeyValues*>& C_MetaverseManager::DetectAllMapScreenshots()
{
	DevMsg("Detecting all map screenshots...\n");
	//size_t found;
	std::string id;
	//std::map<std::string, std::string>::iterator it;

	// "id" prefixed fields: instance/id, instance/mapId, instance/workshopIds, instance/mountIds, map/id, map/workshopIds, map/mountIds
	std::string idPrefixBuf;
	std::vector<std::string> idPrefixFields;
	idPrefixFields.push_back("instance/id");
	idPrefixFields.push_back("instance/mapId");
	idPrefixFields.push_back("instance/workshopIds");
	idPrefixFields.push_back("instance/mountIds");
	idPrefixFields.push_back("map/id");
	idPrefixFields.push_back("map/workshopIds");
	idPrefixFields.push_back("map/mountIds");

	std::vector<std::string> screenshotFolders;	// support for when the shots folder was used instead of the screenshots folder
	screenshotFolders.push_back("screenshots");
	screenshotFolders.push_back("shots");

	unsigned int i;
	unsigned int max = idPrefixFields.size();
	KeyValues* pShotKV;
	FileFindHandle_t pFileFindHandle;

	for (unsigned int j = 0; j < screenshotFolders.size(); j++)
	{
		// get every shot that exists
		const char *pScreenshotInfoFile = g_pFullFileSystem->FindFirstEx(VarArgs("%s\\*.txt", screenshotFolders[j].c_str()), "MOD", &pFileFindHandle);
		while (pScreenshotInfoFile != NULL)
		{
			if (g_pFullFileSystem->FindIsDirectory(pFileFindHandle))
			{
				pScreenshotInfoFile = g_pFullFileSystem->FindNext(pFileFindHandle);
				continue;
			}

			id = pScreenshotInfoFile;
			id = id.substr(0, id.find(".txt"));

			pShotKV = new KeyValues("screenshot");
			if (pShotKV->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\%s", screenshotFolders[j].c_str(), pScreenshotInfoFile), "MOD"))
			{
				// IMPORTANT NOTE: Key values loaded with LoadFromFile forget the data types for their fields and if a string is just numbers, it gets turned into a number instead of a string after loading.
				// This matters if the number started with a 0, because leading zeros get removed for numbers.
				// So to catch this, additional checks must be performed on ID's read from KeyValues files.
				//DevMsg("Here the map ID for the screenshot is: %s or %i or %s and type %s\n", pShotKV->GetString("map/id"), pShotKV->GetInt("map/id"), pShotKV->GetRawString("map/id"), pShotKV->GetDataType("map/id"));

				// "id" prefixed fields: instance/id, instance/mapId, instance/workshopIds, instance/mountIds, map/id, map/workshopIds, map/mountIds
				for (i = 0; i < max; i++)
				{
					idPrefixBuf = pShotKV->GetString(idPrefixFields[i].c_str());
					if (idPrefixBuf != "" && idPrefixBuf.find("id") != 0)
						pShotKV->SetString(idPrefixFields[i].c_str(), VarArgs("id%s", idPrefixBuf.c_str()));
				}

				// now detect which files are available for this screenshot
				// NOTE: This code assumes screenshotFolders[0] = "screenshots"!!!!
				bool bHasBigFile = g_pFullFileSystem->FileExists(VarArgs("%s\\%s.jpg", screenshotFolders[0].c_str(), id.c_str()), "MOD");	// these only existed after the switch to screenshots folder.
				int iHasBigFile = (bHasBigFile) ? 1 : 0;
				pShotKV->SetInt("hasBigFile", iHasBigFile);

				// NOTE: This code assumes screenshotFolders[0] = "screenshots"!!!!
				bool bHasJSFile = g_pFullFileSystem->FileExists(VarArgs("%s\\%s.js", screenshotFolders[0].c_str(), id.c_str()), "MOD");	// these only existed after the switch to screenshots folder.
				int iHasJSFile = (bHasJSFile) ? 1 : 0;
				pShotKV->SetInt("hasJSFile", iHasJSFile);

				// this also has support for when the shots folder was used instead of screenshots folder
				bool bHasThumbFile = g_pFullFileSystem->FileExists(VarArgs("%s\\%s.tga", screenshotFolders[0].c_str(), id.c_str()), "MOD");	// NOTE: This code assumes j's max value is 1!!!!
				if (!bHasThumbFile)
					bHasThumbFile = g_pFullFileSystem->FileExists(VarArgs("%s\\%s.tga", screenshotFolders[1].c_str(), id.c_str()), "MOD");
				int iHasThumbFile = (bHasThumbFile) ? 1 : 0;
				pShotKV->SetInt("hasThumbFile", iHasThumbFile);

				m_mapScreenshots[id] = pShotKV;
			}
			else
				pShotKV->deleteThis();

			pScreenshotInfoFile = g_pFullFileSystem->FindNext(pFileFindHandle);
		}
		g_pFullFileSystem->FindClose(pFileFindHandle);
	}

	return m_mapScreenshots;
}

void C_MetaverseManager::GetAllScreenshotsForInstance(std::vector<KeyValues*>& responseVector, std::string instanceId)
{
	std::vector<KeyValues*> potentialScreenshots;

	std::map<std::string, KeyValues*>::iterator it = m_mapScreenshots.begin();
	while (it != m_mapScreenshots.end())
	{
		if (instanceId == "" || !Q_stricmp(it->second->GetString("instance/id"), instanceId.c_str()))// g_pAnarchyManager->CompareLoadedFromKeyValuesFileId(it->second->GetString("instance/id"), instanceId.c_str())))
		{
			potentialScreenshots.push_back(it->second);
		}

		it++;
	}

	// Now that all of the potentialScreenshots are know, let's sort them in the order we want.
	// That order is chronological.
	int iBestScreenshot;
	int iBestTime;
	int iTestTime;
	while (!potentialScreenshots.empty())
	{
		iBestTime = -1;
		iBestScreenshot = -1;

		for (unsigned int i = 0; i < potentialScreenshots.size(); i++)
		{
			iTestTime = potentialScreenshots[i]->GetInt("created");
			if (iBestTime == -1 || iTestTime < iBestTime)
			{
				iBestTime = iTestTime;
				iBestScreenshot = i;
			}
		}

		if (iBestScreenshot >= 0)
		{
			responseVector.push_back(potentialScreenshots[iBestScreenshot]);
			potentialScreenshots.erase(potentialScreenshots.begin() + iBestScreenshot);
		}
		else
			break;
	}
}

void C_MetaverseManager::GetAllMapScreenshots(std::vector<KeyValues*>& responseVector, std::string mapId, std::string searchTerm, bool bIncludeInloadable)
{
	std::string title;
	std::vector<std::string> tokens;
	std::string goodSearchTerm = searchTerm;
	std::transform(goodSearchTerm.begin(), goodSearchTerm.end(), goodSearchTerm.begin(), ::tolower);

	std::string testInstanceId;

	std::map<std::string, KeyValues*>::iterator it = m_mapScreenshots.begin();
	while (it != m_mapScreenshots.end())
	{
		//DevMsg("Screenshot check: %s vs %s vs %i\n", mapId.c_str(), it->second->GetString("map/id"), it->second->GetInt("map/id"));
		//if (mapId == "" || mapId == it->second->GetString("map/id"))//; it->first)
		testInstanceId = it->second->GetString("instance/id");
		if (testInstanceId.length() > 2)
			testInstanceId = testInstanceId.substr(2);

		if ((mapId == "" || g_pAnarchyManager->CompareLoadedFromKeyValuesFileId(it->second->GetString("map/id"), mapId.c_str())) && g_pAnarchyManager->GetInstanceManager()->GetInstance(testInstanceId))
		{
			if(searchTerm == "")
				responseVector.push_back(it->second);
			else
			{

				title = it->second->GetString("map/title");
				std::transform(title.begin(), title.end(), title.begin(), ::tolower);

				tokens.clear();
				g_pAnarchyManager->Tokenize(title, tokens, "");

				for (unsigned int i = 0; i < tokens.size(); i++)
				{
					if (tokens[i].find(goodSearchTerm) != std::string::npos)
					{
						responseVector.push_back(it->second);
						break;
					}
				}
			}
		}

		it++;
	}
}

KeyValues* C_MetaverseManager::FindMap(const char* mapFile)
{
	KeyValues* map;
	std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
	while (it != m_maps.end())
	{
		map = this->GetActiveKeyValues(it->second);
		//DevMsg("Map name is: %s\n", map->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"));
		if (!Q_stricmp(map->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), mapFile))
			return it->second;

		it++;
	}

	return null;
}

KeyValues* C_MetaverseManager::GetMap(std::string mapId)
{
	std::map<std::string, KeyValues*>::iterator it = m_maps.find(mapId);
	if (it != m_maps.end())
		return it->second;
	return null;
}

void C_MetaverseManager::DetectAllLegacyCabinets()
{
	std::string goodTitle;
	std::string modelId;
	KeyValues* pModel;
	KeyValues* cat = new KeyValues("cat");
	FileFindHandle_t handle;
	const char *pFilename = g_pFullFileSystem->FindFirstEx("resource\\models\\*.cat", "GAME", &handle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			pFilename = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		// load up the cat
		cat->Clear();
		if (cat->LoadFromFile(g_pFullFileSystem, VarArgs("resource\\models\\%s", pFilename), "GAME"))
		{
			modelId = g_pAnarchyManager->GenerateLegacyHash(cat->GetString("model"));

			std::map<std::string, KeyValues*>::iterator oldIt = m_models.find(modelId);
			if (oldIt == m_models.end())
			{
				pModel = new KeyValues("model");

				// update us to 3rd generation
				pModel->SetInt("generation", 3);

				// add standard info (except for id)
				pModel->SetInt("local/info/created", 0);
				pModel->SetString("local/info/owner", "local");
				pModel->SetInt("local/info/removed", 0);
				//pModel->SetString("local/info/remover", "");
				//pModel->SetString("local/info/alias", "");


				pModel->SetString("local/info/id", modelId.c_str());

				pModel->SetString("local/title", cat->GetString("name"));
				pModel->SetString("local/keywords", cat->GetString("category"));
				pModel->SetInt("local/dynamic", 1);
				pModel->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
				pModel->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), cat->GetString("model"));
				//pModel->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), "");

				//pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), "");
				//pModel->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), "");

				// models can be loaded right away because they don't depend on anything else, like items do. (items depend on models)
				DevMsg("Loading cabinet model with ID %s and model %s\n", modelId.c_str(), cat->GetString("model"));
				m_models[modelId] = pModel;
			}
		}

		pFilename = g_pFullFileSystem->FindNext(handle);
	}

	g_pFullFileSystem->FindClose(handle);
	cat->deleteThis();
}

void C_MetaverseManager::DetectAllMaps()
{
	//DevMsg("C_MetaverseManager::DetectAllMaps\n");	// fix me down there at the todo!!

	// Load all the .key files from the library/instances folder.
	// then add their instances:

	/* done elsewhere now
	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(m_db, "SELECT * from instances", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(m_db));

	// TODO: Fix these things to have real values!!  This is very much like the logic done ELSEWHERE!! FIND IT!!

	int length;
	int iGeneration;
	int iLegacy;
	std::string instanceId;
	std::string mapId;
	std::string title;
	std::string file;
	std::string workshopIds;
	std::string mountIds;
	//std::string backpackId;
	std::string style;
	KeyValues* pInstanceKV;
	KeyValues* pInstanceInfoKV;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		// FIXME: TODO: Detect if the map is from a workshop or mount id too!
		pInstanceKV = new KeyValues("instance");
		if (this->LoadSQLKevValues("instances", (const char*)sqlite3_column_text(stmt, 0), pInstanceKV))
		{
			iGeneration = pInstanceKV->GetInt("generation", 3);
			iLegacy = pInstanceKV->GetInt("legacy");

			pInstanceInfoKV = pInstanceKV->FindKey("info/local", true);
			instanceId = pInstanceInfoKV->GetString("id");

			if (instanceId == "")
			{
				DevMsg("Warning: Skipping instance with no ID.\n");
				continue;
			}

			mapId = pInstanceInfoKV->GetString("map");
			title = pInstanceInfoKV->GetString("title");
			if (title == "")
				title = "Unnamed (" + instanceId + ")";
			file = "";
			workshopIds = pInstanceInfoKV->GetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID));
			mountIds = pInstanceInfoKV->GetString(VarArgs("platforms/%s/mountIds", AA_PLATFORM_ID));
			//backpackId = pInstanceInfoKV->GetString(VarArgs("platforms/%s/backpackIds", AA_PLATFORM_ID));
			style = pInstanceInfoKV->GetString("style");

			g_pAnarchyManager->GetInstanceManager()->AddInstance(iLegacy, instanceId, mapId, title, file, workshopIds, mountIds, style);// (kv->GetInt("legacy"), instanceId, mapId, title, file, workshopIds, mountIds, style);//(instanceId, kv->GetString("info/map"), goodTitle, goodLegacyFile);
		}
		pInstanceKV->deleteThis();
		pInstanceKV = null;
	}
	sqlite3_finalize(stmt);
	*/

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	bool bAlreadyExists;

	//DevMsg("Detect first map...\n");
	KeyValues* map = this->DetectFirstMap(bAlreadyExists);
	if (map)
	{
		if (bAlreadyExists)
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Detecting Maps", "detectmaps", "", "", "+0", "detectNextMapCallback");
		else
			pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Detecting Maps", "detectmaps", "", "", "+", "detectNextMapCallback");
	}
	else
		this->OnDetectAllMapsCompleted();

	//DevMsg("Initial logic for DetectAllMaps complete.\n");
}

KeyValues* C_MetaverseManager::CreateModelFromFileTarget(std::string modelFile)
{
	KeyValues* pModel = new KeyValues("model");
	pModel->SetInt("generation", 3);
	// add standard info (except for id)
	pModel->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
	pModel->SetString("local/info/owner", "local");
	//pModel->SetInt("local/info/removed", 0);
	//pModel->SetString("local/info/remover", "");
	//pModel->SetString("local/info/alias", "");

	std::string modelId = g_pAnarchyManager->GenerateLegacyHash(modelFile.c_str());// g_pAnarchyManager->GenerateUniqueId();// > GenerateLegacyHash(itemFile.c_str());
	pModel->SetString("local/info/id", modelId.c_str());

	//DevMsg("Created model (%s) for: %s\n", modelId.c_str(), modelFile.c_str());

	std::string goodTitle = modelFile;
	goodTitle = goodTitle.substr(0, goodTitle.length() - 4);

	size_t foundLastSlash = goodTitle.find_last_of("/\\");
	if (foundLastSlash != std::string::npos)
		goodTitle = goodTitle.substr(foundLastSlash + 1);

	pModel->SetString("local/title", goodTitle.c_str());
	//pModel->SetString("local/preview", );
	//pModel->SetString("local/keywords", );

	//modelFile = VarArgs("%s\\%s", folder, potentialFile);
	if ((modelFile.find("models\\cabinets\\") == 0 || modelFile.find("models/cabinets/") == 0 || modelFile.find("models\\banners\\") == 0 || modelFile.find("models/banners/") == 0 || modelFile.find("models\\frames\\") == 0 || modelFile.find("models/frames/") == 0 || modelFile.find("models\\icons\\") == 0 || modelFile.find("models/icons/") == 0) && (modelFile.find("room_divider.mdl") == std::string::npos && modelFile.find("newton_toy.mdl") == std::string::npos))
		pModel->SetInt("local/dynamic", 1);
	else
		pModel->SetInt("local/dynamic", 0);

	pModel->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
	pModel->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), modelFile.c_str());
	//pModel->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), );
	//pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), );


	KeyValues* modelInfo = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForModelFile(modelFile);
	/*
	KeyValues* modelInfo = new KeyValues("modelInfo");
	modelInfo->SetString("fullfile", modelBuf.c_str());
	modelInfo->SetString("workshopIds", workshopId.c_str());
	modelInfo->SetString("mountIds", mountId.c_str());
	modelInfo->SetString("mountTitle", mountTitle.c_str());
	modelInfo->SetBool("bIsWorkshop", bIsWorkshop);
	modelInfo->SetBool("bIsLegacyImported", bIsLegacyImported);
	return modelInfo;
	*/

	std::string buf = modelInfo->GetString("mountIds");
	if (buf != "")
		pModel->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), buf.c_str());

	buf = modelInfo->GetString("workshopIds");
	if (buf != "")
		pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), buf.c_str());

	// backpack stuff
	buf = g_pAnarchyManager->GetBackpackManager()->DetectRequiredBackpackForModelFile(modelFile);
	if (buf != "")
		pModel->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), buf.c_str());

	modelInfo->deleteThis();
	return pModel;
}

bool C_MetaverseManager::ProcessModel(std::string modelFile)
{
	KeyValues* pModel = this->CreateModelFromFileTarget(modelFile);
	if (pModel)
	{
		g_pAnarchyManager->GetMetaverseManager()->AddModel(pModel);
		//DevMsg("Debug Mode (%s): %s\n", pModel->GetString("local/info/id"), modelFile.c_str());
		//g_pAnarchyManager->GetMetaverseManager()->SaveModel(pModel);
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "models", pModel->GetString("local/info/id"), pModel);
		return true;
	}

	return false;
}

unsigned int C_MetaverseManager::ProcessModels(importInfo_t* pImportInfo)
{
	KeyValues* pSearchInfoKV = new KeyValues("search");
	std::string modelFile;
	std::string standardizedModelFile;
	KeyValues* pEntry;
	KeyValues* pModel;
	KeyValues* modelInfo;
	size_t foundLastSlash;
	std::string buf;
	unsigned int uGoodCount = 0;
	unsigned int uMax = pImportInfo->data.size();
	for (unsigned int i = 0; i < uMax; i++)
	{
		modelFile = pImportInfo->data[i];

		// Check if this MDL already exists...
		standardizedModelFile = modelFile;
		std::replace(standardizedModelFile.begin(), standardizedModelFile.end(), '/', '\\');
		std::transform(standardizedModelFile.begin(), standardizedModelFile.end(), standardizedModelFile.begin(), ::tolower);

		pSearchInfoKV->Clear();
		pSearchInfoKV->SetString("file", standardizedModelFile.c_str());	// platform/AA_SOURCE_PLATFORM_ID/file is automatically used in the search function for models.

		// find the first entry that matches the search params
		auto it = m_models.begin();
		pEntry = g_pAnarchyManager->GetMetaverseManager()->FindLibraryEntry("models", pSearchInfoKV, it);// m_models.begin());
		if (!pEntry)
		{
			pModel = new KeyValues("model");
			pModel->SetInt("generation", 3);
			// add standard info (except for id)
			pModel->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
			pModel->SetString("local/info/owner", "local");
			//pModel->SetInt("local/info/removed", 0);
			//pModel->SetString("local/info/remover", "");
			//pModel->SetString("local/info/alias", "");

			std::string modelId = g_pAnarchyManager->GenerateLegacyHash(modelFile.c_str());	//GenerateUniqueId();// > GenerateLegacyHash(itemFile.c_str());
			pModel->SetString("local/info/id", modelId.c_str());

			std::string goodTitle = modelFile;
			goodTitle = goodTitle.substr(0, goodTitle.length() - 4);

			foundLastSlash = goodTitle.find_last_of("/\\");
			if (foundLastSlash != std::string::npos)
				goodTitle = goodTitle.substr(foundLastSlash + 1);

			pModel->SetString("local/title", goodTitle.c_str());
			//pModel->SetString("local/preview", );
			//pModel->SetString("local/keywords", );

			//modelFile = VarArgs("%s\\%s", folder, potentialFile);
			if ((modelFile.find("models\\cabinets\\") == 0 || modelFile.find("models/cabinets/") == 0 || modelFile.find("models\\banners\\") == 0 || modelFile.find("models/banners/") == 0 || modelFile.find("models\\frames\\") == 0 || modelFile.find("models/frames/") == 0 || modelFile.find("models\\icons\\") == 0 || modelFile.find("models/icons/") == 0) && (modelFile.find("room_divider.mdl") == std::string::npos && modelFile.find("newton_toy.mdl") == std::string::npos))
				pModel->SetInt("local/dynamic", 1);
			else
				pModel->SetInt("local/dynamic", 0);

			pModel->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
			pModel->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), modelFile.c_str());
			//pModel->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), );
			//pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), );


			modelInfo = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForModelFile(modelFile);
			/*
			KeyValues* modelInfo = new KeyValues("modelInfo");
			modelInfo->SetString("fullfile", modelBuf.c_str());
			modelInfo->SetString("workshopIds", workshopId.c_str());
			modelInfo->SetString("mountIds", mountId.c_str());
			modelInfo->SetString("mountTitle", mountTitle.c_str());
			modelInfo->SetBool("bIsWorkshop", bIsWorkshop);
			modelInfo->SetBool("bIsLegacyImported", bIsLegacyImported);
			return modelInfo;
			*/

			buf = modelInfo->GetString("mountIds");
			if (buf != "")
				pModel->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), buf.c_str());

			buf = modelInfo->GetString("workshopIds");
			if (buf != "")
				pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), buf.c_str());

			// backpack stuff
			buf = g_pAnarchyManager->GetBackpackManager()->DetectRequiredBackpackForModelFile(modelFile);
			if (buf != "")
				pModel->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), buf.c_str());

			modelInfo->deleteThis();

			m_models[modelId] = pModel;
			this->SaveSQL(null, "models", modelId.c_str(), pModel);
			uGoodCount++;
		}
	}

	pSearchInfoKV->deleteThis();
	return uGoodCount;
}

void C_MetaverseManager::DetectAllModels()
{
	DevMsg("C_MetaverseManager::DetectAllModels\n");

	//FileFindHandle_t previousDetectedLocalModelFileHandle;
	//std::string pathname = "models";
	//const char* pFilename = g_pFullFileSystem->FindFirstEx("models\\*", "GAME", &previousDetectedLocalModelFileHandle);
	//this->DetectAllModelsRecursive(pathname, previousDetectedLocalModelFileHandle, pFilename);
	unsigned int count = this->DetectAllModelsRecursive("models");
	DevMsg("Detected %u models.\n", count);
}


unsigned int C_MetaverseManager::DetectAllModelsRecursive(const char* folder)//std::string pathname, FileFindHandle_t hFile, const char* pFilename)
{
	//DevMsg("Folder is: %s\n", folder);
	KeyValues* pSearchInfoKV = new KeyValues("search");
	unsigned int count = 0;
	FileFindHandle_t hFileSearch;
	std::string composedFile;
	std::string modelFile;
	std::string standardizedModelFile;
	KeyValues* pEntry;
	KeyValues* pModel;
	KeyValues* modelInfo;
	//size_t foundLastSlash;
	std::string buf;
	const char* potentialFile = g_pFullFileSystem->FindFirstEx(VarArgs("%s\\*", folder), "GAME", &hFileSearch);
	while (potentialFile)
	{
		if (!Q_strcmp(potentialFile, ".") || !Q_strcmp(potentialFile, ".."))
		{
			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}

		composedFile = std::string(folder) + "\\" + std::string(potentialFile);

		if (g_pFullFileSystem->FindIsDirectory(hFileSearch))
		{
			//DevMsg("Checking folder: %s\n", composedFile.c_str());
			count += this->DetectAllModelsRecursive(composedFile.c_str());
			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}
		else
		{
			if (V_GetFileExtension(potentialFile) && !Q_stricmp(V_GetFileExtension(potentialFile), "mdl"))
			{
				//DevMsg("Dump: %s\n", composedFile.c_str());
				modelFile = composedFile;// std::string(folder) + "\\" + std::string(potentialFile);// VarArgs("%s\\%s", folder, potentialFile);

				// Check if this MDL already exists...
				standardizedModelFile = modelFile;
				std::replace(standardizedModelFile.begin(), standardizedModelFile.end(), '/', '\\');
				std::transform(standardizedModelFile.begin(), standardizedModelFile.end(), standardizedModelFile.begin(), ::tolower);

				pSearchInfoKV->Clear();
				pSearchInfoKV->SetString("file", standardizedModelFile.c_str());	// platform/AA_SOURCE_PLATFORM_ID/file is automatically used in the search function for models.

				// find the first entry that matches the search params
				auto it = m_models.begin();
				pEntry = g_pAnarchyManager->GetMetaverseManager()->FindLibraryEntry("models", pSearchInfoKV, it);// m_models.begin());
				if (!pEntry)
				{
					pModel = new KeyValues("model");
					pModel->SetInt("generation", 3);
					// add standard info (except for id)
					pModel->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
					pModel->SetString("local/info/owner", "local");
					//pModel->SetInt("local/info/removed", 0);
					//pModel->SetString("local/info/remover", "");
					//pModel->SetString("local/info/alias", "");

					std::string modelId = g_pAnarchyManager->GenerateLegacyHash(modelFile.c_str()); //GenerateUniqueId();
					pModel->SetString("local/info/id", modelId.c_str());

					std::string goodTitle = potentialFile;
					goodTitle = goodTitle.substr(0, goodTitle.length() - 4);

					//foundLastSlash = goodTitle.find_last_of("/\\");
					//if (foundLastSlash != std::string::npos)
					//	goodTitle = goodTitle.substr(foundLastSlash + 1);

					pModel->SetString("local/title", goodTitle.c_str());
					//pModel->SetString("local/preview", );
					//pModel->SetString("local/keywords", );

					//modelFile = VarArgs("%s\\%s", folder, potentialFile);
					if ((modelFile.find("models\\cabinets\\") == 0 || modelFile.find("models/cabinets/") == 0 || modelFile.find("models\\banners\\") == 0 || modelFile.find("models/banners/") == 0 || modelFile.find("models\\frames\\") == 0 || modelFile.find("models/frames/") == 0 || modelFile.find("models\\icons\\") == 0 || modelFile.find("models/icons/") == 0) && (modelFile.find("room_divider.mdl") == std::string::npos && modelFile.find("newton_toy.mdl") == std::string::npos))
						pModel->SetInt("local/dynamic", 1);
					else
						pModel->SetInt("local/dynamic", 0);

					pModel->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
					pModel->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), modelFile.c_str());
					//pModel->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), );
					//pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), );


					modelInfo = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForModelFile(modelFile);
					/*
					KeyValues* modelInfo = new KeyValues("modelInfo");
					modelInfo->SetString("fullfile", modelBuf.c_str());
					modelInfo->SetString("workshopIds", workshopId.c_str());
					modelInfo->SetString("mountIds", mountId.c_str());
					modelInfo->SetString("mountTitle", mountTitle.c_str());
					modelInfo->SetBool("bIsWorkshop", bIsWorkshop);
					modelInfo->SetBool("bIsLegacyImported", bIsLegacyImported);
					return modelInfo;
					*/

					buf = modelInfo->GetString("mountIds");
					if (buf != "")
						pModel->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), buf.c_str());

					buf = modelInfo->GetString("workshopIds");
					if (buf != "")
						pModel->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), buf.c_str());
					
					// backpack stuff
					buf = g_pAnarchyManager->GetBackpackManager()->DetectRequiredBackpackForModelFile(modelFile);
					if (buf != "")
						pModel->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), buf.c_str());

					modelInfo->deleteThis();

					m_models[modelId] = pModel;
					this->SaveSQL(null, "models", modelId.c_str(), pModel);
					//DevMsg("Added model: %s\n", modelFile.c_str());
					count++;
				}
			}

			potentialFile = g_pFullFileSystem->FindNext(hFileSearch);
			continue;
		}
	}

	g_pFullFileSystem->FindClose(hFileSearch);
	pSearchInfoKV->deleteThis();
	return count;
}

/*
void C_MetaverseManager::DetectAllModelsRecursive(std::string pathname, FileFindHandle_t hFile, const char* pFilename)
{
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(hFile))
		{
			std::string nextpathname = pathname + "\\" + std::string(pFilename);

			this->DetectAllModelsRecursive(nextpathname, hFile, pFilename);
			pFilename = g_pFullFileSystem->FindNext(hFile);
			continue;	// automatically skip to the next one, until a non-folder is hit.
		}

		std::string foundName = pathname + std::string(pFilename);

		KeyValues* active;
		std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
		while (it != m_maps.end())
		{
			active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);

			if (!Q_strcmp(active->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), foundName.c_str()))
			{
				//g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
				bAlreadyExists = true;
				return it->second;
			}

			it++;
		}

		// if we haven't found a map for this yet, let's make one.
		KeyValues* map = new KeyValues("map");
		map->SetInt("generation", 3);

		// add standard info
		//std::string id = g_pAnarchyManager->GenerateUniqueId();

		std::string goodTitle = pFilename;
		size_t found = goodTitle.find(".");
		if (found != std::string::npos)
			goodTitle = goodTitle.substr(0, found);

		std::string id = g_pAnarchyManager->GenerateLegacyHash(goodTitle.c_str());

		map->SetString("local/info/id", id.c_str());
		map->SetInt("local/info/created", 0);
		map->SetString("local/info/owner", "local");
		map->SetInt("local/info/removed", 0);
		map->SetString("local/info/remover", "");
		map->SetString("local/info/alias", "");

		std::string mapName = foundName.substr(0, foundName.length() - 4);
		map->SetString("local/title", mapName.c_str());
		map->SetString("local/keywords", "");
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/file", foundName.c_str());

		KeyValues* stuffKV = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForMapFile(foundName.c_str());
		if (stuffKV)
		{
			map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/workshopIds", stuffKV->GetString("workshopIds"));
			map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/mountIds", stuffKV->GetString("mountIds"));
			stuffKV->deleteThis();
		}

		m_maps[id.c_str()] = map;
		bAlreadyExists = false;

		// update any legacy instances that were detected that use this map
		std::string legacyMapName = foundName.substr(0, foundName.length() - 4);
		DevMsg("LEGACY MAP NAME: %s\n", legacyMapName.c_str());
		g_pAnarchyManager->GetInstanceManager()->LegacyMapIdFix(legacyMapName, id);
		//		if (pInstance)
		//		pInstance->mapId = id;

		return map;
	}

	g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
	return null;
}
*/

void C_MetaverseManager::OnDetectAllMapsCompleted()
{
	DevMsg("Done detecting maps!\n");

	if (m_detectedMapFiles.size() > 0)
	{
		KeyValues* pCachedMapFiles = new KeyValues("maps");
		for (unsigned int i = 0; i < m_detectedMapFiles.size(); i++)
			pCachedMapFiles->CreateNewKey()->SetString("", m_detectedMapFiles[i].c_str());

		pCachedMapFiles->SaveToFile(g_pFullFileSystem, "map_cache.txt", "DEFAULT_WRITE_PATH");
		pCachedMapFiles->deleteThis();
		//m_detectedMapFiles.clear();
	}

	g_pAnarchyManager->OnDetectAllMapsComplete();
}

KeyValues* C_MetaverseManager::GetActiveKeyValues(KeyValues* entry)
{
	// return null if given a null argument to avoid the if-then cluster fuck of error checking each step of this common task
	if (!entry)
		return null;

	KeyValues* active = entry->FindKey("current");
	if (!active)
		active = entry->FindKey("local", true);

	return active;
}

void C_MetaverseManager::SetLibraryBrowserContext(std::string category, std::string id, std::string search, std::string filter)
{
	m_libraryBrowserContextCategory = category;
	m_libraryBrowserContextId = id;
	m_libraryBrowserContextSearch = search;
	m_libraryBrowserContextFilter = filter;
}

std::string C_MetaverseManager::GetLibraryBrowserContext(std::string name)
{
	if (name == "category")
		return m_libraryBrowserContextCategory;
	else if (name == "id")
		return m_libraryBrowserContextId;
	else if (name == "filter")
		return m_libraryBrowserContextFilter;
	else if (name == "search")
		return m_libraryBrowserContextSearch;

	DevMsg("Unhandled Error: Invalid context variable name.\n");
	return "contextError";
}

void C_MetaverseManager::ScaleObject(C_PropShortcutEntity* pEntity, int iDelta)
{
	float goodScale = pEntity->GetModelScale() + 0.1f * iDelta;

//	C_PropShortcutEntity* pEntity = this->GetSpawningObjectEntity();
	engine->ClientCmd(VarArgs("setscale %i %f", pEntity->entindex(), goodScale));	// servercmdfix , false);
	/*
	object_t* object = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pEntity->GetObjectId());
	if (object)
	{
		object->scale += 0.1f * iDelta;	// NOTE: Changes are made to the object here but aren't saved yet!! (is this ok?)
		C_PropShortcutEntity* pEntity = this->GetSpawningObjectEntity();
		engine->ServerCmd(VarArgs("setscale %i %f", pEntity->entindex(), object->scale), false);
	}
	*/

	// if this item is a 3d text item, it needs to resize its children.
	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pEntity->GetObjectId());
	if (pObject) {
		std::string itemId = pObject->itemId;
		std::string modelId = pObject->modelId;
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

							engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", pEntity->entindex(), paramValue.c_str()));
						}
					}
				}
			}
		}
	}
}

void C_MetaverseManager::SetObjectScale(C_PropShortcutEntity* pEntity, float scale)
{
	//pEntity->VPhysicsDestroyObject();
	//pEntity->SetModelScale(scale, 0.0f);
	engine->ClientCmd(VarArgs("setscale %i %f", pEntity->entindex(), scale));	// servercmdfix , false);

	// if this item is a 3d text item, it needs to resize its children.
	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pEntity->GetObjectId());
	std::string itemId = pObject->itemId;
	std::string modelId = pObject->modelId;
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

						engine->ClientCmd(VarArgs("set_text %i \"%s\"\n", pEntity->entindex(), paramValue.c_str()));
					}
				}
			}
		}
	}
}

int C_MetaverseManager::CycleSpawningRotationAxis(int direction)
{
	m_iSpawningRotationAxis += direction;
	if (m_iSpawningRotationAxis > 2)
		m_iSpawningRotationAxis = 0;
	else if (m_iSpawningRotationAxis < 0)
		m_iSpawningRotationAxis = 2;

	return m_iSpawningRotationAxis;
}

void C_MetaverseManager::ResetSpawningAngles()
{
	m_spawningAngles.x = 0;
	m_spawningAngles.y = 0;
	m_spawningAngles.z = 0;
}

void C_MetaverseManager::GetObjectInfo(object_t* pObject, KeyValues* &pObjectInfo, KeyValues* &pItemInfo, KeyValues* &pModelInfo)
{
	pObjectInfo = new KeyValues("objectInfo");

	pObjectInfo->SetString("id", pObject->objectId.c_str());
	pObjectInfo->SetBool("slave", pObject->slave);
	pObjectInfo->SetFloat("scale", pObject->scale);

	// origin
	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->origin.x, pObject->origin.y, pObject->origin.z);
	std::string origin = buf;
	pObjectInfo->SetString("origin", origin.c_str());

	// angles
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->angles.x, pObject->angles.y, pObject->angles.z);
	std::string angles = buf;
	pObjectInfo->SetString("angles", angles.c_str());

	pObjectInfo->SetBool("child", pObject->child);

	pObjectInfo->SetString("anim", pObject->anim.c_str());
	pObjectInfo->SetInt("body", pObject->body);
	pObjectInfo->SetInt("skin", pObject->skin);

	C_PropShortcutEntity* pParentShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->parentEntityIndex));
	std::string parentObjectId = (pParentShortcut) ? pParentShortcut->GetObjectId() : "";
	pObjectInfo->SetString("parentObject", parentObjectId.c_str());

	KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(pObject->itemId));
	if (pItemKV)
	{
		pItemInfo = new KeyValues("itemInfo");

		pItemInfo->SetString("id", pItemKV->GetString("info/id"));
		pItemInfo->SetString("title", pItemKV->GetString("title"));

		std::string workshopId = pItemKV->GetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID));
		pItemInfo->SetString("workshopIds", workshopId.c_str());

		// if there is a workshop ID, get more info.
		SteamWorkshopDetails_t* pWorkshopDetails = g_pAnarchyManager->GetWorkshopManager()->GetWorkshopSubscription(Q_atoui64(workshopId.c_str()));
		if (pWorkshopDetails)
			pItemInfo->SetString("workshopTitle", pWorkshopDetails->title.c_str());
		else
			pItemInfo->SetString("workshopTitle", "");

		/*
		KeyValues* someInfoKV = pItemKV->FindKey(VarArgs("platforms/%s", AA_PLATFORM_ID));
		if (someInfoKV)
		{
			for (KeyValues *sub = someInfoKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				DevMsg("Test value: %s\n", sub->GetName());
		}
		*/
		//pItemInfo->SetString("workshopIds", "TBD");
	}

	KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pObject->modelId));
	if (pModelKV)
	{
		pModelInfo = new KeyValues("modelInfo");

		pModelInfo->SetString("id", pModelKV->GetString("info/id"));
		pModelInfo->SetString("title", pModelKV->GetString("title"));
		
		KeyValues* someInfoKV = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForModelFile(pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
		std::string workshopTitle = "";
		for (KeyValues *sub = someInfoKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			pModelInfo->SetString(sub->GetName(), workshopTitle.c_str());

		std::string workshopId = someInfoKV->GetString("workshopIds");
		someInfoKV->deleteThis();
		SteamWorkshopDetails_t* pWorkshopDetails = g_pAnarchyManager->GetWorkshopManager()->GetWorkshopSubscription(Q_atoui64(workshopId.c_str()));
		pModelInfo->SetString("workshopIds", workshopId.c_str());

		if (pWorkshopDetails)
			pModelInfo->SetString("workshopTitle", pWorkshopDetails->title.c_str());
		else
			pModelInfo->SetString("workshopTitle", "");

		// backpack stuff
		std::string backpackTitle;
		std::string backpackId = g_pAnarchyManager->GetBackpackManager()->DetectRequiredBackpackForModelFile(pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
		if (backpackId != "")
		{
			C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->GetBackpack(backpackId);
			backpackTitle = pBackpack->GetTitle();
		}

		pModelInfo->SetString("backpackIds", backpackId.c_str());
		pModelInfo->SetString("backpackTitle", backpackTitle.c_str());

		/*
		std::string workshopId = pModelKV->GetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID));
		pModelInfo->SetString("workshopIds", workshopId.c_str());

		// if there is a workshop ID, get more info.
		SteamWorkshopDetails_t* pWorkshopDetails = g_pAnarchyManager->GetWorkshopManager()->GetWorkshopSubscription(Q_atoui64(workshopId.c_str()));
		if (pWorkshopDetails)
			pModelInfo->SetString("workshopTitle", pWorkshopDetails->title.c_str());
		else
			pModelInfo->SetString("workshopTitle", "");*/

		//pModelInfo->SetString("mountIds", "TBD");
		
		//pModelInfo->SetString("file", pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
	}
}

void C_MetaverseManager::SetTwitchBotEnabled(bool bValue)
{
	m_pTwitchBotEnabledConVar->SetValue(bValue);
}

void C_MetaverseManager::SetTwitchLiveStatus(bool bValue)
{
	m_bTwitchChannelLive = bValue;
}

void C_MetaverseManager::GameSchemaReceived(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	void* pBuf = malloc(pResult->m_unBodySize);
	steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize);

	std::string responseText = std::string((char*)pBuf);//VarArgs("%s", (unsigned char*)pBuf);
	free(pBuf);
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);

	g_pAnarchyManager->NotifyGameSchemaFetched(responseText);
}

void C_MetaverseManager::UserInfoReceivedHosting(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	//C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	//if (!pNetworkInstance)
	//	return;

	//uint8* pBuf = new uint8(pResult->m_unBodySize);
	void* pBuf = malloc(pResult->m_unBodySize);
	steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize);

	std::string avatarURL = VarArgs("%s", (unsigned char*)pBuf);
	free(pBuf);
	//delete[] pBuf;
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);

	size_t found = avatarURL.find("\"avatarfull\":\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(found + 14);

	found = avatarURL.find("\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(0, found);

	cvar->FindVar("avatar_url")->SetValue(avatarURL.c_str());
	m_bHostSessionNow = true;
}

void C_MetaverseManager::UserInfoReceivedStartup(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	//C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	//if (!pNetworkInstance)
	//	return;

	//uint8* pBuf = new uint8(pResult->m_unBodySize);
	void* pBuf = malloc(pResult->m_unBodySize);
	steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize);

	std::string avatarURL = VarArgs("%s", (unsigned char*)pBuf);
	free(pBuf);
	//delete[] pBuf;
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);

	size_t found = avatarURL.find("\"avatarfull\":\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(found + 14);

	found = avatarURL.find("\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(0, found);

	cvar->FindVar("avatar_url")->SetValue(avatarURL.c_str());
	g_pAnarchyManager->RunAArcade();
}
/*
void C_MetaverseManager::UserInfoReceived(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	//C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	//if (!pNetworkInstance)
	//	return;

	//uint8* pBuf = new uint8(pResult->m_unBodySize);
	void* pBuf = malloc(pResult->m_unBodySize);
	steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize);

	std::string avatarURL = VarArgs("%s", (unsigned char*)pBuf);
	free(pBuf);
	//delete[] pBuf;
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);

	size_t found = avatarURL.find("\"avatarfull\":\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(found + 14);

	found = avatarURL.find("\"");
	if (found != std::string::npos)
		avatarURL = avatarURL.substr(0, found);

	cvar->FindVar("avatar_url")->SetValue(avatarURL.c_str());
	m_bHostSessionNow = true;
}
*/
void C_MetaverseManager::HostSessionNow()
{
	m_bHostSessionNow = false;

	// if we need to extract the overview, now's the time.
	bool bShouldHostNow = true;
	if (cvar->FindVar("sync_overview")->GetBool())
	{
		// 1. resource/overviews/[mapname].txt must exist
		// 2. materials/overviews/[mapname].vtf must exist
		if (g_pFullFileSystem->FileExists(VarArgs("resource/overviews/%s.txt", g_pAnarchyManager->MapName()), "GAME") && g_pFullFileSystem->FileExists(VarArgs("materials/overviews/%s.vtf", g_pAnarchyManager->MapName()), "GAME"))
		{
			bShouldHostNow = false;
			this->ExtractOverviewTGA();
		}
	}

	if ( bShouldHostNow)
		this->ReallyHostNow();
}

void C_MetaverseManager::ClearNetworkDictionaries()
{
	m_networkItemDictionary.clear();
	m_networkModelDictionary.clear();
	m_networkAppDictionary.clear();
	m_networkTypeDictionary.clear();

	// and also clear other bookkeeping
	m_networkRemoteOnlyItems.clear();
	m_networkRemoteOnlyModels.clear();
	m_networkRemoteOnlyTypes.clear();
	m_networkRemoteOnlyApps.clear();
	m_networkMutatedItems.clear();
	m_networkMutatedModels.clear();
	m_networkMutatedTypes.clear();
	m_networkMutatedApps.clear();

	this->DestroyAllDownloadBatches();
	this->DestroyAllUploadBatches();
	m_nowReadyAssets.clear();
}

// NOTE: This method is nearly redundant to the host session's fetching of the user avatar.
// TODO: consolidate
void C_MetaverseManager::FetchUserInfo()
{
	CSteamID sid = steamapicontext->SteamUser()->GetSteamID();
	HTTPRequestHandle requestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, VarArgs("https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?key=%s&steamids=%llu", AA_STEAMWORKS_API_KEY, sid.ConvertToUint64()));

	SteamAPICall_t hAPICall;
	steamapicontext->SteamHTTP()->SendHTTPRequest(requestHandle, &hAPICall);
	m_UserInfoStartupCallback.Set(hAPICall, this, &C_MetaverseManager::UserInfoReceivedStartup);
}

bool C_MetaverseManager::IsRefreshableUserMap(std::string mapFile)
{
	return (g_pFullFileSystem->FileExists(VarArgs("download/maps/%s", mapFile.c_str()), "DEFAULT_WRITE_PATH"));
}

void C_MetaverseManager::RefreshIfUserMap(std::string mapFile_in)
{
	// Find the map entry, to make sure it is legit.
	//KeyValues* pMapKV = this->GetActiveKeyValues(this->FindMap(mapFile_in.c_str()));
	//if (!pMapKV)
		//return;

	std::string mapFile = mapFile_in;//pMapKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
	if (mapFile == "" || mapFile.find("..") != std::string::npos)	// only if a file is found and do **not** allow jumping up from here, as we are about to delete a file.
		return;

	// Confirm that the BSP is in aarcade_user/download/maps/[mapFile].bsp
	if (!g_pFullFileSystem->FileExists(VarArgs("download/maps/%s", mapFile.c_str()), "DEFAULT_WRITE_PATH"))
		return;

	// Remove the BSP
	g_pFullFileSystem->RemoveFile(VarArgs("download/maps/%s", mapFile.c_str()), "DEFAULT_WRITE_PATH");
	g_pAnarchyManager->AddToastMessage(VarArgs("Removed %s in order to re-download.", mapFile.c_str()));
}

bool C_MetaverseManager::IsMapLoadable(std::string mapFile)
{
	bool bExists = g_pFullFileSystem->FileExists(VarArgs("maps/%s", mapFile.c_str()), "GAME");
	return bExists;
}

bool C_MetaverseManager::SmartMergItemKVsMultiplayer(KeyValues* pItemLocalKV, std::string file, std::string preview, std::string stream, std::string screen, std::string marquee)
{
	// merg our existing item with the one from the server, but keep the functionality of ours.
	// Items do *not* automatically save, unless you make other changes to them. 
	// TODO: Keep these separate from the normal values.

	bool bHasMutatedValues = false;
	std::string localValue;
	std::string remoteValue;
	KeyValues* active = this->GetActiveKeyValues(pItemLocalKV);

	// FILE RULES:
	//	REMOTE IS:
	//		HTTP link
	//	AND LOCAL IS NOT:
	//		STEAM GAME (ie. file = number), full local file path, relative local file w/ local app
	localValue = active->GetString("file");
	remoteValue = file;
	if ((remoteValue.find("http") == 0 || remoteValue.find("www") == 0) && remoteValue != localValue)
	{
		if (localValue.length() < 2 || (Q_strcmp(VarArgs("%llu", Q_atoui64(localValue.c_str())), localValue.c_str()) && localValue[1] != ':' && (localValue.find(".") == std::string::npos || !Q_strcmp(active->GetString("app"), ""))))
		{
			active->SetString("file", file.c_str());
			bHasMutatedValues = true;
		}
	}


	// PREVIEW RULES:
	//	REMOTE IS:
	//		HTTP link
	//	AND LOCAL IS NOT:
	//		-
	localValue = active->GetString("preview");
	remoteValue = preview;
	if ((remoteValue.find("http") == 0 || remoteValue.find("www") == 0) && remoteValue != localValue)
	{
		active->SetString("preview", preview.c_str());
		bHasMutatedValues = true;
	}

	// STREAM RULES:
	//	REMOTE IS:
	//		HTTP link
	//	AND LOCAL IS NOT:
	//		-
	localValue = active->GetString("stream");
	remoteValue = stream;
	if ((remoteValue.find("http") == 0 || remoteValue.find("www") == 0) && remoteValue != localValue)
	{
		active->SetString("stream", stream.c_str());
		bHasMutatedValues = true;
	}

	// SCREEN RULES:
	//	REMOTE IS:
	//		HTTP link
	//	AND LOCAL IS NOT:
	//		-
	localValue = active->GetString("screen");
	remoteValue = screen;
	if ((remoteValue.find("http") == 0 || remoteValue.find("www") == 0) && remoteValue != localValue)
	{
		active->SetString("screen", screen.c_str());
		bHasMutatedValues = true;
	}

	// MARQUEE RULES:
	//	REMOTE IS:
	//		HTTP link
	//	AND LOCAL IS NOT:
	//		-
	localValue = active->GetString("marquee");
	remoteValue = marquee;
	if ((remoteValue.find("http") == 0 || remoteValue.find("www") == 0) && remoteValue != localValue)
	{
		active->SetString("marquee", marquee.c_str());
		bHasMutatedValues = true;
	}

	return bHasMutatedValues;
}

/*
void C_MetaverseManager::OnItemUpdated(std::string itemId)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection)
		return;

	auto it = m_networkRemoteOnlyItems.find(itemId);
	if (it == m_networkRemoteOnlyItems.end())
		m_networkMutatedItems[itemId] = true;
}

void C_MetaverseManager::OnModelUpdated(std::string modelId)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection)
		return;

	auto it = m_networkRemoteOnlyModels.find(modelId);
	if (it == m_networkRemoteOnlyModels.end())
		m_networkMutatedModels[modelId] = true;
}

void C_MetaverseManager::OnTypeUpdated(std::string typeId)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection)
		return;

	auto it = m_networkRemoteOnlyTypes.find(typeId);
	if (it == m_networkRemoteOnlyTypes.end())
		m_networkRemoteOnlyTypes[typeId] = true;
}
*/

void C_MetaverseManager::SaveRemoteItemChanges(std::string itemId, bool bIsBulkVolatilePurg, bool bNewOnly)
{
	// construct a new KeyValues for the item
	KeyValues* pItemKV = this->GetLibraryItem(itemId);
	if (!pItemKV)
	{
		DevMsg("ERROR: Item not found in library!\n");
		return;
	}

	KeyValues* pActiveItemKV = this->GetActiveKeyValues(pItemKV);
	if (!pActiveItemKV)
	{
		DevMsg("ERROR: Item invalid.\n");
		return;
	}

	bool bHasChanges = false;

	// bookkeeping
	auto it = m_networkRemoteOnlyItems.find(itemId);
	if (it != m_networkRemoteOnlyItems.end())
	{
		m_networkRemoteOnlyItems.erase(it);
		bHasChanges = true;
	}

	// bookkeeping
	if (!bNewOnly)
	{
		auto it2 = m_networkMutatedItems.find(itemId);
		if (it2 != m_networkMutatedItems.end())
		{
			m_networkMutatedItems.erase(it2);
			bHasChanges = true;
		}
	}

	if (bHasChanges)
	{
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "items", itemId.c_str(), pItemKV, true);

		if (!bIsBulkVolatilePurg)
		{
			// bookkeeping
			KeyValues* pParentKV = m_pVolatileSavesKV->FindKey("items", true);
			KeyValues* pKV = pParentKV->FindKey(itemId.c_str());
			if (pKV)
				pParentKV->RemoveSubKey(pKV);
		}

		// is this item dependent on a type?
		std::string typeId = pActiveItemKV->GetString("type");
		KeyValues* pTypeKV = this->GetLibraryType(typeId);
		KeyValues* pActiveTypeKV = this->GetActiveKeyValues(pTypeKV);
		if (pActiveTypeKV)
			this->SaveRemoteTypeChanges(typeId, true);

		// is this item dependent on a model?
		std::string modelId = pActiveItemKV->GetString("model");
		KeyValues* pModelKV = this->GetLibraryModel(modelId);
		KeyValues* pActiveModelKV = this->GetActiveKeyValues(pModelKV);
		if (pActiveModelKV)
			this->SaveRemoteModelChanges(modelId, true);

		// is this item dependent on an app?
		std::string appId = pActiveItemKV->GetString("app");
		KeyValues* pAppKV = this->GetLibraryApp(appId);
		KeyValues* pActiveAppKV = this->GetActiveKeyValues(pAppKV);
		if (pActiveAppKV)
			this->SaveRemoteAppChanges(appId, true);

		if (!bIsBulkVolatilePurg)
			g_pAnarchyManager->AddToastMessage("Item Saved");
	}
}

void C_MetaverseManager::SaveRemoteTypeChanges(std::string typeId, bool bIsBulkVolatilePurg, bool bNewOnly)
{
	// construct a new KeyValues for the type
	KeyValues* pTypeKV = this->GetLibraryType(typeId);
	if (!pTypeKV)
	{
		DevMsg("ERROR: Type not found in library!\n");
		return;
	}

	KeyValues* pActiveTypeKV = this->GetActiveKeyValues(pTypeKV);
	if (!pActiveTypeKV)
	{
		DevMsg("ERROR: Type invalid.\n");
		return;
	}

	bool bHasChanges = false;

	// bookkeeping
	if (!bNewOnly)
	{
		auto it = m_networkRemoteOnlyTypes.find(typeId);
		if (it != m_networkRemoteOnlyTypes.end())
		{
			m_networkRemoteOnlyTypes.erase(it);
			bHasChanges = true;
		}
	}

	// bookkeeping
	auto it2 = m_networkMutatedTypes.find(typeId);
	if (it2 != m_networkMutatedTypes.end())
	{
		m_networkMutatedTypes.erase(it2);
		bHasChanges = true;
	}

	if (bHasChanges)
	{
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "types", typeId.c_str(), pTypeKV, true);

		if (!bIsBulkVolatilePurg)
		{
			// bookkeeping
			KeyValues* pParentKV = m_pVolatileSavesKV->FindKey("types", true);
			KeyValues* pKV = pParentKV->FindKey(typeId.c_str());
			if (pKV)
				pParentKV->RemoveSubKey(pKV);

			g_pAnarchyManager->AddToastMessage("Type Saved");
		}
	}
}

void C_MetaverseManager::SaveRemoteAppChanges(std::string appId, bool bIsBulkVolatilePurg, bool bNewOnly)
{
	// construct a new KeyValues for the app
	KeyValues* pAppKV = this->GetLibraryApp(appId);
	if (!pAppKV)
	{
		DevMsg("ERROR: App not found in library!\n");
		return;
	}

	KeyValues* pActiveAppKV = this->GetActiveKeyValues(pAppKV);
	if (!pActiveAppKV)
	{
		DevMsg("ERROR: App invalid.\n");
		return;
	}

	bool bHasChanges = false;

	// bookkeeping
	auto it = m_networkRemoteOnlyApps.find(appId);
	if (it != m_networkRemoteOnlyApps.end())
	{
		m_networkRemoteOnlyApps.erase(it);
		bHasChanges = true;
	}

	// bookkeeping
	if (!bNewOnly)
	{
		auto it2 = m_networkMutatedApps.find(appId);
		if (it2 != m_networkMutatedApps.end())
		{
			m_networkMutatedApps.erase(it2);
			bHasChanges = true;
		}
	}

	if (bHasChanges)
	{
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "apps", appId.c_str(), pAppKV, true);

		if (!bIsBulkVolatilePurg)
		{
			// bookkeeping
			KeyValues* pParentKV = m_pVolatileSavesKV->FindKey("apps", true);
			KeyValues* pKV = pParentKV->FindKey(appId.c_str());
			if (pKV)
				pParentKV->RemoveSubKey(pKV);
		}

		// is this app dependent on a type?
		std::string typeId = pActiveAppKV->GetString("type");
		KeyValues* pTypeKV = this->GetLibraryType(typeId);
		KeyValues* pActiveTypeKV = this->GetActiveKeyValues(pTypeKV);
		if (pActiveTypeKV)
			this->SaveRemoteTypeChanges(typeId, true);

		if (!bIsBulkVolatilePurg)
			g_pAnarchyManager->AddToastMessage("App Saved");
	}
}

void C_MetaverseManager::SaveRemoteModelChanges(std::string modelId, bool bIsBulkVolatilePurg, bool bNewOnly)
{
	// construct a new KeyValues for the modle
	KeyValues* pModelKV = this->GetLibraryModel(modelId);
	if (!pModelKV)
	{
		DevMsg("ERROR: Model not found in library!\n");
		return;
	}

	bool bHasChanges = false;

	// bookkeeping
	auto it = m_networkRemoteOnlyModels.find(modelId);
	if (it != m_networkRemoteOnlyModels.end())
	{
		m_networkRemoteOnlyModels.erase(it);
		bHasChanges = true;
	}

	// bookkeeping
	if (!bNewOnly)
	{
		auto it2 = m_networkMutatedModels.find(modelId);
		if (it2 != m_networkMutatedModels.end())
		{
			m_networkMutatedModels.erase(it2);
			bHasChanges = true;
		}
	}

	if (bHasChanges)
	{
		g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "models", modelId.c_str(), pModelKV, true);

		if (!bIsBulkVolatilePurg)
		{
			// bookkeeping
			KeyValues* pParentKV = m_pVolatileSavesKV->FindKey("models", true);
			KeyValues* pKV = pParentKV->FindKey(modelId.c_str());
			if (pKV)
				pParentKV->RemoveSubKey(pKV);

			g_pAnarchyManager->AddToastMessage("Model Saved");
		}
	}
}

bool C_MetaverseManager::IsModelRemoteOnly(std::string modelId)
{
	return (m_networkRemoteOnlyModels.find(modelId) != m_networkRemoteOnlyModels.end());
}

bool C_MetaverseManager::IsTypeRemoteOnly(std::string typeId)
{
	return (m_networkRemoteOnlyTypes.find(typeId) != m_networkRemoteOnlyTypes.end());
}

bool C_MetaverseManager::IsAppRemoteOnly(std::string appId)
{
	return (m_networkRemoteOnlyApps.find(appId) != m_networkRemoteOnlyApps.end());
}

bool C_MetaverseManager::IsItemRemoteOnly(std::string itemId)
{
	return (m_networkRemoteOnlyItems.find(itemId) != m_networkRemoteOnlyItems.end());
}

bool C_MetaverseManager::IsItemMutated(std::string itemId)
{
	return (m_networkMutatedItems.find(itemId) != m_networkMutatedItems.end());
}

bool C_MetaverseManager::IsModelMutated(std::string modelId)
{
	return (m_networkMutatedModels.find(modelId) != m_networkMutatedModels.end());
}

bool C_MetaverseManager::IsTypeMutated(std::string typeId)
{
	return (m_networkMutatedTypes.find(typeId) != m_networkMutatedTypes.end());
}

bool C_MetaverseManager::IsAppMutated(std::string appId)
{
	return (m_networkMutatedApps.find(appId) != m_networkMutatedApps.end());
}

void C_MetaverseManager::OnAssetRequestAdded(std::string requestHash, std::string requestedAsset)
{
	bool bRequestAccepted = false;
	if (m_pCloudAssetsUploadConVar->GetBool() && g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected && g_pAnarchyManager->GetConnectedUniverse()->isHost && g_pFullFileSystem->FileExists(requestedAsset.c_str(), "GAME"))
	{
		/*
		PathTypeQuery_t pathTypeQuery;
		char* path = new char[AA_MAX_STRING];
		g_pFullFileSystem->RelativePathToFullPath(requestedAsset.c_str(), "GAME", path, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);
		if (pathTypeQuery != PATH_IS_NORMAL || strstr(path, ".vpk\\") != null || strstr(path, ".vpk/") != null || strstr(path, ".VPK\\") != null || strstr(path, ".VPK/") != null)
		{
			CUtlBuffer buf;
			if (g_pFullFileSystem->ReadFile(file.c_str(), packingPath, buf))
				g_pFullFileSystem->WriteFile(VarArgs("workshop\\package\\%s", file.c_str()), USE_GAME_PATH, buf);
		}
		*/

		int iExtensionIndex = requestedAsset.length() - 4;
		if (requestedAsset.find(".mdl") == iExtensionIndex || requestedAsset.find(".MDL") == iExtensionIndex)
		{
			// build the search info
			KeyValues* pSearchInfo = new KeyValues("search");	// this gets deleted by the metaverse manager!! (cuz we are not using FindLibraryEntry)
			pSearchInfo->SetString("file", requestedAsset.c_str());

			// start the search
			KeyValues* pModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryModel(pSearchInfo));
			if (pModel)
			{
				std::string modelId = pModel->GetString("info/id");
				std::string batchId = this->AddModelToUploadBatch(modelId);
				this->SendBatch(batchId);
				bRequestAccepted = true;
			}
		}
		else if (requestedAsset.find(".bsp") == iExtensionIndex || requestedAsset.find(".BSP") == iExtensionIndex)
		{
			// do nothing special, as this request is only ever issued by ourselves.

			//DevMsg("Unhandled Map Sync: %s\n", requestedAsset.c_str());
			/*
			std::string searchFileName = requestedAsset;
			std::transform(searchFileName.begin(), searchFileName.end(), searchFileName.begin(), ::tolower);

			std::string ownerId = cvar->FindVar("aamp_client_id")->GetString();
			std::string ownedFileEnding = VarArgs("%s.bsp", ownerId.c_str());

			std::vector<std::string> tokens;
			g_pAnarchyManager->Tokenize(searchFileName, tokens, "_");
			if (tokens.size() > 0)
			{
				if (tokens.at(tokens.size() - 1) == ownedFileEnding)
				{
					DevMsg("Huzzah!  This map is from the local user's Projects folder.\n");
				}
			}
			*/
			// TODO:
			// - PHASE 2: When a user updates their map, guests will want to delete their old version so the new version re-downloads.  Date modified stamps should be compared, combined with availability of the new version on the cloud, to determine if deleting & re-downloading should be done prior to actually connecting to a server.
			// - PHASE 3: Load the VMF to check for materials & models used to add to the additional asset list for this BSP's entry.
			// - fin

			/*
			// build the search info
			KeyValues* pSearchInfo = new KeyValues("search");	// this gets deleted by the metaverse manager!! (cuz we are not using FindLibraryEntry)
			pSearchInfo->SetString("file", requestedAsset.c_str());

			// start the search
			KeyValues* pModel = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindLibraryModel(pSearchInfo));
			if (pModel)
			{
				std::string modelId = pModel->GetString("info/id");
				std::string batchId = this->AddModelToUploadBatch(modelId);
				this->SendBatch(batchId);
				bRequestAccepted = true;
			}
			*/
		}
	}

	if (!bRequestAccepted)
	{
		// rejest the request
		C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");

		std::vector<std::string> args;
		args.push_back(requestHash);
		args.push_back(requestedAsset);
		//g_pAnarchyManager->AddToastMessage(VarArgs("Rejected Cloud Asset Request %s", requestedAsset.c_str()));
		pNetworkInstance->DispatchJavaScriptMethod("assetManager", "requestUnavailable", args);
	}
}

std::string C_MetaverseManager::RequestAsset(std::string file)
{
	if (!g_pFullFileSystem->FileExists(file.c_str(), "GAME"))
	{
		// callback
		C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");

		std::string requestId = g_pAnarchyManager->GenerateUniqueId();	// FIXME: This requestID is not used ANYWHERE, nor is it retained for the duration of the asset download.
		std::vector<std::string> args;
		args.push_back(requestId);
		args.push_back(file);

		std::string fileHash = g_pAnarchyManager->GenerateLegacyHash(file.c_str());
		args.push_back(fileHash);

		// keep track of it so we can show the user what's happening behind the scenes.
		KeyValues* pRequestedKV = new KeyValues("request");
		pRequestedKV->SetString("request", VarArgs("id%s", requestId.c_str()));
		pRequestedKV->SetString("id", VarArgs("id%s", fileHash.c_str()));
		pRequestedKV->SetString("file", file.c_str());
		m_requestedFiles.push_back(pRequestedKV);

		//g_pAnarchyManager->AddToastMessage(VarArgs("Requesting Cloud Asset %s", file.c_str()));

		pNetworkInstance->DispatchJavaScriptMethod("assetManager", "requestAsset", args);

		// notify the transfers UI menu
		C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

		JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
		if (response.IsObject())
		{
			JSObject object = response.ToObject();
			JSArray arguments;

			/*
			JSObject batch;
			batch.SetProperty(WSLit("request"), WSLit(requestId.c_str()));
			batch.SetProperty(WSLit("id"), WSLit(fileHash.c_str()));
			batch.SetProperty(WSLit("file"), WSLit(file.c_str()));
			arguments.Push(batch);
			*/

			JSObject batch;
			AddSubKeys2(pRequestedKV, batch, true);
			arguments.Push(batch);

			object.InvokeAsync(WSLit("onFileRequested"), arguments);
		}


		return requestId;
	}
	else
	{
		//g_pAnarchyManager->AddToastMessage(VarArgs("Asset Already Exists %s", file.c_str()));
		return "";
	}
}

bool C_MetaverseManager::GetTopDownloadCloudAssetBatchFile(KeyValues* pBatchKV, std::string& topFileId, std::string& topFile, std::string& topFileURL)
{
	bool bGotTopFile = false;
	// grab the 1st file that isn't already downloaded...
	KeyValues* pDownloadedKV = pBatchKV->FindKey("downloaded", true);

	/*
	std::string topFileId;
	std::string topFile;
	std::string topFileURL;
	*/

	// check if main file is the top
	std::string batchId = pBatchKV->GetString("id");

	if (batchId.length() < 3)
	{
		DevMsg("ERROR: Invalid batch ID for download.  Ignoring.\n");
		return false;
	}

	batchId = batchId.substr(2);

	if (!pDownloadedKV->FindKey(VarArgs("id%s", batchId.c_str())))
	{
		topFileId = batchId;
		topFile = pBatchKV->GetString("file");
		topFileURL = pBatchKV->GetString("url");
		std::replace(topFile.begin(), topFile.end(), '\\', '/');

		//if (topFile != "")
			bGotTopFile = true;
	}
	else
	{
		std::string fileId;
		// otherwise, find the top file
		for (KeyValues *pOtherKV = pBatchKV->FindKey("other", true)->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
		{
			fileId = pOtherKV->GetName();
			if (fileId.length() < 3)
			{
				DevMsg("ERROR: fileId is length less than 3! Invalid!\n");
				return false;
			}
			fileId = fileId.substr(2);

			if (!pDownloadedKV->FindKey(VarArgs("id%s", fileId.c_str())))
			{
				topFileId = fileId;
				topFile = pOtherKV->GetString("file");
				topFileURL = pOtherKV->GetString("url");
				std::replace(topFile.begin(), topFile.end(), '\\', '/');

				//if (topFile != "")
					bGotTopFile = true;
				break;
			}
		}
	}

	/*if (bGotTopFile)
		DevMsg("Top File: %s\n", topFile.c_str());
	else
		DevMsg("Did not find a valid top file.\n");*/

	return bGotTopFile;
}

void C_MetaverseManager::DownloadNextCloudAssetBatchFile(KeyValues* pBatchKV)
{
	// grab the 1st file that isn't already downloaded...
	std::string topFileId;
	std::string topFile;
	std::string topFileURL;

	std::string batchId = pBatchKV->GetString("id");
	if (batchId.length() < 3)
	{
		DevMsg("ERROR: batchId (echo biscuit) is length less than 3! Invalid!\n");
		return;
	}
	batchId = batchId.substr(2);

	bool bGotTopFile = this->GetTopDownloadCloudAssetBatchFile(pBatchKV, topFileId, topFile, topFileURL);
	if (!bGotTopFile)
	{
		std::string fileId = pBatchKV->GetString("id");
		if (fileId.length() < 3)
		{
			DevMsg("ERROR: fileId (echo zumba) is length less than 3! Invalid!\n");
			return;
		}
		fileId = fileId.substr(2);
		std::string file = pBatchKV->GetString("file");
		g_pAnarchyManager->AddToastMessage(VarArgs("Downloaded %s", file.c_str()));
		this->DestroyDownloadBatch(batchId.c_str());

		// delete current_download.txt
		if (g_pFullFileSystem->FileExists("current_download.txt", "DEFAULT_WRITE_PATH"))
			g_pFullFileSystem->RemoveFile("current_download.txt", "DEFAULT_WRITE_PATH");

		if (file.find(".mdl") == file.length() - 4 || file.find(".MDL") == file.length() - 4)
		{
			// The asset is finished, so time to replace the model of any object waiting for it.
			// Attempt to re-load it...
			//int index = modelinfo->GetModelIndex(file.c_str());
			//modelinfo->UnreferenceModel(index);
			//const model_t* pModel = modelinfo->FindOrLoadModel(file.c_str());

			model_asset_ready_t* pAssetReady = new model_asset_ready_t();
			pAssetReady->fileHash = fileId;
			pAssetReady->file = file;
			m_nowReadyAssets.push_back(pAssetReady);

			g_pAnarchyManager->GetInstanceManager()->OnModelAssetReady(fileId.c_str(), file.c_str());
		}
		else if (file.find(".bsp") == file.length() - 4 || file.find(".BSP") == file.length() - 4)
		{
			DevMsg("Map is finished downloading: %s\n", file.c_str());

			auto found = file.find("/");
			if (found != std::string::npos)
			{
				std::string shortFile = file.substr(found + 1);

				this->DetectMap(shortFile);

				// save it to the map cache right away
				KeyValues* pCachedMapsKV = new KeyValues("maps");
				if (pCachedMapsKV->LoadFromFile(g_pFullFileSystem, "map_cache.txt", "DEFAULT_WRITE_PATH"))
				{
					pCachedMapsKV->CreateNewKey()->SetString("", shortFile.c_str());
					pCachedMapsKV->SaveToFile(g_pFullFileSystem, "map_cache.txt", "DEFAULT_WRITE_PATH");
					pCachedMapsKV->deleteThis();
				}
			}
		}

		// notify the transfers UI menu
		C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

		JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
		if (response.IsObject())
		{
			JSObject object = response.ToObject();
			JSArray arguments;

			std::string batchId = pBatchKV->GetString("id");
			if (batchId.length() >= 3)
			{
				batchId = batchId.substr(2);

				JSObject batch;
				AddSubKeys2(pBatchKV, batch, true);
				arguments.Push(batch);
			}

			object.InvokeAsync(WSLit("onDownloadComplete"), arguments);
		}


		this->DownloadNextCloudAssetBatch();
	}
	else
	{
		if (topFile != "" && !g_pFullFileSystem->FileExists(topFile.c_str(), "GAME"))
		{
			//DevMsg("Downloading file: %s\n", topFile.c_str());

			//g_pAnarchyManager->AddToastMessage(VarArgs("Downloading %s", topFile.c_str()));
			HTTPRequestHandle requestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, topFileURL.c_str());

			// set us as downloading
			//pBatchKV->SetPtr(VarArgs("downloading/id%s", topFileId.c_str()), (void*)(&requestHandle));

			SteamAPICall_t hAPICall;
			steamapicontext->SteamHTTP()->SendHTTPRequest(requestHandle, &hAPICall);
			m_AssetDownloadHTTPResponseCallback.Set(hAPICall, this, &C_MetaverseManager::AssetDownloadHTTPResponse);
		}
		else
		{
			//g_pAnarchyManager->AddToastMessage(VarArgs("Skipping %s", topFile.c_str()));

			// mark us as downloaded...
			pBatchKV->SetBool(VarArgs("downloaded/id%s", topFileId.c_str()), true);

			// save out the current KV in case we crash before the next completion...
			pBatchKV->SaveToFile(g_pFullFileSystem, "current_download.txt", "DEFAULT_WRITE_PATH");

			// call the next one
			this->DownloadNextCloudAssetBatchFile(pBatchKV);
		}
	}
}

void C_MetaverseManager::AssetDownloadHTTPResponse(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	// get the top download batch
	KeyValues* pBatchKV = m_downloadBatches[0];

	// grab the top file of the batch
	std::string topFileId;
	std::string topFile;
	std::string topFileURL;

	if (!this->GetTopDownloadCloudAssetBatchFile(pBatchKV, topFileId, topFile, topFileURL))
	{
		DevMsg("ERROR: Could not find top download batch file!!\n");
		steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
		return;
	}

	// unzip the asset response to the asset location
	//g_pAnarchyManager->AddToastMessage(VarArgs("Download Complete: %s (%u)", topFile.c_str(), pResult->m_unBodySize));

	void* pBuf = malloc(pResult->m_unBodySize);
	if (!steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pResult->m_hRequest, (uint8*)pBuf, pResult->m_unBodySize))
	{
		DevMsg("ERROR: Failed to get cloud response data.\n");
		free(pBuf);
		steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
		return;
	}

	//DevMsg("Received ZIP %s\n", topFile.c_str());

	HZIP hz = OpenZip(pBuf, pResult->m_unBodySize, ZIP_MEMORY);
	if (!hz)
	{
		DevMsg("ERROR: Failed to open cloud response ZIP.\n");
		free(pBuf);
		steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
		return;
	}

	int zipIndex;
	ZIPENTRY zipEntry;
	ZRESULT result = FindZipItem(hz, topFile.c_str(), true, &zipIndex, &zipEntry);
	if (result != ZR_OK)
	{
		DevMsg("ERROR: Could not find file in ZIP %s\n", topFile.c_str());
		CloseZip(hz);
		free(pBuf);
		steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
		return;
	}

	std::string relativeFile = VarArgs("download/%s", topFile.c_str());
	std::replace(relativeFile.begin(), relativeFile.end(), '\\', '/');
	std::string relativePathOnly = relativeFile.substr(0, relativeFile.find_last_of("/"));
	g_pFullFileSystem->CreateDirHierarchy(relativePathOnly.c_str(), "DEFAULT_WRITE_PATH");

	// Now unzip the BSP
	std::string fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
	std::replace(fullFile.begin(), fullFile.end(), '\\', '/');
	fullFile += "/";
	fullFile += relativeFile;
	//DevMsg("Unzipping to %s\n", fullFile.c_str());
	result = UnzipItem(hz, zipIndex, VarArgs("%s", fullFile.c_str()), 0, ZIP_FILENAME);
	if (result != ZR_OK)
	{
		DevMsg("ERROR: Could not UNZIP file to %s\n", relativeFile.c_str());
		CloseZip(hz);
		free(pBuf);
		steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
		return;
	}
	CloseZip(hz);
	free(pBuf);
	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);

	//g_pAnarchyManager->AddToastMessage(VarArgs("Download Complete: %s", topFile.c_str()));

	// mark us as downloaded...
	pBatchKV->SetBool(VarArgs("downloaded/id%s", topFileId.c_str()), true);

	// save out the current KV in case we crash before the next completion...
	pBatchKV->SaveToFile(g_pFullFileSystem, "current_download.txt", "DEFAULT_WRITE_PATH");


	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		std::string batchId = pBatchKV->GetString("id");
		if (batchId.length() >= 3)
		{
			batchId = batchId.substr(2);

			JSObject batch;
			AddSubKeys2(pBatchKV, batch, true);
			arguments.Push(batch);
			arguments.Push(WSLit(topFileId.c_str()));
		}

		object.InvokeAsync(WSLit("onDownloadUpdate"), arguments);
	}


	// call the next one
	this->DownloadNextCloudAssetBatchFile(pBatchKV);
}

/*
void C_MetaverseManager::AssetDownloadHTTPHeadersResponse(HTTPRequestCompleted_t* pResult, bool bIOFailure)
{
	if (pResult->m_bRequestSuccessful)
	{
		uint32 uHeaderSize;
		if (steamapicontext->SteamHTTP()->GetHTTPResponseHeaderSize(pResult->m_hRequest, "Content-Type", &uHeaderSize))
		{
			uint8* fileSize = new uint8[uHeaderSize];
			steamapicontext->SteamHTTP()->GetHTTPResponseHeaderValue(pResult->m_hRequest, "Content-Length", fileSize, uHeaderSize);
			std::string size = (char*)fileSize;
			delete[] fileSize;

			auto pBatchKV = reinterpret_cast<KeyValues*>(pResult->m_ulContextValue);
			for (unsigned int i = 0; i < m_downloadBatches.size(); i++)
			{
				if (m_downloadBatches[i] == pBatchKV)
				{
					DevMsg("File Size: %s for %s\n", size.c_str(), pBatchKV->GetString("file"));
					break;
				}
			}
		}
	}

	steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pResult->m_hRequest);
}
*/

void C_MetaverseManager::DownloadNextCloudAssetBatch()
{
	if (m_downloadBatches.size() < 1)
	{
		//g_pAnarchyManager->AddToastMessage("All Asset Downloads Complete");
		return;
	}

	// get the top download batch
	KeyValues* pBatchKV = m_downloadBatches[0];
	if (Q_strcmp(pBatchKV->GetString("status"), "ready"))
	{
		DevMsg("ERROR: Top download batch status is *NOT* equal to \"ready\".\n");
		return;
	}

	std::string batchId = pBatchKV->GetString("id");
	if (batchId.length() < 3)
	{
		DevMsg("ERROR: batchId (monica ben) is length less than 3! Invalid!\n");
		return;
	}
	batchId = batchId.substr(2);

	pBatchKV->SetString("status", "downloading");

	//g_pAnarchyManager->AddToastMessage(VarArgs("Downloading Cloud Asset %s", pBatchKV->GetString("file")));

	// save current_download.txt
	pBatchKV->SaveToFile(g_pFullFileSystem, "current_download.txt", "DEFAULT_WRITE_PATH");
	this->DownloadNextCloudAssetBatchFile(pBatchKV);
}

void C_MetaverseManager::DownloadCloudAssetBatch(KeyValues* pCloudAssetRequestKV)
{
	m_downloadBatches.push_back(pCloudAssetRequestKV);

	/*
	// Now let's grab file sizes for everything in this download...
	HTTPRequestHandle headerRequestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodHEAD, pCloudAssetRequestKV->GetString("url"));

	auto u64KeyValuesPointer = reinterpret_cast<uintptr_t>(pCloudAssetRequestKV);
	steamapicontext->SteamHTTP()->SetHTTPRequestContextValue(headerRequestHandle, u64KeyValuesPointer);

	SteamAPICall_t hAPICall;
	steamapicontext->SteamHTTP()->SendHTTPRequest(headerRequestHandle, &hAPICall);
	m_AssetDownloadHTTPResponseCallback.Set(hAPICall, this, &C_MetaverseManager::AssetDownloadHTTPHeadersResponse);
	*/


	// notify the transfers UI menu
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

	JSValue response = pHudInstance->GetWebView()->ExecuteJavascriptWithResult(WSLit("transferListener"), WSLit(""));
	if (response.IsObject())
	{
		JSObject object = response.ToObject();
		JSArray arguments;

		std::string batchId = pCloudAssetRequestKV->GetString("id");
		if (batchId.length() >= 3)
		{
			batchId = batchId.substr(2);

			JSObject batch;
			AddSubKeys2(pCloudAssetRequestKV, batch, true);
			arguments.Push(batch);
		}

		object.InvokeAsync(WSLit("onDownloadAdded"), arguments);
	}


	// now process the download, if needed.
	if (m_downloadBatches.size() == 1)
		this->DownloadNextCloudAssetBatch();
	else
	{
		//g_pAnarchyManager->AddToastMessage("Added Asset to Download Queue");
	}
}

void C_MetaverseManager::OnCloudAssetAvailable(KeyValues* pCloudAssetRequestKV)
{
	/* WORKING: Print all file names & info to console...
	DevMsg("Download Asset:\n\t%s\n\t%s\n", pCloudAssetRequestKV->GetString("file"), pCloudAssetRequestKV->GetString("url"));
	for (KeyValues *pOtherKV = pCloudAssetRequestKV->FindKey("other", true)->GetFirstSubKey(); pOtherKV; pOtherKV = pOtherKV->GetNextKey())
	{
		DevMsg("Dependency:\n\t%s\n\t%s\n", pOtherKV->GetString("file"), pOtherKV->GetString("url"));
	}
	*/

	// Add it to the download queue
	this->DownloadCloudAssetBatch(pCloudAssetRequestKV);
}

void C_MetaverseManager::OnCloudAssetUnavailable(std::string requestId, std::string file, std::string fileHash)
{
	//g_pAnarchyManager->AddToastMessage(VarArgs("Cloud Asset Unavailable %s", file.c_str()));

	/*
	// Notify the transfers tab so we can show the user what's happening.
	std::vector<std::string> args;
	args.push_back(requestId);
	args.push_back(file);
	args.push_back(fileHash);

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->DispatchJavaScriptMethod("transferManager", "onCloudAssetRequested", args);*/
}

void C_MetaverseManager::DestroyDownloadBatch(std::string batchId)
{
	// find the batch
	KeyValues* pTargetKV = null;
	unsigned int i;
	for (i = 0; i < m_downloadBatches.size(); i++)
	{
		if (!Q_strcmp(m_downloadBatches[i]->GetString("id"), VarArgs("id%s", batchId.c_str())))
		{
			pTargetKV = m_downloadBatches[i];
			break;
		}
	}

	if (!pTargetKV)
	{
		DevMsg("ERROR: Cannot find download batch to destroy.\n");
		return;
	}

	/*
	// check if the top entry is currently downloading or not...
	if (!Q_strcmp(pTargetKV->GetString("status"), "downloading"))
		pTargetKV->SaveToFile(g_pFullFileSystem, "unfinished_download.txt", "DEFAULT_WRITE_PATH");
	*/

	/* let's try moving it to a downloaded KV instead.
	pTargetKV->deleteThis();
	m_downloadBatches.erase(m_downloadBatches.begin() + i);
	*/
	pTargetKV->SetString("status", "complete");
	m_downloadedBatches.push_back(pTargetKV);
	m_downloadBatches.erase(m_downloadBatches.begin() + i);
}

void C_MetaverseManager::DestroyAllDownloadBatches()
{
	/*
	// check if the top entry is currently downloading or not...
	if (m_downloadBatches.size() > 0)
	{
		// save out the in-process download batch (if its status is "downloading") so it can be resumed next time.
		KeyValues* pTopDownloadBatch = m_downloadBatches[0];
		if (!Q_strcmp(pTopDownloadBatch->GetString("status"), "downloading"))
			pTopDownloadBatch->SaveToFile(g_pFullFileSystem, "unfinished_download.txt", "DEFAULT_WRITE_PATH");
	}
	*/

	for (unsigned int i = 0; i < m_downloadBatches.size(); i++)
		m_downloadBatches[i]->deleteThis();
	m_downloadBatches.clear();

	for (unsigned int i = 0; i < m_downloadedBatches.size(); i++)
		m_downloadedBatches[i]->deleteThis();
	m_downloadedBatches.clear();

	for (unsigned int i = 0; i < m_requestedFiles.size(); i++)
		m_requestedFiles[i]->deleteThis();
	m_requestedFiles.clear();
}

void C_MetaverseManager::OnEntryCreated(std::string mode, std::string id, KeyValues* pEntryDataKV)
{
	if (m_pDebugDisableMPModelsConVar->GetBool() && mode == "models")
		return;

	if (m_pDebugDisableMPItemsConVar->GetBool() && mode == "items")
		return;

	//DevMsg("OnEntryCreated: %s\n", mode.c_str());

	/*
		APPLIES TO IDS FOR: items, models, types, apps, (but not objects)
		- Is this ID already in the m_networkDictionary map?
			- YES: Use the existing ID instead.
			- NO: Create this entry.
	*/
	/*
	DevMsg("Initial created ID: %s (%s)\n", id.c_str(), mode.c_str());

	if (id == "-q6Q8_HRxSBgqwzVLd2N")
	{
		DevMsg("Tester comin up!\n");
	}*/

	KeyValues* pEntryKV;
	if (mode != "objects")
	{
		std::string goodId = id;

		if (mode == "items")
		{
			auto existingIt = m_networkItemDictionary.find(id);
			if (existingIt != m_networkItemDictionary.end())
				goodId = existingIt->second;
		}
		else if (mode == "models")
		{
			auto existingIt = m_networkModelDictionary.find(id);
			if (existingIt != m_networkModelDictionary.end())
				goodId = existingIt->second;
		}
		else if (mode == "types")
		{
			auto existingIt = m_networkTypeDictionary.find(id);
			if (existingIt != m_networkTypeDictionary.end())
				goodId = existingIt->second;
		}
		else if (mode == "apps")
		{
			auto existingIt = m_networkAppDictionary.find(id);
			if (existingIt != m_networkAppDictionary.end())
				goodId = existingIt->second;
		}

		pEntryKV = this->GetLibraryEntry(mode, goodId);

		if (!pEntryKV && goodId != id)
		{
			goodId = id;
			pEntryKV = this->GetLibraryEntry(mode, goodId);
		}

		if (pEntryKV)
		{
			if (mode == "items")
			{
				if (false) 	// disable item mutating, for now. (It results in inaccurate visual representations compared to what the host sees.)
				{
					std::string file = pEntryDataKV->GetString("file");
					std::string preview = pEntryDataKV->GetString("preview");
					std::string stream = pEntryDataKV->GetString("stream");
					std::string screen = pEntryDataKV->GetString("screen");
					std::string marquee = pEntryDataKV->GetString("marquee");
					if (this->SmartMergItemKVsMultiplayer(pEntryKV, file, preview, stream, screen, marquee))
					{
						KeyValues* kv = this->GetActiveKeyValues(pEntryKV);
						if (kv)
						{
							std::string entryId = kv->GetString("info/id");
							if (entryId != "")
							{
								m_networkMutatedItems[entryId] = true;
								this->SaveSQL(null, "items", entryId.c_str(), kv, false, true);
							}
						}
					}
				}
				else
				{
					std::string file = pEntryDataKV->GetString("file");
					std::string preview = pEntryDataKV->GetString("preview");
					std::string stream = pEntryDataKV->GetString("stream");
					std::string screen = pEntryDataKV->GetString("screen");
					std::string marquee = pEntryDataKV->GetString("marquee");
					if (this->SmartMergItemKVsMultiplayer(pEntryKV, file, preview, stream, screen, marquee))
					{
						KeyValues* kv = this->GetActiveKeyValues(pEntryKV);
						if (kv)
						{
							std::string entryId = kv->GetString("info/id");
							if (entryId != "")
							{
								/*
								// just FORCE a few fields to match, to preserve visually consistency with what other players see.
								kv->SetString("preview", pEntryDataKV->GetString("preview"));
								kv->SetString("stream", pEntryDataKV->GetString("stream"));
								kv->SetString("screen", pEntryDataKV->GetString("screen"));
								kv->SetString("marquee", pEntryDataKV->GetString("marquee"));
								*/

								m_networkMutatedItems[entryId] = true;
								this->SaveSQL(null, "items", entryId.c_str(), kv, false, true);
							}
						}
					}
				}
			}
			//else if(mode == "models") ...
		}
		else
		{
			// no DIRECT ID match, so time to SEARCH for the entry...
			if (mode == "items")
			{
				std::string title = pEntryDataKV->GetString("title");
				std::string description = pEntryDataKV->GetString("description");
				std::string file = pEntryDataKV->GetString("file");

				std::string type = pEntryDataKV->GetString("type");
				auto existingTypeIt = m_networkTypeDictionary.find(type);
				if (existingTypeIt != m_networkTypeDictionary.end())
					type = existingTypeIt->second;

				std::string app = pEntryDataKV->GetString("app");
				auto existingAppIt = m_networkAppDictionary.find(app);
				if (existingAppIt != m_networkAppDictionary.end())
					app = existingAppIt->second;

				std::string reference = pEntryDataKV->GetString("reference");
				std::string preview = pEntryDataKV->GetString("preview");
				std::string download = pEntryDataKV->GetString("download");
				std::string stream = pEntryDataKV->GetString("stream");
				std::string screen = pEntryDataKV->GetString("screen");
				std::string marquee = pEntryDataKV->GetString("marquee");
				std::string model = "";// pEntryDataKV->GetString("model");
				std::string keywords = pEntryDataKV->GetString("keywords");

				// FIRST, CHECK FOR REFRENCE FIELD MATCH...
				std::map<std::string, KeyValues*>::iterator it;
				KeyValues* pSearchInfo;
				if (reference != "")
				{
					it = m_items.begin();
					pSearchInfo = new KeyValues("search");

					std::string searchTerm = reference;
					std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);

					pSearchInfo->SetString("reference", searchTerm.c_str());
					pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
					pSearchInfo->deleteThis();
				}

				if (pEntryKV)
				{
					/*
					if (id == "-q6Q8_HRxSBgqwzVLd2N")
					{
						DevMsg("Yar...\n");
						for (KeyValues *sub = pEntryKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
						{
							DevMsg("Key Sub Name: %s\n", sub->GetName());
						}
					}*/
					m_networkItemDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
				}
				else
				{
					// SECOND, CHECK FOR FILE MATCH
					if (file != "")
					{
						it = m_items.begin();
						pSearchInfo = new KeyValues("search");

						std::string searchTerm = file;
						std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);

						pSearchInfo->SetString("file", searchTerm.c_str());
						pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
						pSearchInfo->deleteThis();
					}

					if (pEntryKV)
						m_networkItemDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
					else
					{
						// THIRD, CHECK FOR TITLE MATCH
						// ONLY if our title is longer than 5 characters & not on our list of blacklisted title matches.
						if (title != "" && title.length() > 5 && !this->IsBlacklistedTitleMatch(title))
						{
							it = m_items.begin();
							pSearchInfo = new KeyValues("search");

							std::string searchTerm = title;
							std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);

							KeyValues* pSearchTypeKV = this->GetActiveKeyValues(this->GetLibraryType(type));
							std::string searchType = (pSearchTypeKV) ? pSearchTypeKV->GetString("title") : "";
							std::transform(searchType.begin(), searchType.end(), searchType.begin(), ::tolower);

							pSearchInfo->SetString("title", searchTerm.c_str());
							pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it, true, searchType);
							pSearchInfo->deleteThis();
						}

						if (pEntryKV)
							m_networkItemDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
					}
				}

				// IF NOT FOUND, CREATE IT...
				if (!pEntryKV)
				{
					std::string kvTitle = mode.substr(0, mode.length() - 1);

					pEntryKV = new KeyValues(kvTitle.c_str());
					this->CreateItem(0, goodId, pEntryKV, title, description, file, type, app, reference, preview, download, stream, screen, marquee, model, keywords);
					this->AddItem(pEntryKV);
					m_networkRemoteOnlyItems[goodId] = true;
					this->SaveSQL(null, "items", goodId.c_str(), pEntryKV, false, true);	// so that it's in volatileKV
				}
				else
				{
					if (this->SmartMergItemKVsMultiplayer(pEntryKV, file, preview, stream, screen, marquee))
					{
						KeyValues* kv = this->GetActiveKeyValues(pEntryKV);
						if (kv)
						{
							std::string entryId = kv->GetString("info/id");
							if (entryId != "")
							{
								m_networkMutatedItems[entryId] = true;
								this->SaveSQL(null, "items", entryId.c_str(), kv, false, true);
							}
						}
					}
				}
			}
			else if (mode == "apps")
			{
				// FIRST, CHECK FOR REFRENCE FIELD MATCH...
				std::map<std::string, KeyValues*>::iterator it;
				KeyValues* pSearchInfo;
				if (Q_strcmp(pEntryDataKV->GetString("reference"), ""))
				{
					it = m_items.begin();
					pSearchInfo = new KeyValues("search");
					pSearchInfo->SetString("reference", pEntryDataKV->GetString("reference"));
					pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
					pSearchInfo->deleteThis();
				}

				if (pEntryKV)
					m_networkAppDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
				else
				{
					// SECOND, CHECK FOR TITLE MATCH
					if (Q_strcmp(pEntryDataKV->GetString("title"), ""))
					{
						it = m_items.begin();
						pSearchInfo = new KeyValues("search");
						pSearchInfo->SetString("title", pEntryDataKV->GetString("title"));
						pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
						pSearchInfo->deleteThis();
					}

					if (pEntryKV)
						m_networkAppDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
					/*else
					{
						// THIRD, CHECK FOR TYPE MATCH
						if (pEntryDataKV->GetString("type") != "")
						{
							it = m_items.begin();
							pSearchInfo = new KeyValues("search");
							pSearchInfo->SetString("type", pEntryDataKV->GetString("type"));
							pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
							pSearchInfo->deleteThis();
						}

						if (pEntryKV)
							m_networkAppDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");
					}*/
				}

				// IF NOT FOUND, CREATE IT...
				if (!pEntryKV)
				{
					std::string kvTitle = mode.substr(0, mode.length() - 1);

					pEntryKV = new KeyValues(kvTitle.c_str());
					//this->CreateItem(0, goodId, pEntryKV, title, description, file, type, app, reference, preview, download, stream, screen, marquee, model);

					pEntryKV->SetInt("generation", 3);
					pEntryKV->SetString("local/info/owner", "local");
					pEntryKV->SetString("local/info/id", goodId.c_str());
					pEntryKV->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));

					pEntryKV->SetString("local/title", pEntryDataKV->GetString("title"));
					pEntryKV->SetString("local/file", pEntryDataKV->GetString("file"));
					pEntryKV->SetString("local/commandformat", pEntryDataKV->GetString("commandformat"));
					pEntryKV->SetString("local/description", "");

					std::string type = pEntryDataKV->GetString("type");
					auto existingTypeIt = m_networkTypeDictionary.find(type);
					if (existingTypeIt != m_networkTypeDictionary.end())
						type = existingTypeIt->second;
					pEntryKV->SetString("local/type", type.c_str());

					pEntryKV->SetString("local/reference", pEntryDataKV->GetString("reference"));
					pEntryKV->SetString("local/download", pEntryDataKV->GetString("download"));
					//pEntryKV->SetString("local/screen", pEntryDataKV->GetString("screen"));
					pEntryKV->FindKey("local/filepaths", true);
					this->AddApp(pEntryKV);
					m_networkRemoteOnlyApps[goodId] = true;
					this->SaveSQL(null, "apps", goodId.c_str(), pEntryKV, false, true);	// so that it's in volatileKV
				}
			}
			else if (mode == "models")
			{
				// FIRST, CHECK FOR FILE FIELD MATCH...
				std::map<std::string, KeyValues*>::iterator it;
				KeyValues* pSearchInfo;
				if (Q_strcmp(pEntryDataKV->GetString("file"), ""))
				{
					it = m_models.begin();
					pSearchInfo = new KeyValues("search");
					pSearchInfo->SetString("file", pEntryDataKV->GetString("file"));
					pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
					pSearchInfo->deleteThis();
				}

				if (pEntryKV)
					m_networkModelDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");

				// IF NOT FOUND, CREATE IT...
				//if( pEntryKV )// TODO: If model & item merging is turned back on, this is one of the places that mutated models would have to get appended to.
				if (!pEntryKV)
				{
					std::string kvTitle = mode.substr(0, mode.length() - 1);

					pEntryKV = new KeyValues(kvTitle.c_str());

					pEntryKV->SetInt("generation", 3);
					pEntryKV->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
					pEntryKV->SetString("local/info/owner", "local");

					pEntryKV->SetString("local/info/id", goodId.c_str());
					pEntryKV->SetString("local/title", pEntryDataKV->GetString("title"));
					pEntryKV->SetString("local/keywords", pEntryDataKV->GetString("keywords"));
					pEntryKV->SetBool("local/dynamic", pEntryDataKV->GetBool("dynamic"));
					pEntryKV->SetBool("local/screen", pEntryDataKV->GetString("screen"));
					pEntryKV->SetString(VarArgs("local/platforms/%s/id", AA_PLATFORM_ID), AA_PLATFORM_ID);
					pEntryKV->SetString(VarArgs("local/platforms/%s/file", AA_PLATFORM_ID), pEntryDataKV->GetString("file"));
					pEntryKV->SetString(VarArgs("local/platforms/%s/mountIds", AA_PLATFORM_ID), pEntryDataKV->GetString("mountIds"));
					pEntryKV->SetString(VarArgs("local/platforms/%s/workshopIds", AA_PLATFORM_ID), pEntryDataKV->GetString("workshopIds"));
					//pEntryKV->SetString(VarArgs("local/platforms/%s/backpackIds", AA_PLATFORM_ID), buf.c_str());
					pEntryKV->SetString(VarArgs("local/platforms/%s/download", AA_PLATFORM_ID), pEntryDataKV->GetString("download"));
					this->AddModel(pEntryKV);
					m_networkRemoteOnlyModels[goodId] = true;
					this->SaveSQL(null, "models", goodId.c_str(), pEntryKV, false, true);	// so that it's in volatileKV
				}
			}
			else if (mode == "types")
			{
				// FIRST, CHECK FOR TITLE FIELD MATCH...
				std::map<std::string, KeyValues*>::iterator it;
				KeyValues* pSearchInfo;
				if (Q_strcmp(pEntryDataKV->GetString("title"), ""))
				{
					it = m_types.begin();
					pSearchInfo = new KeyValues("search");
					pSearchInfo->SetString("title", pEntryDataKV->GetString("title"));
					pEntryKV = this->FindLibraryEntry(mode.c_str(), pSearchInfo, it);
					pSearchInfo->deleteThis();
				}

				if (pEntryKV)
					m_networkTypeDictionary[goodId] = this->GetActiveKeyValues(pEntryKV)->GetString("info/id");

				// IF NOT FOUND, CREATE IT...
				if (!pEntryKV)
				{
					std::string kvTitle = mode.substr(0, mode.length() - 1);

					pEntryKV = new KeyValues(kvTitle.c_str());

					// update us to 3rd generation
					pEntryKV->SetInt("generation", 3);
					pEntryKV->SetString("local/info/created", VarArgs("%llu", g_pAnarchyManager->GetTimeNumber()));
					pEntryKV->SetString("local/info/owner", "local");
					pEntryKV->SetString("local/info/id", goodId.c_str());

					pEntryKV->SetString("local/fileformat", pEntryDataKV->GetString("fileformat"));
					pEntryKV->SetString("local/title", pEntryDataKV->GetString("title"));
					pEntryKV->SetInt("local/priority", pEntryDataKV->GetInt("priority"));
					pEntryKV->SetString("local/titleformat", pEntryDataKV->GetString("titleformat"));
					this->AddType(pEntryKV);
					m_networkRemoteOnlyTypes[goodId] = true;
					this->SaveSQL(null, "types", goodId.c_str(), pEntryKV, false, true);	// so that it's in volatileKV
				}
			}
		}
	}
	else
	{
		int body = 0;
		int skin = 0;

		Vector origin;
		UTIL_StringToVector(origin.Base(), pEntryDataKV->GetString("origin"));

		QAngle angles;
		UTIL_StringToVector(angles.Base(), pEntryDataKV->GetString("angles"));

		//std::string anim = pEntryDataKV->GetString("anim");

		std::string item = pEntryDataKV->GetString("item");
		auto existingItemIt = m_networkItemDictionary.find(item);
		if (existingItemIt != m_networkItemDictionary.end())
			item = existingItemIt->second;

		std::string model = pEntryDataKV->GetString("model");
		auto existingModelIt = m_networkModelDictionary.find(model);
		if (existingModelIt != m_networkModelDictionary.end())
			model = existingModelIt->second;
		//else if (!g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("models", model) )//model.find(".mdl") == std::string::npos && model.find(".MDL") == std::string::npos )
		//	model = "";
		
		// confirm that the model exists
		//if (g_pFullFileSystem->FileExists(g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("models", model))->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID))))
		std::string modelFile = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("models", model))->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
		//DevMsg("Model file (%s): %s\n", modelFile.c_str());
		if (item != "" || modelFile != "")// || g_pFullFileSystem->FileExists(modelFile.c_str(), "GAME"))
		{
			if ((item == "" || this->GetLibraryEntry("items", item)) && modelFile != "") // && this->GetLibraryEntry("models", model) )
			{
				object_t* pObject = null;
				if (item != "" )
					pObject = g_pAnarchyManager->GetInstanceManager()->AddObject(id, item, model, origin, angles, pEntryDataKV->GetFloat("scale"), pEntryDataKV->GetString("anim"), pEntryDataKV->GetBool("slave"), body, skin, g_pAnarchyManager->GetTimeNumber(), "local", 0U, "", 0U, "", pEntryDataKV->GetBool("child"), -1);
				else
					pObject = g_pAnarchyManager->GetInstanceManager()->AddObject(id, model, model, origin, angles, pEntryDataKV->GetFloat("scale"), pEntryDataKV->GetString("anim"), pEntryDataKV->GetBool("slave"), body, skin, g_pAnarchyManager->GetTimeNumber(), "local", 0U, "", 0U, "", pEntryDataKV->GetBool("child"), -1);
				g_pAnarchyManager->GetInstanceManager()->SpawnObject(pObject, false);

				// HUD notification for loading bar
				C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

				std::vector<std::string> args;
				pHudInstance->DispatchJavaScriptMethod("objectCreatedListener", "objectCreated", args);

				// write it to the KV
				KeyValues* pInstanceKV = g_pAnarchyManager->GetInstanceManager()->GetCurrentInstanceKV();
				KeyValues* pObjectKV = pInstanceKV->FindKey(VarArgs("objects/%s", id.c_str()), true);

				// position
				char buf[AA_MAX_STRING];
				Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->origin.x, pObject->origin.y, pObject->origin.z);
				std::string position = buf;

				// rotation
				Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->angles.x, pObject->angles.y, pObject->angles.z);
				std::string rotation = buf;

				int slave = (pObject->slave) ? 1 : 0;
				int child = (pObject->child) ? 1 : 0;

				g_pAnarchyManager->GetInstanceManager()->CreateObject(pObjectKV, id, pObject->itemId, pObject->modelId, position, rotation, pObject->scale, pObject->anim, slave, child, body, skin);
				g_pAnarchyManager->GetInstanceManager()->SaveActiveInstance(null, true);
			}
		}
	}

	//for (KeyValues *sub = pEntryDataKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	//{
	//	DevMsg("\t%s = %s\n", sub->GetName(), sub->GetString());
	//}

	pEntryDataKV->deleteThis();

	// callback
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");

	std::vector<std::string> args;
	//args.push_back(cvar->FindVar("avatar_url")->GetString());
	pNetworkInstance->DispatchJavaScriptMethod("clientUpdateCallbacker", "giveNextUpdate", args);
}


/*

CASE A: Object
(1) Make sure the object already exists in the local world.
(2) Determine if itemID or modelID are changed.  If they are, unhandled (for now).
(3) Otherwise, update the object fields on the local object.
(4) Fin

// TODO: implement handling of the rest of these cases too:

CASE B: Item
(1) yadda

CASE C: Model
(1) yadda

CASE D: App
(1) yadda

CASE E: Type
*/
/*
void C_MetaverseManager::OnEntryChanged(std::string mode, std::string id, KeyValues* pEntryDataKV)
{
	DevMsg("OnEntryChanged: %s\n", mode.c_str());
	
	// OBSOLETE NOTE? (PROBABLY)
	// The entry *may* have been changed in such a way that it has NEW additional required entries.
	// IF that is the case, then...
		// Entry should NOT be "initialized" until it is either resolved to a LOCAL LIBRARY entry,
		//    OR **ALL** additional required entries *ARE* "initialized" that THIS entry depends on.(1) yadda

	// CASE: Object
	if (mode == "objects")
	{
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(id);
		if (!pObject)
		{
			DevMsg("ERROR: Object not found to update. Skipping.\n");
			return;
		}

		std::string fieldName;
		std::string fieldValue;
		for (KeyValues *sub = pEntryDataKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			fieldName = sub->GetName();
			//fieldValue = sub->GetString();	// FIXME: more types per field?

			if (fieldName == "scale")
			{
				pObject->scale = sub->GetFloat();
				// if entity exists alrieady, scale it now.

			}
			//else if(fieldName == "itemid")
			//	pObject->itemId = sub->GetString();
			//else if (fieldName == "modelId")
			//	pObject->modelId = sub->GetString();
			else if (fieldName == "anim")
				pObject- = ;
			else if (fieldName == "")
				pObject- = ;
			else if (fieldName == "")
				pObject- = ;

			g_pAnarchyManager->GetInstanceManager()->ApplyChanges(pShortcut);

					// update field with value
					active->SetString(field.c_str(), value.c_str());

					// if any of the following fields were changed, the images on the item should be refreshed:
					if (field == "file" || field == "preview" || field == "screen" || field == "marquee")
						bNeedsTextureUpdate = true;
				}

				// now save the item's changes
				//g_pAnarchyManager->GetMetaverseManager()->SaveItem(pItem);
				g_pAnarchyManager->GetMetaverseManager()->SaveSQL(null, "items", id.c_str(), pItem);

				if (bNeedsTextureUpdate)
				{
					//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
					g_pAnarchyManager->GetCanvasManager()->PrepareRefreshItemTextures(id, "ALL");
					g_pAnarchyManager->GetCanvasManager()->RefreshItemTextures(id, "ALL");
					//g_pAnarchyManager->GetCanvasManager()->RefreshItemTextures(id, "screen");
					//g_pAnarchyManager->GetCanvasManager()->RefreshItemTextures(id, "marquee");
					//g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
				}

				//if (g_pAnarchyManager->GetConnectedUniverse() && g_pAnarchyManager->GetConnectedUniverse()->connected)
				//g_pAnarchyManager->GetMetaverseManager()->SendItemUpdate(id);

				return JSValue(true);
			}
		}
	}


	pEntryDataKV->deleteThis();
}
*/

void C_MetaverseManager::ReallyHostNow()
{
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkInstance)
		return;

	std::vector<std::string> args;
	args.push_back(cvar->FindVar("avatar_url")->GetString());
	args.push_back("");
	args.push_back(cvar->FindVar("aamp_server_key")->GetString());
	args.push_back(g_pAnarchyManager->GetInstanceId());
	args.push_back(cvar->FindVar("aamp_client_id")->GetString());
	args.push_back(cvar->FindVar("aamp_client_key")->GetString());
	//const char *buf = steamapicontext->SteamFriends()->GetPersonaName();

	std::string personaName = steamapicontext->SteamFriends()->GetPersonaName();
	if (personaName == "")
		personaName = "Human Player";

	args.push_back(personaName);
	//args.push_back(cvar->FindVar("aamp_display_name")->GetString());

	args.push_back(cvar->FindVar("aamp_lobby_id")->GetString());
	args.push_back(cvar->FindVar("aamp_public")->GetString());
	args.push_back(cvar->FindVar("aamp_persistent")->GetString());
	args.push_back(cvar->FindVar("aamp_lobby_password")->GetString());

	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "hostSession", args);
}

void C_MetaverseManager::SessionEnded()
{
	bool bIsHost = false;
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		bIsHost = true;

	if (bIsHost)
		return;	// TODO: host ends the session differently than clients do (hosts are not kicked out of the map)

	this->DisconnectSession();

	// FIXME: When clients are able to spawn stuff in the future, they'll need to be able to choose to save or discard changes.

	// For now, just always discard changes.
	//g_pAnarchyManager->PerformAutoSave(SAVEMODE_DISCARD);

	/*
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/sessionEnded.html");
	pHudBrowserInstance->Select();
	pHudBrowserInstance->Focus();
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pHudBrowserInstance);
	*/
}


void C_MetaverseManager::DisconnectSession()
{
	bool bIsHost = false;
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		bIsHost = true;

	if (!bIsHost)
	{
		// FIXME: When clients are able to spawn stuff in the future, they'll need to be able to choose to save or discard changes.
		// For now, just always discard changes.
		g_pAnarchyManager->PerformAutoSave(SAVEMODE_DISCARD);
	}

	// Maybe this method should always be called upon ResetNetwork???
	g_pAnarchyManager->ClearConnectedUniverse();

	this->ClearUploadsLog();
}

void C_MetaverseManager::ConnectSession(std::string universe, std::string instance, std::string lobby)
{
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkInstance)
		return;

	// function(avatarUrl, address, universe, instance, user, passcode, displayName, lobby, isPublic, lobbyPassword)

	std::vector<std::string> args;
	args.push_back(cvar->FindVar("avatar_url")->GetString());
	args.push_back("");
	args.push_back(universe);
	//args.push_back(cvar->FindVar("aamp_server_key")->GetString());
	//args.push_back(g_pAnarchyManager->GetInstanceId());
	args.push_back(instance);	// FIXME: unhardcode me
	args.push_back(cvar->FindVar("aamp_client_id")->GetString());
	args.push_back(cvar->FindVar("aamp_client_key")->GetString());
	//const char *buf = steamapicontext->SteamFriends()->GetPersonaName();

	std::string personaName = steamapicontext->SteamFriends()->GetPersonaName();;
	if (personaName == "")
		personaName = "Human Player";

	args.push_back(personaName);
	//args.push_back(cvar->FindVar("aamp_display_name")->GetString());

	args.push_back(lobby);// cvar->FindVar("aamp_lobby_id")->GetString());
	args.push_back(cvar->FindVar("aamp_public")->GetString());	// FIXME: Isn't this the user's LOCAL value?
	args.push_back(cvar->FindVar("aamp_persistent")->GetString());	// FIXME: Isn't this the user's LOCAL value too?
	args.push_back(cvar->FindVar("aamp_lobby_password")->GetString());	// TODO: should be passed the password too for joining.

	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "joinSession", args);
}

void C_MetaverseManager::FetchGameAchievements(std::string appId)
{
	CSteamID sid = steamapicontext->SteamUser()->GetSteamID();
	HTTPRequestHandle requestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, VarArgs("https://api.steampowered.com/ISteamUserStats/GetSchemaForGame/v2/?key=%s&appid=%s", AA_STEAMWORKS_API_KEY, appId.c_str(), sid.ConvertToUint64()));

	SteamAPICall_t hAPICall;
	steamapicontext->SteamHTTP()->SendHTTPRequest(requestHandle, &hAPICall);
	m_FetchGameSchemaCallback.Set(hAPICall, this, &C_MetaverseManager::GameSchemaReceived);
}

void C_MetaverseManager::HostSession()
{
	//DevMsg("Do it from here now!\n");
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkInstance)
		return;

	m_bHasDisconnected = false;

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->AddHudLoadingMessage("", "", "Connecting to server...", "connectingToServer", "", "", "", "");

	if (!Q_strcmp(cvar->FindVar("avatar_url")->GetString(), ""))
	{
		CSteamID sid = steamapicontext->SteamUser()->GetSteamID();
		HTTPRequestHandle requestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, VarArgs("https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?key=%s&steamids=%llu", AA_STEAMWORKS_API_KEY, sid.ConvertToUint64()));

		SteamAPICall_t hAPICall;
		steamapicontext->SteamHTTP()->SendHTTPRequest(requestHandle, &hAPICall);
		m_UserInfoHostingCallback.Set(hAPICall, this, &C_MetaverseManager::UserInfoReceivedHosting);
	}
	else
		this->HostSessionNow();
}

void C_MetaverseManager::ForgetAvatarDeathList()
{
	m_avatarDeathList.clear();
}

void C_MetaverseManager::RestartNetwork(bool bCleanupAvatars)
{
	//DevMsg("Do it from here now!\n");
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkInstance)
		return;

	this->RemoveAllInstanceUsers();

	if (!bCleanupAvatars)
		this->ForgetAvatarDeathList();

	m_bHasDisconnected = false;
	this->DisconnectSession();
	//g_pAnarchyManager->ClearConnectedUniverse();

	m_say = "";
	m_followingUserId = "";
	m_pLocalUser = null;
	pNetworkInstance->SetUrl("asset://ui/network.html");
}

void C_MetaverseManager::SendObjectRemoved(object_t* object)
{
	if (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected)
		return;

	//if (!g_pAnarchyManager->GetConnectedUniverse()->isHost)
	if (!g_pAnarchyManager->GetConnectedUniverse()->canBuild)
		return;

	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	/*
	0 - object (string)
	1 - instance (string)
	2 - item (string)
	3 - model (string)
	4 - slave (int)
	5 - child (int)
	6 - parentObject (string)
	7 - scale (number)
	8 - origin (string)
	9 - angles (string)
	10 - anim (string)
	*/

	std::vector<std::string> args;
	args.push_back(object->objectId);
	args.push_back(g_pAnarchyManager->GetInstanceId());
	args.push_back(object->itemId);
	args.push_back(object->modelId);
	args.push_back(VarArgs("%i", object->slave));
	args.push_back(VarArgs("%i", object->child));

	std::string parentObjectId;
	C_PropShortcutEntity* pParentShortcut = null;
	if (object->parentEntityIndex >= 0)
	{
		pParentShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(object->parentEntityIndex));
		if (pParentShortcut)
			parentObjectId = pParentShortcut->GetObjectId();
	}
	args.push_back(parentObjectId);

	args.push_back(VarArgs("%.10f", object->scale));

	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->origin.x, object->origin.y, object->origin.z);
	std::string origin = buf;
	args.push_back(origin);

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->angles.x, object->angles.y, object->angles.z);
	std::string angles = buf;
	args.push_back(angles);

	args.push_back(object->anim);

	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "localObjectRemove", args);
}

void C_MetaverseManager::SendLocalChatMsg(std::string chatText)
{
	m_say = chatText;
	g_pAnarchyManager->AddToastMessage(VarArgs("You say: %s", chatText.c_str()), true);
}

void C_MetaverseManager::SendSocialChatMsg(std::string chatText)
{
	g_pAnarchyManager->AddToastMessage(VarArgs("(SOCIAL) You say: %s", chatText.c_str()), true);
	this->SocialSay(chatText);
}

void C_MetaverseManager::FetchOnlineCount()
{
	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "socialCount", params);
}

void C_MetaverseManager::ReportSocialCount(int iCount)
{
	C_AwesomiumBrowserInstance* pHudInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud"));
	if (!pHudInstance)
		return;

	std::vector<std::string> params;
	params.push_back(VarArgs("%i", iCount));
	pHudInstance->DispatchJavaScriptMethod("socialListener", "countUpdate", params);
}

void C_MetaverseManager::SocialChatMsg(std::string userId, std::string displayName, std::string text)
{
	if (Q_strcmp(cvar->FindVar("aamp_client_id")->GetString(), userId.c_str()))
		g_pAnarchyManager->AddToastMessage(VarArgs("(SOCIAL) %s says: %s", displayName.c_str(), text.c_str()), true);
}

void C_MetaverseManager::InstanceUserClicked(user_t* pUser)
{
	if (m_followingUserId == pUser->userId)
	{
		// we were already followed
		if (pUser->entity)
			engine->ClientCmd(VarArgs("removehovergloweffect %i", pUser->entity->entindex()));	// servercmdfix , false);

		m_followingUserId = "";
		g_pAnarchyManager->AddToastMessage(VarArgs("Stopped following %s.", pUser->displayName.c_str()));
	}
	else
	{
		// if we are already following a user, unfollow!
		if (m_followingUserId != "")
		{
			user_t* pFollowedUser = this->GetInstanceUser(m_followingUserId);
			if (pFollowedUser)
			{
				if (pFollowedUser->entity)
					engine->ClientCmd(VarArgs("removehovergloweffect %i", pFollowedUser->entity->entindex()));	// servercmdfix , false);
			}
		}

		if (pUser->entity)
			engine->ClientCmd(VarArgs("addhovergloweffect %i", pUser->entity->entindex()));	// servercmdfix, false);

		m_followingUserId = pUser->userId;
		g_pAnarchyManager->AddToastMessage(VarArgs("Started following %s.", pUser->displayName.c_str()));

		this->SyncToUser(pUser->objectId, "", pUser);
	}
}

void C_MetaverseManager::SocialJoin()
{
	if (!g_pAnarchyManager->GetSocialMode())
		return;

	//g_pAnarchyManager->Tester();
	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	//params.push_back("yadda");
	//avatarUrl, club, user, displayName
	params.push_back(cvar->FindVar("avatar_url")->GetString());
	params.push_back("");
	params.push_back(cvar->FindVar("aamp_client_id")->GetString());
	//const char *buf = steamapicontext->SteamFriends()->GetPersonaName();

	std::string personaName = steamapicontext->SteamFriends()->GetPersonaName();
	if (personaName == "")
		personaName = "Human Player";

	params.push_back(personaName);

	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if( pConnection )
		params.push_back(pConnection->lobby);

	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "socialJoin", params);
}

void C_MetaverseManager::SocialLeave()
{
	//if (!g_pAnarchyManager->GetSocialMode())
	//	return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(cvar->FindVar("aamp_client_id")->GetString());
	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "socialLeave", params);
}

void C_MetaverseManager::SocialSay(std::string text)
{
	if (!g_pAnarchyManager->GetSocialMode())
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(cvar->FindVar("aamp_client_id")->GetString());
	params.push_back(text);
	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "socialSay", params);
}

void C_MetaverseManager::OnNetworkReady()
{
	if (!g_pAnarchyManager->GetSocialMode())
		return;

	this->SocialJoin();
}

void C_MetaverseManager::JoinTwitchChannel(std::string channel)
{
	if (!cvar->FindVar("twitch_enabled")->GetBool())
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(m_twitchUsername);
	params.push_back(channel);
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "join", params);
}

void C_MetaverseManager::OpenTwitchConnection()
{
	if (!cvar->FindVar("twitch_enabled")->GetBool() || g_pAnarchyManager->IsPaused() || m_twitchUsername == "" || m_twitchToken == "" || m_twitchToken == "********")
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(m_twitchUsername);
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "open", params);
}

void C_MetaverseManager::CloseTwitchConnection()
{
	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "close", params);
}

void C_MetaverseManager::LeaveTwitchChannel(std::string channel)
{
	if (!cvar->FindVar("twitch_enabled")->GetBool())
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(m_twitchUsername);
	params.push_back(channel);
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "leave", params);
}

void C_MetaverseManager::SetTwitchConfig(std::string username, std::string token, std::string channel)
{
	m_twitchUsername = username;
	if (token != "********")
		m_twitchToken = token;
	m_twitchChannel = channel;

	this->SaveTwitchConfig();
}

bool C_MetaverseManager::IsTwitchBotEnabled()
{
	return m_pTwitchBotEnabledConVar->GetBool();
}
std::string C_MetaverseManager::GetTwitchTokenSafe()
{
	if (m_twitchToken == "")
		return "";
	else
		return "********";
}

void C_MetaverseManager::LoadTwitchConfig()
{
	KeyValues* pTwitchKV = new KeyValues("twitch");
	if (pTwitchKV->LoadFromFile(g_pFullFileSystem, "twitch.txt", "DEFAULT_WRITE_PATH"))
	{
		m_twitchUsername = pTwitchKV->GetString("username");
		m_twitchToken = pTwitchKV->GetString("token");
		m_twitchChannel = pTwitchKV->GetString("channel");
	}
	pTwitchKV->deleteThis();
}

void C_MetaverseManager::SaveTwitchConfig()
{
	KeyValues* pTwitchKV = new KeyValues("twitch");
	pTwitchKV->SetString("username", m_twitchUsername.c_str());
	pTwitchKV->SetString("token", m_twitchToken.c_str());
	pTwitchKV->SetString("channel", m_twitchChannel.c_str());
	pTwitchKV->SaveToFile(g_pFullFileSystem, "twitch.txt", "DEFAULT_WRITE_PATH");
	pTwitchKV->deleteThis();
}

void C_MetaverseManager::GetAllTransfers(JSObject* response)
{
	//m_iNumImagesLoading
	JSArray downloads;
	JSArray downloaded;
	JSArray requested;
	JSArray uploads;

	// DOWNLOADS
	std::string batchId;
	KeyValues* pBatchKV;
	for (unsigned int i = 0; i < m_downloadBatches.size(); i++)
	{
		pBatchKV = m_downloadBatches[i];
		/*batchId = pBatchKV->GetString("id");
		if (batchId.length() < 3)
			continue;
		batchId = batchId.substr(2);*/

		JSObject batch;
		AddSubKeys2(pBatchKV, batch, true);
		downloads.Push(batch);
	}
	response->SetProperty(WSLit("downloads"), downloads);

	// DOWNLOADED
	for (unsigned int i = 0; i < m_downloadedBatches.size(); i++)
	{
		pBatchKV = m_downloadedBatches[i];

		/*
		batchId = pBatchKV->GetString("id");
		if (batchId.length() < 3)
			continue;
		batchId = batchId.substr(2);
		*/

		JSObject batch;
		AddSubKeys2(pBatchKV, batch, true);
		downloaded.Push(batch);
	}
	response->SetProperty(WSLit("downloaded"), downloaded);

	// REQUESTED
	for (unsigned int i = 0; i < m_requestedFiles.size(); i++)
	{
		pBatchKV = m_requestedFiles[i];

		JSObject batch;
		AddSubKeys2(pBatchKV, batch, true);
		requested.Push(batch);
	}
	response->SetProperty(WSLit("requested"), requested);

	// UPLOADS
	for (KeyValues *sub = m_pUploadsLogKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		JSObject batch;
		AddSubKeys2(sub, batch, true);
		batch.SetProperty(WSLit("id"), WSLit(sub->GetName()));
		uploads.Push(batch);
	}
	/*KeyValues* pUploadBatchKV;
	for (int iUploadBatchIndex = m_uploadBatches.FirstInorder(); iUploadBatchIndex != m_uploadBatches.InvalidIndex(); iUploadBatchIndex = m_uploadBatches.NextInorder(iUploadBatchIndex))
	{
		//uploadBatchId = m_uploadBatches.Key(iUploadBatchIndex);
		pUploadBatchKV = m_uploadBatches.Element(iUploadBatchIndex);

		JSObject batch;
		AddSubKeys2(pUploadBatchKV, batch, true);
		uploads.Push(batch);
	}*/
	response->SetProperty(WSLit("uploads"), uploads);
}

void C_MetaverseManager::FindAllMaps(JSObject* response, std::string searchTerm)
{
	JSArray maps;
	std::string searchFile;
	KeyValues* pMapKV;
	bool bFoundOne = false;
	std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
	while (it != m_maps.end())
	{
		pMapKV = this->GetActiveKeyValues(it->second);

		searchFile = pMapKV->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
		std::transform(searchFile.begin(), searchFile.end(), searchFile.begin(), ::tolower);

		if (searchFile.find(searchTerm) != std::string::npos)
		{
			JSObject mapObject;
			AddSubKeys2(pMapKV, mapObject);
			//response->SetProperty(WSLit("map"), mapObject);
			maps.Push(mapObject);
			bFoundOne = true;
		}

		it++;
	}
	response->SetProperty(WSLit("maps"), maps);
}

bool C_MetaverseManager::IsBlacklistedTitleMatch(std::string title)
{
	std::string searchTitle = title;
	std::transform(searchTitle.begin(), searchTitle.end(), searchTitle.begin(), ::tolower);

	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(m_blacklistedTitleMatches, tokens, ", ");
	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == searchTitle)
			return true;
	}

	return false;
}


int C_MetaverseManager::GetLibraryItemsCount()
{
	return m_items.size();
}

int C_MetaverseManager::GetLibraryMapsCount()
{
	return m_maps.size();
}

int C_MetaverseManager::GetLibraryModelsCount()
{
	return m_models.size();
}

void C_MetaverseManager::TwitchAuthenticate()
{
	if (!cvar->FindVar("twitch_enabled")->GetBool())
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(m_twitchUsername);
	params.push_back(m_twitchToken);
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "authenticate", params);
}

void C_MetaverseManager::SendTwitchChat(std::string channel, std::string text)
{
	if (!cvar->FindVar("twitch_enabled")->GetBool())
		return;

	C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
	if (!pNetworkInstance)
		return;

	std::vector<std::string> params;
	params.push_back(m_twitchUsername);
	if (channel != "")
		params.push_back(channel);
	else
		params.push_back(m_twitchChannel);
	params.push_back(text);
	pNetworkInstance->DispatchJavaScriptMethod("aampTwitch", "say", params);
}

void C_MetaverseManager::SocialSetLobby(std::string lobby)
{
	bool bIsSocial = true;
	if (bIsSocial)	// TODO: make this logical
	{
		C_AwesomiumBrowserInstance* pNetworkInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network"));
		if (!pNetworkInstance)
			return;

		std::vector<std::string> params;
		params.push_back(cvar->FindVar("aamp_client_id")->GetString());
		params.push_back(lobby);
		pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "socialSetLobby", params);
	}
}

void C_MetaverseManager::AvatarObjectCreated(int iEntIndex, std::string userId)
{
	C_DynamicProp* pProp = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(iEntIndex));
	if (!pProp)
	{
		DevMsg("ERROR: Could not obtain avatar prop.\n");
		return;
	}

	user_t* pUser = this->GetInstanceUser(userId);
	if (!pUser)
	{
		DevMsg("ERROR: Could not obtain the user for the avatar.\n");
		return;
	}

	if (pUser->entity)
	{
		DevMsg("ERROR: User already has an entity.\n");
		return;
	}

	pUser->entity = pProp;
	///*
	float fPlayerHeight = 60.0f;
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	float fFudgeHeight = pLocalPlayer->GetAbsOrigin().z + fPlayerHeight;
	//fFudgeHeight += pLocalPlayer->EyePosition().z;

	engine->ClientCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pUser->entity->entindex(), pUser->bodyOrigin.x, pUser->bodyOrigin.y, fFudgeHeight, pUser->bodyAngles.x, pUser->bodyAngles.y, pUser->bodyAngles.z));	// servercmdfix , false);
	//*/
}

void C_MetaverseManager::SendEntryUpdate(std::string mode, std::string entryId)
{
	if (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected)
		return;

	if (!g_pAnarchyManager->GetConnectedUniverse()->isHost)
		return;

	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	std::vector<std::string> args;
	if (mode == "Item")
	{
		KeyValues* pEntryKV = this->GetActiveKeyValues(this->GetLibraryItem(entryId));

		if (!pEntryKV)
		{
			DevMsg("FATAL ERROR: Could not get entry to send update for!\n");
			return;
		}

		/*
			0 - mode (string)
			1 - item (string)
			2 - app (string)
			3 - description (string)
			4 - download (string)
			5 - file (string)
			6 - marquee (string)
			7 - preview (string)
			8 - reference (string)
			9 - screen (string)
			10 - stream (string)
			11 - title (string)
			12 - type (string)
		*/

		args.push_back(mode);
		args.push_back(entryId);
		args.push_back(pEntryKV->GetString("app"));
		args.push_back(pEntryKV->GetString("description"));
		args.push_back(pEntryKV->GetString("download"));
		args.push_back(pEntryKV->GetString("file"));
		args.push_back(pEntryKV->GetString("marquee"));
		args.push_back(pEntryKV->GetString("preview"));
		args.push_back(pEntryKV->GetString("reference"));
		args.push_back(pEntryKV->GetString("screen"));
		args.push_back(pEntryKV->GetString("stream"));
		args.push_back(pEntryKV->GetString("title"));
		args.push_back(pEntryKV->GetString("type"));
	}
	else if (mode == "Model")
	{
		KeyValues* pEntryKV = this->GetActiveKeyValues(this->GetLibraryModel(entryId));

		if (!pEntryKV)
		{
			DevMsg("FATAL ERROR: Could not get entry to send update for!\n");
			return;
		}

		/*
			0 - mode
			1 - model (string)
			2 - dynamic (int)
			3 - keywords (string)
			4 - file (string)
			5 - mountIds (string)
			6 - workshopIds (string)
			7 - title (string)
			8 - screen (string)
			9 - preview (string)
			10 - download (string)
		*/

		args.push_back(mode);
		args.push_back(entryId);
		args.push_back(pEntryKV->GetString("dynamic"));
		args.push_back(pEntryKV->GetString("keywords"));
		args.push_back(pEntryKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)));
		args.push_back(pEntryKV->GetString(VarArgs("platforms/%s/mountIds", AA_PLATFORM_ID)));
		args.push_back(pEntryKV->GetString(VarArgs("platforms/%s/workshopIds", AA_PLATFORM_ID)));
		args.push_back(pEntryKV->GetString("title"));
		args.push_back(pEntryKV->GetString("screen"));
		args.push_back(pEntryKV->GetString("preview"));
		args.push_back(pEntryKV->GetString(VarArgs("platforms/%s/download", AA_PLATFORM_ID)));
	}
	else if (mode == "App")
	{
		KeyValues* pEntryKV = this->GetActiveKeyValues(this->GetLibraryApp(entryId));

		if (!pEntryKV)
		{
			DevMsg("FATAL ERROR: Could not get entry to send update for!\n");
			return;
		}

		/*
			0 - mode
			1 - app (string)
			2 - title (string)
			3 - file (string)
			4 - commandFormat (string)
			5 - type (string)
			6 - download (string)
			7 - reference (string)
			8 - screen (string)commandFormat
			9 - description (string)
			10 - filepaths (string)
		*/

		args.push_back(mode);
		args.push_back(entryId);
		args.push_back(pEntryKV->GetString("title"));
		args.push_back(pEntryKV->GetString("file"));
		args.push_back(pEntryKV->GetString("commandformat"));
		args.push_back(pEntryKV->GetString("type"));
		args.push_back(pEntryKV->GetString("download"));
		args.push_back(pEntryKV->GetString("reference"));
		args.push_back(pEntryKV->GetString("screen"));
		args.push_back(pEntryKV->GetString("description"));
		args.push_back(pEntryKV->GetString("filepaths"));
	}
	else if (mode == "Type")
	{
		KeyValues* pEntryKV = this->GetActiveKeyValues(this->GetLibraryType(entryId));

		if (!pEntryKV)
		{
			DevMsg("FATAL ERROR: Could not get entry to send update for!\n");
			return;
		}

		/*
			0 - mode
			1 - type (string)
			2 - fileformat (string)
			3 - titleformat (string)
			4 - title (string)
			5 - priority (string)
		*/

		args.push_back(mode);
		args.push_back(entryId);
		args.push_back(pEntryKV->GetString("fileformat"));
		args.push_back(pEntryKV->GetString("titleformat"));
		args.push_back(pEntryKV->GetString("title"));
		args.push_back(pEntryKV->GetString("priority"));
	}

	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "localEntryUpdate", args);

}

void C_MetaverseManager::SendObjectUpdate(C_PropShortcutEntity* pShortcut)
{
	if (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected)
		return;

	//if (!g_pAnarchyManager->GetConnectedUniverse()->isHost)
	if (!g_pAnarchyManager->GetConnectedUniverse()->canBuild)
		return;
	
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	std::string objectId = pShortcut->GetObjectId();
	object_t* object = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
	if (!object)
	{
		DevMsg("FATAL ERROR: This shortcut has no object data struct here!\n");
		return;
	}

	/*
		0 - object (string)
		1 - instance (string)
		2 - item (string)
		3 - model (string)
		4 - slave (int)
		5 - child (int)
		6 - parentObject (string)
		7 - scale (number)
		8 - origin (string)
		9 - angles (string)
	*/

	std::vector<std::string> args;
	args.push_back(object->objectId);
	args.push_back(g_pAnarchyManager->GetInstanceId());
	args.push_back(object->itemId);
	args.push_back(object->modelId);
	args.push_back(VarArgs("%i", object->slave));
	args.push_back(VarArgs("%i", object->child));

	std::string parentObjectId;
	C_PropShortcutEntity* pParentShortcut = null;
	if (object->parentEntityIndex >= 0)
	{
		pParentShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(object->parentEntityIndex));
		if (pParentShortcut)
			parentObjectId = pParentShortcut->GetObjectId();
	}
	args.push_back(parentObjectId);

	args.push_back(VarArgs("%.10f", object->scale));
	
	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->origin.x, object->origin.y, object->origin.z);
	std::string origin = buf;
	args.push_back(origin);

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", object->angles.x, object->angles.y, object->angles.z);
	std::string angles = buf;
	args.push_back(angles);
	
	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "localObjectUpdate", args);
}

user_t* C_MetaverseManager::GetInstanceUser(std::string userId)
{
	auto it = m_users.find(userId);
	if (it != m_users.end())
		return it->second;

	return null;
}

void C_MetaverseManager::GetAllInstanceUsers(std::vector<user_t*>& users)
{
	auto it = m_users.begin();
	while (it != m_users.end())
	{
		users.push_back(it->second);
		it++;
	}
}

user_t* C_MetaverseManager::FindInstanceUser(C_DynamicProp* pProp)
{
	auto it = m_users.begin();
	while (it != m_users.end())
	{
		if (it->second->entity == pProp)
			return it->second;
		else
			it++;
	}

	return null;
}

void C_MetaverseManager::RemoveInstanceUser(std::string userId)
{
	auto it = m_users.find(userId);
	if (it != m_users.end())
	{
		user_t* pUser = it->second;

		//if (g_pAnarchyManager->GetInputManager()->GetMainMenuMode())
		//	DevMsg("ERROR: Could not remove entity for user because game is paused!\n");
		//else
			this->RemoveInstanceUser(pUser);

		m_users.erase(it);
	}
}

void C_MetaverseManager::RemoveInstanceUser(user_t* pUser)
{
	//DevMsg("Removing %s...\n", pUser->displayName.c_str());

	if (pUser->entity)
	{
		m_avatarDeathList.push_back(pUser->entity);
		pUser->entity = null;
	}

	// check if we have a pending user update.
	auto it = m_pendingUserUpdates.find(pUser->userId);
	if (it != m_pendingUserUpdates.end())
	{
		delete it->second;
		m_pendingUserUpdates.erase(it);
	}

	//CDynamicProp* pProp = pUser->entity;
	////if (pProp)
	//{
	//	engine->ServerCmd(VarArgs("removeobject %i;\n", pProp->entindex()), false);
	//}

	delete pUser;
}

void C_MetaverseManager::RemoveAllInstanceUsers()
{
	std::vector<user_t*> victims;
	auto it = m_users.begin();
	while (it != m_users.end())
	{
		victims.push_back(it->second);
		it++;
	}

	for (unsigned int i = 0; i < victims.size(); i++)
		this->RemoveInstanceUser(victims[i]);

	m_users.clear();
}

unsigned int C_MetaverseManager::GetNumInstanceUsers()
{
	return m_users.size();
}

void C_MetaverseManager::InstanceUserRemoved(std::string userId)
{
	if (m_pDebugDisableMPPlayersConVar->GetBool())
		return;

	user_t* pUser = this->GetInstanceUser(userId);

	if (m_followingUserId == userId)
	{
		m_followingUserId = "";
		g_pAnarchyManager->AddToastMessage(VarArgs("Stopped following %s.", pUser->displayName.c_str()));
	}

	g_pAnarchyManager->AddToastMessage(VarArgs("%s has LEFT the session.", pUser->displayName.c_str()));
	this->RemoveInstanceUser(userId);
}

void C_MetaverseManager::InstanceUserAddedReceived(std::string userId, std::string sessionId, std::string displayName, bool bIsWebUser)
{
	if (m_pDebugDisableMPPlayersConVar->GetBool())
		return;

	// does this user exist?
	user_t* pUser = this->GetInstanceUser(userId);
	if (pUser)
	{
		DevMsg("ERROR: User already exists.\n");
		return;
	}
	else
	{
		user_t* pUser = new user_t();
		//pUser->instanceId = instanceId;
		pUser->sessionId = sessionId;
		pUser->userId = userId;
		pUser->displayName = displayName;
		pUser->isWebUser = bIsWebUser;	// FIXME: ALWAYS true for now.  But it *should* be part of the user's session info.
		pUser->entity = null;
		pUser->sessionId = (g_pAnarchyManager->GetConnectedUniverse()) ? g_pAnarchyManager->GetConnectedUniverse()->session : "";
		pUser->needsEntity = true;

		m_users[userId] = pUser;

		if (!Q_strcmp(cvar->FindVar("aamp_client_id")->GetString(), userId.c_str()))
		{
			m_pLocalUser = pUser;
			pUser->avatarUrl = std::string(cvar->FindVar("avatar_url")->GetString());
		}
		else
			g_pAnarchyManager->AddToastMessage(VarArgs("%s has JOINED the session.", displayName.c_str()));
	}
}

void C_MetaverseManager::SyncToUser(std::string objectId, std::string oldObjectId, user_t* pUser)
{
	if (oldObjectId != "")
	{
		// 1. find any embedded instance that uses the OLD objectId
		// 2. close all matches.  if a match is the currently selected shortcut, de-select it 1st.

		object_t* pOldObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(oldObjectId);
		if (pOldObject)
		{
			std::string oldItemId = pOldObject->itemId;
			std::string oldTabId = "auto" + oldItemId;
			C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(oldTabId);
			if (pEmbeddedInstance)
			{
				// we have found the victim.  the only reason ever to NOT close it right away is if it's the selected entity for us.
				//if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance() == pEmbeddedInstance)
				C_PropShortcutEntity* pOldShortcut = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
				if (pOldShortcut && pOldShortcut->entindex() == pEmbeddedInstance->GetOriginalEntIndex())
				{
					g_pAnarchyManager->DeselectEntity();
				}
				else
					pEmbeddedInstance->Close();
			}
		}
	}

	if (objectId != "")
	{
		// 1. find the object
		// 2. put it on continuous play (without selecting it 1st) :S

		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(objectId);
		if (pObject)
		{
			C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));

			// only if shortcut is an item
			if (pShortcut && pShortcut->GetItemId() != pShortcut->GetModelId() && pShortcut->GetItemId() != "" && pShortcut->GetModelId() != "")
			{
				//g_pAnarchyManager->AttemptSelectEntity(pShortcut);

				// FIXME: This is redundant code.  It's also in the selectEntity method!
				// TODO: Generalize this into a method of g_pAnarchymanager!!

				std::string tabTitle = "auto" + pShortcut->GetItemId();
				C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(tabTitle);// this->GetWebManager()->FindWebTab(tabTitle);
				if (!pEmbeddedInstance)
				{
					std::string itemId = pShortcut->GetItemId();
					KeyValues* item = g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId);
					if (item)
					{
						KeyValues* active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(item);

						bool bDoAutoInspect = true;

						std::string gameFile = "";
						std::string coreFile = "";
						bool bShouldLibretroLaunch = (g_pAnarchyManager->DetermineLibretroCompatible(item, gameFile, coreFile) && g_pAnarchyManager->GetLibretroManager()->GetInstanceCount() == 0);

						// auto-libretro
						if (cvar->FindVar("auto_libretro")->GetBool() && bShouldLibretroLaunch && g_pFullFileSystem->FileExists(gameFile.c_str()))
						{
							C_LibretroInstance* pLibretroInstance = g_pAnarchyManager->GetLibretroManager()->CreateLibretroInstance();
							pLibretroInstance->Init(tabTitle, VarArgs("%s - Libretro", active->GetString("title", "Untitled")), pShortcut->entindex());
							DevMsg("Setting game to: %s\n", gameFile.c_str());
							pLibretroInstance->SetOriginalGame(gameFile);
							pLibretroInstance->SetOriginalItemId(itemId);
							pLibretroInstance->SetOriginalEntIndex(pShortcut->entindex());	// probably NOT needed?? (or maybe so, from here.)
							if (!pLibretroInstance->LoadCore(coreFile))	// FIXME: elegantly revert back to autoInspect if loading the core failed!
								DevMsg("ERROR: Failed to load core: %s\n", coreFile.c_str());
							pEmbeddedInstance = pLibretroInstance;
							bDoAutoInspect = false;
						}

						if (bDoAutoInspect)
						{
							std::string previewURL;
							if (pUser->twitchLive == "1" && pUser->twitchChannel != "")
								previewURL = std::string("https://player.twitch.tv/?channel=") + pUser->twitchChannel.substr(1) + std::string("&parent=twitch.tv");//std::string("http://www.twitch.tv/popout/") + pUser->twitchChannel.substr(1);
							else
								previewURL = g_pAnarchyManager->encodeURIComponent(active->GetString("preview"));

							std::string uri = "file://";
							uri += engine->GetGameDirectory();

							uri += "/resource/ui/html/autoInspectItem.html?imageflags=" + g_pAnarchyManager->GetAutoInspectImageFlags() + "&id=" + g_pAnarchyManager->encodeURIComponent(itemId) + "&title=" + g_pAnarchyManager->encodeURIComponent(active->GetString("title")) + "&screen=" + g_pAnarchyManager->encodeURIComponent(active->GetString("screen")) + "&marquee=" + g_pAnarchyManager->encodeURIComponent(active->GetString("marquee")) + "&preview=" + previewURL + "&reference=" + g_pAnarchyManager->encodeURIComponent(active->GetString("reference")) + "&stream=" + g_pAnarchyManager->encodeURIComponent(active->GetString("stream")) + "&file=" + g_pAnarchyManager->encodeURIComponent(active->GetString("file")) + "&plbehavior=" + std::string(g_pAnarchyManager->GetYouTubePlaylistBehaviorConVar()->GetString()) + "&annotations=" + std::string(g_pAnarchyManager->GetYouTubeAnnotationsConVar()->GetString()) + "&mixes=" + std::string(g_pAnarchyManager->GetYouTubeMixesConVar()->GetString()) + "&related=" + std::string(g_pAnarchyManager->GetYouTubeRelatedConVar()->GetString()) + "&vbehavior=" + std::string(g_pAnarchyManager->GetYouTubeVideoBehaviorConVar()->GetString()) + "&endbehavior=" + std::string(g_pAnarchyManager->GetYouTubeEndBehaviorConVar()->GetString());

							C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
							pSteamBrowserInstance->Init(tabTitle, uri, "Newly selected item...", null, pShortcut->entindex());
							pSteamBrowserInstance->SetOriginalItemId(itemId);	// FIXME: do we need to do this for original entindex too???
							pSteamBrowserInstance->SetOriginalEntIndex(pShortcut->entindex());	// probably NOT needed?? (or maybe so, from here.)

							pEmbeddedInstance = pSteamBrowserInstance;
						}

						if (pEmbeddedInstance)
							g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(pEmbeddedInstance);
					}
				}
				else
					g_pAnarchyManager->GetCanvasManager()->SetDisplayInstance(pEmbeddedInstance);
			}
		}
	}
}

user_update_t* C_MetaverseManager::FindPendingUserUpdate(std::string userId)
{
	auto it = m_pendingUserUpdates.find(userId);
	if (it != m_pendingUserUpdates.end())
		return it->second;

	return null;
}

object_update_t* C_MetaverseManager::FindPendingObjectUpdate(std::string objectId)
{
	auto it = m_pendingObjectUpdates.find(objectId);
	if (it != m_pendingObjectUpdates.end())
		return it->second;

	return null;
}

void C_MetaverseManager::UserSessionUpdated(int iUpdateMask, std::string userId, std::string sessionId, std::string displayName, std::string itemId, std::string objectId, std::string say, std::string bodyOrigin, std::string bodyAngles, std::string headOrigin, std::string headAngles, std::string mouseX, std::string mouseY, std::string webUrl, std::string avatarUrl, std::string state, std::string launched, std::string twitchChannel, std::string twitchLive)
{
	if (m_pDebugDisableMPPlayersConVar->GetBool())
		return;
	//user_t* pUser = this->GetInstanceUser(userId);
	//if (!pUser)
	//{
		// FIXME: This should be handled very differently after pendingUserUpdates is fully implemented!  User joined / exited messages would be nice to just be special regular session updates in that case, and ALL user session events could be added to the pending list instead.  User IDs will remain constant between user sessions, even if they leave & rejoin.




		/*
		// TODO: This should call UserJoined, and the InstanceUserAdded should do absolutely nothing.
		DevMsg("User session updated for user not yet added to the session: hasOrigin(%i) hasAngles(%i)\n", iUpdateMask & 0x40, iUpdateMask & 0x80);

		pUser = new user_t();
		//pUser->instanceId = instanceId;
		pUser->sessionId = sessionId;
		pUser->userId = userId;
		pUser->displayName = displayName;
		pUser->entity = null;
		pUser->needsEntity = true;

		m_users[userId] = pUser;
		*/
		//g_pAnarchyManager->AddToastMessage(VarArgs("%s has JOINED the session.", displayName.c_str()));

	//	return;	// just return, for now.
	//}

	// find the pending update for this user, if one exists.
	bool bIsPendingUpdate = true;
	user_update_t* pUserUpdate = this->FindPendingUserUpdate(userId);
	if (!pUserUpdate)
	{
		bIsPendingUpdate = false;
		pUserUpdate = new user_update_t();	// create a new update for this user if one doesn't exist already.
	}

	// populate our update
	if (iUpdateMask & 0x1)
		pUserUpdate->userId = userId;

	if (iUpdateMask & 0x2)
		pUserUpdate->sessionId = sessionId;

	if (iUpdateMask & 0x4)
		pUserUpdate->displayName = displayName;

	if (iUpdateMask & 0x8)
		pUserUpdate->itemId = itemId;

	if (iUpdateMask & 0x10)
		pUserUpdate->objectId = objectId;

	if (iUpdateMask & 0x20)
		pUserUpdate->say = say;

	if (iUpdateMask & 0x40)
		pUserUpdate->bodyOrigin = bodyOrigin;

	if (iUpdateMask & 0x80)
		pUserUpdate->bodyAngles = bodyAngles;

	if (iUpdateMask & 0x100)
		pUserUpdate->headOrigin = headOrigin;

	if (iUpdateMask & 0x200)
		pUserUpdate->headAngles = headAngles;

	if (iUpdateMask & 0x400)
		pUserUpdate->mouseX = mouseX;

	if (iUpdateMask & 0x800)
		pUserUpdate->mouseY = mouseY;

	if (iUpdateMask & 0x1000)
		pUserUpdate->webUrl = webUrl;

	if (iUpdateMask & 0x2000)
		pUserUpdate->avatarUrl = avatarUrl;

	if (iUpdateMask & 0x4000)
		pUserUpdate->state = state;

	if (iUpdateMask & 0x8000)
		pUserUpdate->launched = launched;

	if (iUpdateMask & 0x10000)
		pUserUpdate->twitchChannel = twitchChannel;

	if (iUpdateMask & 0x20000)
		pUserUpdate->twitchLive = twitchLive;

	// set/update our update mask
	pUserUpdate->updateMask |= iUpdateMask;

	// now we have a valid user update to process (if we are allowed)
	if (!bIsPendingUpdate && !engine->IsPaused() && !g_pAnarchyManager->IsPaused() && !g_pAnarchyManager->GetInputManager()->GetMainMenuMode() && this->m_pendingUserUpdates.size() == 0)
		this->ProcessUserSessionUpdate(pUserUpdate);
	else
		m_pendingUserUpdates[userId] = pUserUpdate;
}

bool C_MetaverseManager::ProcessObjectUpdate(object_update_t* pObjectUpdate)
{

	if (g_pAnarchyManager->IsPaused() || engine->IsPaused() || !g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || g_pAnarchyManager->GetConnectedUniverse()->isHost)
		return false;

	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pObjectUpdate->objectId);
	if (!pObject)
	{
		DevMsg("ERROR: Object NOT found for update processing: %s\n", pObjectUpdate->objectId.c_str());
		delete pObjectUpdate;
		return false;
	}

	/*
		"item": 0x1,
		"model": 0x2,
		"slave": 0x4,
		"child": 0x8,
		"parentObject": 0x10,
		"scale": 0x20,
		"origin": 0x40,
		"angles": 0x80,
		"anim": 0x100
	*/

	/*
	if (pUserUpdate->updateMask & 0x2)
	{
	//if (pUserUpdate->say != "")
	//g_pAnarchyManager->AddToastMessage(VarArgs("%s says: %s", pUser->displayName.c_str(), pUserUpdate->say.c_str()));
	pUser->sessionId = pUserUpdate->sessionId;
	DevMsg("Session ID changed to: %s\n", pUser->sessionId.c_str());	// session was always "undefined" when this code block was turned on anyways.
	}
	*/


	// 0x1 && 0x2 flags w/ blank item && blank model = object has been removed!!
	bool bIsDying = (pObjectUpdate->updateMask & 0x1 && pObjectUpdate->updateMask & 0x2 && pObjectUpdate->itemId == "" && pObjectUpdate->modelId == "" && pObjectUpdate->objectId != "");

	C_PropShortcutEntity* pShortcut = null;
	C_BaseEntity* pEntity = (pObject->spawned) ? C_BaseEntity::Instance(pObject->entityIndex) : null;
	if (pEntity)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);

	if (bIsDying)
	{
		if (pShortcut)
		{
			if (g_pAnarchyManager->GetSelectedEntity() == pShortcut)
				g_pAnarchyManager->DeselectEntity();

			g_pAnarchyManager->GetInstanceManager()->RemoveEntity(pShortcut);
		}
		else
			g_pAnarchyManager->GetInstanceManager()->RemoveEntity(null, false, pObjectUpdate->objectId);
	}
	else
	{
		if (pObjectUpdate->updateMask & 0x2)
		{
			pObject->modelId = pObjectUpdate->modelId;
			g_pAnarchyManager->GetInstanceManager()->ApplyChanges(null, true, pObjectUpdate->objectId);

			if (pShortcut)
			{
				KeyValues* pModelKV = this->GetActiveKeyValues(this->GetLibraryModel(pObjectUpdate->modelId));
				if (pModelKV)
				{
					std::string modelFile = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
					DevMsg("Changing to model file %s\n", modelFile.c_str());
					g_pAnarchyManager->GetInstanceManager()->ChangeModel(pShortcut, pObjectUpdate->modelId, modelFile, false);// ->ChangeModel(pBaseEntity, modelId, modelFile, false);
				}
			}
		}

		if (pObjectUpdate->updateMask & 0x4)
		{
			pObject->slave = pObjectUpdate->slave;

			if (pShortcut)
				g_pAnarchyManager->SetSlaveScreen(pObjectUpdate->objectId, pObjectUpdate->slave);	// object->slave is set here too, as well as ApplyChanges
		}

		if (pObjectUpdate->updateMask & 0x20)
		{
			pObject->scale = pObjectUpdate->scale;

			if (pShortcut)
				g_pAnarchyManager->GetMetaverseManager()->SetObjectScale(pShortcut, pObjectUpdate->scale);
		}

		bool bNeedsTransformUpdate = false;
		if (pObjectUpdate->updateMask & 0x40)
		{
			if (pObjectUpdate->origin != "")
			{
				UTIL_StringToVector(pObject->origin.Base(), pObjectUpdate->origin.c_str());
				bNeedsTransformUpdate = true;
			}
		}

		if (pObjectUpdate->updateMask & 0x80)
		{
			if (pObjectUpdate->angles != "")
			{
				UTIL_StringToVector(pObject->angles.Base(), pObjectUpdate->angles.c_str());
				bNeedsTransformUpdate = true;
			}
		}

		if (pObjectUpdate->updateMask & 0x100)
		{
			pObject->anim = pObjectUpdate->anim;

			// try disabling this in an attempt to get sequences to play for clients
			if (pShortcut)
				g_pAnarchyManager->SetModelSequence(pShortcut, pObjectUpdate->anim);
		}

		if (bNeedsTransformUpdate && pShortcut)
		{
			g_pAnarchyManager->GetInstanceManager()->ApplyChanges(null, true, pObjectUpdate->objectId);
			engine->ClientCmd(VarArgs("setcabpos %i %f %f %f %f %f %f \"%s\";\n", pShortcut->entindex(), pObject->origin.x, pObject->origin.y, pObject->origin.z, pObject->angles.x, pObject->angles.y, pObject->angles.z, ""));	// servercmdfix, false);


			// write it to the KV
			KeyValues* pInstanceKV = g_pAnarchyManager->GetInstanceManager()->GetCurrentInstanceKV();
			KeyValues* pObjectKV = pInstanceKV->FindKey(VarArgs("objects/%s", pObject->objectId.c_str()), true);

			// position
			char buf[AA_MAX_STRING];
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->origin.x, pObject->origin.y, pObject->origin.z);
			std::string position = buf;

			// rotation
			Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pObject->angles.x, pObject->angles.y, pObject->angles.z);
			std::string rotation = buf;

			int slave = (pObject->slave) ? 1 : 0;
			int child = (pObject->child) ? 1 : 0;
			int body = pObject->body;
			int skin = pObject->skin;

			g_pAnarchyManager->GetInstanceManager()->CreateObject(pObjectKV, pObject->objectId, pObject->itemId, pObject->modelId, position, rotation, pObject->scale, pObject->anim, slave, child, body, skin);
			g_pAnarchyManager->GetInstanceManager()->SaveActiveInstance(null, true);
		}
	}

	delete pObjectUpdate;	// after an update is processed, it is dead.
	return true;
}
bool C_MetaverseManager::ProcessUserSessionUpdate(user_update_t* pUserUpdate)
{
	if (g_pAnarchyManager->IsPaused() || engine->IsPaused() )//g_pAnarchyManager->GetInputManager()->GetMainMenuMode())
	{
		//DevMsg("Mark\n");
		//DevMsg("ERROR: ProcessUserSessionUpdate called while AArcade was paused!!\n");
		//delete pUserUpdate;
		return false;
	}

	user_t* pUser = this->GetInstanceUser(pUserUpdate->userId);
	if (!pUser)
	{
		DevMsg("ERROR: User NOT found for session update processing: %s\n", pUserUpdate->userId.c_str());
		//delete pUserUpdate;
		return false;
	}

	/*
	if (pUserUpdate->updateMask & 0x2)
	{
		//if (pUserUpdate->say != "")
			//g_pAnarchyManager->AddToastMessage(VarArgs("%s says: %s", pUser->displayName.c_str(), pUserUpdate->say.c_str()));
		pUser->sessionId = pUserUpdate->sessionId;
		DevMsg("Session ID changed to: %s\n", pUser->sessionId.c_str());	// session was always "undefined" when this code block was turned on anyways.
	}
	*/

	if (pUserUpdate->updateMask & 0x20)
	{
		if (pUserUpdate->say != "")
			g_pAnarchyManager->AddToastMessage(VarArgs("%s says: %s", pUser->displayName.c_str(), pUserUpdate->say.c_str()), true);

		pUser->say = pUserUpdate->say;
	}

	if (pUserUpdate->updateMask & 0x40 || pUserUpdate->updateMask & 0x80 || pUserUpdate->updateMask & 0x100 || pUserUpdate->updateMask & 0x200)
	{
		if (pUserUpdate->bodyOrigin != "")
			UTIL_StringToVector(pUser->bodyOrigin.Base(), pUserUpdate->bodyOrigin.c_str());

		if (pUserUpdate->bodyAngles != "")
			UTIL_StringToVector(pUser->bodyAngles.Base(), pUserUpdate->bodyAngles.c_str());

		if (pUserUpdate->headOrigin != "")
			UTIL_StringToVector(pUser->headOrigin.Base(), pUserUpdate->headOrigin.c_str());

		if (pUserUpdate->headAngles != "")
			UTIL_StringToVector(pUser->headAngles.Base(), pUserUpdate->headAngles.c_str());

		if (pUser->entity)
		{
			float fPlayerHeight = 60.0f;
			C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();


			float z;
			// FIXME: Simply checking if bodyorigin is equal to 0 is NOT enough to be sure it's a web client! (sometimes 0 z is correct, after all.)
			if (pUser->bodyOrigin.z == 0)
			{
				float fFudgeHeight = pLocalPlayer->GetAbsOrigin().z + fPlayerHeight;
				z = fFudgeHeight;// localPlayerOrigin.z;//pUser->bodyOrigin.z
			}
			else
				z = pUser->bodyOrigin.z + fPlayerHeight;

			//fFudgeHeight += pLocalPlayer->EyePosition().z;

			engine->ClientCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pUser->entity->entindex(), pUser->bodyOrigin.x, pUser->bodyOrigin.y, z, pUser->headAngles.x, pUser->headAngles.y, pUser->headAngles.z));	// servercmdfix , false);
			//engine->ServerCmd(VarArgs("set_object_pos %i %f %f %f %f %f %f;\n", pUser->entity->entindex(), pUser->bodyOrigin.x, pUser->bodyOrigin.y, z, pUser->bodyAngles.x, pUser->bodyAngles.y, pUser->bodyAngles.z), false);
		}
		else if (pUser->needsEntity)
		{
			pUser->needsEntity = false;

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
			std::vector<std::string> modelNames;
			modelNames.push_back("models/players/tube0.mdl");
			/*
			modelNames.push_back("models/players/heads/cowboycarl.mdl");
			modelNames.push_back("models/players/heads/flipflopfred.mdl");
			modelNames.push_back("models/players/heads/hackerhaley.mdl");
			modelNames.push_back("models/players/heads/ninjanancy.mdl");
			modelNames.push_back("models/players/heads/zombiejoe.mdl");
			*/

			unsigned int index = rand() % modelNames.size();	// non-uniform, but who cares :S
			std::string modelName = modelNames[index];

			float x = pUser->bodyOrigin.x;
			float y = pUser->bodyOrigin.y;
			float z;
			float fPlayerHeight = 60.0f;
			// FIXME: Simply checking if bodyorigin is equal to 0 is NOT enough to be sure it's a web client! (sometimes 0 z is correct, after all.)
			if (pUser->bodyOrigin.z == 0)
			{
				C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
				Vector localPlayerOrigin = pLocalPlayer->GetAbsOrigin();
				float fFudgeHeight = localPlayerOrigin.z + fPlayerHeight;
				z = fFudgeHeight;// localPlayerOrigin.z;//pUser->bodyOrigin.z
			}
			else
				z = pUser->bodyOrigin.z + fPlayerHeight;
			//userSessionUpdated

			// FIXME: Need to make this use same logic as choosing a player spawn point, in case the local player is inside of a wall when a new player joins.

			engine->ClientCmd(VarArgs("create_avatar_object \"%s\" %f %f %f %f %f %f \"%s\";\n", modelName.c_str(), x, y, z, pUser->headAngles.x, pUser->headAngles.y, pUser->headAngles.z, pUserUpdate->userId.c_str()));// , false);
			//engine->ServerCmd(VarArgs("create_avatar_object \"%s\" %f %f %f %f %f %f \"%s\";\n", modelName.c_str(), x, y, z, pUser->bodyAngles.x, pUser->bodyAngles.y, pUser->bodyAngles.z, pUserUpdate->userId.c_str()), false);
			//g_pAnarchyManager->AddToastMessage(VarArgs("%s has JOINED the session.", pUser->displayName.c_str()));
		}
		// else there's no entity to do anything with yet.
	}

	if (pUserUpdate->updateMask & 0x4 && pUserUpdate->displayName != pUser->displayName && pUserUpdate->displayName != "" )	// FIXME: We probably don't need to include this with every update anymore. :)
	{
		std::string goodName = (pUserUpdate->displayName != "") ? pUserUpdate->displayName : "Human Player";
		pUser->displayName = goodName;

		g_pAnarchyManager->AddToastMessage(VarArgs("%s changed their name to %s.", pUser->displayName.c_str(), goodName.c_str()));
	}

	if (pUserUpdate->updateMask & 0x10)
	{
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pUserUpdate->objectId);
		if (pObject)
		{
			KeyValues* pItemKV = this->GetActiveKeyValues(this->GetLibraryItem(pObject->itemId));
			if (pItemKV)
			{
				std::string msg = VarArgs("%s tuned into %s", pUser->displayName.c_str(), pItemKV->GetString("title"));
				g_pAnarchyManager->AddToastMessage(msg);
			}
		}

		if (m_followingUserId == pUser->userId)
			this->SyncToUser(pUserUpdate->objectId, pUser->objectId, pUser);

		pUser->objectId = pUserUpdate->objectId;
	}

	if (pUserUpdate->updateMask & 0x2000 && pUserUpdate->avatarUrl != "")
	{
		pUser->avatarUrl = pUserUpdate->avatarUrl;
	}

	if (pUserUpdate->updateMask & 0x4000 && pUserUpdate->state != "")
	{
		pUser->state = pUserUpdate->state;
	}

	if (pUserUpdate->updateMask & 0x8000 && pUserUpdate->launched != "")
	{
		pUser->launched = pUserUpdate->launched;
	}

	if (pUserUpdate->updateMask & 0x10000 && pUserUpdate->twitchChannel != "")
	{
		pUser->twitchChannel = pUserUpdate->twitchChannel;
	}

	if (pUserUpdate->updateMask & 0x20000 && pUserUpdate->twitchLive != "")
	{
		pUser->twitchLive = pUserUpdate->twitchLive;
	}

	/*
	var maskMap = {
	"userId": 0x1,
	"sessionId": 0x2,
	"displayName": 0x4,
	"item": 0x8,
	"object": 0x10,
	"say": 0x20,
	"bodyOrigin": 0x40,
	"bodyAngles": 0x80,
	"headOrigin": 0x100,
	"headAngles": 0x200,
	"mouseX": 0x400,
	"mouseY": 0x800,
	"web": 0x1000,
	"avatar": 0x2000,
	"state": 0x4000,
	"launched": 0x8000,
	"twitchChannel": 0x10000,
	"twitchLive": 0x20000
	};
	*/

	delete pUserUpdate;	// after an update is processed, it is dead.
	return true;
}

void C_MetaverseManager::SendChangeInstanceNotification(std::string instanceId, std::string map)
{
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	std::vector<std::string> args;
	args.push_back(instanceId);
	args.push_back(map);
	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "localUserChangeInstance", args);
}

void C_MetaverseManager::BanSessionUser(std::string userId)
{
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	std::vector<std::string> args;
	args.push_back(userId);
	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "banUser", args);
}

void C_MetaverseManager::UnbanSessionUser(std::string userId)
{
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	std::vector<std::string> args;
	args.push_back(userId);
	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "unbanUser", args);
}

//#include "../public/bitmap/tgaloader.h"

void C_MetaverseManager::OverviewExtracted()
{
	DevMsg("Overview extracted!!\n");
	this->ReallyHostNow();
	/*
	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	KeyValues* pOverviewKV = new KeyValues("overview");
	if (pOverviewKV->LoadFromFile(g_pFullFileSystem, VarArgs("resource/overviews/%s.txt", g_pAnarchyManager->MapName()), "GAME"))
	{
		std::vector<std::string> args;
		args.push_back(VarArgs("%s.tga", g_pAnarchyManager->MapName()));
		args.push_back(VarArgs("%i", pOverviewKV->GetInt("pos_x")));
		args.push_back(VarArgs("%i", pOverviewKV->GetInt("pos_y")));
		args.push_back(VarArgs("%.10f", pOverviewKV->GetFloat("scale")));

		pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "syncOverview", args);
	}
	*/
}

void C_MetaverseManager::ExtractOverviewTGA()
{
	g_pAnarchyManager->AddToastMessage("Extracting map overview VTF and converting...");

	std::string mapName = g_pAnarchyManager->MapName();
	//std::string textureName = "overviews/" + mapName;

	//ITexture* pTexture = g_pMaterialSystem->FindTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, false, 1);

	//std::string goodTextureName = pTexture->GetName();

	bool bShouldHostNow = true;

	// read the screenshot
	FileHandle_t fh = filesystem->Open(VarArgs("materials/overviews/%s.vtf", mapName.c_str()), "rb", "GAME");
	if (fh)
	{
		int file_len = filesystem->Size(fh);
		unsigned char* pImageData = new unsigned char[file_len + 1];

		filesystem->Read((void*)pImageData, file_len, fh);
		pImageData[file_len] = 0; // null terminator

		filesystem->Close(fh);

		// write the screenshot
		// ORDER: FORWARD, RIGHT, BACK, LEFT, BOTTOM, TOP
		//FileHandle_t fh2 = filesystem->Open(VarArgs("screenshots/panoramic/pano/%s.jpg", directions[i].c_str()), "wb", "DEFAULT_WRITE_PATH");

		g_pFullFileSystem->CreateDirHierarchy("screenshots/overviews", "DEFAULT_WRITE_PATH");

		//std::string textureFile = VarArgs("screenshots/overviews/%s.vtf", mapName.c_str());
		FileHandle_t fh2 = filesystem->Open(VarArgs("screenshots/overviews/%s.vtf", mapName.c_str()), "wb", "DEFAULT_WRITE_PATH");
		if (fh2)
		{
			filesystem->Write(pImageData, file_len, fh2);
			filesystem->Close(fh2);

			// cleanup
			delete[] pImageData;

			std::string toolsFolder = g_pAnarchyManager->GetAArcadeToolsFolder();
			std::string userFolder = g_pAnarchyManager->GetAArcadeUserFolder();

			/*
			std::string command = VarArgs("\"%s\\vtf2tga.exe\" -i \"%s\\screenshots\\overviews\\%s.vtf\" -o \"%s\\screenshots\\overviews\\%s.tga\"", toolsFolder.c_str(), userFolder.c_str(), mapName.c_str(), userFolder.c_str(), mapName.c_str());
			DevMsg("%s\n", command.c_str());
			system(command.c_str());
			*/

			FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");
			if (launch_file)
			{
				std::string executable = VarArgs("%s\\vtf2tga.exe", toolsFolder.c_str());
				std::string goodExecutable = "\"" + executable + "\"";
				filesystem->FPrintf(launch_file, "%s:\n", goodExecutable.substr(1, 1).c_str());
				filesystem->FPrintf(launch_file, "cd \"%s\"\n", goodExecutable.substr(1, goodExecutable.find_last_of("/\\", goodExecutable.find("\"", 1)) - 1).c_str());
				filesystem->FPrintf(launch_file, "START \"Launching item...\" %s -i \"%s\\screenshots\\overviews\\%s.vtf\" -o \"%s\\screenshots\\overviews\\%s.tga\"", goodExecutable.c_str(), userFolder.c_str(), mapName.c_str(), userFolder.c_str(), mapName.c_str());
				filesystem->Close(launch_file);
				system("Arcade_Launcher.bat");

				g_pAnarchyManager->WaitForOverviewExtract();
				bShouldHostNow = false;

				/*
				std::string masterCommands = VarArgs(" -i \"%s\\screenshots\\overviews\\%s.vtf\" -o \"%s\\screenshots\\overviews\\%s.tga\"", userFolder.c_str(), mapName.c_str(), userFolder.c_str(), mapName.c_str());

				char pCommands[AA_MAX_STRING];
				Q_strcpy(pCommands, masterCommands.c_str());

				// start the program up
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				// set the size of the structures
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(VarArgs("%s\\vtf2tga.exe", toolsFolder.c_str()),   // the path
				pCommands,        // Command line
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				0,//CREATE_DEFAULT_ERROR_MODE,              //0 // No creation flags
				NULL,           // Use parent's environment block
				VarArgs("%s", toolsFolder.c_str()),           // Use parent's starting directory
				&si,            // Pointer to STARTUPINFO structure
				&pi);
				*/







				//Error("Usage: vtf2tga -i <input vtf> [-o <output tga>] [-mip]\n");
				//g_pFullFileSystem->RemoveFile(VarArgs("screenshots/%s", panoshots[i].c_str()), "DEFAULT_WRITE_PATH");
			}
		}
	}

	if (bShouldHostNow)
		this->ReallyHostNow();

	//g_pFullFileSystem->Save

	//FileHandle_t* pFile = g_pFullFileSystem->Open
	//g_pFullFileSystem->FindFirstEx(VarArgs("materials/%s", textureName.c_str()), "GAME", 
	//std::string textureName = "canvas_hud";// "maps/" + mapName;
	//textureName += "/c-1344_1152_112";

//	ITexture* pTexture = g_pMaterialSystem->FindTexture(textureName.c_str(), TEXTURE_GROUP_VGUI, false, 1);
	//pTexture->Download();

	//IVTFTexture* pVTFTexture = CreateVTFTexture();
	//pVTFTexture->Init()
	//pVTFTexture->
	//IVTFTexture::ImageData()
	//int numRes1 = pTexture->GetResourceTypes(NULL, 0);
	//pTexture->GetResourceData()
	//DevMsg("Texture Name: %s\n", pTexture->GetName());

	/*
	int iWidth = pTexture->GetActualWidth();
	int iHeight = pTexture->GetActualHeight();
	int iDepth = pTexture->GetActualDepth();
	ImageFormat format = pTexture->GetImageFormat();
	int size = ImageLoader::GetMemRequired(iWidth, iHeight, iDepth, format, false);
	ImageLoader::Load(&imageData, "D:\Projects\AArcade-Source\game\aarcade_user\crap\overviews\de_dust2.vtf", iWidth, iHeight, format, 1.0, false);
	*/

	/*
	int iInfoWidth = 0;
	int iInfoHeight = 0;
	ImageFormat infoFormat;
	float fGamma;
	if (TGALoader::GetInfo(textureName.c_str(), &iInfoWidth, &iInfoHeight, &infoFormat, &fGamma))
		DevMsg("Fetched stuff is: %i %i\n", iInfoWidth, iInfoHeight);
	else
		DevMsg("Could not fetch.\n");

	//clientdll->WriteSaveGameScreenshotOfSize("testerJoint.jpg", iWidth, iHeight, false, false);
	//#include "../public/bitmap/tgawriter.h"
	//TGAWriter::WriteToBuffer()
	DevMsg("Texture Loaded: %i\n", g_pMaterialSystem->IsTextureLoaded(textureName.c_str()));

	DevMsg("Width: %i\n", pTexture->GetActualWidth());

	size_t byteSize = 0;
	void* result = pTexture->GetResourceData(1, &byteSize);
	DevMsg("Resource Data Gotten: %i (%u)\n", (int)(result != null), byteSize);
	*/

}

//void C_MetaverseManager::OverviewSyncComplete()
//{
//	this->ReallyHostNow();
	/*
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	std::vector<std::string> args;
	pHudBrowserInstance->DispatchJavaScriptMethod("syncListener", "overviewSyncComplete", args);
	*/
//}

void C_MetaverseManager::PanoSyncComplete(std::string cachedPanoName, std::string panoId)
{
	//PanoSyncComplete
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	std::vector<std::string> args;
	args.push_back(cachedPanoName);
	args.push_back(panoId);
	pHudBrowserInstance->DispatchJavaScriptMethod("syncListener", "panoSyncComplete", args);
}

void C_MetaverseManager::SyncPano()
{
	if (!g_pAnarchyManager->GetConnectedUniverse() || !g_pAnarchyManager->GetConnectedUniverse()->connected || !g_pAnarchyManager->GetConnectedUniverse()->isHost )
		return;

	C_AwesomiumBrowserInstance* pNetworkBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkBrowserInstance)
		return;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	QAngle playerBodyAngles = pPlayer->GetAbsAngles();
	Vector playerBodyOrigin = pPlayer->GetAbsOrigin();

	char buf[AA_MAX_STRING];
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerBodyOrigin.x, playerBodyOrigin.y, playerBodyOrigin.z);
	std::string bodyOrigin = buf;

	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", playerBodyAngles.x, playerBodyAngles.y, playerBodyAngles.z);
	std::string bodyAngles = buf;

	std::vector<std::string> args;
	args.push_back(bodyOrigin);
	args.push_back(bodyAngles);
	args.push_back(VarArgs("%i", ScreenWidth()));
	args.push_back(VarArgs("%i", ScreenHeight()));
	pNetworkBrowserInstance->DispatchJavaScriptMethod("aampNetwork", "syncPano", args);
}

void C_MetaverseManager::ObjectRemoved(std::string objectId)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		return;

	this->ObjectUpdated(false, false, 0x3, objectId, "", "", false, false, "", 1.0, "", "", "");
}

void C_MetaverseManager::ObjectUpdated(bool bIsLocalUserUpdate, bool bIsFreshObject, unsigned int iUpdateMask, std::string id, std::string item, std::string model, bool bSlave, bool bChild, std::string parentObject, float fScale, std::string in_origin, std::string in_angles, std::string anim)
{
	/* maskMap
			"item": 0x1,
			"model": 0x2,
			"slave": 0x4,
			"child": 0x8,
			"parentObject": 0x10,
			"scale": 0x20,
			"origin": 0x40,
			"angles": 0x80,
			"anim": 0x100
	*/

	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		return;

	/*
	if (bIsLocalUserUpdate)
	{
		if (bIsFreshObject)
		{
			DevMsg("Update for existing starter object from local user\n");
		}
		else
			DevMsg("Update for existing object from local user\n");

		return;
	}
	*/

	// find the pending update for this object, if one exists.
	bool bIsPendingUpdate = true;
	object_update_t* pObjectUpdate = this->FindPendingObjectUpdate(id);
	if (!pObjectUpdate)
	{
		bIsPendingUpdate = false;
		pObjectUpdate = new object_update_t();	// create a new update for this object if one doesn't exist already.
	}

	pObjectUpdate->objectId = id;

	// populate our update
	if (iUpdateMask & 0x1)
		pObjectUpdate->itemId = item;

	if (iUpdateMask & 0x2)
		pObjectUpdate->modelId = model;

	if (iUpdateMask & 0x4)
		pObjectUpdate->slave = bSlave;

	//if (iUpdateMask & 0x8)
	//	pObjectUpdate->child = bChild;

	//if (iUpdateMask & 0x10)
	//	pObjectUpdate->parentObject = parentObject;

	if (iUpdateMask & 0x20)
		pObjectUpdate->scale = fScale;

	if (iUpdateMask & 0x40)
		pObjectUpdate->origin = in_origin;

	if (iUpdateMask & 0x80)
		pObjectUpdate->angles = in_angles;

	if (iUpdateMask & 0x100)
		pObjectUpdate->anim = anim;

	// set/update our update mask
	pObjectUpdate->updateMask |= iUpdateMask;

	// now we have a valid object update to process (if we are allowed)
	if (!bIsPendingUpdate && !engine->IsPaused() && !g_pAnarchyManager->IsPaused() && !g_pAnarchyManager->GetInputManager()->GetMainMenuMode() && this->m_pendingObjectUpdates.size() == 0)
		this->ProcessObjectUpdate(pObjectUpdate);
	else
		m_pendingObjectUpdates[id] = pObjectUpdate;

	// callback
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("network");

	std::vector<std::string> args;
	//args.push_back(cvar->FindVar("avatar_url")->GetString());
	pNetworkInstance->DispatchJavaScriptMethod("clientUpdateCallbacker", "giveNextUpdate", args);
}

KeyValues* C_MetaverseManager::DetectRequiredWorkshopForMapFile(std::string mapFile)
{
	// BEST CAST: this map already has an entry in the library
	// TODO: work

	// else, WORST CASE: figure out where the map is from based on its file location
	char* fullPath = new char[AA_MAX_STRING];
	PathTypeQuery_t pathTypeQuery;
	//std::string fullPathBuf;

	std::string mapBuf = mapFile;
	
	size_t found = mapBuf.find_last_of("/\\");
	if (found != std::string::npos)
		mapBuf = mapBuf.substr(found + 1);

	mapBuf = std::string("maps/") + mapBuf;

	g_pFullFileSystem->RelativePathToFullPath(mapBuf.c_str(), "GAME", fullPath, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);
	mapBuf = fullPath;

	// mapBuf is now a full file path to the BSP
	std::string workshopId;
	std::string mountId;
	std::string mountTitle;
	bool bIsWorkshop = false;
	//bool bIsLegacyWorkshop = false;
	//bool bIsLegacyImported = false;
	bool bIsLegacyImportedGen1Workshop = false;	// legacy imported GEN 1 workshop maps get treated like regular legacy imported non-workshop maps

	std::string baseDir = engine->GetGameDirectory();
	std::string importedLegacyDir = g_pAnarchyManager->GetLegacyFolder();
	std::string workshopDir = g_pAnarchyManager->GetWorkshopFolder();

	// Source gives the map path in all-lowercase (WHY GABE N?!) so the paths we test against also need to be lowercase
	std::transform(baseDir.begin(), baseDir.end(), baseDir.begin(), ::tolower);
	std::transform(importedLegacyDir.begin(), importedLegacyDir.end(), importedLegacyDir.begin(), ::tolower);
	std::transform(workshopDir.begin(), workshopDir.end(), workshopDir.begin(), ::tolower);

	if (importedLegacyDir != "" && importedLegacyDir.find_last_of("\\") == importedLegacyDir.length() - 1)
	{
		// check for the very old GEN1 workshop maps being imported from a legacy folder.  importing these are not supported (but subscribing to them is)
		std::string legacyWorkshopGen1MapsDir = importedLegacyDir + "workshop\\workshopmaps\\maps\\";

		if (mapBuf.find(legacyWorkshopGen1MapsDir) == 0)
			bIsLegacyImportedGen1Workshop = true;
	}
	
	if (!bIsLegacyImportedGen1Workshop)
	{
		//DevMsg("Map: %s vs %s\n", mapBuf.c_str(), workshopDir.c_str());
		// check for content from the workshop
		if (mapBuf.find(workshopDir) == 0)
		{
			bIsWorkshop = true;
			// extract the workshop ID
			workshopId = mapBuf.substr(workshopDir.length());
			workshopId = workshopId.substr(0, workshopId.find_last_of("\\"));
			workshopId = workshopId.substr(0, workshopId.find_last_of("\\"));

			// TODO: determine if this is a legacy workshop map (somehow)
			//bIsLegacyWorkshop = ?;
		}
	}

	C_Mount* pMount = g_pAnarchyManager->GetMountManager()->FindOwningMount(mapBuf);
	if (pMount)
	{
		//	DevMsg("Yar.\n");
		mountId = pMount->GetId();
		mountTitle = pMount->GetTitle();
	}

	// populate mapInfo with stuff
	KeyValues* mapInfo = new KeyValues("mapInfo");
	mapInfo->SetString("fullfile", mapBuf.c_str());
	mapInfo->SetString("workshopIds", workshopId.c_str());
	mapInfo->SetString("mountIds", mountId.c_str());
	mapInfo->SetString("mountTitle", mountTitle.c_str());
	mapInfo->SetBool("bIsWorkshop", bIsWorkshop);
	mapInfo->SetBool("bIsLegacyImportedGen1Workshop", bIsLegacyImportedGen1Workshop);
	return mapInfo;
}

KeyValues* C_MetaverseManager::DetectRequiredWorkshopForModelFile(std::string modelFile)
{
	// BEST CAST: this model already has an entry in the library
	// TODO: work

	// else, WORST CASE: figure out where the map is from based on its file location
	char* fullPath = new char[AA_MAX_STRING];
	PathTypeQuery_t pathTypeQuery;
	//std::string fullPathBuf;

	std::string modelBuf = modelFile;

	/*
	size_t found = modelBuf.find_last_of("/\\");
	if (found != std::string::npos)
		modelBuf = modelBuf.substr(found + 1);

	modelBuf = std::string("maps/") + modelBuf;
	*/
	//DevMsg("Here model is: %s\n", modelBuf.c_str());
	g_pFullFileSystem->RelativePathToFullPath(modelBuf.c_str(), "GAME", fullPath, AA_MAX_STRING, FILTER_NONE, &pathTypeQuery);
	modelBuf = fullPath;
	//DevMsg("And now it is: %s\n", modelBuf.c_str());

	// modelBuf is now a full file path to the BSP
	std::string workshopId;
	std::string mountId;
	std::string mountTitle;
	bool bIsWorkshop = false;
	bool bIsLegacyImported = false;
	//bool bIsLegacyWorkshop = false;

	std::string baseDir = engine->GetGameDirectory();
	std::string importedLegacyDir = g_pAnarchyManager->GetLegacyFolder();
	std::string workshopDir = g_pAnarchyManager->GetWorkshopFolder();

	// Source gives the model path in all-lowercase (WHY GABE N?!) so the paths we test against also need to be lowercase
	std::transform(baseDir.begin(), baseDir.end(), baseDir.begin(), ::tolower);
	std::transform(importedLegacyDir.begin(), importedLegacyDir.end(), importedLegacyDir.begin(), ::tolower);
	std::transform(workshopDir.begin(), workshopDir.end(), workshopDir.begin(), ::tolower);

	if (modelBuf.find(importedLegacyDir) == 0)
		bIsLegacyImported = true;

	// check for content from the workshop
	if (modelBuf.find(workshopDir) == 0)
	{
		bIsWorkshop = true;

		// extract the workshop ID
		workshopId = modelBuf.substr(workshopDir.length());
		workshopId = workshopId.substr(0, workshopId.find("\\"));

		// TODO: determine if this is a legacy workshop model (somehow)
		//bIsLegacyWorkshop = ?;
	}

	// determine mount ID

	C_Mount* pMount = g_pAnarchyManager->GetMountManager()->FindOwningMount(modelBuf);
	if (pMount)
	{
	//	DevMsg("Yar.\n");
		mountId = pMount->GetId();
		mountTitle = pMount->GetTitle();
	}

	// populate mapInfo with stuff
	KeyValues* modelInfo = new KeyValues("modelInfo");
	modelInfo->SetString("fullfile", modelBuf.c_str());
	modelInfo->SetString("workshopIds", workshopId.c_str());
	modelInfo->SetString("mountIds", mountId.c_str());
	modelInfo->SetString("mountTitle", mountTitle.c_str());	// FIXME: This is singluar while the other ones are plural.  Hmmmmm.
	modelInfo->SetBool("bIsWorkshop", bIsWorkshop);
	modelInfo->SetBool("bIsLegacyImported", bIsLegacyImported);
	return modelInfo;
}

void C_MetaverseManager::FlagDynamicModels()
{
	// technically shouldn't be needed, but it does allow addon cabinet models to be added to the list just by living in the right folder, and thats cool.
	std::string buf;
	KeyValues* active;
	std::map<std::string, KeyValues*>::iterator it = m_models.begin();
	while (it != m_models.end())
	{
		active = this->GetActiveKeyValues(it->second);
		//active = it->second->FindKey("current");
		//if (!active)
		//	active = it->second->FindKey("local", true);

		if (active->GetInt("dynamic") != 1)
		{
			buf = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
			if ((buf.find("models\\cabinets\\") == 0 || buf.find("models/cabinets/") == 0 || buf.find("models\\banners\\") == 0 || buf.find("models/banners/") == 0 || buf.find("models\\frames\\") == 0 || buf.find("models/frames/") == 0 || buf.find("models\\icons\\") == 0 || buf.find("models/icons/") == 0) && (buf.find("room_divider.mdl") == std::string::npos && buf.find("newton_toy.mdl") == std::string::npos))
			{
				active->SetInt("dynamic", 1);
				DevMsg("Flagged %s as dynamic model\n", buf.c_str());
			}
		}
		/*
		else
		{
			buf = active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
			if ()// || (buf.find("models\\cabinets\\") != 0 && buf.find("models/cabinets/") != 0 && buf.find("models\\banners\\") != 0 && buf.find("models/banners/") != 0 && buf.find("models\\frames\\") != 0 && buf.find("models/frames/") != 0 && buf.find("models\\icons\\") != 0 && buf.find("models/icons/") != 0))
			{
				active->SetInt("dynamic", 0);
				DevMsg("Un-flagged %s as dynamic model\n", buf.c_str());
			}
		}
		*/

		it++;
	}
}

unsigned int C_MetaverseManager::LoadCachedMaps()
{
	unsigned int uCount = 0;
	bool bSuccess = false;
	bool bAlreadyExisted = false;
	KeyValues* pCachedMapsKV = new KeyValues("maps");
	if (pCachedMapsKV->LoadFromFile(g_pFullFileSystem, "map_cache.txt", "DEFAULT_WRITE_PATH"))
	{
		bSuccess = true;
		for (KeyValues *sub = pCachedMapsKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			//DevMsg("Filename: %s\n", sub->GetString());

			if (this->CreateMapEntry(sub->GetString(), bAlreadyExisted) && !bAlreadyExisted)
				uCount++;
		}
	}

	pCachedMapsKV->deleteThis();
	return uCount;
}

KeyValues* C_MetaverseManager::CreateMapEntry(const char* pFilename, bool& bAlreadyExisted)
{
	std::string foundName = pFilename;
	//foundName = VarArgs("maps\\%s", pFilename);

	// MAKE THE FILE PATH NICE
	//char path_buffer[AA_MAX_STRING];
	int iAAMaxString = foundName.length() + 1;
	char* path_buffer = new char[iAAMaxString];
	Q_strncpy(path_buffer, foundName.c_str(), iAAMaxString);
	V_FixSlashes(path_buffer);
	foundName = path_buffer;
	delete[] path_buffer;
	// FINISHED MAKING THE FILE PATH NICE

	KeyValues* active;
	std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
	while (it != m_maps.end())
	{
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);

		if (!Q_strcmp(active->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), foundName.c_str()))
		{
			//g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
			bAlreadyExisted = true;
			return it->second;
		}

		it++;
	}

	m_detectedMapFiles.push_back(pFilename);

	// if we haven't found a map for this yet, let's make one.
	KeyValues* map = new KeyValues("map");
	map->SetInt("generation", 3);

	// add standard info
	//std::string id = g_pAnarchyManager->GenerateUniqueId();

	std::string goodTitle = pFilename;
	size_t found = goodTitle.find(".");
	if (found != std::string::npos)
		goodTitle = goodTitle.substr(0, found);

	std::string id = g_pAnarchyManager->GenerateLegacyHash(goodTitle.c_str());

	map->SetString("local/info/id", id.c_str());
	map->SetInt("local/info/created", 0);
	map->SetString("local/info/owner", "local");
	map->SetInt("local/info/removed", 0);
	map->SetString("local/info/remover", "");
	map->SetString("local/info/alias", "");

	std::string mapName = foundName.substr(0, foundName.length() - 4);
	map->SetString("local/title", mapName.c_str());
	map->SetString("local/keywords", "");
	map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/file", foundName.c_str());

	KeyValues* stuffKV = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForMapFile(foundName.c_str());
	if (stuffKV)
	{
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/workshopIds", stuffKV->GetString("workshopIds"));
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/mountIds", stuffKV->GetString("mountIds"));
		stuffKV->deleteThis();
	}

	m_maps[id.c_str()] = map;
	bAlreadyExisted = false;

	// update any legacy instances that were detected that use this map
	std::string legacyMapName = foundName.substr(0, foundName.length() - 4);
	//DevMsg("LEGACY MAP NAME: %s\n", legacyMapName.c_str());
	g_pAnarchyManager->GetInstanceManager()->LegacyMapIdFix(legacyMapName, id);
	//		if (pInstance)
	//		pInstance->mapId = id;

	return map;
}

KeyValues* C_MetaverseManager::DetectFirstMap(bool& bAlreadyExists)
{
	//if (m_previousDetectLocalMapFilePath != "")	// FIXME: need a way to detect if there is already a DetectFirst/Next query active.
		//this->DetectLocalMapClose();

	bool bAlreadyExisted = false;
	KeyValues* pMapKV;
	//instance_t* pInstance;
	const char *pFilename = g_pFullFileSystem->FindFirstEx("maps\\*.bsp", "GAME", &m_previousDetectLocalMapFileHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(m_previousDetectLocalMapFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(m_previousDetectLocalMapFileHandle);
			continue;
		}

		pMapKV = CreateMapEntry(pFilename, bAlreadyExisted);
		bAlreadyExists = bAlreadyExisted;
		return pMapKV;
	}

	g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
	return null;
}

KeyValues* C_MetaverseManager::DetectNextMap(bool& bAlreadyExists)
{
	bool bAlreadyExisted = false;
	KeyValues* pMapKV;
	//instance_t* pInstance;
	const char *pFilename = g_pFullFileSystem->FindNext(m_previousDetectLocalMapFileHandle);
	while (pFilename != NULL)
	{
		if (g_pFullFileSystem->FindIsDirectory(m_previousDetectLocalMapFileHandle))
		{
			pFilename = g_pFullFileSystem->FindNext(m_previousDetectLocalMapFileHandle);
			continue;
		}

		pMapKV = CreateMapEntry(pFilename, bAlreadyExisted);
		bAlreadyExists = bAlreadyExisted;
		return pMapKV;
		/*
		std::string foundName = pFilename;
		//foundName = VarArgs("maps\\%s", pFilename);

		// MAKE THE FILE PATH NICE
		char path_buffer[AA_MAX_STRING];
		Q_strcpy(path_buffer, foundName.c_str());
		V_FixSlashes(path_buffer);
		foundName = path_buffer;
		// FINISHED MAKING THE FILE PATH NICE

		KeyValues* active;
		std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
		while (it != m_maps.end())
		{
			active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);

			if (!Q_strcmp(active->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), foundName.c_str()))
			{
				//g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
				bAlreadyExists = true;
				return it->second;
			}

			it++;
		}

		// if we haven't found a map for this yet, let's make one.
		KeyValues* map = new KeyValues("map");
		map->SetInt("generation", 3);

		// add standard info
		//std::string id = g_pAnarchyManager->GenerateUniqueId();

		std::string goodTitle = pFilename;
		size_t found = goodTitle.find(".");
		if (found != std::string::npos)
			goodTitle = goodTitle.substr(0, found);

		//DevMsg("Generating hash based on: %s\n", goodTitle.c_str());
		std::string id = g_pAnarchyManager->GenerateLegacyHash(goodTitle.c_str());

		map->SetString("local/info/id", id.c_str());
		map->SetInt("local/info/created", 0);
		map->SetString("local/info/owner", "local");
		map->SetInt("local/info/removed", 0);
		map->SetString("local/info/remover", "");
		map->SetString("local/info/alias", "");

		std::string mapName = foundName.substr(0, foundName.length() - 4);
		map->SetString("local/title", mapName.c_str());
		map->SetString("local/keywords", "");
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/file", foundName.c_str());

		KeyValues* stuffKV = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForMapFile(foundName.c_str());
		if (stuffKV)
		{
			map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/workshopIds", stuffKV->GetString("workshopIds"));
			map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/mountIds", stuffKV->GetString("mountIds"));
			stuffKV->deleteThis();
		}

		m_maps[id.c_str()] = map;
		bAlreadyExists = false;

		// update any legacy instances that were detected that use this map
		std::string legacyMapName = foundName.substr(0, foundName.length() - 4);
		g_pAnarchyManager->GetInstanceManager()->LegacyMapIdFix(legacyMapName, id);
//		pInstance = g_pAnarchyManager->GetInstanceManager()->FindInstance(foundName.substr(0, foundName.length() - 4));
	//	if (pInstance)
		//	pInstance->mapId = id;

		return map;
		*/
	}

	g_pFullFileSystem->FindClose(m_previousDetectLocalMapFileHandle);
	return null;
}

KeyValues* C_MetaverseManager::DetectMap(std::string mapFilename)
{
	// FIXME: Shouldn't this also use CreateMapEntry instead?
	// this method checks if the map already exists, but doesn't check if the actual file exists.

	KeyValues* active;
	std::map<std::string, KeyValues*>::iterator it = m_maps.begin();
	while (it != m_maps.end())
	{
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		if (!Q_strcmp(active->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), mapFilename.c_str()))
		{
			//bAlreadyExists = true;
			return it->second;
		}

		it++;
	}

	// if we haven't found a map for this yet, let's make one.
	KeyValues* map = new KeyValues("map");
	map->SetInt("generation", 3);

	// add standard info
	//std::string id = g_pAnarchyManager->GenerateUniqueId();

	std::string goodTitle = mapFilename;
	size_t found = goodTitle.find(".");
	if (found != std::string::npos)
		goodTitle = goodTitle.substr(0, found);

	std::string id = g_pAnarchyManager->GenerateLegacyHash(goodTitle.c_str());

	map->SetString("local/info/id", id.c_str());
	map->SetInt("local/info/created", 0);
	map->SetString("local/info/owner", "local");
	map->SetInt("local/info/removed", 0);
	map->SetString("local/info/remover", "");
	map->SetString("local/info/alias", "");

	std::string mapName = mapFilename.substr(0, mapFilename.length() - 4);
	map->SetString("local/title", mapName.c_str());
	map->SetString("local/keywords", "");
	map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/file", mapFilename.c_str());

	KeyValues* stuffKV = g_pAnarchyManager->GetMetaverseManager()->DetectRequiredWorkshopForMapFile(mapFilename.c_str());
	if (stuffKV)
	{
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/workshopIds", stuffKV->GetString("workshopIds"));
		map->SetString("local/platforms/-KJvcne3IKMZQTaG7lPo/mountIds", stuffKV->GetString("mountIds"));
		stuffKV->deleteThis();
	}

	m_maps[id.c_str()] = map;

	return map;
}

// Legacy, because it has a file path.
KeyValues* C_MetaverseManager::LoadLocalItem(std::string file, std::string filePath)
{

	//DevMsg("Attemping to load item %s from %s\n", file.c_str(), filePath.c_str());
	KeyValues* pItem = new KeyValues("item");
	bool bLoaded;

	if (filePath != "")
	{
		std::string fullFile = filePath + file;
		bLoaded = pItem->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), "");
	}
	else
		bLoaded = pItem->LoadFromFile(g_pFullFileSystem, file.c_str(), "MOD");

	if (!bLoaded)
	{
		pItem->deleteThis();
		pItem = null;
	}
	else
	{
		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItem);

		std::string id = pActive->GetString("info/id");

		std::vector<std::string>* pDefaultFields = g_pAnarchyManager->GetMetaverseManager()->GetDefaultFields();

		unsigned int max = (*pDefaultFields).size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (!pActive->FindKey((*pDefaultFields)[i].c_str()))
				pActive->SetString((*pDefaultFields)[i].c_str(), "");
		}

		if (!pActive->FindKey("type") || !Q_strcmp(pActive->GetString("type"), ""))
			pActive->SetString("type", AA_DEFAULT_TYPEID);
		
		//DevMsg("Finished loading item\n");

		auto it = m_items.find(id);
		if (it != m_items.end())
		{
			// FIXME: Merg with existing item (keeping in mind that non-legacy items overpower legacy items
			if (it->second->GetBool("loadedFromLegacy"))
			{
				this->SmartMergItemKVs(pItem, it->second);
				it->second->deleteThis();
				it->second = pItem;

				// Then remove the loadedFromLegacy tag, if it exits.
				KeyValues* pTargetKey = pItem->FindKey("loadedFromLegacy");
				if (pTargetKey)
					pItem->RemoveSubKey(pTargetKey);
			}
			else
			{
				this->SmartMergItemKVs(it->second, pItem);
				pItem->deleteThis();
				pItem = it->second;
			}
		}
		else
			m_items[id] = pItem;
	}

	return pItem;
}

unsigned int C_MetaverseManager::LoadAllLocalItems(sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;

	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, "SELECT * from items", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

	int length;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		if (length == 0)
		{
			DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
			continue;
		}

		KeyValues* pItem = new KeyValues("item");

		CUtlBuffer buf(0, length, 0);
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pItem->ReadAsBinary(buf);
		buf.Purge();

		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pItem);

		count++;

		std::string id = pActive->GetString("info/id");

		std::vector<std::string>* pDefaultFields = g_pAnarchyManager->GetMetaverseManager()->GetDefaultFields();

		unsigned int max = (*pDefaultFields).size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (!pActive->FindKey((*pDefaultFields)[i].c_str()))
				pActive->SetString((*pDefaultFields)[i].c_str(), "");
		}

		if (!pActive->FindKey("type") || !Q_strcmp(pActive->GetString("type"), ""))
			pActive->SetString("type", AA_DEFAULT_TYPEID);

		if (pResponseMap)
			(*pResponseMap)[id] = pItem;
		else
			m_items[id] = pItem;
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.
	return count;
}

KeyValues* C_MetaverseManager::LoadLocalModel(std::string file, std::string filePath)
{
	// make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	KeyValues* pModel = new KeyValues("model");
	bool bLoaded;

	if (filePath != "")
	{
		std::string fullFile = filePath + file;
		bLoaded = pModel->LoadFromFile(g_pFullFileSystem, fullFile.c_str(), "");
	}
	else
		bLoaded = pModel->LoadFromFile(g_pFullFileSystem, file.c_str(), "MOD");

	if (!bLoaded)
	{
		pModel->deleteThis();
		pModel = null;
	}
	else
	{
		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pModel);

		std::string id = pActive->GetString("info/id");

		std::vector<std::string> defaultFields;	// Should platform specific fields be expected as well?  Jump off that bridge when you cross it.
		defaultFields.push_back("title");
		defaultFields.push_back("keywords");
		defaultFields.push_back("dynamic");

		unsigned int max = defaultFields.size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (!pActive->FindKey(defaultFields[i].c_str()))
				pActive->SetString(defaultFields[i].c_str(), "");
		}

		m_models[id] = pModel;
	}

	return pModel;
}

unsigned int C_MetaverseManager::LoadAllLocalModels(unsigned int& numDynamic, sqlite3* pDb, std::map<std::string, KeyValues*>* pResponseMap)
{
	if (!pDb)
		pDb = m_db;

	// make it use the new shinnit
	unsigned int count = 0;
	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare(pDb, "SELECT * from models", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		DevMsg("prepare failed: %s\n", sqlite3_errmsg(pDb));

	int length;
	while (sqlite3_step(stmt) == SQLITE_ROW)	// THIS IS WHERE THE LOOP CAN BE BROKEN UP AT!!
	{
		length = sqlite3_column_bytes(stmt, 1);

		if (length == 0)
		{
			DevMsg("WARNING: Zero-byte KeyValues skipped.\n");
			continue;
		}

		//DevMsg("SQL Row: %s\n", sqlite3_column_text(stmt, 0));

		KeyValues* pModel = new KeyValues("model");

		CUtlBuffer buf(0, length, 0);
		buf.CopyBuffer(sqlite3_column_blob(stmt, 1), length);
		pModel->ReadAsBinary(buf);
		buf.Purge();

		// TODO: Look up any alias here first!!
		KeyValues* pActive = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(pModel);

		count++;

		std::string id = pActive->GetString("info/id");
		//DevMsg("ID: %s vs %s\n", pActive->GetString("info/id"), id.c_str());

		std::vector<std::string> defaultFields;	// Should platform specific fields be expected as well?  Jump off that bridge when you cross it.
		defaultFields.push_back("title");
		defaultFields.push_back("keywords");
		defaultFields.push_back("dynamic");

		unsigned int max = defaultFields.size();
		for (unsigned int i = 0; i < max; i++)
		{
			if (!pActive->FindKey(defaultFields[i].c_str()))
				pActive->SetString(defaultFields[i].c_str(), "");
		}


		if (pResponseMap)
			(*pResponseMap)[id] = pModel;
		else
			m_models[id] = pModel;

		if (pActive->GetBool("dynamic"))
			numDynamic++;
	}
	sqlite3_finalize(stmt);	// TODO: error checking?  Maybe not needed, if this is like a close() operation.
	return count;
}

std::string C_MetaverseManager::ResolveLegacyModel(std::string legacyModel)
{
	// iterate through the models
	KeyValues* active;
	for (std::map<std::string, KeyValues*>::iterator it = m_models.begin(); it != m_models.end(); ++it)
	{
		active = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);

		// MAKE THE FILE PATH NICE
		//char niceModel[AA_MAX_STRING];
		int iAAMaxString = legacyModel.length() + 1;
		char* niceModel = new char[iAAMaxString];
		Q_strncpy(niceModel, legacyModel.c_str(), iAAMaxString);
		V_FixSlashes(niceModel);

		//for (int i = 0; niceModel[i] != '\0'; i++)
		//	niceModel[i] = tolower(niceModel[i]);
		// FINISHED MAKING THE FILE PATH NICE

		if (!Q_stricmp(active->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID)), niceModel))
		{
			delete[] niceModel;
			return it->first;
		}

		delete[] niceModel;
	}

	return "";
}