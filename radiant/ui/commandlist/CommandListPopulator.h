#pragma once

#include "ieventmanager.h"
#include "CommandList.h"
#include "wxutil/TreeModel.h"

/* greebo: The CommandListPopulator is an Event visitor class that cycles
 * through all the registered events and stores the name and the associated
 * shortcut representation into the given GtkListStore widget.
 */
namespace ui
{

class CommandListPopulator :
	public IEventVisitor
{
private:
	// The list store the items should be added to
	wxutil::TreeModel::Ptr _listStore;

	const CommandList::Columns& _columns;

public:
	CommandListPopulator(wxutil::TreeModel::Ptr listStore,
						 const CommandList::Columns& columns) :
		_listStore(listStore),
		_columns(columns)
	{}

	void visit(const std::string& eventName, const IEventPtr& ev)
	{
		// Allocate a new list store element
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.command] = eventName;
		row[_columns.key] = GlobalEventManager().getAcceleratorStr(ev, true);

		row.SendItemAdded();
	}

}; // class CommandListPopulator

} // namespace ui
