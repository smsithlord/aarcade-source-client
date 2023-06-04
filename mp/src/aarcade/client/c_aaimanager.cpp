#include "cbase.h"
#include "c_anarchymanager.h"
#include "c_aaimanager.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_AAIManager::C_AAIManager()
{
	//DevMsg("AAIManager: Constructor\n");
	m_pAnimatedImagesKV = null;
	m_pLastRenderedListKV = null;
}

C_AAIManager::~C_AAIManager()
{
	//DevMsg("AAIManager: Destructor\n");
	m_pAnimateMP4sConVar = null;
	//m_pDeveloperConVar = null;

	if (m_pAnimatedImagesKV)
	{
		m_pAnimatedImagesKV->deleteThis();
		m_pAnimatedImagesKV = null;
	}

	if (m_pLastRenderedListKV) {
		m_pLastRenderedListKV->deleteThis();
		m_pLastRenderedListKV = null;
	}
}

void C_AAIManager::Init()
{
	// load the Always Animating Images file from aarcade_user/animated_images.txt
	m_pAnimatedImagesKV = new KeyValues("items");
	if (!m_pAnimatedImagesKV->LoadFromFile(g_pFullFileSystem, "animated_images.txt", "DEFAULT_WRITE_PATH"))
	{
		m_pAnimatedImagesKV->deleteThis();
		m_pAnimatedImagesKV = new KeyValues("items");
	}

	m_pLastRenderedListKV = new KeyValues("items");
	m_pAnimateMP4sConVar = cvar->FindVar("always_animate_mp4s");
}

void C_AAIManager::Reset()
{
	// our web tab has been closed.
	// un-bookkeep everything.
	m_items.clear();
}

void C_AAIManager::CreateBrowserInstance()
{
	C_SteamBrowserInstance* m_pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("aai");
	if (m_pBrowserInstance)
		return;

	// create the web tab if it doesn't already exist.
	std::string url = "file://";
	url += engine->GetGameDirectory();
	url += "/resource/ui/html/aai.html";

	C_SteamBrowserInstance* pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
	pBrowserInstance->Init("aai", url, "Always Animating Images");
}

void C_AAIManager::SaveKeyValues()
{
	if (!m_pAnimatedImagesKV)
		return;

	m_pAnimatedImagesKV->SaveToFile(g_pFullFileSystem, "animated_images.txt", "DEFAULT_WRITE_PATH");
}

bool C_AAIManager::ShouldAnimateItem(std::string itemId)
{
	if (!m_pAnimatedImagesKV || itemId == "")
		return false;

	return (m_pAnimatedImagesKV->FindKey(VarArgs("id%s", itemId.c_str())) != null);
}

void C_AAIManager::ToggleMarkAnimatedItem(std::string itemId)
{
	if (this->ShouldAnimateItem(itemId))
		this->UnmarkAnimateItem(itemId);
	else
		this->MarkAnimateItem(itemId);
}

void C_AAIManager::MarkAnimateItem(std::string itemId)
{
	if (!m_pAnimatedImagesKV)
		return;

	m_pAnimatedImagesKV->SetBool(VarArgs("id%s", itemId.c_str()), true);
	this->SaveKeyValues();
}

void C_AAIManager::UnmarkAnimateItem(std::string itemId)
{
	if (!m_pAnimatedImagesKV)
		return;

	KeyValues* pKV = m_pAnimatedImagesKV->FindKey(VarArgs("id%s", itemId.c_str()));
	if (pKV)
		m_pAnimatedImagesKV->RemoveSubKey(pKV);

	this->SaveKeyValues();
	this->RemoveItemMapping(itemId);
}

void C_AAIManager::GetItemMapping(std::string itemId, float &flScaleX, float &flScaleY, float &flOffsetX, float &flOffsetY)
{
	bool bExisted;
	unsigned int uNumItems = m_items.size();
	unsigned int uItemIndex;
	for (uItemIndex = 0; uItemIndex < uNumItems; uItemIndex++)
	{
		if (m_items[uItemIndex] == itemId)
		{
			bExisted = true;
			break;
		}
	}

	if (!bExisted)
	{
		// create it.
		m_items.push_back(itemId);
		uNumItems++;
		//uItemIndex++;
	}

	// We now have a valid uItemIndex.
	// Do the MATH to figure out how to scale this SOB.
	// It will be based on uNumItems & uItemIndex's position in it.

	float flScaleX_out = 1.0f;
	float flScaleY_out = 1.0f;
	float flOffsetX_out = 0.0f;
	float flOffsetY_out = 0.0f;

	// TODO: MATH.  Store values in the variables above.
	unsigned int uGridSize = ceil(sqrt(uNumItems+1));
	unsigned int remainder = (uItemIndex) % uGridSize;

	unsigned int uCol = (unsigned int)floor(uItemIndex % uGridSize);
	unsigned int uRow = (unsigned int)floor(uItemIndex / (uGridSize * 1.0f));

	flScaleX_out = 1.0f * (1.0f / (uGridSize * 1.0f));// *(uItemIndex * 1.0f);
	flScaleY_out = 1.0f * (1.0f / (uGridSize * 1.0f));// *(uItemIndex * 1.0f);

	/*if (flScaleX_out <= 0.0f)
		flScaleX_out = 1.0f;
	if (flScaleY_out <= 0.0f)
		flScaleY_out = 1.0f;*/

	flOffsetX_out = (1.0f / (uGridSize * 1.0f)) * (uCol * 1.0f);

	//unsigned int remainder = (uItemIndex+1) % uGridSize;
	//unsigned int row = (division < 1 ) ? division : 
	flOffsetY_out = (1.0f / (uGridSize * 1.0f)) * (uRow * 1.0f);

	/*
	// TOP LEFT 25%
	VMatrix matrix = pTransformVar->GetMatrixValue();
	matrix.Base()[scaleX] = 0.5f;	// 1.0f / uGridSize;
	matrix.Base()[scaleY] = 0.5f;	// 1.0f / uGridSize;
	matrix.Base()[offsetX] = 0.0f;
	matrix.Base()[offsetY] = 0.0f;
	pTransformVar->SetMatrixValue(matrix);

	// TOP RIGHT 25%
	VMatrix matrix = pTransformVar->GetMatrixValue();
	matrix.Base()[scaleX] = 0.5f;
	matrix.Base()[scaleY] = 0.5f;
	matrix.Base()[offsetX] = 0.5f;
	matrix.Base()[offsetY] = 0.0f;
	pTransformVar->SetMatrixValue(matrix);

	// BOTTOM LEFT 25%
	VMatrix matrix = pTransformVar->GetMatrixValue();
	matrix.Base()[scaleX] = 0.5f;
	matrix.Base()[scaleY] = 0.5f;
	matrix.Base()[offsetX] = 0.0f;
	matrix.Base()[offsetY] = 0.5f;
	pTransformVar->SetMatrixValue(matrix);

	// BOTTOM RIGHT 25%
	VMatrix matrix = pTransformVar->GetMatrixValue();
	matrix.Base()[scaleX] = 0.5f;
	matrix.Base()[scaleY] = 0.5f;
	matrix.Base()[offsetX] = 0.5f;
	matrix.Base()[offsetY] = 0.5f;
	pTransformVar->SetMatrixValue(matrix);
	*/

	// Assign the out variables to the ones that were passed into us.
	flScaleX = flScaleX_out;
	flScaleY = flScaleY_out;
	flOffsetX = flOffsetX_out;
	flOffsetY = flOffsetY_out;

	this->DidRender(itemId);

	if (!bExisted)
	{
		//if (!m_pDeveloperConVar) {
			//m_pDeveloperConVar = cvar->FindVar("developer");
		//}

		//if (m_pDeveloperConVar->GetInt() == 1) {
			//g_pAnarchyManager->AddToastMessage("Added 1 animated image to atlas");
		//}
		this->SendOnItemAdded(uItemIndex, itemId);
	}
}

void C_AAIManager::RemoveItemMapping(std::string itemId)
{
	//if (!m_pDeveloperConVar) {
		//m_pDeveloperConVar = cvar->FindVar("developer");
	//}

	unsigned int size = m_items.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (m_items[i] == itemId)
		{
			m_items.erase(m_items.begin() + i);
			//if (m_pDeveloperConVar->GetInt() == 1) {
				//g_pAnarchyManager->AddToastMessage("Removed 1 animated image to atlas");
			//}
			this->SendOnItemRemoved(itemId);
			return;
		}
	}

	KeyValues* pVictimKV = m_pLastRenderedListKV->FindKey(itemId.c_str());
	if (pVictimKV) {
		m_pLastRenderedListKV->RemoveSubKey(pVictimKV);
		pVictimKV = null;
	}
}

void C_AAIManager::OnReadyNow()
{
	unsigned int size = m_items.size();
	for (unsigned int i = 0; i < size; i++)
		this->SendOnItemAdded(i, m_items[i]);
}

void C_AAIManager::Update() {
	int iCurFrame = gpGlobals->framecount;
	std::vector<std::string> victims;
	for (KeyValues *sub = m_pLastRenderedListKV->GetFirstSubKey(); sub; sub = sub->GetNextKey()) {
		if (iCurFrame - sub->GetInt() > 360) { // 6 seconds at 60fps = 360 frames
			victims.push_back(std::string(sub->GetName()));
		}
	}

	for (unsigned int i = 0; i < victims.size(); i++) {
		this->RemoveItemMapping(victims[i]);
	}
}

void C_AAIManager::DidRender(std::string itemId) {
	m_pLastRenderedListKV->SetInt(itemId.c_str(), gpGlobals->framecount);
}

void C_AAIManager::SendOnItemRemoved(std::string itemId)
{
	C_SteamBrowserInstance* m_pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("aai");
	if (m_pBrowserInstance)
	{
		std::string code = std::string("OnItemRemoved(\"") + itemId + "\");";
		m_pBrowserInstance->InjectJavaScript(code);
	}
}

void C_AAIManager::SendOnItemAdded(unsigned int uIndex, std::string itemId)
{
	C_SteamBrowserInstance* m_pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance("aai");
	if (m_pBrowserInstance)
	{
		KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId));
		if (pItemKV)
		{
			std::string code = std::string(VarArgs("OnItemAdded(%i, %u, \"", m_pAnimateMP4sConVar->GetInt(), uIndex)) + itemId + std::string("\", \"") + g_pAnarchyManager->encodeURIComponent(pItemKV->GetString("file")) + std::string("\", \"") + g_pAnarchyManager->encodeURIComponent(pItemKV->GetString("screen")) + std::string("\", \"") + g_pAnarchyManager->encodeURIComponent(pItemKV->GetString("marquee")) + std::string("\", \"") + g_pAnarchyManager->encodeURIComponent(pItemKV->GetString("preview")) + std::string("\", \"") + g_pAnarchyManager->encodeURIComponent(pItemKV->GetString("stream")) + std::string("\");");
			m_pBrowserInstance->InjectJavaScript(code);
		}
	}
	else
		this->CreateBrowserInstance();
}