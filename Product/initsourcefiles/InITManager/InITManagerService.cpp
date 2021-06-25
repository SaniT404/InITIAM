/****************************** Module Header ******************************\
* Module Name:  InITUpdateService.cpp
* Project:      InITManager
*
* Provides a sample service class that derives from the service base class -
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
#pragma region Includes
#include "definitions.h"
#include "wrappers.h"
#include "InITManagerService.h"
#include "ThreadPool.h"
//#include "computerquery.h"
//#include "softwarequery.h"
//#include "userquery.h"
//#include "SimpleIni.h"
#include "jsonparse.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <Windows.h>
#include <Wtsapi32.h>
#include <Userenv.h>
#include <atlstr.h>
#include <stdio.h>

#include <comdef.h>
#include <Wbemidl.h>
#include <curl/curl.h>
#pragma endregion

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1;

InITManagerService::InITManagerService(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue) :
	InITManagerServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue),
	m_ServiceLogger(spdlog::basic_logger_mt("InITManagerLogger", SERVICE_LOG_FILEPATH))
{
	m_fStopping = FALSE;

	// Create a manual-reset event that is not signaled at first to indicate 
	// the stopped signal of the service.
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hStoppedEvent == NULL)
	{
		throw GetLastError();
	}
}


InITManagerService::~InITManagerService(void)
{
	if (m_hStoppedEvent)
	{
		CloseHandle(m_hStoppedEvent);
		m_hStoppedEvent = NULL;
	}
}

bool InITManagerService::setClientGuid(std::string guid)
{
	if (1 == 1) // use regex to check validity of guid;  return false if invalid
	{
		this->m_ClientGuid = guid;
		return true;
	}
	else
		return false;
}

bool InITManagerService::setClientGuid(std::wstring wguid)
{
	if (1 == 1) // use regex to check validity of guid;  return false if invalid
	{
		std::string guid(wguid.begin(), wguid.end());
		this->m_ClientGuid = guid;
		return true;
	}
	else
		return false;
}

// uses chrono and ctime libraries
int InITManagerService::setSchedulingUtility(int hour, int minutes, int seconds, int period)
{
	struct tm ct;
	std::time_t t = std::time(nullptr);																	// Set time variable to nothing
	gmtime_s(&ct, &t);																					// Set current time
	ct.tm_min = minutes;
	ct.tm_sec = seconds;																				// Set current minute and second to 0
	ct.tm_hour = hour;																					// Set current hour to 01
	ct.tm_mday++;																						// Add 1 day
	t = std::mktime(&ct);																				// Set t to new scheduled time (1:00AM)

	std::chrono::system_clock::time_point tomorrow_at_1 = std::chrono::system_clock::from_time_t(t);	// Set tomorrow_at_1 to t (scheduled time)

	auto until_scheduled = (tomorrow_at_1 - std::chrono::system_clock::now());							// Find how long it is until scheduled time

	std::chrono::duration<int, std::ratio<60 * 60 * 24> > one_day(period);								// Create one day duration
	this->m_timePeriod = one_day;																		// Set m_timePeriod to one_day duration

	std::chrono::system_clock::time_point today = std::chrono::system_clock::now();						// today = now()
	std::chrono::system_clock::time_point tomorrow = today + until_scheduled;							// Set next scheduled time by adding now() and time between now and next schedule

	char todaystr[26];
	std::time_t time_now = std::chrono::system_clock::to_time_t(today);									// PRINT NOW()
	ctime_s(todaystr, sizeof(todaystr), &this->m_timeScheduled);
	std::cout << "Today is: " << todaystr << std::endl;													//

	char tomorrowstr[26];
	this->m_timeScheduled = std::chrono::system_clock::to_time_t(tomorrow);								// PRINT NEXT SCHEDULED TIME
	ctime_s(tomorrowstr, sizeof(tomorrowstr), &this->m_timeScheduled);
	std::cout << "Tomorrow is: " << tomorrowstr << std::endl;											//

	return 0;
}

const std::string InITManagerService::getClientGuid() const
{
	return this->m_ClientGuid;
}

const std::wstring InITManagerService::getClientGuidw() const
{
	std::wstring guid(this->m_ClientGuid.begin(), this->m_ClientGuid.end());
	return guid;
}

std::string InITManagerService::getInITServiceVersion(const std::string servicename)
{
	std::string versionstr = "";

	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;

	// Open the local default service control manager database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT |
		SC_MANAGER_CREATE_SERVICE);
	if (schSCManager == NULL)
	{
		wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}

	// InITInventoryManger Service
	schService = OpenService(schSCManager, L"CppWindowsService", SC_MANAGER_CONNECT);
	if (schService == NULL)
	{
		wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
		versionstr += "NULL;";
	}
	// get service version and append to versionstr<----------------------------------------------------------------------------------------

	// Future InIT services here______________________________

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}
	return "";
}
/*
// callback function writes data to a std::ostream
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

CURLcode curlPost(std::string url, std::string postdata, std::ostream& os, long timeout = 30)
{
	CURL *curl;
	CURLcode res;

	bool succeeded = true;
	std::string returndata;

	/* In windows, this will init the winsock stuff */
	//res = curl_global_init(CURL_GLOBAL_DEFAULT);
	/* Check for errors *//*
	if (res != CURLE_OK)
	{
		return res;
	}

	curl = curl_easy_init();
	if (curl)
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

		/* Check for errors *//*
		if (res != CURLE_OK)
		{
			return res;
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return res;
}
*/
//
//   FUNCTION: CSampleService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void InITManagerService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{

	// Log a service start message to the Application log.
	WriteEventLogEntry(L"InITManager Starting in OnStart",EVENTLOG_INFORMATION_TYPE);
	this->m_ServiceLogger->info("***SERVICE_STARTING***");

	this->setSchedulingUtility(1, 0, 0, 1);		// 1 = hour, 0 = minute, 0 = second, 1 = "daily"... 1:00 AM daily
	this->m_ServiceLogger->info("Set scheduling utility");
	this->m_ServiceLogger->flush();

	// Set member variable std::wstring m_ClientGuid from registry
	HKEY hKeyguid;
	std::wstring wcguid;
	std::wstring regpath = SERVICE_DEFAULT_REG;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKeyguid);
	GetStringRegKey(hKeyguid, L"clientguid", wcguid, L"null");
	RegCloseKey(hKeyguid);
	this->setClientGuid(wcguid);

	// Get client version
	HKEY hKeyver;
	LONG nError;
	std::wstring wcver;
	regpath = SERVICE_HOME_REG;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKeyver);
	GetStringRegKey(hKeyver, L"version", wcver, L"null");
	RegCloseKey(hKeyver);

	// Update registry if client version is different
	if (wcver != SERVICE_VERSION)
	{
		HKEY hKey;
		std::wstring regpath = SERVICE_HOME_REG;
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_SET_VALUE | KEY_WOW64_64KEY, &hKey);
		nError = RegSetValueEx(hKey, L"version", NULL, REG_SZ, (const BYTE*)wcver.c_str(), static_cast<DWORD>((wcver.size() + 1)) * sizeof(wchar_t));
		if (ERROR_SUCCESS != nError)
		{
			std::cout << "Error: Could not set registry value: version" << std::endl << "\tERROR: " << nError << std::endl;
		}
		else
			std::wcout << "Successfully set InITManager version to " << wcver << std::endl;
	}
	

	// Queue the main service function for execution in a worker thread.
	CThreadPool::QueueUserWorkItem(&InITManagerService::ServiceWorkerThread, this);
}

/*
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}
void downloadUpdate(std::string downloadurl, char* filename)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	std::string url = downloadurl;
	char outfilename[FILENAME_MAX] = "";
	curl = curl_easy_init();
	if (curl) 
	{
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
}
*/

// Update Services
void InITManagerService::update()
{
	this->m_ServiceLogger->info("Checking for updates to InIT services...");
	this->m_ServiceLogger->flush();

	// POST service versions
	std::string pstdata = "";
	//pstdata += SERVICE_IDENTIFY_POST;
	std::stringstream versions_out;
	std::stringstream verurl;
	verurl << SERVICE_URL << SERVICE_UPDATE_URL << this->getClientGuid();
	// retreive intended versions
	CURLcode res;
	res = curlPost(verurl.str(), versions_out);
	this->m_ServiceLogger->info("CURL Return: ", res);
	this->m_ServiceLogger->info(versions_out.str());
	this->m_ServiceLogger->flush();
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
		this->m_ServiceLogger->info("JSON Parse: Successful!");
		this->m_ServiceLogger->flush();
	}
	else
	{
		this->m_ServiceLogger->info("JSON Parse: Failed!");
		this->m_ServiceLogger->flush();
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
		this->m_ServiceLogger->info(parseresult.str());
		this->m_ServiceLogger->flush();

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
		this->m_ServiceLogger->info(msg.str());
		this->m_ServiceLogger->flush();

		// Does the client service match the server's intended version?
		// If not, update.
		if (current_version != intended_version)
		{
			// Does the intended version exist?
			// If so, stop service and update.
			if(intended_version != L"null")
			{
				// Stop the service so we can update
				/*
				SC_HANDLE serviceDbHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
				SC_HANDLE serviceHandle = OpenService(serviceDbHandle, wSvcName.c_str(), SC_MANAGER_ALL_ACCESS);

				//SERVICE_STATUS_PROCESS status;
				DWORD bytesNeeded;
				QueryServiceStatusEx(serviceHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded);
				
				std::wstringstream logmessage;
				logmessage << "Stopping " << wSvcName << " - Version: " << current_version << " - Intended Version: " << intended_version;
				this->m_ServiceLogger->info(logmessage.str());*/
				/*
				if (status.dwCurrentState == SERVICE_RUNNING)
				{
					// Send stop code
					BOOL b = ControlService(serviceHandle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);

					if (b)
						this->m_ServiceLogger->info("Stopped InITIAM");
					else
						this->m_ServiceLogger->info("Failed to stop InITIAM");
				}
				else
				{
					this->m_ServiceLogger->info("InITIAM already stopped");
				}
				*/

				// update/guid/ => all services tied to that guid and their versions
				// update/guid/servicename => returns the intended version of that service.

				// update/guid/servicename/version => returns "filetag" for specified release and version.  
					// acccess https://init.sgcity.org/srv/files/tmplink.php?filetag=filetag.  Will download release if filetag hasn't been used.
				
				// Get URL for download
				pstdata = "service=" + it->first;
				std::stringstream download_out;
				curlPost(verurl.str(), download_out, pstdata);

				// Download the update package.
				downloadFile(download_out.str(), "package.zip");
				this->m_ServiceLogger->info("Downloaded update for InITIAM");

				// Unzip the update package
				/*
				std::wstring path = L"..\\" + wSvcName + L"\\";
				BSTR bstrpath = SysAllocString(path.c_str());
				Unzip2Folder(L"package.zip", bstrpath);
				this->m_ServiceLogger->info("Installed update for InITIAM");

				// Delete update package.zip
				DeleteDirectory(L"package.zip");
				this->m_ServiceLogger->info("Removed update package");*/

				// Start service back up
				/*
				this->m_ServiceLogger->info("Starting InITIAM back up");
				QueryServiceStatusEx(serviceHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded);

				if (status.dwCurrentState != SERVICE_RUNNING)
				{
					BOOL b = StartService(serviceHandle, NULL, NULL);

					if (b)
						this->m_ServiceLogger->info("Started InITIAM");
					else
						this->m_ServiceLogger->info("Failed to start InITIAM");
				}
				else
				{
					this->m_ServiceLogger->info("InITIAM already started");
				}

				// Close handles
				CloseServiceHandle(serviceHandle);
				CloseServiceHandle(serviceDbHandle);
				*/
			}
			
		}
		// else do nothing
	}
	// If service version differs, 

	this->m_ServiceLogger->info("Completed updates.");
}

std::string InITManagerService::checkin()
{
	// Instantiate variable to hold web request result
	CURLcode checkinres;
	// Instantiate JSON parser which will parse checkinres
	JSONParser checkinjson;
	// Create URL for web request
	std::string guid = this->getClientGuid();
	std::stringstream url;
	url << SERVICE_URL << SERVICE_CHECKIN_URL << guid;
	// Create POST data - Service identification
	std::string pstdata = SERVICE_IDENTIFY_POST;

	// Prepare string stream to receive web page's results
	std::ostringstream checkinreturn;
	// Perform GET request
	checkinres = curlPost(url.str(), checkinreturn, pstdata);
	// Log checkin failure
	if (checkinres != CURLE_OK)
	{
		this->m_ServiceLogger->info("Checkin: curl_easy_perform() failed: " + checkinreturn.str());
	}
	// Log checkin success
	else
	{
		this->m_ServiceLogger->info("Checkin returned: " + checkinreturn.str());
		// If JSON parse fails, log failure
		if (!checkinjson.parse(checkinreturn.str()))
		{
			this->m_ServiceLogger->info("Checkin JSON parsing failed.  Invalid JSON.");
		}
	}

	this->m_ServiceLogger->flush();
	return checkinreturn.str();
}

//
//   FUNCTION: CSampleService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void InITManagerService::ServiceWorkerThread(void)
{
	this->m_ServiceLogger->info("In ServiceWorkerThread...");
	this->m_ServiceLogger->flush();
	std::size_t second = 0;

	update();
	//bool downloaded = false;
	// Periodically check if the service is stopping.
	while (!m_fStopping)
	{
		// Perform main service function here...
		std::chrono::system_clock::time_point today = std::chrono::system_clock::now();	// Get current time
		std::time_t time_now = std::chrono::system_clock::to_time_t(today);				// Convert current time to time_t
		if (time_now <= this->m_timeScheduled)
		{
			::Sleep(1000);
			second += 1;

			if (second == 30)
			{
				this->m_ServiceLogger->info("tick==30");

				// Checkin:
				std::string checkinres = checkin();

				// Update all InIT Services:
				// update();

				// Reset 30 second timer:
				second = 0;
			}
		}
		else if (time_now >= this->m_timeScheduled)
		{
			// Checkin:
			std::string checkin();

			// Update all InIT Services:
			//update();
		}
	}

	// Signal the stopped event.
	SetEvent(m_hStoppedEvent);
}

//
//   FUNCTION: CSampleService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void InITManagerService::OnStop()
{
	// Log a service stop message to the Application log.
	WriteEventLogEntry(L"InITManager in OnStop",
		EVENTLOG_INFORMATION_TYPE);
	this->m_ServiceLogger->info("Stopping service...");

	// Indicate that the service is stopping and wait for the finish of the 
	// main service function (ServiceWorkerThread).
	m_fStopping = TRUE;
	if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	{
		throw GetLastError();
	}
	this->m_ServiceLogger->info("Stopped service.");
}

void InITManagerService::testOnStartandOnStop(int argc, wchar_t *argv[])
{
	std::cout << "OnStart" << std::endl;
	//this->OnStart(argc, argv);
	std::cout << "OnStop" << std::endl;
	//this->OnStop();
}