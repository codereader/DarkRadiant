#include "Win32Registry.h"

#include <iostream>

#if defined(WIN32)
#include "windows.h"
#endif

namespace game {

#if defined(WIN32)

// Win32 implementation
std::string Win32Registry::getKeyValue(const std::string& key, const std::string& value) {
	HKEY hkey;

	unsigned long retVal = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		key.c_str(),
		0,
		KEY_QUERY_VALUE,
		&hkey
	);

	if (retVal != ERROR_SUCCESS) {
		std::cout << "Win32Registry: Could not open registry key: " << key << "\n";
		return "";
	}

	DWORD type = REG_SZ;	// Query a string
	BYTE buffer[1024];		// The target buffer
	DWORD bufferSize = sizeof(buffer);

	retVal = RegQueryValueEx(hkey, 				 // the previously opened HKEY 
							 value.c_str(),		 // The value
							 NULL, 				 // Reserved, must be NULL
							 &type, 			 // type = SZ
							 (LPBYTE)&buffer, 	 // The target buffer pointer
							 (LPDWORD)&bufferSize // pointer to the buffer size
	);
	// The bufferSize variable now contains the length of the returned string

	// Close the key regularly
	RegCloseKey(hkey);

	if (retVal != ERROR_SUCCESS) {
		std::cout << "Win32Registry: Could not query value: " << value << "\n";
		return "";
	}
	
	// NULL-Terminate the returned string and return the value
	if (bufferSize < sizeof(buffer)) {
		buffer[bufferSize] = '\0';
		std::string result = (char*)&buffer;
		return result;
	}
	
	// Boundary check not passed, return an empty string
	return "";
}

#else

// non-Win32 OS, default implementation returns an empty string
std::string Win32Registry::getKeyValue(const std::string& key, const std::string& value) {
	return "";
}

#endif

} // namespace game
