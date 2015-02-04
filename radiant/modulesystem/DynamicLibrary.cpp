#include "DynamicLibrary.h"

#include "itextstream.h"

namespace module {

/**
 * =============================== WIN32 ======================================
 */
#if defined(WIN32)

#define FORMAT_BUFSIZE 2048

// Helper method to retrieve the error when DLL load failed.
const wchar_t* FormatGetLastError() {
	static wchar_t buf[FORMAT_BUFSIZE];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			buf,
			FORMAT_BUFSIZE, NULL);
	return buf;
}

DynamicLibrary::DynamicLibrary(const std::string& filename) :
	_name(filename.begin(), filename.end()),
	_library(LoadLibrary(_name.c_str()))
{
	if (_library == 0)
    {
        rConsoleError() << "LoadLibrary failed: '" << filename << "'" << std::endl;
        rConsoleError() << "GetLastError: " << FormatGetLastError();
	}
}

DynamicLibrary::~DynamicLibrary() {
	if (!failed()) {
		FreeLibrary(_library);
	}
}

bool DynamicLibrary::failed() {
	return _library == 0;
}

DynamicLibrary::FunctionPointer DynamicLibrary::findSymbol(const std::string& symbol) {
	// Try to lookup the symbol address
	FunctionPointer address = GetProcAddress(_library, symbol.c_str());

	// Emit a warning if the lookup failed
	if (address == 0) {
		rConsoleError() << "GetProcAddress failed: '" << symbol << "'" << std::endl;
		rConsoleError() << "GetLastError: " << FormatGetLastError();
	}

	return address;
}

std::string DynamicLibrary::getName() const {
	std::string out(_name.begin(), _name.end());
	return out;
}

/**
 * =============================== POSIX ======================================
 */
#elif defined(POSIX)

DynamicLibrary::DynamicLibrary(const std::string& filename) :
	_name(filename),
	_dlHandle(dlopen(_name.c_str(), RTLD_NOW)) // load the file using the OS function dlopen
{}

DynamicLibrary::~DynamicLibrary() {
	if (!failed()) {
		dlclose(_dlHandle);
	}
}

bool DynamicLibrary::failed() {
	return _dlHandle == 0;
}

// Find a symbol in the library
DynamicLibrary::FunctionPointer DynamicLibrary::findSymbol(const std::string& symbol) {
	// Try to obtain the function pointer and cast it to a void* pointer
	FunctionPointer p = reinterpret_cast<FunctionPointer>(
		dlsym(_dlHandle, symbol.c_str())
	);

	if (p == 0) {
		const char* error = dlerror();
		if (error != NULL) {
            rConsoleError() << error << std::endl;
		}
	}

	return p;
}

std::string DynamicLibrary::getName() const {
	return _name;
}

#else
#error "unsupported platform"
#endif

}
