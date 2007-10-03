#ifndef DYNAMICLIBRARYLOADER_H_
#define DYNAMICLIBRARYLOADER_H_

#include "DynamicLibrary.h"

namespace module {

/**
 * greebo: This is a small helper class that takes a DynamicLibrary,
 *         tries to lookup the entry point symbol and registers it with
 *         the ModuleRegistry on success.
 * 
 *         Pass the library candidate to the constructor.
 */
class DynamicLibraryLoader {
public:
	/** greebo: The constructor takes the candidate dynamic library and a list
	 *          where the library pointer is added to on success. 
	 */
	DynamicLibraryLoader(DynamicLibraryPtr library, DynamicLibraryList& targetList);
};

} // namespace module

#endif /*DYNAMICLIBRARYLOADER_H_*/
