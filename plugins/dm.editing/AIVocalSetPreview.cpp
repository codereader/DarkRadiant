#include "AIVocalSetPreview.h"

#include "i18n.h"
#include "isound.h"
#include "gtkutil/LeftAlignedLabel.h"
#include <gtk/gtk.h>

namespace ui {

AIVocalSetPreview::AIVocalSetPreview()
{
	_widget = gtk_hbox_new(FALSE, 12);
	
	gtk_box_pack_start(GTK_BOX(_widget), createControlPanel(), TRUE, TRUE, 0);
	
	// Trigger the initial update of the widgets
	update();
}

GtkWidget* AIVocalSetPreview::createControlPanel()
{
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

void AIVocalSetPreview::setVocalSetEclass(const IEntityClassPtr& vocalSetDef)
{
	_vocalSetDef = vocalSetDef;
	update();
}

void AIVocalSetPreview::update()
{
	// If the soundshader string is empty, desensitise the widgets
	gtk_widget_set_sensitive(_widget, _vocalSetDef != NULL ? TRUE : FALSE);
}

std::string AIVocalSetPreview::getRandomSoundFile()
{
	// TODO

	return "";
}

void AIVocalSetPreview::onPlay(GtkWidget*, AIVocalSetPreview* self)
{
	gtk_label_set_markup(GTK_LABEL(self->_statusLabel), "");

	std::string file = self->getRandomSoundFile();
	
	if (!file.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(file))
		{
			gtk_label_set_markup(
				GTK_LABEL(self->_statusLabel), 
				_("<b>Error:</b> File not found.")
			);
		}
	}
}

void AIVocalSetPreview::onStop(GtkWidget*, AIVocalSetPreview* self)
{
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();
	gtk_label_set_markup(GTK_LABEL(self->_statusLabel), "");
}

} // namespace ui
