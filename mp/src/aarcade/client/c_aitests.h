#ifndef C_AITESTS_H
#define C_AITESTS_H

#include <string>

class C_AITests
{
public:
	C_AITests();
	~C_AITests();

	std::string CreateVTFFromJPG(std::string vtfFileName, std::string jpgFileName);
	//bool CloneModelAssetAndReplaceMaterialTexture(const char* mdlPath, const char* vtfPath);
	bool CloneModelMaterialAndApplyTexture(const char* targetMaterialName, const char* targetMaterialVarName, const char* mdlPath, const char* vtfPath);
	bool CloneModelAndApplyMaterial(const char* mdlPath, const char* vmtPath);
	bool ChangeModelInternalNameAndSave(const char* originalModelPath, const char* newModelPath, const char* newModelInternalName);
	void SaveModelWithMaterial(const char* modelFilename, const char* materialFilename);

//private:
};

//extern C_AITests* g_pAITests;

#endif