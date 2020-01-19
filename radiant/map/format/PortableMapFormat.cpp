#include "PortableMapFormat.h"

#include "PortableMapReader.h"
#include "PortableMapWriter.h"

namespace map
{

std::size_t PortableMapFormat::VERSION = 1;

// RegisterableModule implementation
const std::string& PortableMapFormat::getName() const
{
	static std::string _name(typeid(PortableMapFormat).name());
	return _name;
}

const StringSet& PortableMapFormat::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_LAYERSYSTEM);
		_dependencies.insert(MODULE_BRUSHCREATOR);
		_dependencies.insert(MODULE_PATCHDEF2);
		_dependencies.insert(MODULE_PATCHDEF3);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void PortableMapFormat::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for mapx files
	GlobalMapFormatManager().registerMapFormat("mapx", shared_from_this());
}

void PortableMapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& PortableMapFormat::getMapFormatName() const
{
	static std::string _name = "Portable Map";
	return _name;
}

const std::string& PortableMapFormat::getGameType() const
{
	static std::string _gameType = "doom3";
	return _gameType;
}

IMapReaderPtr PortableMapFormat::getMapReader(IMapImportFilter& filter) const
{
	return std::make_shared<PortableMapReader>(filter);
}

IMapWriterPtr PortableMapFormat::getMapWriter() const
{
	return std::make_shared<PortableMapWriter>();
}

bool PortableMapFormat::allowInfoFileCreation() const
{
	return false;
}

bool PortableMapFormat::canLoad(std::istream& stream) const
{
	return PortableMapReader::CanLoad(stream);
}

}
