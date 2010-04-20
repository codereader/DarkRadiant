#include "FilterEditor.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"

namespace ui {

	namespace {
		const int DEFAULT_SIZE_X = 550;
	    const int DEFAULT_SIZE_Y = 350;
		const char* const WINDOW_TITLE_EDIT = N_("Edit Filter");
		const char* const WINDOW_TITLE_VIEW = N_("View Filter");

		const char* const RULE_HELP_TEXT = 
			N_("Filter rules are applied in the shown order.\n" \
			"<b>Match</b> is accepting regular expressions.\n" \
			"<b>Object</b>-type filters can be used to match <b>patch</b> or <b>brush</b>.");

		enum {
			COL_INDEX,
			COL_TYPE,
			COL_TYPE_STR,
			COL_REGEX,
			COL_ACTION,
			NUM_COLS
		};

		enum {
			WIDGET_NAME_ENTRY,
			WIDGET_ADD_RULE_BUTTON,
			WIDGET_MOVE_RULE_UP_BUTTON,
			WIDGET_MOVE_RULE_DOWN_BUTTON,
			WIDGET_DELETE_RULE_BUTTON,
			WIDGET_HELP_TEXT,
		};
	}

FilterEditor::FilterEditor(Filter& filter, GtkWindow* parent, bool viewOnly) :
	BlockingTransientWindow(viewOnly ? _(WINDOW_TITLE_VIEW) : _(WINDOW_TITLE_EDIT), parent),
	_originalFilter(filter),
	_filter(_originalFilter), // copy-construct
	_ruleStore(gtk_list_store_new(NUM_COLS, G_TYPE_INT,	// index
												G_TYPE_INT, // type
												G_TYPE_STRING, // type string
												G_TYPE_STRING, // regex match
												G_TYPE_STRING)),  // show/hide
	_selectedRule(-1),
	_result(NUM_RESULTS),
	_updateActive(false),
	_viewOnly(viewOnly)
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Create the widgets
	populateWindow();

	// Update the widget contents
	update();

	// Show and block
	show();
}

FilterEditor::Result FilterEditor::getResult() {
	return _result;
}

void FilterEditor::populateWindow() {
	// Create the dialog vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the name entry box
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Name") + "</b>"), FALSE, FALSE, 0);

	_widgets[WIDGET_NAME_ENTRY] = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_widgets[WIDGET_NAME_ENTRY], 18, 1), FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_NAME_ENTRY]), "changed", G_CALLBACK(onNameEdited), this);
	
	// And the rule treeview
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Rules") + "</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createCriteriaPanel(), TRUE, TRUE, 0);

	// Add the help text
	if (!_viewOnly) {
		_widgets[WIDGET_HELP_TEXT] = gtkutil::LeftAlignedLabel(_(RULE_HELP_TEXT));
		gtk_box_pack_start(GTK_BOX(vbox), _widgets[WIDGET_HELP_TEXT], FALSE, FALSE, 0);
	}

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), GTK_WIDGET(vbox));
}

void FilterEditor::update() {
	// Avoid callback loops
	_updateActive = true;

	// Populate the criteria store
	gtk_list_store_clear(_ruleStore);

	_selectedRule = -1;

	// Traverse the criteria of the Filter to be edited
	for (std::size_t i = 0; i < _filter.rules.size(); ++i) {
		GtkTreeIter iter;
		const FilterRule& rule = _filter.rules[i];

		// Allocate a new list store element and store its pointer into <iter>
		gtk_list_store_append(_ruleStore, &iter);

		int typeIndex = getTypeIndexForString(rule.type);
		
		gtk_list_store_set(_ruleStore, &iter, 
			COL_INDEX, static_cast<int>(i), 
			COL_TYPE, typeIndex,
			COL_TYPE_STR, rule.type.c_str(),
			COL_REGEX, rule.match.c_str(),
			COL_ACTION, rule.show ? _("show") : _("hide"),
			-1
		);
	}

	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_NAME_ENTRY]), _filter.name.c_str());

	updateWidgetSensitivity();

	_updateActive = false;
}

GtkWidget* FilterEditor::createCriteriaPanel() {
	// Create an hbox for the treeview and the action buttons
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	// Create a new treeview
	_ruleView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_ruleStore)));
		
	gtkutil::TextColumn indexCol(_("Index"), COL_INDEX);
	gtkutil::TextColumn regexCol(_("Match"), COL_REGEX);

	// Create the cell renderer for the action choice
	GtkCellRenderer* actionComboRenderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(actionComboRenderer), "has-entry", FALSE, NULL);
	g_object_set(G_OBJECT(actionComboRenderer), "text-column", 1, NULL);
	g_object_set(G_OBJECT(actionComboRenderer), "editable", TRUE, NULL);

	// Create the store
	GtkListStore* actionStore = createActionStore();
	g_object_set(G_OBJECT(actionComboRenderer), "model", GTK_TREE_MODEL(actionStore), NULL);

	// Construct the column itself
	GtkTreeViewColumn* actionCol = gtk_tree_view_column_new_with_attributes(
		_("Action"), 
		actionComboRenderer, 
		"markup", COL_ACTION,
		NULL
	);
	g_signal_connect(G_OBJECT(actionComboRenderer), "edited", G_CALLBACK(onActionEdited), this);

	// Regex editing
	GtkCellRendererText* rend = regexCol.getCellRenderer();
	g_object_set(G_OBJECT(rend), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(onRegexEdited), this);

	// Create the cell renderer for the type choice
	GtkCellRenderer* typeComboRenderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(typeComboRenderer), "has-entry", FALSE, NULL);
	g_object_set(G_OBJECT(typeComboRenderer), "text-column", 1, NULL);
	g_object_set(G_OBJECT(typeComboRenderer), "editable", TRUE, NULL);

	// Create the typestore
	GtkListStore* typeStore = createTypeStore();
	g_object_set(G_OBJECT(typeComboRenderer), "model", GTK_TREE_MODEL(typeStore), NULL);

	// Construct the column itself
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new_with_attributes(
		_("Type"), 
		typeComboRenderer, 
		"markup", COL_TYPE_STR,
		NULL
	);
	g_signal_connect(G_OBJECT(typeComboRenderer), "edited", G_CALLBACK(onTypeEdited), this);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_ruleView), indexCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_ruleView), typeCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_ruleView), regexCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_ruleView), actionCol);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(_ruleView));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onRuleSelectionChanged), this);

	// Action buttons
	_widgets[WIDGET_ADD_RULE_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_ADD);
	_widgets[WIDGET_MOVE_RULE_UP_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	_widgets[WIDGET_DELETE_RULE_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_RULE_BUTTON]), "clicked", G_CALLBACK(onAddRule), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_MOVE_RULE_UP_BUTTON]), "clicked", G_CALLBACK(onMoveRuleUp), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON]), "clicked", G_CALLBACK(onMoveRuleDown), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_RULE_BUTTON]), "clicked", G_CALLBACK(onDeleteRule), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_RULE_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_MOVE_RULE_UP_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_MOVE_RULE_DOWN_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_DELETE_RULE_BUTTON], FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::ScrolledFrame(GTK_WIDGET(_ruleView)), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), actionVBox, FALSE, FALSE, 0);

	return gtkutil::LeftAlignment(hbox, 18, 1);
}

GtkListStore* FilterEditor::createTypeStore() {
	// Create the typestore
	GtkTreeIter iter;
	int index = 0;

	GtkListStore* typeStore = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	
	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "entityclass", -1);

	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "texture", -1);

	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "object", -1);

	return typeStore;
}

GtkListStore* FilterEditor::createActionStore() {
	// Create the typestore
	GtkTreeIter iter;

	GtkListStore* actionStore = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	
	gtk_list_store_append(actionStore, &iter);
	gtk_list_store_set(actionStore, &iter, 0, TRUE, 1, _("show"), -1);

	gtk_list_store_append(actionStore, &iter);
	gtk_list_store_set(actionStore, &iter, 0, FALSE, 1, _("hide"), -1);

	return actionStore;
}

GtkWidget* FilterEditor::createButtonPanel() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	if (_viewOnly) {
		// OK button
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		// Connect the OK button to the "CANCEL" event
		g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onCancel), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	}
	else {
		// Save button
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
		
		// Cancel Button
		GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
		g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, TRUE, TRUE, 0);
	}
	
	return gtkutil::RightAlignment(buttonHBox);	
}

int FilterEditor::getTypeIndexForString(const std::string& type) {
	// Switch on the string
	if (type == "entityclass") {
		return 0;
	}
	else if (type == "texture") {
		return 1;
	}
	else if (type == "object") {
		return 2;
	}
	
	return -1;
}

void FilterEditor::save() {
	_filter.name = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_NAME_ENTRY]));

	// Copy the working set over the actual Filter
	_originalFilter = _filter;
}

void FilterEditor::updateWidgetSensitivity() {

	if (_viewOnly) {
		gtk_widget_set_sensitive(_widgets[WIDGET_NAME_ENTRY], FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(_ruleView), FALSE);

		gtk_widget_set_sensitive(_widgets[WIDGET_ADD_RULE_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_UP_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_RULE_BUTTON], FALSE);
		return;
	}

	if (_selectedRule != -1) {

		bool lastSelected = (_selectedRule + 1 >= static_cast<int>(_filter.rules.size()) || _filter.rules.size() <= 1);
		bool firstSelected = (_selectedRule <= 0 || _filter.rules.size() <= 1);

		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_UP_BUTTON], firstSelected ? FALSE : TRUE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON], lastSelected ? FALSE : TRUE);
		gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_RULE_BUTTON], TRUE);
	}
	else {
		// no rule selected
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_UP_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_RULE_BUTTON], FALSE);
	}
}

void FilterEditor::onSave(GtkWidget* widget, FilterEditor* self) {
	self->save();
	self->_result = RESULT_OK;
	self->destroy();
}

void FilterEditor::onCancel(GtkWidget* widget, FilterEditor* self) {
	self->_result = RESULT_CANCEL;
	self->destroy();
}

void FilterEditor::onRegexEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_ruleStore), &iter, path)) {
		// The iter points to the edited cell now, get the criterion number
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_ruleStore), &iter, COL_INDEX);
		
		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(self->_filter.rules.size()));

		self->_filter.rules[index].match = new_text;

		// Update the liststore item
		gtk_list_store_set(self->_ruleStore, &iter, COL_REGEX, new_text, -1);
	}
}

void FilterEditor::onTypeEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_ruleStore), &iter, path)) {
		// Look up the type index for "new_text"
		int typeIndex = self->getTypeIndexForString(new_text);

		// The iter points to the edited cell, get the criterion number
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_ruleStore), &iter, COL_INDEX);
		
		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(self->_filter.rules.size()));

		self->_filter.rules[index].type = new_text;

		// Update the liststore item
		gtk_list_store_set(self->_ruleStore, &iter, 
			COL_TYPE, typeIndex,
			COL_TYPE_STR, new_text, 
			-1
		);
	}
}

void FilterEditor::onActionEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_ruleStore), &iter, path)) {
		// The iter points to the edited cell, get the criterion number
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_ruleStore), &iter, COL_INDEX);
		
		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(self->_filter.rules.size()));

		// Update the bool flag
		self->_filter.rules[index].show = (std::string(new_text) == _("show"));
		
		// Update the liststore item
		gtk_list_store_set(self->_ruleStore, &iter, 
			COL_ACTION, new_text,
			-1
		);
	}
}

void FilterEditor::onNameEdited(GtkEditable* editable, FilterEditor* self) {
	if (self->_updateActive) return;

	self->_filter.name = gtk_entry_get_text(GTK_ENTRY(editable));
}

void FilterEditor::onRuleSelectionChanged(GtkTreeSelection* sel, FilterEditor* self) {
	// Get the selection
	GtkTreeIter selected;
	bool hasSelection = gtk_tree_selection_get_selected(sel, NULL, &selected) ? true : false;

	if (hasSelection) {
		self->_selectedRule = gtkutil::TreeModel::getInt(
			GTK_TREE_MODEL(self->_ruleStore), &selected, COL_INDEX
		);
	}
	else {
		self->_selectedRule = -1;
	}

	self->updateWidgetSensitivity();
}

void FilterEditor::onAddRule(GtkWidget* widget, FilterEditor* self) {
	FilterRule newRule("texture", "textures/", false);
	self->_filter.rules.push_back(newRule);

	self->update();
}

void FilterEditor::onMoveRuleUp(GtkWidget* widget, FilterEditor* self) {
	if (self->_selectedRule >= 1) {
		FilterRule temp = self->_filter.rules[self->_selectedRule - 1];
		self->_filter.rules[self->_selectedRule - 1] = self->_filter.rules[self->_selectedRule];
		self->_filter.rules[self->_selectedRule] = temp;

		self->update();
	}
}

void FilterEditor::onMoveRuleDown(GtkWidget* widget, FilterEditor* self) {
	if (self->_selectedRule < static_cast<int>(self->_filter.rules.size()) - 1) {
		FilterRule temp = self->_filter.rules[self->_selectedRule + 1];
		self->_filter.rules[self->_selectedRule + 1] = self->_filter.rules[self->_selectedRule];
		self->_filter.rules[self->_selectedRule] = temp;

		self->update();
	}
}

void FilterEditor::onDeleteRule(GtkWidget* widget, FilterEditor* self) {
	if (self->_selectedRule != -1) {
		// Let the rules slip down one index each
		for (std::size_t i = self->_selectedRule; i+1 < self->_filter.rules.size(); ++i) {
			self->_filter.rules[i] = self->_filter.rules[i+1];
		}

		// Remove one item, it is the superfluous one now
		self->_filter.rules.pop_back();

		self->update();
	}
}

} // namespace ui
