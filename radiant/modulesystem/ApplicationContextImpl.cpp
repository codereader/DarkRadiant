#include "ApplicationContextImpl.h"

#include "stream/textstream.h"
#include "string/string.h"
#include "debugging/debugging.h"
#include "itextstream.h"
#include "iregistry.h"
#include "os/path.h"
#include "os/dir.h"

#include <boost/algorithm/string/predicate.hpp>

#if defined(WIN32)
#include <windows.h>
#endif

namespace module {

/**
 * Return the application path of the current Radiant instance.
 */
const std::string& ApplicationContextImpl::getApplicationPath() const {
	return _appPath;
}

/**
 * Return the settings path of the current Radiant instance.
 */
const std::string& ApplicationContextImpl::getSettingsPath() const {
	return _settingsPath;
}

const std::string& ApplicationContextImpl::getBitmapsPath() const {
	return _bitmapsPath;
}

std::ostream& ApplicationContextImpl::getOutputStream() const {
	return globalOutputStream();
}

std::ostream& ApplicationContextImpl::getWarningStream() const {
	return globalWarningStream();
}

std::ostream& ApplicationContextImpl::getErrorStream() const {
	return globalErrorStream();
}

// ============== OS-Specific Implementations go here ===================

// ================ POSIX ====================
#if defined(POSIX)

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h> 

#include <glib/gutils.h>

const char* LINK_NAME =
#if defined (__linux__)
  "/proc/self/exe"
#else // FreeBSD and OSX
  "/proc/curproc/file"
#endif
;

/// brief Returns the filename of the executable belonging to the current process, or 0 if not found.
char* getexename(char *buf, char* argv[]) {
	/* Now read the symbolic link */
	int ret = readlink(LINK_NAME, buf, PATH_MAX);

	if (ret == -1) {
		globalOutputStream() << "getexename: falling back to argv[0]: '" << argv[0] << "'";
		const char* path = realpath(argv[0], buf);
		if (path == NULL) {
			/* In case of an error, leave the handling up to the caller */
			return "";
		}
	}

	/* Ensure proper NUL termination */
	buf[ret] = 0;

	/* delete the program name */
	*(strrchr(buf, '/')) = '\0';

	// NOTE: we build app path with a trailing '/'
	// it's a general convention in Radiant to have the slash at the end of directories
	if (buf[strlen(buf)-1] != '/') {
		strcat(buf, "/");
	}

	return buf;
}

void ApplicationContextImpl::initialise(int argc, char* argv[]) {
	// Give away unnecessary root privileges.
	// Important: must be done before calling gtk_init().
	char *loginname;
	struct passwd *pw;
	seteuid(getuid());
	
	if (geteuid() == 0 && 
		(loginname = getlogin()) != 0 && 
		(pw = getpwnam(loginname)) != 0)
	{
		setuid(pw->pw_uid);
	}

	initArgs(argc, argv);

    // Initialise the home directory path
    std::string home = os::standardPathWithSlash(g_get_home_dir()) + ".darkradiant/";
    os::makeDirectory(home);
    _homePath = home;

	{
		char real[PATH_MAX];
		_appPath = getexename(real, argv);
		ASSERT_MESSAGE(!_appPath.empty(), "failed to deduce app path");
	}
	
	// Initialise the relative paths
	initPaths();
}

// ================ WIN32 ====================
#elif defined(WIN32)

void ApplicationContextImpl::initialise(int argc, char* argv[]) {
	
	initArgs(argc, argv);
	{
		const char* appData = getenv("APPDATA");

		if (appData == NULL) {
			throw std::runtime_error("Critical: cannot find APPDATA environment variable.\n");
		}

		std::string home = appData;
		home += "\\DarkRadiant\\";
		os::makeDirectory(home);
		_homePath = home;
	}
	{
		// get path to the editor
		char filename[MAX_PATH+1];
		GetModuleFileName(0, filename, MAX_PATH);
		char* last_separator = strrchr(filename, '\\');
		if (last_separator != 0) {
			*(last_separator+1) = '\0';
		}
		else {
			filename[0] = '\0';
		}
		
		// Make sure we have forward slashes
		_appPath = os::standardPath(filename);
	}
	// Initialise the relative paths
	initPaths();
}

#else
#error "unsupported platform"
#endif

// ============== OS-Specific Implementations end ===================

void ApplicationContextImpl::initArgs(int argc, char* argv[]) {
	int i, j, k;
	
	for (i = 1; i < argc; i++) {
		for (k = i; k < argc; k++)
			if (argv[k] != 0)
				break;
	
		if (k > i) {
			k -= i;
			for (j = i + k; j < argc; j++)
				argv[j-k] = argv[j];
			argc -= k;
		}
	}
}

void ApplicationContextImpl::initPaths() {
	// Ensure that the homepath ends with a slash
	if (!boost::algorithm::ends_with(_homePath, "/")) {
		_homePath += "/";
	}
	
	// Make sure the home folder exists (attempt to create it)
	os::makeDirectory(_homePath);

	_settingsPath = _homePath;
	os::makeDirectory(_settingsPath);

#if defined(POSIX) && defined(PKGDATADIR)
    _bitmapsPath = os::standardPathWithSlash(PKGDATADIR) + "bitmaps/";
#else
	_bitmapsPath = _appPath + "bitmaps/";
#endif
}

void ApplicationContextImpl::savePathsToRegistry() const {
	GlobalRegistry().set(RKEY_APP_PATH, _appPath);
	GlobalRegistry().set(RKEY_HOME_PATH, _homePath);
	GlobalRegistry().set(RKEY_SETTINGS_PATH, _settingsPath);
	GlobalRegistry().set(RKEY_BITMAPS_PATH, _bitmapsPath);
}

} // namespace module
