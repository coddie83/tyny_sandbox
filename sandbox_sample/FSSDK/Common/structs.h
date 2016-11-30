#pragma  once
//
//  Name of port used to communicate
//

const PWSTR ScannerPortName = L"\\ScannerPort";
// Operations
#define UNSUPPORTED 0
#define CREATE 1
#define CLOSE 2
#define ERASE 3
#define FIND_FILE 4
#define LOAD 5
#define UNLOAD 6

// RegOperations

#define RDeleteKey = 5;

// suboperation
#define RO 1
#define WRITE 2
#define REG_PRE 3
#define REG_POST 4

// component
#define COM_FILE 0
#define COM_REGISTRY 1
#define COM_PROCESS_NOTIFY 2

#define SCANNER_READ_BUFFER_SIZE   1024
#define MAX_STRING 260
#define MAX_DATA 2048

#pragma  pack (push )
#pragma pack (1)

typedef struct _MINFILTER_NOTIFICATION {
    unsigned long iComponent;
    unsigned long Operation; // OPERATION : CREATE , ERASE , FIND_FILE
	unsigned long suboperation; // WRITE, RO
	union
    {
        unsigned long isDirectory; // TRUE if object is directory, not a file
        unsigned long ValType; // type of value for registry
    };

	BOOLEAN isExecute; // TRUE, if object is opened with EXECUTE access (TRAVERSE for dir))
	BOOLEAN isNameValid; // TRUE if wsFileName contains name. FALSE if file opened by ID.
    union{
        HANDLE pid; // PROCESS ID
        HANDLE parentPid; // parent process ID on process create notification
    };
    UCHAR szImageName[17]; // process name (parent process name)
	union
    {
        WCHAR wsFileName[MAX_STRING]; // File or directory  name operation is performed on
        WCHAR wsRegKeyName[MAX_STRING]; // RegKey name operation is performed on
        WCHAR wsOldName[MAX_STRING]; // for rename op
        UCHAR szChildName[17]; // child process name
    };

    HANDLE childPid; // child process Id
} MINFILTER_NOTIFICATION, *PMINFILTER_NOTIFICATION;

typedef struct _MINFILTER_REPLY {
    BOOLEAN bAllow; // operation allowed if true (file will be shown if operation is enumeration of files)
	union
    {
       // supersedes with wsFileName on create op (by substituting FileObject)
       BOOLEAN bSupersedeFile;
       // if true, emulates successfull registry operation. Vista and higher only, ignored on XP
       BOOLEAN bEmulateSuccessRegistry;
    };

	WCHAR wsFileName[MAX_STRING]; // for supresede op on files only
} MINFILTER_REPLY, *PMINFILTER_REPLY;
#pragma  pack (pop)

typedef enum _REG_OPERATIONS {
    ROP_NtDeleteKey,
    ROP_NtDeleteValueKey,
    ROP_NtCreateKeyEx,
    ROP_NtOpenKeyEx,
    ROP_NtRenameKey,
    ROP_NtSetValueKey
} REG_OPERATIONS;
