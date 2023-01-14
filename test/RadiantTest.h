#pragma once

#include "gtest/gtest.h"
#include <sigc++/functors/mem_fun.h>
#include <iostream>

#include "imessagebus.h"
#include "iradiant.h"
#include "imap.h"
#include "icommandsystem.h"

#include "TestContext.h"
#include "TestLogFile.h"
#include "HeadlessOpenGLContext.h"
#include "module/CoreModule.h"
#include "messages/GameConfigNeededMessage.h"
#include "messages/NotificationMessage.h"
#include "FakeClipboardModule.h"

namespace test
{

/**
 * Test fixture setting up the application context and
 * the radiant core module.
 */
class RadiantTest :
	public ::testing::Test
{
protected:
	// The RadiantApp owns the ApplicationContext which is then passed to the
	// ModuleRegistry as a reference.
	radiant::TestContext _context;

	std::unique_ptr<module::CoreModule> _coreModule;

	std::size_t _gameSetupListener;
	std::size_t _notificationListener;

	std::shared_ptr<gl::HeadlessOpenGLContextModule> _glContextModule;
	std::shared_ptr<FakeClipboardModule> _fakeClipboard;

    std::unique_ptr<TestLogFile> _testLogFile;

protected:
	RadiantTest()
	{
		// Initialise the context, disable the sound player
		std::string exePath("ignored");
		std::string arg("--disable-sound");
		char* args[] = { exePath.data(), arg.data() };
		_context.initialise(2, args);

		try
		{
			_coreModule.reset(new module::CoreModule(_context));

			auto* radiant = _coreModule->get();

			module::RegistryReference::Instance().setRegistry(radiant->getModuleRegistry());
			module::initialiseStreams(radiant->getLogWriter());

            initTestLog();
		}
		catch (module::CoreModule::FailureException & ex)
		{
			// Streams are not yet initialised, so log to std::err at this point
            std::cerr << "RadiantTest initialisation failed:\n\t";
			std::cerr << ex.what() << std::endl;

            // Re-throw the exception and abort the test. There's no point in
            // continuing if we couldn't construct the core module.
            throw;
		}
	}

    /// Override this to perform custom actions before the core module starts
    virtual void preStartup() {}

	void SetUp() override
	{
        // Invoke any custom actions needed by subclasses
        preStartup();

		// Set up the test game environment
		setupGameFolder();
        setupTestModules();

		// Wire up the game-config-needed handler, we need to respond
		_gameSetupListener = _coreModule->get()->getMessageBus().addListener(
			radiant::IMessage::Type::GameConfigNeeded,
			radiant::TypeListener<game::ConfigurationNeeded>(
				sigc::mem_fun(this, &RadiantTest::handleGameConfigMessage)));

		_notificationListener = _coreModule->get()->getMessageBus().addListener(
			radiant::IMessage::Type::Notification,
			radiant::TypeListener<radiant::NotificationMessage>(
				sigc::mem_fun(this, &RadiantTest::handleNotification)));

		try
		{
			// Startup the application
			_coreModule->get()->startup();
		}
		catch (const radiant::IRadiant::StartupFailure & ex)
		{
			// An unhandled exception during module initialisation => display a popup and exit
			rError() << "Unhandled Exception: " << ex.what() << std::endl;
			abort();
		}

		_glContextModule->createContext();

        GlobalMapModule().createNewMap();
	}

    /// Override this to perform custom actions before the main module shuts down
    virtual void preShutdown() {}

	void TearDown() override
	{
        // Invoke any pre-shutdown custom code
        preShutdown();

		_coreModule->get()->getMessageBus().removeListener(_notificationListener);
		_coreModule->get()->getMessageBus().removeListener(_gameSetupListener);

		// Issue a shutdown() call to all the modules
		module::GlobalModuleRegistry().shutdownModules();

        // Invoke any post-shutdown custom code
        postShutdown();
	}

    /// Override this to perform custom actions after the main module shuts down
    virtual void postShutdown() {}

	virtual ~RadiantTest()
	{
        _coreModule->get()->getLogWriter().detach(_testLogFile.get());
        _testLogFile->close();
        _testLogFile.reset();

		module::shutdownStreams();
		_coreModule.reset();
        _fakeClipboard.reset();
        _glContextModule.reset();
	}

protected:
    void initTestLog()
    {
        auto fullPath = _context.getCacheDataPath() + "test.log";
        _testLogFile.reset(new TestLogFile(fullPath));
        _coreModule->get()->getLogWriter().attach(_testLogFile.get());
    }

	virtual void setupGameFolder()
	{}

	virtual void setupTestModules()
	{
		_glContextModule = std::make_shared<gl::HeadlessOpenGLContextModule>();
		_fakeClipboard = std::make_shared<FakeClipboardModule>();

		_coreModule->get()->getModuleRegistry().registerModule(_glContextModule);
		_coreModule->get()->getModuleRegistry().registerModule(_fakeClipboard);
	}

	virtual void loadMap(const std::string& mapsRelativePath)
	{
		GlobalCommandSystem().executeCommand("OpenMap", mapsRelativePath);
	}

    static constexpr const char* DEFAULT_GAME_TYPE = "The Dark Mod 2.0 (Standalone)";

	virtual void handleGameConfigMessage(game::ConfigurationNeeded& message)
	{
		game::GameConfiguration config;

		config.gameType = DEFAULT_GAME_TYPE;
		config.enginePath = _context.getTestProjectPath();

		message.setConfig(config);
		message.setHandled(true);
	}

	void handleNotification(radiant::NotificationMessage& msg)
	{
		switch (msg.getType())
		{
		case radiant::NotificationMessage::Information:
			rMessage() << msg.getMessage() << std::endl;
			break;

		case radiant::NotificationMessage::Warning:
			rWarning() << msg.getMessage() << std::endl;
			break;

		case radiant::NotificationMessage::Error:
			rError() << msg.getMessage() << std::endl;
			break;
		};
	}
};

}
