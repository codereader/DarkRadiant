#include "CoreModule.h"

#include "DynamicLibrary.h"

// In Linux the CORE_MODULE_LIBRARY symbol is defined in config.h
#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define CORE_MODULE_LIBRARY "DarkRadiantCore"
#endif

namespace module
{

CoreModule::CoreModule(ApplicationContext& context) :
	_instance(nullptr)
{
	std::string coreModuleFile = std::string(CORE_MODULE_LIBRARY) + MODULE_FILE_EXTENSION;

	fs::path coreModulePath = context.getLibraryPath();
	coreModulePath /= coreModuleFile;

	if (!fs::exists(coreModulePath))
	{
		throw FailureException("Cannot find the main module " + coreModulePath.string());
	}

	_coreModuleLibrary.reset(new DynamicLibrary(coreModulePath.string()));

	if (_coreModuleLibrary->failed())
	{
		throw FailureException("Cannot load the main module " + _coreModuleLibrary->getName());
	}

	auto symbol = _coreModuleLibrary->findSymbol(QUOTE(SYMBOL_CREATE_RADIANT));

	if (symbol == nullptr)
	{
		throw FailureException("Main module " + _coreModuleLibrary->getName() +
			" doesn't expose the symbol " + QUOTE(SYMBOL_CREATE_RADIANT));
	}

	auto createFunc = reinterpret_cast<CreateRadiantFunc>(symbol);

	_instance = createFunc(context);
}

CoreModule::~CoreModule()
{
	destroy();
}

radiant::IRadiant* CoreModule::get()
{
	return _instance;
}

void CoreModule::destroy()
{
	if (_instance)
	{
		assert(_coreModuleLibrary);

		auto symbol = _coreModuleLibrary->findSymbol(QUOTE(SYMBOL_DESTROY_RADIANT));

		if (symbol == nullptr)
		{
			throw FailureException("Main module " + _coreModuleLibrary->getName() +
				" doesn't expose the symbol " + QUOTE(SYMBOL_DESTROY_RADIANT));
		}

		auto destroyFunc = reinterpret_cast<DestroyRadiantFunc>(symbol);

		destroyFunc(_instance);
		_instance = nullptr;
	}
}

}
