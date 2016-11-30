#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include "psmanager.h"
#include "rulemng.h"
#include <fsproxy.h>
#pragma comment(lib,"fsproxy.lib")

class CService : public ISimplifiedCallbacks
{
    ISetup* pSetup;
    IRuleManager* pRuleManager;
    CProcessManager* pProcessManager;
    bool bFilteringStarted;

    DWORD dwNotifyConnId;
    HANDLE hServicingThreadStop;
    HANDLE hNotificationThreadStop;
    HANDLE hServicingThread;
    HANDLE hNotificationThread;
    CRITICAL_SECTION cs_notify;
	enum {RULE_NOT_FOUND=0};

    friend DWORD WINAPI ServicingThread(LPVOID lpService);
    friend DWORD WINAPI NotificationThread(LPVOID lpService);
    // private
    int FindMatchingRule(const MINFILTER_NOTIFICATION* pNotification, CRule& rule);
    bool ProcessServiceCommand(DWORD connectionId, CData* pInputData = NULL);

    // ISimplifiedCallBack helpers

    int ProcessLoadNotify(MINFILTER_NOTIFICATION* pNotification, bool bLoad);
    int FS_Notify(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply);
    int FS_PrepareReply(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule);
    bool FS_Deny(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule);
    bool FS_Emulate(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule);
    // helpers
    std::wstring ToNative(const std::wstring& path);
    std::wstring ToDos(const std::wstring& path);
    bool CreateComplexDirectory(const wchar_t* szPath);
    bool FileNameCmp(const wchar_t* szFullName, const wchar_t* szShortName);

public:

     CService();
    ~CService();
    // ISimplifiedCallBack

    bool IsOperationImplemented(const MINFILTER_NOTIFICATION* pNotification);
    int GenericCallback(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply);

    // For service class users
	bool isFilteringActive();
    bool ToggleFiltering(int nThreads = 1);
    bool LoadRules();
};
