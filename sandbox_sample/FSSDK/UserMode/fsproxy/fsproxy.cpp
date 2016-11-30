// fsproxy.cpp : Defines the entry point for the DLL application.
//

//#include "stdafx.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>
#include <fltuser.h>
#include "../../Common/structs.h"

#include <dontuse.h>
#include "fsproxy.h"
#define SERVICE_NAME   L"minflt"
#define SERVICE_ACCESS (STANDARD_RIGHTS_REQUIRED | \
	SERVICE_QUERY_CONFIG | \
	SERVICE_QUERY_STATUS | \
	SERVICE_START | SERVICE_STOP)

struct MINFILTER_MESSAGE
{
	//
	//  Required structure header.
	//

	FILTER_MESSAGE_HEADER MessageHeader;

	//
	//  Private scanner-specific fields begin here.
	//

	MINFILTER_NOTIFICATION Notification;

	//
	//  Overlapped structure: this is not really part of the message
	//  However we embed it instead of using a separately allocated overlap structure
	//

	OVERLAPPED Ovlp; // MUST ALWAYS BE LAST FIELD - i.e after message header + Notification, since is used as size
} ;

typedef struct _MINFILTER_REPLY_MESSAGE {
	//
	//  Required structure header.
	//

	FILTER_REPLY_HEADER ReplyHeader;

	//
	//  Private scanner-specific fields begin here.
	//

	MINFILTER_REPLY Reply;
} MINFILTER_REPLY_MESSAGE, *PMINFILTER_REPLY_MESSAGE;

#ifdef _MANAGED
#pragma managed(push, off)
#endif
class CSetup;
DWORD WINAPI
WorkerThread(
			 __in CSetup* Context
			 );

class CSetup : public ISetup
{
friend DWORD WINAPI	WorkerThread(__in CSetup* Context);
private:
	bool bFilteringStatus;
	volatile ULONG nActiveThreads;
	ICallbacks* pCallBacks;
	ISimplifiedCallbacks* pSimplifiedCallBacks;
	HANDLE Port;
	HANDLE Completion;
	volatile SHORT dwTerminateThreads;

	bool FilteringThread(MINFILTER_MESSAGE* message)
	{
		MINFILTER_NOTIFICATION* notification;
		MINFILTER_REPLY_MESSAGE replyMessage;
		if (pCallBacks == NULL && pSimplifiedCallBacks == NULL )
		{
			AbandonThreads();

			return TRUE;
		}

		notification = &message->Notification;

		replyMessage.ReplyHeader.Status = 0;
		replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;

		// setting  default settings to allow the operation
		replyMessage.Reply.bAllow = true;
		replyMessage.Reply.bSupersedeFile = false;
		if (pSimplifiedCallBacks)
		{
			if (pSimplifiedCallBacks->IsOperationImplemented(notification)) pSimplifiedCallBacks->GenericCallback(notification,&(replyMessage.Reply));
		}
		else if (pCallBacks)
		{
			if (pCallBacks->IsOperationImplemented(notification))
			{
				if (notification->iComponent == COM_FILE && notification->Operation == CREATE) pCallBacks->FS_Create(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_FILE && notification->Operation == CLOSE) pCallBacks->FS_Close(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_FILE && notification->Operation == ERASE) pCallBacks->FS_Erase(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_FILE && notification->Operation == FIND_FILE) pCallBacks->FS_FindFile(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_FILE && notification->Operation == LOAD) pCallBacks->FS_Load(notification,&(replyMessage.Reply));

				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtCreateKeyEx)
					pCallBacks->REG_NtCreateKeyEx(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtDeleteKey)
					pCallBacks->REG_NtDeleteKey(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtDeleteValueKey)
					pCallBacks->REG_NtDeleteValueKey(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtOpenKeyEx)
					pCallBacks->REG_NtOpenKeyEx(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtRenameKey)
					pCallBacks->REG_NtRenameKey(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_REGISTRY && notification->Operation == ROP_NtSetValueKey)
					pCallBacks->REG_NtSetValueKey(notification,&(replyMessage.Reply));

				if (notification->iComponent == COM_PROCESS_NOTIFY && notification->Operation == LOAD) pCallBacks->PS_Load(notification,&(replyMessage.Reply));
				if (notification->iComponent == COM_PROCESS_NOTIFY && notification->Operation == UNLOAD) pCallBacks->PS_Unload(notification,&(replyMessage.Reply));
			}
		}

		HRESULT hr = FilterReplyMessage( Port,
			(PFILTER_REPLY_HEADER) &replyMessage,
			sizeof( FILTER_REPLY_HEADER ) + sizeof(MINFILTER_REPLY ));

		if (!SUCCEEDED( hr ))
		{
			return FALSE;
       }

		return TRUE;
	}
	void AbandonThreads()
	{
		InterlockedAnd16(&dwTerminateThreads,0);
		InterlockedOr16(&dwTerminateThreads,1);
	}
public:
	CSetup()
	{
		bFilteringStatus = FALSE;
		pCallBacks = NULL;
		pSimplifiedCallBacks = NULL;
		nActiveThreads = 0;
		Port =NULL;
		Completion = NULL;
	}
	ULONG ThreadsNumber()
	{
		return nActiveThreads;
	}
	int DoFiltering(bool bStartFiltering, int iThreadCount)
	{
		if (!bStartFiltering)
		{
			AbandonThreads();
			while (nActiveThreads);

			return TRUE;
		}
		if (pCallBacks == NULL && pSimplifiedCallBacks == NULL) return FALSE;
		InterlockedAnd16(&dwTerminateThreads,0);
		HRESULT hr = FilterConnectCommunicationPort( ScannerPortName,
			0,
			NULL,
			0,
			NULL,
			&Port );

		if (IS_ERROR( hr )) {
			return FALSE;
		}

		//
		//  Create a completion port to associate with this handle.
		//
		Completion = CreateIoCompletionPort( Port,
			NULL,
			0,
			iThreadCount );

		if (Completion == NULL) {
			CloseHandle( Port );
			Port = NULL;
			return FALSE;
		}

		//
		//  Create specified number of threads.
		//
		DWORD threadId;

		for (int i = 0; i < iThreadCount; i++)
		{
			HANDLE hThread = CreateThread( NULL,
				0,
				(LPTHREAD_START_ROUTINE)WorkerThread,
				this,
				0,
				&threadId );

			if (hThread == NULL) {
				InterlockedOr16(&dwTerminateThreads,1);
				return FALSE;
			}
			MINFILTER_MESSAGE* msg = (MINFILTER_MESSAGE*)malloc( sizeof( MINFILTER_MESSAGE ) );

			if (msg == NULL) {
				hr = ERROR_NOT_ENOUGH_MEMORY;
				InterlockedOr16(&dwTerminateThreads,1);
				return FALSE;
			}

			memset( &msg->Ovlp, 0, sizeof( OVERLAPPED ) );

			//
			//  Request messages from the filter driver.
			//

			hr = FilterGetMessage( Port,
				&msg->MessageHeader,
				FIELD_OFFSET( MINFILTER_MESSAGE, Ovlp ),
				&msg->Ovlp );

			if (hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING )) {
				InterlockedOr16(&dwTerminateThreads,1);
				return FALSE;
			}
		}
		bFilteringStatus = TRUE;
		return TRUE;
	}
	bool FilteringStatus()
	{
		return bFilteringStatus;
	}
	int setCallbacks(ICallbacks* _pCallBacks)
	{
		this->pCallBacks = _pCallBacks;
		return 0;
	}
	int setSimplifiedCallbacks(ISimplifiedCallbacks* _pCallBacks)
	{
		this->pSimplifiedCallBacks = _pCallBacks;
		return 0;
	}
	bool LoadDriver()
	{
		SERVICE_STATUS_PROCESS  serviceInfo;
		DWORD                   bytesRequired;

		SC_HANDLE hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;
		if (NULL == hSCManager) {
			return FALSE;
		}

		SC_HANDLE hService = OpenServiceW( hSCManager,
			SERVICE_NAME,
			SERVICE_ACCESS);

		if (NULL == hService) {
			return FALSE;
		}

		if (!QueryServiceStatusEx( hService,
			SC_STATUS_PROCESS_INFO,
			(UCHAR *)&serviceInfo,
			sizeof(serviceInfo),
			&bytesRequired))
		{
			return FALSE;
		}

		if(serviceInfo.dwCurrentState != SERVICE_RUNNING)
		{
			//
			// Service hasn't been started yet, so try to start service
			//
			if (!StartService(hService, 0, NULL)) {
				return FALSE;
			}
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return TRUE;
	}
	bool UnloadDriver()
	{
		SERVICE_STATUS_PROCESS  serviceInfo;
		DWORD                   bytesRequired;

		SC_HANDLE hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;
		if (NULL == hSCManager) {
			return FALSE;
		}

		SC_HANDLE hService = OpenServiceW( hSCManager,
			SERVICE_NAME,
			SERVICE_ACCESS);

		if (NULL == hService) {
			return FALSE;
		}

		if (!QueryServiceStatusEx( hService,
			SC_STATUS_PROCESS_INFO,
			(UCHAR *)&serviceInfo,
			sizeof(serviceInfo),
			&bytesRequired))
		{
			return FALSE;
		}

		if(serviceInfo.dwCurrentState == SERVICE_RUNNING)
		{
			//
			// Service hasn't been started yet, so try to start service
			//
			SERVICE_STATUS ServStatus;
			if (!ControlService(hService,SERVICE_CONTROL_STOP, &ServStatus)) {
				return FALSE;
			}
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return TRUE;
	}
};
static CSetup Setup;
DWORD WINAPI
WorkerThread(
			 __in CSetup* Context
			 )
{
	LPOVERLAPPED pOvlp;
	DWORD outSize;
	HRESULT hr;
	ULONG_PTR key;

	MINFILTER_MESSAGE* message = NULL;

	InterlockedIncrement(&Context->nActiveThreads);
	while (InterlockedCompareExchange16(&(Context->dwTerminateThreads),1,1) == 0)
	{
		BOOL bResult = GetQueuedCompletionStatus(Context->Completion, &outSize, &key, &pOvlp, INFINITE );
		if (!bResult)
		{
			break;
		}

		//
		//  Obtain the message: note that the message we sent down via FltGetMessage() may NOT be
		//  the one dequeued off the completion queue: this is solely because there are multiple
		//  threads per single port handle. Any of the FilterGetMessage() issued messages can be
		//  completed in random order - and we will just dequeue a random one.
		//
		message = CONTAINING_RECORD( pOvlp, MINFILTER_MESSAGE, Ovlp );

			if (!Context->FilteringThread(message))
			{
				break;
			}

			memset( &message->Ovlp, 0, sizeof( OVERLAPPED ) );

			hr = FilterGetMessage(Context->Port,
				(PFILTER_MESSAGE_HEADER)message,
				sizeof(MINFILTER_NOTIFICATION ) + sizeof(FILTER_MESSAGE_HEADER),
				&message->Ovlp );

			if (!SUCCEEDED(hr))
				if (hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ))
				{
					break;
				}
	}
	InterlockedDecrement(&Context->nActiveThreads);
	if (Context->nActiveThreads <=0 )
	{
		if (Context->Port) CloseHandle(Context->Port);
		if (Context->Completion) CloseHandle(Context->Completion);
		Context->Port = NULL;
		Context->Completion = NULL;
		Context->bFilteringStatus = FALSE;
	}
	if (message)
	{
		free(message);
		message =NULL;
	}
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

ISetup* __stdcall Init()
{
	return &Setup;
}