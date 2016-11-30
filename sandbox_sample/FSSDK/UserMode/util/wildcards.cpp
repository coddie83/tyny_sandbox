#include "stdafx.h"
#include "wildcards.h"

#pragma warning (disable: 4996) // unsafe strings function warnings

bool CWildCard::Match( const wchar_t* szExpression, const wchar_t* szSample, CState* pAutomation /*= NULL*/, int index /*= 0 */ )
{
	// build graph
	size_t exlen = wcslen(szExpression);
	size_t samplelen =  wcslen(szSample);

	CState* p;
	CState* pGraph = pAutomation;

	if (!pAutomation)
	{
		pGraph = new CState;
		pGraph->bFin = false;
		p = pGraph;
		for (size_t i = 0; i < exlen; i++)
		{
			CPair pair;
			if (szExpression[i] != L'*')
			{
				pair.ch = szExpression[i];

				// new state we can move by ch
				CState* pNextState = new CState;
				pNextState->bFin = (i == exlen-1);

				pair.pState = pNextState;
				p->pairs.push_back(pair);
				// get to next state
				p= pNextState;
			}
			else if (szExpression[i]==L'*')
			{
				CState* pNextState = p;
				pair.ch = szExpression[i];
				// new state we can move by ch
				pair.pState = pNextState;
				p->pairs.push_back(pair);
				pNextState->bFin = (i == exlen-1);
				// don't get to next state
			}
		}
	}

	// check over the pattern!
	bool bResult = false;
	CState* pState = pGraph;
	if (pState->bFin && index ==  samplelen)
	{
		bResult = true;
	}

	for (size_t i = index; i < samplelen; i++)
	{
		if (bResult) break;
		wchar_t ch = szSample[i];
		bool bShifted = false;
		for (int j = 0; j < (int)pState->pairs.size() ; j++)
		{
			wchar_t reg_char = pState->pairs[j].ch;
			if ((ch) == (reg_char) || reg_char == L'*' || reg_char == L'?')
			{
				if (Match(szExpression,szSample,pState->pairs[j].pState,(int)i+1))
				{
					// if any pair matches , break!
					bResult = true;
					bShifted = true;
					break;
				}
			}
		}
		if (!bShifted)
		{
			bResult = false;
			break;
		}
		if (bResult) break;
	}

	// delete automation
	if (!pAutomation) delete pGraph;

	return bResult;
}

bool CWildCard::match( const wchar_t* szExpression, const wchar_t* szSample,bool* pAbsMatch )
{
	wchar_t* szExpressionL = new wchar_t[wcslen(szExpression)+1];
    wchar_t* szSampleL = new wchar_t[wcslen(szSample)+1];
    wcscpy(szSampleL,szSample);
    wcscpy(szExpressionL,szExpression);
    CharLower(szSampleL);
	CharLower(szExpressionL);
	bool bResult = (!StrCmpI(szExpressionL,szSampleL));
	if (bResult)
	{
		delete [] szExpressionL;
		delete [] szSampleL;
		if (pAbsMatch) *pAbsMatch = TRUE;
		return bResult;
	}

    for (size_t i =0; i < wcslen(szExpressionL); i++)
    {
        if (szExpression[i] == L'?' || szExpression[i] == L'*')
        {
            bool bResult = Match(szExpressionL,szSampleL,NULL,0);
            delete [] szExpressionL;
            delete [] szSampleL;
            if (pAbsMatch) *pAbsMatch = FALSE;
			return bResult;
        }
    }

    delete [] szExpressionL;
    delete [] szSampleL;
    return bResult;
}