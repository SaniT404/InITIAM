/****************************** Module Header ******************************\
* Module Name:  SampleService.h
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

#pragma once

#include "InITIAMServiceBase.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <string>
#include <chrono>
#include <ctime>
#include <ratio>
#include <iomanip>
#include <iostream>
#include <io.h>   // For access().
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <mutex>


class InITIAMService : public InITIAMServiceBase
{
public:

    InITIAMService(PWSTR pszServiceName, 
        BOOL fCanStop = TRUE, 
        BOOL fCanShutdown = TRUE, 
        BOOL fCanPauseContinue = FALSE);
    virtual ~InITIAMService(void);

	const std::string getClientGuid() const;
	const std::wstring getClientGuidw() const;

	const std::string getCurrentVersion() const;
	const std::string getPOSTAddress() const;

	

	std::string checkin();

	

protected:
	void executeScheduledLogs();

	bool setClientGuid(std::string guid);
	bool setClientGuid(std::wstring wguid);

	bool setCurrentVersion(std::string ver);
	bool setPOSTAddress(std::string address);

	//bool checkRegistryValues();
	//void generateRegistry();

	int setSchedulingUtility();  // Temporarily hardcoded to 1:00 AM


    virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);
    virtual void OnStop();

    void ServiceWorkerThread(void);

	int update();

private:
	mutable std::mutex m_Mutex;  //Synchronizes access to member data.

	std::time_t m_timeScheduled;
	std::chrono::duration<int, std::ratio<60 * 60 * 24> > m_timePeriod;

    BOOL m_fStopping;
    HANDLE m_hStoppedEvent;

	std::string m_ClientGuid;
	std::string m_CurrentVersion;
	std::string m_POSTAddress;

	std::shared_ptr<spdlog::logger> m_ServiceLogger;
};