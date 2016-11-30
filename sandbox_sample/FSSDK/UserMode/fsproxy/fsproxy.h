// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FSPROXY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FSPROXY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#define FSPROXY_API extern "C"

#include "callbacks.h"
#ifndef ISETUP_DEFINED
#define ISETUP_DEFINED
class ISetup
{
public:
	virtual int   setCallbacks(ICallbacks* pCallbacks) = 0 ;
	virtual int   setSimplifiedCallbacks(ISimplifiedCallbacks* pCallbacks) =0;
	virtual int   DoFiltering(bool bStartFiltering, int iThreadsCount) = 0;
	virtual bool  FilteringStatus() = 0;
	virtual ULONG ThreadsNumber() = 0;
	virtual bool LoadDriver() = 0;
	virtual bool UnloadDriver() = 0;
};
#endif

FSPROXY_API ISetup* __stdcall Init();
