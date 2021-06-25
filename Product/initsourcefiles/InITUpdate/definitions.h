#pragma once

#define dir_delimter '/'
#define MAX_FILENAME 512
#define READ_SIZE 8192

// 
// Settings of the service
// 

// Task Scheduler Task Name
#define TASK_NAME				L"InITUpdate"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""

// The name of the account under which the service should run L"NT AUTHORITY\\LocalService"
#define SERVICE_ACCOUNT          L"LocalSystem"

// The password to the service account name
#define SERVICE_PASSWORD         NULL

// The version of the service
#define InIT_UPDATE_VERSION			L"0.1"

//
// Network Settings
//

// Server URL
#define SERVICE_URL				"https://initiam.net/srv/"

// Update URL
#define SERVICE_UPDATE_URL		"update/"

//
// HTTPS Settings
//

// InIT public key path
#define SERVICE_PUBLIC_KEY		""

//
// File System Settings
//

// Default path
#define SERVICE_DEFAULT_PATH	L"C:\\Program Files\\InIT\\InITUpdate\\"

// Log filepath
#define SERVICE_LOG_FILEPATH	"C:\\Program Files\\InIT\\InITUpdate\\update.txt"

//
// Registry Settings
//

// Default path
#define SERVICE_DEFAULT_REG	L"SOFTWARE\\InIT\\"