#include "AasFileManager.h"

#include "itextstream.h"
#include "Doom3AasFileLoader.h"

#include "iarchive.h"
#include "ifilesystem.h"

namespace map
{

void AasFileManager::registerLoader(const IAasFileLoaderPtr& loader)
{
    _loaders.insert(loader);
}

void AasFileManager::unregisterLoader(const IAasFileLoaderPtr& loader)
{
    _loaders.erase(loader);
}

IAasFileLoaderPtr AasFileManager::getLoaderForStream(std::istream& stream)
{
    IAasFileLoaderPtr loader;

    for (const IAasFileLoaderPtr& candidate : _loaders)
    {
        // Rewind the stream before passing it to the format for testing
		stream.seekg(0, std::ios_base::beg);

		if (candidate->canLoad(stream))
		{
            loader = candidate;
            break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);
    
    return loader;
}

const std::string& AasFileManager::getName() const
{
	static std::string _name("Z" + std::string(MODULE_AASFILEMANAGER));
	return _name;
}

const StringSet& AasFileManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void AasFileManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

    // Register the Doom 3 AAS format
    registerLoader(std::make_shared<Doom3AasFileLoader>());

#if _DEBUG
    ArchiveTextFilePtr file = GlobalFileSystem().openTextFile("maps/city_area.aas32");

    std::istream stream(&file->getInputStream());
    IAasFileLoaderPtr loader = getLoaderForStream(stream);

    if (loader && loader->canLoad(stream))
    {
        stream.seekg(0, std::ios_base::beg);

        loader->loadFromStream(stream);
    }
#endif
}

}
