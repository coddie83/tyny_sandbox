// util.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <strsafe.h>
#include <aclapi.h>
#include "util.h"
#include "wildcards.h"

#include <shlobj.h>
#include <ShellAPI.h>

#include <structs.h>

#include "assert.h"
#include <Dbghelp.h>
#include <Psapi.h>

#pragma comment (lib,"Dbghelp.lib")
#pragma comment (lib, "Psapi.lib")

struct DOS_TO_NATIVE
{
    std::wstring dosLetter;
    std::wstring nativeName; // ie \Device\HarddiskVolume1 (no ending slash)
};

typedef std::vector<DOS_TO_NATIVE> dosnativevector;

size_t GetPatternMatchingStrength(const wchar_t* szPattern)
{
    size_t ilen = wcslen(szPattern);
    size_t iScore = ilen;
    if (!ilen) return 0;

    for (size_t i = 0; i < ilen; i++)
    {
        if (szPattern[i]==L'?' || szPattern[i]==L'*') iScore--;
    }
    return iScore;
}

// checks if szSample matches szPattern
// returns: 0, if no match , matching strength from 0 to 100 otherwise.

UTIL_API int match( const wchar_t* szPattern, const wchar_t* szSample, bool* pAbsMatch )
{
	if (szPattern[0] == L'*') if (!szPattern[1]) return true;
	CWildCard wc;
    bool bResult = wc.match(szPattern,szSample, pAbsMatch);
    if (!bResult) return 0;
    return 1;
}

dosnativevector CreateMapping()
{
    dosnativevector namesMapping;

    int realSize = 10;
    wchar_t* pBuffer = new wchar_t[realSize];
    ZeroMemory(pBuffer,realSize*sizeof(wchar_t));
    int newRealSize = GetLogicalDriveStrings(realSize,pBuffer);
    if (newRealSize >realSize )
    {
        realSize = newRealSize;
        delete [] pBuffer;
        pBuffer = new wchar_t[realSize];
        ZeroMemory(pBuffer,realSize*sizeof(wchar_t));
        GetLogicalDriveStrings(realSize,pBuffer);
    }
    wchar_t* p = pBuffer;
    while (*p)
    {
        DOS_TO_NATIVE dn;
        dn.dosLetter = *p;
        dn.dosLetter= dn.dosLetter + L":";
        namesMapping.push_back(dn);
        p+=4;
    }
    for (size_t i  = 0; i < namesMapping.size(); i++)
    {
        wchar_t szNativeName[MAX_PATH];
        ZeroMemory(szNativeName,MAX_PATH*sizeof(wchar_t));
        if (QueryDosDevice(namesMapping[i].dosLetter.c_str(),szNativeName,MAX_STRING))
        {
            namesMapping[i].nativeName = szNativeName;
        }
    }
    delete [] pBuffer;
    return namesMapping;
}

UTIL_API int NativeNameToDosName( const wchar_t* szNative, wchar_t* DosName,size_t maxchars )
{
	if (wcslen(szNative )< 2) return FALSE;
    if (szNative[1]==L':' )
    {
        // already dos name
        wcscpy_s(DosName,maxchars,szNative);
        return TRUE;
    }

    dosnativevector namesMapping = CreateMapping();
    for (size_t i = 0; i < namesMapping.size(); i++)
    {
        DOS_TO_NATIVE DosToNative = namesMapping[i];
        const wchar_t* searchFor =namesMapping[i].nativeName.c_str();
        if (wcsstr(szNative,searchFor)== szNative )
        {
            wcscpy_s(DosName,maxchars,namesMapping[i].dosLetter.c_str()); // i.e C:
            wchar_t* p = (wchar_t*)szNative;
            p+= wcslen(namesMapping[i].nativeName.c_str());
             return wcscat_s(DosName,maxchars,p) == 0; // i.e  \dir\file
        }
    }
	wcscpy_s(DosName,maxchars,szNative);
    if (wcslen(DosName)) DosName[0]=L'n';
	return FALSE;
}

UTIL_API int DosNameToNativeName( const wchar_t* szDosName, wchar_t* szNativeName, size_t maxchars )
{
    if (wcslen(szDosName )< 2)
    {
        wcscpy_s(szNativeName,maxchars,szDosName);
        return FALSE;
    }
    if (szDosName[1]!=L':' )
    {
        // non-dos name, assume that it is already native
        return wcscpy_s(szNativeName,maxchars,szDosName) == 0;
    }
    dosnativevector namesMapping = CreateMapping();
    for (size_t i = 0; i < namesMapping.size(); i++)
    {
        DOS_TO_NATIVE DosToNative = namesMapping[i];
        if (wcsstr(szDosName,namesMapping[i].dosLetter.c_str())== szDosName )
        {
            wcscpy_s(szNativeName,maxchars,namesMapping[i].nativeName.c_str()); // i.e \Device\HardDiskVolume1
            wchar_t* p = (wchar_t*)szDosName;
            p+= wcslen(namesMapping[i].dosLetter.c_str());
            wcscat_s(szNativeName,maxchars,p); // i.e  \dir\file

			return TRUE;
        }
    }

    wcscpy_s(szNativeName,maxchars,szDosName);
	size_t index = wcslen(szNativeName);

    return FALSE;
}

UTIL_API  int GetNativeDeviceNameLen( const wchar_t* szName )
{
    dosnativevector namesMapping = CreateMapping();
    for (size_t i = 0; i < namesMapping.size(); i++)
    {
        DOS_TO_NATIVE DosToNative = namesMapping[i];
        if (wcsstr(szName,namesMapping[i].nativeName.c_str())== szName )
        {
            return (int)namesMapping[i].nativeName.length();
        }
    }
    return 0;
}
UTIL_API  BOOL CreateComplexDirectory( const wchar_t* szPath )
{
    wchar_t* PathCopy =  new wchar_t[wcslen(szPath)+1];
    wchar_t* pPath = PathCopy;
    ZeroMemory(pPath,(wcslen(szPath)+1)*sizeof(wchar_t));
    wcscpy(pPath,szPath);

	while (*pPath)
    {
        while(*pPath != L'\\')
        {
            if (*pPath == NULL) break;
            pPath++;
        }
        if (*pPath == L'\\')
        {
			*pPath = L'\0';
             CreatePublicDirectory(PathCopy);

            *pPath=L'\\';
            pPath++;
        }
        else // *pPath == NULL
        {
			CreatePublicDirectory(PathCopy);
        }
    }
    delete [] PathCopy;
    return true;
}

UTIL_API BOOL CreatePublicDirectory( const wchar_t* szPath )
{
	BOOL bRes = FALSE;
	DWORD dwRes;
	PSID pEveryoneSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa;
	HKEY hkSub = NULL;

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = GENERIC_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

	// Create a new ACL that contains the new ACEs.
	dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes)
	{
		goto Cleanup;
	}

	// Initialize a security descriptor.
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == pSD)
	{
		goto Cleanup;
	}

	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION))
	{
		goto Cleanup;
	}

	// Add the ACL to the security descriptor.
	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,     // bDaclPresent flag
		pACL,
		FALSE))   // not a default DACL
	{
		goto Cleanup;
	}

	// Initialize a security attributes structure.
	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	// Use the security attributes to set the security descriptor
	// when you create a key.

	bRes= CreateDirectory(szPath,&sa);
Cleanup:

	if (pEveryoneSID)
		FreeSid(pEveryoneSID);

	if (pACL)
		LocalFree(pACL);
	if (pSD)
		LocalFree(pSD);

	return bRes;
}

UTIL_API  bool NonEmptyFile( const wchar_t* szFileName )
{
    if (!PathFileExists(szFileName))  return TRUE;
    if (PathIsDirectory(szFileName)) return FALSE;
	HANDLE h = CreateFile(szFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    bool bResult = (GetFileSize(h,NULL) > 0 );
    CloseHandle(h);
    return bResult;
}

UTIL_API  bool IsSandboxedFile( const wchar_t* szFileName , const wchar_t* wszSandboxRoot )
{
	if (wszSandboxRoot == NULL)
	{
		return false;
	}
	return (StrStrI(szFileName,wszSandboxRoot) == szFileName );
}

//
// Purpose:
//   Updates the service DACL to grant start, stop, delete, and read
//   control access to the Guest account.
//
// Parameters:
//   None
//
// Return value:
//   None
//
VOID __stdcall DoUpdateSvcDacl(const wchar_t* szSvcName)
{
	EXPLICIT_ACCESS      ea;
	SECURITY_DESCRIPTOR  sd;
	PSECURITY_DESCRIPTOR psd            = NULL;
	PACL                 pacl           = NULL;
	PACL                 pNewAcl        = NULL;
	BOOL                 bDaclPresent   = FALSE;
	BOOL                 bDaclDefaulted = FALSE;
	DWORD                dwError        = 0;
	DWORD                dwSize         = 0;
	DWORD                dwBytesNeeded  = 0;

	// Get a handle to the SCM database.

	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service

	SC_HANDLE schService = OpenServiceW(
		schSCManager,              // SCManager database
		szSvcName,                 // name of service
		READ_CONTROL | WRITE_DAC); // access

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Get the current security descriptor.

	if (!QueryServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION,
		&psd,           // using NULL does not work on all versions
		0,
		&dwBytesNeeded))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwSize = dwBytesNeeded;
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY, dwSize);
			if (psd == NULL)
			{
				// Note: HeapAlloc does not support GetLastError.
				printf("HeapAlloc failed\n");
				goto dacl_cleanup;
			}

			if (!QueryServiceObjectSecurity(schService,
				DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
			{
				printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
				goto dacl_cleanup;
			}
		}
		else
		{
			printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
			goto dacl_cleanup;
		}
	}

	// Get the DACL.

	if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl,
		&bDaclDefaulted))
	{
		printf("GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Build the ACE.
	/////////////////////
	PSID pEveryoneSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;

	if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		dwError = GetLastError();
		printf("AllocateAndInitializeSid failed(%d)\n", dwError);
		goto dacl_cleanup;
	}

/*
	BuildExplicitAccessWithName(&ea, TEXT("Everyone"),
		SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
		SET_ACCESS, NO_INHERITANCE);
		*/

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = GENERIC_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

	// Create a new ACL that contains the new ACEs.

	dwError = SetEntriesInAcl(1, &ea, NULL, &pNewAcl);
	if (dwError != ERROR_SUCCESS)
	{
		printf("SetEntriesInAcl failed(%d)\n", dwError);
		goto dacl_cleanup;
	}

	// Initialize a new security descriptor.

	if (!InitializeSecurityDescriptor(&sd,
		SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL in the security descriptor.

	if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
	{
		printf("SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL for the service object.

	if (!SetServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION, &sd))
	{
		printf("SetServiceObjectSecurity failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}
	else printf("Service DACL updated successfully\n");

dacl_cleanup:
	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService);

	if(NULL != pNewAcl)
		LocalFree((HLOCAL)pNewAcl);
	if(NULL != psd)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
}

UTIL_API bool IsFileExists( wchar_t* wszFileName )
{
	DWORD dwAttr = GetFileAttributes(wszFileName);

	if(dwAttr == INVALID_FILE_ATTRIBUTES)
		return false;
	else
		return true;
}

UTIL_API void ToDos(const std::wstring& path, std::wstring& result)
{
	unsigned uLength = path.length() + 1;
	wchar_t* szResult = new wchar_t[uLength];
	ZeroMemory(szResult, uLength*sizeof(wchar_t));
	NativeNameToDosName(path.c_str(), szResult, uLength);
	result = szResult;
	delete [] szResult;
}

UTIL_API void ToNative( const std::wstring& path, std::wstring& Result )
{
	ULONG ulccBufSize = path.length() + MAX_PATH ;
	wchar_t* szResult = new wchar_t[ulccBufSize];
	ZeroMemory(szResult,ulccBufSize*sizeof(wchar_t));
	DosNameToNativeName(path.c_str(),szResult,ulccBufSize);
	std::wstring  NPath = szResult;
	delete [] szResult;
	Result = NPath;
}