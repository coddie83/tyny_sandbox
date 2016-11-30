#pragma  once

///////////////////////////////////////////////////////////////////////////
//
//  Global variables
//
///////////////////////////////////////////////////////////////////////////
typedef struct _SCANNER_STREAM_HANDLE_CONTEXT {
    BOOLEAN Unused;
} SCANNER_STREAM_HANDLE_CONTEXT, *PSCANNER_STREAM_HANDLE_CONTEXT;

typedef struct _MINFILTER_DATA {
    //
    //  The object that identifies this driver.
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER Filter;

    //
    //  Listens for incoming connections
    //

    PFLT_PORT ServerPort;

    //
    //  User process that connected to the port
    //

    PEPROCESS UserProcess;

    //
    //  Client port for a connection to user-mode
    //

    PFLT_PORT ClientPort;
} MINFILTER_DATA, *PMINFILTER_DATA;

extern MINFILTER_DATA MfltData;

#pragma warning(push)
#pragma warning(disable:4200) // disable warnings for structures with zero length arrays.

#pragma warning(pop)

///////////////////////////////////////////////////////////////////////////
//
//  Prototypes for the startup and unload routines used for
//  this Filter.
//
//
///////////////////////////////////////////////////////////////////////////
//DRIVER_INITIALIZE DriverEntry;
extern "C"
NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS
DriverUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
FSQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FSPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
FSPreCleanup (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

NTSTATUS
FSInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CreateProcessNotify (
                                   IN HANDLE  ParentId,
                                   IN HANDLE  ProcessId,
                                   IN BOOLEAN  Create
                                   );

NTSTATUS FSGenerateFileNameCallback(  PFLT_INSTANCE Instance,  PFILE_OBJECT FileObject,  PFLT_CALLBACK_DATA CallbackData,  FLT_FILE_NAME_OPTIONS NameOptions,  PBOOLEAN CacheFileNameInformation,  PFLT_NAME_CONTROL FileName );
NTSTATUS FSNormalizeNameComponentExCallback(  PFLT_INSTANCE Instance,  PFILE_OBJECT FileObject,  PCUNICODE_STRING ParentDirectory,  USHORT VolumeNameLength,  PCUNICODE_STRING Component,  PFILE_NAMES_INFORMATION ExpandComponentName,  ULONG ExpandComponentNameLength,  FLT_NORMALIZE_NAME_FLAGS Flags,  PVOID *NormalizationContext );
NTSTATUS FSNormalizeNameComponentCallback(  PFLT_INSTANCE Instance,  PCUNICODE_STRING ParentDirectory,  USHORT VolumeNameLength,  PCUNICODE_STRING Component,  PFILE_NAMES_INFORMATION ExpandComponentName, __in ULONG ExpandComponentNameLength,  FLT_NORMALIZE_NAME_FLAGS Flags, PVOID *NormalizationContext );