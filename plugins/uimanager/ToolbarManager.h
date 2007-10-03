#ifndef TOOLBARMANAGER_H_
#define TOOLBARMANAGER_H_

#include <map>
#include <string>
#include "iuimanager.h"
#include "xmlutil/Node.h"

/* The Toolbarmanager parses the specified XML file on construction of the class
 * and creates the according toolbars.
 * 
 * All existing events are automatically connected to the widgets. 
 * 
 * Obtain a loaded toolbar by calling getToolbar(<toolbarName>);
 */
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkToolbar GtkToolbar;
typedef struct _GtkTooltips GtkTooltips;

namespace ui {

class ToolbarManager :
	public IToolbarManager 
{
	// This is where the generated GtkToolbars are stored after parsing the XML file 
	typedef std::map<const std::string, GtkToolbar*> ToolbarMap;
	
	ToolbarMap _toolbars;
	GtkTooltips* _tooltips;

public:
	// Public methods
	GtkToolbar* getToolbar(const std::string& toolbarName);
	
	// Load toolbars from registry
	void initialise();
	
private:
	/** greebo: Loads the toolbars from the registry
	 */
	void loadToolbars();
	
	GtkToolbar*	createToolbar(xml::Node&);
	
	GtkWidget* createToolItem(xml::Node&);
	
	bool toolbarExists(const std::string& toolbarName);			

}; // class ToolbarManager	

} // namespace ui

#endif /*TOOLBARMANAGER_H_*/
