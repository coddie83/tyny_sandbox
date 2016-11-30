// Sandbox.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <WinIoCtl.h>
#include <Ntsecapi.h>
#include <Dbt.h>
#include "rulemng.h"
#include "psmanager.h"
#include "callbackimpl.h"

#define SERVICE_NAME L"Sandbox"

SERVICE_STATUS          SandboxServiceStatus;
SERVICE_STATUS_HANDLE   SandboxServiceStatusHandle;

VOID SvcDebugOut(LPSTR String, DWORD Status);
VOID  WINAPI SandboxServiceCtrlHandler (DWORD opcode);
VOID WINAPI  SandboxServiceStart (DWORD argc, LPTSTR *argv);
DWORD SandboxServiceInitialization (DWORD argc, LPTSTR *argv,
							   DWORD *specificError);

CService* pService = NULL;

DWORD  Main();

VOID
SetError (
		  DWORD Code, char* szString
		  )

		  /*++

		  Routine Description:

		  This routine will display an error message based off of the Win32 error
		  code that is passed in. This allows the user to see an understandable
		  error message instead of just the code.

		  Arguments:

		  Code - The error code to be translated.

		  Return Value:

		  None.

		  --*/

{
	CHAR                                    buffer[80] ;
	DWORD                                    count ;

	//
	// Translate the Win32 error code into a useful message.
	//

	count = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		Code,
		0,
		buffer,
		sizeof( buffer )/sizeof( CHAR ),
		NULL) ;

	//
	// Make sure that the message could be translated.
	//

	if (count == 0) {
		printf("\nError could not be translated.\n Code: %d\n", Code) ;
		sprintf(szString,"Error could not be translated\n");
		return;
	}
	else {
		//
		// Display the translated error.
		//

		sprintf(szString,"Status : %s\n", buffer);
		return;
	}
}

bool RegisterService(LPCWSTR szFile)
{
	SC_HANDLE hSCM =  OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (!hSCM) return false;

	SC_HANDLE hService = CreateService(
		hSCM,
		SERVICE_NAME,
		L"Sample Service",
		SC_MANAGER_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		szFile,
		NULL,NULL,NULL,NULL,NULL);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		return FALSE;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	SERVICE_TABLE_ENTRY   DispatchTable[] =
	{
		{ SERVICE_NAME, SandboxServiceStart      },
		{ NULL,              NULL          }
	};

	if (argc > 1)
	{
		if (!wcsicmp(argv[1],L"/i"))
		{
			// install service and exit
			RegisterService(argv[0]);
			return 0;
		}

		if (!wcsicmp(argv[1],L"/debug"))
		{
			// debug mode -- run not as service, but do all the service's work anyway
			//TODO: remove this mode in release!
			Main();
			return 0;
		}
		return 0;
	}

	if (!StartServiceCtrlDispatcher( DispatchTable))
	{
		SvcDebugOut(" [Sandbox_SERVICE] StartServiceCtrlDispatcher (%d)\n",
			GetLastError());
	}

	return 0;
}
VOID SvcDebugOut(LPSTR String, DWORD Status)
{
	CHAR  Buffer[1024];
	if (strlen(String) < 1000)
	{
		sprintf(Buffer, String, Status);
		OutputDebugStringA(Buffer);
	}
}

VOID WINAPI SandboxServiceStart (DWORD argc, LPTSTR *argv)
{
	DWORD status;
	DWORD specificError;

	SandboxServiceStatus.dwServiceType        = SERVICE_WIN32;
	SandboxServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
	SandboxServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_PAUSE_CONTINUE;
	SandboxServiceStatus.dwWin32ExitCode      = 0;
	SandboxServiceStatus.dwServiceSpecificExitCode = 0;
	SandboxServiceStatus.dwCheckPoint         = 0;
	SandboxServiceStatus.dwWaitHint           = 0;

	SandboxServiceStatusHandle = RegisterServiceCtrlHandler(
		SERVICE_NAME,
		SandboxServiceCtrlHandler);

	if (SandboxServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
	{
		SvcDebugOut(" [Sandbox_SERVICE] RegisterServiceCtrlHandler 	failed %d\n", GetLastError());
		return;
	}

	// Initialization code goes here.
	status = SandboxServiceInitialization(argc,argv, &specificError);

	// Handle error condition
	if (status != NO_ERROR)
	{
		SandboxServiceStatus.dwCurrentState       = SERVICE_STOPPED;
		SandboxServiceStatus.dwCheckPoint         = 0;
		SandboxServiceStatus.dwWaitHint           = 0;
		SandboxServiceStatus.dwWin32ExitCode      = status;
		SandboxServiceStatus.dwServiceSpecificExitCode = specificError;

		SetServiceStatus (SandboxServiceStatusHandle, &SandboxServiceStatus);
		return;
	}

	// Initialization complete - report running status.
	SandboxServiceStatus.dwCurrentState       = SERVICE_RUNNING;
	SandboxServiceStatus.dwCheckPoint         = 0;
	SandboxServiceStatus.dwWaitHint           = 0;

	if (!SetServiceStatus (SandboxServiceStatusHandle, &SandboxServiceStatus))
	{
		status = GetLastError();
		SvcDebugOut(" [Sandbox_SERVICE] SetServiceStatus error	%ld\n",status);
	}

	// This is where the service does its work.
	Main();

	SandboxServiceStatus.dwCurrentState       = SERVICE_STOPPED;
	SandboxServiceStatus.dwCheckPoint         = 0;
	SandboxServiceStatus.dwWaitHint           = 0;
	SandboxServiceStatus.dwWin32ExitCode      = status;
	SandboxServiceStatus.dwServiceSpecificExitCode = specificError;
	SetServiceStatus (SandboxServiceStatusHandle, &SandboxServiceStatus);

	//////////////////////////////////////////////////////////////////////////
	SvcDebugOut(" [Sandbox_SERVICE] Returning the Main Thread \n",0);

	return;
}

// Stub initialization function.
DWORD SandboxServiceInitialization(DWORD   argc, LPTSTR  *argv,
							  DWORD *specificError)
{
	argv;
	argc;
	specificError;
	return(0);
}
VOID WINAPI SandboxServiceCtrlHandler (DWORD Opcode)
{
	DWORD status;

	switch(Opcode)
	{
	case SERVICE_CONTROL_PAUSE:
		// Do whatever it takes to pause here.
		if (pService)
		{
			if (pService->isFilteringActive())		pService->ToggleFiltering(0);
		}
		SandboxServiceStatus.dwCurrentState = SERVICE_PAUSED;
		break;

	case SERVICE_CONTROL_CONTINUE:
		// Do whatever it takes to continue here.
		// TODO: set appropriate number of threads here
		if (pService)	if (!pService->isFilteringActive())		pService->ToggleFiltering(1);
		SandboxServiceStatus.dwCurrentState = SERVICE_RUNNING;
		break;

	case SERVICE_CONTROL_STOP:
		// Do whatever it takes to stop here.
		if (pService)
		{
			if (pService->isFilteringActive())		pService->ToggleFiltering(0);
		}
		SandboxServiceStatus.dwWin32ExitCode = 0;
		SandboxServiceStatus.dwCurrentState  = SERVICE_STOPPED;
		SandboxServiceStatus.dwCheckPoint    = 0;
		SandboxServiceStatus.dwWaitHint      = 0;

		if (!SetServiceStatus (SandboxServiceStatusHandle,
			&SandboxServiceStatus))
		{
			status = GetLastError();
			SvcDebugOut(" [Sandbox_SERVICE] SetServiceStatus error %ld\n",
				status);
		}

		SvcDebugOut(" [Sandbox_SERVICE] Leaving SandboxService \n",0);
		return;

	case SERVICE_CONTROL_INTERROGATE:
		// Fall through to send current status.
		break;

	default:
		SvcDebugOut(" [Sandbox_SERVICE] Unrecognized opcode %ld\n",
			Opcode);
	}

	// Send current status.
	if (!SetServiceStatus (SandboxServiceStatusHandle,  &SandboxServiceStatus))
	{
		status = GetLastError();
		SvcDebugOut(" [Sandbox_SERVICE] SetServiceStatus error %ld\n",
			status);
	}
	return;
}

// This is where service does it's work after initialization.
// When this function terminates, service main thread exits.
DWORD Main()
{
	if (!pService)
	{
		pService = new CService();
	}

	if (!pService->LoadRules())
	{
		SvcDebugOut(" [Sandbox_SERVICE] Failed to load rules",0);
	}
	HANDLE hThread ;

	if (!pService->ToggleFiltering(5))
	{
		SvcDebugOut(" [Sandbox_SERVICE] Failed to activate filtering. This error  is fatal",0);
		delete pService;
		return FALSE;
	}
	// wait until servicing thread terminates
	while (true)
	{
		Sleep(INFINITE);
	}

	SvcDebugOut(" [Sandbox_SERVICE] Terminating !",0);
	delete pService;
	pService = NULL;

	return 0;
}