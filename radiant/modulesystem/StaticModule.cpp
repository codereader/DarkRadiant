#include "StaticModule.h"

#include <cassert>

namespace module
{

namespace internal
{

StaticModuleList::~StaticModuleList()
{
	// We assume that the list instance is cleared by the time the
	// application is shut down
	assert(empty());
}

void StaticModuleList::Add(const ModuleCreationFunc& creationFunc)
{
	Instance().push_back(creationFunc);
}

void StaticModuleList::RegisterModules()
{
	for (const auto& creationFunc : Instance())
	{
		module::GlobalModuleRegistry().registerModule(creationFunc());
	}

	// Be sure the list is cleared after registration
	Instance().clear();
}

StaticModuleList& StaticModuleList::Instance()
{
	static StaticModuleList _list;
	return _list;
}

}

}
