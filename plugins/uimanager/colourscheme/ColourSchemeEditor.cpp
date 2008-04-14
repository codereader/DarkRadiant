#include "ColourSchemeEditor.h"
#include "ColourSchemeManager.h"
#include "iregistry.h"
#include "iradiant.h"
#include "ibrush.h"
#include "iscenegraph.h"

#include <gtk/gtk.h>

#include "gtkutil/TreeModel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog.h"

namespace ui {

	namespace {
		const unsigned int GDK_FULL_INTENSITY = 65535;
	}

ColourSchemeEditor::ColourSchemeEditor() :
	BlockingTransientWindow(EDITOR_WINDOW_TITLE, GlobalRadiant().getMainWindow())
{	
    gtk_window_set_position(GTK_WINDOW(getWindow()), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(getWindow()), EDITOR_DEFAULT_SIZE_X, EDITOR_DEFAULT_SIZE_Y);
	
	// Get the constructed windowframe and pack it into the editor widget
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), constructWindow());
	
	// Load all the list items
  	populateTree();
  	
  	// Connect the selection callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
  	
	// Highlight the currently selected scheme
	selectActiveScheme();
	updateColourSelectors();
	
	// Connect the signal AFTER selecting the active scheme	
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this);
	
	// Be sure that everything is properly destroyed upon window closure
	g_signal_connect(G_OBJECT(getWindow()), 
					 "delete_event", 
					 G_CALLBACK(_onDeleteEvent),
					 this);
}	
	
/*	Loads all the scheme items into the list
 */
void ColourSchemeEditor::populateTree() {
	GtkTreeIter iter;
  
	ColourSchemeMap allSchemes = ColourSchemeManager::Instance().getSchemeList();
  
	for (ColourSchemeMap::iterator scheme = allSchemes.begin(); 
		 scheme != allSchemes.end(); scheme++)
	{
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 0, scheme->first.c_str(), -1);
	}
}

void ColourSchemeEditor::createTreeView() {
	// Create the listStore
	_listStore = gtk_list_store_new(1, G_TYPE_STRING);
	
	// Create the treeView
    _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
	
	// Unreference the list store so it will be destroyed with the treeview 
    g_object_unref(G_OBJECT(_listStore));
	
	// Create a new column and set its parameters  
 	GtkTreeViewColumn* col = gtk_tree_view_column_new();
  
  	// Pack the new column into the treeView
  	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), col);
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);

  	// Create a new text renderer and attach it to the column
  	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  	gtk_tree_view_column_pack_start(col, renderer, TRUE);
  	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
}

GtkWidget* ColourSchemeEditor::constructButtons() {
	
	// Create the buttons and put them into a horizontal box
	GtkWidget* buttonBox = gtk_hbox_new(TRUE, 12);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	gtk_box_pack_end(GTK_BOX(buttonBox), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(buttonBox), cancelButton, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
	
	return gtkutil::RightAlignment(buttonBox);
}

// Construct buttons underneath the list box
GtkWidget* ColourSchemeEditor::constructTreeviewButtons() {
	GtkWidget* buttonBox = gtk_hbox_new(TRUE, 6);
	
	_deleteButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	GtkWidget* copyButton = gtk_button_new_from_stock(GTK_STOCK_COPY);
	
	gtk_box_pack_start(GTK_BOX(buttonBox), copyButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), _deleteButton, TRUE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(copyButton), "clicked", G_CALLBACK(callbackCopy), this);
	g_signal_connect(G_OBJECT(_deleteButton), "clicked", G_CALLBACK(callbackDelete), this);
	
	return buttonBox;
}

GtkWidget* ColourSchemeEditor::constructWindow() {
	// The vbox that separates the buttons and the upper part of the window
	GtkWidget* vbox = gtk_vbox_new(FALSE, 12);

	// Place the buttons at the bottom of the window
	gtk_box_pack_end(GTK_BOX(vbox), constructButtons(), FALSE, FALSE, 0);

	// This is the box for the treeview and the whole rest
	GtkWidget* hbox = gtk_hbox_new(FALSE, 12);

	// VBox containing the tree view and copy/delete buttons underneath
	GtkWidget* treeAndButtons = gtk_vbox_new(FALSE, 6);

	// Create the treeview and pack it into the treeViewFrame
	createTreeView();
	gtk_box_pack_start(GTK_BOX(treeAndButtons),	
			gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(treeAndButtons), 
			constructTreeviewButtons(),	FALSE, FALSE, 0);

	// Pack the treeViewFrame into the hbox
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(treeAndButtons), FALSE, FALSE, 0);

	// The Box containing the Colour, pack it into the right half of the hbox
	_colourFrame = gtk_frame_new(NULL);
	_colourBox = gtk_vbox_new(FALSE, 5);
	
	gtk_container_add(GTK_CONTAINER(_colourFrame), _colourBox);

	gtk_box_pack_start(GTK_BOX(hbox), _colourFrame, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	
	return vbox;
}

void ColourSchemeEditor::selectActiveScheme() {
	GtkTreeIter iter;
		
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(_listStore), &iter)) {
		
		do {
			// Get the value
			GValue val = {0, 0};
			gtk_tree_model_get_value(GTK_TREE_MODEL(_listStore), &iter, 0, &val);
			// Get the string
			std::string name = g_value_get_string(&val);
			
			if (ColourSchemeManager::Instance().isActive(name)) {
				gtk_tree_selection_select_iter(_selection, &iter);
				
				// Set the button sensitivity correctly for read-only schemes
				gtk_widget_set_sensitive(
					_deleteButton, 
					!ColourSchemeManager::Instance().getScheme(name).isReadOnly()
				);
				
				return;
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(_listStore), &iter));
	}
}

void ColourSchemeEditor::deleteSchemeFromList() {
	GtkTreeIter iter;
	GtkTreeModel* model;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
	
	// Select the first element
	gtk_tree_model_get_iter_first(model, &iter);
	gtk_tree_selection_select_iter(_selection, &iter);
}

std::string ColourSchemeEditor::getSelectedScheme() {
	GtkTreeIter iter;
	GtkTreeModel* model;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		// Acquire the value from the column and return
		return gtkutil::TreeModel::getString(model, &iter, 0);	
	}
	else {
		return "";
	}
}

GtkWidget* ColourSchemeEditor::constructColourSelector(ColourItem& colour, const std::string& name) {
	// Get the description of this colour item from the registry
	std::string descriptionPath = std::string("user/ui/colourschemes/description/") + name;
	std::string description = GlobalRegistry().get(descriptionPath);
	
	// Create a new horizontal divider
	GtkWidget* hbox = gtk_hbox_new(FALSE, 10);
	
	  GdkColor tempColour;
	  Vector3 tempColourVector = colour;
	  tempColour.red 	= static_cast<unsigned int>(GDK_FULL_INTENSITY * tempColourVector[0]);
	  tempColour.green 	= static_cast<unsigned int>(GDK_FULL_INTENSITY * tempColourVector[1]);
	  tempColour.blue 	= static_cast<unsigned int>(GDK_FULL_INTENSITY * tempColourVector[2]);
	
	  // Create the colour button 
	  GtkWidget* button = gtk_color_button_new_with_color(&tempColour);
	  gtk_color_button_set_title(GTK_COLOR_BUTTON(button), description.c_str());
	  ColourItem* colourPtr = &colour;
		
	  // Connect the signal, so that the ColourItem class is updated along with the colour button
	  g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(callbackColorChanged), colourPtr);
	  gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	  GtkWidget* label = gtk_label_new(description.c_str());
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	  
	return hbox;
}

void ColourSchemeEditor::updateColourSelectors() {
	gtk_widget_hide(_colourBox);
	gtk_widget_destroy(_colourBox);
	
	GtkWidget* curVbox = gtk_vbox_new(FALSE, 5); 
	
	// Create the column container
	_colourBox = gtk_hbox_new(FALSE, 12);
		
	gtk_container_add(GTK_CONTAINER(_colourFrame), _colourBox);
		
	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme( getSelectedScheme() );
	
	// Retrieve the list with all the ColourItems of this scheme
	ColourItemMap& colourMap = scheme.getColourMap();
	
	ColourItemMap::iterator it;
	unsigned int i = 1;
	// Cycle through all the ColourItems and save them into the registry	
	for (it = colourMap.begin(), i = 1; 
		 it != colourMap.end();
		 it++, i++) 
	{
		GtkWidget* colourSelector = constructColourSelector(it->second, it->first);
		gtk_box_pack_start(GTK_BOX(curVbox), colourSelector, FALSE, FALSE, 5);
		
		// Have we reached the maximum number of colours per column?
		if (i % COLOURS_PER_COLUMN == 0) {
			// yes, pack the current column into the _colourBox and create a new vbox
			gtk_box_pack_start(GTK_BOX(_colourBox), curVbox, FALSE, FALSE, 5);
			curVbox = gtk_vbox_new(FALSE, 5);
		}
	}
	
	// Pack the remaining items into the last column
	gtk_box_pack_start(GTK_BOX(_colourBox), curVbox, FALSE, FALSE, 0);
	
	gtk_widget_show_all(_colourBox);	
}

void ColourSchemeEditor::updateWindows() {
	// Call the update, so all colours can be previewed
	GlobalRadiant().updateAllWindows();
	GlobalBrushCreator().clipperColourChanged();
	SceneChangeNotify();
}

void ColourSchemeEditor::selectionChanged() {
	std::string activeScheme = getSelectedScheme();
	
	// Update the colour selectors to reflect the newly selected scheme
	updateColourSelectors();
	
	// Check, if the currently selected scheme is read-only
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(activeScheme);
	gtk_widget_set_sensitive(_deleteButton, (!scheme.isReadOnly()));
	
	// Set the active Scheme, so that the views are updated accordingly
	ColourSchemeManager::Instance().setActive(activeScheme);
	
	updateWindows();
}

void ColourSchemeEditor::deleteScheme() {
	std::string name = getSelectedScheme();
	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(name);
	
	if (!scheme.isReadOnly()) {
		// Remove the actual scheme from the ColourSchemeManager 
		ColourSchemeManager::Instance().deleteScheme(name);
		
		// Remove the selected item from the GtkListStore
		deleteSchemeFromList();
	}
}

std::string ColourSchemeEditor::inputDialog(const std::string& title, const std::string& label) {
	GtkWidget* dialog;
	GtkWidget* labelWidget;
	GtkWidget* entryWidget;
    std::string returnValue;
    
  	dialog = gtk_dialog_new_with_buttons(title.c_str(), GTK_WINDOW(getWindow()),
                                         GTK_DIALOG_DESTROY_WITH_PARENT, 
                                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);
                                         
	labelWidget = gtk_label_new(label.c_str());
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), labelWidget);
	
	entryWidget = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entryWidget);
	
	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	
	if (response == GTK_RESPONSE_OK) {
		returnValue = gtk_entry_get_text(GTK_ENTRY(entryWidget));
	}
	else {
		returnValue = "";
	}
	
	gtk_widget_destroy(dialog);
	
	return returnValue;
}

void ColourSchemeEditor::copyScheme() {
	GtkTreeIter iter;
	std::string name = getSelectedScheme();
	std::string newName = inputDialog("Copy Colour Scheme", "Enter a name for the new scheme:");
	
	if (newName.empty()) {
		return; // empty name
	}

	// greebo: Check if the new name is already existing
	if (ColourSchemeManager::Instance().schemeExists(newName)) {
		gtkutil::errorDialog("A Scheme with that name already exists.", GTK_WINDOW(getWindow()));
		return;
	}

	// Copy the scheme
	ColourSchemeManager::Instance().copyScheme(name, newName);
	ColourSchemeManager::Instance().setActive(newName);
	
	// Add the new list item to the ListStore
	gtk_list_store_append(_listStore, &iter);
	gtk_list_store_set(_listStore, &iter, 0, newName.c_str(), -1);
	
	// Highlight the copied scheme
	selectActiveScheme();
}

// GTK Callback Routines

void ColourSchemeEditor::callbackCopy(GtkWidget* widget, ColourSchemeEditor* self) {
	self->copyScheme();
}

void ColourSchemeEditor::callbackDelete(GtkWidget* widget, ColourSchemeEditor* self) {
	self->deleteScheme();
}

void ColourSchemeEditor::callbackColorChanged(GtkWidget* widget, ColourItem* colourItem) {
	GdkColor colour;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &colour);
	
	// Update the colourItem class
	colourItem->set(float(colour.red) / GDK_FULL_INTENSITY, 
					float(colour.green) / GDK_FULL_INTENSITY, 
					float(colour.blue) / GDK_FULL_INTENSITY);
	
	// Call the update, so all colours can be previewed
	updateWindows();
}

// This is called when the colourscheme selection is changed
void ColourSchemeEditor::callbackSelChanged(GtkWidget* widget, ColourSchemeEditor* self) {
	self->selectionChanged();
}

void ColourSchemeEditor::callbackOK(GtkWidget* widget, ColourSchemeEditor* self) {
	ColourSchemeManager::Instance().setActive(self->getSelectedScheme());
	ColourSchemeManager::Instance().saveColourSchemes();
	
	self->destroy();
}

// Destroy self function
void ColourSchemeEditor::doCancel() {
	// Restore all the colour settings from the XMLRegistry, changes get lost
	ColourSchemeManager::Instance().restoreColourSchemes();
	
	// Call the update, so all restored colours are displayed
	updateWindows();

	destroy();
}

// Cancel button callback
void ColourSchemeEditor::callbackCancel(GtkWidget* widget, ColourSchemeEditor* self) {
	self->doCancel();
}

// Window destroy callback
void ColourSchemeEditor::_onDeleteEvent(GtkWidget* w, 
										GdkEvent* e, 
										ColourSchemeEditor* self)
{
	self->doCancel();
}

void ColourSchemeEditor::editColourSchemes() {
	 ColourSchemeEditor editor;
	 editor.show(); // enter GTK main loop
}


} // namespace ui

