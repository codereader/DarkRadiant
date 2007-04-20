#include "CustomStimEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/StockIconMenuItem.h"
#include "string/string.h"

namespace ui {

	namespace {
		const unsigned int TREE_VIEW_WIDTH = 250;
		const unsigned int TREE_VIEW_HEIGHT = 200;
	}

/** greebo: Constructor creates all the widgets
 */
CustomStimEditor::CustomStimEditor(StimTypes& stimTypes) :
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	populatePage();
	
	// Setup the context menu items and connect them to the callbacks
	createContextMenu();
}

CustomStimEditor::operator GtkWidget*() {
	gtk_widget_show_all(_pageHBox);
	return _pageHBox;
}

void CustomStimEditor::setEntity(SREntityPtr entity) {
	_entity = entity;
}

/** greebo: As the name states, this creates the context menu widgets.
 */
void CustomStimEditor::createContextMenu() {
	// Menu widgets
	_contextMenu.menu = gtk_menu_new();
		
	// Each menu gets a delete item
	_contextMenu.remove = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
														   "Delete Stim Type");
	_contextMenu.add = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
														   "Add Stim Type");
	
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu),
						  _contextMenu.add);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu.menu), 
						  _contextMenu.remove);

	// Connect up the signals
	g_signal_connect(G_OBJECT(_contextMenu.remove), "activate",
					 G_CALLBACK(onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_contextMenu.add), "activate",
					 G_CALLBACK(onContextMenuAdd), this);
	
	// Show menus (not actually visible until popped up)
	gtk_widget_show_all(_contextMenu.menu);
}

/** greebo: Creates all the widgets
 */
void CustomStimEditor::populatePage() {
	_pageHBox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(_pageHBox), 6);
	
	_list = gtk_tree_view_new();
	gtk_widget_set_size_request(_list, TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);
	
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_list));

	GtkListStore* listStore = _stimTypes;
	
	// Setup a treemodel filter to display the custom stims only
	_customStimStore = gtk_tree_model_filter_new(GTK_TREE_MODEL(listStore), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(_customStimStore), ST_CUSTOM_COL);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(_list), _customStimStore);
	g_object_unref(_customStimStore); // treeview owns reference now

	// Connect the signals to the callbacks
	g_signal_connect(G_OBJECT(_selection), "changed", 
					 G_CALLBACK(onSelectionChange), this);
	/*g_signal_connect(G_OBJECT(_list), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);*/
	g_signal_connect(G_OBJECT(_list), "button-release-event", 
					 G_CALLBACK(onTreeViewButtonRelease), this);
					 
	// Add the columns to the treeview
	// ID number
	GtkTreeViewColumn* numCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(numCol, "ID");
	GtkCellRenderer* numRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(numCol, numRenderer, FALSE);
	gtk_tree_view_column_set_attributes(numCol, numRenderer, 
										"text", ST_ID_COL,
										NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), numCol);
	
	// The Type
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(typeCol, "Type");
	
	GtkCellRenderer* typeIconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(typeCol, typeIconRenderer, FALSE);
	
	GtkCellRenderer* typeTextRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(typeCol, typeTextRenderer, FALSE);
	
	gtk_tree_view_column_set_attributes(typeCol, typeTextRenderer, 
										"text", ST_CAPTION_COL,
										NULL);
	gtk_tree_view_column_set_attributes(typeCol, typeIconRenderer, 
										"pixbuf", ST_ICON_COL,
										NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), typeCol);
	
	GtkWidget* listVBox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(listVBox), gtkutil::ScrolledFrame(_list), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(listVBox), createListButtons(), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(_pageHBox), listVBox, FALSE, FALSE, 0);
	
	_propertyWidgets.vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_pageHBox), _propertyWidgets.vbox, TRUE, TRUE, 0);
	
	// The name widgets
	GtkWidget* nameHBox = gtk_hbox_new(FALSE, 6);
	_propertyWidgets.nameLabel = gtk_label_new("Name:");
	_propertyWidgets.nameEntry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(nameHBox), _propertyWidgets.nameLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(nameHBox), _propertyWidgets.nameEntry, TRUE, TRUE, 0);
	
	// Connect the entry field
	g_signal_connect(G_OBJECT(_propertyWidgets.nameEntry), "changed", G_CALLBACK(onEntryChanged), this);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), nameHBox, FALSE, FALSE, 0);
	
	GtkWidget* infoText = gtkutil::LeftAlignedLabel(
		"<b>Note:</b> Please beware that deleting custom stims may\n"
		"affect other entities as well. So check before you delete." 
	);
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), infoText, FALSE, FALSE, 0);
	
	update();
}

GtkWidget* CustomStimEditor::createListButtons() {
	GtkWidget* hbox = gtk_hbox_new(TRUE, 6);
	
	_listButtons.add = gtk_button_new_with_label("Add Stim Type");
	gtk_button_set_image(
		GTK_BUTTON(_listButtons.add), 
		gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON)
	);
	
	_listButtons.remove = gtk_button_new_with_label("Remove Stim Type");
	gtk_button_set_image(
		GTK_BUTTON(_listButtons.remove), 
		gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON)
	);
	
	gtk_box_pack_start(GTK_BOX(hbox), _listButtons.add, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _listButtons.remove, TRUE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(_listButtons.add), "clicked", G_CALLBACK(onAddStimType), this);
	g_signal_connect(G_OBJECT(_listButtons.remove), "clicked", G_CALLBACK(onRemoveStimType), this);
	
	return hbox; 
}

void CustomStimEditor::entryChanged(GtkEditable* editable) {
	if (editable = GTK_EDITABLE(_propertyWidgets.nameEntry)) {
		// Set the caption of the curently selected stim type
		std::string caption = gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.nameEntry));
		// Pass the call to the helper class 
		_stimTypes.setStimTypeCaption(getIdFromSelection(), caption);
		
		if (_entity != NULL) {
			_entity->updateListStores();
		}
	}
}

void CustomStimEditor::update() {
	_updatesDisabled = true;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		gtk_widget_set_sensitive(_propertyWidgets.vbox, TRUE);
		
		StimType stimType = _stimTypes.get(id);
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.nameEntry), 
			stimType.caption.c_str()
		);
		
		gtk_widget_set_sensitive(_contextMenu.remove, TRUE);
	}
	else {
		gtk_widget_set_sensitive(_propertyWidgets.vbox, FALSE);
		gtk_widget_set_sensitive(_contextMenu.remove, FALSE);
	}
	
	_updatesDisabled = false;
}

void CustomStimEditor::selectId(int id) {
	// Setup the selectionfinder to search for the id
	gtkutil::TreeModel::SelectionFinder finder(id, ST_ID_COL);

	gtk_tree_model_foreach(
		_customStimStore,
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		// Set the active row of the list to the given effect
		gtk_tree_selection_select_iter(_selection, &iter);
	}
}

void CustomStimEditor::addStimType() {
	// Add a new stim type with the lowest free custom id
	int id = _stimTypes.getFreeCustomStimId();
	
	_stimTypes.add(id,
				   intToStr(id),
				   "CustomStimType",
				   "Custom Stim",
				   ICON_CUSTOM_STIM,
				   true);
	
	selectId(id);
}

int CustomStimEditor::getIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(_selection, &model, &iter);
	
	return (anythingSelected) ? gtkutil::TreeModel::getInt(model, &iter, ST_ID_COL) : -1;
}

void CustomStimEditor::removeStimType() {
	_stimTypes.remove(getIdFromSelection());
}

void CustomStimEditor::onAddStimType(GtkWidget* button, CustomStimEditor* self) {
	self->addStimType();
}

void CustomStimEditor::onRemoveStimType(GtkWidget* button, CustomStimEditor* self) {
	// Delete the selected stim type from the list
	self->removeStimType();
}

void CustomStimEditor::onEntryChanged(GtkEditable* editable, CustomStimEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->entryChanged(editable);
}

void CustomStimEditor::onSelectionChange(GtkTreeSelection* selection, CustomStimEditor* self) {
	self->update();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuDelete(GtkWidget* w, CustomStimEditor* self) {
	// Delete the selected stim from the list
	self->removeStimType();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuAdd(GtkWidget* w, CustomStimEditor* self) {
	self->addStimType();
}

gboolean CustomStimEditor::onTreeViewButtonRelease(
	GtkTreeView* view, GdkEventButton* ev, CustomStimEditor* self)
{
	// Single click with RMB (==> open context menu)
	if (ev->button == 3) {
		gtk_menu_popup(GTK_MENU(self->_contextMenu.menu), 
					   NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	
	return FALSE;
}
	
} // namespace ui
