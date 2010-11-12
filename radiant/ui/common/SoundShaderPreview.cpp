#include "SoundShaderPreview.h"

#include "i18n.h"
#include "isound.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TreeModel.h"
#include <iostream>

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/liststore.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>

namespace ui {

SoundShaderPreview::SoundShaderPreview() :
	Gtk::HBox(false, 12),
	_soundShader("")
{
	_treeView = Gtk::manage(new Gtk::TreeView);
	_treeView->set_size_request(-1, 130);
	_treeView->append_column(_("Sound Files"), _columns.shader);

	// Connect the "changed" signal
	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this,&SoundShaderPreview::onSelectionChanged));

	pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeView)), true, true);
	pack_start(createControlPanel(), false, false);

	// Trigger the initial update of the widgets
	update();
}

Gtk::Widget& SoundShaderPreview::createControlPanel()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	vbox->set_size_request(200, -1);

	// Create the playback button
	_playButton = Gtk::manage(new Gtk::Button(Gtk::Stock::MEDIA_PLAY));
	_stopButton = Gtk::manage(new Gtk::Button(Gtk::Stock::MEDIA_STOP));

	_playButton->signal_clicked().connect(sigc::mem_fun(*this, &SoundShaderPreview::onPlay));
	_stopButton->signal_clicked().connect(sigc::mem_fun(*this, &SoundShaderPreview::onStop));

	Gtk::HBox* btnHBox = Gtk::manage(new Gtk::HBox(true, 6));

	btnHBox->pack_start(*_playButton, true, true);
	btnHBox->pack_start(*_stopButton, true, true);

	vbox->pack_end(*btnHBox, false, false);

	_statusLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	vbox->pack_end(*_statusLabel, false, false);

	return *vbox;
}

void SoundShaderPreview::setSoundShader(const std::string& soundShader)
{
	_soundShader = soundShader;
	update();
}

void SoundShaderPreview::update()
{
	// Clear the current treeview model
	_treeView->unset_model();

	// If the soundshader string is empty, desensitise the widgets
	set_sensitive(!_soundShader.empty());

	if (!_soundShader.empty())
	{
		// We have a sound shader, update the liststore

		// Get the list of sound files associated to this shader
		const ISoundShaderPtr& shader = GlobalSoundManager().getSoundShader(_soundShader);

		if (!shader->getName().empty())
		{
			// Create a new liststore and pack it into the treeview
			_listStore = Gtk::ListStore::create(_columns);
			_treeView->set_model(_listStore);

			// Retrieve the list of associated filenames (VFS paths)
			SoundFileList list = shader->getSoundFileList();

			for (std::size_t i = 0; i < list.size(); ++i)
			{
				Gtk::TreeModel::iterator iter = _listStore->append();
				Gtk::TreeModel::Row row = *iter;

				row[_columns.shader] = list[i];

				// Pre-select the first sound file, for the user's convenience
				if (i == 0)
				{
					_selection->select(iter);
				}
			}
		}
		else
		{
			// Not a valid soundshader, switch to inactive
			set_sensitive(false);
		}
	}
}

std::string SoundShaderPreview::getSelectedSoundFile()
{
	Gtk::TreeModel::Row selected = *_selection->get_selected();

	if (selected)
	{
		return Glib::ustring(selected[_columns.shader]);
	}
	else
	{
		return "";
	}
}

void SoundShaderPreview::onSelectionChanged()
{
	std::string selectedFile = getSelectedSoundFile();

	// Set the sensitivity of the playbutton accordingly
	_playButton->set_sensitive(!selectedFile.empty());
}

void SoundShaderPreview::onPlay()
{
	_statusLabel->set_markup("");

	std::string selectedFile = getSelectedSoundFile();

	if (!selectedFile.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(selectedFile))
		{
			_statusLabel->set_markup(_("<b>Error:</b> File not found."));
		}
	}
}

void SoundShaderPreview::onStop()
{
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();

	_statusLabel->set_markup("");
}

} // namespace ui
