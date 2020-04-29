#include "CoreModule.h"

#include "DynamicLibrary.h"

namespace module
{

CoreModule::CoreModule(ApplicationContext& context) :
	_instance(nullptr)
{
	std::string coreModuleFile = std::string("DarkRadiantCore") + MODULE_FILE_EXTENSION;

	fs::path coreModulePath = context.getApplicationPath();
	coreModulePath /= coreModuleFile;

	if (!fs::exists(coreModulePath))
	{
		throw FailureException("Cannot find the main module " + coreModuleFile);
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
