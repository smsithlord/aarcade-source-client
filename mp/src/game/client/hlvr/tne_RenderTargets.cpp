#include "cbase.h"
#include "tne_RenderTargets.h"
//#include "../../aarcade/c_anarchymanager.h"
#include "materialsystem\imaterialsystem.h"
#include "rendertexture.h"
#include "sourcevr/isourcevirtualreality.h"
#include "../aarcade/client/c_anarchymanager.h"

#include "hlvr/proxydll.h"

//ConVar hlvr_scope_render_size("hlvr_scope_render_size", "1", FCVAR_ARCHIVE, "Multiplier for scope render");
//ConVarRef hlvr_sdk("hlvr_sdk");

/*
ITexture* CTNERenderTargets::CreateScopeTexture( IMaterialSystem* pMaterialSystem )
{
	float scopeSize = 1024;
	if (float mult = hlvr_scope_render_size.GetFloat())
	{
		scopeSize *= mult;
	}

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Scope",
		//(int)hmdGetBufferSize().x, (int)hmdGetBufferSize().y,
		scopeSize, scopeSize,
		RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SEPARATE, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CTNERenderTargets::CreateVGuiTexture(IMaterialSystem* pMaterialSystem)
{
	int iScreenWidth = ScreenWidth();
	int iScreenHeight = ScreenHeight();

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_vgui",
		iScreenWidth, iScreenHeight,
		RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);

}


ITexture* CTNERenderTargets::CreateMenuTexture(IMaterialSystem* pMaterialSystem)
{
	int iScreenWidth = ScreenWidth();
	int iScreenHeight = ScreenHeight();

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_menu",
		iScreenWidth, iScreenHeight,
		RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);

}
*/
/*
ITexture* CTNERenderTargets::CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem)
{
	//int iScreenWidth = 2160;// 1920;// ScreenWidth();//
	//int iScreenHeight = 1200;//1080;//  ScreenHeight();//

	POINT vrres;// = hmdGetResolution();
	vrres.x = 1920;// 2160;
	vrres.y = 1080;// 1200;
	unsigned int uScreenWidth = vrres.x;
	unsigned int uScreenHeight = vrres.y;

	int pseudo_sdk_version = 2;
	ITexture* pTexture = pMaterialSystem->CreateNamedRenderTargetTextureEx2("_rt_two_eyes_VR", uScreenWidth, uScreenHeight, RT_SIZE_LITERAL, (pseudo_sdk_version != 2) ? pMaterialSystem->GetBackBufferFormat() : IMAGE_FORMAT_RGBA16161616F, MATERIAL_RT_DEPTH_SEPARATE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, CREATERENDERTARGETFLAGS_HDR);

	if (pTexture)
		DevMsg("Created VR Two Eyes HMD Render Target\n");
	else
		DevMsg("Failed to create VR Two Eyes HMD Render Target\n");
	return pTexture;
}
*/
/*
ITexture* CTNERenderTargets::CreateVROneEyeTextureQuarterSize(IMaterialSystem* pMaterialSystem)
{
	int iScreenWidth = 1920;// ScreenWidth();
	int iScreenHeight = 1080;// ScreenHeight();

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_one_eye_quarter_size_1_VR",
		(int)iScreenWidth / 4, (int)iScreenHeight / 2,
		RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);
}

ITexture* CreateVROneEyeTextureFullSize(IMaterialSystem* pMaterialSystem)
{
	int iScreenWidth = 1920;// ScreenWidth();
	int iScreenHeight = 1080;// ScreenHeight();

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_one_eye_VR",
		(int)iScreenWidth / 2, (int)iScreenHeight,
		RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);
}
*/

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CTNERenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{ 
	//m_ScopeTexture.Init( CreateScopeTexture( pMaterialSystem ) ); 
	//m_MenuTexture.Init(CreateMenuTexture(pMaterialSystem));
	//m_VGuiTexture.Init(CreateVGuiTexture(pMaterialSystem));

	/*
	m_VRTwoEyesHMDRenderTarget.Init(CreateVRTwoEyesHMDRenderTarget(pMaterialSystem));
	m_VROneEyeTextureQuarterSize.Init(CreateVROneEyeTextureQuarterSize(pMaterialSystem));
	
	// m_VROneEyeTextureFullSize.Init(CreateVROneEyeTextureFullSize(pMaterialSystem)); No need for now
	*/
	// Water effects & camera from the base class (standard HL2 targets) 
	g_pAnarchyManager->InitClientRenderTargets(pMaterialSystem, pHardwareConfig);
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig );
}

//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CTNERenderTargets::ShutdownClientRenderTargets()
{ 
	//m_ScopeTexture.Shutdown();
	//m_MenuTexture.Shutdown();
	//m_VGuiTexture.Shutdown();

	/*
	m_VRTwoEyesHMDRenderTarget.Shutdown();
	m_VROneEyeTextureQuarterSize.Shutdown();
	*/

	// Clean up standard HL2 RTs (camera and water) 
	g_pAnarchyManager->ShutdownClientRenderTargets();
	BaseClass::ShutdownClientRenderTargets();
}
 
//add the interface!
static CTNERenderTargets g_TNERenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTNERenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_TNERenderTargets  );
CTNERenderTargets* TNERenderTargets = &g_TNERenderTargets;