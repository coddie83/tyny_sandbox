#pragma once
#define UTIL_API
#include <Objbase.h>
#include <vector>
#include <iostream>

#define CONFIG_PATH 1
#define PLUGINS_PATH 2
#define SIGNATURES_PATH 3

#define CONFIG_FOLDER L"Config"

#define PAGE_SIZE 4096
#define MAX_SECTIONS 20
#define GUID_LENGTH 38 // GUID length considering braces
#define MUST_BE_FREED // prefix means, that user getting a variable must call free() for it. TODO: discus and add it to code
#define CHECK_SERVICE_STATUS_PERIOD 2000 // 2 seconds

#define FILE_EXISTANCE_GHOST_EXISTS 1
#define FILE_EXISTANCE_EXISTS 2
#define FILE_EXISTANCE_NOT_FOUND 0

#define SHARED_PTR(type,name,constructor,destructor) shared_ptr<type>name(constructor, destructor) // shared_ptr initializing macro

#define IsStringDigit IsStringDigitW
class IFilewallStartupNotifications
{
public:
	virtual void Notification(DWORD nSecondsPassed) = 0;
	virtual void ChangeOpacityLevel() = 0;
};

typedef std::vector<std::wstring> str_array;

//#define MEASURE(x,y) { int xx_iTime = GetTickCount(); wchar_t szb[20]; x ; xx_iTime = GetTickCount() - xx_iTime; OutputDebugString(y); _itow(xx_iTime,szb,10);  OutputDebugStringW(szb); OutputDebugStringW(L"\n"); };
#define MEASURE(x,y) x;

	typedef struct
	{
		IMAGE_DOS_HEADER dosheader;
		IMAGE_NT_HEADERS ntHeaders;
		IMAGE_SECTION_HEADER Sections[MAX_SECTIONS];
		unsigned char EntryPointCode[PAGE_SIZE];
	} PEFILE;

	UTIL_API int match(const wchar_t* szPattern, const wchar_t* szSample, bool* pAbsMatch);
	UTIL_API int NativeNameToDosName(const wchar_t* szNative, wchar_t* DosName,size_t maxchars);
	UTIL_API int DosNameToNativeName(const wchar_t* szDosName, wchar_t* szNativeName, size_t maxchars);
    UTIL_API int GetNativeDeviceNameLen(const wchar_t* szName);
    UTIL_API int GetCrc32 (unsigned char *block, unsigned int length);
    //************************************
    // Method:    GetSimilarityRate
    // FullName:  GetSimilarityRate
    // Access:    public
    // Returns:   COMLAYER_API int
    // Qualifier:
    // Parameter: const wchar_t * szFileName1 - first file to compare
    // Parameter: const wchar_t * szFileName2 - second file to compare
    // Parameter: int * pThresold - function sets it to recommended thresold value for positive detection. can be NULL
    // Return value: similarity of files in percents
    //************************************

    UTIL_API int GetSimilarityRate( const wchar_t* szFileName1, const wchar_t* szFileName2, int* pThresold );
    UTIL_API bool FileNameCmp(const wchar_t* szFullName, const wchar_t* szShortName);
    UTIL_API int RegNativeToUser( const wchar_t* szNativeName, wchar_t* szUserName, HKEY* pHkey );
	UTIL_API int RegUserToNative(const wchar_t* szUserName,  wchar_t* szNativeName, HKEY Hkey);
	UTIL_API BOOL GetServicePath(int pathType, wchar_t* szPath);
    UTIL_API BOOL CreateComplexDirectory( const wchar_t* szPath );
    UTIL_API BOOL CreateKey(HKEY hRoot,const wchar_t* szKey, HKEY& key, BOOL bOpen = FALSE);
    UTIL_API BOOL IsKeyExists(const char* szKey);
	UTIL_API  BOOL SaveRegistryKey( HKEY hRoot , const wchar_t* szSubkey, const wchar_t* szFileName );
    UTIL_API  bool RestoreRegistryKey( const wchar_t* szFileName );
    UTIL_API  bool TouchFile( const wchar_t* szFileName );
    UTIL_API  BOOL TouchDirectory( const wchar_t* szFileName );
    UTIL_API  bool NonEmptyFile( const wchar_t* szFileName );
	UTIL_API  bool IsSandboxedFile( const wchar_t* szFileName , const wchar_t* wszSandboxRoot );
	UTIL_API bool UnSandboxFile( wchar_t* szFileName , const wchar_t* wszSandboxRoot, size_t ccMaxFileName );
	// copies key (subkeys and values) but NON-recursively. new key & vals are assigned access rights ALL ACCESS to EVERYONE
    UTIL_API bool CopyKey( const wchar_t* szSourceName, const wchar_t* szDest, const wchar_t* szSandBoxRoot );
  	UTIL_API BOOL GetPeInfo( const wchar_t* szFileName,  PEFILE* PEFile );
	UTIL_API int GetExecutableFileCRC(const wchar_t* szFileName);
	UTIL_API int GetFileCRC(const wchar_t* szFileName);
	UTIL_API int GetFileOptimalCRC(const wchar_t* szFileName);
    UTIL_API int GetAvailDrives(wchar_t* szNames);
	UTIL_API HRESULT ResolveIt(const wchar_t* lpszLinkFile, wchar_t* lpszPath,ULONG* dwRequiredSize);
	bool DeleteDirectory(const wchar_t* lpszDir);
	VOID __stdcall DoUpdateSvcDacl(const wchar_t* szSvcName);
	UTIL_API BOOL CreatePublicDirectory(const wchar_t* szPath);
	UTIL_API BOOL MakeFilePublic( const wchar_t* szPath );
	UTIL_API HANDLE LoadImageResource(HMODULE hMod,DWORD ResId, DWORD dwType, DWORD dwWidth, DWORD dwHeight, COLORREF crTransparencyColor = 0);
	UTIL_API bool StopFilewall();
	UTIL_API bool StartFilewall(IFilewallStartupNotifications* pNotifications);
	UTIL_API DWORD IsFilewallRunning(DWORD dwWaitInSecs );
	BOOL DeleteRegistryTree (HKEY hKeyRoot, const wchar_t* lpSubKey);
	UTIL_API BOOL __stdcall AllowMessagesForPriviledgedWindow(HWND hWnd, UINT* pMessagesArray , BOOL bAllow );
	UTIL_API BOOL __stdcall ProtectProcessFromTerminationXP(HANDLE hProcess,BOOL bProtect);
	UTIL_API GUID CreateGUID(COINIT coinit = COINIT_MULTITHREADED);
	UTIL_API bool GuidToString(GUID guidVal, wchar_t* wszGUID, size_t ch_bufsize);
	UTIL_API void ComputeMD5(const wchar_t* wszString, wchar_t* wszMD5);
	UTIL_API PVOID Hook( const char* szModuleName, const char* szFunctionName, PVOID HookerAddress );
	UTIL_API bool isExported(wchar_t* wszFileName, const char** ppFileNames);
	UTIL_API bool IsStringDigitW(const wchar_t* wczInput);
	UTIL_API void ParseFolders(LPTSTR sPath, LPTSTR sExtension, void* pVector);
	UTIL_API bool SetParamRegistry(const wchar_t* wszRegPath, wchar_t* wszValue, wchar_t* regKey);
	UTIL_API bool IsSoftLinksReadyOS();
	UTIL_API void UpdateAllWindows();
	UTIL_API BOOL ResetMagicDate(const wchar_t* wszPath);
	UTIL_API std::vector<std::wstring> Split(const std::wstring& s, wchar_t seperator);
	//************************************
    // wszExension		- file extension (.doc for example)
	// wszOut			- output buffer
	// wszCommand		- associated command like "open", "print" etc.
	// dwSizeInElements - size of wszOut
	//************************************
	UTIL_API BOOL GetAssociatedApp(const wchar_t* wszExtension, wchar_t* wszOut, const wchar_t* wszCommand, DWORD* dwSizeInElements);
	/////////////////
	UTIL_API wchar_t* GetLastName(wchar_t* pObjectPath);
	UTIL_API int NativePipeToShortPipe(const wchar_t* szNative, wchar_t* DosName);
	UTIL_API int DosPipeToNativePipe(const wchar_t* wszDos, wchar_t* wszNativeName);
	UTIL_API bool IsPipeExists(const wchar_t* wszShortName);
	UTIL_API LONG RegCloneBranch(HKEY hkeyDestRoot, HKEY hkeySrcRoot);
	UTIL_API bool CopyRegistryBranch(HKEY hkDestKey, const wchar_t* wcszDestRoot, HKEY hkSrcKey, const wchar_t* wcszSrcRoot, bool ChangeExistingKeys = true);
	UTIL_API BOOL GetFileNameFromHandle( IN HANDLE hFile, OUT LPWSTR lpFullPath, IN OUT DWORD* pSizeInChars);
	UTIL_API bool IsFileExists(wchar_t* wszFileName);
	UTIL_API bool TerminateProc(HANDLE hProcess);
	UTIL_API void ToDos(const std::wstring& path, std::wstring& result);
	UTIL_API void ToNative(const std::wstring& path, std::wstring& result);
	UTIL_API bool NonGhostFileExists(const wchar_t* wszFilename, BOOL bDeleteGhost);
	UTIL_API DWORD FileExistance(const wchar_t* wszFilename);
	UTIL_API ULONG  NtDeleteFileWrap(PWSTR FileName);
	UTIL_API void OpenSomething(IN const wchar_t *pStr);
	UTIL_API BOOL GetShutDownPrivelege();
	UTIL_API BOOL RebootSystem();
	UTIL_API bool IsWin8orLater();
	UTIL_API str_array GetBrowsers();
	UTIL_API HANDLE CreatePublicEvent(const TCHAR *pName);

	enum AUTORUN_FLAGS: DWORD
	{
		AF_REGISTRY = 0x1,
		AF_STARTUP_FOLDER = 0x2,
		AF_ONCE = 0x4
	};
	UTIL_API bool SetAutorun(const wchar_t *pPath, DWORD dwFlags = (AF_REGISTRY | AF_ONCE));

	////////////////////////////////////

	static UTIL_API std::wstring guidtostring( GUID guidVal )
	{
		std::vector<wchar_t> std_GUID(MAX_PATH);
		StringFromGUID2(guidVal, &std_GUID[0], std_GUID.size());
		std::wstring product = &std_GUID[0];

		if (product.length() < 3)
			return L"";

		product = product.substr(1, product.length() - 2);
		size_t index = 0;
		while ( ( index = product.find(L'-', index) ) != std::string::npos )
			product = product.substr(0, index++).append(product.substr(index));

		return product;
	}

	template <class T>
	class auto_arr_init
	{
#define CONSTRUCTOR_PARAMS OUT T ***_pp, IN const int n, IN const T t, ...
#define CONSTRUCTOR_BODY va_list vl;\
		pp = NULL;\
		if(n > 0 && _pp)\
		{\
			va_start(vl, n);\
			pp = new T*[n + 1];\
			for(int i = 0; i < n; i++)\
			{\
				pp[i] = new T(va_arg(vl, T));\
			}\
			va_end(vl);\
			pp[n] = NULL;\
			*_pp = pp;\
		}

	private:
		T **pp;
		inline void destroy()
		{
			if(pp)
			{
				int i = -1;
				while(pp[++i]) delete pp[i]; // free T pointers
				delete [] pp; // free pointers array
			}
		}
		inline void init(CONSTRUCTOR_PARAMS)
		{
			CONSTRUCTOR_BODY
		};
	public:
		inline auto_arr_init()
		{
			pp = NULL;
		};
		/* used for object reconstruction */
		inline void operator()(CONSTRUCTOR_PARAMS)
		{
			this->destroy();
			CONSTRUCTOR_BODY
		}
		inline auto_arr_init(CONSTRUCTOR_PARAMS)
		{
			CONSTRUCTOR_BODY
		};
		inline ~auto_arr_init()
		{
			this->destroy();
		};
	};