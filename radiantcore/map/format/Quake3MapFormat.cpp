#include "Quake3MapFormat.h"

#include "parser/DefTokeniser.h"

#include "Quake3MapReader.h"
#include "Quake3MapWriter.h"

#include "Doom3MapFormat.h"

#include "module/StaticModule.h"

namespace map
{

// RegisterableModule implementation
const std::string& Quake3MapFormat::getName() const
{
	static std::string _name("Quake3MapLoader");
	return _name;
}

const StringSet& Quake3MapFormat::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Quake3MapFormat::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for maps and regions
	GlobalMapFormatManager().registerMapFormat("map", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("reg", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("pfb", shared_from_this());
}

void Quake3MapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& Quake3MapFormat::getMapFormatName() const
{
	static std::string _name = "Quake 3";
	return _name;
}

const std::string& Quake3MapFormat::getGameType() const
{
	static std::string _gameType = "quake3";
	return _gameType;
}

IMapReaderPtr Quake3MapFormat::getMapReader(IMapImportFilter& filter) const
{
	return IMapReaderPtr(new Quake3MapReader(filter));
}

IMapWriterPtr Quake3MapFormat::getMapWriter() const
{
	return IMapWriterPtr(new Quake3MapWriter);
}

bool Quake3MapFormat::allowInfoFileCreation() const
{
	// allow .darkradiant files to be saved
	return true;
}

bool Quake3MapFormat::canLoad(std::istream& stream) const
{
	// Instantiate a tokeniser to read the first few tokens
	parser::BasicDefTokeniser<std::istream> tok(stream);

	try
	{
		// Require the opening brace of the first entity as first token
		tok.assertNextToken("{");
		
		// That's it for the moment being
		return true;
	}
	catch (parser::ParseException&)
	{}

	return false;
}

module::StaticModule<Quake3MapFormat> q3MapModule;

} // namespace map
