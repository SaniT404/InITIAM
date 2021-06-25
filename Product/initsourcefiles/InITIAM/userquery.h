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
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
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

class UserQuery
{
public:
	UserQuery();

	//std::wofstream getSoftOStream();
	std::wstring wStr();
	std::string str();

	void wInsert(std::wstring in, bool endoffile);
	void insert(std::string in, bool endoffile);

	const size_t getEntries() const;
protected:
	int initUserStream();
private:
	size_t m_Entries;
	std::wstring m_Delimiter;
	std::wstringstream m_UserWSStream;
	std::wstring m_GUID;
};

//std::wofstream& operator<<(std::wofstream& os, const SoftwareQuery& query);