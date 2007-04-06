#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/image.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog.h"
#include "string/string.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "EffectEditor.h"

#include <iostream>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Stim/Response Editor";
	
	const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	
	const std::string LABEL_STIMRESPONSE_LIST = "<b>Stims/Responses</b>";
	const std::string LABEL_ADD_STIMRESPONSE = "<b>Add Stim/Response</b>";
	const std::string LABEL_RESPONSE_EFFECTS = "<b>Response Effects</b>";
	
	const char* LABEL_SAVE = "Save to Entity";
	const char* LABEL_REVERT = "Reload from Entity";
	
	const char* NO_ENTITY_ERROR = "A single entity must be selected to edit "
								  "Stim/Response properties.";
	
	const unsigned int TREE_VIEW_WIDTH = 280;
	const unsigned int TREE_VIEW_HEIGHT = 240;
	const unsigned int OPTIONS_LABEL_WIDTH = 140; 
}

StimResponseEditor::StimResponseEditor() :
	_entity(NULL),
	_updatesDisabled(false)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", 
					 G_CALLBACK(onDelete), this);
	g_signal_connect(G_OBJECT(_dialog), "key-press-event", 
					 G_CALLBACK(onWindowKeyPress), this);
	
	// Create the widgets
	populateWindow();
	createContextMenus();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void StimResponseEditor::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_dialog);
}

void StimResponseEditor::toggleWindow() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide_all(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Scan the selection for entities
		rescanSelection();
		
		// Has the rescan found an entity (the pointer is non-NULL then)
		if (_entity != NULL) {
			// Now show the dialog window again
			gtk_widget_show_all(_dialog);
		}
		else {
			gtkutil::errorDialog(NO_ENTITY_ERROR, 
								 GlobalRadiant().getMainWindow());
			gtk_widget_hide_all(_dialog);
		}
	}
}

static void textCellDataFunc(GtkTreeViewColumn* treeColumn,
							 GtkCellRenderer* cell,
							 GtkTreeModel* treeModel,
							 GtkTreeIter* iter, 
							 gpointer data)
{
	bool inherited = gtkutil::TreeModel::getBoolean(treeModel, iter, INHERIT_COL);
	
	g_object_set(G_OBJECT(cell), "sensitive", !inherited, NULL);
}

void StimResponseEditor::populateWindow() {
	
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(_dialog), _dialogVBox);
	
	// Create the notebook and add it to the vbox
	_notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(_notebook), TRUE, TRUE, 0);
	
	// The tab label items (icon + label)
	GtkWidget* stimLabelHBox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(
    	GTK_BOX(stimLabelHBox), 
    	gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_STIM)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(stimLabelHBox), gtk_label_new("Stims"), FALSE, FALSE, 3);
	
	GtkWidget* responseLabelHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(
    	GTK_BOX(responseLabelHBox), 
    	gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_RESPONSE)), 
    	FALSE, FALSE, 0
    );
	gtk_box_pack_start(GTK_BOX(responseLabelHBox), gtk_label_new("Responses"), FALSE, FALSE, 3);
	
	// Show the widgets before using them as label, they won't appear otherwise
	gtk_widget_show_all(stimLabelHBox);
	gtk_widget_show_all(responseLabelHBox);
	
	// Cast the helper class to a widget and add it to the notebook page
	_stimPageNum = gtk_notebook_append_page(_notebook, _stimEditor, stimLabelHBox);
	_responsePageNum = gtk_notebook_append_page(_notebook, _responseEditor, responseLabelHBox);
	
	// Stim/Response list section
    gtk_box_pack_start(GTK_BOX(_dialogVBox), 
    				   gtkutil::LeftAlignedLabel(LABEL_STIMRESPONSE_LIST), 
    				   FALSE, FALSE, 0);
	
	GtkWidget* srHBox = gtk_hbox_new(FALSE, 0);
	
	// Pack it into an alignment so that it is indented
	GtkWidget* srAlignment = gtkutil::LeftAlignment(GTK_WIDGET(srHBox), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(srAlignment), TRUE, TRUE, 0);
	
	_entitySRView = gtk_tree_view_new();
	gtk_widget_set_size_request(_entitySRView, TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);
	
	_entitySRSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_entitySRView));

	// Connect the signals
	g_signal_connect(G_OBJECT(_entitySRSelection), "changed", 
					 G_CALLBACK(onSRSelectionChange), this);
	g_signal_connect(G_OBJECT(_entitySRView), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_entitySRView), "button-release-event", 
					 G_CALLBACK(onTreeViewButtonRelease), this);
	
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
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_entitySRView), numCol);
	
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
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_entitySRView), classCol);
	
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
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_entitySRView), typeCol);
	
	gtk_box_pack_start(GTK_BOX(srHBox), 
		gtkutil::ScrolledFrame(_entitySRView), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(srHBox), createSRWidgets(), TRUE, TRUE, 6);
	
	// Response effects section
    gtk_box_pack_start(GTK_BOX(_dialogVBox),
    				   gtkutil::LeftAlignedLabel(LABEL_RESPONSE_EFFECTS),
    				   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_dialogVBox), 
					   gtkutil::LeftAlignment(createEffectWidgets(), 18, 1.0),
					   TRUE, TRUE, 0);
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

// Lower dialog buttons
GtkWidget* StimResponseEditor::createButtons() {

	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* saveButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), saveButton, TRUE, TRUE, 0);
	
	// Close Button
	_closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(
		G_OBJECT(_closeButton), "clicked", G_CALLBACK(onClose), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), _closeButton, TRUE, TRUE, 0);
	
	// Revert button
	GtkWidget* revertButton = 
		gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
	g_signal_connect(
		G_OBJECT(revertButton), "clicked", G_CALLBACK(onRevert), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), revertButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

GtkWidget* StimResponseEditor::createSRWidgets() {
	_srWidgets.vbox = gtk_vbox_new(FALSE, 6);
	
	// Create the buttons
	_srWidgets.stimButton = gtk_toggle_button_new();
	_srWidgets.respButton = gtk_toggle_button_new();
	g_signal_connect(G_OBJECT(_srWidgets.stimButton), "toggled", G_CALLBACK(onClassChange), this);
	g_signal_connect(G_OBJECT(_srWidgets.respButton), "toggled", G_CALLBACK(onClassChange), this);
	
	GtkWidget* stimImg = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_STIM));
	GtkWidget* respImg = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_RESPONSE));
	gtk_widget_set_size_request(stimImg, 20, -1);
	gtk_widget_set_size_request(respImg, 20, -1);
	
	GtkWidget* stimLabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(stimLabel), "<b>Stim</b>");
	
	GtkWidget* respLabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(respLabel), "<b>Response</b>");
	
	GtkWidget* stimBtnHbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(stimBtnHbox), stimImg, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(stimBtnHbox), stimLabel, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_srWidgets.stimButton), stimBtnHbox);
	
	GtkWidget* respBtnHbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(respBtnHbox), respImg, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(respBtnHbox), respLabel, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_srWidgets.respButton), respBtnHbox);
	
	// combine the buttons to a hbox
	GtkWidget* btnHbox = gtk_hbox_new(TRUE, 6);
	gtk_box_pack_start(GTK_BOX(btnHbox), _srWidgets.stimButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(btnHbox), _srWidgets.respButton, TRUE, TRUE, 0);
	
	// Pack the button Hbox to the SRWidgets
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), btnHbox, FALSE, FALSE, 0);
	
	// Type Selector
	GtkWidget* typeHBox = gtk_hbox_new(FALSE, 0);
	
	GtkWidget* typeLabel = gtkutil::LeftAlignedLabel("Type:");
	// Cast the helper class onto a ListStore and create a new treeview
	GtkListStore* stimListStore = _stimTypes;
	_srWidgets.typeList = gtk_combo_box_new_with_model(GTK_TREE_MODEL(stimListStore));
	gtk_widget_set_size_request(_srWidgets.typeList, -1, -1);
	g_object_unref(stimListStore); // tree view owns the reference now
	
	g_signal_connect(G_OBJECT(_srWidgets.typeList), "changed", G_CALLBACK(onTypeSelect), this);
	
	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	GtkCellRenderer* iconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_srWidgets.typeList), iconRenderer, FALSE);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_srWidgets.typeList), nameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_srWidgets.typeList), nameRenderer, "text", 1);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_srWidgets.typeList), iconRenderer, "pixbuf", 2);
	gtk_cell_renderer_set_fixed_size(iconRenderer, 26, -1);
	
	gtk_box_pack_start(GTK_BOX(typeHBox), typeLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(
		GTK_BOX(typeHBox), 
		gtkutil::LeftAlignment(_srWidgets.typeList, 12, 1.0f), 
		TRUE, TRUE,	0
	);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), typeHBox, FALSE, FALSE, 0);
	
	// Active
	_srWidgets.active = gtk_check_button_new_with_label("Active");
	g_signal_connect(G_OBJECT(_srWidgets.active), "toggled", G_CALLBACK(onActiveToggle), this);
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), _srWidgets.active, FALSE, FALSE, 0);
	
	// Use Bounds
	_srWidgets.useBounds = gtk_check_button_new_with_label("Use bounds");
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), _srWidgets.useBounds, FALSE, FALSE, 0);
	
	// Radius
	GtkWidget* radiusHBox = gtk_hbox_new(FALSE, 0);
	_srWidgets.radiusToggle = gtk_check_button_new_with_label("Radius:");
	gtk_widget_set_size_request(_srWidgets.radiusToggle, OPTIONS_LABEL_WIDTH, -1);
	_srWidgets.radiusEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(radiusHBox), _srWidgets.radiusToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(radiusHBox), _srWidgets.radiusEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), radiusHBox, FALSE, FALSE, 0);
	
	// Magnitude
	GtkWidget* magnHBox = gtk_hbox_new(FALSE, 0);
	_srWidgets.magnToggle = gtk_check_button_new_with_label("Magnitude:");
	gtk_widget_set_size_request(_srWidgets.magnToggle, OPTIONS_LABEL_WIDTH, -1);
	_srWidgets.magnEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(magnHBox), _srWidgets.magnToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(magnHBox), _srWidgets.magnEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), magnHBox, FALSE, FALSE, 0);
	
	// Falloff exponent
	GtkWidget* falloffHBox = gtk_hbox_new(FALSE, 0);
	_srWidgets.falloffToggle = gtk_check_button_new_with_label("Falloff Exponent:");
	gtk_widget_set_size_request(_srWidgets.falloffToggle, OPTIONS_LABEL_WIDTH, -1);
	_srWidgets.falloffEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(falloffHBox), _srWidgets.falloffToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(falloffHBox), _srWidgets.falloffEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), falloffHBox, FALSE, FALSE, 0);
	
	// Time Interval
	GtkWidget* timeHBox = gtk_hbox_new(FALSE, 0);
	_srWidgets.timeIntToggle = gtk_check_button_new_with_label("Time interval:");
	gtk_widget_set_size_request(_srWidgets.timeIntToggle, OPTIONS_LABEL_WIDTH, -1);
	_srWidgets.timeIntEntry = gtk_entry_new();
	_srWidgets.timeUnitLabel = gtkutil::RightAlignedLabel("ms");
	gtk_widget_set_size_request(_srWidgets.timeUnitLabel, 24, -1);
	
	gtk_box_pack_start(GTK_BOX(timeHBox), _srWidgets.timeIntToggle, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(timeHBox), _srWidgets.timeUnitLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeHBox), _srWidgets.timeIntEntry, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), timeHBox, FALSE, FALSE, 0);
	
	// Timer type
	_srWidgets.timerTypeToggle = gtk_check_button_new_with_label("Timer restarts after firing");
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), _srWidgets.timerTypeToggle, FALSE, FALSE, 0);
	
	// Connect the checkboxes
	g_signal_connect(G_OBJECT(_srWidgets.useBounds), "toggled", G_CALLBACK(onBoundsToggle), this);
	g_signal_connect(G_OBJECT(_srWidgets.radiusToggle), "toggled", G_CALLBACK(onRadiusToggle), this);
	g_signal_connect(G_OBJECT(_srWidgets.timeIntToggle), "toggled", G_CALLBACK(onTimeIntervalToggle), this);
	g_signal_connect(G_OBJECT(_srWidgets.magnToggle), "toggled", G_CALLBACK(onMagnitudeToggle), this);
	g_signal_connect(G_OBJECT(_srWidgets.falloffToggle), "toggled", G_CALLBACK(onFalloffToggle), this);
	g_signal_connect(G_OBJECT(_srWidgets.timerTypeToggle), "toggled", G_CALLBACK(onTimerTypeToggle), this);
	
	// Connect the entry fields
	g_signal_connect(G_OBJECT(_srWidgets.magnEntry), "changed", G_CALLBACK(onMagnitudeChanged), this);
	g_signal_connect(G_OBJECT(_srWidgets.falloffEntry), "changed", G_CALLBACK(onFalloffChanged), this);
	g_signal_connect(G_OBJECT(_srWidgets.radiusEntry), "changed", G_CALLBACK(onRadiusChanged), this);
	g_signal_connect(G_OBJECT(_srWidgets.timeIntEntry), "changed", G_CALLBACK(onTimeIntervalChanged), this);
	
	return _srWidgets.vbox;
}

// Create the response script list widgets
GtkWidget* StimResponseEditor::createEffectWidgets() {

	_effectWidgets.view = gtk_tree_view_new();
	_effectWidgets.selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_effectWidgets.view));
	gtk_widget_set_size_request(_effectWidgets.view, -1, 200);
	
	// Connect the signals
	g_signal_connect(G_OBJECT(_effectWidgets.selection), "changed",
					 G_CALLBACK(onEffectSelectionChange), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-press-event",
					 G_CALLBACK(onTreeViewButtonPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-release-event",
					 G_CALLBACK(onTreeViewButtonRelease), this);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("#", EFFECT_INDEX_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Effect", EFFECT_CAPTION_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Description", EFFECT_ARGS_COL)
	);
	
	// Return the tree view in a frame
	return gtkutil::ScrolledFrame(_effectWidgets.view);
}

// Create the context menus
void StimResponseEditor::createContextMenus() {
	
	// Menu widgets
	_stimListContextMenu = gtk_menu_new();
	_effectListContextMenu = gtk_menu_new();
	
	// Each menu gets a delete item
	_srWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
														   "Delete");
	_srWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
														   "Add new S/R");
	gtk_menu_shell_append(GTK_MENU_SHELL(_stimListContextMenu), 
						  _srWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_stimListContextMenu), 
						  _srWidgets.deleteMenuItem);

	_effectWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
															   "Add new Effect");
	_effectWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
															   "Delete");
	_effectWidgets.upMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_UP,
															   "Move Up");
	_effectWidgets.downMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_DOWN,
															   "Move Down");
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.upMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.downMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.deleteMenuItem);

	// Connect up the signals
	g_signal_connect(G_OBJECT(_srWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(_onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_srWidgets.addMenuItem), "activate",
					 G_CALLBACK(_onContextMenuAdd), this);
					 
	g_signal_connect(G_OBJECT(_effectWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(_onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_effectWidgets.addMenuItem), "activate",
					 G_CALLBACK(_onContextMenuAdd), this);
	g_signal_connect(G_OBJECT(_effectWidgets.upMenuItem), "activate",
					 G_CALLBACK(_onContextMenuEffectUp), this);
	g_signal_connect(G_OBJECT(_effectWidgets.downMenuItem), "activate",
					 G_CALLBACK(_onContextMenuEffectDown), this);

	// Show menus (not actually visible until popped up)
	gtk_widget_show_all(_stimListContextMenu);
	gtk_widget_show_all(_effectListContextMenu);
}

void StimResponseEditor::update() {
	gtk_widget_set_sensitive(_dialogVBox, _entity != NULL);
	
	updateSRWidgets();
	gtk_widget_set_sensitive(_closeButton, TRUE);
}

void StimResponseEditor::rescanSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	_srEntity = SREntityPtr();
	
	if (info.entityCount == 1 && info.totalCount == 1) {
		// Get the entity instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& node = instance.path().top();
		
		_entity = Node_getEntity(node);
		
		_srEntity = SREntityPtr(new SREntity(_entity));
		
		// Get the liststores from the SREntity and pack them
		GtkListStore* listStore = _srEntity->getStimResponseStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_entitySRView), GTK_TREE_MODEL(listStore));
		g_object_unref(listStore);
		
		// Clear the treeview (unset the model)
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
	}
	
	if (_entity != NULL) {
		std::string title = WINDOW_TITLE + " (" + _entity->getKeyValue("name") + ")";
		gtk_window_set_title(GTK_WINDOW(_dialog), title.c_str()); 
	}
	else {
		gtk_window_set_title(GTK_WINDOW(_dialog), WINDOW_TITLE.c_str());
	}
	
	// Update the widgets
	update();
}

void StimResponseEditor::updateSRWidgets() {
	_updatesDisabled = true;
	
	int id = getIdFromSelection();
	
	if (id > 0 && _srEntity != NULL) {
		// Update all the widgets
		gtk_widget_set_sensitive(_srWidgets.vbox, TRUE);
		
		StimResponse& sr = _srEntity->get(id);
		
		// Get the iter into the liststore pointing at the correct STIM_YYYY
		GtkTreeIter typeIter = _stimTypes.getIterForName(sr.get("type"));
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_srWidgets.typeList), &typeIter);
		
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.respButton),
			(sr.get("class") == "R")
		);
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.stimButton),
			(sr.get("class") == "S")
		);
		
		// Active
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.active),
			(sr.get("state") == "1")
		);
		
		// Use Bounds
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.useBounds),
			sr.get("use_bounds") == "1"
		);
		gtk_widget_set_sensitive(_srWidgets.useBounds, sr.get("class") == "S");
		
		// Use Radius
		bool useRadius = (sr.get("radius") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.radiusToggle),
			useRadius
		);
		gtk_entry_set_text(
			GTK_ENTRY(_srWidgets.radiusEntry), 
			sr.get("radius").c_str()
		);
		gtk_widget_set_sensitive(
			_srWidgets.radiusEntry, 
			useRadius && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_srWidgets.radiusToggle, sr.get("class") == "S");
		
		// Use Time interval
		bool useTimeInterval = (sr.get("time_interval") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.timeIntToggle),
			useTimeInterval
		);
		gtk_entry_set_text(
			GTK_ENTRY(_srWidgets.timeIntEntry), 
			sr.get("time_interval").c_str()
		);
		gtk_widget_set_sensitive(
			_srWidgets.timeIntEntry, 
			useTimeInterval && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(
			_srWidgets.timeUnitLabel, 
			useTimeInterval && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_srWidgets.timeIntToggle, sr.get("class") == "S");
		
		// Timer Type
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.timerTypeToggle),
			sr.get("timer_type") == "RELOAD"
		);
		gtk_widget_set_sensitive(
			_srWidgets.timerTypeToggle, 
			sr.get("class") == "S" && useTimeInterval
		);
		
		// Use Magnitude
		bool useMagnitude = (sr.get("magnitude") != "");
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.magnToggle),
			useMagnitude && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(_srWidgets.magnToggle, sr.get("class") == "S");
		gtk_entry_set_text(
			GTK_ENTRY(_srWidgets.magnEntry),
			sr.get("magnitude").c_str()
		);
		gtk_widget_set_sensitive(
			_srWidgets.magnEntry, 
			useMagnitude && sr.get("class") == "S"
		);
		
		// Use falloff exponent widgets
		bool useFalloff = (sr.get("falloffexponent") != "");
		
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_srWidgets.falloffToggle),
			useFalloff && sr.get("class") == "S"
		);
		gtk_entry_set_text(
			GTK_ENTRY(_srWidgets.falloffEntry),
			sr.get("falloffexponent").c_str()
		);
		gtk_widget_set_sensitive(
			_srWidgets.falloffToggle, 
			useMagnitude && sr.get("class") == "S"
		);
		gtk_widget_set_sensitive(
			_srWidgets.falloffEntry, 
			useMagnitude && sr.get("class") == "S" && useFalloff
		);
		
		// Disable the editing of inherited properties completely
		if (sr.inherited()) {
			gtk_widget_set_sensitive(_srWidgets.vbox, FALSE);
		}
		
		GtkListStore* effectStore = sr.getEffectStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), GTK_TREE_MODEL(effectStore));
		g_object_unref(effectStore);
		
		// Allow effect editing for non-inherited responses only
		gtk_widget_set_sensitive(
			_effectWidgets.view, 
			(sr.get("class") == "R" && !sr.inherited())
		);
		
		// The response effect list may be empty, so force an update of the
		// context menu sensitivity, in the case the "selection changed" 
		// signal doesn't get called
		updateEffectContextMenu();
		
		// Update the delete context menu item
		gtk_widget_set_sensitive(_srWidgets.deleteMenuItem,	!sr.inherited());
	}
	else {
		gtk_widget_set_sensitive(_srWidgets.vbox, FALSE);
		
		// Disable the "delete" context menu item
		gtk_widget_set_sensitive(_srWidgets.deleteMenuItem, FALSE);
		
		// Clear the effect tree view
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
	}
	
	_updatesDisabled = false;
}

void StimResponseEditor::addStimResponse() {
	if (_srEntity == NULL) return;

	// Create a new StimResponse object
	int id = _srEntity->add();
	
	// Get a reference to the newly allocated object
	StimResponse& sr = _srEntity->get(id);
	sr.set("type", _stimTypes.getFirstName());

	// Refresh the values in the liststore
	_srEntity->updateListStore();
}

void StimResponseEditor::removeStimResponse() {
	int id = getIdFromSelection();
	
	if (id > 0) {
		_srEntity->remove(id);
	}
}

void StimResponseEditor::addEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response
		if (sr.get("class") == "R") {
			// Add a new effect and update all the widgets
			sr.addEffect(effectIndex);
			update();
		}
	}
}

void StimResponseEditor::removeEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Remove the effect and update all the widgets
			sr.deleteEffect(effectIndex);
			update();
		}
	}
}

void StimResponseEditor::selectEffectIndex(const unsigned int index) {
	// Setup the selectionfinder to search for the index string
	gtkutil::TreeModel::SelectionFinder finder(index, EFFECT_INDEX_COL);

	gtk_tree_model_foreach(
		gtk_tree_view_get_model(GTK_TREE_VIEW(_effectWidgets.view)),
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		// Set the active row of the list to the given effect
		gtk_tree_selection_select_iter(_effectWidgets.selection, &iter);
	}
}

void StimResponseEditor::moveEffect(int direction) {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Move the index (swap the specified indices)
			sr.moveEffect(effectIndex, effectIndex + direction);
			update();
			// Select the moved effect after the update
			selectEffectIndex(effectIndex + direction);
		}
	}
}

int StimResponseEditor::getEffectIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_effectWidgets.selection, &model, &iter
	);
	
	if (anythingSelected && _srEntity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, EFFECT_INDEX_COL);
	}
	else {
		return -1;
	}
}

int StimResponseEditor::getIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_entitySRSelection, &model, &iter
	);
	
	if (anythingSelected && _srEntity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, ID_COL);
	}
	else {
		return -1;
	}
}

void StimResponseEditor::setProperty(const std::string& key, const std::string& value) {
	int id = getIdFromSelection();
	
	if (id > 0) {
		if (!_srEntity->get(id).inherited()) {
			_srEntity->setProperty(id, key, value);
		}
	}
	
	updateSRWidgets();
}

void StimResponseEditor::save() {
	// Consistency check can go here
	
	// Save the working set to the entity
	_srEntity->save(_entity);
}

void StimResponseEditor::editEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Create a new effect editor (self-destructs)
			new EffectEditor(GTK_WINDOW(_dialog), sr, effectIndex, *this);
			
			// The editor is modal and will destroy itself, our work is done
		}
	}
}

void StimResponseEditor::updateEffectContextMenu() {
	// Check if we have anything selected at all
	int curEffectIndex = getEffectIdFromSelection();
	int highestEffectIndex = 0;
	
	bool anythingSelected = curEffectIndex >= 0;
	
	int srId = getIdFromSelection();
	if (srId > 0) {
		StimResponse& sr = _srEntity->get(srId);
		highestEffectIndex = sr.highestEffectIndex();
	}
	
	bool upActive = anythingSelected && curEffectIndex > 1;
	bool downActive = anythingSelected && curEffectIndex < highestEffectIndex;
	
	// Enable or disable the "Delete" context menu items based on the presence
	// of a selection.
	gtk_widget_set_sensitive(_effectWidgets.deleteMenuItem, anythingSelected);
		
	gtk_widget_set_sensitive(_effectWidgets.upMenuItem, upActive);
	gtk_widget_set_sensitive(_effectWidgets.downMenuItem, downActive);
}

// Static GTK Callbacks
gboolean StimResponseEditor::onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self) {
	// Toggle the visibility of the window
	self->toggle();
	
	// Don't propagate the delete event
	return TRUE;
}

// Callback for S/R treeview selection changes
void StimResponseEditor::onSRSelectionChange(GtkTreeSelection* selection, 
										   StimResponseEditor* self) 
{
	if (self->_updatesDisabled)	return; // Callback loop guard
	
	// Update the other widgets
	self->updateSRWidgets();
}

// Callback for effects treeview selection changes
void StimResponseEditor::onEffectSelectionChange(GtkTreeSelection* selection, 
										   StimResponseEditor* self) 
{
	if (self->_updatesDisabled)	return; // Callback loop guard
	// Update the sensitivity
	self->updateEffectContextMenu();
}

void StimResponseEditor::onClassChange(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled)	return; // Callback loop guard
	
	if (GTK_WIDGET(toggleButton) == self->_srWidgets.stimButton) {
		self->setProperty("class", "S");
	}
	else {
		self->setProperty("class", "R");
	}
}

void StimResponseEditor::onActiveToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->setProperty("state", gtk_toggle_button_get_active(toggleButton) ? "1" : "0");
}

void StimResponseEditor::onBoundsToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->setProperty("use_bounds",	gtk_toggle_button_get_active(toggleButton) ? "1" : "");
}

void StimResponseEditor::onTimerTypeToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->setProperty("timer_type",	gtk_toggle_button_get_active(toggleButton) ? "RELOAD" : "");
}

void StimResponseEditor::onRadiusToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.radiusEntry));
	
	if (gtk_toggle_button_get_active(toggleButton)) {
		entryText += (entryText.empty()) ? "10" : "";	
	}
	else {
		entryText = "";
	}
	self->setProperty("radius", entryText);
}

void StimResponseEditor::onMagnitudeToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.magnEntry));
	
	if (gtk_toggle_button_get_active(toggleButton)) {
		entryText += (entryText.empty()) ? "10" : "";	
	}
	else {
		entryText = "";
	}
	self->setProperty("magnitude", entryText);
}

void StimResponseEditor::onFalloffToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.falloffEntry));
	
	if (gtk_toggle_button_get_active(toggleButton)) {
		entryText += (entryText.empty()) ? "1" : "";	
	}
	else {
		entryText = "";
	}
	self->setProperty("falloffexponent", entryText);
}

void StimResponseEditor::onTimeIntervalToggle(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.timeIntEntry));
	
	if (gtk_toggle_button_get_active(toggleButton)) {
		entryText += (entryText.empty()) ? "1000" : "";	
	}
	else {
		entryText = "";
	}
	self->setProperty("time_interval", entryText);
}

void StimResponseEditor::onTypeSelect(GtkComboBox* widget, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(widget, &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(widget);
		std::string name = gtkutil::TreeModel::getString(model, &iter, 3); // 3 = StimTypes::NAME_COL
		self->setProperty("type", name);
	}
}

void StimResponseEditor::onMagnitudeChanged(GtkEditable* editable, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard

	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.magnEntry));
	if (!entryText.empty()) {
		self->setProperty("magnitude", entryText);
	}
}

void StimResponseEditor::onFalloffChanged(GtkEditable* editable, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard

	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.falloffEntry));
	if (!entryText.empty()) {
		self->setProperty("falloffexponent", entryText);
	}
}

void StimResponseEditor::onTimeIntervalChanged(GtkEditable* editable, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard

	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.timeIntEntry));
	if (!entryText.empty()) {
		self->setProperty("time_interval", entryText);
	}
}

void StimResponseEditor::onRadiusChanged(GtkEditable* editable, StimResponseEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard

	std::string entryText = gtk_entry_get_text(GTK_ENTRY(self->_srWidgets.radiusEntry));
	if (!entryText.empty()) {
		self->setProperty("radius", entryText);
	}
}

void StimResponseEditor::onSave(GtkWidget* button, StimResponseEditor* self) {
	self->save();
}

void StimResponseEditor::onClose(GtkWidget* button, StimResponseEditor* self) {
	self->toggleWindow();
}

void StimResponseEditor::onRevert(GtkWidget* button, StimResponseEditor* self) {
	self->rescanSelection();
}

gboolean StimResponseEditor::onTreeViewKeyPress(
	GtkTreeView* view, GdkEventKey* event, StimResponseEditor* self)
{
	if (event->keyval == GDK_Delete) {
		if (view == GTK_TREE_VIEW(self->_entitySRView)) {
			self->removeStimResponse();
		}
		else if (view == GTK_TREE_VIEW(self->_effectWidgets.view)) {
			self->removeEffect();
		}
		
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

gboolean StimResponseEditor::onWindowKeyPress(
	GtkWidget* dialog, GdkEventKey* event, StimResponseEditor* self)
{
	if (event->keyval == GDK_Escape) {
		self->toggleWindow();
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

// Button click events on TreeViews
gboolean StimResponseEditor::onTreeViewButtonPress(
	GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self)
{
	if (ev->type == GDK_2BUTTON_PRESS) {
		// Call the effect editor upon double click
		self->editEffect();
	}
	
	return FALSE;
}

// Button release event on TreeViews
gboolean StimResponseEditor::onTreeViewButtonRelease(
	GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self)
{
	// Single click with RMB (==> open context menu)
	if (ev->button == 3) {
		if (view == GTK_TREE_VIEW(self->_entitySRView))
			gtk_menu_popup(GTK_MENU(self->_stimListContextMenu), NULL, NULL, 
						   NULL, NULL, 1, GDK_CURRENT_TIME);
		else if (view == GTK_TREE_VIEW(self->_effectWidgets.view))
			gtk_menu_popup(GTK_MENU(self->_effectListContextMenu), NULL, NULL, 
						   NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	
	return FALSE;
}

// Delete context menu items activated
void StimResponseEditor::_onContextMenuDelete(GtkWidget* w, 
											  StimResponseEditor* self)
{
	if (w == self->_srWidgets.deleteMenuItem)
		self->removeStimResponse();
	else if (w == self->_effectWidgets.deleteMenuItem)
		self->removeEffect();
}

// Delete context menu items activated
void StimResponseEditor::_onContextMenuAdd(GtkWidget* w, 
										   StimResponseEditor* self)
{
	if (w == self->_srWidgets.addMenuItem) {
		self->addStimResponse();
	}
	else if (w == self->_effectWidgets.addMenuItem) {
		self->addEffect();
	}
}

void StimResponseEditor::_onContextMenuEffectUp(GtkWidget* widget, 
	StimResponseEditor* self)
{
	self->moveEffect(-1);
}

void StimResponseEditor::_onContextMenuEffectDown(GtkWidget* widget, 
	StimResponseEditor* self)
{
	self->moveEffect(+1);
}

// Static command target
void StimResponseEditor::toggle() {
	Instance().toggleWindow();
}

StimResponseEditor& StimResponseEditor::Instance() {
	static StimResponseEditor _instance;
	return _instance;
}

} // namespace ui
