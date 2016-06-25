#pragma once

#include "iaasfile.h"
#include <set>

namespace map
{

class AasFileManager :
    public IAasFileManager
{
private:
    // A mapping between extensions and format modules
	// Multiple modules can register themselves for a single extension
	typedef std::set<IAasFileLoaderPtr> Loaders;
	Loaders _loaders;

    AasTypeList _typeList;
    bool _typesLoaded;

public:
    AasFileManager();

    // IAasFileManager implementation
    void registerLoader(const IAasFileLoaderPtr& loader) override;
    void unregisterLoader(const IAasFileLoaderPtr& loader) override;
    IAasFileLoaderPtr getLoaderForStream(std::istream& stream) override;
    AasTypeList getAasTypes() override;
    AasType getAasTypeByName(const std::string& typeName) override;
    std::list<AasFileInfo> getAasFilesForMap(const std::string& mapPath) override;

    // RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

private:
    void ensureAasTypesLoaded();
};

} // namespace
