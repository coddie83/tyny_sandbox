#include "stdafx.h"
#include "rulemng.h"
#include <ShlObj.h>
CServerSideRuleManager::CServerSideRuleManager()
{
	InitializeCriticalSection(&cs);
}
CServerSideRuleManager::~CServerSideRuleManager()
{
	DeleteCriticalSection(&cs);
}
size_t CServerSideRuleManager::AddRule(const CRule& rule)
{
	static DWORD ruleId = 0;
	EnterCriticalSection(&cs);
	bool bRuleFound = false;
	for (int i  = 0; i < rules.size(); i++)
	{
		if (rules[i]==rule)
		{
			LeaveCriticalSection(&cs);
			return i;
		}
	}

	rules.push_back(rule);
	LeaveCriticalSection(&cs);
	return rules.size()-1;
}
bool CServerSideRuleManager::DeleteRule(int index)
{
	EnterCriticalSection(&cs);
	if (rules.size() <= index)
	{
		LeaveCriticalSection(&cs);
		return false;
	}
	rules.erase(rules.begin()+index);
	LeaveCriticalSection(&cs);
	return true;
}
CRule CServerSideRuleManager::GetRule(int index)
{
	EnterCriticalSection(&cs);
    CRule result = rules[index];
    LeaveCriticalSection(&cs);
    return result;
}
size_t CServerSideRuleManager::GetRuleCount()
{
	EnterCriticalSection(&cs);
    int rulesCount = rules.size();
    LeaveCriticalSection(&cs);
    return rulesCount;
}
static CServerSideRuleManager ruleManager;
IRuleManager* GetRuleManager()
{
	return &ruleManager;
}