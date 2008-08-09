#include "GroupSpecifierPanel.h"

#include <gtk/gtkentry.h>
#include <gtk/gtkentrycompletion.h>
#include <gtk/gtkliststore.h>

namespace objectives {

namespace ce {

// Reg helper
GroupSpecifierPanel::RegHelper GroupSpecifierPanel::_regHelper;

GroupSpecifierPanel::GroupSpecifierPanel() :
	_widget(gtk_entry_new())
{
	// Set up the auto-completion
	GtkEntryCompletion* completion = gtk_entry_completion_new();
		
	// Create the liststore and associate it with the entrycompletion object
	GtkListStore* listStore = createCompletionListStore();
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(listStore));
	g_object_unref(listStore);

	// The first column of the liststore is the one we use for auto-completion
	gtk_entry_completion_set_text_column(completion, 0);

	// Associate the auto-completion object with the text entry
	gtk_entry_set_completion(GTK_ENTRY(_widget), completion);
}

GroupSpecifierPanel::~GroupSpecifierPanel() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

GtkWidget* GroupSpecifierPanel::_getWidget() const {
	return _widget;
}

void GroupSpecifierPanel::setValue(const std::string& value) {
    gtk_entry_set_text(GTK_ENTRY(_widget), value.c_str());
}

std::string GroupSpecifierPanel::getValue() const {
    return std::string(gtk_entry_get_text(GTK_ENTRY(_widget)));
}

GtkListStore* GroupSpecifierPanel::createCompletionListStore() {
	// A new one-column liststore
	GtkListStore* listStore = gtk_list_store_new(1, G_TYPE_STRING);

	GtkTreeIter iter;
	
	// Insert the default items into the list
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 0, "loot_total", -1);

	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 0, "loot_gold", -1);

	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 0, "loot_jewels", -1);

	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 0, "loot_goods", -1);
	
	return listStore;
}

} // namespace ce

} // namespace objectives
