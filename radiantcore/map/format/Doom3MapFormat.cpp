#include "Doom3MapFormat.h"

#include "itextstream.h"

#include "parser/DefTokeniser.h"

#include "Doom3MapReader.h"
#include "Doom3MapWriter.h"

#include "module/StaticModule.h"

namespace map
{

// RegisterableModule implementation
const std::string& Doom3MapFormat::getName() const {
	static std::string _name("Doom3MapLoader");
	return _name;
}

const StringSet& Doom3MapFormat::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Doom3MapFormat::initialiseModule(const IApplicationContext& ctx)
{
	// Register ourselves as map format for maps and regions
	GlobalMapFormatManager().registerMapFormat("map", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("reg", shared_from_this());
}

void Doom3MapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& Doom3MapFormat::getMapFormatName() const
{
	static std::string _name = "Doom 3";
	return _name;
}

const std::string& Doom3MapFormat::getGameType() const
{
	static std::string _gameType = "doom3";
	return _gameType;
}

IMapReaderPtr Doom3MapFormat::getMapReader(IMapImportFilter& filter) const
{
	return IMapReaderPtr(new Doom3MapReader(filter));
}

IMapWriterPtr Doom3MapFormat::getMapWriter() const
{
	return IMapWriterPtr(new Doom3MapWriter);
}

bool Doom3MapFormat::allowInfoFileCreation() const
{
	// allow .darkradiant files to be saved
	return true;
}

bool Doom3MapFormat::canLoad(std::istream& stream) const
{
	// Instantiate a tokeniser to read the first few tokens
	parser::BasicDefTokeniser<std::istream> tok(stream);

	try
	{
		// Require a "Version" token
		tok.assertNextToken("Version");

		// Require specific version, return true on success
		return (std::stof(tok.nextToken()) == MAP_VERSION_D3);
	}
	catch (parser::ParseException&)
	{}
	catch (std::invalid_argument&)
	{}

	return false;
}

module::StaticModuleRegistration<Doom3MapFormat> d3MapModule;

} // namespace map
