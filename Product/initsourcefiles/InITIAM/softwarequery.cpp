#include "softwarequery.h"

SoftwareQuery::SoftwareQuery()
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

	this->initSoftwareStream();
}

std::wstring SoftwareQuery::wStr()
{
	return this->m_SoftWSStream.str();
}

std::string SoftwareQuery::str()
{
	std::wstring wstr = this->m_SoftWSStream.str();
	std::string str(wstr.begin(), wstr.end());
	return str;
}

void SoftwareQuery::wInsert(std::wstring in, bool endoffile)
{
	if (!endoffile)
		this->m_SoftWSStream << in << m_Delimiter;
	else
	{
		this->m_SoftWSStream << in << L"\n";
		this->m_Entries += 1;
	}
}

void SoftwareQuery::insert(std::string in, bool endoffile)
{
	std::wstring win(in.begin(), in.end());
	if (!endoffile)
		this->m_SoftWSStream << win << m_Delimiter;
	else
	{
		this->m_SoftWSStream << win << L"\n";
		this->m_Entries += 1;
	}
}

const size_t SoftwareQuery::getEntries() const
{
	return this->m_Entries;
}

int SoftwareQuery::initSoftwareStream()
{
	// Query
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
	hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_Product",
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

	//this->m_SoftWSStream << L"Name,Vendor,Version,InstallDate,InstallState\n";
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
				this->wInsert(this->m_GUID, false);
				this->wInsert(L"NULL", false);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(this->m_GUID, false);
					this->wInsert(L"Array types not supported (yet)", false);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.bstrVal;
					this->wInsert(this->m_GUID, false);
					this->wInsert(ss.str(), false);
				}
			}
		}
		VariantClear(&vtProp);

		hr = pclsObj->Get(L"Vendor", 0, &vtProp, 0, 0);// String
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

		hr = pclsObj->Get(L"Version", 0, &vtProp, 0, 0);// String
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

		hr = pclsObj->Get(L"InstallDate", 0, &vtProp, 0, 0);// String
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

		hr = pclsObj->Get(L"InstallState", 0, &vtProp, 0, 0);// Sint16
		if (!FAILED(hr))
		{
			if ((vtProp.vt == VT_NULL) || (vtProp.vt == VT_EMPTY))
			{
				this->wInsert(L"NULL", true);
			}
			else
			{
				if ((vtProp.vt & VT_ARRAY))
				{
					this->wInsert(L"Array types not supported (yet)", true);
				}
				else
				{
					std::wstringstream ss;
					ss << vtProp.iVal;
					this->wInsert(ss.str(), true);
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