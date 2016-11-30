#pragma warning(disable:4995)

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "..\..\Common\structs.h"
#include "miniflt.h"
#include <NtStrSafe.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  Structure that contains all the global data structures
//  used throughout the scanner.
//

MINFILTER_DATA MfltData;
ULONG ProcessNameOffset;

ULONG GetProcessNameOffset()
{
	PEPROCESS pSystemProcess = PsInitialSystemProcess;
	ULONG offset = 0;
	UCHAR* pName = (UCHAR*)pSystemProcess;
	UCHAR Tmp[17];
	do
	{
		memset(Tmp, 0, 17);
		memcpy(Tmp, pName, 16);
		if (!_strnicmp((const char*)Tmp, "SYSTEM", 16))
		{
			return pName - (UCHAR*)pSystemProcess;
		}
		pName++;
	} while (true);
	return 0;
}

//
//  Function prototypes
//

NTSTATUS
FSPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID
FSPortDisconnect (
    __in_opt PVOID ConnectionCookie
    );

//
//  Constant FLT_REGISTRATION structure for our filter.  This
//  initializes the callback routines our filter wants to register
//  for.  This is only used to register with the filter manager
//

const FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      FSPreCreate,
	  NULL
	},

    { IRP_MJ_CLEANUP,
      0,
      FSPreCleanup,
      NULL},

    { IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {
    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    NULL,                //  Context Registration.
    Callbacks,                          //  Operation callbacks
    DriverUnload,                      //  FilterUnload
    FSInstanceSetup,               //  InstanceSetup
    FSQueryTeardown,               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    FSGenerateFileNameCallback,     // GenerateFileName
    FSNormalizeNameComponentCallback,  // NormalizeNameComponent

    NULL,                                //  NormalizeContextCleanup
#if FLT_MGR_LONGHORN
	NULL,                // TransactionNotification
	FSNormalizeNameComponentExCallback, // NormalizeNameComponentEx
#endif // FLT_MGR_LONGHORN
};
LARGE_INTEGER regCookie;

struct NAMEINFO
{
    OBJECT_NAME_INFORMATION header;
    char buffer [MAX_DATA];
};
void CleanNotification(MINFILTER_NOTIFICATION* notification)
{
    memset(notification,0,sizeof(MINFILTER_NOTIFICATION));
}
void SetProcessName(PMINFILTER_NOTIFICATION notification, HANDLE pid = 0)
{
    PEPROCESS pProcess;

    if (!pid)
    {
        pProcess = IoGetCurrentProcess();
    }
    else
    {
       if (!NT_SUCCESS(PsLookupProcessByProcessId(pid,&pProcess))) return;
    }

    UCHAR* name = (UCHAR*)pProcess;
    name += ProcessNameOffset;
    if (!pid)
    {
        memset(notification->szImageName ,0,17);
        memcpy(notification->szImageName,name,16);
    }
    else
    {
        memset(notification->szChildName ,0,17);
        memcpy(notification->szChildName,name,16);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//    Filter initialization and unload routines.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
/*++()

Routine Description:

    This is the initialization routine for the Filter driver.  This
    registers the Filter with the filter manager and initializes all
    its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.
--*/
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;
    PSECURITY_DESCRIPTOR sd;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );
    ProcessNameOffset =  GetProcessNameOffset();
    DbgPrint("Loading driver");
    //
    //  Register with filter manager.
    //

	status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &MfltData.Filter );

    if (!NT_SUCCESS( status )) {
        DbgPrint("RegisterFilter failure 0x%x \n",status);
	return status;
    }

    //
    //  Create a communication port.
    //

    RtlInitUnicodeString( &uniString, ScannerPortName );

    //
    //  We secure the port so only ADMINs & SYSTEM can acecss it.
    //

    status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );

    if (NT_SUCCESS( status )) {
        InitializeObjectAttributes( &oa,
                                    &uniString,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    sd );

        status = FltCreateCommunicationPort( MfltData.Filter,
                                             &MfltData.ServerPort,
                                             &oa,
                                             NULL,
                                             FSPortConnect,
                                             FSPortDisconnect,
                                             NULL,
                                             1 );
        //
        //  Free the security descriptor in all cases. It is not needed once
        //  the call to FltCreateCommunicationPort() is made.
        //

        FltFreeSecurityDescriptor( sd );
        regCookie.QuadPart = 0;

        if (NT_SUCCESS( status )) {
            //
            //  Start filtering I/O.
            //
            DbgPrint(" Starting Filtering \n");
            status = FltStartFiltering( MfltData.Filter );

            if (NT_SUCCESS(status))
            {
                  status = PsSetCreateProcessNotifyRoutine(CreateProcessNotify,FALSE);
                   if (NT_SUCCESS(status))
                   {
                       DbgPrint(" All done! \n");
                       return STATUS_SUCCESS;
                   }
            }
            DbgPrint(" Something went wrong \n");
            FltCloseCommunicationPort( MfltData.ServerPort );
        }
    }

    FltUnregisterFilter( MfltData.Filter );
    if (regCookie.QuadPart)  CmUnRegisterCallback(regCookie);
    return status;
}

NTSTATUS
FSPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
/*++

Routine Description

    This is called when user-mode connects to the server port - to establish a
    connection

Arguments

    ClientPort - This is the client connection port that will be used to
        send messages from the filter

    ServerPortCookie - The context associated with this port when the
        minifilter created this port.

    ConnectionContext - Context from entity connecting to this port (most likely
        your user mode service)

    SizeofContext - Size of ConnectionContext in bytes

    ConnectionCookie - Context to be passed to the port disconnect routine.

Return Value

    STATUS_SUCCESS - to accept the connection

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie );

    ASSERT( MfltData.ClientPort == NULL );
    ASSERT( MfltData.UserProcess == NULL );

    //
    //  Set the user process and port.
    //

    MfltData.UserProcess = PsGetCurrentProcess();
    MfltData.ClientPort = ClientPort;
    return STATUS_SUCCESS;
}

VOID
FSPortDisconnect(
     __in_opt PVOID ConnectionCookie
     )
/*++

Routine Description

    This is called when the connection is torn-down. We use it to close our
    handle to the connection

Arguments

    ConnectionCookie - Context from the port connect routine

Return value

    None

--*/
{
    UNREFERENCED_PARAMETER( ConnectionCookie );

    PAGED_CODE();

    //
    //  Close our handle to the connection: note, since we limited max connections to 1,
    //  another connect will not be allowed until we return from the disconnect routine.
    //

    FltCloseClientPort( MfltData.Filter, &MfltData.ClientPort );

    //
    //  Reset the user-process field.
    //

    MfltData.UserProcess = NULL;
}

NTSTATUS
DriverUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for the Filter driver.  This unregisters the
    Filter with the filter manager and frees any allocated global data
    structures.

Arguments:

    None.

Return Value:

    Returns the final status of the deallocation routines.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    //
    //  Close the server port.
    //

    FltCloseCommunicationPort( MfltData.ServerPort );

    //
    //  Unregister the filter
    //

    FltUnregisterFilter( MfltData.Filter );
    if (regCookie.QuadPart) CmUnRegisterCallback(regCookie);
    PsSetCreateProcessNotifyRoutine(CreateProcessNotify,TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
FSInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called by the filter manager when a new instance is created.
    We specified in the registry that we only want for manual attachments,
    so that is all we should receive here.

Arguments:

    FltObjects - Describes the instance and volume which we are being asked to
        setup.

    Flags - Flags describing the type of attachment this is.

    VolumeDeviceType - The DEVICE_TYPE for the volume to which this instance
        will attach.

    VolumeFileSystemType - The file system formatted on this volume.

Return Value:

  FLT_NOTIFY_STATUS_ATTACH              - we wish to attach to the volume
  FLT_NOTIFY_STATUS_DO_NOT_ATTACH       - no, thank you

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    ASSERT( FltObjects->Filter == MfltData.Filter );

    //
    //  Don't attach to network volumes.
    //

    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FSQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is the instance detach routine for the filter. This
    routine is called by filter manager when a user initiates a manual instance
    detach. This is a 'query' routine: if the filter does not want to support
    manual detach, it can return a failure status

Arguments:

    FltObjects - Describes the instance and volume for which we are receiving
        this query teardown request.

    Flags - Unused

Return Value:

    STATUS_SUCCESS - we allow instance detach to happen

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
FSPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre create callback.  We need to remember whether this file has been
    opened for write access.  If it has, we'll want to rescan it in cleanup.
    This scheme results in extra scans in at least two cases:
    -- if the create fails (perhaps for access denied)
    -- the file is opened for write access but never actually written to
    The assumption is that writes are more common than creates, and checking
    or setting the context in the write path would be less efficient than
    taking a good guess before the create.

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    CompletionContext - Output parameter which can be used to pass a context
        from this pre-create callback to the post-create callback.

Return Value:

   FLT_PREOP_SUCCESS_WITH_CALLBACK - If this is not our user-mode process.
   FLT_PREOP_SUCCESS_NO_CALLBACK - All other threads.

--*/
{
    MINFILTER_REPLY reply;
	NTSTATUS Status;
	PMINFILTER_NOTIFICATION notification = NULL;
	PFLT_FILE_NAME_INFORMATION pFileInfo;

	UNICODE_STRING uniFileName;

	WCHAR  wszTemp[MAX_STRING];

	UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    //
    //  See if this create is being done by our user process.
    //

	if (MfltData.ClientPort == NULL)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (IoThreadToProcess( Data->Thread ) == MfltData.UserProcess) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
	if (Data->Iopb->OperationFlags == SL_OPEN_PAGING_FILE) return FLT_PREOP_SUCCESS_NO_CALLBACK;

	__try
	{
		notification = (PMINFILTER_NOTIFICATION)FltAllocatePoolAlignedWithTag(FltObjects->Instance,NonPagedPool,sizeof(MINFILTER_NOTIFICATION),'ncaS');
		if (!notification) 			__leave;
		CleanNotification(notification);
        notification->Operation = CREATE;
		notification->suboperation = RO;
        notification->iComponent = COM_FILE;

		if (Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & ( FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES) ) notification->suboperation = WRITE;
		if (Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_EXECUTE)
			notification->isExecute = TRUE;
		else
			notification->isExecute = FALSE;
		if (Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & DELETE ) notification->suboperation = WRITE;
		ULONG ulOptions = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;
		if (ulOptions & FILE_DIRECTORY_FILE)
			notification->isDirectory = TRUE;
		else
			notification->isDirectory = FALSE;
		if (ulOptions & FILE_DELETE_ON_CLOSE) notification->Operation = ERASE;
		if (ulOptions & FILE_OPEN_BY_FILE_ID)
			notification->isNameValid = FALSE;
		else
			notification->isNameValid = TRUE;
		notification->pid = PsGetCurrentProcessId();
        SetProcessName(notification);

		if (!NT_SUCCESS(FltGetFileNameInformation(Data,FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,&pFileInfo)))
		{
			__leave ;
		}
		FltParseFileNameInformation(pFileInfo);

		RtlCopyMemory(notification->wsFileName,pFileInfo->Name.Buffer,pFileInfo->Name.Length);
		FltReleaseFileNameInformation(pFileInfo);

		ULONG replyLength = sizeof( MINFILTER_REPLY );

		Status = FltSendMessage( MfltData.Filter,
			&MfltData.ClientPort,
			notification,
			sizeof(MINFILTER_NOTIFICATION),
			&reply,
			&replyLength,
			NULL );

		if (notification != NULL)
		{
			FltFreePoolAlignedWithTag( FltObjects->Instance, notification, 'nacS' );
			notification = NULL;
		};
		if (!NT_SUCCESS(Status))	__leave;

		if (!reply.bAllow)
		{
			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			Data->IoStatus.Information = 0;
			return FLT_PREOP_COMPLETE;
		}

		// do we have a supersede request? If so, close the file and open a superseding one
		if (reply.bSupersedeFile)
		{
			// retrieve volume form name
			// File format possble: \Device\HardDiskVolume1\Windows\File,
			// or \DosDevices\C:\Windows\File OR \??\C:\Windows\File or C:\Windows\File
			RtlZeroMemory(wszTemp,MAX_STRING*sizeof(WCHAR));
			// \Device\HardDiskvol\file or \DosDevice\C:\file

			int endIndex = 0;
			int nSlash = 0; // number of slashes found
			int len = wcslen(reply.wsFileName);
			while (nSlash < 3 )
			{
				if (endIndex == len ) break;
				if (reply.wsFileName[endIndex]==L'\\') nSlash++;
				endIndex++;
			}
			endIndex--;
			if (nSlash != 3) return FLT_PREOP_SUCCESS_NO_CALLBACK; // failure in filename
			WCHAR savedch = reply.wsFileName[endIndex];
			reply.wsFileName[endIndex] = UNICODE_NULL;
			RtlInitUnicodeString(&uniFileName,reply.wsFileName);
			HANDLE h;
			PFILE_OBJECT pFileObject;

			reply.wsFileName[endIndex] =  savedch;
			NTSTATUS Status = RtlStringCchCopyW(wszTemp,MAX_STRING,reply.wsFileName + endIndex );
			RtlInitUnicodeString(&uniFileName,wszTemp);

			Status = IoReplaceFileObjectName(Data->Iopb->TargetFileObject, reply.wsFileName, wcslen(reply.wsFileName)*sizeof(wchar_t));
			Data->IoStatus.Status = STATUS_REPARSE;
			Data->IoStatus.Information = IO_REPARSE;
			FltSetCallbackDataDirty(Data);
			return FLT_PREOP_COMPLETE;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (notification != NULL)
	{
		FltFreePoolAlignedWithTag( FltObjects->Instance, notification, 'nacS' );
		notification = NULL;
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
FSPreCleanup (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre cleanup callback.  If this file was opened for write access, we want
    to rescan it now.

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    CompletionContext - Output parameter which can be used to pass a context
        from this pre-cleanup callback to the post-cleanup callback.

Return Value:

    Always FLT_PREOP_SUCCESS_NO_CALLBACK.

--*/
{
    NTSTATUS status;
    MINFILTER_REPLY reply;
	PMINFILTER_NOTIFICATION notification = NULL;
	PFLT_FILE_NAME_INFORMATION pFileInfo;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );
	if (MfltData.ClientPort == NULL) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (IoThreadToProcess( Data->Thread ) == MfltData.UserProcess)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (   Data->Iopb->OperationFlags == SL_OPEN_PAGING_FILE)  return FLT_PREOP_SUCCESS_NO_CALLBACK;

	__try
	{
		notification = (PMINFILTER_NOTIFICATION)FltAllocatePoolAlignedWithTag(FltObjects->Instance,NonPagedPool,sizeof(MINFILTER_NOTIFICATION),'ncaS');
		if (!notification)
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		CleanNotification(notification);
        SetProcessName(notification);
        notification->iComponent = COM_FILE;
        notification->Operation = CLOSE;
		notification->pid = PsGetCurrentProcessId();
		if (!NT_SUCCESS(FltGetFileNameInformation(Data,FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,&pFileInfo)))
		{
			__leave ;
		}
		FltParseFileNameInformation(pFileInfo);
		RtlCopyMemory(notification->wsFileName,pFileInfo->Name.Buffer,pFileInfo->Name.Length);
		FltReleaseFileNameInformation(pFileInfo);
    	ULONG replyLength = sizeof( MINFILTER_REPLY );
		status = FltSendMessage( MfltData.Filter,
			&MfltData.ClientPort,
			notification,
			sizeof(MINFILTER_NOTIFICATION),
			&reply,
			&replyLength,
			NULL );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (notification!=NULL)
	{
		FltFreePoolAlignedWithTag(FltObjects->Instance,notification,'ncaS');
		notification= NULL;
	}

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

VOID
CreateProcessNotify (
                     IN HANDLE  ParentId,
                     IN HANDLE  ProcessId,
                     IN BOOLEAN  Create
                     )
{
    MINFILTER_NOTIFICATION* notification;
    MINFILTER_REPLY* reply;

    if (MfltData.ClientPort == NULL)
    {
        return;
    }
    if (PsGetCurrentProcess() == MfltData.UserProcess )
    {
        return;
    }

    notification = (PMINFILTER_NOTIFICATION)ExAllocatePoolWithTag(PagedPool,sizeof(MINFILTER_NOTIFICATION),'ncaS');
    reply = (PMINFILTER_REPLY)ExAllocatePoolWithTag(PagedPool,sizeof(MINFILTER_NOTIFICATION),'ncaS');
    if (!notification || !reply)
    {
        if (notification) ExFreePoolWithTag(notification,'ncaS');
        if (reply) ExFreePoolWithTag(reply,'ncaS');
        return;
    }
    CleanNotification(notification);

    notification->iComponent = COM_PROCESS_NOTIFY;
    if (Create)
    {
        notification->Operation = LOAD;
    }
    else
    {
        notification->Operation = UNLOAD;
    }

    notification->parentPid = ParentId;
    notification->childPid = ProcessId;

    // Set Parent Process Info
    SetProcessName(notification);

    // Set New Process Id ( being either created or unloaded)
    SetProcessName(notification,ProcessId);
	ULONG replyLength = sizeof( MINFILTER_REPLY );
    NTSTATUS status = FltSendMessage( MfltData.Filter,
        &MfltData.ClientPort,
        notification,
        sizeof(MINFILTER_NOTIFICATION),
        reply,
        &replyLength,
        NULL );
    // Despite of the fact that we ignore client reply here, we still require client to send a reply to us.
    // Future versions of the driver might be able to do some processing according to client response.
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(notification,'ncaS');
        ExFreePoolWithTag(reply,'ncaS');

        return;
    }
    ExFreePoolWithTag(notification,'ncaS');
    ExFreePoolWithTag(reply,'ncaS');

    return;
}

NTSTATUS FSGenerateFileNameCallback(  PFLT_INSTANCE Instance,  PFILE_OBJECT FileObject,  PFLT_CALLBACK_DATA CallbackData,  FLT_FILE_NAME_OPTIONS NameOptions,  PBOOLEAN CacheFileNameInformation,  PFLT_NAME_CONTROL FileName )
{
	NTSTATUS status = STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION belowFileName = NULL;
	__try
	{
		//
		//  We expect to only get requests for opened, short and normalized names.
		//  If we get something else, fail.
		//

		if (!FlagOn( NameOptions, FLT_FILE_NAME_OPENED ) &&
			!FlagOn( NameOptions, FLT_FILE_NAME_SHORT ) &&
			!FlagOn( NameOptions, FLT_FILE_NAME_NORMALIZED )) {
				return STATUS_NOT_SUPPORTED ;
		}

		//
		// First we need to get the file name. We're going to call
		// FltGetFileNameInformation below us to get the file name from FltMgr.
		// However, it is possible that we're called by our own minifilter for
		// the name so in order to avoid an infinite loop we must make sure to
		// remove the flag that tells FltMgr to query this same minifilter.
		//
		ClearFlag( NameOptions, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER );
		SetFlag( NameOptions, FLT_FILE_NAME_DO_NOT_CACHE );
		//
		// this will be called for FltGetFileNameInformationUnsafe as well and
		// in that case we don't have a CallbackData, which changes how we call
		// into FltMgr.
		//
		if (CallbackData == NULL)
		{
			//
			// This must be a call from FltGetFileNameInformationUnsafe.
			// However, in order to call FltGetFileNameInformationUnsafe the
			// caller MUST have an open file (assert).
			//
			ASSERT( FileObject->FsContext != NULL );
			status = FltGetFileNameInformationUnsafe( FileObject,
				Instance,
				NameOptions,
				&belowFileName );
			if (!NT_SUCCESS(status))
			{
				__leave;
			}
		}
		else
		{
			//
			// We have a callback data, we can just call FltMgr.
			//
			status = FltGetFileNameInformation( CallbackData,
				NameOptions,
				&belowFileName );
			if (!NT_SUCCESS(status))
			{
				__leave;
			}
		}
		//
		// At this point we have a name for the file (the opened name) that
		// we'd like to return to the caller. We must make sure we have enough
		// buffer to return the name or we must grow the buffer. This is easy
		// when using the right FltMgr API.
		//
		status = FltCheckAndGrowNameControl( FileName, belowFileName->Name.Length );
		if (!NT_SUCCESS(status))
		{
			__leave;
		}
		//
		// There is enough buffer, copy the name from our local variable into
		// the caller provided buffer.
		//
		RtlCopyUnicodeString( &FileName->Name, &belowFileName->Name );
		//
		// And finally tell the user they can cache this name.
		//
		*CacheFileNameInformation = TRUE;
	}
	__finally
	{
		if ( belowFileName != NULL)
		{
			FltReleaseFileNameInformation( belowFileName );
		}
	}
	return status;
}

NTSTATUS FSNormalizeNameComponentExCallback(  PFLT_INSTANCE Instance,  PFILE_OBJECT FileObject,  PCUNICODE_STRING ParentDirectory,  USHORT VolumeNameLength,  PCUNICODE_STRING Component,  PFILE_NAMES_INFORMATION ExpandComponentName,  ULONG ExpandComponentNameLength,  FLT_NORMALIZE_NAME_FLAGS Flags,  PVOID *NormalizationContext )
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE parentDirHandle = NULL;
	OBJECT_ATTRIBUTES parentDirAttributes;
	BOOLEAN isDestinationFile;
	BOOLEAN isCaseSensitive;
	IO_STATUS_BLOCK ioStatus;
#if FLT_MGR_LONGHORN
	IO_DRIVER_CREATE_CONTEXT driverContext;
	PTXN_PARAMETER_BLOCK txnParameter = NULL;
#endif // FLT_MGR_LONGHORN

	__try
	{
		//
		// Initialize the boolean variables. we only use the case sensitivity
		// one but we initialize both just to point out that you can tell
		// whether Component is a "destination" (target of a rename or hardlink
		// creation operation).
		//
		isCaseSensitive = BooleanFlagOn( Flags,
			FLTFL_NORMALIZE_NAME_CASE_SENSITIVE );
		isDestinationFile = BooleanFlagOn( Flags,
			FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME );
		//
		// Open the parent directory for the component we're trying to
		// normalize. It might need to be a case sensitive operation so we
		// set that flag if necessary.
		//
		InitializeObjectAttributes( &parentDirAttributes,
			(PUNICODE_STRING)ParentDirectory,
			OBJ_KERNEL_HANDLE | (isCaseSensitive ? OBJ_CASE_INSENSITIVE : 0 ),
			NULL,
			NULL );
#if FLT_MGR_LONGHORN
		//
		// In Vista and newer this must be done in the context of the same
		// transaction the FileObject belongs to.
		//
		IoInitializeDriverCreateContext( &driverContext );
		txnParameter = IoGetTransactionParameterBlock( FileObject );
		driverContext.TxnParameters = txnParameter;
		status = FltCreateFileEx2( MfltData.Filter,
			Instance,
			&parentDirHandle,
			NULL,
			FILE_LIST_DIRECTORY | SYNCHRONIZE,
			&parentDirAttributes,
			&ioStatus,
			0,
			FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK,
			&driverContext );
#else // !FLT_MGR_LONGHORN
		//
		// preVista we don't care about transactions
		//
	status = FltCreateFile( MfltData.Filter,
			Instance,
			&parentDirHandle,
			FILE_LIST_DIRECTORY | SYNCHRONIZE,
			&parentDirAttributes,
			&ioStatus,
			0,
			FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK );
#endif // FLT_MGR_LONGHORN
		if (!NT_SUCCESS(status))
		{
			__leave;
		}
		//
		// Now that we have a handle to the parent directory of Component, we
		// need to query its long name from the file system. We're going to use
		// ZwQueryDirectoryFile because the handle we have for the directory
		// was opened with FltCreateFile and so targeting should work just fine.
		//
		status = ZwQueryDirectoryFile( parentDirHandle,
			NULL,
			NULL,
			NULL,
			&ioStatus,
			ExpandComponentName,
			ExpandComponentNameLength,
			FileNamesInformation,
			TRUE,
			(PUNICODE_STRING)Component,
			TRUE );
	}
	__finally
	{
		if (parentDirHandle != NULL)
		{
			FltClose( parentDirHandle );
		}
	}
	return status;
}

NTSTATUS FSNormalizeNameComponentCallback(  PFLT_INSTANCE Instance,  PCUNICODE_STRING ParentDirectory,  USHORT VolumeNameLength,  PCUNICODE_STRING Component,  PFILE_NAMES_INFORMATION ExpandComponentName, __in ULONG ExpandComponentNameLength,  FLT_NORMALIZE_NAME_FLAGS Flags, PVOID *NormalizationContext )
{
	//
	// This is just a thin wrapper over FSNormalizeNameComponentExCallback.
	// Please note that we don't pass in a FILE_OBJECT because we don't
	// have one.
	//
	return FSNormalizeNameComponentExCallback( Instance,
		NULL,
		ParentDirectory,
		VolumeNameLength,
		Component,
		ExpandComponentName,
		ExpandComponentNameLength,
		Flags,
		NormalizationContext );
}