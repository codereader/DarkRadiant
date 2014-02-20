#pragma once

#include <set>
#include <string>
#include "iuimanager.h"
#include "xmlutil/Node.h"
#include <boost/shared_ptr.hpp>

/* The Toolbarmanager parses the specified XML file on construction of the class
 * and creates the according toolbars.
 *
 * All existing events are automatically connected to the widgets.
 *
 * Obtain a loaded toolbar by calling getToolbar(<toolbarName>);
 */
namespace Gtk
{
	class ToolItem;
}

class wxToolBarToolBase;

namespace ui {

class ToolbarManager :
	public IToolbarManager
{
	// This is where the available Toolbar names are stored after parsing the XML file
	typedef std::set<std::string> ToolbarList;
	ToolbarList _toolbars;

public:
	// Returns the toolbar that is named toolbarName
	wxToolBar* getwxToolbar(const std::string& toolbarName, wxWindow* parent);
	Gtk::Toolbar* getToolbar(const std::string& toolbarName);

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
	wxToolBar* createWxToolbar(xml::Node& node, wxWindow* parent);
	Gtk::Toolbar* createToolbar(xml::Node&);

	/**
	 * Checks the passed xmlNode for a recognized item (ToolButton, ToggleToolButton, Separator)
	 * Returns the widget or NULL if nothing useful is found.
	 */
	Gtk::ToolItem* createToolItem(xml::Node&);
	wxToolBarToolBase* createWxToolItem(wxToolBar* toolbar, xml::Node& node);

	bool toolbarExists(const std::string& toolbarName);
};

} // namespace ui
