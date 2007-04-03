#include "EffectEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include <iostream>

	namespace {
		const std::string WINDOW_TITLE = "Edit Response Effect";
		
		// The enumeration for populating the GtkListStore 
		enum {
			EFFECT_TYPE_NAME_COL,		// The name ("effect_damage")
			EFFECT_TYPE_CAPTION_COL,	// The caption ("Damage")
			EFFECT_TYPE_NUM_COLS
		};
	}

EffectEditor::EffectEditor(GtkWindow* parent) :
	DialogWindow(WINDOW_TITLE, parent)
{
	gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
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
	
	_argVBox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(
		GTK_BOX(_dialogVBox), 
		gtkutil::LeftAlignment(_argVBox, 18, 1.0f), 
		TRUE, TRUE, 0
	); 
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
	
	// Parse the argument types from the effect and create the widgets
	createArgumentWidgets(effect);
	
	gtk_widget_show_all(_window);
}

void EffectEditor::createArgumentWidgets(ResponseEffect& effect) {
	ResponseEffect::ArgumentList& list = effect.getArguments();

	for (ResponseEffect::ArgumentList::iterator i = list.begin(); 
		 i != list.end(); i++)
	{
		int index = i->first;
		ResponseEffect::Argument& arg = i->second;
		
		if (arg.type == "s") {
			// Create a new string argument item
			ArgumentItemPtr itemPtr(new StringArgument(arg));
			_argumentItems.push_back(itemPtr);
			
			// Cast the item onto a GtkWidget* and add it to the list
			GtkWidget* widget = *itemPtr;
			gtk_box_pack_start(GTK_BOX(_argVBox), widget, FALSE, FALSE, 0);
		}
		
		GtkWidget* label = gtk_label_new(i->second.title.c_str());
		gtk_box_pack_start(GTK_BOX(_dialogVBox), label, FALSE, FALSE, 0);
	}
}
