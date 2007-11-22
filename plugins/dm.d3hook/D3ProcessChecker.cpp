#include "D3ProcessChecker.h"

#ifdef WIN32

#include <string>
#include <windows.h>
#include "Psapi.h"

bool D3ProcessChecker::D3IsRunning() {
	DWORD processes[1024];
	DWORD num;

	if (!EnumProcesses(processes, sizeof(processes), &num)) {
		return false;
	}

	// Iterate over the processes
	for (int i = 0; i < int(num/sizeof(DWORD)); i++) {
		char szProcessName[MAX_PATH] = "unknown";

		// Get the handle for this process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i]);

		if (hProcess) {
			HMODULE hMod;
			DWORD countBytes;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &countBytes)) {
				
				GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName));

				std::string processName(szProcessName);

				if (processName == "DOOM3.exe") {
					//globalOutputStream() << "Doom3 found!\n";

					HMODULE hModules[1024];
					DWORD cbNeeded;

					if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
						for (unsigned int m = 0; m < (cbNeeded / sizeof(HMODULE)); m++)	{
							TCHAR szModName[MAX_PATH];

							// Get the full path to the module's file.
							if (GetModuleBaseName(hProcess, hModules[m], szModName, sizeof(szModName)/sizeof(TCHAR))) {
								// Print the module name and handle value.
								if (std::string(szModName) == "gamex86.dll") {
									//globalOutputStream() << "Module found: " << szModName << "\n";

									CloseHandle(hProcess); // close the handle, we're terminating
									return true;
								}
							}
						}
					}
				}
			}
		}

		CloseHandle(hProcess);
	}

	return false;
}

#else

// Linux implementation (TODO)

bool D3ProcessChecker::D3IsRunning() {
	return true;
}

#endif

