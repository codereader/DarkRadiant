#include "SoundShaderPreview.h"

#include "isound.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TreeModel.h"
#include <gtk/gtk.h>
#include <iostream>

namespace ui {

	namespace {
		enum FileListCols {
			FILENAME_COL,	// The filename (VFS path)
			NUM_COLS
		};
	}

SoundShaderPreview::SoundShaderPreview() :
	_soundShader("")
{
	_widget = gtk_hbox_new(FALSE, 12);
	
	_treeView = gtk_tree_view_new();
	gtk_widget_set_size_request(_treeView, -1, 130);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_treeView),
		gtkutil::TextColumn("Sound Files", FILENAME_COL)
	);
	
	// Point the TreeSelection to this treeview
	_treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	g_signal_connect(G_OBJECT(_treeSelection), "changed", 
					 G_CALLBACK(onSelectionChange), this);
	
	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), createControlPanel(), FALSE, FALSE, 0);
	
	// Trigger the initial update of the widgets
	update();
}

GtkWidget* SoundShaderPreview::createControlPanel() {
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_set_size_request(vbox, 200, -1);
	
	// Create the playback button
	_playButton = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	_stopButton = gtk_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
	g_signal_connect(G_OBJECT(_playButton), "clicked", G_CALLBACK(onPlay), this);
	g_signal_connect(G_OBJECT(_stopButton), "clicked", G_CALLBACK(onStop), this);
	
	GtkWidget* btnHBox = gtk_hbox_new(TRUE, 6);
	gtk_box_pack_start(GTK_BOX(btnHBox), _playButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(btnHBox), _stopButton, TRUE, TRUE, 0);
	
	gtk_box_pack_end(GTK_BOX(vbox), btnHBox, FALSE, FALSE, 0);
	
	_statusLabel = gtkutil::LeftAlignedLabel("");
	gtk_box_pack_end(GTK_BOX(vbox), _statusLabel, FALSE, FALSE, 0);
	
	return vbox; 
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
			_listStore = gtk_list_store_new(NUM_COLS, G_TYPE_STRING);
			gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), GTK_TREE_MODEL(_listStore));
			
			// Retrieve the list of associated filenames (VFS paths)
			SoundFileList list = shader.getSoundFileList();
			
			for (unsigned int i = 0; i < list.size(); i++) {
				GtkTreeIter iter;
				gtk_list_store_append(_listStore, &iter);
				gtk_list_store_set(_listStore, &iter, 
								   FILENAME_COL, list[i].c_str(),
								   -1);
			}
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

std::string SoundShaderPreview::getSelectedSoundFile() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = 
		gtk_tree_selection_get_selected(_treeSelection, &model, &iter) ? true : false;
	
	if (anythingSelected) {
		return gtkutil::TreeModel::getString(model, &iter, FILENAME_COL);
	}
	else {
		return "";
	}
}

void SoundShaderPreview::onSelectionChange(
	GtkTreeSelection* ts, SoundShaderPreview* self)
{
	std::string selectedFile = self->getSelectedSoundFile();
	
	// Set the sensitivity of the playbutton accordingly
	gtk_widget_set_sensitive(self->_playButton, !selectedFile.empty());
}

void SoundShaderPreview::onPlay(GtkButton* button, SoundShaderPreview* self) {
	gtk_label_set_markup(GTK_LABEL(self->_statusLabel), "");
	std::string selectedFile = self->getSelectedSoundFile();
	
	if (!selectedFile.empty()) {
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(selectedFile)) {
			gtk_label_set_markup(
				GTK_LABEL(self->_statusLabel), 
				"<b>Error:</b> File not found."
			);
		}
	}
}

void SoundShaderPreview::onStop(GtkButton* button, SoundShaderPreview* self) {
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();
	gtk_label_set_markup(GTK_LABEL(self->_statusLabel), "");
}

} // namespace ui
