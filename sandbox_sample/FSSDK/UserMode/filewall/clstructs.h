#pragma once
#pragma  pack (push )
#pragma pack (1)
#define ACTION_BASE 0
#define AUX_BASE 0xFF
#define USER_MSG_SIZE 4096

enum Action 
{
	allow = ACTION_BASE,
	deny,
	emulate
};
enum Aux
{
	set_sandbox = AUX_BASE
};

#define ACTION_ALLOW ACTIONBASE
#define ACTION_DENY 1
#define ACTION_EMULATE 2

struct CRule
{
	
	
	DWORD pid;
	WCHAR ImageName[MAX_PATH];  // DOS format
	WCHAR SandBoxRoot[MAX_PATH]; // DOS format
	
	union
	{
		DWORD dwAction; // allow , deny , emulate, set_sandbox
		DWORD dwAuxData; // type of data - sandbox data
	};
    
	
	MINFILTER_NOTIFICATION GenericNotification; // may include wildcards
	bool operator==(const CRule& right)
	{
		bool bCommonResult = ( 
			!_wcsicmp(ImageName,right.ImageName) 
			&&
			dwAction == right.dwAction
			&&
			!_wcsicmp(SandBoxRoot,right.SandBoxRoot) 
			);
		bool bActionResult = (
			GenericNotification.iComponent == right.GenericNotification.iComponent
			&&
			GenericNotification.Operation == right.GenericNotification.Operation
			&&
			GenericNotification.suboperation == right.GenericNotification.suboperation
			&&
			GenericNotification.ValType == GenericNotification.ValType
			&&
			!_wcsicmp(GenericNotification.wsFileName,right.GenericNotification.wsFileName) 
			&&
			(pid == right.pid)
			
		);
		return bCommonResult && bActionResult;
	};
	

};
struct CData 
{
	
    char szServer[40]; // Server and port (destination, in format "hostname:port" ) or pipe name
	union
	{
		DWORD dwCommand; // Command : 
		// Client side (gui): add/delete rule, FF,FN,FC rule , FF,FN,FC processes ,set sandbox root , FF,FN,FC log records
		// add client's listening port to send notifications to.
		// Server side (service): rule/process/log enum reply, FS notification, generic req reply (dwErrorCode only) 

		DWORD dwErrorCode;
	};
    union
    {
       CRule rule;
       unsigned char UserMessage[USER_MSG_SIZE]; // custom data area for usage by plugins
    };
	union
	{
		DWORD dwRuleCount;
		DWORD dwRuleIndex;
	};
	MINFILTER_REPLY immediateReply;

};
class IRuleManager
{
public:
	virtual size_t GetRuleCount() = 0;
	virtual CRule GetRule(int index) = 0;
	virtual size_t AddRule(const CRule& rule) = 0;
	virtual bool DeleteRule(int index) = 0;

};
#pragma  pack (pop )


