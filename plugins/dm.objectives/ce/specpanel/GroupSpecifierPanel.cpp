#include "GroupSpecifierPanel.h"

#include <gtkmm/entrycompletion.h>

namespace objectives
{

namespace ce
{

// Reg helper
GroupSpecifierPanel::RegHelper GroupSpecifierPanel::_regHelper;

GroupSpecifierPanel::GroupSpecifierPanel() :
	_listStore(Gtk::ListStore::create(_columns))
{
	// Set up the auto-completion
	Glib::RefPtr<Gtk::EntryCompletion> completion = Gtk::EntryCompletion::create();

	// Create the liststore and associate it with the entrycompletion object
	completion->set_model(_listStore);

	// The first column of the liststore is the one we use for auto-completion
	completion->set_text_column(_columns.name);

	// Associate the auto-completion object with the text entry
	set_completion(completion);
}

void GroupSpecifierPanel::populateCompletionListStore()
{
	// Insert the default items into the list
	Gtk::TreeModel::Row row = *_listStore->append();

	row[_columns.name] = "loot_total";

	row = *_listStore->append();
	row[_columns.name] = "loot_gold";

	row = *_listStore->append();
	row[_columns.name] = "loot_jewels";

	row = *_listStore->append();
	row[_columns.name] = "loot_goods";
}

} // namespace ce

} // namespace objectives
