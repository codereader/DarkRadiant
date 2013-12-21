#include "Quake3MapFormat.h"

#include "itextstream.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "ibrush.h"
#include "ipatch.h"
#include "igame.h"
#include "iregistry.h"
#include "igroupnode.h"

#include "parser/DefTokeniser.h"

#include "scenelib.h"

#include "i18n.h"
#include "string/string.h"

#include <boost/lexical_cast.hpp>

#include "Quake3MapReader.h"
#include "Quake3MapWriter.h"

#include "Doom3MapFormat.h"

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
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_LAYERSYSTEM);
		_dependencies.insert(MODULE_BRUSHCREATOR);
		_dependencies.insert(MODULE_PATCH + DEF2);
		_dependencies.insert(MODULE_PATCH + DEF3);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Quake3MapFormat::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for maps and regions
	GlobalMapFormatManager().registerMapFormat("map", shared_from_this());

	// Register the map file extension in the FileTypeRegistry
	GlobalFiletypes().registerPattern("map", FileTypePattern(_("Quake 3 map"), "map", "*.map"));
}

void Quake3MapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
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
	char line[255];
	stream.getline(line, sizeof(line));

	// The first line in a Quake 3 map is empty
	if (strlen(line) > 0)
	{
		return false;
	}

	stream.getline(line, sizeof(line));

	// The second line should be the entity 0 comment
	if (strcmp(line, "// entity 0") != 0)
	{
		return false;
	}

	// This appears to be a quake 3 map, so let's return positive
	return true;
}

} // namespace map
