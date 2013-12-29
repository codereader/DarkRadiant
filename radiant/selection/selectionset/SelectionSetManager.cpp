#include "SelectionSetManager.h"

#include "itextstream.h"
#include "iradiant.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "modulesystem/StaticModule.h"
#include "SelectionSetToolmenu.h"

#include <gtkmm/toolbar.h>
#include <gtkmm/separatortoolitem.h>

#include <boost/bind.hpp>

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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_RADIANT);
	}

	return _dependencies;
}

void SelectionSetManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Register for the startup event
	GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &SelectionSetManager::onRadiantStartup)
    );

	GlobalCommandSystem().addCommand("DeleteAllSelectionSets",
		boost::bind(&SelectionSetManager::deleteAllSelectionSets, this, _1));

	GlobalEventManager().addCommand("DeleteAllSelectionSets", "DeleteAllSelectionSets");
}

void SelectionSetManager::shutdownModule()
{
	_selectionSets.clear();
}

void SelectionSetManager::onRadiantStartup()
{
	// Get the horizontal toolbar and add a custom widget
	Gtk::Toolbar* toolbar = GlobalMainFrame().getToolbar(IMainFrame::TOOLBAR_HORIZONTAL);

	// Insert a separator at the end of the toolbar
	Gtk::ToolItem* item = Gtk::manage(new Gtk::SeparatorToolItem);
	toolbar->insert(*item, -1);

	item->show();

	// Construct a new tool menu object
	SelectionSetToolmenu* toolmenu = Gtk::manage(new SelectionSetToolmenu);

	toolbar->insert(*toolmenu, -1);
}

void SelectionSetManager::addObserver(Observer& observer)
{
	_observers.insert(&observer);
}

void SelectionSetManager::removeObserver(Observer& observer)
{
	_observers.erase(&observer);
}

void SelectionSetManager::notifyObservers()
{
	for (Observers::iterator i = _observers.begin(); i != _observers.end(); )
	{
		(*i++)->onSelectionSetsChanged();
	}
}

void SelectionSetManager::foreachSelectionSet(const VisitorFunc& functor)
{
	for (SelectionSets::const_iterator i = _selectionSets.begin(); i != _selectionSets.end(); )
	{
		functor((i++)->second);
	}
}

void SelectionSetManager::foreachSelectionSet(Visitor& visitor)
{
	foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		visitor.visit(set);
	});
}

ISelectionSetPtr SelectionSetManager::createSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	if (i == _selectionSets.end())
	{
		// Create new set
		std::pair<SelectionSets::iterator, bool> result = _selectionSets.insert(
			SelectionSets::value_type(name, SelectionSetPtr(new SelectionSet(name))));

		i = result.first;

		notifyObservers();
	}

	return i->second;
}

void SelectionSetManager::deleteSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	if (i != _selectionSets.end())
	{
		_selectionSets.erase(i);

		notifyObservers();
	}
}

void SelectionSetManager::deleteAllSelectionSets()
{
	_selectionSets.clear();
	notifyObservers();
}

void SelectionSetManager::deleteAllSelectionSets(const cmd::ArgumentList& args)
{
	deleteAllSelectionSets();
}

ISelectionSetPtr SelectionSetManager::findSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	return (i != _selectionSets.end()) ? i->second : ISelectionSetPtr();
}

// Define the static SelectionSetManager module
module::StaticModule<SelectionSetManager> selectionSetManager;

} // namespace
