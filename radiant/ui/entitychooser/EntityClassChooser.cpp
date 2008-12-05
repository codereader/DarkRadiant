#include "EntityClassChooser.h"
#include "EntityClassTreePopulator.h"

#include "mainframe.h"
#include "ieclass.h"
#include "iregistry.h"
#include "gtkutil/dialog.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/IconTextColumn.h"
#include "string/string.h"

#include "entity.h" // Entity_createFromSelection()

namespace ui
{

// Display the singleton instance
std::string EntityClassChooser::chooseEntityClass() {
	static EntityClassChooser instance;
	return instance.showAndBlock();
}

// Show the dialog

std::string EntityClassChooser::showAndBlock() {
	// Show the widget and set keyboard focus to the tree view
	gtk_widget_show_all(_widget);
	gtk_widget_grab_focus(_treeView);

	_modelPreview.initialisePreview();

	// Update the member variables
	updateSelection();
	
	// Enter recursive main loop
	gtk_main();
	
	// Return the last selection (may be "" if dialog was cancelled)
	return _selectedName;
}

// Constructor. Creates GTK widgets.

EntityClassChooser::EntityClassChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(NULL),
  _selection(NULL),
  _okButton(NULL),
  _selectedName("")
{
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), ECLASS_CHOOSER_TITLE);

	// Set the default size of the window
	
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	gtk_window_set_default_size(
		GTK_WINDOW(_widget), gint(w * 0.7f), gint(h * 0.6f)
	);

	_modelPreview.setSize(gint(w*0.3f));

	// Create GUI elements and pack into main VBox
	
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createUsagePanel(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtonPanel(), FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);

	gtk_box_pack_start(GTK_BOX(hbox), vbx, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _modelPreview, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(_widget), hbox);

	// Signals
	g_signal_connect(_widget, "delete-event", G_CALLBACK(callbackHide), this);

}

// Create the tree view

GtkWidget* EntityClassChooser::createTreeView() {

	// Set up the TreeModel, and populate it with the list of entity
	// classes by using a visitor class.
	_treeStore = gtk_tree_store_new(N_COLUMNS, 
									G_TYPE_STRING,		// name
									GDK_TYPE_PIXBUF,	// icon
									G_TYPE_BOOLEAN);	// directory flag
	EntityClassTreePopulator visitor(_treeStore);
	GlobalEntityClassManager().forEach(visitor);
	
	// Construct the tree view widget with the now-populated model
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), TRUE);

	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	gtk_tree_selection_set_mode(_selection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelectionChanged), this);

	// Single column with icon and name
	GtkTreeViewColumn* col = 
		gtkutil::IconTextColumn("Classname", NAME_COLUMN, ICON_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, NAME_COLUMN);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), col);				

	// greebo: Trigger a sort operation by simulating a mouseclick on the column
	gtk_tree_view_column_clicked(col);

	// Pack treeview into a scrolled frame and return
	return gtkutil::ScrolledFrame(_treeView);
}

// Create the entity usage information panel
GtkWidget* EntityClassChooser::createUsagePanel() {

	// Create a GtkTextView
	_usageTextView = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_usageTextView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_usageTextView), FALSE);

	return gtkutil::ScrolledFrame(_usageTextView);	
}

// Create the button panel
GtkWidget* EntityClassChooser::createButtonPanel() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	_okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	
	g_signal_connect(
		G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this
	);
	g_signal_connect(
		G_OBJECT(_okButton), "clicked", G_CALLBACK(callbackOK), this
	);

	gtk_box_pack_end(GTK_BOX(hbx), _okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	return gtkutil::RightAlignment(hbx);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass) {

	// Lookup the IEntityClass instance
	IEntityClassPtr e = GlobalEntityClassManager().findOrInsert(eclass, true);	

	// Set the usage panel to the IEntityClass' usage information string
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_usageTextView));
	
	// Create the concatenated usage string
	std::string usage = "";
	EntityClassAttributeList usageAttrs = e->getAttributeList("editor_usage");
	for (EntityClassAttributeList::const_iterator i = usageAttrs.begin();
		 i != usageAttrs.end();
		 ++i)
	{
		// Add only explicit (non-inherited) usage strings
		if (!i->inherited) {
			if (!usage.empty())
				usage += std::string("\n") + i->value;
			else
				usage += i->value;
		}
	}
	
	gtk_text_buffer_set_text(buf, usage.c_str(), -1);
}

void EntityClassChooser::updateSelection() {
	// Prepare to check for a selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	// Add button is enabled if there is a selection and it is not a folder.
	if (gtk_tree_selection_get_selected(_selection, &model, &iter)
		&& !gtkutil::TreeModel::getBoolean(model, &iter, DIR_FLAG_COLUMN)) 
	{
		// Make the OK button active 
		gtk_widget_set_sensitive(_okButton, TRUE);

		// Set the panel text with the usage information
		std::string selName = gtkutil::TreeModel::getString(
			model, &iter, NAME_COLUMN
		); 
		updateUsageInfo(selName);

		// Lookup the IEntityClass instance
		IEntityClassPtr eclass = GlobalEntityClassManager().findClass(selName);	

		if (eclass != NULL) {
			_modelPreview.setModel(eclass->getAttribute("model").value);
			_modelPreview.setSkin(eclass->getAttribute("skin").value);
		}

		// Update the _selectionName field
		_selectedName = selName;
	}
	else {
		_modelPreview.setModel("");
		_modelPreview.setSkin("");

		gtk_widget_set_sensitive(_okButton, FALSE);
	}
}

/* GTK CALLBACKS */

gboolean EntityClassChooser::callbackHide(GtkWidget* widget, GdkEvent* ev, EntityClassChooser* self)
{
	// greebo: Clear the selected name on hide, we don't want to create another entity when 
	// the user clicks on the X in the upper right corner.
	self->_selectedName = "";

	gtk_widget_hide(self->_widget);
	gtk_main_quit();

	return TRUE; // don't propagate the event
}

void EntityClassChooser::callbackCancel(GtkWidget* widget, 
										EntityClassChooser* self) 
{
	self->_selectedName = "";
	gtk_widget_hide(self->_widget);
	gtk_main_quit();
}

void EntityClassChooser::callbackOK(GtkWidget* widget, EntityClassChooser* self) 
{
	gtk_widget_hide(self->_widget);
	gtk_main_quit();
}

void EntityClassChooser::callbackSelectionChanged(GtkWidget* widget, 
												  EntityClassChooser* self) 
{
	self->updateSelection();
}

} // namespace ui
