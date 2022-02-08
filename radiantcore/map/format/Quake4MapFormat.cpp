#include "Quake4MapFormat.h"

#include "parser/DefTokeniser.h"

#include "Quake4MapReader.h"
#include "Quake4MapWriter.h"

#include "Doom3MapFormat.h"
#include "module/StaticModule.h"

namespace map
{

// RegisterableModule implementation
const std::string& Quake4MapFormat::getName() const
{
	static std::string _name("Quake4MapLoader");
	return _name;
}

const StringSet& Quake4MapFormat::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Quake4MapFormat::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for maps, regions and prefabs
	GlobalMapFormatManager().registerMapFormat("map", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("reg", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("pfb", shared_from_this());
}

void Quake4MapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& Quake4MapFormat::getMapFormatName() const
{
	static std::string _name = "Quake 4";
	return _name;
}

const std::string& Quake4MapFormat::getGameType() const
{
	static std::string _gameType = "quake4";
	return _gameType;
}

IMapReaderPtr Quake4MapFormat::getMapReader(IMapImportFilter& filter) const
{
	return IMapReaderPtr(new Quake4MapReader(filter));
}

IMapWriterPtr Quake4MapFormat::getMapWriter() const
{
	return IMapWriterPtr(new Quake4MapWriter);
}

bool Quake4MapFormat::allowInfoFileCreation() const
{
	// allow .darkradiant files to be saved
	return true;
}

bool Quake4MapFormat::canLoad(std::istream& stream) const
{
	// Instantiate a tokeniser to read the first few tokens
	parser::BasicDefTokeniser<std::istream> tok(stream);

	try
	{
		// Require a "Version" token
		tok.assertNextToken("Version");

		// Require specific version, return true on success
		return (std::stof(tok.nextToken()) == MAP_VERSION_Q4);
	}
	catch (parser::ParseException&)
	{}
	catch (std::invalid_argument&)
	{}

	return false;
}

module::StaticModuleRegistration<Quake4MapFormat> q4MapModule;

} // namespace map
