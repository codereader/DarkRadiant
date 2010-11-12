#include "Doom3PrefabFormat.h"

#include "i18n.h"
#include "itextstream.h"
#include "ifiletypes.h"

#include "MapImportInfo.h"

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
	globalOutputStream() << getName() << ": initialiseModule called." << std::endl;

	// Add one more "map" type, with the pfb extension
	GlobalFiletypes().addType(
		"map", getName(), FileTypePattern(_("Doom 3 prefab"), "*.pfb"));

	// Add the filepatterns for the prefab (different order)
	GlobalFiletypes().addType(
		"prefab", getName(), FileTypePattern(_("Doom 3 prefab"), "*.pfb"));
	GlobalFiletypes().addType(
		"prefab", getName(), FileTypePattern(_("Doom 3 map"), "*.map"));
	GlobalFiletypes().addType(
		"prefab", getName(), FileTypePattern(_("Doom 3 region"), "*.reg"));
}

void Doom3PrefabFormat::onMapParsed(const MapImportInfo& importInfo) const
{
	// Ignore any layer information (do not attempt to load it)

	// Just process the func_static child primitives
	addOriginToChildPrimitives(importInfo.root);
}

} // namespace
