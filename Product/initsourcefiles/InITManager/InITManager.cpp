// InITUpdater.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#pragma region Includes
#define _WIN32_DCOM
#include "definitions.h"
#include "wrappers.h"
//#include <Windows.h>
//#include <Shldisp.h>
#include <atlstr.h> 
//#include <strsafe.h>
#include "zlib.h"
//#include "contrib/minizip/unzip.h"
#include "minizip/unzip.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
//#include <fstream>
#include <vector>
#include <iostream>
//#include <curl/curl.h>
#include <ctime>
#include <nlohmann/json.hpp>



#include "InITManagerService.h"
#include "InITManagerServiceBase.h"
#include "InITManagerServiceInstaller.h"
#pragma endregion
#pragma comment(lib, "kernel32.lib")

// for convenience
using json = nlohmann::json;

SC_HANDLE schService;

SC_HANDLE schSCManager;

//
// Purpose: 
//   Starts the service if possible.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoStartSvc()
{
	std::cout << "STARTING DoStartSvc()... " << std::endl;
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	// Get a handle to the SCM database. 
	std::cout << "Getting handle to SCM... " << std::endl;
	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // servicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.
	std::cout << "Getting handle to service..." << std::endl;
	SC_HANDLE schService;
	schService = OpenService(
		schSCManager,         // SCM database 
		SERVICE_NAME,            // name of service 
		SERVICE_ALL_ACCESS);  // full access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Check the status in case the service is not stopped. 
	std::cout << "Checking service status in case not stopped..." << std::endl;
	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // size needed if buffer is too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Check if the service is already running. It would be possible 
	// to stop the service here, but for simplicity this example just returns. 
	std::cout << "checking if service is already running..." << std::endl;
	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		printf("Cannot start the service because it is already running\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Save the tick count and initial checkpoint.
	std::cout << "Saving tick count and initial checkpoint..." << std::endl;
	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.
	std::cout << "Waiting for service to stop before starting..." << std::endl;
	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status until the service is no longer stop pending. 

		if (!QueryServiceStatusEx(
			schService,                     // handle to service 
			SC_STATUS_PROCESS_INFO,         // information level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // size needed if buffer is too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				printf("Timeout waiting for service to stop\n");
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return;
			}
		}
	}

	// Attempt to start the service.
	std::cout << "Attempting to start the service..." << std::endl;
	if (!StartService(
		schService,  // handle to service 
		0,           // number of arguments 
		NULL))      // no arguments 
	{
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service start pending...\n");

	// Check the status until the service is no longer start pending. 
	std::cout << "Checking to see if service is no longer start pending..." << std::endl;
	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // info level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // if buffer too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status again. 

		if (!QueryServiceStatusEx(
			schService,             // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // if buffer too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}

	// Determine whether the service is running.

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		printf("Service started successfully.\n");
	}
	else
	{
		printf("Service not started. \n");
		printf("  Current State: %d\n", ssStatus.dwCurrentState);
		printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
		printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
		printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Stops the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
BOOL __stdcall StopDependentServices()
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS   lpDependencies = NULL;
	ENUM_SERVICE_STATUS     ess;
	SC_HANDLE               hDepService;
	SERVICE_STATUS_PROCESS  ssp;

	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out

							 // Pass a zero-length buffer to get the required buffer size.
	if (EnumDependentServices(schService, SERVICE_ACTIVE,
		lpDependencies, 0, &dwBytesNeeded, &dwCount))
	{
		// If the Enum call succeeds, then there are no dependent
		// services, so do nothing.
		return TRUE;
	}
	else
	{
		if (GetLastError() != ERROR_MORE_DATA)
			return FALSE; // Unexpected error

						  // Allocate a buffer for the dependencies.
		lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
			GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

		if (!lpDependencies)
			return FALSE;

		__try {
			// Enumerate the dependencies.
			if (!EnumDependentServices(schService, SERVICE_ACTIVE,
				lpDependencies, dwBytesNeeded, &dwBytesNeeded,
				&dwCount))
				return FALSE;

			for (i = 0; i < dwCount; i++)
			{
				ess = *(lpDependencies + i);
				// Open the service.
				hDepService = OpenService(schSCManager,
					ess.lpServiceName,
					SERVICE_STOP | SERVICE_QUERY_STATUS);

				if (!hDepService)
					return FALSE;

				__try {
					// Send a stop code.
					if (!ControlService(hDepService,
						SERVICE_CONTROL_STOP,
						(LPSERVICE_STATUS)&ssp))
						return FALSE;

					// Wait for the service to stop.
					while (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						Sleep(ssp.dwWaitHint);
						if (!QueryServiceStatusEx(
							hDepService,
							SC_STATUS_PROCESS_INFO,
							(LPBYTE)&ssp,
							sizeof(SERVICE_STATUS_PROCESS),
							&dwBytesNeeded))
							return FALSE;

						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount() - dwStartTime > dwTimeout)
							return FALSE;
					}
				}
				__finally
				{
					// Always release the service handle.
					CloseServiceHandle(hDepService);
				}
			}
		}
		__finally
		{
			// Always free the enumeration buffer.
			HeapFree(GetProcessHeap(), 0, lpDependencies);
		}
	}
	return TRUE;
}
void DoStopSvc()
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwStartTime = GetTickCount();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 30000; // 30-second time-out
	DWORD dwWaitTime;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.


	schService = OpenService(
		schSCManager,         // SCM database 
		SERVICE_NAME,            // name of service 
		SERVICE_STOP |
		SERVICE_QUERY_STATUS |
		SERVICE_ENUMERATE_DEPENDENTS);

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Make sure the service is not already stopped.

	if (!QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		printf("Service is already stopped.\n");
		goto stop_cleanup;
	}

	// If a stop is pending, wait for it.

	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		printf("Service stop pending...\n");

		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssp.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			printf("Service stopped successfully.\n");
			goto stop_cleanup;
		}

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			printf("Service stop timed out.\n");
			goto stop_cleanup;
		}
	}

	// If the service is running, dependencies must be stopped first.

	StopDependentServices();

	// Send a stop code to the service.

	if (!ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS)&ssp))
	{
		printf("ControlService failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	// Wait for the service to stop.

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);
		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			printf("Wait timed out\n");
			goto stop_cleanup;
		}
	}
	printf("Service stopped successfully\n");

stop_cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

//
//   FUNCTION: UninstallService
//
//   PURPOSE: Stop and remove the service from the local service control 
//   manager database.
//
//   PARAMETERS: 
//   * pszServiceName - the name of the service to be removed.
//
//   NOTE: If the function fails to uninstall the service, it prints the 
//   error in the standard output stream for users to diagnose the problem.
//

int GetMachineGUID(std::wstring& struuid)
{
	HKEY hKey = 0;
	wchar_t buf[255] = { 0 };
	DWORD dwType = 0;
	DWORD dwBufSize = sizeof(buf);
	const wchar_t* subkey = L"Software\\Microsoft\\Cryptography";

	if (RegOpenKey(HKEY_LOCAL_MACHINE, subkey, &hKey) == ERROR_SUCCESS)
	{
		dwType = REG_SZ;
		if (RegQueryValueEx(hKey, L"MachineGuid", 0, &dwType, (BYTE*)buf, &dwBufSize) == ERROR_SUCCESS)
		{
			std::wcout << "key value is '" << buf << "'\n";
			//printf(" %s", buf);
			std::wcout << std::endl;
		}
		else
			std::wcout << "can not query for key value\n" << std::endl;
		RegCloseKey(hKey);

	}
	else
		std::wcout << "Can not open key\n";
	std::wcout << L"GUID = " << buf << "\n";
	RegCloseKey(hKey);

	struuid = buf;
	return 0;
}


inline bool fileExists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

/*
std::vector<std::string> getDeleteOrders(std::string order_filepath)
{
	std::string line;
	std::vector<std::string> files;
	std::ifstream deleteorders(order_filepath);
	while (deleteorders >> line)
	{
		files.push_back(line);
	}

	return files;
}
*/
/*
void executeDeleteOrders(std::vector<std::string> files)
{
	std::vector<std::string>::iterator it;
	for (it = files.begin(); it != files.end(); it++)
	{
		DeleteFileA(it->c_str());
	}
}
*/

BOOL WINAPI IsDBGServiceAnExe(VOID)
{
	HANDLE hProcessToken = NULL;
	DWORD groupLength = 50;

	PTOKEN_GROUPS groupInfo = (PTOKEN_GROUPS)LocalAlloc(0,
		groupLength);

	SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
	PSID InteractiveSid = NULL;
	PSID ServiceSid = NULL;
	DWORD i;

	// Start with assumption that process is an EXE, not a Service.
	BOOL fExe = TRUE;


	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,
		&hProcessToken))
		goto ret;

	if (groupInfo == NULL)
		goto ret;

	if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
		groupLength, &groupLength))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto ret;

		LocalFree(groupInfo);
		groupInfo = NULL;

		groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

		if (groupInfo == NULL)
			goto ret;

		if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
			groupLength, &groupLength))
		{
			goto ret;
		}
	}

	//
	//  We now know the groups associated with this token.  We want to look to see if
	//  the interactive group is active in the token, and if so, we know that
	//  this is an interactive process.
	//
	//  We also look for the "service" SID, and if it's present, we know we're a service.
	//
	//  The service SID will be present iff the service is running in a
	//  user account (and was invoked by the service controller).
	//


	if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0,
		0,
		0, 0, 0, 0, 0, &InteractiveSid))
	{
		goto ret;
	}

	if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0,
		0, 0, 0, 0, &ServiceSid))
	{
		goto ret;
	}

	for (i = 0; i < groupInfo->GroupCount; i += 1)
	{
		SID_AND_ATTRIBUTES sanda = groupInfo->Groups[i];
		PSID Sid = sanda.Sid;

		//
		//  Check to see if the group we're looking at is one of
		//  the 2 groups we're interested in.
		//

		if (EqualSid(Sid, InteractiveSid))
		{
			//
			//  This process has the Interactive SID in its
			//  token.  This means that the process is running as
			//  an EXE.
			//
			goto ret;
		}
		else if (EqualSid(Sid, ServiceSid))
		{
			//
			//  This process has the Service SID in its
			//  token.  This means that the process is running as
			//  a service running in a user account.
			//
			fExe = FALSE;
			goto ret;
		}
	}

	//
	//  Neither Interactive or Service was present in the current users token,
	//  This implies that the process is running as a service, most likely
	//  running as LocalSystem.
	//
	fExe = FALSE;

ret:

	if (InteractiveSid)
		FreeSid(InteractiveSid);

	if (ServiceSid)
		FreeSid(ServiceSid);

	if (groupInfo)
		LocalFree(groupInfo);

	if (hProcessToken)
		CloseHandle(hProcessToken);

	return(fExe);
}

//
//  FUNCTION: wmain(int, wchar_t *[])
//
//  PURPOSE: entrypoint for the application.
// 
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    wmain() either performs the command line task, or run the service.
//

int wmain(int argc, wchar_t *argv[])
{
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"install", argv[1] + 1) == 0)
		{
			// Install the service when the command is 
			// "-install" or "/install".
			InstallService(
				SERVICE_NAME,               // Name of service
				SERVICE_DISPLAY_NAME,       // Name to display
				SERVICE_AUTO_START,         // Service start type
				SERVICE_DEPENDENCIES,       // Dependencies
				SERVICE_ACCOUNT,            // Service running account
				SERVICE_PASSWORD            // Password of the account
			);

			installReg();
			installDir();

			//std::string url = "https://initiam.net/srv/files/tmplink.php?filetag=15at1mpraf5f";
			//downloadFile(url, "package.zip");

			/*
			std::wcout << "Downloading update..." << std::endl;
			downloadFile("https://init.sgcity.org/srv/update/hello.zip", "hello.zip");

			std::wcout << "Installing Update..." << std::endl;
			CComBSTR file("C:\\Users\\collin.walker\\Desktop\\InITService\\C++\\x64\\Release\\hello.zip");
			CComBSTR folder("C:\\Windows\\System32\\init\\");
			Unzip2Folder(file, folder);

			std::wcout << "Installation Complete" << std::endl;
			*/
		}
		else if (_wcsicmp(L"uninstall", argv[1] + 1) == 0)
		{
			// Uninstall the service when the command is 
			// "-remove" or "/remove".
			UninstallService(SERVICE_NAME);

			std::cout << "Uninstallation complete." << std::endl;
		}
		else if (_wcsicmp(L"cleanup", argv[1] + 1) == 0)
		{
			HKEY key;
			std::wstring strInITIMKey = L"SOFTWARE\\InIT\\";
			LPTSTR tstrInITIMKey = L"SOFTWARE\\InIT\\";
			LONG nError;
			nError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strInITIMKey.c_str(), 0, 0, &key);
			if (ERROR_NO_MATCH == nError || ERROR_FILE_NOT_FOUND == nError)
				std::wcout << "InventoryManager registry keys not found at HKEY_LOCAL_MACHINE\\" << strInITIMKey << std::endl;
			else
			{
				bool bSuccess;
				bSuccess = (RegDelnode(HKEY_LOCAL_MACHINE, tstrInITIMKey) != 0);

				if (bSuccess)
					std::cout << "Successfully cleaned up InITInventoryManager's registry files." << std::endl;
				else
					std::cout << "Failed to clean up InITInventoryManager's registry files." << std::endl;
			}

			// Delete 'c:\mydir' and its subdirectories
			int         iRC = 0;
			std::wstring strDirectoryToDelete = SERVICE_DEFAULT_PATH;
			iRC = DeleteDirectory(strDirectoryToDelete);
			if (iRC)
			{
				std::wcout << "Error " << iRC << ": Failed to delete directory " << strDirectoryToDelete << std::endl;
			}

			std::cout << "Cleanup complete." << std::endl;
		}
		else if (_wcsicmp(L"start", argv[1] + 1) == 0)
		{
			// Start the service when the command is 
			// "-start" or "/start".
			DoStartSvc();
		}
		else if (_wcsicmp(L"stop", argv[1] + 1) == 0)
		{
			// Stop the service when the command is 
			// "-stop" or "/stop".
			DoStopSvc();
		}
	}
	else
	{
		wprintf(L"Parameters:\n");
		wprintf(L" -install  to install the service.\n");
		wprintf(L" -start  to start the service.\n");
		wprintf(L" -stop  to stop the service.\n");
		wprintf(L" -uninstall   to uninstall the service.\n");

		InITManagerService service(SERVICE_NAME);
		if (!InITManagerServiceBase::Run(service))
		{
			wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
		}
	}

	return 0;
}


/*
int wmain(int argc, wchar_t *argv[])
{
	if (IsDBGServiceAnExe() == 1)
	{
		std::cout << "This is an exe!" << std::endl;
		InITManagerService service1(SERVICE_NAME);
		std::cout << "I made a service!" << std::endl;
		std::cout << "Testing on start and on stop!..." << std::endl;
		service1.testOnStartandOnStop(argc, argv);
		//service1.TestStartupAndStop(args);
	}
	else
	{
		if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
		{
			if (_wcsicmp(L"install", argv[1] + 1) == 0)
			{
				// Install the service when the command is 
				// "-install" or "/install".
				InstallService(
					SERVICE_NAME,               // Name of service
					SERVICE_DISPLAY_NAME,       // Name to display
					SERVICE_START_TYPE,         // Service start type
					SERVICE_DEPENDENCIES,       // Dependencies
					SERVICE_ACCOUNT,            // Service running account
					SERVICE_PASSWORD            // Password of the account
				);

				// Setup registry structure
				// Checks to see if strInITKeyName exists and if not, creates it.

				DWORD rtime = 0;
				HKEY InITKey;
				DWORD dwDisposition;
				LONG nError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SERVICE_DEFAULT_REG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &InITKey, &dwDisposition);

				if (ERROR_SUCCESS != nError)
				{
					std::cout << "Error: Could not open/create registry key HKEY_LOCAL_MACHINE\\" << SERVICE_DEFAULT_REG << std::endl << "\tERROR: " << dwDisposition << std::endl;
				}
				else
				{
					std::cout << "Successfully " << ((REG_CREATED_NEW_KEY == dwDisposition) ? "created" : "opened") << " registry key HKEY_LOCAL_MACHINE\\" << SERVICE_DEFAULT_REG << std::endl;

					// See https://www.experts-exchange.com/questions/10171094/Using-RegSetKeySecurity.html for example
					//SECURITY_DESCRIPTOR sd;
					//PACL pDacl = NULL;    
					//RegSetKeySecurity(InITKey);

					// Generate guid and convert to a string
					//
					std::wstring clientguid;
					GUID guid;

					HRESULT hr = CoCreateGuid(&guid);
					if (FAILED(hr))
					{
						std::cout << "Error: Could not generate clientguid" << std::endl << "\tERROR: " << (int)hr << std::endl;
					}
					else
					{
						WCHAR guidString[40] = { 0 };
						int len = StringFromGUID2(guid, guidString, 40);
						if (len > 2)
						{
							// Sets clientguid value and ties to strInITKeyName
							clientguid.assign(&guidString[1], len - 3);
						}
					}

					// Setup registry values

					nError = RegSetValueEx(InITKey, L"clientguid", NULL, REG_SZ, (const BYTE*)clientguid.c_str(), static_cast<DWORD>((clientguid.size() + 1)) * sizeof(wchar_t));
					if (ERROR_SUCCESS != nError)
						std::cout << "Error: Could not set registry value: clientguid" << std::endl << "\tERROR: " << nError << std::endl;
					else
						std::wcout << "Successfully set InIT clientguid to " << clientguid << std::endl;

					std::wstring strInITManagerKeyName = SERVICE_HOME_REG;
					nError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, strInITManagerKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &InITKey, &dwDisposition);

					std::wstring ver = SERVICE_VERSION;
					nError = RegSetValueEx(InITKey, L"version", NULL, REG_SZ, (const BYTE*)ver.c_str(), static_cast<DWORD>((ver.size() + 1)) * sizeof(wchar_t));
					if (ERROR_SUCCESS != nError)
						std::cout << "Error: Could not set registry value: version" << std::endl << "\tERROR: " << nError << std::endl;
					else
						std::wcout << "Successfully set InITManager version to " << ver << std::endl;


					RegCloseKey(InITKey);
				}

				std::wcout << "Setting up InIT..." << std::endl;
				if (CreateDirectory(L"C:\\Windows\\System32\\InIT", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
				{
					std::wcout << "System32 directory InIT successfully created" << std::endl;
					if (CreateDirectory(L"C:\\Windows\\System32\\InIT\\InITManager", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
					{
						std::wcout << "init directory initmanager successfully created" << std::endl;
					}
					else
					{
						std::wcout << "Failed to create System32 folder InITManager." << std::endl;
					}
				}
				else
					std::wcout << "Failed to create System32 folder InITManager." << std::endl;

				std::wcout << "Downloading update..." << std::endl;
				downloadFile("https://init.sgcity.org/srv/update/hello.zip", "hello.zip");

				std::wcout << "Installing Update..." << std::endl;
				CComBSTR file("C:\\Users\\collin.walker\\Desktop\\InITService\\C++\\x64\\Release\\hello.zip");
				CComBSTR folder("C:\\Windows\\System32\\init\\");
				Unzip2Folder(file, folder);

				std::wcout << "Installation Complete" << std::endl;

			}
			else if (_wcsicmp(L"uninstall", argv[1] + 1) == 0)
			{
				// Uninstall the service when the command is 
				// "-remove" or "/remove".
				UninstallService(SERVICE_NAME);

				HKEY key;
				std::wstring strInITIMKey = L"SOFTWARE\\InIT\\";
				LPTSTR tstrInITIMKey = L"SOFTWARE\\InIT\\";
				LONG nError;
				nError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strInITIMKey.c_str(), 0, 0, &key);
				if (ERROR_NO_MATCH == nError || ERROR_FILE_NOT_FOUND == nError)
					std::wcout << "InventoryManager registry keys not found at HKEY_LOCAL_MACHINE\\" << strInITIMKey << std::endl;
				else
				{
					bool bSuccess;
					bSuccess = (RegDelnode(HKEY_LOCAL_MACHINE, tstrInITIMKey) != 0);

					if (bSuccess)
						std::cout << "Successfully cleaned up InITInventoryManager's registry files." << std::endl;
					else
						std::cout << "Failed to clean up InITInventoryManager's registry files." << std::endl;
				}

				// Delete 'c:\mydir' and its subdirectories
				int         iRC = 0;
				std::wstring strDirectoryToDelete = SERVICE_DEFAULT_PATH;
				iRC = DeleteDirectory(strDirectoryToDelete);
				if (iRC)
				{
					std::wcout << "Error " << iRC << ": Failed to delete directory " << strDirectoryToDelete << std::endl;
				}

				std::cout << "Uninstallation complete." << std::endl;
			}
			else if (_wcsicmp(L"start", argv[1] + 1) == 0)
			{
				// Start the service when the command is 
				// "-start" or "/start".
				DoStartSvc();
			}
			else if (_wcsicmp(L"stop", argv[1] + 1) == 0)
			{
				// Stop the service when the command is 
				// "-stop" or "/stop".
				DoStopSvc();
			}
		}
		else
		{
			wprintf(L"Parameters:\n");
			wprintf(L" -install  to install the service.\n");
			wprintf(L" -start  to start the service.\n");
			wprintf(L" -stop  to stop the service.\n");
			wprintf(L" -uninstall   to uninstall the service.\n");

			InITManagerService service(SERVICE_NAME);
			if (!InITManagerServiceBase::Run(service))
			{
				wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
			}
		}

		return 0;
	}
}
*/