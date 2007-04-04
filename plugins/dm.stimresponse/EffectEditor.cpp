#include "EffectEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include <iostream>

	namespace {
		const std::string WINDOW_TITLE = "Edit Response Effect";
		const unsigned int WINDOW_MIN_WIDTH = 300;
		const unsigned int WINDOW_MIN_HEIGHT = 50;
		
		// The enumeration for populating the GtkListStore 
		enum {
			EFFECT_TYPE_NAME_COL,		// The name ("effect_damage")
			EFFECT_TYPE_CAPTION_COL,	// The caption ("Damage")
			EFFECT_TYPE_NUM_COLS
		};
	}

EffectEditor::EffectEditor(GtkWindow* parent) :
	DialogWindow(WINDOW_TITLE, parent),
	_argTable(NULL),
	_tooltips(NULL)
{
	gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	setWindowSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
	
	_effectStore = gtk_list_store_new(EFFECT_TYPE_NUM_COLS,
									  G_TYPE_STRING,
									  G_TYPE_STRING,
									  -1);

	// Retrieve the map from the ResponseEffectTypes object
	ResponseEffectTypeMap& effectTypes = 
		ResponseEffectTypes::Instance().getMap();

	for (ResponseEffectTypeMap::iterator i = effectTypes.begin(); 
		  i != effectTypes.end(); i++)
	{
		std::string caption = i->second->getValueForKey("editor_caption");
		
		GtkTreeIter iter;
		
		gtk_list_store_append(_effectStore, &iter);
		gtk_list_store_set(_effectStore, &iter, 
						   EFFECT_TYPE_NAME_COL, i->first.c_str(),
						   EFFECT_TYPE_CAPTION_COL, caption.c_str(),
						   -1);
	}
	
	populateWindow();
}

void EffectEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(_window), _dialogVBox);
	
	GtkWidget* effectHBox = gtk_hbox_new(FALSE, 0);
	
	_effectTypeCombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_effectStore));
	g_object_unref(_effectStore); // combo box owns the GTK reference now
	
	// Add the cellrenderer for the caption
	GtkCellRenderer* captionRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_effectTypeCombo), captionRenderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_effectTypeCombo), 
								  captionRenderer, "text", EFFECT_TYPE_CAPTION_COL);
								  
	GtkWidget* effectLabel = gtkutil::LeftAlignedLabel("Effect:");
	
	gtk_box_pack_start(GTK_BOX(effectHBox), effectLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(effectHBox), 
		gtkutil::LeftAlignment(_effectTypeCombo, 12, 1.0f), 
		TRUE, TRUE, 0
	);
	
	gtk_box_pack_start(GTK_BOX(_dialogVBox), effectHBox, FALSE, FALSE, 0);
	
	GtkWidget* argLabel = gtkutil::LeftAlignedLabel("<b>Arguments</b>");
	gtk_box_pack_start(GTK_BOX(_dialogVBox), argLabel, FALSE, FALSE, 0);
	
	GtkWidget* saveButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(onSave), this);
	
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(buttonHBox), saveButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, FALSE, FALSE, 6);
	
	gtk_box_pack_end(GTK_BOX(_dialogVBox), buttonHBox, FALSE, FALSE, 0);
}

void EffectEditor::editEffect(StimResponse& response, const unsigned int effectIndex) {
	ResponseEffect& effect = response.getResponseEffect(effectIndex);
	
	// Setup the selectionfinder to search for the name string
	gtkutil::TreeModel::SelectionFinder finder(effect.getName(), EFFECT_TYPE_NAME_COL);
	
	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_effectStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	GtkTreeIter found = finder.getIter();
	
	// Set the active row of the combo box to the current response effect type
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_effectTypeCombo), &found);
	
	// Create the alignment container that hold the (exchangable) widget table
	_argAlignment = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(_argAlignment), 0, 0, 18, 0);
	gtk_box_pack_start(GTK_BOX(_dialogVBox), _argAlignment, TRUE, TRUE, 0);
	
	// Parse the argument types from the effect and create the widgets
	createArgumentWidgets(effect);
	
	gtk_widget_show_all(_window);
}

void EffectEditor::createArgumentWidgets(ResponseEffect& effect) {
	ResponseEffect::ArgumentList& list = effect.getArguments();

	// Remove the old table if there exists one
	if (_argTable != NULL) {
		gtk_container_remove(GTK_CONTAINER(_argAlignment), _argTable);
		gtk_widget_hide(_argTable);
		gtk_widget_destroy(_argTable);
	}
	
	if (_tooltips != NULL) {
		g_object_unref(_tooltips);
	}
	
	_tooltips = gtk_tooltips_new();
	gtk_tooltips_enable(_tooltips);
	
	// Setup the table with default spacings
	_argTable = gtk_table_new(list.size(), 3, false);
    gtk_table_set_col_spacings(GTK_TABLE(_argTable), 12);
    gtk_table_set_row_spacings(GTK_TABLE(_argTable), 6);
	
	gtk_container_add(GTK_CONTAINER(_argAlignment), _argTable); 

	for (ResponseEffect::ArgumentList::iterator i = list.begin(); 
		 i != list.end(); i++)
	{
		int index = i->first;
		ResponseEffect::Argument& arg = i->second;
		ArgumentItemPtr item;

		if (arg.type == "s") {
			// Create a new string argument item
			item = ArgumentItemPtr(new StringArgument(arg, _tooltips));
		}
		else if (arg.type == "f") {
			// Create a new string argument item
			item = ArgumentItemPtr(new FloatArgument(arg, _tooltips));
		}
		else if (arg.type == "v") {
			// Create a new string argument item
			item = ArgumentItemPtr(new VectorArgument(arg, _tooltips));
		}
		else if (arg.type == "e") {
			// Create a new string argument item
			item = ArgumentItemPtr(new EntityArgument(arg, _tooltips));
		}
		
		if (item != NULL) {
			_argumentItems.push_back(item);
			
			// The label
			gtk_table_attach(
				GTK_TABLE(_argTable), item->getLabelWidget(),
				0, 1, index-1, index, // index starts with 1, hence the -1
				GTK_FILL, (GtkAttachOptions)0, 0, 0
			);
			
			// The edit widgets
			gtk_table_attach_defaults(
				GTK_TABLE(_argTable), item->getEditWidget(),
				1, 2, index-1, index // index starts with 1, hence the -1
			);
			
			// The help widgets
			gtk_table_attach(
				GTK_TABLE(_argTable), item->getHelpWidget(),
				2, 3, index-1, index, // index starts with 1, hence the -1
				(GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0
			);
		}
	}
}

void EffectEditor::save() {
	// Request each argument item to save the content into the argument
	for (unsigned int i = 0; i < _argumentItems.size(); i++) {
		_argumentItems[i]->save();
	}
}

// Static GTK callbacks
void EffectEditor::onSave(GtkWidget* button, EffectEditor* self) {
	// Save the arguments into the objects
	self->save();
	
	// Call the inherited DialogWindow::destroy method 
	self->destroy();
}

void EffectEditor::onCancel(GtkWidget* button, EffectEditor* self) {
	// Call the inherited DialogWindow::destroy method 
	self->destroy();
}
