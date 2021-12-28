#ifndef C_CANVAS_MANAGER_H
#define C_CANVAS_MANAGER_H

//#include "c_canvas.h"
#include <vector>
#include "c_canvasregen.h"
#include "c_embeddedinstance.h"
#include "c_websurfaceproxy.h"

struct DeferredCleanupTexture_t
{
	uint64 startTime;
	ITexture* pTexture;
};

class C_CanvasManager
{
public:
	C_CanvasManager();
	~C_CanvasManager();

	void Update();

	CCanvasRegen* GetOrCreateRegen();

	DeferredCleanupTexture_t* FindDeferredCleanupTexture(ITexture* pTexture);
	DeferredCleanupTexture_t* FindDeferredCleanupTexture(std::string textureName);

	void DoOrDeferTextureCleanup(ITexture* pTexture);
	void TextureNotDeferred(ITexture* pTexture);
	void CleanupTextures(bool bForce = false);
	void CleanupTexture(ITexture* pTexture);

	bool IsPriorityEmbeddedInstance(C_EmbeddedInstance* pEmbeddedInstance);
	unsigned int GetNumPriorityEmbeddedInstances() { return 1; }	// the selected instance, and the hud instance
	bool ShouldRender(C_EmbeddedInstance* pEmbeddedInstance, bool bPreTest = false);	// ALL EMBEDDED INSTANCES DECENDANTS SHOULD ACTUALLY RENDER IF THIS FUNCTION RETURNS TRUE, FOR BOOKKEEPING REASONS!!! (unless pretest = true)
	// FIXME: That means that all isDirty tests need to take place FROM HERE, or PRIOR to calling this method.
	// The above 2 comments are probably bullshit and uneeded.  Look int oit if you want, or take my fuckin word for it and remove it.

	void RegisterProxy(CWebSurfaceProxy* pProxy);

	void SetMaterialEmbeddedInstanceId(IMaterial* pMaterial, std::string id);

	C_EmbeddedInstance* FindEmbeddedInstance(IMaterial* pMaterial);
	C_EmbeddedInstance* FindEmbeddedInstance(std::string id);
	C_EmbeddedInstance* FindEmbeddedInstanceByEntityIndex(int iEntityIndex);

	//void IncrementVisibleCanvasesCurrentFrame() { m_iVisibleCanvasesCurrentFrame++; }
	//void IncrementVisiblePriorityCanvasesCurrentFrame() { m_iVisiblePriorityCanvasesCurrentFrame++; }
	//void RenderQueueAdd(C_EmbeddedInstance* pEmbeddedInstance);
	//void PriorityRenderQueueAdd(C_EmbeddedInstance* pEmbeddedInstance);

	// when seen, add to the next queue
	//void PriorityRenderSeen(C_EmbeddedInstance* pEmbeddedInstance);
	bool RenderSeen(C_EmbeddedInstance* pEmbeddedInstance);
	void RenderUnseen(C_EmbeddedInstance* pEmbeddedInstance);

	//void AllowPriorityRender(C_EmbeddedInstance* pEmbeddedInstance);
	void AllowRender(C_EmbeddedInstance* pEmbeddedInstance);

	void LevelShutdownPreEntity();
	void LevelShutdownPostEntity();
	void CloseAllInstances();
	void CloseInstance(std::string id);
	void SwitchToInstance(std::string id);
	void GetAllInstances(std::vector<C_EmbeddedInstance*>& embeddedInstances);

	void CaptureInstanceThumbnail(C_EmbeddedInstance* pEmbeddedInstance);

	ITexture* GetItemTexture(std::string itemId, std::string channel);
	void PrepareRefreshItemTextures(std::string itemId, std::string channel);
	void RefreshItemTextures(std::string itemId, std::string channel);

	void UnreferenceTexture(ITexture* pTexture);
	void UnreferenceEmbeddedInstance(C_EmbeddedInstance* pEmbeddedInstance);

	C_EmbeddedInstance* GetFirstInstanceToDisplay();

	void SetDeferredTextureCleanup(bool bValue);

	void SetItemTextureCacheName(ITexture* pTexture, std::string file);
	std::string GetItemTextureCacheName(ITexture* pTexture);

	// accessors
	C_EmbeddedInstance* GetDisplayInstance() { return m_pDisplayInstance; }
	bool GetDeferredTextureCleanup() { return m_pUseDeferredTextureCleanUpConVar->GetBool(); }// m_bUseDeferredTextureCleanUp;
	unsigned int GetNumPendingTextureCleanup() { return m_pendingTextureCleanup.size(); }
	//int GetLastPriorityRenderedFrame() { return m_iLastPriorityRenderedFrame; }
	/*int GetNumVisiblePriorityCanvasesLastFrame() { return m_iVisiblePriorityCanvasesLastFrame; }
	int GetNumVisibleCanvasesLastFrame() { return m_iVisibleCanvasesLastFrame; }
	int GetLastRenderedFrame() { return m_iLastRenderedFrame; }
	int GetLastPriorityRenderedFrame() { return m_iLastPriorityRenderedFrame; }*/

	// mutators
	void SetDisplayInstance(C_EmbeddedInstance* pEmbeddedInstance) { m_pDisplayInstance = pEmbeddedInstance;  }
	void SetDifferentDisplayInstance(C_EmbeddedInstance* pEmbeddedInstance);
	/*void SetLastRenderedFrame(int frame) { m_iLastRenderedFrame = frame; }
	void SetLastPriorityRenderedFrame(int frame) { m_iLastPriorityRenderedFrame = frame; }*/
	// SetDeferredTextureCleanup(bool bValue) { m_bUseDeferredTextureCleanUp = bValue; }
	
private:
	ConVar* m_pThrottleEmbeddedRenderConVar;
//	bool m_bUseDeferredTextureCleanUp;
	ConVar* m_pUseDeferredTextureCleanUpConVar;
	std::vector<std::string> m_deadTextures;
	std::map<ITexture*, std::string> m_textureCacheNames;
	//std::vector<std::map<std::string, DeferredCleanupTexture_t*>::iterator> m_deadTextures;
	//std::vector<ITexture*> m_pendingTextureCleanup;
	//std::map<std::string, DeferredCleanupTexture_t*> m_pendingTextureCleanup;
	std::vector<DeferredCleanupTexture_t*> m_pendingTextureCleanup;
	C_EmbeddedInstance* m_pDisplayInstance;

//	CCanvasRegen* m_pRegen;
	//int m_iLastAllowedRenderedFrame;
	//int m_iLastAllowedPriorityRenderedFrame;
	int m_iLastRenderedFrame;
	int m_iLastPriorityRenderedFrame;	// this is more like "last allowed priorityrendereed frame", doesn't mean it actually rendered. (this can be fixed.) TODO: Fix this.
	/*int m_iVisibleCanvasesLastFrame;// = -1;
	int m_iVisiblePriorityCanvasesLastFrame;// = -1;
	int m_iVisibleCanvasesCurrentFrame;// = 0;
	int m_iVisiblePriorityCanvasesCurrentFrame;// = 0;*/
	//CUtlVector<C_EmbeddedInstance*> m_visibleCanvasesLastFrame;
	//CUtlVector<C_EmbeddedInstance*> m_visiblePriorityCanvasesLastFrame;

	/*CUtlVector<C_EmbeddedInstance*> m_visibleCanvasesCurrentFrame;
	CUtlVector<C_EmbeddedInstance*> m_visiblePriorityCanvasesCurrentFrame;*/

	// the modifiable list that will be allowed to render
	CUtlVector<C_EmbeddedInstance*> m_nextPriorityCanvasRenderQueue;
	CUtlVector<C_EmbeddedInstance*> m_nextCanvasRenderQueue;

	// the overflow list 
	CUtlVector<C_EmbeddedInstance*> m_priorityCanvasRenderQueue;
	CUtlVector<C_EmbeddedInstance*> m_canvasRenderQueue;
	
//	int m_iLastRenderedFrame = -1;
//	int m_iLastPriorityRenderedFrame = -1;
	//m_pWebBrowser = null;
	CCanvasRegen* m_pCanvasRegen;
	//C_EmbeddedInstance* m_pSelectedEmbeddedInstance;
	std::vector<CWebSurfaceProxy*> m_webSurfaceProxies;
	std::map<IMaterial*, std::string> m_materialEmbeddedInstanceIds;
};

#endif