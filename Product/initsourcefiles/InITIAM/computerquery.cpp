#include "computerquery.h"

ComputerQuery::ComputerQuery()
	: m_Delimiter(L"<|>"), m_Entries(0)
{
	this->initComputerQuery();
}

int ComputerQuery::initComputerQuery()
{
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

	this->initComputerSystem();
	this->initNetworkAdapterConfiguration();
	this->initOperatingSystem();
	this->initProcessor();
	this->initLogicalDisk();

	// GUID,Name,Manufacturer,Model,Domain,Workgroup,IPV4Address,IPV6Address,MACAddress,CPU,Cores,RAM,DriveCapacity,DriveSpace,OS,OSVersion,OSArchitecture,OSSerialNumber,OSInstallDate,LastBootUpTime,NumberOfUsers\n
	this->wInsert(strguid, false);
	this->wInsert(this->getComputerSystem().at(L"Name"), false);
	this->wInsert(this->getComputerSystem().at(L"Manufacturer"), false);
	this->wInsert(this->getComputerSystem().at(L"Model"), false);
	this->wInsert(this->getComputerSystem().at(L"Domain"), false);
	this->wInsert(this->getComputerSystem().at(L"Workgroup"), false);
	this->wInsert(this->getNetworkAdapterConfiguration().at(L"IPV4Address"), false);
	this->wInsert(this->getNetworkAdapterConfiguration().at(L"IPV6Address"), false);
	this->wInsert(this->getNetworkAdapterConfiguration().at(L"MACAddress"), false);
	this->wInsert(this->getProcessor().at(L"Name"), false);
	this->wInsert(this->getProcessor().at(L"NumberOfCores"), false);
	this->wInsert(this->getComputerSystem().at(L"TotalPhysicalMemory"), false);
	this->wInsert(this->getLogicalDisk().at(L"Size"), false);
	this->wInsert(this->getLogicalDisk().at(L"FreeSpace"), false);
	this->wInsert(this->getOperatingSystem().at(L"Caption"), false);
	this->wInsert(this->getOperatingSystem().at(L"Version"), false);
	this->wInsert(this->getOperatingSystem().at(L"OSArchitecture"), false);
	this->wInsert(this->getOperatingSystem().at(L"SerialNumber"), false);
	this->wInsert(this->getOperatingSystem().at(L"InstallDate"), false);
	this->wInsert(this->getOperatingSystem().at(L"LastBootUpTime"), false);
	this->wInsert(this->getOperatingSystem().at(L"NumberOfUsers"), true);
	return 0;
}

std::wstring ComputerQuery::wStr()
{
	return this->m_CompWSStream.str();
}

std::string ComputerQuery::str()
{
	std::wstring wstr = this->m_CompWSStream.str();
	std::string str(wstr.begin(), wstr.end());
	return str;
}

void ComputerQuery::wInsert(std::wstring in, bool endoffile)
{
	if (!endoffile)
		this->m_CompWSStream << in << this->m_Delimiter;
	else
	{
		this->m_CompWSStream << in << L"\n";
		this->m_Entries += 1;
	}
}

void ComputerQuery::insert(std::string in, bool endoffile)
{
	std::wstring win(in.begin(), in.end());
	if (!endoffile)
		this->m_CompWSStream << win << this->m_Delimiter;
	else
	{
		this->m_CompWSStream << win << "\n";
		this->m_Entries += 1;
	}
}

const size_t ComputerQuery::getEntries() const
{
	return this->m_Entries;
}

const std::map<std::wstring, std::wstring> ComputerQuery::getComputerSystem() const
{
	return this->m_ComputerSystem;
}

const std::map<std::wstring, std::wstring> ComputerQuery::getNetworkAdapterConfiguration() const
{
	return this->m_NetworkAdapterConfiguration;
}

const std::map<std::wstring, std::wstring> ComputerQuery::getOperatingSystem() const
{
	return this->m_OperatingSystem;
}

const std::map<std::wstring, std::wstring> ComputerQuery::getProcessor() const
{
	return this->m_Processor;
}

const std::map<std::wstring, std::wstring> ComputerQuery::getLogicalDisk() const
{
	return this->m_LogicalDisk;
}

int ComputerQuery::initComputerSystem() // Domain, Manufacturer, Model, Name, TotalPhysicalMemory, Workgroup
{
	this->m_ComputerSystem.clear();

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
		authIdent.PasswordLength = static_cast<ULONG>(wcslen(pszPwd));	// size_t > ULONG
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = static_cast<ULONG>(wcslen(pszName));	// size_t > ULONG
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_ComputerSystem",
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

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		hr = pclsObj->Get(L"Domain", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Domain", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Domain", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Domain", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Manufacturer", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Manufacturer", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Manufacturer", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Model", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Model", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Model", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Name", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Name", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Name", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"TotalPhysicalMemory", 0, &vtProp, 0, 0);// Uint64
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"TotalPhysicalMemory", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"TotalPhysicalMemory", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"TotalPhysicalMemory", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Workgroup", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Workgroup", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Workgroup", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_ComputerSystem.insert(std::pair<std::wstring, std::wstring>(L"Workgroup", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);


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

int ComputerQuery::initNetworkAdapterConfiguration()
{
	this->m_NetworkAdapterConfiguration.clear();

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
		authIdent.PasswordLength = static_cast<ULONG>(wcslen(pszPwd));	// size_t > ULONG
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = static_cast<ULONG>(wcslen(pszName));		// size_t > ULONG
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_NetworkAdapterConfiguration",
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

	int mac = 0;
	int ip = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		hr = pclsObj->Get(L"IPAddress", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{

			if ((vtProp.vt & VT_ARRAY) && ip == 0)
			{
				BSTR Element = NULL;
				SAFEARRAY *pSafeArray = vtProp.parray;

				for (long i = 0; i <= 1; i++)
				{
					hres = SafeArrayGetElement(pSafeArray, &i, &Element);
					if (i == 0)
					{
						std::wstringstream ss;
						ss << Element;
						this->m_NetworkAdapterConfiguration.insert(std::pair<std::wstring, std::wstring>(L"IPV4Address", ss.str()));
					}
					else if (i == 1)
					{
						std::wstringstream ss;
						ss << Element;
						this->m_NetworkAdapterConfiguration.insert(std::pair<std::wstring, std::wstring>(L"IPV6Address", ss.str()));
					}
				}

				//SafeArrayDestroy(pSafeArray);
				ip++;
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"MACAddress", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if (((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY)) && mac == 0)
			{
				this->m_NetworkAdapterConfiguration.insert(std::pair<std::wstring, std::wstring>(L"MACAddress", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_NetworkAdapterConfiguration.insert(std::pair<std::wstring, std::wstring>(L"MACAddress", L"Array types not supported (yet)"));
				}
				else
				{
					if (mac == 0)
					{
						std::wstringstream ss;
						ss << vtProp.bstrVal;
						this->m_NetworkAdapterConfiguration.insert(std::pair<std::wstring, std::wstring>(L"MACAddress", ss.str()));
						mac++;
					}
				}
			}
		}
		VariantClear(&vtProp);

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

int ComputerQuery::initOperatingSystem() // Caption, InstallDate, LastBootUpTime, NumberOfUsers, OSArchitecture, RegisteredUser, SerialNumber, Version
{
	this->m_OperatingSystem.clear();
	// The Win32_OperatingSystem class represents an operating system installed on a Win32 computer system. Any operating system that can be installed on a Win32 system is a descendent (or member) of this class.
	// Example: Microsoft Windows 95.

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
		authIdent.PasswordLength = static_cast<ULONG>(wcslen(pszPwd));	// size_t > ULONG
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = static_cast<ULONG>(wcslen(pszName));	// size_t > ULONG
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_OperatingSystem",
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

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Caption", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Caption", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Caption", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"InstallDate", 0, &vtProp, 0, 0);// DateTime
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"InstallDate", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"InstallDate", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"InstallDate", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);
		
		hr = pclsObj->Get(L"LastBootUpTime", 0, &vtProp, 0, 0);// DateTime
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"LastBootUpTime", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"LastBootUpTime", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"LastBootUpTime", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"NumberOfUsers", 0, &vtProp, 0, 0);// Uint32
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"NumberOfUsers", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"NumberOfUsers", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.uintVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"NumberOfUsers", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"OSArchitecture", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"OSArchitecture", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"OSArchitecture", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"OSArchitecture", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);// Uint16
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"SerialNumber", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"SerialNumber", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.uiVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"SerialNumber", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Version", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Version", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Version", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_OperatingSystem.insert(std::pair<std::wstring, std::wstring>(L"Version", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);


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

int ComputerQuery::initProcessor()
{
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
		authIdent.PasswordLength = static_cast<ULONG>(wcslen(pszPwd));	// size_t > ULONG
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = static_cast<ULONG>(wcslen(pszName));		// size_t > ULONG
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_Processor",
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

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"Name", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"Name", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"Name", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);
		
		hr = pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0);// Uint32
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"NumberOfCores", L"NULL"));
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"NumberOfCores", L"Array types not supported (yet)"));
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.uintVal;
					this->m_Processor.insert(std::pair<std::wstring, std::wstring>(L"NumberOfCores", ss.str()));
				}
			}
		}
		VariantClear(&vtProp);

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

int ComputerQuery::initLogicalDisk() // FreeSpace, Size
{
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
		authIdent.PasswordLength = static_cast<ULONG>(wcslen(pszPwd));	// size_t > ULONG
		authIdent.Password = (USHORT*)pszPwd;
		authIdent.User = (USHORT*)pszName;
		authIdent.UserLength = static_cast<ULONG>(wcslen(pszName));	// size_t > ULONG
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_LogicalDisk",
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

	int fspace = 0;
	int sizespace = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn || FAILED(hr))
			break;

		VARIANT vtProp;

		hr = pclsObj->Get(L"FreeSpace", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr) && fspace < 1)
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"FreeSpace", L"NULL"));
				fspace++;
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"FreeSpace", L"Array types not supported (yet)"));
					fspace++;
				}
				else
				{
					std::wstringstream ss;
					ss << (vtProp.bstrVal); // 1073741824 = Num of Bytes in GB.
					this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"FreeSpace", ss.str()));
					fspace++;
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Size", 0, &vtProp, 0, 0);// String
		if (!FAILED(hr) && sizespace < 1)
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"Size", L"NULL"));
				sizespace++;
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"Size", L"Array types not supported (yet)"));
					sizespace++;
				}
				else
				{
					std::wstringstream ss;
					ss << (vtProp.bstrVal); // 1073741824 = Num of Bytes in GB.
					this->m_LogicalDisk.insert(std::pair<std::wstring, std::wstring>(L"Size", ss.str()));
					sizespace++;
				}
			}
		}
		VariantClear(&vtProp);

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

