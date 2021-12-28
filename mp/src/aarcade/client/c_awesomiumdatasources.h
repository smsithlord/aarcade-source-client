#ifndef C_AWESOMIUM_DATA_SOURCES_H
#define C_AWESOMIUM_DATA_SOURCES_H

#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
//#include "vgui_controls/Controls.h"

using namespace Awesomium;

class NewWindowDataSource : public DataSource
{
public:
	NewWindowDataSource();
	virtual ~NewWindowDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};

class UiDataSource : public DataSource
{
public:
	UiDataSource();
	virtual ~UiDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};

class ScreenshotDataSource : public DataSource
{
public:
	ScreenshotDataSource();
	virtual ~ScreenshotDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};

class LocalDataSource : public DataSource
{
public:
	LocalDataSource();
	virtual ~LocalDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};

/*
class NewWindowDataSource : public ResourceInterceptor
{
public:
	NewWindowDataSource();
	virtual ~NewWindowDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};
*/

class CacheDataSource : public DataSource
{
public:
	CacheDataSource();
	virtual ~CacheDataSource();
	virtual void OnRequest(int request_id, const ResourceRequest& request, const WebString& path);
};


#endif