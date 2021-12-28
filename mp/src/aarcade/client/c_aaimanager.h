#ifndef C_AAI_MANAGER_H
#define C_AAI_MANAGER_H

#include <vector>

class C_AAIManager
{
public:
	C_AAIManager();
	~C_AAIManager();

	void Init();

	void Reset();	// called when its web tab is closed.

	void CreateBrowserInstance();
	void SaveKeyValues();

	bool ShouldAnimateItem(std::string itemId);

	void MarkAnimateItem(std::string itemId);
	void UnmarkAnimateItem(std::string itemId);
	void ToggleMarkAnimatedItem(std::string itemId);

	void GetItemMapping(std::string itemId, float &flScaleX, float &flScaleY, float &flOffsetX, float &flOffsetY);
	void RemoveItemMapping(std::string itemId);

	void SendOnItemAdded(unsigned int uIndex, std::string itemId);
	void SendOnItemRemoved(std::string itemId);

	void OnReadyNow();

private:
	std::vector<std::string> m_items;
	KeyValues* m_pAnimatedImagesKV;
};

#endif