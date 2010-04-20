#include "ComponentsDialog.h"
#include "Objective.h"
#include "ce/ComponentEditorFactory.h"
#include "util/TwoColumnTextCombo.h"

#include "itextstream.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include "i18n.h"
#include <vector>
#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace objectives
{

/* CONSTANTS */

namespace {

	const char* const DIALOG_TITLE = N_("Edit Objective");

	// Widget enum
	enum {
		WIDGET_OBJ_DESCRIPTION_ENTRY,
		WIDGET_OBJ_STATE_COMBO,
		WIDGET_OBJ_MANDATORY_FLAG,
		WIDGET_OBJ_IRREVERSIBLE_FLAG,
		WIDGET_OBJ_ONGOING_FLAG,
		WIDGET_OBJ_VISIBLE_FLAG,
		WIDGET_OBJ_ENABLING_OBJS,
		WIDGET_OBJ_SUCCESS_LOGIC,
		WIDGET_OBJ_FAILURE_LOGIC,
		WIDGET_OBJ_COMPLETION_SCRIPT,
		WIDGET_OBJ_FAILURE_SCRIPT,
		WIDGET_OBJ_COMPLETION_TARGET,
		WIDGET_OBJ_FAILURE_TARGET,
		WIDGET_EDIT_PANEL,
		WIDGET_TYPE_COMBO,
		WIDGET_STATE_FLAG,
		WIDGET_IRREVERSIBLE_FLAG,
		WIDGET_INVERTED_FLAG,
		WIDGET_PLAYER_RESPONSIBLE_FLAG,
		WIDGET_COMPEDITOR_PANEL,
	};

	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}
	
} // namespace

// Main constructor
ComponentsDialog::ComponentsDialog(GtkWindow* parent, Objective& objective) :
	gtkutil::BlockingTransientWindow(_(DIALOG_TITLE), parent),
	_objective(objective),
	_componentList(gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING)),
	_components(objective.components), // copy the components to our local working set
	_updateMutex(false),
	_timer(500, _onIntervalReached, this)
{
	// Dialog contains list view, edit panel and buttons
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), createObjectiveEditPanel(), FALSE, FALSE, 0);

	gtk_box_pack_start(
		GTK_BOX(vbx), gtkutil::LeftAlignedLabel(makeBold(_("Components"))), FALSE, FALSE, 0
	);

	GtkWidget* compvbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(compvbox), createListView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(compvbox), createEditPanel(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(compvbox), createComponentEditorPanel(), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignment(compvbox, 12, 1.0f) , TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	// Populate the list of components
	populateObjectiveEditPanel();
	populateComponents();

	// Add contents to main window
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), vbx);
}

ComponentsDialog::~ComponentsDialog() {
	_timer.disable();
}

// Create the panel for editing the currently-selected objective
GtkWidget* ComponentsDialog::createObjectiveEditPanel() {

	// Table for entry boxes
	GtkWidget* table = gtk_table_new(8, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);

	int row = 0;
	
	// Objective description
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Description"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_OBJ_DESCRIPTION_ENTRY] = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_OBJ_DESCRIPTION_ENTRY], 
							  1, 2, row, row+1);
	
	row++;

	// Difficulty Selection
	gtk_table_attach(GTK_TABLE(table),
					 gtkutil::LeftAlignedLabel(makeBold(_("Difficulty"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table),
							  _diffPanel.getWidget(),
							  1, 2, row, row+1);

	row++;

	// State selection
	gtk_table_attach(GTK_TABLE(table),
					 gtkutil::LeftAlignedLabel(makeBold(_("Initial state"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_OBJ_STATE_COMBO] = gtk_combo_box_new_text();
	gtk_table_attach_defaults(GTK_TABLE(table),
							  _widgets[WIDGET_OBJ_STATE_COMBO],
							  1, 2, row, row+1);

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	GtkComboBox* combo = GTK_COMBO_BOX(_widgets[WIDGET_OBJ_STATE_COMBO]);
	gtk_combo_box_append_text(combo, "INCOMPLETE");
	gtk_combo_box_append_text(combo, "COMPLETE");
	gtk_combo_box_append_text(combo, "FAILED");
	gtk_combo_box_append_text(combo, "INVALID");
	
	row++;

	// Options checkboxes.
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Flags>"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), createObjectiveFlagsTable(), 1, 2, row, row+1);
	
	row++;

	// Enabling objectives
	GtkWidget* enablingObjs = gtk_entry_new();
	_widgets[WIDGET_OBJ_ENABLING_OBJS] = enablingObjs;

	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Enabling Objectives"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), enablingObjs, 1, 2, row, row+1);
	
	row++;

	// Logic
	GtkWidget* logicHBox = gtk_hbox_new(FALSE, 6);
		
	// Success Logic
	GtkWidget* successLogic = gtk_entry_new();
	_widgets[WIDGET_OBJ_SUCCESS_LOGIC] = successLogic;

	// Failure Logic
	GtkWidget* failureLogic = gtk_entry_new();
	_widgets[WIDGET_OBJ_FAILURE_LOGIC] = failureLogic;

	gtk_box_pack_start(GTK_BOX(logicHBox), successLogic, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(logicHBox), gtkutil::LeftAlignedLabel(makeBold(_("Failure Logic"))), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(logicHBox), failureLogic, TRUE, TRUE, 0);

	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Sucess Logic"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), logicHBox, 1, 2, row, row+1);

	row++;

	// Completion/failure scripts

	GtkWidget* scriptHBox = gtk_hbox_new(FALSE, 6);
		
	// Completion Script
	GtkWidget* completionScript = gtk_entry_new();
	_widgets[WIDGET_OBJ_COMPLETION_SCRIPT] = completionScript;

	// Failure Script
	GtkWidget* failureScript = gtk_entry_new();
	_widgets[WIDGET_OBJ_FAILURE_SCRIPT] = failureScript;

	gtk_box_pack_start(GTK_BOX(scriptHBox), completionScript, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(scriptHBox), gtkutil::LeftAlignedLabel(makeBold(_("Failure Script"))), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(scriptHBox), failureScript, TRUE, TRUE, 0);

	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Completion Script"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), scriptHBox, 1, 2, row, row+1);
	
	row++;

	// Completion/failure targets

	GtkWidget* targetHBox = gtk_hbox_new(FALSE, 6);
		
	// Completion Target
	GtkWidget* completionTarget = gtk_entry_new();
	_widgets[WIDGET_OBJ_COMPLETION_TARGET] = completionTarget;

	// Failure Target
	GtkWidget* failureTarget = gtk_entry_new();
	_widgets[WIDGET_OBJ_FAILURE_TARGET] = failureTarget;

	gtk_box_pack_start(GTK_BOX(targetHBox), completionTarget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(targetHBox), gtkutil::LeftAlignedLabel(makeBold(_("Failure Target"))), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(targetHBox), failureTarget, TRUE, TRUE, 0);

	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Completion Target"))),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), targetHBox, 1, 2, row, row+1);

	// Pack items into a vbox and return
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), table, FALSE, FALSE, 0);

	return vbx;
}

// Create table of flag checkboxes
GtkWidget* ComponentsDialog::createObjectiveFlagsTable() {
	
	GtkWidget* hbx = gtk_hbox_new(FALSE, 12);

	_widgets[WIDGET_OBJ_MANDATORY_FLAG] =
		gtk_check_button_new_with_label(_("Mandatory")); 
	_widgets[WIDGET_OBJ_IRREVERSIBLE_FLAG] =
		gtk_check_button_new_with_label(_("Irreversible")); 
	_widgets[WIDGET_OBJ_ONGOING_FLAG] =
		gtk_check_button_new_with_label(_("Ongoing")); 
	_widgets[WIDGET_OBJ_VISIBLE_FLAG] =
		gtk_check_button_new_with_label(_("Visible")); 

	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_OBJ_MANDATORY_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_OBJ_ONGOING_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_OBJ_IRREVERSIBLE_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_OBJ_VISIBLE_FLAG], 
					   FALSE, FALSE, 0);

	return hbx;
}

// Create list view
GtkWidget* ComponentsDialog::createListView() {

	// Create tree view and connect selection changed callback	
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_componentList));
	_componentSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(_componentSel), "changed",
					 G_CALLBACK(_onSelectionChanged), this);

	// Number column
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn(_("Type"), 1, false));

	// Create Add and Delete buttons for components
	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	GtkWidget* delButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(addButton), "clicked", 
					 G_CALLBACK(_onAddComponent), this);
	g_signal_connect(G_OBJECT(delButton), "clicked", 
					 G_CALLBACK(_onDeleteComponent), this);
	
	GtkWidget* buttonsBox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(buttonsBox), addButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(buttonsBox), delButton, TRUE, TRUE, 0);
	
	// Put the buttons box next to the list view
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), buttonsBox, FALSE, FALSE, 0);

	return hbx;	
}

// Create edit panel
GtkWidget* ComponentsDialog::createEditPanel() {
	
	// Table
	GtkWidget* table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_widget_set_sensitive(table, FALSE); // disabled until selection
	
	// Component type dropdown
	GtkWidget* cmb = util::TwoColumnTextCombo();
	_widgets[WIDGET_TYPE_COMBO] = cmb;

	// Pack dropdown into table
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Type"))),
					 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_TYPE_COMBO]), "changed",
					 G_CALLBACK(_onTypeChanged), this);
	gtk_table_attach_defaults(GTK_TABLE(table),
							  _widgets[WIDGET_TYPE_COMBO],
							  1, 2, 0, 1);
							  
	// Populate the combo box. The set is in ID order.
	for (ComponentTypeSet::const_iterator i = ComponentType::SET_ALL().begin();
		 i != ComponentType::SET_ALL().end();
		 ++i)
	{
		GtkListStore* ls = GTK_LIST_STORE(
				gtk_combo_box_get_model(GTK_COMBO_BOX(cmb))
		);
		GtkTreeIter iter;
		gtk_list_store_append(ls, &iter);
		gtk_list_store_set(
			ls, &iter, 
			0, i->getDisplayName().c_str(), 
			1, i->getName().c_str(),
			-1
		);	
	}
	
	// Flags hbox
	_widgets[WIDGET_STATE_FLAG] = 
		gtk_check_button_new_with_label(_("Satisfied at start"));
	_widgets[WIDGET_IRREVERSIBLE_FLAG] = 
		gtk_check_button_new_with_label(_("Irreversible"));  
	_widgets[WIDGET_INVERTED_FLAG] =
		gtk_check_button_new_with_label(_("Boolean NOT"));  
	_widgets[WIDGET_PLAYER_RESPONSIBLE_FLAG] =
		gtk_check_button_new_with_label(_("Player responsible"));

	g_signal_connect(G_OBJECT(_widgets[WIDGET_STATE_FLAG]), "toggled", 
		G_CALLBACK(_onCompToggleChanged), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_IRREVERSIBLE_FLAG]), "toggled", 
		G_CALLBACK(_onCompToggleChanged), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_INVERTED_FLAG]), "toggled", 
		G_CALLBACK(_onCompToggleChanged), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_PLAYER_RESPONSIBLE_FLAG]), "toggled", 
		G_CALLBACK(_onCompToggleChanged), this);

	GtkWidget* flagsBox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(flagsBox), _widgets[WIDGET_STATE_FLAG],
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(flagsBox), _widgets[WIDGET_IRREVERSIBLE_FLAG],
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(flagsBox), _widgets[WIDGET_INVERTED_FLAG],
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(flagsBox), _widgets[WIDGET_PLAYER_RESPONSIBLE_FLAG],
					   FALSE, FALSE, 0);
	
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel(makeBold(_("Flags"))),
					 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), flagsBox, 1, 2, 1, 2, 
					 GTK_FILL, GTK_FILL, 0, 0);
	
	// Save and return the panel table
	_widgets[WIDGET_EDIT_PANEL] = table;
	return table;
}

// ComponentEditor panel
GtkWidget* ComponentsDialog::createComponentEditorPanel()
{
    // Invisible frame to contain the ComponentEditor
	_widgets[WIDGET_COMPEDITOR_PANEL] = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(
        GTK_FRAME(_widgets[WIDGET_COMPEDITOR_PANEL]), GTK_SHADOW_NONE
    );
    gtk_container_set_border_width(
        GTK_CONTAINER(_widgets[WIDGET_COMPEDITOR_PANEL]), 6
    );
    
    // Visible frame 
    GtkContainer* borderFrame = GTK_CONTAINER(gtk_frame_new(NULL));
    gtk_container_add(borderFrame, _widgets[WIDGET_COMPEDITOR_PANEL]);

	return GTK_WIDGET(borderFrame);
}

// Create buttons
GtkWidget* ComponentsDialog::createButtons() 
{
	// Create a homogeneous hbox
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

// Populate the component list
void ComponentsDialog::populateComponents() {
	
	// Clear the list store
	gtk_list_store_clear(_componentList);
	
	// Add components from the Objective to the list store
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_componentList, &iter);
		gtk_list_store_set(_componentList, &iter, 
						   0, i->first, 
						   1, i->second.getString().c_str(),
						   -1);	
	}
}

void ComponentsDialog::updateComponents() {
	// Traverse all components and update the items in the list
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		// Find the item in the list store (0th column carries the ID)
		gtkutil::TreeModel::SelectionFinder finder(i->first, 0);

		// Traverse the model
		gtk_tree_model_foreach(
			GTK_TREE_MODEL(_componentList), 
			gtkutil::TreeModel::SelectionFinder::forEach, 
			&finder
		);

		// Check if we found the item
		if (finder.getPath() != NULL) {
			GtkTreeIter iter = finder.getIter();
			gtk_list_store_set(
				_componentList, &iter, 
				0, i->first, 
				1, i->second.getString().c_str(),
				-1
			);
		}
	}
}

// Populate the edit panel
void ComponentsDialog::populateEditPanel(int index) {
	// Get the component
	Component& comp = _components[index];
	
	// Set the flags
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_STATE_FLAG]), 
		comp.isSatisfied() ? TRUE : FALSE
	);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_IRREVERSIBLE_FLAG]), 
		comp.isIrreversible() ? TRUE : FALSE
	);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_INVERTED_FLAG]), 
		comp.isInverted() ? TRUE : FALSE
	);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_PLAYER_RESPONSIBLE_FLAG]), 
		comp.isPlayerResponsible() ? TRUE : FALSE
	);

	// Change the type combo if necessary. Since the combo box was populated in
    // ID order, we can simply use our ComponentType's ID as an index.
    GtkComboBox* typeCombo = GTK_COMBO_BOX(_widgets[WIDGET_TYPE_COMBO]); 
    if (gtk_combo_box_get_active(typeCombo) != comp.getType().getId())
    {
        // Change the combo selection (this triggers a change of the
        // ComponentEditor panel)
        gtk_combo_box_set_active(typeCombo, comp.getType().getId());
    }
    else 
    {
        // Update the ComponentEditor ourselves, since the new Component has the
        // same type but we still want to refresh the panel with the new
        // contents
        changeComponentEditor(comp);
    }
}

// Populate the edit panel widgets using the given objective number
void ComponentsDialog::populateObjectiveEditPanel() {
	// Disable GTK callbacks while we're at it
	_updateMutex = true;

	// Get the objective
	const Objective& obj = _objective;
	
	// Set description text
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_DESCRIPTION_ENTRY]),
					   obj.description.c_str());
				
	// Update the difficulty panel
	_diffPanel.populateFromObjective(obj);

	// Set initial state enum
	gtk_combo_box_set_active(GTK_COMBO_BOX(_widgets[WIDGET_OBJ_STATE_COMBO]),
							 obj.state);
					   
	// Set flags
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_IRREVERSIBLE_FLAG]),
		obj.irreversible ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_ONGOING_FLAG]),
		obj.ongoing ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_MANDATORY_FLAG]),
		obj.mandatory ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_VISIBLE_FLAG]),
		obj.visible ? TRUE : FALSE);
		
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_ENABLING_OBJS]),
					   obj.enablingObjs.c_str());

	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_SUCCESS_LOGIC]),
					   obj.logic.successLogic.c_str());
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_LOGIC]),
					   obj.logic.failureLogic.c_str());

	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_COMPLETION_SCRIPT]),
					   obj.completionScript.c_str());
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_SCRIPT]),
					   obj.failureScript.c_str());

	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_COMPLETION_TARGET]),
					   obj.completionTarget.c_str());
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_TARGET]),
					   obj.failureTarget.c_str());

	_updateMutex = false;
}

// Get selected component index
int ComponentsDialog::getSelectedIndex() {
	// Get the selection if valid
	GtkTreeModel* model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(_componentSel, &model, &iter)) {

		// Valid selection, return the contents of the index column
		int idx;
		gtk_tree_model_get(model, &iter, 0, &idx, -1);
		
		return idx;
	}
	else {
		return -1;
	}
}

// Change component editor
void ComponentsDialog::changeComponentEditor(Component& compToEdit) {

	// greebo: Get a new component editor, any previous one will auto-destroy and
	// remove its widget from the container.
	_componentEditor = ce::ComponentEditorFactory::create(
        compToEdit.getType().getName(), compToEdit
	);

	if (_componentEditor != NULL) {
		// Get the widget from the ComponentEditor and show it
		GtkWidget* editor = _componentEditor->getWidget();
		gtk_widget_show_all(editor);
		
		// Pack the widget into the containing frame
		gtk_container_add(
			GTK_CONTAINER(_widgets[WIDGET_COMPEDITOR_PANEL]),
			editor
		);
	}
}

// Safely write the ComponentEditor contents to the Component
void ComponentsDialog::checkWriteComponent() {
	if (_componentEditor) {
        _componentEditor->writeToComponent();
	}
}

void ComponentsDialog::save() {
	// Write the objective properties
	_objective.description = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_DESCRIPTION_ENTRY]));

	// Save the difficulty settings
	_diffPanel.writeToObjective(_objective);

	// Set the initial state enum value from the combo box index
	_objective.state = static_cast<Objective::State>(
		gtk_combo_box_get_active(GTK_COMBO_BOX(_widgets[WIDGET_OBJ_STATE_COMBO]))
	);

	// Determine which checkbox is toggled, then update the appropriate flag
	_objective.mandatory = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_MANDATORY_FLAG])) ? true : false;

	_objective.visible = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_VISIBLE_FLAG])) ? true : false;

	_objective.ongoing = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_ONGOING_FLAG])) ? true : false;

	_objective.irreversible = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_OBJ_IRREVERSIBLE_FLAG])) ? true : false;

	// Enabling objectives
	_objective.enablingObjs = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_ENABLING_OBJS]));

	// Success/failure logic
	_objective.logic.successLogic = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_SUCCESS_LOGIC]));
	_objective.logic.failureLogic = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_LOGIC]));

	// Completion/Failure script
	_objective.completionScript = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_COMPLETION_SCRIPT]));
	_objective.failureScript = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_SCRIPT]));

	// Completion/Failure targets
	_objective.completionTarget = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_COMPLETION_TARGET]));
	_objective.failureTarget = gtk_entry_get_text(
		GTK_ENTRY(_widgets[WIDGET_OBJ_FAILURE_TARGET]));

	// Write the components
	checkWriteComponent();

	// Copy the working set over the ones in the objective
	_objective.components.swap(_components);
}

/* GTK CALLBACKS */

// Save button
void ComponentsDialog::_onOK(GtkWidget* w, ComponentsDialog* self) {
	self->save();
	self->destroy();
}

// Cancel button
void ComponentsDialog::_onCancel(GtkWidget* w, ComponentsDialog* self) {
	// Destroy the dialog without saving
    self->destroy();
}

void ComponentsDialog::_onCompToggleChanged(GtkToggleButton* togglebutton, ComponentsDialog* self) {
	if (self->_updateMutex) return;

	// Update the Component working copy. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = self->getSelectedIndex();
	assert(idx >= 0);

	Component& comp = self->_components[idx];

	if (GTK_WIDGET(togglebutton) == self->_widgets[WIDGET_STATE_FLAG]) {
		comp.setSatisfied(gtk_toggle_button_get_active(togglebutton) ? true : false);
	}
	else if (GTK_WIDGET(togglebutton) == self->_widgets[WIDGET_IRREVERSIBLE_FLAG]) {
		comp.setIrreversible(gtk_toggle_button_get_active(togglebutton) ? true : false);
	}
	else if (GTK_WIDGET(togglebutton) == self->_widgets[WIDGET_INVERTED_FLAG]) {
		comp.setInverted(gtk_toggle_button_get_active(togglebutton) ? true : false);
	}
	else if (GTK_WIDGET(togglebutton) == self->_widgets[WIDGET_PLAYER_RESPONSIBLE_FLAG]) {
		comp.setPlayerResponsible(gtk_toggle_button_get_active(togglebutton) ? true : false);
	}

	// Update the list store
	self->updateComponents();
}

// Selection changed
void ComponentsDialog::_onSelectionChanged(GtkTreeSelection* sel,
										   ComponentsDialog* self)
{
    // Save the existing ComponentEditor contents if req'd
    self->checkWriteComponent();

	// Get the selection if valid
	GtkTreeModel* model;
	GtkTreeIter iter;
	if (!gtk_tree_selection_get_selected(sel, &model, &iter)) 
    {
		// Disable the edit panel and remove the ComponentEditor
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_PANEL], FALSE);
        self->_componentEditor = objectives::ce::ComponentEditorPtr();

		// Turn off the periodic update of the component list
		self->_timer.disable();
	}
	else {
		// Otherwise populate edit panel with the current component index
		int component;
		gtk_tree_model_get(model, &iter, 0, &component, -1); 
		
		self->populateEditPanel(component);

		// Enable the edit panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_PANEL], TRUE);

		// Turn on the periodic update
		self->_timer.enable();
	}
}

// Add a new component
void ComponentsDialog::_onAddComponent(GtkWidget* w, ComponentsDialog* self) 
{
	Objective::ComponentMap& components = self->_components;
	
	// Find an unused component number (starting from 1)
	for (int idx = 1; idx < INT_MAX; ++idx) {
		if (components.find(idx) == components.end()) {
			// Unused, add a new component here
			Component comp;
			components.insert(std::make_pair(idx, comp));
			break;
		}
	}
	
	// Refresh the component list
	self->populateComponents();
}

// Remove a component
void ComponentsDialog::_onDeleteComponent(GtkWidget* w, ComponentsDialog* self) 
{
	// Delete the selected component
	int idx = self->getSelectedIndex();
	if (idx != -1) 
    {
        // Remove the selection first, so our selection-changed callback does not
        // attempt to writeToComponent() after the Component has already been deleted
        gtk_tree_selection_unselect_all(self->_componentSel);

        // Erase the actual component
		self->_components.erase(idx);
	}
	
	// Refresh the list
	self->populateComponents();		
}

// Type combo changed
void ComponentsDialog::_onTypeChanged(GtkWidget* w, ComponentsDialog* self) 
{
	// Get the current selection
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w), &iter);
	std::string selectedText = gtkutil::TreeModel::getString(
			gtk_combo_box_get_model(GTK_COMBO_BOX(w)),
			&iter,
			1
	); 
	
	// Update the Objective object. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = self->getSelectedIndex();
	assert(idx >= 0);
	Component& comp = self->_components[idx];

    // Store the newly-selected type in the Component
	comp.setType(ComponentType::getComponentType(selectedText));
	
    // Change the ComponentEditor
    self->changeComponentEditor(comp);

	// Update the components list with the new display string
	GtkTreeModel* model;
	GtkTreeIter compIter;
	gtk_tree_selection_get_selected(self->_componentSel, &model, &compIter);
	gtk_list_store_set(
		GTK_LIST_STORE(model), &compIter, 1, comp.getString().c_str(), -1
	);
}

gboolean ComponentsDialog::_onIntervalReached(gpointer data) {
	ComponentsDialog* self = reinterpret_cast<ComponentsDialog*>(data);

	if (self->_updateMutex) return TRUE;

	self->checkWriteComponent();
	self->updateComponents();

	// Return true, so that the timer gets called again
	return TRUE;
}

} // namespace objectives
