#include "iarchive.h"

#include "itextstream.h"
#include "ZipArchive.h"

namespace archive
{

class Pk4ArchiveLoader :
	public ArchiveLoader
{
public:
	// greebo: Returns the opened file or NULL if failed.
	virtual ArchivePtr openArchive(const std::string& name) override
	{
		return std::make_shared<ZipArchive>(name);
	}

	virtual const std::string& getExtension() override
	{
		static std::string _ext("pk4");
		return _ext;
	}

	// RegisterableModule implementation
	const std::string& getName() const override
	{
		static std::string _name(MODULE_ARCHIVE + "PK4");
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;
		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << getName() << "::initialiseModule called" << std::endl;
	}
};

}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<archive::Pk4ArchiveLoader>());
}
