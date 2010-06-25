#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include "ieventmanager.h"

#include <gtk/gtktoolitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcomboboxentry.h>
#include <gtk/gtkliststore.h>

#include "gtkutil/ComboBox.h"
#include "gtkutil/LeftAlignedLabel.h"

namespace selection
{

SelectionSetToolmenu::SelectionSetToolmenu() :
	_toolItem(gtk_tool_item_new()),
	_listStore(gtk_list_store_new(1, G_TYPE_STRING)),
	_entry(gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(_listStore), 0))
{
	// Hbox containing all our items
	GtkWidget* hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(_toolItem), hbox);

	// Pack Label
	gtk_box_pack_start(GTK_BOX(hbox), 
		gtkutil::LeftAlignedLabel(_("Selection Set: ")), FALSE, FALSE, 0);

	// Pack Combo Box
	gtk_box_pack_start(GTK_BOX(hbox), _entry, TRUE, TRUE, 0);

	// Connect the signals
	GtkWidget* childEntry = gtk_bin_get_child(GTK_BIN(_entry));
	g_signal_connect(G_OBJECT(childEntry), "activate", G_CALLBACK(onEntryActivated), this); 

	g_signal_connect(G_OBJECT(_entry), "changed", G_CALLBACK(onSelectionChanged), this);

	// Populate the list
	updateItems();

	// Add self as observer
	GlobalSelectionSetManager().addObserver(*this);
}

SelectionSetToolmenu::~SelectionSetToolmenu()
{
	GlobalSelectionSetManager().removeObserver(*this);
}

GtkToolItem* SelectionSetToolmenu::getToolItem()
{
	gtk_widget_show_all(GTK_WIDGET(_toolItem));
	return _toolItem;
}

void SelectionSetToolmenu::onSelectionSetsChanged()
{
	updateItems();
}

void SelectionSetToolmenu::updateItems()
{
	// Clear all items from the treemodel first
	gtk_list_store_clear(_listStore);

	// Populate the list store with all available selection sets
	class Visitor :
		public ISelectionSetManager::Visitor
	{
	private:
		GtkListStore* _store;
	public:
		Visitor(GtkListStore* store) :
			_store(store)
		{}

		void visit(const ISelectionSetPtr& set)
		{
			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter, 
							   0, set->getName().c_str(),
							   -1);
		}

	} visitor(_listStore);

	GlobalSelectionSetManager().foreachSelectionSet(visitor);
}

void SelectionSetToolmenu::onEntryActivated(GtkEntry* entry, 
											SelectionSetToolmenu* self)
{
	// Create new selection set if possible
	std::string name = gtk_entry_get_text(entry);

	if (name.empty()) return;

	ISelectionSetPtr set = GlobalSelectionSetManager().createSelectionSet(name);

	assert(set != NULL);

	set->assignFromCurrentScene();

	// Clear the entry again
	gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void SelectionSetToolmenu::onSelectionChanged(GtkComboBox* comboBox, 
											  SelectionSetToolmenu* self)
{
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(comboBox, &iter))
	{
		std::string name = gtkutil::ComboBox::getActiveText(comboBox);

		if (name.empty()) return;

		ISelectionSetPtr set = GlobalSelectionSetManager().findSelectionSet(name);

		if (set == NULL) return;

		// The user can choose to DESELECT the set nodes when holding down shift
		if ((GlobalEventManager().getModifierState() & GDK_SHIFT_MASK) != 0)
		{
			set->deselect();
		}
		else
		{
			set->select();
		}

		GtkWidget* childEntry = gtk_bin_get_child(GTK_BIN(self->_entry));
		gtk_entry_set_text(GTK_ENTRY(childEntry), "");
	}
}

} // namespace
