#include "ToolbarManager.h"

#include <stdexcept>
#include "itextstream.h"
#include "ieventmanager.h"
#include "iregistry.h"
#include "i18n.h"

#include <gtkmm/separatortoolitem.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/image.h>

namespace ui
{

void ToolbarManager::initialise()
{
	try
	{
		// Query the registry
		loadToolbars();
	}
	catch (std::runtime_error& e)
	{
		std::cout << "ToolbarManager: Warning: " << e.what() << std::endl;
	}
}

Gtk::Toolbar* ToolbarManager::getToolbar(const std::string& toolbarName)
{
	// Check if the toolbarName exists
	if (toolbarExists(toolbarName))
	{
		// Instantiate the toolbar with buttons
		rMessage() << "ToolbarManager: Instantiating toolbar: " << toolbarName << std::endl;

		// Build the path into the registry, where the toolbar should be found
		std::string toolbarPath = std::string("//ui//toolbar") + "[@name='"+ toolbarName +"']";
		xml::NodeList toolbarList = GlobalRegistry().findXPath(toolbarPath);

		if (!toolbarList.empty())
		{
			return createToolbar(toolbarList[0]);
		}
		else {
			rError() << "ToolbarManager: Critical: Could not instantiate " << toolbarName << std::endl;
			return NULL;
		}
	}
	else
	{
		rError() << "ToolbarManager: Critical: Named toolbar doesn't exist: " << toolbarName << std::endl;
		return NULL;
	}
}

Gtk::ToolItem* ToolbarManager::createToolItem(xml::Node& node)
{
	const std::string nodeName = node.getName();

	Gtk::ToolItem* toolItem = NULL;

	if (nodeName == "separator")
	{
		toolItem = Gtk::manage(new Gtk::SeparatorToolItem);
	}
	else if (nodeName == "toolbutton" || nodeName == "toggletoolbutton")
	{
		// Found a button, load the values that are shared by both types
		const std::string name 		= node.getAttributeValue("name");
		const std::string icon 		= node.getAttributeValue("icon");
		const std::string tooltip 	= _(node.getAttributeValue("tooltip").c_str());
		const std::string action 	= node.getAttributeValue("action");

		Gtk::ToolButton* toolButton = NULL;

		if (nodeName == "toolbutton")
		{
			// Create a new GtkToolButton and assign the right callback
			toolButton = Gtk::manage(new Gtk::ToolButton(name));
		}
		else
		{
			// Create a new GtkToggleToolButton and assign the right callback
			toolButton = Gtk::manage(new Gtk::ToggleToolButton);
		}

		IEventPtr ev = GlobalEventManager().findEvent(action);

		if (!ev->empty())
		{
			ev->connectWidget(toolButton);

			// Tell the event to update the state of this button
			ev->updateWidgets();
		}
		else
		{
			rError() << "ToolbarManager: Failed to lookup command " << action << std::endl;
		}

		// Set the tooltip, if not empty
		if (!tooltip.empty())
		{
			toolButton->set_tooltip_text(tooltip);
		}

		// Load and assign the icon, if specified
		if (!icon.empty())
		{
			Gtk::Image* image = Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbufWithMask(icon)));
			image->show();
			toolButton->set_icon_widget(*image);
		}

		toolItem = toolButton;
	}
	else
	{
		return NULL;
	}

	toolItem->show();
	return toolItem;
}

Gtk::Toolbar* ToolbarManager::createToolbar(xml::Node& node)
{
	// Get all action children elements
	xml::NodeList toolItemList = node.getChildren();
	Gtk::Toolbar* toolbar = NULL;

	if (!toolItemList.empty())
	{
		// Create a new toolbar
		toolbar = Gtk::manage(new Gtk::Toolbar);
		toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);

		// Try to set the alignment, if the attribute is properly set
		std::string align = node.getAttributeValue("align");

		toolbar->set_orientation(align == "vertical" ? Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL);

		for (std::size_t i = 0; i < toolItemList.size(); ++i)
		{
			// Create and get the toolItem with the parsing
			Gtk::ToolItem* toolItem = createToolItem(toolItemList[i]);

			// It is possible that no toolItem is returned, only add it if it's safe to do so
			if (toolItem != NULL)
			{
				toolbar->insert(*toolItem, -1);
			}
		}
	}
	else
	{
		throw std::runtime_error("No elements in toolbar.");
	}

	return toolbar;
}

bool ToolbarManager::toolbarExists(const std::string& toolbarName)
{
	return (_toolbars.find(toolbarName) != _toolbars.end());
}

void ToolbarManager::loadToolbars()
{
	xml::NodeList toolbarList = GlobalRegistry().findXPath("//ui//toolbar");

	if (!toolbarList.empty())
	{
		for (std::size_t i = 0; i < toolbarList.size(); ++i)
		{
			std::string toolbarName = toolbarList[i].getAttributeValue("name");

			if (toolbarExists(toolbarName))
			{
				//rMessage() << "This toolbar already exists: ";
				continue;
			}

			rMessage() << "Found toolbar: " << toolbarName << std::endl;

			_toolbars.insert(toolbarName);
		}
	}
	else
	{
		throw std::runtime_error("No toolbars found.");
	}
}

} // namespace ui
