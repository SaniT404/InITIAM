#pragma once

#include "definitions.h"
#include <curl/curl.h>
#include <windows.h>
#include <Shldisp.h>
#include <strsafe.h>
#include <sstream>
#include <fstream>
#include <iostream>

//--------------------------------------//
//             Installation             //
//--------------------------------------//
// installs necessary registry settings //
bool installReg();

// installs necessary directory settings //
bool installDir();

// checks to see if (on start) service was just updated //
bool wasUpdated();

//-------------------------------------------------//
//	                    CURL                       //
//-------------------------------------------------//
// callback function writes data to a std::ostream //
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp);

// Send web request.  os will be modified to contain any returning information from url.	//
// Include postdata if performing POST request.												//
CURLcode curlPost(std::string url, std::ostream& os, std::string postdata = "", long timeout = 30L);

// callback function writes data to a std::ostream //
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

// Downloads file specified in downloadurl, naming the new windows file as filename //
void downloadFile(std::string downloadurl, std::string filename);

//---------------------------------------------------------------------------------------------//
//                                           REGISTRY                                          //
//---------------------------------------------------------------------------------------------//
// gets the subkey of hKeyRoot if exists and re-call itself else delete key and return success //
BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey);

// Calls RegDelnodeRecurse with the parameters provided //
BOOL RegDelnode(HKEY hKeyRoot, LPTSTR lpSubKey);

// Gets specified key.  Choose one based on return type needed.
LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue);
LONG GetBoolRegKey(HKEY hKey, const std::wstring &strValueName, bool &bValue, bool bDefaultValue);
LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue);

// Specifically retrieves MachineUUID.  Use to ease burden of querying Registry through the above registry getters //
int GetMachineUUID(std::wstring& struuid);

//--------------------------------------------//
//                 FILE SYSTEM                //
//--------------------------------------------//
// Unzips lpZipFile to the specified lpFolder //
bool Unzip2Folder(BSTR lpZipFile, BSTR lpFolder);

// Deletes specified directory, and deletes sub-directories if bDeleteSubdirectories = true (default)
int DeleteDirectory(const std::wstring &refcstrRootDirectory, bool bDeleteSubdirectories = true);