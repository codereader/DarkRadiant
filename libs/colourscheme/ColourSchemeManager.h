#ifndef COLOURSCHEMEMANAGER_H_
#define COLOURSCHEMEMANAGER_H_

#include <string>
#include <map>
#include "ColourScheme.h"

namespace ui {

// A map storing all the ColourSchemes with the name as key
typedef std::map<const std::string, ColourScheme> ColourSchemeMap;

class ColourSchemeManager {
	private:
		// This is where all the schemes are stored in
		ColourSchemeMap _colourSchemes;
		
		// the name of the active colourscheme
		std::string _activeScheme;
		
	public:
		// Constructor
		ColourSchemeManager() {}
		~ColourSchemeManager() {}
		
		// Checks if the specified scheme already exists
		bool schemeExists(const std::string& name);
		
		bool isActive(const std::string& name);
		void setActive(const std::string& name);
		
		ColourScheme& getActiveScheme();
		ColourScheme& getScheme(const std::string& name);
		ColourSchemeMap& getSchemeList();
		
		// Loads/saves all the schemes from the registry
		void loadColourSchemes();
		void saveColourSchemes();
		
		// Saves the specified scheme into the registry 
		void saveScheme(const std::string& name);
		void deleteScheme(const std::string& name);
		void copyScheme(const std::string& fromName, const std::string& toName);
		
		// Reverts all changes to the current objects and re-load them from the registry
		void restoreColourSchemes();
};

} // namespace ui

#endif /*COLOURSCHEMEMANAGER_H_*/
