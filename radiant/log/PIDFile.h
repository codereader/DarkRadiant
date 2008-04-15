#ifndef _PID_FILE_H_
#define _PID_FILE_H_

#include "gtkutil/messagebox.h"
#include "settings/PreferenceSystem.h"
#include "modulesystem/ModuleRegistry.h"

#define PID_FILENAME "darkradiant.pid"

namespace applog {

/**
 * greebo: This is a scoped object which creates a .pid file on construction
 *         and removes it on destruction. On program crash, the file doesn't
 *         get erased, which is detected during the next application startup.
 */
class PIDFile
{
	std::string _filename;

public:
	PIDFile(const std::string& filename) {
		module::ModuleRegistry& registry = module::ModuleRegistry::Instance();
		_filename = registry.getApplicationContext().getSettingsPath() + filename;

		FILE* pid = fopen(_filename.c_str(), "r");
		
		// Check for an existing radiant.pid file
		if (pid != NULL) {
			fclose(pid);

			if (remove(_filename.c_str()) == -1) {
				std::string msg = "WARNING: Could not delete " + _filename;
				gtk_MessageBox(NULL, msg.c_str(), "DarkRadiant", eMB_OK, eMB_ICONERROR);
			}

			std::string msg("Radiant failed to start properly the last time it was run.\n");
			msg += "The failure may be related to invalid preference settings.\n";
			msg += "Do you want to rename your local user.xml file and restore the default settings?";

			if (gtk_MessageBox(0, msg.c_str(), "Radiant - Startup Failure", 
				   eMB_YESNO, eMB_ICONQUESTION) == eIDYES) 
			{
				resetPreferences();
			}
		}

		// create a primary .pid for global init run
		pid = fopen(_filename.c_str(), "w");
		if (pid) {
			fclose(pid);
		}
	};

	~PIDFile() {
		if (remove(_filename.c_str()) == -1) {
			std::string msg = "WARNING: Could not delete " + _filename;
			gtk_MessageBox(NULL, msg.c_str(), "DarkRadiant", eMB_OK, eMB_ICONERROR );
		}
	}
};

} // namespace applog

#endif /* _PID_FILE_H_ */
