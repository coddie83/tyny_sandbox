#include "stdafx.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "callbackimpl.h"

///
CService::CService()
{
    bFilteringStarted = FALSE;
    dwNotifyConnId = 0;
    InitializeCriticalSection(&cs_notify);
    this->pSetup = Init();
    this->pProcessManager = new CProcessManager;
    this->pRuleManager = GetRuleManager();
    pSetup->setSimplifiedCallbacks(this);
    hServicingThreadStop = CreateEvent(NULL,TRUE,TRUE,NULL);
    hNotificationThreadStop = CreateEvent(NULL,TRUE,TRUE,NULL);
    hServicingThread = hNotificationThread = NULL;
}
CService::~CService()
{
    DeleteCriticalSection(&cs_notify);
	if (bFilteringStarted)
    {
        pSetup->DoFiltering(FALSE,0);
		pSetup->UnloadDriver();
    }
    delete pProcessManager;
    CloseHandle(hNotificationThreadStop);
    CloseHandle(hServicingThreadStop);
}
bool CService::LoadRules()
{
	CRule rule;
	ZeroMemory(&rule, sizeof(rule));
	rule.dwAction = emulate;
	wcscpy(rule.ImageName,L"cmd.exe");
	rule.GenericNotification.iComponent = COM_FILE;
	rule.GenericNotification.Operation = CREATE;
	wcscpy(rule.GenericNotification.wsFileName,L"\\Device\\Harddisk*\\*.txt");
	wcscpy(rule.SandBoxRoot,L"C:\\Sandbox");
	GetRuleManager()->AddRule(rule);
	return true;
}
bool CService::ToggleFiltering( int nThreads /*= 1*/ )
{
	if (!bFilteringStarted)
	{
		if (!pSetup->LoadDriver()) return false;
		if (!pSetup->DoFiltering(TRUE,nThreads)) return false;
	}
	else
	{
		if (!pSetup->DoFiltering(FALSE,0)) return false;
		if (!pSetup->UnloadDriver()) return false;
	}
	bFilteringStarted = !bFilteringStarted;
	return true;
}
bool CService::isFilteringActive()
{
    return bFilteringStarted;
}
bool CService::IsOperationImplemented(const MINFILTER_NOTIFICATION* pNotification)
{
    // TODO: define implemented operation explicitly to maintain compatibility with future versions of driver
    //       where another messages may be introduced

    return TRUE;
}
int CService::GenericCallback(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply)
{
	if (pNotification->iComponent == COM_PROCESS_NOTIFY && pNotification->Operation == LOAD) return ProcessLoadNotify(pNotification, TRUE);
    if (pNotification->iComponent == COM_PROCESS_NOTIFY && pNotification->Operation == UNLOAD) return ProcessLoadNotify(pNotification, FALSE);
	if (pNotification->iComponent == COM_FILE) return FS_Notify(pNotification,pReply);

    return TRUE;
}

int CService::ProcessLoadNotify(MINFILTER_NOTIFICATION* pNotification, bool bLoad)
{
    CProcessManager* pManager = CProcessManager::GetProcessManager();

    DWORD pid = (DWORD)pNotification->childPid;
    TProcessData data = pManager->GetProcessByPid(pid);
    if (!bLoad)
    {
        for (int i =0; i < pManager->GetProcessCount(); i++)
        {
            if (pManager->GetProcessData(i).pid == data.pid)
            {
                pManager->RemoveProcess(i);
                break;
            }
        }
    }

    return TRUE;
}

int CService::FindMatchingRule(const MINFILTER_NOTIFICATION* pNotification, CRule& Rule)
{
    int iMaxScore = 0;
    int iScore = 0;
    ZeroMemory(&Rule,sizeof(CRule));
    CRule curRule;
    curRule.dwAction = 0xFFFFFFFF;
    Rule.dwAction = 0xFFFFFFFF;
    bool bFoundExact = FALSE;
    iScore = 0;
    for (int i = 0; i < pRuleManager->GetRuleCount() ; i++)
    {
        if (iScore) return TRUE;
        CRule r = pRuleManager->GetRule(i);

        bool bMatchesInCommon =
            (pNotification->iComponent == r.GenericNotification.iComponent)
                                     &&
            (pNotification->Operation == r.GenericNotification.Operation);
        if (!r.pid)
        {
            if (wcslen(r.ImageName))
            {
                TProcessData data = CProcessManager::GetProcessManager()->GetProcessByPid((DWORD)pNotification->pid);
                bMatchesInCommon = bMatchesInCommon &&  (FileNameCmp(data.name.c_str(),r.ImageName));
            }
        }
        else
        {
            if (r.pid != (DWORD)pNotification->pid) continue;
        }
        if (bMatchesInCommon)
        {
            iScore++;
        }
        else
            continue;

         // TODO: Should I convert name in generic notification from dos name to native? Let's say rules are stored as dos names...
        if (wcslen(r.GenericNotification.wsFileName))
        {
			bMatchesInCommon =  bMatchesInCommon && match(r.GenericNotification.wsFileName,pNotification->wsFileName,NULL);
            if (bMatchesInCommon)
				iScore++;
			else
				iScore = 0;
        }
		if (iScore)
		{
			Rule = r;
		}
    }
    if (Rule.dwAction == 0xFFFFFFFF) return RULE_NOT_FOUND;
	return iScore;
}

int CService::FS_Notify( MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply )
{
	CRule rule;
    int iRetries = 0;

    int iResult = FindMatchingRule(pNotification,rule);
	if (iResult != RULE_NOT_FOUND)
	{
		FS_PrepareReply(pNotification,pReply,rule);
	}
	return TRUE;
}

int CService::FS_PrepareReply( MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule )
{
    switch(rule.dwAction)
    {
    case deny:
        return FS_Deny(pNotification,pReply,rule);
    case emulate:
        return FS_Emulate(pNotification,pReply,rule);
        break;
    }
    return TRUE;
}

bool CService::FS_Deny( MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule )
{
    pReply->bAllow = FALSE;
    return TRUE;
}

bool CService::FS_Emulate( MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply, const CRule& rule )
{
    using namespace std;
    // form new path
    // chek if path exists, if not - create/copy
	if (IsSandboxedFile(ToDos(pNotification->wsFileName).c_str(),rule.SandBoxRoot))
	{
		pReply->bSupersedeFile  = FALSE;
		pReply->bAllow = TRUE;
		return true;
	}
	wchar_t* originalPath = pNotification->wsFileName; // in native
    int iLen = GetNativeDeviceNameLen(originalPath);
    wstring relativePath;
    for (int i = iLen ; i < wcslen(originalPath); i++) relativePath += originalPath[i];
    wstring substitutedPath = ToNative(rule.SandBoxRoot) + relativePath;
    if (PathFileExists(ToDos(originalPath).c_str()))
    {
        if (PathIsDirectory(ToDos(originalPath).c_str()) )
        {
            // just an empty directory - simply create it in sandbox

            CreateComplexDirectory(ToDos(substitutedPath).c_str() );
        }
        else
        {
            // full file name provided - create a copy of the file in sandbox, if not already present

            wstring path = ToDos(substitutedPath);
            wchar_t* pFileName = PathFindFileName(path.c_str());
            int iFilePos = pFileName - path.c_str();
            wstring Dir;
            for (int i = 0; i< iFilePos-1; i++) Dir = Dir + path[i];

            CreateComplexDirectory(ToDos(Dir).c_str());
            CopyFile(ToDos(originalPath).c_str(),path.c_str(),TRUE);
        }
     }
    else
    {
		// no such file, but we have to create parent directory if not exists
		wstring path = ToDos(substitutedPath);
		wchar_t* pFileName = PathFindFileName(path.c_str());
		int iFilePos = pFileName - path.c_str();
		wstring Dir;
		for (int i = 0; i< iFilePos-1; i++) Dir = Dir + path[i];

		CreateComplexDirectory(ToDos(Dir).c_str());
    }
    wcscpy(pReply->wsFileName,substitutedPath.c_str());
    pReply->bSupersedeFile  = TRUE;
    pReply->bAllow = TRUE;
    return true;
}

std::wstring CService::ToDos(const std::wstring& path)
{
    wchar_t* szResult = new wchar_t[MAX_PATH];
    ZeroMemory(szResult,MAX_PATH*sizeof(wchar_t));
    NativeNameToDosName(path.c_str(),szResult,MAX_PATH);
    std::wstring  DosPath = szResult;
    delete [] szResult;
    return DosPath;
}
std::wstring CService::ToNative( const std::wstring& path )
{
    wchar_t* szResult = new wchar_t[MAX_PATH];
    ZeroMemory(szResult,MAX_PATH*sizeof(wchar_t));
    DosNameToNativeName(path.c_str(),szResult,MAX_PATH);
    std::wstring  NativePath = szResult;
    delete [] szResult;
    return NativePath;
}

bool CService::CreateComplexDirectory( const wchar_t* szPath )
{
    wchar_t* PathCopy =  new wchar_t[wcslen(szPath)+1];
    wchar_t* pPath = PathCopy;
    ZeroMemory(pPath,(wcslen(szPath)+1)*sizeof(wchar_t));
    wcscpy(pPath,szPath);
    while (*pPath)
    {
        while(*pPath != L'\\')
        {
            if (*pPath == NULL) break;
            pPath++;
        }
        if (*pPath == L'\\')
        {
            *pPath = NULL;
            CreateDirectory(PathCopy,0);
            *pPath=L'\\';
            pPath++;
        }
        else // *pPath == NULL
        {
            CreateDirectory(PathCopy,0);
        }
    }
    delete [] PathCopy;
    return true;
}

bool CService::FileNameCmp( const wchar_t* szFullName, const wchar_t* szShortName )
{
    if (!StrCmpI(szFullName,szShortName)) return true;
    if (!StrCmpI(PathFindFileName(szFullName),szShortName)) return true;
    return false;
}