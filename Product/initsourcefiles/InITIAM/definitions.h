#pragma once

// 
// Settings of the service
// 

// Internal name of the service
#define SERVICE_NAME						L"InITIAM"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME				L"InITIAM"

// Service start options.
#define SERVICE_START_TYPE					SERVICE_DEMAND_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES				L""

// The name of the account under which the service should run L"NT AUTHORITY\\LocalService"
#define SERVICE_ACCOUNT						L"LocalSystem"

// The password to the service account name
#define SERVICE_PASSWORD					NULL

// The version of the service
#define SERVICE_VERSION						L"1.0"

//
// Network Settings
//

// Server URL
#define SERVICE_URL							"https://initiam.net/srv/"

// Register URL
#define SERVICE_REGISTER_URL				"register/"

// Checkin URL
#define SERVICE_CHECKIN_URL					"checkin/"

// Computer Log URL
#define SERVICE_COMPUTER_LOG_URL			"log/computers/"

// Software Log URL
#define SERVICE_SOFTWARE_LOG_URL			"log/software/"

// Software Log URL
#define SERVICE_USERS_LOG_URL				"log/users/"

//
// HTTPS Settings
//

// InIT public key path
#define SERVICE_PUBLIC_KEY		""

// Service POST
#define SERVICE_IDENTIFY_POST	"service=InITIAM"

//
// File System Settings
//

// Default path
#define SERVICE_DEFAULT_PATH				L"C:\\Windows\\System32\\init\\"

// Home path
#define SERVICE_HOME_PATH					L"C:\\Windows\\System32\\init\\initiam\\"

// Log path
#define SERVICE_LOG_PATH					L"C:\\Windows\\System32\\init\\initiam\\logs\\"

// Log filepath
#define SERVICE_LOG_FILEPATH				"C:\\Windows\\System32\\init\\initiam\\logs\\iam.txt"

//
// Registry Settings
//

// Default path
#define SERVICE_DEFAULT_REG					L"SOFTWARE\\InIT\\"

// Home path
#define SERVICE_HOME_REG					L"SOFTWARE\\InIT\\InITIAM"

