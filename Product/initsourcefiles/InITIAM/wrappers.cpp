#include "wrappers.h"


//--------------------------------------//
//             Installation             //
//--------------------------------------//
// installs necessary registry settings //
bool installReg()
{
	// Setup registry structure and values

	// Checks to see if strInITKeyName exists and if not, creates it.
	DWORD rtime = 0;
	HKEY InITKey;
	DWORD dwDisposition;
	std::wstring strInITManagerKeyName = SERVICE_HOME_REG;
	LONG nError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, strInITManagerKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &InITKey, &dwDisposition);

	if (ERROR_SUCCESS != nError)
	{
		std::cout << "Error: Could not open/create registry key HKEY_LOCAL_MACHINE\\" << SERVICE_HOME_REG << std::endl << "\tERROR: " << dwDisposition << std::endl;
		return false;
	}
	else
	{
		std::cout << "Successfully " << ((REG_CREATED_NEW_KEY == dwDisposition) ? "created" : "opened") << " registry key HKEY_LOCAL_MACHINE\\" << SERVICE_DEFAULT_REG << std::endl;
		
		// if version query fails OR updated query returns 1: Set version value
		HKEY hKey;
		std::wstring wver;
		DWORD upd;
		std::wstring regpath = SERVICE_HOME_REG;
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKey);
		GetStringRegKey(hKey, L"version", wver, L"null");
		GetDWORDRegKey(hKey, L"updated", upd, 0);
		RegCloseKey(hKey);

		if (wver == L"null" || upd == 1)
		{
			// Set version registry value.
			std::wstring ver = SERVICE_VERSION;
			nError = RegSetValueEx(InITKey, L"version", NULL, REG_SZ, (const BYTE*)ver.c_str(), static_cast<DWORD>((ver.size() + 1)) * sizeof(wchar_t));
			if (ERROR_SUCCESS != nError)
			{
				std::cout << "Error: Could not set registry value: version" << std::endl << "\tERROR: " << nError << std::endl;
				return false;
			}
			else
				std::wcout << "Successfully set InITIAM version to " << ver << std::endl;
		}

		// Set updated registry value.  (depending on situation, will create it or update it)
		DWORD updated = 0;
		nError = RegSetValueEx(InITKey, L"updated", NULL, REG_DWORD, (const BYTE*)&updated, sizeof(updated));
		if (ERROR_SUCCESS != nError)
		{
			std::cout << "Error: Could not set registry value: version" << std::endl << "\tERROR: " << nError << std::endl;
			return false;
		}
		else
			std::wcout << "Successfully set InITIAM updated value to " << updated << std::endl;
	}

	RegCloseKey(InITKey);
	return true;
}

// installs necessary directory settings //
bool installDir()
{
	std::wcout << "Setting up InIT directory..." << std::endl;
	// System32 "Home" path
	if (CreateDirectory(SERVICE_DEFAULT_PATH, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
	{
		// Service sub-directory
		if (CreateDirectory(SERVICE_HOME_PATH, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
		{
			std::wcout << "System32 directory InIT\\InITIAM successfully created" << std::endl;
			// Service's log sub-directory
			if (CreateDirectory(SERVICE_LOG_PATH, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
			{
				std::wcout << "InIT/InITIAM/logs directory successfully created" << std::endl;
			}
			else
			{
				std::wcout << "Failed to create InIT/InITIAM/logs directory." << std::endl;
				return false;
			}
		}
		else
		{
			std::wcout << "Failed to create System32 folder InIT\\InITIAM." << std::endl;
			return false;
		}
	}
	else
	{
		std::wcout << "Failed to create System32 folder InIT." << std::endl;
		return false;
	}
	
	return true;
}

// checks to see if (on start) service was just updated //
bool wasUpdated()
{
	// Set member variable std::wstring m_ClientGuid from registry
	HKEY hKey;
	DWORD updated;
	std::wstring regpath = SERVICE_HOME_REG;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKey);
	GetDWORDRegKey(hKey, L"updated", updated, 1);

	if (updated != 1)
	{
		return false;
	}
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
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

// Downloads file specified in downloadurl, naming the new windows file as filename //
void downloadFile(std::string downloadurl, std::string filename)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	std::string url = downloadurl;
	char outfilename[FILENAME_MAX];
	strcpy_s(outfilename, FILENAME_MAX, filename.c_str());

	curl_version_info_data * vinfo = curl_version_info(CURLVERSION_NOW);
	if (vinfo->features & CURL_VERSION_SSL)
		printf("CURL: SSL enabled\n");
	else
		printf("CURL: SSL not enabled\n");

	curl = curl_easy_init();
	if (curl)
	{
		errno_t err;
		err = fopen_s(&fp, outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		/* Setup the https:// verification options - note we do this on all requests as there may
		be a redirect from http to https and we still want to verify */
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(curl, CURLOPT_CAINFO, "initpub.crt");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

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

LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
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


LONG GetBoolRegKey(HKEY hKey, const std::wstring &strValueName, bool &bValue, bool bDefaultValue)
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


LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
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
bool Unzip2Folder(BSTR lpZipFile, BSTR lpFolder)
{
	IShellDispatch *pISD;
	Folder  *pZippedFile = 0L;
	Folder  *pDestination = 0L;

	long FilesCount = 0;
	IDispatch* pItem = 0L;
	FolderItems *pFilesInside = 0L;

	VARIANT Options, OutFolder, InZipFile, Item;
	CoInitialize(NULL);
	__try {
		if (CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD) != S_OK)
			return 1;

		InZipFile.vt = VT_BSTR;
		InZipFile.bstrVal = lpZipFile;
		pISD->NameSpace(InZipFile, &pZippedFile);
		if (!pZippedFile)
		{
			pISD->Release();
			return 1;
		}

		OutFolder.vt = VT_BSTR;
		OutFolder.bstrVal = lpFolder;
		pISD->NameSpace(OutFolder, &pDestination);
		if (!pDestination)
		{
			pZippedFile->Release();
			pISD->Release();
			return 1;
		}

		pZippedFile->Items(&pFilesInside);
		if (!pFilesInside)
		{
			pDestination->Release();
			pZippedFile->Release();
			pISD->Release();
			return 1;
		}

		pFilesInside->get_Count(&FilesCount);
		if (FilesCount < 1)
		{
			pFilesInside->Release();
			pDestination->Release();
			pZippedFile->Release();
			pISD->Release();
			return 0;
		}

		pFilesInside->QueryInterface(IID_IDispatch, (void**)&pItem);

		Item.vt = VT_DISPATCH;
		Item.pdispVal = pItem;

		Options.vt = VT_I4;
		Options.lVal = 1024 | 512 | 16 | 4;//http://msdn.microsoft.com/en-us/library/bb787866(VS.85).aspx

		bool retval = pDestination->CopyHere(Item, Options) == S_OK;

		pItem->Release(); pItem = 0L;
		pFilesInside->Release(); pFilesInside = 0L;
		pDestination->Release(); pDestination = 0L;
		pZippedFile->Release(); pZippedFile = 0L;
		pISD->Release(); pISD = 0L;

		return retval;

	}
	__finally
	{
		CoUninitialize();
	}
}

// Deletes specified directory, and deletes sub-directories if bDeleteSubdirectories = true (default)
int DeleteDirectory(const std::wstring &refcstrRootDirectory, bool bDeleteSubdirectories)
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