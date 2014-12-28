#pragma once

#include <set>
#include <string>
#include "iuimanager.h"
#include "xmlutil/Node.h"
#include <memory>

/* The Toolbarmanager parses the specified XML file on construction of the class
 * and creates the according toolbars.
 *
 * All existing events are automatically connected to the widgets.
 *
 * Obtain a loaded toolbar by calling getToolbar(<toolbarName>);
 */
class wxToolBarToolBase;

namespace ui {

class ToolbarManager :
	public IToolbarManager
{
	// This is where the available Toolbar names are stored after parsing the XML file
	typedef std::set<std::string> ToolbarList;
	ToolbarList _toolbars;

	static int _nextToolItemId;

public:
	// Returns the toolbar that is named toolbarName
	wxToolBar* getToolbar(const std::string& toolbarName, wxWindow* parent);

	// Load toolbars from registry
	void initialise();

private:
	/**
	 * Parses the XML Document for toolbars and instantiates them
	 * Returns nothing, toolbars can be obtained via GetToolbar()
	 */
	void loadToolbars();

	/**
	 * Creates a toolbar based on the data found in the passed xmlNode
	 * Returns the fully populated wxToolBar.
	 */
	wxToolBar* createToolbar(xml::Node& node, wxWindow* parent);

	/**
	 * Checks the passed xmlNode for a recognized item (ToolButton, ToggleToolButton, Separator)
	 * Returns the widget or NULL if nothing useful is found.
	 */
	wxToolBarToolBase* createToolItem(wxToolBar* toolbar, xml::Node& node);

	bool toolbarExists(const std::string& toolbarName);
};

} // namespace ui
