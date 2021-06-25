#include "userquery.h"

UserQuery::UserQuery()
	: m_Delimiter(L"<|>")
{
	// Machine GUID
	std::wstring strguid;
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

	strguid = buf;
	this->m_GUID = strguid;

	this->initUserStream();
}

std::wstring UserQuery::wStr()
{
	return this->m_UserWSStream.str();
}

std::string UserQuery::str()
{
	std::wstring wstr = this->m_UserWSStream.str();
	std::string str(wstr.begin(), wstr.end());
	return str;
}

void UserQuery::wInsert(std::wstring in, bool endoffile)
{
	if (!endoffile)
		this->m_UserWSStream << in << this->m_Delimiter;
	else
	{
		this->m_UserWSStream << in << L"\n";
		this->m_Entries += 1;
	}
}

void UserQuery::insert(std::string in, bool endoffile)
{
	std::wstring win(in.begin(), in.end());
	if (!endoffile)
		this->m_UserWSStream << win << this->m_Delimiter;
	else
	{
		this->m_UserWSStream << win << L"\n";
		this->m_Entries += 1;
	}
}

const size_t UserQuery::getEntries() const
{
	return this->m_Entries;
}

bool IsBrowsePath(const std::wstring& path)
{
	return (path == _T(".") || path == _T(".."));
}

size_t calculateDirSize(std::wstring &path, vector<std::wstring> *errVect = NULL, uint64_t size = 0) 
{
	WIN32_FIND_DATA data;
	HANDLE sh = NULL;
	sh = FindFirstFile((path + L"\\*").c_str(), &data);

	if (sh == INVALID_HANDLE_VALUE)
	{
		//if we want, store all happened error  
		if (errVect != NULL)
			errVect->push_back(path);
		return size;
	}

	do
	{
		// skip current and parent
		if (!IsBrowsePath(data.cFileName))
		{
			// if found object is ...
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				// directory, then search it recursievly
				size = calculateDirSize(path + L"\\" + data.cFileName, NULL, size);
			else
				// otherwise get object size and add it to directory size
				size += (uint64_t)(data.nFileSizeHigh * (MAXDWORD)+data.nFileSizeLow);
		}

	} while (FindNextFile(sh, &data)); // do

	FindClose(sh);

	return size;
}

bool fileExists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

int UserQuery::initUserStream()
{
	std::wstring delimiter = L"<|>";

	wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = L"user";
	wchar_t pszPwd[CREDUI_MAX_PASSWORD_LENGTH + 1] = L"password";
	BSTR strNetworkResource;
	//To use a WMI remote connection set localconn to false and configure the values of the pszName, pszPwd and the name of the remote machine in strNetworkResource
	bool localconn = true;
	strNetworkResource = localconn ? L"\\\\.\\root\\CIMV2" : L"\\\\remote--machine\\root\\CIMV2";

	COAUTHIDENTITY *userAcct = NULL;
	COAUTHIDENTITY authIdent;

	// Initialize COM. ------------------------------------------

	HRESULT hres;
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		return 1;                  // Program has failed.
	}

	// Set general COM security levels --------------------------

	if (localconn)
		hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities
			NULL                         // Reserved
		);
	else
		hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
			RPC_C_IMP_LEVEL_IDENTIFY,    // Default Impersonation
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities
			NULL                         // Reserved
		);

	if (FAILED(hres))
	{
		cout << "Failed to initialize security. Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		CoUninitialize();
		return 1;                    // Program has failed.
	}

	// Obtain the initial locator to WMI -------------------------

	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object." << " Err code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices *pSvc = NULL;

	if (localconn)
		hres = pLoc->ConnectServer(
			_bstr_t(strNetworkResource),      // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object
			&pSvc                    // pointer to IWbemServices proxy
		);
	else
		hres = pLoc->ConnectServer(
			_bstr_t(strNetworkResource),  // Object path of WMI namespace
			_bstr_t(pszName),             // User name
			_bstr_t(pszPwd),              // User password
			NULL,                // Locale
			NULL,                // Security flags
			NULL,				 // Authority
			NULL,                // Context object
			&pSvc                // IWbemServices proxy
		);

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

	cout << "Connected to root\\CIMV2 WMI namespace" << endl;

	// Set security levels on the proxy -------------------------
	if (localconn)
		hres = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities
		);
	else
	{
		// Create COAUTHIDENTITY that can be used for setting security on proxy
		memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
		authIdent.PasswordLength = wcslen(pszPwd);
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = wcslen(pszName);
		authIdent.Domain = 0;
		authIdent.DomainLength = 0;
		authIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
		userAcct = &authIdent;

		hres = CoSetProxyBlanket(
			pSvc,                           // Indicates the proxy to set
			RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
			COLE_DEFAULT_PRINCIPAL,         // Server principal name
			RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
			userAcct,                       // client identity
			EOAC_NONE                       // proxy capabilities
		);
	}

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket. Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Use the IWbemServices pointer to make requests of WMI ----

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_UserProfile",
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres))
	{
		cout << "ExecQuery failed" << " Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Secure the enumerator proxy
	if (!localconn)
	{

		hres = CoSetProxyBlanket(
			pEnumerator,                    // Indicates the proxy to set
			RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
			COLE_DEFAULT_PRINCIPAL,         // Server principal name
			RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
			userAcct,                       // client identity
			EOAC_NONE                       // proxy capabilities
		);

		if (FAILED(hres))
		{
			cout << "Could not set proxy blanket on enumerator. Error code = 0x" << hex << hres << endl;
			cout << _com_error(hres).ErrorMessage() << endl;
			pEnumerator->Release();
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return 1;               // Program has failed.
		}
	}

	// Get the data from the WQL sentence
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	//this->m_UserWSStream << L"LocalPath,SID,LastUseTime,Status,HealthStatus,DesktopSize,DocumentsSize,PicturesSize,MusicSize,VideosSize,FavoritesSize,DownloadsSize,LinksSize,SearchesSize,SavedGamesSize,AppdataRoamingSize\n";
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		std::wstring userpath = L"";
		hr = pclsObj->Get(L"LocalPath", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				userpath = L"";
				this->wInsert(this->m_GUID, false);
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					userpath = L"";
					this->wInsert(this->m_GUID, false);
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					userpath = ss.str();
					this->wInsert(this->m_GUID, false);
					this->wInsert(ss.str(), false);
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"SID", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->wInsert(ss.str(), false);
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"LastUseTime", 0, &vtProp, 0, 0);// Datetime
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->wInsert(ss.str(), false);
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Status", 0, &vtProp, 0, 0);// Uint32
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.uintVal;
					std::wstring ssstr = ss.str();
					if (ssstr == L"8")
						this->wInsert(L"Corrupted", false);
					else if (ssstr == L"4")
						this->wInsert(L"Mandatory", false);
					else if (ssstr == L"2")
						this->wInsert(L"Roaming", false);
					else if (ssstr == L"1")
						this->wInsert(L"Temporary", false);
					else
						this->wInsert(L"Undefined", false);
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"HealthStatus", 0, &vtProp, 0, 0);// Uint8
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bVal;
					std::wstring ssstr = ss.str();
					if (ssstr == L"3")
						this->wInsert(L"NULL", false);
					else if (ssstr == L"2")
						this->wInsert(L"Caution", false);
					else if (ssstr == L"1")
						this->wInsert(L"Unhealthy", false);
					else if (ssstr == L"0")
						this->wInsert(L"Healthy", false);
					else
						this->wInsert(L"Undefined", false);
				}
			}
		}

		VariantClear(&vtProp);

		std::string strUserPath(userpath.begin(), userpath.end());
		unsigned long long desktopsize, documentssize, picturessize, musicsize, videossize, favoritessize, downloadssize, linkssize, searchessize, savedgamessize, appdataroamingsize;
		if (userpath != L"")
		{
			if (fileExists(strUserPath))											// Desktop
			{
				desktopsize = calculateDirSize(userpath + L"\\Desktop");
				//desktopsize = desktopsize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << desktopsize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Documents
			{
				documentssize = calculateDirSize(userpath + L"\\Documents");
				//documentssize = documentssize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << documentssize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Pictures
			{
				picturessize = calculateDirSize(userpath + L"\\Pictures");
				//picturessize = picturessize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << picturessize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Music
			{
				musicsize = calculateDirSize(userpath + L"\\Music");
				//musicsize = musicsize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << musicsize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Videos
			{
				videossize = calculateDirSize(userpath + L"\\Videos");
				//videossize = videossize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << videossize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Favorites
			{
				favoritessize = calculateDirSize(userpath + L"\\Favorites");
				//favoritessize = favoritessize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << favoritessize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Downloads
			{
				downloadssize = calculateDirSize(userpath + L"\\Downloads");
				//downloadssize = downloadssize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << downloadssize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Links
			{
				linkssize = calculateDirSize(userpath + L"\\Links");
				//linkssize = linkssize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << linkssize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Searches
			{
				searchessize = calculateDirSize(userpath + L"\\Searches");
				//searchessize = searchessize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << searchessize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// Saved Games
			{
				savedgamessize = calculateDirSize(userpath + L"\\Saved Games");
				//savedgamessize = savedgamessize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << savedgamessize;
				this->wInsert(ss.str(), false);
			}
			else
				this->wInsert(L"NULL", false);

			if (fileExists(strUserPath))											// AppdataRoaming
			{
				appdataroamingsize = calculateDirSize(userpath + L"\\AppData\\Roaming");
				//appdataroamingsize = appdataroamingsize / 1048576; // 1048576 = MB.
				std::wstringstream ss;
				ss << std::fixed;
				ss << appdataroamingsize;
				this->wInsert(ss.str(), true);
				ss << std::scientific;
			}
			else
				this->wInsert(L"NULL", true);
		}
		else
		{
			for (int i = 0; i < 10; i++)
			{
				this->wInsert(L"NULL", false);
			}
			this->wInsert(L"NULL", true);
		}

		pclsObj->Release();
		pclsObj = NULL;
	}

	// Cleanup

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	if (pclsObj != NULL)
		pclsObj->Release();

	CoUninitialize();
	return 0;   // Program successfully completed.
}