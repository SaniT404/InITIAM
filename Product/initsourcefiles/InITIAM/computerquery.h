/****************************** Module Header ******************************\
* Module Name:  wmi_computer.h
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
*
* Provides a wmi computer class that derives from the service base class -
* CServiceBase. The sample service logs the service start and stop
* information to the Application event log, and shows how to run the main
* function of the service in a thread pool worker thread.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "stdafx.h"
#define _WIN32_DCOM
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")

//CREDENTIAL structure
//http://msdn.microsoft.com/en-us/library/windows/desktop/aa374788%28v=vs.85%29.aspx
#define CRED_MAX_USERNAME_LENGTH            513
#define CRED_MAX_CREDENTIAL_BLOB_SIZE       512
#define CREDUI_MAX_USERNAME_LENGTH CRED_MAX_USERNAME_LENGTH
#define CREDUI_MAX_PASSWORD_LENGTH (CRED_MAX_CREDENTIAL_BLOB_SIZE / 2)

class ComputerQuery
{
public:
	ComputerQuery();

	std::wstring wStr();
	std::string str();

	void wInsert(std::wstring in, bool endoffile);
	void insert(std::string in, bool endoffile);

	const size_t getEntries() const;

	const std::map<std::wstring, std::wstring> getComputerSystem() const;
	const std::map<std::wstring, std::wstring> getNetworkAdapterConfiguration() const;
	const std::map<std::wstring, std::wstring> getOperatingSystem() const;
	const std::map<std::wstring, std::wstring> getProcessor() const;
	const std::map<std::wstring, std::wstring> getLogicalDisk() const;

	std::map<std::wstring, std::wstring> m_ComputerSystem;
	std::map<std::wstring, std::wstring> m_NetworkAdapterConfiguration;
	std::map<std::wstring, std::wstring> m_OperatingSystem;
	std::map<std::wstring, std::wstring> m_Processor;
	std::map<std::wstring, std::wstring> m_LogicalDisk;
protected:
	int initComputerQuery();

	int initComputerSystem();
	int initNetworkAdapterConfiguration();
	int initOperatingSystem();
	int initProcessor();
	int initLogicalDisk();
private:
	size_t m_Entries;
	std::wstring m_Delimiter;
	std::wstringstream m_CompWSStream;
};