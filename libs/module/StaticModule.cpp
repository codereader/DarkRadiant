#include "StaticModule.h"

#include <cassert>

namespace module
{

namespace internal
{

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

    Instance().clear(); // clear the static list after dispatching all modules
}

StaticModuleList& StaticModuleList::Instance()
{
	static StaticModuleList _list;
	return _list;
}

}

}
