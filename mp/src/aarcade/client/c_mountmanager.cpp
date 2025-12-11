#include "cbase.h"

#include "aa_globals.h"
#include "c_mountmanager.h"
#include "c_anarchymanager.h"
#include "filesystem.h"
#include "../../public/registry.cpp"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_MountManager::C_MountManager()
{
	DevMsg("MountManager: Constructor\n");
}

C_MountManager::~C_MountManager()
{
	DevMsg("MountManager: Destructor\n");
}

void C_MountManager::Init()
{
	DevMsg("MountManager: Init\n");

	// Get all Steam library folders
	HKEY key;
	if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Valve\\Steam"), &key) == ERROR_SUCCESS)
	{
		char value[AA_MAX_STRING];
		DWORD value_length = AA_MAX_STRING;
		DWORD flags = REG_SZ;
		RegQueryValueEx(key, "SteamPath", NULL, &flags, (LPBYTE)&value, &value_length);
		RegCloseKey(key);

		V_FixSlashes(value);
		/*
		if (value[3] == '\\')
		{
			std::string buf = value;
			buf = buf.substr(0, 2) + buf.substr(3);
			Q_strcpy(value, buf.c_str());
		}
		*/

		// primary
		m_libraryPaths.push_back(VarArgs("%s\\SteamApps\\common\\", value));

		// primary mod
		//if (g_pFullFileSystem->FindFirstEx(VarArgs("%s\\SteamApps\\sourcemods", value), "", &pHandle))
		if (g_pFullFileSystem->IsDirectory(VarArgs("%s\\SteamApps\\sourcemods", value)))
			m_libraryPaths.push_back(VarArgs("%s\\SteamApps\\sourcemods\\", value));
		//g_pFullFileSystem->FindClose(pHandle);

		// load %value/config/config.vdf to find other library paths
		KeyValues* pConfigKV = new KeyValues("InstallConfigStore");
		//pConfigKV->UsesEscapeSequences(true);
		if (pConfigKV->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\config\\config.vdf", value), ""))
		{
			// For now, try searching until a search path is not found.
			// However, search paths may not be sequential, there might be numbers missing.
			// If that turns out to be the case, just check for the 1st 100 paths or something.

			std::string buf;
			std::string base = "Software/Valve/Steam/BaseInstallFolder_";
			KeyValues* pKey = pConfigKV->FindKey(VarArgs("%s1", base.c_str()));
			for (int i = 2; pKey; i++)
			{
				buf = pKey->GetString();
				if (buf.at(3) == '\\')
					buf = buf.substr(0, 2) + buf.substr(3);

			//	DevMsg("here it found: %s\n", buf.c_str());
				m_libraryPaths.push_back(VarArgs("%s\\SteamApps\\common\\", buf.c_str()));

				if (g_pFullFileSystem->IsDirectory(VarArgs("%s\\SteamApps\\sourcemods", buf.c_str())))
					m_libraryPaths.push_back(VarArgs("%s\\SteamApps\\sourcemods\\", buf.c_str()));
				
				pKey = pConfigKV->FindKey(VarArgs("%s%i", base.c_str(), i));
			}
		}
		pConfigKV->deleteThis();
	}

	/*
	for (unsigned int i = 0; i < m_libraryPaths.size(); i++)
	{
		DevMsg("Path: %s\n", m_libraryPaths[i].c_str());
	}
	*/
}

void C_MountManager::DetectGamePaths(std::vector<std::string> &libraryPaths)
{
	libraryPaths.clear();

	// Get SteamPath
	IRegistry *reg = InstanceRegistry("Steam");
	const char *rawSteamPath = reg ? reg->ReadString("SteamPath", "UNKNOWN") : "UNKNOWN";
	ReleaseInstancedRegistry(reg);

	char steamPath[1024];
	Q_strncpy(steamPath, rawSteamPath, sizeof(steamPath));
	steamPath[sizeof(steamPath) - 1] = '\0';
	V_FixSlashes(steamPath);

	// Helper: add /steamapps/common/
	auto AddCommon = [&](const char *base)
	{
		char fixed[1024];
		Q_strncpy(fixed, base, sizeof(fixed));
		fixed[sizeof(fixed) - 1] = '\0';
		V_FixSlashes(fixed);

		char full[1024];
		Q_snprintf(full, sizeof(full), "%s/steamapps/common/", fixed);
		V_FixSlashes(full);

		libraryPaths.push_back(full);
	};

	// Load libraryfolders.vdf
	char vdfPath[1024];
	Q_snprintf(vdfPath, sizeof(vdfPath), "%s/steamapps/libraryfolders.vdf", steamPath);
	V_FixSlashes(vdfPath);

	KeyValues *kv = new KeyValues("LibraryFolders");
	kv->UsesEscapeSequences(true);
	bool loaded = kv->LoadFromFile(g_pFullFileSystem, vdfPath, "");

	if (loaded)
	{
		for (int i = 0; i < 100; ++i)
		{
			KeyValues *entry = kv->FindKey(VarArgs("%d", i));
			if (!entry)
				continue;

			const char *path = entry->GetFirstSubKey()
				? entry->GetString("path", "")
				: entry->GetString();

			if (path && path[0])
				AddCommon(path);
		}
	}

	kv->deleteThis();

	// Ensure at least SteamPath
	if (libraryPaths.empty())
		AddCommon(steamPath);

	// Add sourcemods only for SteamPath
	char smPath[1024];
	Q_snprintf(smPath, sizeof(smPath), "%s/steamapps/sourcemods", steamPath);
	V_FixSlashes(smPath);

	if (g_pFullFileSystem->IsDirectory(smPath))
	{
		char full[1024];
		Q_snprintf(full, sizeof(full), "%s/", smPath);
		V_FixSlashes(full);
		libraryPaths.push_back(full);
	}

	// Output detected paths
	for (size_t i = 0; i < libraryPaths.size(); ++i)
		DevMsg("Detected path: %s\n", libraryPaths[i].c_str());
}

/*
void C_MountManager::DetectGamePaths(std::vector<std::string>& libraryPaths)
{
	DevMsg("MountManager: Detecting game paths...\n");

	// Read from the registry using iregistry
	IRegistry *reg = InstanceRegistry("Steam");
	reg->ReadString("SteamPath");
	//std::string steamPath = reg->ReadString("SteamPath", "UNKNOWN");
	const char *path = reg->ReadString("SteamPath", "UNKNOWN");
	char steamPath[1024];
	Q_strncpy(steamPath, path, sizeof(steamPath));
	steamPath[sizeof(steamPath) - 1] = '\0'; // ensure null termination
	V_FixSlashes(steamPath);

	ReleaseInstancedRegistry(reg);
	DevMsg("SteamPath from IRegistery is: %s\n", steamPath);

	// Get all Steam library folders
	HKEY key;
	if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Valve\\Steam"), &key) == ERROR_SUCCESS)
	{
		DevMsg("Successfully opened key Software\\Valve\\Steam\n");
		char value[AA_MAX_STRING];
		DWORD value_length = AA_MAX_STRING;
		DWORD flags = REG_SZ;
		RegQueryValueEx(key, "SteamPath", NULL, &flags, (LPBYTE)&value, &value_length);
		RegCloseKey(key);

		V_FixSlashes(value);
		DevMsg("SteamPath from Windows is: %s\n", steamPath);
		
		//if (value[3] == '\\')
		//{
		//std::string buf = value;
		//buf = buf.substr(0, 2) + buf.substr(3);
		//Q_strcpy(value, buf.c_str());
		//}

		// primary
		libraryPaths.push_back(VarArgs("%s\\SteamApps\\common\\", value));

		// primary mod
		//if (g_pFullFileSystem->FindFirstEx(VarArgs("%s\\SteamApps\\sourcemods", value), "", &pHandle))
		if (g_pFullFileSystem->IsDirectory(VarArgs("%s\\SteamApps\\sourcemods", value)))
			libraryPaths.push_back(VarArgs("%s\\SteamApps\\sourcemods\\", value));
		//g_pFullFileSystem->FindClose(pHandle);

		// load %value/config/config.vdf to find other library paths
		KeyValues* pConfigKV = new KeyValues("InstallConfigStore");
		//pConfigKV->UsesEscapeSequences(true);
		if (pConfigKV->LoadFromFile(g_pFullFileSystem, VarArgs("%s\\config\\config.vdf", value), ""))
		{
			DevMsg("Successfully loaded file %s\\config\\config.vdf\n", value);
			// For now, try searching until a search path is not found.
			// However, search paths may not be sequential, there might be numbers missing.
			// If that turns out to be the case, just check for the 1st 100 paths or something.

			std::string buf;
			std::string base = "Software/Valve/Steam/BaseInstallFolder_";
			KeyValues* pKey = pConfigKV->FindKey(VarArgs("%s1", base.c_str()));
			for (int i = 2; pKey; i++)
			{
				buf = pKey->GetString();
				if (buf.at(3) == '\\')
					buf = buf.substr(0, 2) + buf.substr(3);

				//	DevMsg("here it found: %s\n", buf.c_str());
				libraryPaths.push_back(VarArgs("%s\\SteamApps\\common\\", buf.c_str()));

				if (g_pFullFileSystem->IsDirectory(VarArgs("%s\\SteamApps\\sourcemods", buf.c_str())))
					libraryPaths.push_back(VarArgs("%s\\SteamApps\\sourcemods\\", buf.c_str()));

				pKey = pConfigKV->FindKey(VarArgs("%s%i", base.c_str(), i));
			}
		}
		else {
			DevMsg("Failed to load from file %s\\config\\config.vdf\n", value);
		}
		pConfigKV->deleteThis();
	}
	else {
		DevMsg("Failed to RegOpenKey Software\\Valve\\Steam\n");
	}

//	for (unsigned int i = 0; i < m_libraryPaths.size(); i++)
//		DevMsg("Path: %s\n", m_libraryPaths[i].c_str());
}*/

bool C_MountManager::LoadMountsFromKeyValues(std::string filename)
{
	unsigned int mountCount = 0;

	bool bMountsEnabled = cvar->FindVar("mounts")->GetBool();

	KeyValues* pKv = new KeyValues("mounts");
	if (bMountsEnabled && pKv->LoadFromFile(g_pFullFileSystem, filename.c_str(), "MOD"))
	{
		for (KeyValues *pMountKv = pKv->GetFirstSubKey(); pMountKv; pMountKv = pMountKv->GetNextKey())
		{
			C_Mount* pMount = new C_Mount();

			std::string id = pMountKv->GetString("id");
			std::string title = pMountKv->GetString("title");
			std::string base = pMountKv->GetString("base");
			std::vector<std::string> paths;
			for (KeyValues *sub = pMountKv->FindKey("paths", true)->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				paths.push_back(sub->GetString());

			pMount->Init(id, title, base, paths);

			//		if (pKv->GetBool("active"))
			//	{
			if (pMount->Activate())
				mountCount++;

			m_mounts[id] = pMount;
			//		}
		}
	}

	pKv->deleteThis();

	std::string num = VarArgs("%u", mountCount);
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Mounting Source Engine Games", "mounts", "0", num, num);

	return false;
}

void C_MountManager::GetAllMounts(std::vector<C_Mount*>& responseVector)
{
	std::map<std::string, C_Mount*>::iterator it = m_mounts.begin();
	while (it != m_mounts.end())
	{
		responseVector.push_back(it->second);
		it++;
	}
}

C_Mount* C_MountManager::GetMount(std::string id)
{
	std::map<std::string, C_Mount*>::iterator it = m_mounts.begin();
	while (it != m_mounts.end())
	{
		if (it->first == id)
			return it->second;

		it++;
	}

	return null;
}

// NOTE: THe file should be a FULL file path, not a relative one.
C_Mount* C_MountManager::FindOwningMount(std::string file)
{
	//DevMsg("And here file is: %s\n", file.c_str());
	std::map<std::string, C_Mount*>::iterator it = m_mounts.begin();
	while (it != m_mounts.end())
	{
		if (it->second->DoesOwn(file))
			return it->second;

		it++;
	}

	return null;
}