#pragma  once
#include <string>
#include <vector>
struct TProcessData
{
    std::wstring name;
    DWORD pid;
};
class CProcessManager
{
	CRITICAL_SECTION cs;

public:
     CProcessManager();
     ~CProcessManager();
     void InitManager();
     int GetProcessCount();
     TProcessData GetProcessData(int index);
     void AddProcess(TProcessData data);
     void RemoveProcess(int index);
     static CProcessManager* GetProcessManager();
     TProcessData  GetProcessByPid(DWORD pid);
private:
    TProcessData ParseProcess(DWORD pid);
    std::vector<TProcessData> processes;
    static CProcessManager* pManager;
};
