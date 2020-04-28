#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "os/fs.h"

#include "DynamicLibrary.h"

#define SYMBOL_CREATE_RADIANT CreateRadiant
#define SYMBOL_DESTROY_RADIANT DestroyRadiant
#define Q(x) #x
#define QUOTE(x) Q(x)

namespace module
{

class CoreModule
{
private:
	typedef radiant::IRadiant* (*CreateRadiantFunc)(ApplicationContext& context);
	typedef void (*DestroyRadiantFunc)(radiant::IRadiant* radiant);

	radiant::IRadiant* _instance;

	DynamicLibraryPtr _coreModuleLibrary;

public:
	class FailureException :
		public std::runtime_error
	{
	public:
		FailureException(const std::string& msg) :
			std::runtime_error(msg)
		{}
	};

	CoreModule(ApplicationContext& context) :
		_instance(nullptr)
	{
		std::string coreModuleFile = std::string("DarkRadiantCore") + MODULE_FILE_EXTENSION;

		fs::path coreModulePath = context.getApplicationPath();
		coreModulePath /= coreModuleFile;

		if (!fs::exists(coreModulePath))
		{
			throw FailureException("Cannot find the main module " + coreModuleFile);
		}

		_coreModuleLibrary = std::make_shared<DynamicLibrary>(coreModulePath.string());

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

	~CoreModule()
	{
		destroy();
	}

	radiant::IRadiant* get()
	{
		return _instance;
	}

private:
	void destroy()
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
};

}
