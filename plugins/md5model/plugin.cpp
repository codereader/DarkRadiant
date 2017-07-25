#include "imodule.h"

#include "MD5ModelLoader.h"
#include "MD5AnimationCache.h"

namespace md5
{

class MD5Module :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const
	{
		static std::string _name("MD5Module");
		return _name;
	}

	const StringSet& getDependencies() const
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_MODELFORMATMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx)
	{
		GlobalModelFormatManager().registerImporter(std::make_shared<md5::MD5ModelLoader>());
	}
};

}

// DarkRadiant module entry point
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<md5::MD5Module>());
	registry.registerModule(std::make_shared<md5::MD5AnimationCache>());
}
