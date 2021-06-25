#include "wrappers.h"
#include "jsonparse.h"


std::wstring getClientGuid()
{
	// Set member variable std::wstring m_ClientGuid from registry
	HKEY hKey;
	std::wstring wcguid;
	std::wstring regpath = SERVICE_DEFAULT_REG;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKey);
	GetStringRegKey(hKey, L"clientguid", wcguid, L"null");
	RegCloseKey(hKey);
	return wcguid;
}

// checks to see if (on start) service was just updated //
bool wasUpdated(std::wstring service_name)
{
	// Set member variable std::wstring m_ClientGuid from registry
	HKEY hKey;
	DWORD updated;
	std::wstring regpath = SERVICE_DEFAULT_REG;
	regpath += service_name;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKey);
	GetDWORDRegKey(hKey, L"updated", updated, 1);

	if (updated != 1)
	{
		return false;
	}

	return true;
}

int create_task(std::wstring taskname)
{
	//  ------------------------------------------------------
	//  Initialize COM.
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("\nCoInitializeEx failed: %x", hr);
		return 1;
	}

	//  Set general COM security levels.
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr))
	{
		printf("\nCoInitializeSecurity failed: %x", hr);
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Create a name for the task.
	LPCWSTR wszTaskName = taskname.c_str();

	//  Set the executable path to InITUpdate.exe
	std::wstring wstrExecutablePath = L"C:\\Program Files\\InIT\\InITUpdate\\InITUpdate.exe";


	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr))
	{
		printf("Failed to create an instance of ITaskService: %x", hr);
		CoUninitialize();
		return 1;
	}

	//  Connect to the task service.
	hr = pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		printf("ITaskService::Connect failed: %x", hr);
		pService->Release();
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
	{
		printf("Cannot get Root folder pointer: %x", hr);
		pService->Release();
		CoUninitialize();
		return 1;
	}

	//  If the same task exists, remove it.
	pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

	//  Create the task definition object to create the task.
	ITaskDefinition* pTask = NULL;
	hr = pService->NewTask(0, &pTask);

	pService->Release();  // COM clean up.  Pointer is no longer used.
	if (FAILED(hr))
	{
		printf("Failed to CoCreate an instance of the TaskService class: %x", hr);
		pRootFolder->Release();
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Get the registration info for setting the identification.
	IRegistrationInfo* pRegInfo = NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
	{
		printf("\nCannot get identification pointer: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	BSTR desription = SysAllocString(L"Keeps your InIT software up to date. If this task is disabled or stopped, your InIT software will not be kept up to date, meaning security vulnerabilities that may arise cannot be fixed and features may not work. This task uninstalls itself when there is no InIT software using it.");
	hr = pRegInfo->put_Author(SysAllocString(L"InIT"));
	hr = pRegInfo->put_Description(desription);
	pRegInfo->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put identification info: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Create the principal for the task - these credentials
	//  are overwritten with the credentials passed to RegisterTaskDefinition
	IPrincipal* pPrincipal = NULL;
	hr = pTask->get_Principal(&pPrincipal);
	if (FAILED(hr))
	{
		printf("\nCannot get principal pointer: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Set up principal logon type to interactive logon TASK_LOGON_SERVICE_ACCOUNT TASK_LOGON_INTERACTIVE_TOKEN
	hr = pPrincipal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT);
	hr = pPrincipal->put_UserId(SysAllocString(L"NT AUTHORITY\\SYSTEM"));
	pPrincipal->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put principal info: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Create the settings for the task
	ITaskSettings* pSettings = NULL;
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		printf("\nCannot get settings pointer: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Set setting values for the task.  
	hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
	pSettings->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put setting information: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	// Set the idle settings for the task.
	IIdleSettings* pIdleSettings = NULL;
	hr = pSettings->get_IdleSettings(&pIdleSettings);
	if (FAILED(hr))
	{
		printf("\nCannot get idle setting information: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	hr = pIdleSettings->put_WaitTimeout(SysAllocString(L"PT5M"));
	pIdleSettings->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put idle setting information: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}


	//  ------------------------------------------------------
	//  Get the trigger collection to insert the time trigger.
	ITriggerCollection* pTriggerCollection = NULL;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
	{
		printf("\nCannot get trigger collection: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Add the time trigger to the task.
	ITrigger* pTrigger_d = NULL;
	ITrigger* pTrigger_l = NULL;
	hr = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger_d);
	hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger_l);
	pTriggerCollection->Release();
	if (FAILED(hr))
	{
		printf("\nCannot create trigger: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	IDailyTrigger* pDailyTrigger = NULL;
	hr = pTrigger_d->QueryInterface(
		IID_IDailyTrigger, (void**)&pDailyTrigger);
	pTrigger_d->Release();
	if (FAILED(hr))
	{
		printf("\nQueryInterface call failed for IDailyTrigger: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	ILogonTrigger* pLogonTrigger = NULL;
	hr = pTrigger_l->QueryInterface(
		IID_ILogonTrigger, (void**)&pLogonTrigger);
	pTrigger_l->Release();
	if (FAILED(hr))
	{
		printf("\nQueryInterface call failed for ILogonTrigger: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	hr = pDailyTrigger->put_Id(_bstr_t(L"Trigger1"));
	if (FAILED(hr))
		printf("\nCannot put trigger ID: %x", hr);
	hr = pLogonTrigger->put_Id(_bstr_t(L"Trigger2"));
	if (FAILED(hr))
		printf("\nCannot put trigger ID: %x", hr);

	//  Set the task to start at a certain time. The time 
	//  format should be YYYY-MM-DDTHH:MM:SS(+-)(timezone).
	//  For example, the start boundary below
	//  is January 1st 2005 at 12:05
	hr = pDailyTrigger->put_StartBoundary(_bstr_t(L"2005-01-01T23:30:00"));
	hr = pDailyTrigger->put_DaysInterval(1);
	pDailyTrigger->Release();
	if (FAILED(hr))
	{
		printf("\nCannot add start boundary to trigger: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	pLogonTrigger->Release();


	//  ------------------------------------------------------
	//  Add an action to the task. This task will execute notepad.exe.     
	IActionCollection* pActionCollection = NULL;

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		printf("\nCannot get Task collection pointer: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Create the action, specifying that it is an executable action.
	IAction* pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(hr))
	{
		printf("\nCannot create the action: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	IExecAction* pExecAction = NULL;
	//  QI for the executable task pointer.
	hr = pAction->QueryInterface(
		IID_IExecAction, (void**)&pExecAction);
	pAction->Release();
	if (FAILED(hr))
	{
		printf("\nQueryInterface call failed for IExecAction: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Set the path of the executable to notepad.exe.
	hr = pExecAction->put_Path(_bstr_t(wstrExecutablePath.c_str()));
	hr = pExecAction->put_Arguments(_bstr_t(L"/Update"));
	pExecAction->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put action path: %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  ------------------------------------------------------
	//  Save the task in the root folder.
	IRegisteredTask* pRegisteredTask = NULL;
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(wszTaskName),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask);
	if (FAILED(hr))
	{
		printf("\nError saving the Task : %x", hr);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	printf("\n Success! Task successfully registered. ");

	//  Clean up.
	pRootFolder->Release();
	pTask->Release();
	pRegisteredTask->Release();
	CoUninitialize();
	return 0;
}

// updates InIT services //
// Update Services
bool update()
{
	//std::shared_ptr<spdlog::logger> m_UpdateLogger;
	//m_UpdateLogger = spdlog::basic_logger_mt("InITUpdateLogger", SERVICE_LOG_FILEPATH);
	std::cout << "Checking for updates to InIT services..." << std::endl;

	// POST service versions
	std::string pstdata = "";
	//pstdata += SERVICE_IDENTIFY_POST;
	std::wstring guid = getClientGuid();
	std::string sguid(guid.begin(), guid.end());
	std::stringstream versions_out;
	std::stringstream verurl;
	verurl << SERVICE_URL << SERVICE_UPDATE_URL << sguid;
	// retreive intended versions
	CURLcode res;
	res = curlPost(verurl.str(), versions_out);
	std::cout << "CURL Return: " << res << std::endl;
	std::cout << versions_out.str() << std::endl;
	// Convert intended versions to JSON
	JSONParser serviceversions;
	//std::string versions = versions_out.str();
	//json json_versions = R"({"InITManager":"1.0","InITTESTING" : "null" })"_json;
	//this->m_ServiceLogger->info("JSON Parse: ", json_versions.get<std::string>());
	//rapidjson::Document jsonversions;
	//bool success = jsonversions.Parse(versions_out.str().c_str()).HasParseError();
	//std::ostringstream ss;
	//ss << std::boolalpha << success;
	if (serviceversions.parse(versions_out.str()))
	{
		std::cout << "JSON Parse: Successful!" << std::endl;
	}
	else
	{
		std::cout << "JSON Parse: Failed!" << std::endl;
	}


	// Prepare processing variables
	std::wstring intended_version;
	std::wstring current_version;
	std::wstring regpath;
	std::map<std::string, std::string>::iterator it;
	// Get JSON Parse result
	std::map<std::string, std::string> result = serviceversions.getResult();
	// For all service versions: update if current version is not intended version
	for (it = result.begin(); it != result.end(); it++)
	{
		// Read service name and version from service update request
		std::wstring wSvcName(it->first.begin(), it->first.end());
		std::wstring intended_version(it->second.begin(), it->second.begin());

		// Log JSON Parse Results
		std::stringstream parseresult;
		parseresult << "Service: " << it->first << " Version: " << it->second;
		std::cout << parseresult.str() << std::endl;

		// Get version
		HKEY hkey;
		regpath = SERVICE_DEFAULT_REG;
		regpath += (wSvcName + L"\\");
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hkey);
		GetStringRegKey(hkey, L"version", current_version, L"null");
		RegCloseKey(hkey);

		std::stringstream msg;
		std::string currver(current_version.begin(), current_version.end());
		msg << it->first << " " << currver << " -> " << it->second;
		std::cout << msg.str() << std::endl;

		// Does the client service match the server's intended version?
		// If not, update.
		if (current_version != intended_version)
		{
			// Does the intended version exist?
			// If so, stop service and update.
			if (intended_version != L"null")
			{
				// Build service executable path and arguments
				std::string SvcName(wSvcName.begin(), wSvcName.end());
				std::wstring servicepath_w = SERVICE_DEFAULT_PATH;
				std::string servicepath(servicepath_w.begin(), servicepath_w.end());
				std::stringstream executablepath;
				executablepath << "\"" << servicepath << "..\\" << SvcName << "\\" << SvcName << ".exe\"";
				std::string args = " /Stop";
				std::cout << "Stopping " << executablepath.str() << std::endl;
				// Stop the service so we can update
				int retCode = system((executablepath.str() + args).c_str());
				std::cout << retCode << std::endl;
				// Uninstall the service so we can update
				args = " /Uninstall";
				retCode = system((executablepath.str() + args).c_str());
				// update/guid/ => all services tied to that guid and their versions
				// update/guid/servicename => returns the intended version of that service.

				// update/guid/servicename/version => returns "filetag" for specified release and version.  
					// acccess https://init.sgcity.org/srv/files/tmplink.php?filetag=filetag.  Will download release if filetag hasn't been used.
				CURLcode res;
				// Get URL for download
				pstdata = "service=" + it->first + "&";
				pstdata += ("version=" + it->second);
				std::cout << "POST Data: " << pstdata << std::endl;
				std::stringstream download_out;
				res = curlPost(verurl.str(), download_out, pstdata);
				JSONParser urlparse(download_out.str());
				std::string downloadurl = urlparse.getResult().at("url");

				std::cout << "Download URL: " << downloadurl << "CURL return: " << res << std::endl;

				// Download the update package.
				std::wstring wdownloadpath = SERVICE_DEFAULT_PATH; wdownloadpath += L"package.zip";
				std::string downloadpath(wdownloadpath.begin(), wdownloadpath.end());
				downloadFile(downloadurl, downloadpath);
				std::cout << "Downloaded update for " << it->first << std::endl;

				// Unzip the update package

				std::wstringstream zippath_w;
				zippath_w << SERVICE_DEFAULT_PATH << L"package.zip";
				std::wstring zp = zippath_w.str();
				std::string zippath(zp.begin(), zp.end());
				
				std::wstringstream destpath_w;
				destpath_w << SERVICE_DEFAULT_PATH << L"..\\" << wSvcName << L"\\";
				std::wstring dp = destpath_w.str();
				std::string destpath(dp.begin(), dp.end());
				std::cout << "Unzipping " << zippath << " to: " << destpath << std::endl;
				//std::stringstream logmsg;
				//logmsg << "Unzipping " << zippath << " to: " << destpath;
				//m_UpdateLogger->info(logmsg.str());
				//m_UpdateLogger->flush();
				unzip(zippath, destpath);
				std::cout << "Installed update for " << it->first << std::endl;

				// Delete update package.zip
				if (remove(downloadpath.c_str()) != 0)
					perror("Error deleting file");
				else
					puts("File successfully deleted");
				std::cout << "Removed update package" << std::endl;

				// Install the service again
				args = " /Install";
				retCode = system((executablepath.str() + args).c_str());
				std::cout << retCode << std::endl;
				// Start service back up
				args = " /Start";
				retCode = system((executablepath.str() + args).c_str());
				std::cout << retCode << std::endl;
			}

		}
		// else do nothing
	}
	// If service version differs, 

	std::cout << "Completed updates." << std::endl;
	return true;
}

//-------------------------------------------------//
//	                    CURL                       //
//-------------------------------------------------//
// callback function writes data to a std::ostream //
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
{
	if (userp)
	{
		std::ostream& os = *static_cast<std::ostream*>(userp);
		std::streamsize len = size * nmemb;
		if (os.write(static_cast<char*>(buf), len))
			return len;
	}

	return 0;
}

// Post to website.  os will be modified to contain any returning information from url //
CURLcode curlPost(std::string url, std::ostream& os, std::string postdata, long timeout)
{
	// Variable instantiation
	CURL* curl;
	CURLcode res;

	// In windows, this will init the winsock stuff
	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	/* Check for errors */
	if (res != CURLE_OK)
	{
		return res;
	}

	// Begin CURL request intialization
	curl = curl_easy_init();
	if (curl)
	{
		// If post data is not provided, perform GET request.
		if (postdata == "")
		{
			if (CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
				&& CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
				&& CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
				&& CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_FILE, &os))
				&& CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
				&& CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())))
			{
				res = curl_easy_perform(curl);
			}
		}
		// Otherwise perform POST request
		else
		{
			if (CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_FILE, &os)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_POST, 1L)) &&
				CURLE_OK == (res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str()))
				)
			{
				res = curl_easy_perform(curl);
			}
		}

		/* Check for errors */
		if (res != CURLE_OK)
		{
			return res;
		}
		// Request cleanup
		curl_easy_cleanup(curl);
	}
	// Global cleanup
	curl_global_cleanup();
	// Return result of request
	return res;
}

// callback function writes data to a std::ostream //
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

// Downloads file specified in downloadurl, naming the new windows file as filename //
void downloadFile(std::string downloadurl, std::string filename)
{
	CURL* curl;
	FILE* fp;
	CURLcode res;
	std::string url = downloadurl;
	char outfilename[FILENAME_MAX];
	strcpy_s(outfilename, FILENAME_MAX, filename.c_str());

	curl_version_info_data* vinfo = curl_version_info(CURLVERSION_NOW);
	if (vinfo->features & CURL_VERSION_SSL)
		printf("CURL: SSL enabled\n");
	else
		printf("CURL: SSL not enabled\n");

	curl = curl_easy_init();
	if (curl)
	{
		fopen_s(&fp, outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		/* Setup the https:// verification options - note we do this on all requests as there may
		be a redirect from http to https and we still want to verify */
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
		//curl_easy_setopt(curl, CURLOPT_CAINFO, "initpub.crt");
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); //Prevent "longjmp causes uninitialized stack frame" bug
		//curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
}

//----------//
// REGISTRY //
//----------//
//          //
BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having
	// to recurse.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
		return TRUE;

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS)
	{
		if (lResult == ERROR_FILE_NOT_FOUND) {
			printf("Key not found.\n");
			return TRUE;
		}
		else {
			printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.

	lpEnd = lpSubKey + lstrlen(lpSubKey);

	if (*(lpEnd - 1) != TEXT('\\'))
	{
		*lpEnd = TEXT('\\');
		lpEnd++;
		*lpEnd = TEXT('\0');
	}

	// Enumerate the keys

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
		NULL, NULL, &ftWrite);

	if (lResult == ERROR_SUCCESS)
	{
		do {

			StringCchCopy(lpEnd, MAX_PATH * 2, szName);

			if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
				break;
			}

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
				NULL, NULL, &ftWrite);

		} while (lResult == ERROR_SUCCESS);
	}

	lpEnd--;
	*lpEnd = TEXT('\0');

	RegCloseKey(hKey);

	// Try again to delete the key.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
		return TRUE;

	return FALSE;
}

BOOL RegDelnode(HKEY hKeyRoot, LPTSTR lpSubKey)
{
	TCHAR szDelKey[MAX_PATH * 2];

	StringCchCopy(szDelKey, MAX_PATH * 2, lpSubKey);
	return RegDelnodeRecurse(hKeyRoot, szDelKey);
}

LONG GetDWORDRegKey(HKEY hKey, const std::wstring& strValueName, DWORD& nValue, DWORD nDefaultValue)
{
	nValue = nDefaultValue;
	DWORD dwBufferSize(sizeof(DWORD));
	DWORD nResult(0);
	LONG nError = ::RegQueryValueExW(hKey,
		strValueName.c_str(),
		0,
		NULL,
		reinterpret_cast<LPBYTE>(&nResult),
		&dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		nValue = nResult;
	}
	return nError;
}


LONG GetBoolRegKey(HKEY hKey, const std::wstring& strValueName, bool& bValue, bool bDefaultValue)
{
	DWORD nDefValue((bDefaultValue) ? 1 : 0);
	DWORD nResult(nDefValue);
	LONG nError = GetDWORDRegKey(hKey, strValueName.c_str(), nResult, nDefValue);
	if (ERROR_SUCCESS == nError)
	{
		bValue = (nResult != 0) ? true : false;
	}
	return nError;
}


LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue)
{
	strValue = strDefaultValue;
	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
	}
	return nError;
}

int GetMachineUUID(std::wstring& struuid)
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
	std::wcout << "GetMachineGuid(): " << struuid << std::endl;
	return 0;
}

//-------------//
// FILE SYSTEM //
//-------------//
//             //
int unzip(const std::string& zipPath, const std::string& desPath)
{
	int err;
	struct zip* hZip = zip_open(zipPath.c_str(), 0, &err);
	if (hZip)
	{
		size_t totalIndex = zip_get_num_entries(hZip, 0);
		std::cout << "ZIP-TotalEntries: " << totalIndex << std::endl;
		for (size_t i = 0; i < totalIndex; i++)
		{
			struct zip_stat st;
			zip_stat_init(&st);
			zip_stat_index(hZip, i, 0, &st);

			struct zip_file* zf = zip_fopen_index(hZip, i, 0);
			if (!zf)
			{
				zip_close(hZip);
				return 1;
			}

			std::vector<char> buffer;
			buffer.resize(st.size);
			zip_fread(zf, buffer.data(), st.size);
			zip_fclose(zf);

			// your code here: write buffer to file
			// desPath
			// st.name: the file name
			std::ofstream fout(desPath + st.name, std::ios::out | std::ios::binary);
			fout.write((char*)&buffer[0], buffer.size());
			fout.close();

		}
		zip_close(hZip);
	}
	return 0;
}

int DeleteDirectory(const std::wstring& refcstrRootDirectory, bool bDeleteSubdirectories)
{
	bool            bSubdirectory = false;       // Flag, indicating whether
												 // subdirectories have been found
	HANDLE          hFile;                       // Handle to directory
	std::wstring     strFilePath;                 // Filepath
	std::wstring     strPattern;                  // Pattern
	WIN32_FIND_DATA FileInformation;             // File information


	strPattern = refcstrRootDirectory + L"\\*.*";
	hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + L"\\" + FileInformation.cFileName;

				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bDeleteSubdirectories)
					{
						// Delete subdirectory
						int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
						if (iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					// Set file attributes
					if (::SetFileAttributes(strFilePath.c_str(),
						FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();

					// Delete file
					if (::DeleteFile(strFilePath.c_str()) == FALSE)
						return ::GetLastError();
				}
			}
		} while (::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if (!bSubdirectory)
			{
				// Set directory attributes
				if (::SetFileAttributes(refcstrRootDirectory.c_str(),
					FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();

				// Delete directory
				if (::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}

	return 0;
}