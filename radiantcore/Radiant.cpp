#include "Radiant.h"

#include <iomanip>
#include "version.h"

#include "string/convert.h"
#include "module/CoreModule.h"

#include "log/LogStream.h"
#include "log/LogWriter.h"
#include "log/LogFile.h"
#include "log/SegFaultHandler.h"
#include "module/StaticModule.h"
#include "messagebus/MessageBus.h"
#include "settings/LanguageManager.h"

#include "xmlutil/XmlModule.h"

namespace radiant
{

namespace
{
	const char* const TIME_FMT = "%Y-%m-%d %H:%M:%S";
}

Radiant::Radiant(IApplicationContext& context) :
	_context(context),
	_messageBus(new MessageBus)
{
    xmlutil::initModule();

	// Set the stream references for rMessage(), redirect std::cout, etc.
	applog::LogStream::InitialiseStreams(getLogWriter());

    // Initialise the GlobalErorrHandler() function object, which is used by ASSERT_MESSAGE
    // This is usually a function owned by the UI module to show a popup
    GlobalErrorHandler() = _context.getErrorHandlingFunction();

	// Attach the logfile to the logwriter
	createLogFile();

#ifdef POSIX
    applog::SegFaultHandler::Install();
#endif

	_moduleRegistry.reset(new module::ModuleRegistry(_context));

	_languageManager.reset(new language::LanguageManager);
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

    // Shutdown the libxml2 DLL
    xmlutil::shutdownModule();
}

applog::ILogWriter& Radiant::getLogWriter()
{
	return applog::LogWriter::Instance();
}

module::ModuleRegistry& Radiant::getModuleRegistry()
{
	return *_moduleRegistry;
}

radiant::IMessageBus& Radiant::getMessageBus()
{
	return *_messageBus;
}

language::ILanguageManager& Radiant::getLanguageManager()
{
	return *_languageManager;
}

void Radiant::startup()
{
	try
	{
		// Register the modules hosted in this binary
		module::internal::StaticModuleList::RegisterModules();

		module::GlobalModuleRegistry().loadAndInitialiseModules();
	}
	catch (const std::exception& e)
	{
		rError() << "Exception initialising modules: " << e.what() << std::endl;
		throw StartupFailure(e.what()); // translate the exception
	}
}

void Radiant::createLogFile()
{
	_logFile.reset(new applog::LogFile(_context.getCacheDataPath() + "darkradiant.log"));

	if (_logFile->isOpen())
	{
		getLogWriter().attach(_logFile.get());

		rMessage() << "Started logging to " << _logFile->getFullPath() << std::endl;

		rMessage() << "This is " << RADIANT_APPNAME_FULL() << std::endl;

		std::time_t t = std::time(nullptr);
		std::tm tm = *std::localtime(&t);

		// Write timestamp and thread information
		rMessage() << "Today is " << std::put_time(&tm, TIME_FMT) << std::endl;
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

void Radiant::initialiseModule(const IApplicationContext& ctx)
{}

std::shared_ptr<Radiant>& Radiant::InstancePtr()
{
	static std::shared_ptr<Radiant> _instancePtr;
	return _instancePtr;
}

}

extern "C" DARKRADIANT_DLLEXPORT radiant::IRadiant* SYMBOL_CREATE_RADIANT(IApplicationContext& context)
{
	auto& instancePtr = radiant::Radiant::InstancePtr();

	// Create a new instance, but ensure that this has only been called once
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
