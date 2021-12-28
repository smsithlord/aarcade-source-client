#ifndef TNERENDERTARGETS_H_
#define TNERENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif
 
#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render   targets
 
// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;
 
class CTNERenderTargets : public CBaseClientRenderTargets
{ 
	// no networked vars 
	DECLARE_CLASS_GAMEROOT( CTNERenderTargets, CBaseClientRenderTargets );
public: 
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();
 
	//ITexture* CreateScopeTexture( IMaterialSystem* pMaterialSystem );
	//ITexture* CreateMenuTexture(IMaterialSystem* pMaterialSystem);
	//ITexture* CreateVGuiTexture(IMaterialSystem* pMaterialSystem);
	//ITexture* CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem);
	//ITexture* CreateVROneEyeTextureQuarterSize(IMaterialSystem* pMaterialSystem);
	//ITexture* CreateVROneEyeTextureFullSize(IMaterialSystem* pMaterialSystem);

private:
	//CTextureReference		m_ScopeTexture; 
	//CTextureReference		m_MenuTexture;
	//CTextureReference		m_VGuiTexture;
	//CTextureReference		m_VRTwoEyesHMDRenderTarget;
	//CTextureReference		m_VROneEyeTextureQuarterSize;
	//CTextureReference		m_VROneEyeTextureFullSize;
};
 
extern CTNERenderTargets* TNERenderTargets;
 
#endif //TNERENDERTARGETS_H_