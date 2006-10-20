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
		void 		loadToolbars();
		GtkToolbar*	createToolbar(xml::Node&);
		GtkWidget* 	createToolItem(xml::Node&, GtkToolbar*);
		bool 		toolbarExists(const std::string& toolbarName);
		
		ToolbarMap 		_toolbars;
		GtkTooltips* 	_tooltips;
	
	public:
		// Constructor
		ToolbarCreator();
		
		// Public methods
		GtkToolbar* getToolbar(const std::string& toolbarName);
		
		// Destructor						
		~ToolbarCreator() {}			
}; // class ToolbarCreator	

} // namespace toolbar

#endif /*TOOLBAR_H_*/
