#include "AasFileManager.h"

#include "itextstream.h"

#include "iarchive.h"
#include "ieclass.h"
#include "ifilesystem.h"
#include "eclass.h"

#include "module/StaticModule.h"

namespace map
{

namespace
{
    const char* const AAS_TYPES_ENTITYDEF = "aas_types";
}

AasFileManager::AasFileManager() :
    _typesLoaded(false)
{}

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

void AasFileManager::ensureAasTypesLoaded()
{
    if (_typesLoaded) return;

    _typesLoaded = true;
    _typeList.clear();

    IEntityClassPtr aasTypesClass = GlobalEntityClassManager().findClass(AAS_TYPES_ENTITYDEF);

    if (aasTypesClass)
    {
        eclass::AttributeList list = eclass::getSpawnargsWithPrefix(*aasTypesClass, "type");

        for (const EntityClassAttribute& attr : list)
        {
            AasType type;
            type.entityDefName = attr.getValue();

            IEntityClassPtr aasType = GlobalEntityClassManager().findClass(type.entityDefName);

            if (!aasType)
            {
                rWarning() << "Could not find entityDef for AAS type " << type.entityDefName <<
                    " mentioned in " << AAS_TYPES_ENTITYDEF << " entityDef." << std::endl;
                continue;
            }

            type.fileExtension = aasType->getAttributeValue("fileExtension");
            _typeList.push_back(type);
        }
    }
}

AasTypeList AasFileManager::getAasTypes()
{
    ensureAasTypesLoaded();

    return _typeList;
}

AasType AasFileManager::getAasTypeByName(const std::string& typeName)
{
    ensureAasTypesLoaded();

    for (AasType& type : _typeList)
    {
        if (type.entityDefName == typeName)
        {
            return type;
        }
    }

    throw std::runtime_error("Could not find AAS type " + typeName);
}

std::list<AasFileInfo> AasFileManager::getAasFilesForMap(const std::string& mapPath)
{
    std::list<AasFileInfo> list;

    AasTypeList types = getAasTypes();

    for (const AasType& type : types)
    {
        std::string path = mapPath;

        // Cut off the extension
        path = path.substr(0, path.rfind('.'));
        path += "." + type.fileExtension;

        ArchiveTextFilePtr file = GlobalFileSystem().openTextFileInAbsolutePath(path);

        if (file)
        {
            // Add this file to the list
            list.push_back(AasFileInfo());
            list.back().absolutePath = path;
            list.back().type = type;
        }
    }

    return list;
}

const std::string& AasFileManager::getName() const
{
	static std::string _name(MODULE_AASFILEMANAGER);
	return _name;
}

const StringSet& AasFileManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
        _dependencies.insert(MODULE_ECLASSMANAGER);
	}

	return _dependencies;
}

void AasFileManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

// Define the static AasFileManager module
module::StaticModuleRegistration<AasFileManager> aasFileManagerModule;

}
