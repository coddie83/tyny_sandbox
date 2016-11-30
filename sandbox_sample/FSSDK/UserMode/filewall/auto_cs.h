#pragma  once
//#include "util.h"
#include <vector>

#define GUID_LENGTH 38 // GUID length considering braces

class auto_critical_section_lock
{
	static const int csSpinCount = 1000;
	PCRITICAL_SECTION pCriticalSection;
private:
	auto_critical_section_lock()
	{
		// forbidden empty constructor
	}

public:
	auto_critical_section_lock(PCRITICAL_SECTION p)
	{
		pCriticalSection = p;
		EnterCriticalSection(pCriticalSection);
	}
	~auto_critical_section_lock()
	{
		LeaveCriticalSection(pCriticalSection);
	}
};
