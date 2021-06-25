/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      InITInventoryManager
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

#pragma region Includes
#include "definitions.h"
#include "InITIAMService.h"
#include "ThreadPool.h"
#include "computerquery.h"
#include "softwarequery.h"
#include "userquery.h"
#include "SimpleIni.h"
#include "jsonparse.h"
#include <vector>
#include <Wtsapi32.h>
#include <Userenv.h>
#include <stdio.h>
#include <malloc.h>
#include "wrappers.h"

#include <comdef.h>
#include <Wbemidl.h>
#pragma endregion

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1;
#define TOTALBYTES    8192
#define BYTEINCREMENT 4096

InITIAMService::InITIAMService(PWSTR pszServiceName, 
                               BOOL fCanStop, 
                               BOOL fCanShutdown, 
                               BOOL fCanPauseContinue) :
	InITIAMServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue),
	m_ClientGuid(""),
	m_CurrentVersion(""),
	m_POSTAddress("-"),
	m_ServiceLogger(spdlog::basic_logger_mt("InITIAMLogger", SERVICE_LOG_FILEPATH))
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


InITIAMService::~InITIAMService(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

const std::string InITIAMService::getClientGuid() const
{
	return this->m_ClientGuid;
}

const std::wstring InITIAMService::getClientGuidw() const
{
	std::wstring guid(this->m_ClientGuid.begin(), this->m_ClientGuid.end());
	return guid;
}

const std::string InITIAMService::getCurrentVersion() const
{
	return this->m_CurrentVersion;
}

const std::string InITIAMService::getPOSTAddress() const
{
	return this->m_POSTAddress;
}

bool InITIAMService::setClientGuid(std::string guid)
{
	if (1 == 1) // use regex to check validity of guid;  return false if invalid
	{
		this->m_ClientGuid = guid;
		return true;
	}
	else
		return false;
}

bool InITIAMService::setClientGuid(std::wstring wguid)
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

bool InITIAMService::setCurrentVersion(std::string ver)
{
	if (1 == 1) // use regex to check validity of guid;  return false if invalid
	{
		this->m_CurrentVersion = ver;
		return true;
	}
	else
		return false;
}

bool InITIAMService::setPOSTAddress(std::string address)
{
	if (1 == 1) // use regex to check validity of guid;  return false if invalid
	{
		this->m_POSTAddress = address;
		return true;
	}
	else
		return false;
}

int InITIAMService::setSchedulingUtility()  // Temporarily hardcoded to 1:00 AM
{
	std::time_t t = std::time(nullptr);																	// Set time variable to nothing
	std::tm ct = *std::gmtime(&t);																		// Set current time
	ct.tm_min = ct.tm_sec = 0;																			// Set current minute and second to 0
	ct.tm_hour = 01;																					// Set current hour to 01
	ct.tm_mday++;																						// Add 1 day
	t = std::mktime(&ct);																				// Set t to new scheduled time (1:00AM)

	std::chrono::system_clock::time_point tomorrow_at_1 = std::chrono::system_clock::from_time_t(t);	// Set tomorrow_at_1 to t (scheduled time)

	auto until_scheduled = (tomorrow_at_1 - std::chrono::system_clock::now());							// Find how long it is until scheduled time

	std::chrono::duration<int, std::ratio<60 * 60 * 24> > one_day(1);									// Create one day duration
	this->m_timePeriod = one_day;																		// Set m_timePeriod to one_day duration

	std::chrono::system_clock::time_point today = std::chrono::system_clock::now();						// today = now()
	std::chrono::system_clock::time_point tomorrow = today + until_scheduled;							// Set next scheduled time by adding now() and time between now and next schedule

	std::time_t time_now = std::chrono::system_clock::to_time_t(today);									// PRINT NOW()
	std::cout << "Today is: " << std::ctime(&time_now) << std::endl;									//

	this->m_timeScheduled = std::chrono::system_clock::to_time_t(tomorrow);								// PRINT NEXT SCHEDULED TIME
	std::cout << "Tomorrow is: " << std::ctime(&this->m_timeScheduled) << std::endl;					//

	return 0;
}

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
void InITIAMService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"InITIAM Starting in OnStart", EVENTLOG_INFORMATION_TYPE);
	this->m_ServiceLogger->info("***SERVICE_STARTING***");

	// Log version to Application log.
	std::wstring wver = SERVICE_VERSION;
	std::string ver(wver.begin(), wver.end());
	this->m_ServiceLogger->info("Version: " + ver);

	this->setSchedulingUtility();  // Hardcoded to set scheduled update at 1:00 AM

	// Set member variable std::wstring m_ClientGuid from registry
	HKEY hKeyguid;
	std::wstring wcguid;
	std::wstring regpath = SERVICE_DEFAULT_REG;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath.c_str(), NULL, KEY_READ, &hKeyguid);
	GetStringRegKey(hKeyguid, L"clientguid", wcguid, L"null");
	RegCloseKey(hKeyguid);
	this->setClientGuid(wcguid);

	this->m_ServiceLogger->info("Initialized ClientGUID: " + this->m_ClientGuid);
	this->m_ServiceLogger->flush();

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
    CThreadPool::QueueUserWorkItem(&InITIAMService::ServiceWorkerThread, this);
}

//
//   FUNCTION: CSampleService::executeScheduledLogs()
//
//   PURPOSE: The method executes the given log (.vbs). 
//
void InITIAMService::executeScheduledLogs()
{
	this->m_ServiceLogger->info("Executing Logs...");
	std::string guid = this->getClientGuid();
	

	// Create Queries and Curl Post variables
	// Computer Query
	ComputerQuery computerquery;
	std::string computerdata = computerquery.str();
	std::string computervar = "data=" + computerdata;
	std::stringstream computerurl;
	computerurl << SERVICE_URL << SERVICE_COMPUTER_LOG_URL << guid;
	// Software Query
	SoftwareQuery softwarequery;
	std::string softwaredata = softwarequery.str();
	std::string softwarevar = "data=" + softwaredata;
	std::stringstream softwareurl;
	softwareurl << SERVICE_URL << SERVICE_SOFTWARE_LOG_URL << guid;
	// User Query
	UserQuery userquery;
	std::string userdata = userquery.str();
	std::string uservar = "data=" + userdata;
	std::stringstream userurl;
	userurl << SERVICE_URL << SERVICE_USERS_LOG_URL << guid;

	// Write logs to file, then Post to webserver with previously defined variables
	std::string failed = "curl_easy_perform() failed: ";
	//_____________________________ COMPUTER LOG _____________________________//
	std::wofstream computerlog;
	computerlog.open("init\\initiam\\logs\\computerlog.txt");
	computerlog << computerquery.wStr();
	computerlog.close();
	
	// Insert to database
	CURLcode compres;
	std::ostringstream compinsertreturn;
	compres = curlPost(computerurl.str(), compinsertreturn, computervar);
	compinsertreturn << compres;
	if (compres != CURLE_OK)
		this->m_ServiceLogger->info(failed += compinsertreturn.str());
	else
		this->m_ServiceLogger->info("computer query returned: " + compinsertreturn.str());

	//_______________________________ USER LOG _______________________________//
	std::wofstream userlog;
	userlog.open("init\\initiam\\logs\\userlog.txt");
	userlog << userquery.wStr();
	userlog.close();

	// Insert to database
	CURLcode userres;
	std::ostringstream userinsertreturn;
	userres = curlPost(userurl.str(), userinsertreturn, uservar);
	if (compres != CURLE_OK)
		this->m_ServiceLogger->info(failed += userinsertreturn.str());
	else
		this->m_ServiceLogger->info("user query returned: " + userinsertreturn.str());

	//_____________________________ SOFTWARE LOG _____________________________//
	std::wofstream softwarelog;
	softwarelog.open("init\\initiam\\logs\\softwarelog.txt");
	softwarelog << softwarequery.wStr();
	softwarelog.close();

	// Insert to database
	CURLcode softres;
	std::ostringstream softinsertreturn;
	softres = curlPost(softwareurl.str(), softinsertreturn, softwarevar);
	this->m_ServiceLogger->info(softwareurl.str());
	if (softres != CURLE_OK)
		this->m_ServiceLogger->info(failed += softinsertreturn.str());
	else
		this->m_ServiceLogger->info("software query returned: " + softinsertreturn.str());

	this->m_ServiceLogger->flush();
	return;
}
//-----------------------------------------------------------------------------//
//**************************  END executeScheduledLogs()  *********************//
//-----------------------------------------------------------------------------//

std::string InITIAMService::checkin()
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

int InITIAMService::update()
{
	std::string checkinreturn = this->checkin();
	JSONParser checkin(checkinreturn);

	if (this->m_CurrentVersion != checkin.getResult().at("version"))
	{
		this->m_ServiceLogger->info("Not on correct version.  Updating to version " + checkin.getResult().at("version"));
		
		// stop CppWindowsService.
		// download update
		// install update
		// start CppWindowsService
	}
	return 0;
}



//
//   FUNCTION: CSampleService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void InITIAMService::ServiceWorkerThread(void)
{
	size_t second = 0;

	// Execute the logs
	this->executeScheduledLogs();
	WriteEventLogEntry(L"InITService Ran Logs in SchedulingThread",
		EVENTLOG_INFORMATION_TYPE);

    // Periodically check if the service is stopping.
    while (!m_fStopping)
    {
        // Perform main service function here...

		std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(today);
		if (time_now <= this->m_timeScheduled)
		{
			::Sleep(1000);
			second += 1;

			if (second == 300)
			{
				// Checkin:
				std::string checkinres = checkin();

				// Execute the logs
				this->executeScheduledLogs();
				WriteEventLogEntry(L"InITService Ran Logs in SchedulingThread",
					EVENTLOG_INFORMATION_TYPE);

				// Reset 30 second timer:
				second = 0;
			}
		}
		else if (time_now >= this->m_timeScheduled)
		{
			WriteEventLogEntry(L"InITService Running Logs in SchedulingThread...",
				EVENTLOG_INFORMATION_TYPE);

			//Adjust next scheduled time
			std::lock_guard<std::mutex> lock(m_Mutex);
			std::time_t t = std::time(nullptr);
			std::tm ct = *std::gmtime(&t);
			ct.tm_min = ct.tm_sec = 0;
			ct.tm_hour = 01;
			ct.tm_mday++;
			t = std::mktime(&ct);

			std::chrono::system_clock::time_point tomorrow_at_1 = std::chrono::system_clock::from_time_t(t);

			auto until_scheduled = (tomorrow_at_1 - std::chrono::system_clock::now());

			std::chrono::duration<int, std::ratio<60 * 60 * 24> > one_day(1);
			this->m_timePeriod = one_day;

			today = std::chrono::system_clock::now();
			std::chrono::system_clock::time_point tomorrow = today + until_scheduled;

			time_now = std::chrono::system_clock::to_time_t(today);

			this->m_timeScheduled = std::chrono::system_clock::to_time_t(tomorrow);

			//Run Logs
			this->executeScheduledLogs();

			this->checkin();
			
			WriteEventLogEntry(L"InITService Ran Logs in SchedulingThread",
				EVENTLOG_INFORMATION_TYPE);
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
void InITIAMService::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"CppWindowsService in OnStop", 
        EVENTLOG_INFORMATION_TYPE);
	this->m_ServiceLogger->info("Stopping service...");

    // Indicate that the service is stopping and wait for the finish of the 
    // main service function (ServiceWorkerThread).
    m_fStopping = TRUE;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
		this->m_ServiceLogger->info("Throwing error because stop timed out.");
        throw GetLastError();
    }
	//this->checkUpdates();
	this->m_ServiceLogger->info("Stopped service.");
}