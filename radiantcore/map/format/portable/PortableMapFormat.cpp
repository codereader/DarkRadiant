#include "PortableMapFormat.h"

#include "imapformat.h"

#include "PortableMapReader.h"
#include "PortableMapWriter.h"

#include "module/StaticModule.h"

namespace map
{

namespace format
{

std::size_t PortableMapFormat::Version = 1;
const char* PortableMapFormat::Name = PORTABLE_MAP_FORMAT_NAME;

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
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void PortableMapFormat::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for mapx and pfbx files
	GlobalMapFormatManager().registerMapFormat("mapx", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("pfbx", shared_from_this());
}

void PortableMapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& PortableMapFormat::getMapFormatName() const
{
	static std::string _name = Name;
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

module::StaticModuleRegistration<PortableMapFormat> portableMapModule;

}

}
