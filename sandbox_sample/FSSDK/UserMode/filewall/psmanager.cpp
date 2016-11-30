#include "stdafx.h"
#include "psmanager.h"
#include "psapi.h"

TProcessData CProcessManager::GetProcessByPid(DWORD pid)
{
    auto_critical_section_lock lock(&cs);
	return ParseProcess(pid);
}
TProcessData CProcessManager::ParseProcess(DWORD pid)
{
    WCHAR szProcessName[MAX_PATH] = L"<unknown>";

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, pid );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if(GetModuleFileNameExW( hProcess, NULL, szProcessName,
                sizeof(szProcessName)/sizeof(WCHAR) ))
		{
			TProcessData data;
			data.name = szProcessName;
			data.pid = pid;
			AddProcess(data);
			CloseHandle( hProcess );
			return  data;
		}
    }
    CloseHandle( hProcess );
    TProcessData emptydata;
    emptydata.pid = 0xFFFFFFFF;
    return  emptydata;
 }
void CProcessManager::InitManager()
{
    InitializeCriticalSection(&cs);
	// Get the list of process identifiers.
	auto_critical_section_lock lock(&cs);
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return;

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Set the name and process identifier for each process.

    for ( i = 0; i < cProcesses; i++ )
    {
        DWORD pid = aProcesses[i];
        ParseProcess( pid );
    }
    return ;
}
void CProcessManager::AddProcess(TProcessData data)
{
	for (int i =0 ; i < processes.size(); i++)
    {
        if (processes[i].pid == data.pid) return;
    }
    this->processes.push_back(data);
}
void CProcessManager::RemoveProcess(int index)
{
    auto_critical_section_lock lock(&cs);
	if (index < processes.size() && index >=0) processes.erase(processes.begin()+index);
}
TProcessData CProcessManager::GetProcessData(int index)
{
    auto_critical_section_lock lock(&cs);
	return processes[index];
}
int CProcessManager::GetProcessCount()
{
    return processes.size();
}
CProcessManager* CProcessManager::GetProcessManager()
{
    if (!pManager)
    {
        pManager = new CProcessManager;
    }
    return pManager;
}
CProcessManager::CProcessManager()
{
	InitManager();
}
CProcessManager::~CProcessManager()
{
     DeleteCriticalSection(&cs);
     pManager = NULL;
}
CProcessManager* CProcessManager::pManager = NULL;