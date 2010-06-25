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
	_selectionSets.clear();
}

void SelectionSetManager::onRadiantStartup()
{
	// Get the horizontal toolbar and add a custom widget
	GtkToolbar* toolbar = GlobalMainFrame().getToolbar(IMainFrame::TOOLBAR_HORIZONTAL);

	// Insert a separator at the end of the toolbar
	GtkToolItem* item = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
	gtk_toolbar_insert(toolbar, item, -1);

	gtk_widget_show(GTK_WIDGET(item));

	// Construct a new tool menu object
	_toolmenu = SelectionSetToolmenuPtr(new SelectionSetToolmenu);

	gtk_toolbar_insert(toolbar, _toolmenu->getToolItem(), -1);	
}

ISelectionSetPtr SelectionSetManager::createSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	if (i == _selectionSets.end())
	{
		// Create new set
		std::pair<SelectionSets::iterator, bool> result = _selectionSets.insert(
			SelectionSets::value_type(name, SelectionSetPtr(new SelectionSet)));

		i = result.first;
	}

	return i->second;
}

void SelectionSetManager::deleteSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	if (i != _selectionSets.end())
	{
		_selectionSets.erase(i);
	}
}

ISelectionSetPtr SelectionSetManager::findSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	return (i != _selectionSets.end()) ? i->second : ISelectionSetPtr();
}

// Define the static SelectionSetManager module
module::StaticModule<SelectionSetManager> selectionSetManager;

} // namespace
