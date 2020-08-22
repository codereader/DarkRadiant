#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "os/fs.h"

#define SYMBOL_CREATE_RADIANT CreateRadiant
#define SYMBOL_DESTROY_RADIANT DestroyRadiant
#define Q(x) #x
#define QUOTE(x) Q(x)

namespace module
{

class DynamicLibrary;

class CoreModule
{
private:
	typedef radiant::IRadiant* (*CreateRadiantFunc)(IApplicationContext& context);
	typedef void (*DestroyRadiantFunc)(radiant::IRadiant* radiant);

	radiant::IRadiant* _instance;

	std::unique_ptr<DynamicLibrary> _coreModuleLibrary;

public:
	class FailureException :
		public std::runtime_error
	{
	public:
		FailureException(const std::string& msg) :
			std::runtime_error(msg)
		{}
	};

	CoreModule(IApplicationContext& context);

	~CoreModule();

	radiant::IRadiant* get();

private:
	void destroy();
};

}
