#include "Radiant.h"

#include <iomanip>
#include "version.h"

#include "string/convert.h"
#include "module/CoreModule.h"

#include "log/LogStream.h"
#include "log/LogWriter.h"
#include "log/LogFile.h"
#include "modulesystem/ModuleRegistry.h"

#ifndef POSIX
#include "settings/LanguageManager.h"
#endif

#include <wx/version.h>

namespace radiant
{

namespace
{
	const char* const TIME_FMT = "%Y-%m-%d %H:%M:%S";
}

Radiant::Radiant(ApplicationContext& context) :
	_context(context)
{
	// Set the stream references for rMessage(), redirect std::cout, etc.
	applog::LogStream::InitialiseStreams(getLogWriter());

	// Attach the logfile to the logwriter
	createLogFile();

	_moduleRegistry.reset(new module::ModuleRegistry);
	_moduleRegistry->setContext(_context);

#ifndef POSIX
	// Initialise the language based on the settings in the user settings folder
	language::LanguageManager().init(_context);
#endif
}

Radiant::~Radiant()
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

applog::ILogWriter& Radiant::getLogWriter()
{
	return applog::LogWriter::Instance();
}

module::ModuleRegistry& Radiant::getModuleRegistry()
{
	return *_moduleRegistry;
}

void Radiant::createLogFile()
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

const std::string& Radiant::getName() const
{
	static std::string _name(MODULE_RADIANT_CORE);
	return _name;
}

const StringSet& Radiant::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void Radiant::initialiseModule(const ApplicationContext& ctx)
{}

std::shared_ptr<Radiant>& Radiant::InstancePtr()
{
	static std::shared_ptr<Radiant> _instancePtr;
	return _instancePtr;
}

}

extern "C" DARKRADIANT_DLLEXPORT radiant::IRadiant* SYMBOL_CREATE_RADIANT(ApplicationContext& context)
{
	auto& instancePtr = radiant::Radiant::InstancePtr();

	// Create a new instance, but ensure that this has only be called once
	assert(!instancePtr);

	instancePtr.reset(new radiant::Radiant(context));

	// Add this module to the registry it's holding
	instancePtr->getModuleRegistry().registerModule(instancePtr);
	instancePtr->getModuleRegistry().initialiseCoreModule();

	return instancePtr.get();
}

extern "C" DARKRADIANT_DLLEXPORT void SYMBOL_DESTROY_RADIANT(radiant::IRadiant* radiant)
{
	assert(radiant::Radiant::InstancePtr().get() == radiant);

	radiant::Radiant::InstancePtr().reset();
}
