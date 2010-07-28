#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include <gtk/gtktoolitem.h>
#include <gtk/gtktoolbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcomboboxentry.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkstock.h>

#include "gtkutil/ComboBox.h"
#include "gtkutil/LeftAlignedLabel.h"

namespace selection
{

	namespace
	{
		const char* const ENTRY_TOOLTIP = N_("Enter a name and hit ENTER to save a set.\n\n"
			"Select an item from the dropdown list to restore the selection.\n\n"
			"Hold SHIFT when opening the dropdown list and selecting the item to de-select the set.");
	}

SelectionSetToolmenu::SelectionSetToolmenu() :
	_toolItem(gtk_tool_item_new()),
	_listStore(gtk_list_store_new(1, G_TYPE_STRING)),
	_clearSetsButton(NULL),
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

	// Add tooltip
	gtk_widget_set_tooltip_markup(_entry, _(ENTRY_TOOLTIP));

	// Add clear button
	{
		GtkWidget* image = gtk_image_new_from_pixbuf(GlobalUIManager().getLocalPixbufWithMask("delete.png")->gobj());
		gtk_widget_show(image);

		_clearSetsButton = gtk_tool_button_new(image, _("Clear Selection Sets"));
		
		// Set tooltip
		gtk_tool_item_set_tooltip_text(_clearSetsButton, _("Clear Selection Sets"));

		// Connect event
		g_signal_connect(G_OBJECT(_clearSetsButton), "clicked", G_CALLBACK(onDeleteAllSetsClicked), this);

		gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(_clearSetsButton), FALSE, FALSE, 0);
	}

	// Connect the signals
	GtkWidget* childEntry = gtk_bin_get_child(GTK_BIN(_entry));
	g_signal_connect(G_OBJECT(childEntry), "activate", G_CALLBACK(onEntryActivated), this); 

	g_signal_connect(G_OBJECT(_entry), "changed", G_CALLBACK(onSelectionChanged), this);

	// Populate the list
	update();

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
	update();
}

void SelectionSetToolmenu::update()
{
	// Clear all items from the treemodel first
	gtk_list_store_clear(_listStore);

	// Populate the list store with all available selection sets
	class Visitor :
		public ISelectionSetManager::Visitor
	{
	private:
		GtkListStore* _store;

		bool _hasItems;
	public:
		Visitor(GtkListStore* store) :
			_store(store),
			_hasItems(false)
		{}

		void visit(const ISelectionSetPtr& set)
		{
			_hasItems = true;

			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter, 
							   0, set->getName().c_str(),
							   -1);
		}

		bool foundItems() const
		{
			return _hasItems;
		}

	} visitor(_listStore);

	GlobalSelectionSetManager().foreachSelectionSet(visitor);

	// Tool button is sensitive if we have items in the list
	gtk_widget_set_sensitive(GTK_WIDGET(_clearSetsButton), visitor.foundItems() ? TRUE : FALSE);
}

void SelectionSetToolmenu::onEntryActivated(GtkEntry* entry, 
											SelectionSetToolmenu* self)
{
	// Create new selection set if possible
	std::string name = gtk_entry_get_text(entry);

	if (name.empty()) return;

	// don't create empty sets
	if (GlobalSelectionSystem().countSelected() == 0) 
	{
		ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
			_("Cannot create selection set"), 
			_("Cannot create a selection set, there is nothing selected in the current scene."), 
			ui::IDialog::MESSAGE_CONFIRM);

		dialog->run();
		return; 
	}

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

void SelectionSetToolmenu::onDeleteAllSetsClicked(GtkToolButton* toolbutton, 
												  SelectionSetToolmenu* self) 
{
	ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
		_("Delete all selection sets?"), 
		_("This will delete all set definitions. The actual map objects will not be affected by this step.\n\nContinue with that operation?"), 
		ui::IDialog::MESSAGE_ASK);

	ui::IDialog::Result result = dialog->run();

	if (result == ui::IDialog::RESULT_YES)
	{
		GlobalSelectionSetManager().deleteAllSelectionSets();
	}
}

} // namespace
