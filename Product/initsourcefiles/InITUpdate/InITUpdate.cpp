// InITUpdate.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <curl/curl.h>
#include "definitions.h"
#include "wrappers.h"





int wmain(int argc, wchar_t* argv[])
{
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"update", argv[1] + 1) == 0)
		{
			update();
		}
		else if (_wcsicmp(L"install", argv[1] + 1) == 0)
		{
			create_task(L"InIT Update");
			//create_task(L"test_taskbruh2");
			//create_task_trigger(L"test_taskbruh2", TASK_EVENT_TRIGGER_AT_LOGON);
		}
		else if (_wcsicmp(L"cleanup", argv[1] + 1) == 0)
		{
			std::cout << "Cleaning..." << std::endl;
		}
		else if (_wcsicmp(L"start", argv[1] + 1) == 0)
		{
			std::cout << "Starting..." << std::endl;
		}
		else if (_wcsicmp(L"stop", argv[1] + 1) == 0)
		{
			std::cout << "Stopping..." << std::endl;
		}
	}
	else
	{
		wprintf(L"Parameters:\n");
		wprintf(L" -install  to install the service.\n");
		wprintf(L" -start  to start the service.\n");
		wprintf(L" -stop  to stop the service.\n");
		wprintf(L" -uninstall   to uninstall the service.\n");
	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
