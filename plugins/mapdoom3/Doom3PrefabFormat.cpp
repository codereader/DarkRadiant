#include "Doom3PrefabFormat.h"

#include "i18n.h"
#include "itextstream.h"
#include "ifiletypes.h"

namespace map
{

// RegisterableModule implementation
const std::string& Doom3PrefabFormat::getName() const
{
	static std::string _name("Doom3PrefabLoader");
	return _name;
}

void Doom3PrefabFormat::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format
	GlobalMapFormatManager().registerMapFormat("pfb", shared_from_this());

	// Register the prefab file extension for the "map" filetype
	GlobalFiletypes().registerPattern("map", FileTypePattern(_("Doom 3 prefab"), "pfb", "*.pfb"));

	// Register the prefab file extensions for the "prefab" filetype
	GlobalFiletypes().registerPattern("prefab", FileTypePattern(_("Doom 3 prefab"), "pfb", "*.pfb"));
	GlobalFiletypes().registerPattern("prefab", FileTypePattern(_("Doom 3 map"), "map", "*.map"));
	GlobalFiletypes().registerPattern("prefab", FileTypePattern(_("Doom 3 region"), "reg", "*.reg"));
}

void Doom3PrefabFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

bool Doom3PrefabFormat::allowInfoFileCreation() const
{
	return false;
}

} // namespace
