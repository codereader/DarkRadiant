#pragma once

#include "gtest/gtest.h"
#include <sigc++/functors/mem_fun.h>
#include <iostream>

#include "imessagebus.h"
#include "iradiant.h"
#include "imap.h"
#include "icommandsystem.h"

#include "TestContext.h"
#include "module/CoreModule.h"
#include "messages/GameConfigNeededMessage.h"

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
		}
		catch (module::CoreModule::FailureException & ex)
		{
			// Streams are not yet initialised, so log to std::err at this point
			std::cerr << ex.what() << std::endl;
		}
	}

	void SetUp() override
	{
		// Set up the test game environment
		setupGameFolder();

		// Wire up the game-config-needed handler, we need to respond
		_gameSetupListener = _coreModule->get()->getMessageBus().addListener(
			radiant::IMessage::Type::GameConfigNeeded,
				radiant::TypeListener<game::ConfigurationNeeded>(
					sigc::mem_fun(this, &RadiantTest::handleGameConfigMessage)));

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
	}

	void TearDown() override
	{
		_coreModule->get()->getMessageBus().removeListener(_gameSetupListener);

		// Issue a shutdown() call to all the modules
		module::GlobalModuleRegistry().shutdownModules();
	}

	~RadiantTest()
	{
		_coreModule.reset();
	}

protected:
	virtual void setupGameFolder()
	{
		
	}

	virtual void loadMap(const std::string& modRelativePath)
	{
		GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
	}

	void handleGameConfigMessage(game::ConfigurationNeeded& message)
	{
		game::GameConfiguration config;

		config.gameType = "The Dark Mod 2.0 (Standalone)";
		config.enginePath = _context.getTestResourcePath();

		message.setConfig(config);
		message.setHandled(true);
	}
};