#pragma once

#include "gtest/gtest.h"
#include <iostream>

#include "module/ApplicationContextBase.h"
#include "module/CoreModule.h"

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
	radiant::ApplicationContextBase _context;

	std::unique_ptr<module::CoreModule> _coreModule;

protected:
	RadiantTest()
	{
		// Initialise the context
		_context.initialise(0, nullptr);

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

	~RadiantTest()
	{
		// Issue a shutdown() call to all the modules
		module::GlobalModuleRegistry().shutdownModules();

		_coreModule.reset();
	}
};