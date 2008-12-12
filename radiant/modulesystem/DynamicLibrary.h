#ifndef DYNAMICLIBRARY_H_
#define DYNAMICLIBRARY_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

/** greebo: This file declares the classes encapsulating a dynamically linked library.
 *          Each DL class must define a FunctionPointer typedef and a findSymbol() method. 
 * 
 *  		The declaration is platform-specific, currently Win32 and POSIX are declared. 
 */

/** 
 * =============================== WIN32 ======================================
 */
#if defined(WIN32)

#include <windows.h>

namespace module {

/** greebo: WIN32 DynamicLibrary. Loads a DLL given in the constructor.
 */
class DynamicLibrary {
	// The full filename of this library
	std::string _name;
	
	// The library handle
	HMODULE _library;
	
public:
	// The win32 function pointer typedef for calling symbols
	typedef FARPROC FunctionPointer;

	// Constructor, pass the full filename to load this DLL 
	DynamicLibrary(const std::string& filename);
	
	~DynamicLibrary();
	
	// Returns TRUE if the library load failed. (TODO: greebo: Remove this in favour of exceptions?)
	bool failed();
	
	/** greebo: The lookup method for symbols. Returns a platform-specific function 
	 *          pointer that can be used to call the symbol.          
	 */
	FunctionPointer findSymbol(const std::string& symbol);
	
	// Returns the filename of this module
	std::string getName() const;
};

} // namespace module

/** 
 * =============================== POSIX ======================================
 */
#elif defined(POSIX)

#include <dlfcn.h>

namespace module {

class DynamicLibrary {
	// The full filename of this library
	std::string _name;
		
	// The handle for accessing the dynamic library
	void* _dlHandle;
	
public:
	// The posix function pointer typedef for calling symbols
	typedef int (* FunctionPointer)();

	// Constructor, pass the full filename to load this DLL
	DynamicLibrary(const std::string& filename);
	
	~DynamicLibrary();
	
	// Returns TRUE if the library load failed. (TODO: greebo: Remove this in favour of exceptions?)
	bool failed();

	/** greebo: The lookup method for symbols. Returns a platform-specific function 
	 *          pointer that can be used to call the symbol.          
	 */
	FunctionPointer findSymbol(const std::string& symbol);
	
	// Returns the filename of this module
	std::string getName() const;
};

} // namespace module

#else
#error "unsupported platform"
#endif

namespace module {
	// Shared ptr typedef
	typedef boost::shared_ptr<DynamicLibrary> DynamicLibraryPtr;
	
	// A list of allocated libraries
	typedef std::vector<DynamicLibraryPtr> DynamicLibraryList;
}

#endif /*DYNAMICLIBRARY_H_*/
