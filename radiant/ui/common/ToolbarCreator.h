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
 * Obtain a loaded toolbar by calling getToolbar(<toolbarName>);
 */

namespace ui {

// This is where the generated GtkToolbars are stored after parsing the XML file 
typedef std::map<const std::string, GtkToolbar*> ToolbarMap;

class ToolbarCreator {
	private:
		// The path and the name of the XML file
		const std::string _uiXmlPath;
		const std::string _uiXmlFile;
		
		void 		parseXml(xml::Document&);
		GtkToolbar*	createToolbar(xml::Node&);
		GtkWidget* 	createToolItem(xml::Node&, GtkToolbar*);
		
		ToolbarMap _toolbars;
	
	public:
		// Constructor
		ToolbarCreator(const std::string& uiXmlPath, const std::string& uiXmlFile = "ui.xml");
		
		// Public methods
		GtkToolbar* getToolbar(const std::string&);
		
		// Destructor						
		~ToolbarCreator() {}			
}; // class ToolbarCreator	

} // namespace toolbar

#endif /*TOOLBAR_H_*/
