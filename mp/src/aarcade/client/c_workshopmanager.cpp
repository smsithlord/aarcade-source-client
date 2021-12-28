#include "cbase.h"
#include "c_workshopmanager.h"

#include "aa_globals.h"
#include "c_anarchymanager.h"
#include "filesystem.h"

#include "XUnzip.h"
#include <algorithm>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_WorkshopManager::C_WorkshopManager()
{
	DevMsg("WorkshopManager: Constructor\n");
	m_bWorkshopEnabled = cvar->FindVar("workshop")->GetBool();
}

C_WorkshopManager::~C_WorkshopManager()
{
	DevMsg("WorkshopManager: Destructor\n");

	unsigned int max, i;
	auto it = m_details.begin();
	while (it != m_details.end())
	{
		max = it->second->keyValueTags.size();
		for (i = 0; i < max; i++)
			delete it->second->keyValueTags[i];

		it->second->keyValueTags.clear();

		delete it->second;
		it++;
	}

	m_details.clear();
}

void C_WorkshopManager::Init()
{
	DevMsg("WorkshopManager: Init\n");

	if (!m_bWorkshopEnabled)
	{
		DevMsg("Workshop disabled, skipping.\n");
		g_pAnarchyManager->OnWorkshopManagerReady();
		return;
	}

	if (!steamapicontext)
		DevMsg("CRITICAL ERROR: No steamapicontext detected! Try closing & restarting Steam, then launching again!\n");
	else
	{
		DevMsg("Detecting Steam user...\n");
		ISteamUser* pUser = steamapicontext->SteamUser();
		if (!pUser)
			DevMsg("CRITICAL ERROR: Could not detect Steam user!\n");

		C_WorkshopQuery* pWorkshopQuery = new C_WorkshopQuery();

		// Start the 
		AccountID_t aid = pUser->GetSteamID().GetAccountID();

		DevMsg("Building workshop UGC query for user ID %u and app ID %i\n", aid, engine->GetAppID());

		UGCQueryHandle_t hUGCQuery = steamapicontext->SteamUGC()->CreateQueryUserUGCRequest(aid, k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_Items, k_EUserUGCListSortOrder_SubscriptionDateDesc, engine->GetAppID(), engine->GetAppID(), 1);
		SteamAPICall_t hAPICall = steamapicontext->SteamUGC()->SendQueryUGCRequest(hUGCQuery);

		DevMsg("Init workshop query...\n");
		pWorkshopQuery->Init(hAPICall);
		DevMsg("Finished initializing workshop query.\n");
	}
}

void C_WorkshopManager::OnQueryComplete(C_WorkshopQuery* pQuery)
{
	DevMsg("C_WorkshopManager::OnQueryComplete\n");

	if (pQuery)
		delete pQuery;

	g_pAnarchyManager->OnWorkshopManagerReady();
}

C_WorkshopQuery::C_WorkshopQuery()
{
	m_uPageNum = 1;
}

C_WorkshopQuery::~C_WorkshopQuery()
{
	DevMsg("C_WorkshopQuery::Destructor\n");
}

void C_WorkshopQuery::Init(SteamAPICall_t hAPICall)
{
	DevMsg("C_WorkshopQuery::Init\n");
	callback.Set(hAPICall, this, &C_WorkshopQuery::OnUGCQueried);
}

void C_WorkshopQuery::OnUGCQueried(SteamUGCQueryCompleted_t* pResult, bool bIOFailure)
{
	DevMsg("C_WorkshopQuery::OnUGCQueried\n");
	if (pResult->m_eResult == k_EResultOK)
	{
		DevMsg("Batch Size: %u Total Found: %u\n", pResult->m_unNumResultsReturned, pResult->m_unTotalMatchingResults);
		//m_UGCQueryHandle = pResult->m_handle;
	//	m_iUGCQueryNumReturned = pResult->m_unNumResultsReturned;
		//m_uTotalCount = pResult->m_unTotalMatchingResults;

//		this->SteamUGCQueryInit();


		
//		m_details

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");

		unsigned int previousNum = (m_uPageNum - 1) * 50;

		char url[AA_MAX_STRING];
		char additionalPreviewURL[AA_MAX_STRING];
		char key[AA_MAX_STRING];
		char value[AA_MAX_STRING];
		// Changed for Steamworks update.
		//uint64 numSubscriptions;
		//uint64 numFavorites;
		//uint64 numAdditionalPreviews;
		uint32 numSubscriptions;
		uint32 numFavorites;
		uint32 numAdditionalPreviews;
		uint32 numKeyValueTags;
		SteamWorkshopKeyValueTag_t* pKeyValueTag;
		//uint32 additionalURLSize;
		//uint32 previewURLSize;

		// Changed for Steamworks update.
		//EItemPreviewType previewType;
		bool bAdditionalIsImage;
		unsigned int i;
		for (i = 0; i < pResult->m_unNumResultsReturned; i++)
		{
			SteamUGCDetails_t* pDetails = new SteamUGCDetails_t;
			steamapicontext->SteamUGC()->GetQueryUGCResult(pResult->m_handle, i, pDetails);

			if (pDetails->m_eResult == k_EResultOK)
			{
				//m_details[pDetails->m_nPublishedFileId] = pDetails;

				// copy the details because the data accessible in the callback is only temporary
				SteamWorkshopDetails_t* pSteamWorkshopDetails = new SteamWorkshopDetails_t();
				pSteamWorkshopDetails->banned = pDetails->m_bBanned;
				pSteamWorkshopDetails->created = pDetails->m_rtimeCreated;
				pSteamWorkshopDetails->description = std::string(pDetails->m_rgchDescription);
				pSteamWorkshopDetails->file = pDetails->m_hFile;
				pSteamWorkshopDetails->filename = std::string(pDetails->m_pchFileName);
				pSteamWorkshopDetails->fileSize = pDetails->m_nFileSize;
				pSteamWorkshopDetails->numChildren = pDetails->m_unNumChildren;
				pSteamWorkshopDetails->owner = pDetails->m_ulSteamIDOwner;
				pSteamWorkshopDetails->preview = pDetails->m_hPreviewFile;
				pSteamWorkshopDetails->previewSize = pDetails->m_nPreviewFileSize;
				pSteamWorkshopDetails->publishedFileId = pDetails->m_nPublishedFileId;
				pSteamWorkshopDetails->score = pDetails->m_flScore;
				pSteamWorkshopDetails->subscribed = pDetails->m_rtimeAddedToUserList;
				pSteamWorkshopDetails->tags = std::string(pDetails->m_rgchTags);
				pSteamWorkshopDetails->tagsTruncated = pDetails->m_bTagsTruncated;
				pSteamWorkshopDetails->title = std::string(pDetails->m_rgchTitle);
				pSteamWorkshopDetails->type = pDetails->m_eFileType;
				pSteamWorkshopDetails->updated = pDetails->m_rtimeUpdated;
				pSteamWorkshopDetails->url = std::string(pDetails->m_rgchURL);
				pSteamWorkshopDetails->visibility = pDetails->m_eVisibility;
				pSteamWorkshopDetails->votesDown = pDetails->m_unVotesDown;
				pSteamWorkshopDetails->votesUp = pDetails->m_unVotesUp;

				steamapicontext->SteamUGC()->GetQueryUGCPreviewURL(pResult->m_handle, i, url, AA_MAX_STRING);
				pSteamWorkshopDetails->previewURL = std::string(url);

				// Changed for Steamworks update.
				steamapicontext->SteamUGC()->GetQueryUGCStatistic(pResult->m_handle, i, k_EItemStatistic_NumSubscriptions, &numSubscriptions);
				//steamapicontext->SteamUGC()->GetQueryUGCStatistic(pResult->m_handle, i, k_EItemStatistic_NumSubscriptions, &numSubscriptions);
				pSteamWorkshopDetails->numSubscriptions = numSubscriptions;

				steamapicontext->SteamUGC()->GetQueryUGCStatistic(pResult->m_handle, i, k_EItemStatistic_NumFavorites, &numFavorites);
				pSteamWorkshopDetails->numFavorites = numFavorites;
				
				numAdditionalPreviews = steamapicontext->SteamUGC()->GetQueryUGCNumAdditionalPreviews(pResult->m_handle, i);

				char originalFileName[AA_MAX_STRING];
				for (unsigned int j = 0; j < numAdditionalPreviews; j++)
				{
					// Changed for Steamworks update
					steamapicontext->SteamUGC()->GetQueryUGCAdditionalPreview(pResult->m_handle, i, j, additionalPreviewURL, AA_MAX_STRING, &bAdditionalIsImage);
					//steamapicontext->SteamUGC()->GetQueryUGCAdditionalPreview(pResult->m_handle, i, j, additionalPreviewURL, AA_MAX_STRING, originalFileName, AA_MAX_STRING, &previewType);
					pSteamWorkshopDetails->additionalPreviewURLs.push_back(std::string(additionalPreviewURL));
				}

				numKeyValueTags = steamapicontext->SteamUGC()->GetQueryUGCNumKeyValueTags(pResult->m_handle, i);
				for (unsigned int j = 0; j < numKeyValueTags; j++)
				{
					steamapicontext->SteamUGC()->GetQueryUGCKeyValueTag(pResult->m_handle, i, j, key, AA_MAX_STRING, value, AA_MAX_STRING);
					pKeyValueTag = new SteamWorkshopKeyValueTag_t();
					pKeyValueTag->key = std::string(key);
					pKeyValueTag->value = std::string(value);
					pSteamWorkshopDetails->keyValueTags.push_back(pKeyValueTag);
				}


				g_pAnarchyManager->GetWorkshopManager()->AddWorkshopDetails(pSteamWorkshopDetails);

				std::string num = VarArgs("%u", i + previousNum + 1);
				std::string max = VarArgs("%u", pResult->m_unTotalMatchingResults);
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Fetching Workshop Subscriptions", "workfetch", "0", max, num);
			}
		}

		steamapicontext->SteamUGC()->ReleaseQueryUGCRequest(pResult->m_handle);

		if (previousNum + pResult->m_unNumResultsReturned < pResult->m_unTotalMatchingResults)
		{
			m_uPageNum++;

			AccountID_t aid = steamapicontext->SteamUser()->GetSteamID().GetAccountID();
			UGCQueryHandle_t hUGCQuery = steamapicontext->SteamUGC()->CreateQueryUserUGCRequest(aid, k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_Items, k_EUserUGCListSortOrder_SubscriptionDateDesc, engine->GetAppID(), engine->GetAppID(), m_uPageNum);

			SteamAPICall_t hAPICall = steamapicontext->SteamUGC()->SendQueryUGCRequest(hUGCQuery);
			callback.Set(hAPICall, this, &C_WorkshopQuery::OnUGCQueried);
		}
		else
			g_pAnarchyManager->GetWorkshopManager()->OnQueryComplete(this);
			//DevMsg("Finsihed querying workshop content!\n");
	}
}

void C_WorkshopManager::MountWorkshop(PublishedFileId_t id, bool& bIsLegacy, unsigned int& uNumItems, unsigned int& uNumModels, SteamWorkshopDetails_t* pDetails)
{
	// TODO TODO TODO TODO
	// Follow the WorkshopRedux flowchart!

	/*if (id == 339933242)// 340146129)
	{
		DevMsg("Mounting test workshop subscription...\n");
	}
	else
	{
		DevMsg("Skipping non-DreamVault workshop addon %llu\n", id);
		this->MountNextWorkshop();
		return;
	}*/

	/*if (id != 349871036)//349871036)
	{
		DevMsg("Skipping non-DreamVault workshop addon %llu\n", id);
		this->MountNextWorkshop();
		return;
	}*/

	//DevMsg("Mounting workshop addon w/ ID: %llu\n", id);	// TODO: Determine mount ids & backpack Ids before calling load first item.

	SteamWorkshopDetails_t* details = null;
	if (pDetails)
		details = pDetails;
	else
	{
		auto foundMount = m_details.find(id);
		if (foundMount != m_details.end())
			details = m_details[id];
	}
	
	if (!details)
	{
		DevMsg("Skipping invalid workshop addon %llu\n", id);
		this->MountNextWorkshop();
		return;
	}

	unsigned int iStateFlags = steamapicontext->SteamUGC()->GetItemState(details->publishedFileId);
	if (iStateFlags & k_EItemStateSubscribed && iStateFlags & k_EItemStateInstalled && !(iStateFlags & k_EItemStateNeedsUpdate))
	{
		//DevMsg("Item is ready to do shit.\n");

		uint32 unTimeStamp;
		uint64 unSizeOnDisk;
		char installFolder[AA_MAX_STRING];
		unsigned int unFolderSize = AA_MAX_STRING;

		if (!steamapicontext->SteamUGC()->GetItemInstallInfo(details->publishedFileId, &unSizeOnDisk, installFolder, sizeof(installFolder), &unTimeStamp))
		{
			Msg("Workshop Manager: Error - Failed to get install information about the workshop item about to be consumed.\n");
			this->MountNextWorkshop();
			//this->OnMountWorkshopFail();
			return;
		}

//		DevMsg("Workshop Manager: The following workshop item is about to be consumed:\n");
//		DevMsg("\tID: %llu\n", details->m_nPublishedFileId);
//		DevMsg("\tSize On Disk: %u\n", unSizeOnDisk);
//		DevMsg("\tFolder: %s\n", installFolder);
//		DevMsg("\tLegacy Item: %i\n", (iStateFlags & k_EItemStateLegacyItem));

		if ((iStateFlags & k_EItemStateLegacyItem))
		{
			bIsLegacy = true;

			/* old code to simply skip this case
				C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Skipping Gen 1 Legacy Workshop Subscriptions", "skiplegacyworkshops", "", "", "+");
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Models", "workshoplibrarymodels", "", "", "0");	// For progres bar order
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Loading Workshop Items", "workshoplibraryitems", "", "", "0");	// For progres bar order
				pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Mounting Workshop Subscriptions", "mountworkshops", "", std::string(VarArgs("%u", m_details.size())), "+");
				this->MountNextWorkshop();
				return;
			*/

			// This code merely unzips the GEN 1 stuff into a sub-folder of custom and mounts it as normal.
			// TODO: Eventually, GEN 1 will have to be consumed & generate a log in consumed/ just like other workshop versions will be doing.

			std::string installFullPath = VarArgs("%s", installFolder);
			std::string targetPath = VarArgs("custom\\%llu.vpk", details->publishedFileId);

			std::string vpkFullFile = g_pAnarchyManager->GetAArcadeUserFolder();
			std::replace(vpkFullFile.begin(), vpkFullFile.end(), '\\', '/');
			vpkFullFile += "/custom/";
			vpkFullFile += VarArgs("%llu", details->publishedFileId);//zipEntry.name;
			vpkFullFile += ".vpk";

			//if (g_pFullFileSystem->IsDirectory(targetPath.c_str(), "DEFAULT_WRITE_PATH"))
			if (!g_pFullFileSystem->FileExists(targetPath.c_str(), "DEFAULT_WRITE_PATH"))
			//{
				// if this folder already exists, then its content will be mounted as normal along with all other subfolders of custom.
			//	this->MountNextWorkshop();
			//	return;
			//}
			//else
			{
				// unzip it into aarcade_user/custom/[SUBSCRIPTION_ID]/
				std::string zipFile = installFullPath;// +details->filename;

				//char pFilename[AA_MAX_STRING];
				int iAAMaxString = zipFile.length() + 1;
				char* pFilename = new char[iAAMaxString];
				Q_strncpy(pFilename, zipFile.c_str(), iAAMaxString);
				//DevMsg("Open BIN ZIP file: %s\n", pFilename);
				HZIP hz = OpenZip(pFilename, 0, ZIP_FILENAME);
				if (!hz)
				{
					DevMsg("workshop: ABORTED: Failed to open ZIP file %s\n", pFilename);
					this->MountNextWorkshop();
					//this->OnMountWorkshopFail();
					delete[] pFilename;
					return;
				}
				else
				{
					int zipIndex = 0;
					ZIPENTRY zipEntry;
					ZRESULT result = GetZipItem(hz, zipIndex, &zipEntry);

					std::string entryTesterExtension;
					size_t entryExtensionFound;
					bool bAddedVPK = false;
					bool bAddedMap = false;
					while (result == ZR_OK)
					{
						if (zipEntry.attr & FILE_ATTRIBUTE_DIRECTORY)
						{
							zipIndex++;
							result = GetZipItem(hz, zipIndex, &zipEntry);
							continue;
						}

						entryTesterExtension = zipEntry.name;
						std::transform(entryTesterExtension.begin(), entryTesterExtension.end(), entryTesterExtension.begin(), ::tolower);
						entryExtensionFound = entryTesterExtension.find_last_of(".");
						if (entryExtensionFound != std::string::npos)
							entryTesterExtension = entryTesterExtension.substr(entryExtensionFound + 1);
						else
							entryTesterExtension = "";

						if (entryTesterExtension != "" && entryTesterExtension == "vpk")
						{
							// actually unzip the item

							//DevMsg("Unzip file: %s\n", vpkFullFile.c_str());
							//char pTargetFilename[AA_MAX_STRING];
							int iAAMaxString = vpkFullFile.length() + 1;
							char* pTargetFilename = new char[iAAMaxString];
							Q_strncpy(pTargetFilename, vpkFullFile.c_str(), iAAMaxString);

							// make sure custom folder exists
							g_pFullFileSystem->CreateDirHierarchy("custom", "DEFAULT_WRITE_PATH");

							ZRESULT unzipResult = UnzipItem(hz, zipIndex, pTargetFilename, 0, ZIP_FILENAME);
							if (result != ZR_OK)
							{
								DevMsg("ERROR: Could not UNZIP file to %s\n", vpkFullFile.c_str());
							}
							else
							{
								bAddedVPK = true;
							}
							delete[] pTargetFilename;
						}
						else if (entryTesterExtension != "" && entryTesterExtension == "bsp")
						{
							// actually unzip the item
							std::string fullFile = g_pAnarchyManager->GetAArcadeUserFolder();
							std::replace(fullFile.begin(), fullFile.end(), '\\', '/');
							fullFile += "/custom/legacyworkshopmaps/maps/";
							fullFile += zipEntry.name;

							//DevMsg("Unzip file: %s\n", fullFile.c_str());
							//char pTargetFilename[AA_MAX_STRING];
							int iAAMaxString = fullFile.length() + 1;
							char* pTargetFilename = new char[iAAMaxString];
							Q_strncpy(pTargetFilename, fullFile.c_str(), iAAMaxString);

							// make sure custom folder exists
							if (!g_pFullFileSystem->IsDirectory("custom/legacyworkshopmaps", "DEFAULT_WRITE_PATH"))
							{
								// actually unzip the item
								std::string fullLegacyWorkshopMapsPath = g_pAnarchyManager->GetAArcadeUserFolder();
								std::replace(fullLegacyWorkshopMapsPath.begin(), fullLegacyWorkshopMapsPath.end(), '\\', '/');
								fullLegacyWorkshopMapsPath += "/custom/legacyworkshopmaps";

								// The maps folder in custom only needs to be mounted if it was just created.
								g_pFullFileSystem->CreateDirHierarchy("custom/legacyworkshopmaps/maps", "DEFAULT_WRITE_PATH");
								g_pFullFileSystem->AddSearchPath(fullLegacyWorkshopMapsPath.c_str(), "GAME", PATH_ADD_TO_TAIL);
								//DevMsg("Mounted %s\n", fullLegacyWorkshopMapsPath.c_str());
							}

							ZRESULT unzipResult = UnzipItem(hz, zipIndex, pTargetFilename, 0, ZIP_FILENAME);
							if (result != ZR_OK)
							{
								DevMsg("ERROR: Could not UNZIP file to %s\n", fullFile.c_str());
							}
							else
							{
								bAddedMap = true;
							}

							delete[] pTargetFilename;
						}
						//else
						//	DevMsg("Skipping other file: %s\n", zipEntry.name);

						zipIndex++;
						result = GetZipItem(hz, zipIndex, &zipEntry);
					}

					CloseZip(hz);
				}

				delete[] pFilename;
			}

			if (g_pFullFileSystem->FileExists(targetPath.c_str(), "DEFAULT_WRITE_PATH"))
			{
				// Determine if we should consume this or just mount it
				// If there is no consumed/[subscription ID].txt OR if the generation listed in the file is different than its current generation OR if the workshop item has since been updated.
				
				KeyValues* pConsumedKV = new KeyValues("consumed");
				bool bNeedsConsume = false;

				if (pConsumedKV->LoadFromFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH"))
				{
					std::string consumedString = pConsumedKV->GetString("consumed");
					if (consumedString.length() < 2)
						consumedString = "0";
					else
						consumedString = consumedString.substr(1);

					uint64 consumedBuf = Q_atoui64(consumedString.c_str());

					uint32 uConsumedTime = consumedBuf & 0xFFFFFFFF;

					bool bDoAudit = false;
					if (uConsumedTime < details->updated || pConsumedKV->GetInt("generation") != 1)
					{
						pConsumedKV->SetString("consumed", VarArgs("t%llu", g_pAnarchyManager->GetTimeNumber()));
						pConsumedKV->SetInt("generation", 1);
						//pConsumedKV->SaveToFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH");
						bDoAudit = true;
					}

					C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->CreateBackpack(vpkFullFile, id, bDoAudit);
					pBackpack->Activate();
					if (bDoAudit)
					{
						pConsumedKV->SaveToFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH");

						// The VPK will only not already be mounted if it was just now created.
						g_pFullFileSystem->AddSearchPath(vpkFullFile.c_str(), "GAME", PATH_ADD_TO_TAIL);
						//DevMsg("Mounted %s for %llu\n", vpkFullFile.c_str(), details->publishedFileId);
					}
					pConsumedKV->deleteThis();
				}
				else
				{
					C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->CreateBackpack(vpkFullFile, id, true);
					pBackpack->Activate();

					g_pFullFileSystem->AddSearchPath(vpkFullFile.c_str(), "GAME", PATH_ADD_TO_TAIL);
					//DevMsg("Mounted %s for %llu\n", vpkFullFile.c_str(), details->publishedFileId);

					/*
					std::string id = VarArgs("%llu", details->publishedFileId);

					std::string mountIds;
					g_pAnarchyManager->ScanForLegacySaveRecursive(vpkFullFile, "", id, mountIds, null);

					// now start loading the items from it
					g_pAnarchyManager->GetMetaverseManager()->LoadFirstLocalItemLegacy(true, vpkFullFile, id, mountIds, null);
					*/

					pConsumedKV->SetString("consumed", VarArgs("t%llu", g_pAnarchyManager->GetTimeNumber()));
					pConsumedKV->SetInt("generation", 1);
					pConsumedKV->SaveToFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH");
					pConsumedKV->deleteThis();
				}

				/*

				//std::string fullPath = VarArgs("%s\\", installFolder);
				DevMsg("Consume a legacy workshop item: %s\n", vpkFullFile.c_str());

				std::string id = VarArgs("%llu", details->publishedFileId);

				std::string mountIds;
				g_pAnarchyManager->ScanForLegacySaveRecursive(vpkFullFile, "", id, mountIds, null);

				// now start loading the items from it
				g_pAnarchyManager->GetMetaverseManager()->LoadFirstLocalItemLegacy(true, vpkFullFile, id, mountIds, null);
				return;
				*/
			}
		}	
		else
		{
			bIsLegacy = false;
			// mount non-legacy

			// NOTE: The [id].txt file contains a list of every file in the archive, but this info can be derrived from the folder path anyways.
			//std::string file = VarArgs("%s/resource/workshop/%llu.txt", installFolder, id);
			std::string infoFile = VarArgs("%s/info.txt", installFolder);
			KeyValues* pInfoKV = new KeyValues("info");
			if (!pInfoKV->LoadFromFile(g_pFullFileSystem, infoFile.c_str()))
			{
				DevMsg("WorkshopManager: Failed to load info file.\n");
				pInfoKV->deleteThis();
				this->MountNextWorkshop();
				return;
			}

			int iDefinedGeneration = pInfoKV->GetInt("generation", 2);
			pInfoKV->deleteThis();

			KeyValues* pConsumedKV = new KeyValues("consumed");
			bool bNeedsConsume = false;

			if (pConsumedKV->LoadFromFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH"))
			{
				std::string consumedString = pConsumedKV->GetString("consumed");
				if (consumedString.length() < 2)
					consumedString = "0";
				else
					consumedString = consumedString.substr(1);

				uint64 consumedBuf = Q_atoui64(consumedString.c_str());

				uint32 uConsumedTime = consumedBuf & 0xFFFFFFFF;

				bool bDoAudit = false;
				if (uConsumedTime < details->updated || pConsumedKV->GetInt("generation") != iDefinedGeneration)
				{
					pConsumedKV->SetString("consumed", VarArgs("t%llu", g_pAnarchyManager->GetTimeNumber()));
					pConsumedKV->SetInt("generation", iDefinedGeneration);
					bDoAudit = true;
				}

				std::string fullPath = VarArgs("%s\\", installFolder);
				C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->CreateBackpack(fullPath, id, bDoAudit);
				pBackpack->Activate();
				if ( bDoAudit)
					pConsumedKV->SaveToFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH");
				pConsumedKV->deleteThis();
			}
			else
			{
				std::string fullPath = VarArgs("%s\\", installFolder);
				C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->CreateBackpack(fullPath, id, true);
				pBackpack->Activate();

				/*
				std::string id = VarArgs("%llu", details->publishedFileId);

				std::string mountIds;
				g_pAnarchyManager->ScanForLegacySaveRecursive(vpkFullFile, "", id, mountIds, null);

				// now start loading the items from it
				g_pAnarchyManager->GetMetaverseManager()->LoadFirstLocalItemLegacy(true, vpkFullFile, id, mountIds, null);
				*/

				pConsumedKV->SetString("consumed", VarArgs("t%llu", g_pAnarchyManager->GetTimeNumber()));
				pConsumedKV->SetInt("generation", iDefinedGeneration);
				pConsumedKV->SaveToFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH");
				pConsumedKV->deleteThis();
			}
			g_pFullFileSystem->AddSearchPath(installFolder, "GAME", PATH_ADD_TO_TAIL);






			/*
			g_pFullFileSystem->AddSearchPath(installFolder, "GAME", PATH_ADD_TO_TAIL);





			// does this workshop subscription has a consumed record

			//KeyValues* pConsumedKV = new KeyValues("consumed");
			//if (pConsumedKV->LoadFromFile(g_pFullFileSystem, VarArgs("consumed/%llu.txt", id), "DEFAULT_WRITE_PATH"))
			//{
			//	DevMsg("Consumed log exists!!\n\nTODO: Do somethin, tons-of-fun.\n");

				// If an entry exists & all is good to go, use the consumed/%llu.db instead of doing individual file reads.
				// TODO: work
				// ...

			//	pConsumedKV->deleteThis();
			//}
			//else
			//{
				//pConsumedKV->deleteThis();

				std::string fullPath = VarArgs("%s\\", installFolder);

				DevMsg("Consume a legacy workshop item: %s\n", fullPath.c_str());
				//C_Backpack* pBackpack = g_pAnarchyManager->GetBackpackManager()->CreateBackpack(fullPath);
				//pBackpack->LegacyAuditDb();

				//std::string idString = VarArgs("%llu", id);
				//C_Backpack* pBackpack = new C_Backpack();
				//pBackpack->Init(idString, "", fullPath, id);
				//pBackpack->MergDb();
				//pBackpack->CloseDb();

				//DevMsg("Done initing backpack for GEN2 legacy workshop item.\n");
				//g_pFullFileSystem->AddSearchPath(installFolder, "GAME", PATH_ADD_TO_TAIL);
				////return;

				// FIXME: Actually check that this IS a legacy workshop item!! (generation < 3)

				// Time to consume this GEN 2 workshop item.
				// That means creating a library db and consumed KV for it.

				// This is much like auditing a backpack.
				// Tell the AnarchyManager we want to track consummption

				std::string id = VarArgs("%llu", details->publishedFileId);

				std::string mountIds;
				g_pAnarchyManager->ScanForLegacySaveRecursive(fullPath, "", id, mountIds, null);

				// now start loading the items from it
				g_pAnarchyManager->GetMetaverseManager()->LoadFirstLocalItemLegacy(true, fullPath, id, mountIds, null);
				return;
				*/
			//}
		}
	}
	else
		DevMsg("WARNING: Workshop mount state not ready!\n");

	//DevMsg("Done mounting workshop addon.\n");
	//this->MountNextWorkshop();
	
	// DON'T CALL THIS UNTIL AFTER MAPS ARE LOADED TOO!! (only items have been loaded so far)
	// wellll, maybe its OK to to call it now.  maps should be loaded LAST, after everything else is already in.
	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Mounting Workshop Subscriptions", "mountworkshops", "", std::string(VarArgs("%u", g_pAnarchyManager->GetWorkshopManager()->GetNumDetails())), "+", "mountNextWorkshopCallback");
}

void C_WorkshopManager::OnMountWorkshopFail()
{
	DevMsg("CRITICAL ERROR: Failed to mount workshop thing.\n");

	// FIXME this junction should take place in the anarchy manager!!
	//g_pAnarchyManager->GetMetaverseManager()->DetectAllLegacyCabinets();
	g_pAnarchyManager->GetWorkshopManager()->OnMountWorkshopSucceed();
}

void C_WorkshopManager::OnMountWorkshopSucceed()
{
	g_pAnarchyManager->GetMetaverseManager()->ResolveLoadLocalItemLegacyBuffer();

	if (g_pAnarchyManager->GetMetaverseManager()->GetPreviousLocaLocalItemLegacyWorkshopIds() != "")
	{
		// DON'T CALL THIS UNTIL AFTER MAPS ARE LOADED TOO!! (only items have been loaded so far)
		// wellll, maybe its OK to to call it now.  maps should be loaded LAST, after everything else is already in.
		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		pHudBrowserInstance->AddHudLoadingMessage("progress", "", "Mounting Workshop Subscriptions", "mountworkshops", "", std::string(VarArgs("%u", g_pAnarchyManager->GetWorkshopManager()->GetNumDetails())), "+", "mountNextWorkshopCallback");
	}
	else
	{
		g_pAnarchyManager->OnMountAllWorkshopsComplete();
	}
}

void C_WorkshopManager::AddWorkshopDetails(SteamWorkshopDetails_t* pDetails)
{
	m_details[pDetails->publishedFileId] = pDetails;
}

void C_WorkshopManager::GetAllWorkshopSubscriptions(std::vector<SteamWorkshopDetails_t*>& details)
{
	auto it = m_details.begin();
	while (it != m_details.end())
	{
		details.push_back(it->second);
		it++;
	}
}

SteamWorkshopDetails_t* C_WorkshopManager::GetWorkshopSubscription(PublishedFileId_t id)
{
	auto it = m_details.find(id);
	if (it != m_details.end())
		return it->second;
	else
		return null;
}

void C_WorkshopManager::MountFirstWorkshop()
{
	// start the search
	m_bMountWorkshopIsLegacy = false;
	m_uMountWorkshopuNumItems = 0;
	m_uMountWorkshopNumModels = 0;

	m_previousMountWorkshopIterator = m_details.begin();
	if (m_previousMountWorkshopIterator != m_details.end())
		this->MountWorkshop(m_previousMountWorkshopIterator->first, m_bMountWorkshopIsLegacy, m_uMountWorkshopuNumItems, m_uMountWorkshopNumModels, m_previousMountWorkshopIterator->second);
	else
	{
		this->OnMountWorkshopFail();
		return;
	}

	return;
}

void C_WorkshopManager::MountNextWorkshop()
{
	// continue the search
	m_previousMountWorkshopIterator++;
	if (m_previousMountWorkshopIterator != m_details.end())
		this->MountWorkshop(m_previousMountWorkshopIterator->first, m_bMountWorkshopIsLegacy, m_uMountWorkshopuNumItems, m_uMountWorkshopNumModels, m_previousMountWorkshopIterator->second);
	else
		this->OnMountWorkshopFail();
}

unsigned int C_WorkshopManager::GetNumDetails()
{
	return m_details.size();
}

SteamWorkshopDetails_t* C_WorkshopManager::GetDetails(unsigned int index)
{
	unsigned int count = 0;
	auto it = m_details.begin();
	while (it != m_details.end() && count <= index)
	{
		if (count == index)
			return it->second;

		count++;
		it++;
	}

	return null;
}