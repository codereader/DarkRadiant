#include "SoundShaderPreview.h"

#include "isound.h"
#include "gtkutil/ScrolledFrame.h"
#include <gtk/gtk.h>
#include <iostream>

namespace ui {

SoundShaderPreview::SoundShaderPreview() :
	_soundShader("")
{
	_widget = gtk_hbox_new(FALSE, 12);
	
	_treeView = gtk_tree_view_new();
	gtk_widget_set_size_request(_treeView, -1, 200);
	
	// Point the TreeSelection to this treeview
	_treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	
	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), gtk_label_new("Test"), FALSE, FALSE, 0);
	
	// Trigger the initial update of the widgets
	update();
}

void SoundShaderPreview::setSoundShader(const std::string& soundShader) {
	_soundShader = soundShader;
	update();
}

void SoundShaderPreview::update() {
	// Clear the current treeview model
	gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);
	
	// If the soundshader string is empty, desensitise the widgets
	gtk_widget_set_sensitive(_widget, !_soundShader.empty());
	
	if (!_soundShader.empty()) {
		// We have a sound shader, update the liststore
		
		// Get the list of sound files associated to this shader
		const ISoundShader& shader = GlobalSoundManager().getSoundShader(_soundShader);
		
		if (!shader.getName().empty()) {
			// Create a new liststore and pack it into the treeview
			_listStore = gtk_list_store_new(1, G_TYPE_STRING);
			gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), GTK_TREE_MODEL(_listStore));
			
			//SoundFileList list = getSoundFileList
		}
		else {
			// Not a valid soundshader, switch to inactive
			gtk_widget_set_sensitive(_widget, FALSE);
		}
	}
}

SoundShaderPreview::operator GtkWidget*() {
	return _widget;
}
	
} // namespace ui
