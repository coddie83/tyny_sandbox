#pragma  once
#include <vector>
#include <string>

class CWildCard
{
	struct CState;
	struct CPair
	{
		wchar_t ch;
		CState* pState;
	};

	struct CState
	{
		bool bFin;
		std::vector<CPair> pairs;
		~CState()
		{
			for (size_t i = 0 ; i < pairs.size(); i++)
			{
				if (pairs[i].pState != this) delete pairs[i].pState;
			}
			pairs.clear();
		}
	};
    bool Match(const wchar_t* szExpression, const wchar_t* szSample, CState* pAutomation = NULL, int index = 0 );
public:
    bool match(const wchar_t* szExpression, const wchar_t* szSample,bool* pAbsMatch);
};
