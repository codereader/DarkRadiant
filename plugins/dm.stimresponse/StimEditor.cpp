#include "StimEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/image.h"
#include "gtkutil/StockIconMenuItem.h"

#include "SREntity.h"

namespace ui {

	namespace {
		const unsigned int OPTIONS_LABEL_WIDTH = 140;
		
		static void textCellDataFunc(GtkTreeViewColumn* treeColumn,
							 GtkCellRenderer* cell,
							 GtkTreeModel* treeModel,
							 GtkTreeIter* iter, 
							 gpointer data)
		{
			bool inherited = gtkutil::TreeModel::getBoolean(treeModel, iter, INHERIT_COL);
			
			g_object_set(G_OBJECT(cell), "sensitive", !inherited, NULL);
		}
	}

StimEditor::StimEditor(StimTypes& stimTypes) :
	ClassEditor(stimTypes)
{
	populatePage();
	
	// Setup the context menu items and connect them to the callbacks
	createContextMenu();
}

void StimEditor::populatePage() {
	GtkWidget* srHBox = gtk_hbox_new(FALSE, 0);
	
	// Pack it into an alignment so that it is indented
	gtk_box_pack_start(GTK_BOX(_pageVBox), GTK_WIDGET(srHBox), TRUE, TRUE, 0);
	
	// ID number
	GtkTreeViewColumn* numCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(numCol, "#");
	GtkCellRenderer* numRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(numCol, numRenderer, FALSE);
	gtk_tree_view_column_set_attributes(numCol, numRenderer, 
										"text", INDEX_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(numCol, numRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), numCol);
	
	// The S/R icon
	GtkTreeViewColumn* classCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(classCol, "S/R");
	GtkCellRenderer* pixbufRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(classCol, pixbufRenderer, FALSE);
	gtk_tree_view_column_set_attributes(classCol, pixbufRenderer, 
										"pixbuf", CLASS_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(classCol, pixbufRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), classCol);
	
	// The Type
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(typeCol, "Type");
	
	GtkCellRenderer* typeIconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(typeCol, typeIconRenderer, FALSE);
	
	GtkCellRenderer* typeTextRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(typeCol, typeTextRenderer, FALSE);
	
	gtk_tree_view_column_set_attributes(typeCol, typeTextRenderer, 
										"text", CAPTION_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(typeCol, typeTextRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_column_set_attributes(typeCol, typeIconRenderer, 
										"pixbuf", ICON_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(typeCol, typeIconRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), typeCol);
	
	gtk_box_pack_start(GTK_BOX(srHBox), 
		gtkutil::ScrolledFrame(_list), FALSE, FALSE, 0);
	
	// The property pane
	gtk_box_pack_start(GTK_BOX(srHBox), createPropertyWidgets(), TRUE, TRUE, 6);
}

void StimEditor::setEntity(SREntityPtr entity) {
	// Pass the call to the base class
	ClassEditor::setEntity(entity);
	
	if (entity != NULL) {
		GtkListStore* listStore = _entity->getStimStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_list), GTK_TREE_MODEL(listStore));
		g_object_unref(listStore); // treeview owns reference now
	}
}

GtkWidget* StimEditor::createPropertyWidgets() {
	_propertyWidgets.vbox = gtk_vbox_new(FALSE, 6);
	
	// Type Selector
	GtkWidget* typeHBox = gtk_hbox_new(FALSE, 0);
	
	GtkWidget* typeLabel = gtkutil::LeftAlignedLabel("Type:");
	// Cast the helper class onto a ListStore and create a new treeview
	GtkListStore* stimListStore = _stimTypes;
	_propertyWidgets.typeList = gtk_combo_box_new_with_model(GTK_TREE_MODEL(stimListStore));
	gtk_widget_set_size_request(_propertyWidgets.typeList, -1, -1);
	g_object_unref(stimListStore); // tree view owns the reference now
	
	//g_signal_connect(G_OBJECT(_propertyWidgets.typeList), "changed", G_CALLBACK(onTypeSelect), this);
	
	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	GtkCellRenderer* iconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_propertyWidgets.typeList), iconRenderer, FALSE);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_propertyWidgets.typeList), nameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_propertyWidgets.typeList), nameRenderer, "text", 1);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_propertyWidgets.typeList), iconRenderer, "pixbuf", 2);
	gtk_cell_renderer_set_fixed_size(iconRenderer, 26, -1);
	
	gtk_box_pack_start(GTK_BOX(typeHBox), typeLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(
		GTK_BOX(typeHBox), 
		gtkutil::LeftAlignment(_propertyWidgets.typeList, 12, 1.0f), 
		TRUE, TRUE,	0
	);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), typeHBox, FALSE, FALSE, 0);
	
	// Active
	_propertyWidgets.active = gtk_check_button_new_with_label("Active");
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), _propertyWidgets.active, FALSE, FALSE, 0);
	
	// Use Bounds
	_propertyWidgets.useBounds = gtk_check_button_new_with_label("Use bounds");
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), _propertyWidgets.useBounds, FALSE, FALSE, 0);
	
	// Radius
	GtkWidget* radiusHBox = gtk_hbox_new(FALSE, 0);
	_propertyWidgets.radiusToggle = gtk_check_button_new_with_label("Radius:");
	gtk_widget_set_size_request(_propertyWidgets.radiusToggle, OPTIONS_LABEL_WIDTH, -1);
	_propertyWidgets.radiusEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(radiusHBox), _propertyWidgets.radiusToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(radiusHBox), _propertyWidgets.radiusEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), radiusHBox, FALSE, FALSE, 0);
	
	// Magnitude
	GtkWidget* magnHBox = gtk_hbox_new(FALSE, 0);
	_propertyWidgets.magnToggle = gtk_check_button_new_with_label("Magnitude:");
	gtk_widget_set_size_request(_propertyWidgets.magnToggle, OPTIONS_LABEL_WIDTH, -1);
	_propertyWidgets.magnEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(magnHBox), _propertyWidgets.magnToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(magnHBox), _propertyWidgets.magnEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), magnHBox, FALSE, FALSE, 0);
	
	// Falloff exponent
	GtkWidget* falloffHBox = gtk_hbox_new(FALSE, 0);
	_propertyWidgets.falloffToggle = gtk_check_button_new_with_label("Falloff Exponent:");
	gtk_widget_set_size_request(_propertyWidgets.falloffToggle, OPTIONS_LABEL_WIDTH, -1);
	_propertyWidgets.falloffEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(falloffHBox), _propertyWidgets.falloffToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(falloffHBox), _propertyWidgets.falloffEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), falloffHBox, FALSE, FALSE, 0);
	
	// Time Interval
	GtkWidget* timeHBox = gtk_hbox_new(FALSE, 0);
	_propertyWidgets.timeIntToggle = gtk_check_button_new_with_label("Time interval:");
	gtk_widget_set_size_request(_propertyWidgets.timeIntToggle, OPTIONS_LABEL_WIDTH, -1);
	_propertyWidgets.timeIntEntry = gtk_entry_new();
	_propertyWidgets.timeUnitLabel = gtkutil::RightAlignedLabel("ms");
	gtk_widget_set_size_request(_propertyWidgets.timeUnitLabel, 24, -1);
	
	gtk_box_pack_start(GTK_BOX(timeHBox), _propertyWidgets.timeIntToggle, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(timeHBox), _propertyWidgets.timeUnitLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeHBox), _propertyWidgets.timeIntEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), timeHBox, FALSE, FALSE, 0);
	
	// Timer type
	_propertyWidgets.timerTypeToggle = gtk_check_button_new_with_label("Timer restarts after firing");
	gtk_box_pack_start(GTK_BOX(_propertyWidgets.vbox), _propertyWidgets.timerTypeToggle, FALSE, FALSE, 0);
	
	// The map associating entry fields to stim property keys  
	_entryWidgets[GTK_EDITABLE(_propertyWidgets.radiusEntry)] = "time_interval";
	_entryWidgets[GTK_EDITABLE(_propertyWidgets.timeIntEntry)] = "radius";
	_entryWidgets[GTK_EDITABLE(_propertyWidgets.magnEntry)] = "magnitude";
	_entryWidgets[GTK_EDITABLE(_propertyWidgets.falloffEntry)] = "falloffexponent";
	
	// Connect the checkboxes
	g_signal_connect(G_OBJECT(_propertyWidgets.active), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.useBounds), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.radiusToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.timeIntToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.magnToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.falloffToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.timerTypeToggle), "toggled", G_CALLBACK(onCheckboxToggle), this);
	
	// Connect the entry fields
	g_signal_connect(G_OBJECT(_propertyWidgets.magnEntry), "changed", G_CALLBACK(onEntryChanged), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.falloffEntry), "changed", G_CALLBACK(onEntryChanged), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.radiusEntry), "changed", G_CALLBACK(onEntryChanged), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.timeIntEntry), "changed", G_CALLBACK(onEntryChanged), this);
	
	return gtkutil::LeftAlignment(_propertyWidgets.vbox, 6, 1.0f);
}

void StimEditor::checkBoxToggled(GtkToggleButton* toggleButton) {
	GtkWidget* toggleWidget = GTK_WIDGET(toggleButton);
	bool active = gtk_toggle_button_get_active(toggleButton);
	
	if (toggleWidget == _propertyWidgets.active) {
		setProperty("state", active ? "1" : "0");
	}
	else if (toggleWidget == _propertyWidgets.useBounds) {
		setProperty("use_bounds", active ? "1" : "");
	}
	else if (toggleWidget == _propertyWidgets.timerTypeToggle) {
		setProperty("timer_type", active ? "RELOAD" : "");
	}
	else if (toggleWidget == _propertyWidgets.radiusToggle) {
		std::string entryText = 
			gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.radiusEntry));
	
		// Enter a default value for the entry text, if it's empty up till now.
		if (active) {
			entryText += (entryText.empty()) ? "10" : "";	
		}
		else {
			entryText = "";
		}
		setProperty("radius", entryText);
	}
	else if (toggleWidget == _propertyWidgets.magnToggle) {
		std::string entryText = 
			gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.magnEntry));
	
		// Enter a default value for the entry text, if it's empty up till now.
		if (active) {
			entryText += (entryText.empty()) ? "10" : "";	
		}
		else {
			entryText = "";
		}
		setProperty("magnitude", entryText);
	}
	else if (toggleWidget == _propertyWidgets.falloffToggle) {
		std::string entryText = 
			gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.falloffEntry));
	
		// Enter a default value for the entry text, if it's empty up till now.
		if (active) {
			entryText += (entryText.empty()) ? "1" : "";	
		}
		else {
			entryText = "";
		}
		setProperty("falloffexponent", entryText);
	}
	else if (toggleWidget == _propertyWidgets.timeIntToggle) {
		std::string entryText = 
			gtk_entry_get_text(GTK_ENTRY(_propertyWidgets.timeIntEntry));
	
		// Enter a default value for the entry text, if it's empty up till now.
		if (active) {
			entryText += (entryText.empty()) ? "1000" : "";	
		}
		else {
			entryText = "";
		}
		setProperty("time_interval", entryText);
	}
}

void StimEditor::entryChanged(GtkEditable* editable) {
	// Try to find the key this entry widget is associated to
	EntryMap::iterator found = _entryWidgets.find(editable);
	
	if (found != _entryWidgets.end()) {
		std::string entryText = gtk_entry_get_text(GTK_ENTRY(editable));
		if (!entryText.empty()) {
			setProperty(found->second, entryText);
		}
	}
}

void StimEditor::removeItem(GtkTreeView* view) {
	// The argument <view> is not needed, there is only one list
	
	// Get the selected stim ID
	int id = getIdFromSelection();
	
	if (id > 0) {
		_entity->remove(id);
	}
}

void StimEditor::openContextMenu(GtkTreeView* view) {
	gtk_menu_popup(GTK_MENU(_contextMenu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

void StimEditor::addStim() {
	if (_entity == NULL) return;

	// Create a new StimResponse object
	int id = _entity->add();
	
	// Get a reference to the newly allocated object
	StimResponse& sr = _entity->get(id);
	sr.set("type", _stimTypes.getFirstName());

	// Refresh the values in the liststore
	_entity->updateListStores();
}

// Create the context menus
void StimEditor::createContextMenu() {
	// Menu widgets
	_contextMenu = gtk_menu_new();
		
	// Each menu gets a delete item
	_propertyWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
														   "Delete Stim");
	_propertyWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
														   "Add Stim");
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu),
						  _propertyWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_contextMenu), 
						  _propertyWidgets.deleteMenuItem);

	// Connect up the signals
	g_signal_connect(G_OBJECT(_propertyWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_propertyWidgets.addMenuItem), "activate",
					 G_CALLBACK(onContextMenuAdd), this);
					 
	// Show menus (not actually visible until popped up)
	gtk_widget_show_all(_contextMenu);
}

void StimEditor::update() {
	_updatesDisabled = true; // avoid unwanted callbacks
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		// Update all the widgets
		gtk_widget_set_sensitive(_propertyWidgets.vbox, TRUE);
		
		StimResponse& sr = _entity->get(id);
		
		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		GtkTreeIter typeIter = _stimTypes.getIterForName(sr.get("type"));
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_propertyWidgets.typeList), &typeIter);
		
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.respButton),
			(sr.get("class") == "R")
		);
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.stimButton),
			(sr.get("class") == "S")
		);
		
		// Active
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.active),
			(sr.get("state") == "1")
		);
		
		// Use Bounds
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.useBounds),
			sr.get("use_bounds") == "1"
		);
		gtk_widget_set_sensitive(_propertyWidgets.useBounds, sr.get("class") == "S");
		
		// Use Radius
		bool useRadius = (sr.get("radius") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.radiusToggle),
			useRadius
		);
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.radiusEntry), 
			sr.get("radius").c_str()
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.radiusEntry, 
			useRadius && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_propertyWidgets.radiusToggle, sr.get("class") == "S");
		
		// Use Time interval
		bool useTimeInterval = (sr.get("time_interval") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.timeIntToggle),
			useTimeInterval
		);
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.timeIntEntry), 
			sr.get("time_interval").c_str()
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.timeIntEntry, 
			useTimeInterval && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.timeUnitLabel, 
			useTimeInterval && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_propertyWidgets.timeIntToggle, sr.get("class") == "S");
		
		// Timer Type
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.timerTypeToggle),
			sr.get("timer_type") == "RELOAD"
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.timerTypeToggle, 
			sr.get("class") == "S" && useTimeInterval
		);
		
		// Use Magnitude
		bool useMagnitude = (sr.get("magnitude") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.magnToggle),
			useMagnitude && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_propertyWidgets.magnToggle, sr.get("class") == "S");
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.magnEntry),
			sr.get("magnitude").c_str()
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.magnEntry, 
			useMagnitude && sr.get("class") == "S"
		);
		
		// Use falloff exponent widgets
		bool useFalloff = (sr.get("falloffexponent") != "");
		
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_propertyWidgets.falloffToggle),
			useFalloff && sr.get("class") == "S"
		);
		gtk_entry_set_text(
			GTK_ENTRY(_propertyWidgets.falloffEntry),
			sr.get("falloffexponent").c_str()
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.falloffToggle, 
			useMagnitude && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(
			_propertyWidgets.falloffEntry, 
			useMagnitude && sr.get("class") == "S" && useFalloff
		);
		
		// Disable the editing of inherited properties completely
		if (sr.inherited()) {
			gtk_widget_set_sensitive(_propertyWidgets.vbox, FALSE);
		}
		
		// Update the delete context menu item
		gtk_widget_set_sensitive(_propertyWidgets.deleteMenuItem,	!sr.inherited());
	}
	else {
		gtk_widget_set_sensitive(_propertyWidgets.vbox, FALSE);
		// Disable the "delete" context menu item
		gtk_widget_set_sensitive(_propertyWidgets.deleteMenuItem, FALSE);
	}
	
	_updatesDisabled = false;
}

void StimEditor::selectionChanged() {
	update();
}

// Delete context menu items activated
void StimEditor::onContextMenuDelete(GtkWidget* w, StimEditor* self) {
	// Delete the selected stim from the list
	self->removeItem(GTK_TREE_VIEW(self->_list));
}

// Delete context menu items activated
void StimEditor::onContextMenuAdd(GtkWidget* w, StimEditor* self) {
	self->addStim();
}

} // namespace ui
