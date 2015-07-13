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

public:
    // IAasFileManager implementation
    void registerLoader(const IAasFileLoaderPtr& loader) override;
    void unregisterLoader(const IAasFileLoaderPtr& loader) override;
    IAasFileLoaderPtr getLoaderForStream(std::istream& stream) override;

    // RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
};

} // namespace
