#include "ColourSchemeManager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ieclasscolours.h"

#include "module/StaticModule.h"

namespace colours
{

const char* const COLOURSCHEME_VERSION = "1.0";

void ColourSchemeManager::foreachScheme(const std::function<void(const std::string&, IColourScheme&)>& functor)
{
	for (auto& pair : _colourSchemes)
	{
		functor(pair.first, pair.second);
	}
}

/*	returns true, if the scheme called <name> exists
 */
bool ColourSchemeManager::schemeExists(const std::string& name)
{
	return _colourSchemes.find(name) != _colourSchemes.end();
}

ColourScheme& ColourSchemeManager::getActiveScheme()
{
	return _colourSchemes[_activeScheme];
}

ColourScheme& ColourSchemeManager::getColourScheme(const std::string& name)
{
	return _colourSchemes[name];
}

bool ColourSchemeManager::isActive(const std::string& name)
{
	return schemeExists(name) && name == _activeScheme;
}

void ColourSchemeManager::setActive(const std::string& name)
{
	if (schemeExists(name))
	{
		_activeScheme = name;
	}
}

void ColourSchemeManager::restoreColourSchemes()
{
	// Clear the whole colourScheme map and reload it from the registry
	_colourSchemes.clear();
	loadColourSchemes();
    emitEclassOverrides();
}

void ColourSchemeManager::deleteScheme(const std::string& name)
{
	if (schemeExists(name))
	{
		// Delete the scheme from the map
		_colourSchemes.erase(name);

		// Choose a new theme from the list, if the active scheme was deleted
		if (_activeScheme == name && !_colourSchemes.empty())
		{
			_activeScheme = _colourSchemes.begin()->second.getName();
		}
	}
}

void ColourSchemeManager::saveScheme(const std::string& name)
{
	std::string basePath = "user/ui/colourschemes";

	// Re-create the schemeNode
	xml::Node schemeNode = GlobalRegistry().createKeyWithName(basePath, "colourscheme", name);

	schemeNode.setAttributeValue("version", COLOURSCHEME_VERSION);

	// Set the readonly attribute if necessary
	if (_colourSchemes[name].isReadOnly())
	{
		schemeNode.setAttributeValue("readonly", "1");
	}

	// Set the active attribute, if this is the active scheme
	if (name == _activeScheme)
	{
		schemeNode.setAttributeValue("active", "1");
	}

	// This will be the path where all the <colour> nodes are added to
	std::string schemePath = basePath + "/colourscheme[@name='" + name + "']";

	// Retrieve the list with all the ColourItems of this scheme
	auto& scheme = _colourSchemes[name];

	// Cycle through all the ColourItems and save them into the registry
	scheme.foreachColour([&](const std::string& name, colours::IColourItem& item)
	{
		auto colourNode = GlobalRegistry().createKeyWithName(schemePath, "colour", name);
		colourNode.setAttributeValue("value", string::to_string(item.getColour()));
	});
}

void ColourSchemeManager::saveColourSchemes()
{
	// Delete all existing schemes from the registry
	GlobalRegistry().deleteXPath("user/ui/colourschemes//colourscheme");

	// Save all schemes that are stored in memory
	for (auto it = _colourSchemes.begin(); it != _colourSchemes.end(); ++it)
	{
		if (!it->first.empty())
		{
			// Save the scheme whose name is stored in it->first
			saveScheme(it->first);
		}
	}

	// Flush the whole colour scheme structure and re-load it from the registry.
	// This is to remove any remaining artifacts.
	restoreColourSchemes();
}

void ColourSchemeManager::loadColourSchemes()
{
	// load from XMLRegistry
	rMessage() << "ColourSchemeManager: Loading colour schemes..." << std::endl;

	// Find all <scheme> nodes
	auto schemeNodes = GlobalRegistry().findXPath(
		"user/ui/colourschemes/colourscheme[@version='" + std::string(COLOURSCHEME_VERSION) + "']"
	);

	if (schemeNodes.empty())
	{
		rMessage() << "ColourSchemeManager: No schemes found..." << std::endl;
		return;
	}

	std::string schemeName = "";
    _activeScheme = "";

	// Cycle through all found scheme nodes
	for (const auto& node : schemeNodes)
	{
		schemeName = node.getAttributeValue("name");

		// If the scheme is already in the list, skip it
		if (!schemeExists(schemeName))
		{
			// Construct the ColourScheme class from the xml::node
			_colourSchemes[schemeName] = ColourScheme(node);

			// Check, if this is the currently active scheme
			if (_activeScheme.empty() && node.getAttributeValue("active") == "1")
			{
				_activeScheme = schemeName;
			}
		}
		else if (node.getAttributeValue("readonly") == "1")
		{
			// Scheme exists, but we have a factory-defined scheme
			// try to merge any missing items into the existing scheme
			ColourScheme readOnlyScheme(node);

			_colourSchemes[schemeName].mergeMissingItemsFromScheme(readOnlyScheme);
		}
	}

	// If there isn't any active scheme yet, take the last one as active scheme
	if (_activeScheme.empty() && !schemeNodes.empty())
	{
		_activeScheme = schemeName;
	}
}

void ColourSchemeManager::copyScheme(const std::string& fromName, const std::string& toName)
{
	if (schemeExists(fromName))
	{
		// Copy the actual entry
		_colourSchemes[toName] = _colourSchemes[fromName];
		_colourSchemes[toName].setReadOnly(false);
	}
	else
	{
		rMessage() << "ColourSchemeManager: Scheme " << fromName << " does not exist!" << std::endl;
	}
}

Vector3 ColourSchemeManager::getColour(const std::string& colourName)
{
	return _colourSchemes[_activeScheme].getColour(colourName).getColour();
}

const std::string& ColourSchemeManager::getName() const
{
	static std::string _name(MODULE_COLOURSCHEME_MANAGER);
	return _name;
}

const StringSet& ColourSchemeManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_ECLASS_COLOUR_MANAGER);
	}

	return _dependencies;
}

void ColourSchemeManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	loadColourSchemes();
    emitEclassOverrides();
}

void ColourSchemeManager::emitEclassOverrides()
{
    auto& colourManager = GlobalEclassColourManager();
    colourManager.clearOverrideColours();

    // Apply the overrides for the known entity classes
    auto& activeScheme = getActiveScheme();

    colourManager.addOverrideColour("worldspawn", activeScheme.getColour("default_brush").getColour());
    colourManager.addOverrideColour("light", activeScheme.getColour("light_volumes").getColour());
}

module::StaticModuleRegistration<ColourSchemeManager> colourSchemeManagerModule;

} // namespace ui
