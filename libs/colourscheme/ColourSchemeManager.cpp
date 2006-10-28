#include "ColourSchemeManager.h"
#include "xmlutil/XMLRegistry.h"
#include "plugin.h"

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

namespace ui {

/*	returns true, if the scheme called <name> exists
 */ 
bool ColourSchemeManager::schemeExists(const std::string& name) {
	ColourSchemeMap::iterator it = _colourSchemes.find(name);
   	return (it != _colourSchemes.end());
}

ColourSchemeMap& ColourSchemeManager::getSchemeList() {
	return _colourSchemes;
}

ColourScheme& ColourSchemeManager::getActiveScheme() {
	return _colourSchemes[_activeScheme];
}

ColourScheme& ColourSchemeManager::getScheme(const std::string& name) {
	return _colourSchemes[name];
}

/*	Returns true, if the scheme called <name> is currently active
 */
bool ColourSchemeManager::isActive(const std::string& name) {
	if (schemeExists(name)) {
		return (name == _activeScheme);
	}
	else {
		return false;
	}
}

void ColourSchemeManager::setActive(const std::string& name) {
	if (schemeExists(name)) {
		_activeScheme = name;
	}
}

void ColourSchemeManager::restoreColourSchemes() {
	// Clear the whole colourScheme map and reload it from the registry
	_colourSchemes.clear();	
	loadColourSchemes();
}

void ColourSchemeManager::deleteScheme(const std::string& name) {
	if (schemeExists(name)) {
		// Delete the scheme from the map
		_colourSchemes.erase(name);
		
		// Choose a new theme from the list, if the active scheme was deleted
		if (_activeScheme == name) {
			ColourSchemeMap::iterator it = _colourSchemes.begin();
			_activeScheme = it->second.getName();
		}
	}
}

/*	Saves the scheme called <name> into the registry
 */
void ColourSchemeManager::saveScheme(const std::string& name) {
	std::string basePath = "user/ui/colourschemes";
	
	// Re-create the schemeNode
	xml::Node schemeNode = registry().createKeyWithName(basePath, "scheme", name);
	
	// Set the readonly attribute if necessary
	if (_colourSchemes[name].isReadOnly()) {
		schemeNode.setAttributeValue("readonly", "1");
	}
	
	// Set the active attribute, if this is the active scheme
	if (name == _activeScheme) {
		schemeNode.setAttributeValue("active", "1");
	}
		
	// This will be the path where all the <colour> nodes are added to
	std::string schemePath = basePath + "/scheme[@name='" + name + "']";
		
	// Retrieve the list with all the ColourItems of this scheme
	ColourItemVec& colourVec = _colourSchemes[name].getColourList();
	
	// Cycle through all the ColourItems and save them into the registry	
	for (unsigned int j=0; j < colourVec.size(); j++) {
		// Retrieve the name of the ColourItem
		std::string name = colourVec[j].getName();
			
		// Cast the ColourItem onto a std::string
		std::string colour = colourVec[j];
			
		xml::Node colourNode = registry().createKeyWithName(schemePath, "colour", name);
		colourNode.setAttributeValue("value", colour);
	}
}

void ColourSchemeManager::saveColourSchemes() {
	// Delete all existing schemes from the registry
	registry().deleteXPath("user/ui//colourschemes");
	
	// Save all schemes that are stored in memory 
	for (ColourSchemeMap::iterator it = _colourSchemes.begin(); it != _colourSchemes.end(); it++) {
		if (it->first != "") {
			// Save the scheme whose name is stored in it->first
			saveScheme(it->first);
		}
	}
	
	// Flush the whole colour scheme structure and re-load it from the registry.
	// This is to remove any remaining artifacts from the memory. 
	restoreColourSchemes();
}

void ColourSchemeManager::loadColourSchemes() {
	std::string schemeName = "";
	
	// load from XMLRegistry
	globalOutputStream() << "ColourSchemeManager: Loading colour schemes...\n";
	
	// Find all <scheme> nodes
	xml::NodeList schemeNodes = registry().findXPath("user/ui/colourschemes/scheme");
	
	if (schemeNodes.size()>0) {
		// Cycle through all found scheme nodes	
		for (unsigned int i = 0; i < schemeNodes.size(); i++) {
			schemeName = schemeNodes[i].getAttributeValue("name");
			
			// If the scheme is already in the list, skip it
			if (!schemeExists(schemeName)) {
				// Construct the ColourScheme class from the xml::node
				_colourSchemes[schemeName] = ColourScheme(schemeNodes[i]);
				
				// Check, if this is the currently active scheme
				if (schemeNodes[i].getAttributeValue("active") == "1") {
					_activeScheme = schemeName;
				}
			}
		}
		
		// If there isn't any active scheme yet, take the last one as active scheme
		if (_activeScheme == "" && schemeNodes.size() > 0) {
			_activeScheme = schemeName;
		}
	} 
	else {
		globalOutputStream() << "ColourSchemeManager: No schemes found...\n";
	}
}

void ColourSchemeManager::copyScheme(const std::string& fromName, const std::string& toName) {
	if (schemeExists(fromName)) {
		// Copy the actual entry
		_colourSchemes[toName] = _colourSchemes[fromName];
		_colourSchemes[toName].setReadOnly(false);
	}
	else {
		globalOutputStream() << "ColourSchemeManager: Scheme " << fromName.c_str() << " does not exist!\n";
	}
}

} // namespace ui
