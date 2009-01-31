#include "ColourSchemeManager.h"
#include "iregistry.h"

#include "stringio.h"
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

/*	Dumps the current in-memory content of the colourschemes to globalOutputStream() 
 */
void ColourSchemeManager::dump() {
	globalOutputStream() << "Dump: Number of schemes: " << static_cast<int>(_colourSchemes.size()) << "\n";
	
	for (ColourSchemeMap::iterator it = _colourSchemes.begin(); it != _colourSchemes.end(); it++) {
		globalOutputStream() << "Dump: Schemename: " << it->first.c_str() << "\n";
		
		// Retrieve the list with all the ColourItems of this scheme
		ColourItemMap& colourMap = _colourSchemes[it->first].getColourMap();
	
		globalOutputStream() << "Dump: Number of ColourItems: " << 
								static_cast<int>(colourMap.size()) << "\n";
	
		// Cycle through all the ColourItems and save them into the registry	
		for (ColourItemMap::iterator c = colourMap.begin(); c != colourMap.end(); c++) {
			globalOutputStream() << "Dump: Colourname: " << c->first.c_str() << ", ";
			std::string colourValue = c->second;
			globalOutputStream() << "Dump: Colourvalue: " << colourValue.c_str() << "\n";
		}
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
	xml::Node schemeNode = GlobalRegistry().createKeyWithName(basePath, "scheme", name);
	
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
	ColourItemMap& colourMap = _colourSchemes[name].getColourMap();
	
	// Cycle through all the ColourItems and save them into the registry	
	for (ColourItemMap::iterator it = colourMap.begin(); it != colourMap.end(); it++) {
		// Retrieve the name of the ColourItem
		std::string name = it->first;
		
		// Cast the ColourItem onto a std::string
		std::string colour = it->second;
			
		xml::Node colourNode = GlobalRegistry().createKeyWithName(schemePath, "colour", name);
		colourNode.setAttributeValue("value", colour);
	}
}

void ColourSchemeManager::saveColourSchemes() {
	// Delete all existing schemes from the registry
	GlobalRegistry().deleteXPath("user/ui/colourschemes//scheme");
	
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
	xml::NodeList schemeNodes = GlobalRegistry().findXPath("user/ui/colourschemes/scheme");
	
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

Vector3 ColourSchemeManager::getColour(const std::string& colourName) {
	// Cast the ColourItem object onto a Vector3
	return _colourSchemes[_activeScheme].getColour(colourName);
}

ColourItem& ColourSchemeManager::getColourItem(const std::string& colourName) {
	return _colourSchemes[_activeScheme].getColour(colourName);
}

ColourSchemeManager& ColourSchemeManager::Instance() {
	static ColourSchemeManager _manager;
	return _manager;
}

} // namespace ui
