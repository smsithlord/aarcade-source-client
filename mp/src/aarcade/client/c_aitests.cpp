#include "cbase.h"
#include "aa_globals.h"
#include "c_aitests.h"
//#include "c_anarchymanager.h"

//#include <KeyValues.h>
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "tier1/utlbuffer.h"
#include "filesystem.h"
#include "studio.h"
#include "../../public/mdllib/mdllib.h"
#include "../../game/client/cdll_client_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//C_AITests g_AITests;
//extern C_AITests* g_pAITests(&g_AITests);

C_AITests::C_AITests()
{
	DevMsg("AITests: Constructor\n");
}

C_AITests::~C_AITests()
{
	DevMsg("AITests: Destructor\n");
}

std::string C_AITests::CreateVTFFromJPG(std::string vtfFileName, std::string jpgFileName)
{
	DevMsg("Create VTF from JPG...\n");
	// TODO: work
	return "";
}

/*
void C_AITests::SaveModelWithMaterial(const char* modelFilename, const char* materialFilename)
{
	// Load the model using the MDLReader class
	MDLReader mdlReader;
	if (!mdlReader.Open(modelFilename))
	{
		Msg("Failed to load model %s\n", modelFilename);
		return;
	}

	// Get the studio header from the MDLReader
	studiohdr_t* pStudioHdr = mdlReader.GetStudioHdr();
	if (!pStudioHdr)
	{
		Msg("Failed to get studio header from model %s\n", modelFilename);
		mdlReader.Close();
		return;
	}

	// Change the model's internal name
	Q_strncpy(pStudioHdr->name, materialFilename, sizeof(pStudioHdr->name));

	// Replace any material named "DynVTFScreen" with the input material name
	for (int i = 0; i < pStudioHdr->numtextures; i++)
	{
		mstudiotexture_t* pTexture = &((mstudiotexture_t*)(((byte*)pStudioHdr) + pStudioHdr->textureindex))[i];
		if (Q_stricmp(pTexture->name, "DynVTFScreen") == 0)
		{
			Q_strncpy(pTexture->name, materialFilename, sizeof(pTexture->name));
		}
	}

	// Add the "materials/models/automats" directory to the model's material search paths
	//char searchPath[MAX_PATH];
	//Q_snprintf(searchPath, sizeof(searchPath), "materials/models/automats;%s", pStudioHdr->name);
	//pStudioHdr->SetSearchPaths(searchPath);

	// Save the modified model using the MDLReader class
	const char* extension = Q_GetFileExtension(modelFilename);
	char newFilename[MAX_PATH];
	Q_snprintf(newFilename, sizeof(newFilename), "%s_modified.%s", modelFilename, extension);
	if (!mdlReader.Save(newFilename))
	{
		Msg("Failed to save modified model %s\n", newFilename);
	}

	// Close the MDLReader
	mdlReader.Close();
}
*/

bool C_AITests::ChangeModelInternalNameAndSave(const char* originalModelPath, const char* newModelPath, const char* newModelInternalName)
{
	// Load the original model file into a CUtlBuffer
	CUtlBuffer buffer;
	if (!g_pFullFileSystem->ReadFile(originalModelPath, "MOD", buffer))
	{
		Warning("Failed to read model file: %s\n", originalModelPath);
		return false;
	}

	// Load the model data into a studiohdr_t struct
	studiohdr_t* pStudioHdr = (studiohdr_t*)buffer.Base();

	/*
	// Check if the buffer has a valid model file
	if (pStudioHdr->id != IDSTUDIOHEADER)
	{
		Warning("Invalid model file: %s\n", originalModelPath);
		return false;
	}*/

	// Change the model's internal name
	Q_strncpy(pStudioHdr->name, newModelInternalName, sizeof(pStudioHdr->name));



	// Get a pointer to the model's first texture
	mstudiotexture_t* pTexture = (mstudiotexture_t*)((byte*)pStudioHdr + pStudioHdr->textureindex);
	DevMsg("First tex name: %s\n", pTexture->pszName());
	//std::string newMaterialName = "villa_chair";
	//std::string newMaterialName = "dynvtfscren";
	//Q_strcpy(pTexture->pszName(), newMaterialName.c_str());
	Q_strcpy(pTexture->pszName(), "dynvtfscren");
	//Q_strncpy(pTexture->pszName(), newMaterialName.c_str(), newMaterialName.length());
	//Q_strncpy(pTexture->pszName(), "models/automats/cloneddynvtfscreen", sizeof(pTexture->pszName()));
	//Q_strncpy(pStudioHdr->name, "automats/cloneddynvtfscreen", sizeof(pStudioHdr->name));



	// Save the modified model with a new filename
	if (!g_pFullFileSystem->WriteFile(newModelPath, "DEFAULT_WRITE_PATH", buffer))
	{
		Warning("Failed to write new model file: %s\n", newModelPath);
		return false;
	}

	// Successfully saved the new model
	return true;
}

bool C_AITests::CloneModelAndApplyMaterial(const char* mdlPath, const char* vmtPath)
{
	return false;
	/*
	// Load the model from the specified path
	CUtlBuffer mdlBuffer(0, 0, CUtlBuffer::TEXT_BUFFER);
	if (!g_pFullFileSystem->ReadFile(mdlPath, "MOD", mdlBuffer))
	{
		// Failed to load the model
		return false;
	}

	// Get a pointer to the model's studiohdr_t struct
	studiohdr_t* pStudioHdr = (studiohdr_t*)mdlBuffer.Base();

	// Get a pointer to the model's first texture
	mstudiotexture_t* pTexture = (mstudiotexture_t*)((byte*)pStudioHdr + pStudioHdr->textureindex);

	// Modify the model name
	Q_strncpy(pStudioHdr->name, "automats/cloneddynvtfscreen", sizeof(pStudioHdr->name));
	*/
	/*
	// Modify the texture's path to reference the provided VMT
	char* pTextureName = (char*)pTexture->name;

	// Find the last forward slash in the texture path
	char* pLastSlash = strrchr(pTextureName, '/');

	if (pLastSlash)
	{
		// Replace the texture name with the provided VMT path
		strcpy(pLastSlash + 1, vmtPath);
	}
	*/

	/*
	// Save the modified model
	g_pFullFileSystem->CreateDirHierarchy("models/automats");

	char newMdlPath[MAX_PATH];
	sprintf_s(newMdlPath, "models/automats/cloneddynvtfscreen.mdl");

	//g_pFullFileSystem->WriteFile(newMdlPath, "DEFAULT_WRITE_PATH", mdlBuffer);

	// Write the modified model and vertex data to file
	CUtlBuffer outBuf;
	outBuf.SetBufferType(true, true);  // enable growable and text mode
	outBuf.Put(pStudioHdr, sizeof(*pStudioHdr));
	outBuf.Put((byte*)mdlBuffer.Base() + sizeof(*pStudioHdr), mdlBuffer.TellMaxPut() - sizeof(*pStudioHdr));
	if (!g_pFullFileSystem->WriteFile(newMdlPath, "DEFAULT_WRITE_PATH", outBuf))
	{
		Warning("Failed to write model '%s'\n", "models/automats/cloneddynvtfscreen.mdl");
		return false;
	}

	// Clean up
	mdlBuffer.Purge();
	outBuf.Purge();
	return true;
	*/
	/*

	// Load the input model
	CUtlBuffer mdlBuf;
	if (!g_pFullFileSystem->ReadFile(mdlPath, "GAME", mdlBuf))
	{
		Warning("Failed to load model '%s'\n", mdlPath);
		return;
	}

	// Load the vertex data using mdlcache
	MDLHandle_t hMdl = mdlcache->FindMDL(mdlPath);
	if (hMdl == MDLHANDLE_INVALID)
	{
		Warning("Failed to find model '%s' in mdlcache\n", mdlPath);
		return;
	}
	studiohdr_t* pStudioHdr = mdlcache->GetStudioHdr(hMdl);
	if (!pStudioHdr)
	{
		Warning("Failed to get studiohdr for model '%s'\n", mdlPath);
		return;
	}
	mstudiobodyparts_t* pBodyParts = (mstudiobodyparts_t*)((byte*)pStudioHdr + pStudioHdr->bodypartindex);
	mstudiomodel_t* pModels = (mstudiomodel_t*)((byte*)pBodyParts->pModel(0));
	mstudiomesh_t* pMeshes = (mstudiomesh_t*)((byte*)pModels->pMesh(0));

	// Modify the first material
	if (pStudioHdr->numtextures < 1)
	{
		Warning("Model '%s' has no textures\n", mdlPath);
		return;
	}
	mstudiotexture_t* pTexture = pStudioHdr->pTexture(0);
	Q_strncpy(pTexture->pszName(), "models/automats/cloneddynvtfscreen", sizeof(pTexture->pszName()));

	// Modify the model name
	Q_strncpy(pStudioHdr->name, "automats/cloneddynvtfscreen", sizeof(pStudioHdr->name));

	// Copy the vertex data
	vertexFileHeader_t* pVvdHdr = mdlcache->GetVertexData(hMdl);
	if (!pVvdHdr)
	{
		Warning("Failed to get vertex data for model '%s'\n", mdlPath);
		return;
	}
	const int numVerts = pVvdHdr->numLODs[0].numVerts;
	const int vvdSize = numVerts * pVvdHdr->vertexStreamSize[0];
	CUtlBuffer vvdBuf;
	vvdBuf.CopyBuffer(pVvdHdr, sizeof(*pVvdHdr));
	vvdBuf.CopyBuffer((byte*)pVvdHdr + pVvdHdr->vertexDataStart, vvdSize);

	// Write the modified model and vertex data to file
	CUtlBuffer outBuf;
	outBuf.SetBufferType(true, true);  // enable growable and text mode
	outBuf.Put(pStudioHdr, sizeof(*pStudioHdr));
	outBuf.Put((byte*)mdlBuf.Base() + sizeof(*pStudioHdr), mdlBuf.TellMaxPut() - sizeof(*pStudioHdr));
	if (!g_pFullFileSystem->WriteFile("models/automats/cloneddynvtfscreen.mdl", NULL, outBuf))
	{
		Warning("Failed to write model '%s'\n", "models/automats/cloneddynvtfscreen.mdl");
		return false;
	}
	if (!g_pFullFileSystem->WriteFile("models/automats/cloneddynvtfscreen.vvd", NULL, vvdBuf))
	{
		Warning("Failed to write vertex data for model '%s'\n", "models/automats/cloneddynvtfscreen.vvd");
		return false;
	}

	// Clean up
	mdlcache->Release(hMdl);
	mdlBuf.Purge();
	outBuf.Purge();
	vvdBuf.Purge();

	return true;
	*/
	/*
	// Load the input model
	CUtlBuffer mdlBuf;
	if (!g_pFullFileSystem->ReadFile(mdlPath, "GAME", mdlBuf))
	{
		Warning("Failed to load model '%s'\n", mdlPath);
		return false;
	}
	studiohdr_t* pStudioHdr = (studiohdr_t*)mdlBuf.Base();
	if (!pStudioHdr)
	{
		Warning("Failed to parse model '%s'\n", mdlPath);
		return false;
	}

	// Modify the first material
	if (pStudioHdr->numtextures < 1)
	{
		Warning("Model '%s' has no textures\n", mdlPath);
		return false;
	}
	mstudiotexture_t* pTexture = pStudioHdr->pTexture(0);
	//Q_strncpy(pTexture->pszName(), "models/automats/cloneddynvtfscreen", sizeof(pTexture->pszName()));

	// Modify the model name
	Q_strcpy(pStudioHdr->name, "automats\\cloneddynvtfscreen.mdl");

	// Write the modified model to file
	CUtlBuffer outBuf;
	outBuf.SetBufferType(true, true);  // enable growable and text mode
	outBuf.Put(pStudioHdr, sizeof(*pStudioHdr));
	outBuf.Put((byte*)mdlBuf.Base() + sizeof(*pStudioHdr), mdlBuf.TellMaxPut() - sizeof(*pStudioHdr));
	if (!g_pFullFileSystem->WriteFile("models/automats/cloneddynvtfscreen.mdl", "DEFAULT_WRITE_PATH", outBuf))
	{
		Warning("Failed to write model '%s'\n", "models/automats/cloneddynvtfscreen.mdl");
		return false;
	}

	// Clean up
	mdlBuf.Purge();
	outBuf.Purge();
	return true;*/
}

//bool C_AITests::CloneModelAssetAndReplaceMaterialTexture(const char* mdlPath, const char* vtfPath)
bool C_AITests::CloneModelMaterialAndApplyTexture(const char* targetMaterialName, const char* targetMaterialVarName, const char* mdlPath, const char* vtfPath)
{
	// Load the original material as a KeyValues structure
	KeyValues* originalMaterialKeyValues = new KeyValues("Material");
	if (!originalMaterialKeyValues->LoadFromFile(g_pFullFileSystem, VarArgs("materials/%s.vmt", targetMaterialName), "MOD"))
	{
		Warning("Failed to load material '%s'\n", targetMaterialName);
		originalMaterialKeyValues->deleteThis();
		return false;
	}

	// Clone the material and save it to disk
	KeyValues* clonedMaterialKeyValues = originalMaterialKeyValues->MakeCopy();
	clonedMaterialKeyValues->SetName(originalMaterialKeyValues->GetName());

	// Set the texture parameter to our VTF
	clonedMaterialKeyValues->SetString(targetMaterialVarName, vtfPath);

	// Save the cloned material to disk
	std::string generatedName = "ClonedDynVTFScreen";
	g_pFullFileSystem->CreateDirHierarchy("materials/models/automats", "DEFAULT_WRITE_PATH");
	if (!clonedMaterialKeyValues->SaveToFile(g_pFullFileSystem, VarArgs("materials/models/automats/%s.vmt", generatedName.c_str()), "DEFAULT_WRITE_PATH"))
	{
		Warning("Failed to save cloned material to disk\n");
		clonedMaterialKeyValues->deleteThis();
		return false;
	}

	originalMaterialKeyValues->deleteThis();
	clonedMaterialKeyValues->deleteThis();

	Msg("Successfully cloned material with texture '%s'\n", targetMaterialName);

	// now clone & modify the model...

	return true;



	/*
	// Load the model and find all of its materials (actually only the first 1024 used on it)
	const model_t* TheModel = modelinfo->FindOrLoadModel(mdlPath);

	IMaterial* pMaterials[1024];
	for (int x = 0; x < 1024; x++)
		pMaterials[x] = NULL;

	modelinfo->GetModelMaterials(TheModel, 1024, &pMaterials[0]);

	std::string materialName;
	for (int x = 0; x < 1024; x++)
	{
		if (pMaterials[x])
		{
			materialName = pMaterials[x]->GetName();
			//if (materialName.find(targetMaterialName) !=)
		}
			DevMsg("Material name: %s\n", pMaterials[x]->GetName());
			//this->AdoptMaterial(pAdoptedFilesKV, pMaterials[x], null, customSubFolder, bDoNotReallyAdopt);
	}



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




	return true;*/


	/*
	// Find the "DynVTFScreen" material used by the MDL
	CUtlBuffer mdlBuffer;
	if (!g_pFullFileSystem->ReadFile(mdlPath, nullptr, mdlBuffer))
	{
		Warning("Failed to load MDL '%s'\n", mdlPath);
		return false;
	}

	const studiohdr_t* studioHeader = (const studiohdr_t*)mdlBuffer.Base();
	const int textureOffset = studioHeader->textureindex;

	const mstudiotexture_t* textureInfo = (const mstudiotexture_t*)((const byte*)studioHeader + textureOffset);
	const char* materialTextureName = textureInfo->pszName();
	DevMsg("Found texture: %s\n", materialTextureName);
	
	IMaterial* pTestMaterial = null;
	pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_WORLD);

	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_MODEL);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_VGUI);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_PARTICLE);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_DECAL);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_SKYBOX);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_OTHER);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_PRECACHED);
	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
		pTestMaterial = g_pMaterialSystem->FindMaterial(materialTextureName, TEXTURE_GROUP_UNACCOUNTED);

	if (!pTestMaterial || pTestMaterial->IsErrorMaterial())
	{
		DevMsg("Could not find any texture for: %s\n", materialTextureName);
		return false;
	}

	std::string goodMaterialName = (pTestMaterial) ? pTestMaterial->GetName() : materialTextureName;
	*/
	/*
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
	}*/
	//DevMsg("Good texture name: %s\n", goodMaterialName.c_str());


	/*
	// Load the original material as a KeyValues structure
	KeyValues* originalMaterialKeyValues = new KeyValues("Material");
	if (!originalMaterialKeyValues->LoadFromFile(g_pFullFileSystem, materialTextureName, "MOD"))
	{
		Warning("Failed to load material '%s'\n", materialTextureName);
		originalMaterialKeyValues->deleteThis();
		return false;
	}

	// Clone the material and save it to disk
	KeyValues* clonedMaterialKeyValues = originalMaterialKeyValues->MakeCopy();
	clonedMaterialKeyValues->SetName("ClonedDynVTFScreen");

	// Set the texture parameter to our VTF
	clonedMaterialKeyValues->SetString("$basetexture", vtfPath);

	// Save the cloned material to disk
	g_pFullFileSystem->CreateDirHierarchy("materials/models", "DEFAULT_WRITE_PATH");
	if (!clonedMaterialKeyValues->SaveToFile(g_pFullFileSystem, "materials/models/ClonedDynVTFScreen.vmt", "DEFAULT_WRITE_PATH"))
	{
		Warning("Failed to save cloned material to disk\n");
		clonedMaterialKeyValues->deleteThis();
		return false;
	}

	originalMaterialKeyValues->deleteThis();
	clonedMaterialKeyValues->deleteThis();

	Msg("Successfully cloned material with texture '%s'\n", materialTextureName);
	return true;
	*/
}