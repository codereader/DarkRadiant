#include "SelectionSetManager.h"

#include "itextstream.h"
#include "iradiant.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "modulesystem/StaticModule.h"

#include <gtk/gtktoolbar.h>
#include <gtk/gtkseparatortoolitem.h>

namespace selection
{

const std::string& SelectionSetManager::getName() const
{
	static std::string _name("SelectionSetManager");
	return _name;
}

const StringSet& SelectionSetManager::getDependencies() const
{
	static StringSet _dependencies;
	
	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_RADIANT);
	}
	
	return _dependencies;
}

void SelectionSetManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
	
	// Register for the startup event
	GlobalRadiant().addEventListener(shared_from_this());
}

void SelectionSetManager::shutdownModule()
{
	// nothing yet
}

void SelectionSetManager::onRadiantStartup()
{
	// Get the horizontal toolbar and add a custom widget
	GtkToolbar* toolbar = GlobalMainFrame().getToolbar(IMainFrame::TOOLBAR_HORIZONTAL);

	//GtkToolItem* item = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
	//gtk_toolbar_insert(toolbar, item, -1);

	gtk_widget_show(GTK_WIDGET(item));
}

// Define the static SelectionSetManager module
module::StaticModule<SelectionSetManager> selectionSetManager;

} // namespace
