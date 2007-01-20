#ifndef COMMANDLISTPOPULATOR_H_
#define COMMANDLISTPOPULATOR_H_

#include "ieventmanager.h"

/* greebo: The CommandListPopulator is an Event visitor class that cycles
 * through all the registered events and stores the name and the associated
 * shortcut representation into the given GtkListStore widget.
 */

namespace ui {

class CommandListPopulator :
	public IEventVisitor 
{
	// The list store the items should be added to
	GtkListStore* _listStore;
	
public:
	CommandListPopulator(GtkListStore* listStore) : 
		_listStore(listStore)
	{}

	void visit(const std::string& eventName, IEventPtr event) {
		GtkTreeIter iter;
		
		// Allocate a new list store element and store its pointer into <iter>
		gtk_list_store_append(_listStore, &iter);
		
		const std::string accelerator = GlobalEventManager().getAcceleratorStr(event, true);
		
		gtk_list_store_set(_listStore, &iter, 0, eventName.c_str(), 
											  1, accelerator.c_str(), -1);
	}
	
}; // class CommandListPopulator
	
} // namespace ui

#endif /*COMMANDLISTPOPULATOR_H_*/
