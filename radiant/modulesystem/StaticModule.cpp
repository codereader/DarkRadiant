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

void StaticModuleList::Add(const RegisterableModulePtr& module)
{
	Instance().push_back(module);
}

void StaticModuleList::ForEachModule(const std::function<void(const RegisterableModulePtr&)>& func)
{
	for (const auto& module : Instance())
	{
		func(module);
	}
}

void StaticModuleList::Clear()
{
	Instance().clear();
}

StaticModuleList& StaticModuleList::Instance()
{
	static StaticModuleList _list;
	return _list;
}

}

}
