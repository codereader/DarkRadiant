#include "ToolbarCreator.h"

#include <gtk/gtk.h>
#include <string>

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include "commands.h"
#include "gtkutil/pointer.h"
#include "gtkutil/image.h"
#include "gtkutil/button.h"
#include "xmlutil/Document.h"
#include "xmlutil/AttributeNotFoundException.h"

// This is needed to correctly connect the ToggleButton to Radiant's callbacks
// The "handler" object data was set in createToolItem
void toggleButtonSetActiveNoSignal(GtkToggleToolButton* button, gboolean active)
{
  guint handler_id = gpointer_to_int(g_object_get_data(G_OBJECT(button), "handler"));
  g_signal_handler_block(G_OBJECT(button), handler_id);
  gtk_toggle_tool_button_set_active(button, active);
  g_signal_handler_unblock(G_OBJECT(button), handler_id);
}

void toggleButtonSetActiveCallback(GtkToggleToolButton& button, bool active)
{
  toggleButtonSetActiveNoSignal(&button, active);
}
typedef ReferenceCaller1<GtkToggleToolButton, bool, toggleButtonSetActiveCallback> ToggleButtonSetActiveCaller;

namespace ui {

/*	Returns the toolbar that is named toolbarName
 */
GtkToolbar* ToolbarCreator::getToolbar(const std::string& toolbarName) {
	return _toolbars[toolbarName];
}

/* Checks the passed xmlNode for a recognized item (ToolButton, ToggleToolButton, Separator)
 * Returns the widget or NULL if nothing useful is found   
 */
GtkWidget* ToolbarCreator::createToolItem(xml::Node& node, GtkToolbar* toolbar) {
	const std::string nodeName = node.getName();
	GtkWidget* toolItem;
	
	if (nodeName == "separator") {
		toolItem = GTK_WIDGET(gtk_separator_tool_item_new());			
	}
	else if (nodeName == "toolbutton" || nodeName == "toggletoolbutton") {
		// Found a button, load the values that are shared by both types
		const std::string name 		= node.getAttributeValue("name");
		const std::string icon 		= node.getAttributeValue("icon");
		const std::string tooltip 	= node.getAttributeValue("tooltip");
		const std::string action 	= node.getAttributeValue("action");
		
		if (nodeName == "toolbutton") {
			// Create a new GtkToolButton and assign the right callback
			toolItem = GTK_WIDGET(gtk_tool_button_new(NULL, name.c_str()));
						
			const Callback cb = GlobalCommands_find(action.c_str()).m_callback;
			g_signal_connect_swapped(G_OBJECT(toolItem), "clicked", G_CALLBACK(cb.getThunk()), cb.getEnvironment());
		}
		else {
			// Create a new GtkToggleToolButton and assign the right callback
			toolItem = GTK_WIDGET(gtk_toggle_tool_button_new());
			
			const Toggle toggle = GlobalToggles_find(action.c_str());
			const Callback cb = toggle.m_command.m_callback;
			guint handler = g_signal_connect_swapped(G_OBJECT(toolItem), "toggled", 
													 G_CALLBACK(cb.getThunk()), cb.getEnvironment());
			g_object_set_data(G_OBJECT(toolItem), "handler", gint_to_pointer(handler));
			
			GtkToggleToolButton* toggleToolButton = GTK_TOGGLE_TOOL_BUTTON(toolItem);
							
			toggle.m_exportCallback(ToggleButtonSetActiveCaller(*toggleToolButton));				
		}
		
		// Set the tooltip, if not empty
		if (tooltip != "") {
			gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolItem), _tooltips, tooltip.c_str(), "");
		}
		
		// Load and assign the icon, if specified
		if (icon != "") {
			GtkWidget* image = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(icon.c_str()));
			gtk_widget_show(image);
			gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(toolItem), image);
		}
	}
	else {
		return NULL;
	}
			
	gtk_widget_show(toolItem);
	return toolItem;
}

/*	Creates a toolbar based on the data found in the passed xmlNode
 * 	Returns the fully populated GtkToolbar 	
 */
GtkToolbar* ToolbarCreator::createToolbar(xml::Node& node) {
	// Get all action children elements 
	xml::NodeList toolItemList = node.getChildren();
	GtkWidget* toolbar;
		
	if (toolItemList.size() > 0) {
		// Create a new toolbar
		toolbar = gtk_toolbar_new();
		gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
		
		for (unsigned int i = 0; i < toolItemList.size(); i++) {
			// Create and get the toolItem with the parsing 
			GtkWidget* toolItem = createToolItem(toolItemList[i], GTK_TOOLBAR(toolbar));
			
			// It is possible that no toolItem is returned, only add it if it's safe to do so
			if (toolItem != NULL) {    				
				gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolItem), -1);    				
			}
		}
	}
	else {
		throw xml::AttributeNotFoundException("No elements in toolbar.");
	}
	
	return GTK_TOOLBAR(toolbar);
}

/* Parses the XML Document for toolbars and instantiates them
 * Returns nothing, toolbars can be obtained via GetToolbar()
 */
void ToolbarCreator::parseXml(xml::Document& xmlDoc) {
	xml::NodeList toolbarList = xmlDoc.findXPath("/ui//toolbar");
	
	if (toolbarList.size() > 0) {
		// Create a new tooltips element
		_tooltips = gtk_tooltips_new();
		gtk_tooltips_enable(_tooltips);
		
		for (unsigned int i = 0; i < toolbarList.size(); i++) {
			std::string toolbarName = toolbarList[i].getAttributeValue("name");
			
			globalOutputStream() << "Found toolbar: " << toolbarName.c_str();
			globalOutputStream() << "\n";
			
			_toolbars[toolbarName] = createToolbar(toolbarList[i]);
		}
	}
	else {
		throw xml::AttributeNotFoundException("No toolbars found.");
	}
}


/* Constructor: Load the definitions from the specified XML file
 */
ToolbarCreator::ToolbarCreator(const std::string& uiXmlPath, const std::string& uiXmlFile):
	_uiXmlPath(uiXmlPath),
	_uiXmlFile(uiXmlFile)
 {
	const std::string xmlFile = _uiXmlPath + _uiXmlFile;
	
	xmlDocPtr pXmlDoc = xmlParseFile(xmlFile.c_str());
	
	if (pXmlDoc) {
		globalOutputStream() << "Loading toolbar information from " << xmlFile.c_str() << "\n";
		
		try {
			// Create an xml::Document class and pass it to the parser 
			xml::Document xmlDoc(pXmlDoc);
			parseXml(xmlDoc);
		}
		catch (xml::AttributeNotFoundException e) {
			globalOutputStream() << "Warning in " << xmlFile.c_str() << ": " << e.what() << "\n";
		}
	
		globalOutputStream() << "Finished loading toolbar information.\n";
	}
	else {
		globalOutputStream() << "Could not open file: " << xmlFile.c_str() << "\n";
	}
}

} // namespace toolbar
