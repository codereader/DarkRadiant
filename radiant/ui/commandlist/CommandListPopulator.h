#ifndef COMMANDLISTPOPULATOR_H_
#define COMMANDLISTPOPULATOR_H_

#include "ieventmanager.h"
#include "CommandList.h"

/* greebo: The CommandListPopulator is an Event visitor class that cycles
 * through all the registered events and stores the name and the associated
 * shortcut representation into the given GtkListStore widget.
 */
namespace ui
{

class CommandListPopulator :
	public IEventVisitor
{
	// The list store the items should be added to
	Glib::RefPtr<Gtk::ListStore> _listStore;

	const CommandList::Columns& _columns;

public:
	CommandListPopulator(const Glib::RefPtr<Gtk::ListStore>& listStore,
						 const CommandList::Columns& columns) :
		_listStore(listStore),
		_columns(columns)
	{}

	void visit(const std::string& eventName, const IEventPtr& ev)
	{
		// Allocate a new list store element
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.command] = eventName;
		row[_columns.key] = GlobalEventManager().getAcceleratorStr(ev, true);
	}

}; // class CommandListPopulator

} // namespace ui

#endif /*COMMANDLISTPOPULATOR_H_*/
