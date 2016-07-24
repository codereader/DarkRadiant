#include "InfoFileManager.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

namespace map
{

void InfoFileManager::registerInfoFileModule(const IMapInfoFileModulePtr& module)
{
	if (_modules.find(module) != _modules.end())
	{
		rWarning() << "Duplicate info file module registered: " << module->getName() << std::endl;
		return;
	}

	_modules.insert(module);
}

void InfoFileManager::unregisterInfoFileModule(const IMapInfoFileModulePtr& module)
{
	if (_modules.find(module) == _modules.end())
	{
		rWarning() << "Trying to unregister non-existent info file module: " << module->getName() << std::endl;
		return;

	}

	_modules.erase(module);
}

void InfoFileManager::foreachModule(const std::function<void(IMapInfoFileModule&)>& functor)
{
	for (const IMapInfoFileModulePtr& module : _modules)
	{
		functor(*module);
	}
}

const std::string& InfoFileManager::getName() const
{
	static std::string _name(MODULE_MAPINFOFILEMANAGER);
	return _name;
}

const StringSet& InfoFileManager::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void InfoFileManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

void InfoFileManager::shutdownModule()
{
	rMessage() << getName() << "::shudownModule called." << std::endl;

	_modules.clear();
}

// Define the static InfoFileManager module
module::StaticModule<InfoFileManager> infoFileManagerModule;

}
