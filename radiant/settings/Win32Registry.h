#ifndef WIN32REGISTRY_H_
#define WIN32REGISTRY_H_

#include <string>

/** greebo: This provides a convenience method to retrieve a string
 * 			from the Windows Registry using the Windows API.
 * 
 * 			It is safe to call this function in Linux, the non-Win32 implementations
 * 			just return an empty string.  
 */
namespace game {

class Win32Registry
{
public:
	/** greebo: Returns the given key value from HKEY_LOCAL_MACHINE
	 * 
	 * @key: The registry key relative to HKEY_LOCAL_MACHINE, e.g. \SOFTWARE\id\Doom 3
	 * @value: The subkey value, e.g. "InstallPath"
	 * 
	 * This will return "" for non-Win32 systems (IFDEFs in the implementation)
	 */
	static std::string getKeyValue(const std::string& key, const std::string& value);
};

} // namespace game

#endif /*WIN32REGISTRY_H_*/
