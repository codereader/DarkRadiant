#include "iradiant.h"

#include <iomanip>
#include "version.h"

#include "string/convert.h"
#include "module/CoreModule.h"

#include "log/LogStream.h"
#include "log/LogWriter.h"
#include "log/LogFile.h"
#include "modulesystem/ModuleRegistry.h"

#include <wx/version.h>

namespace radiant
{

namespace
{
	const char* const TIME_FMT = "%Y-%m-%d %H:%M:%S";
}

class Radiant :
	public IRadiant
{
private:
	ApplicationContext& _context;

	std::unique_ptr<applog::LogFile> _logFile;

	std::unique_ptr<module::ModuleRegistry> _moduleRegistry;

public:
	Radiant(ApplicationContext& context) :
		_context(context)
	{
		// Set the stream references for rMessage(), redirect std::cout, etc.
		applog::LogStream::InitialiseStreams(getLogWriter());

		// Attach the logfile to the logwriter
		createLogFile();

		_moduleRegistry.reset(new module::ModuleRegistry);
		_moduleRegistry->setContext(_context);
	}

	~Radiant()
	{
		_moduleRegistry.reset();

		// Close the log file
		if (_logFile)
		{
			_logFile->close();
			getLogWriter().detach(_logFile.get());
			_logFile.reset();
		}

		applog::LogStream::ShutdownStreams();
	}

	applog::ILogWriter& getLogWriter() override
	{
		return applog::LogWriter::Instance();
	}

	IModuleRegistry& getModuleRegistry() override
	{
		return *_moduleRegistry;
	}

private:
	void createLogFile()
	{
		_logFile.reset(new applog::LogFile(_context.getSettingsPath() + "darkradiant.log"));

		if (_logFile->isOpen())
		{
			getLogWriter().attach(_logFile.get());

			rMessage() << "Started logging to " << _logFile->getFullPath() << std::endl;

			rMessage() << "This is " << RADIANT_APPNAME_FULL() << std::endl;

			std::time_t t = std::time(nullptr);
			std::tm tm = *std::localtime(&t);

			// Write timestamp and thread information
			rMessage() << "Today is " << std::put_time(&tm, TIME_FMT) << std::endl;

			// Output the wxWidgets version to the logfile
			std::string wxVersion = string::to_string(wxMAJOR_VERSION) + ".";
			wxVersion += string::to_string(wxMINOR_VERSION) + ".";
			wxVersion += string::to_string(wxRELEASE_NUMBER);

			rMessage() << "wxWidgets Version: " << wxVersion << std::endl;
		}
		else
		{
			rConsoleError() << "Failed to create log file '"
				<< _logFile->getFullPath() << ", check write permissions in parent directory."
				<< std::endl;
		}
	}
};

}

extern "C" DARKRADIANT_DLLEXPORT radiant::IRadiant* SYMBOL_CREATE_RADIANT(ApplicationContext& context)
{
	return new radiant::Radiant(context);
}

extern "C" DARKRADIANT_DLLEXPORT void SYMBOL_DESTROY_RADIANT(radiant::IRadiant* radiant)
{
	delete radiant;
}
