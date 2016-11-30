#pragma once
#include "../util/util.h"
#include <vector>
#include <iostream>
#include <fstream>

class CServerSideRuleManager : public IRuleManager
{
private:

	CRITICAL_SECTION cs;
	std::vector<CRule> rules;

public:
	CServerSideRuleManager();
	~CServerSideRuleManager();
	size_t AddRule(const CRule& rule);
	bool DeleteRule(int index);
	CRule GetRule(int index);
	size_t GetRuleCount();
};
IRuleManager* GetRuleManager();