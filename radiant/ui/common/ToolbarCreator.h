#ifndef TOOLBAR_H_
#define TOOLBAR_H_

#include <gtk/gtk.h>
#include <string>
#include <map>
#include "xmlutil/Document.h"

/* Parses the specified XML file on construction of the class
 * and creates the according toolbars.
 * 
 * All existing radiant callbacks are automatically connected to the widgets. 
 * 
 * Obtain a loaded toolbar by calling GetToolbar(<toolbarName>);
 */

namespace toolbar {
	
	// This is where the generated GtkToolbars are stored after parsing the XML file 
	typedef std::map<const std::string, GtkToolbar*> ToolbarMap;
	
	class ToolbarCreator {
		private:
			// The path and the name of the XML file
			const std::string _gameToolsPath;
			const std::string _uiXmlFile;
			
			void ParseXml(xml::Document doc);
			GtkToolbar* CreateToolbar(xml::Node& node);
			GtkWidget* CreateToolItem(xml::Node& node, GtkToolbar*);
			
			ToolbarMap _toolbars;
		
		public:
			// Constructor
			ToolbarCreator(const char* gameToolsPath, const char* uiXmlFile = "ui.xml");
			
			// Public methods
			GtkToolbar* GetToolbar(const std::string&);
			
			// Destructor						
			~ToolbarCreator() {}			
	}; // class ToolbarCreator	
	
} // namespace toolbar

#endif /*TOOLBAR_H_*/
