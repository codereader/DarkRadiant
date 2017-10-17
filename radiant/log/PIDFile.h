#pragma once

#include "wxutil/dialog/MessageBox.h"
#include "settings/PreferenceSystem.h"
#include "modulesystem/ModuleRegistry.h"

#include "os/file.h"
#include "string/replace.h"

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
	PIDFile(const std::string& filename)
	{
		module::ModuleRegistry& registry = module::ModuleRegistry::Instance();
		_filename = registry.getApplicationContext().getSettingsPath() + filename;

		FILE* pid = fopen(_filename.c_str(), "r");

		// Check for an existing radiant.pid file
		if (pid != NULL)
		{
			fclose(pid);

			removePIDFile();

#ifndef _DEBUG
			fs::path path = registry.getApplicationContext().getSettingsPath();
			path /= "darkradiant.log";
			std::string logPath = path.string();
			string::replace_all(logPath, "\\\\", "\\");
			string::replace_all(logPath, "//", "/");

			std::string msg("Radiant failed to start properly the last time it was run.\n");
			msg += "If this is happening again, you might want to check the log file in\n";
			msg += logPath;

			wxutil::Messagebox box("DarkRadiant - Startup Failure", msg, ui::IDialog::MESSAGE_CONFIRM);
			box.run();
#endif
		}

		// create a primary .pid for global init run
		pid = fopen(_filename.c_str(), "w");

		if (pid)
		{
			fclose(pid);
		}
	};

	~PIDFile()
	{
		removePIDFile();
	}

private:
	void removePIDFile()
	{
		if (remove(_filename.c_str()) == -1)
		{
			wxutil::Messagebox::ShowError("WARNING: Could not delete " + _filename);
		}
	}
};

} // namespace applog
