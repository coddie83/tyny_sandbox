#pragma once
#include "../../Common/structs.h"
class ICallbacks
{
public:
	virtual bool IsOperationImplemented(const MINFILTER_NOTIFICATION* pNotification) = 0;

	// File System
	virtual int FS_Create(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int FS_Close(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int FS_Erase(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int FS_Load(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int FS_FindFile(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;

	// Registry

	virtual int REG_NtDeleteKey(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int REG_NtDeleteValueKey(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int REG_NtCreateKeyEx(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int REG_NtOpenKeyEx(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int REG_NtRenameKey(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int REG_NtSetValueKey(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;

	// Process manager events

	virtual int PS_Load(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
	virtual int PS_Unload(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
};

class ISimplifiedCallbacks
{
public:
	virtual bool IsOperationImplemented(const MINFILTER_NOTIFICATION* pNotification) = 0;
	virtual int GenericCallback(MINFILTER_NOTIFICATION* pNotification, MINFILTER_REPLY* pReply) = 0;
};
