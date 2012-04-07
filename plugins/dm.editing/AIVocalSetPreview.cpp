#include "AIVocalSetPreview.h"

#include "i18n.h"
#include "isound.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "eclass.h"

#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace ui {

AIVocalSetPreview::AIVocalSetPreview() :
	Gtk::HBox(false, 12)
{
	pack_start(createControlPanel(), true, true, 0);

	// Trigger the initial update of the widgets
	update();

	srand(static_cast<unsigned int>(time(NULL)));
}

Gtk::Widget& AIVocalSetPreview::createControlPanel()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	vbox->set_size_request(200, -1);

	// Create the playback button
	_playButton = Gtk::manage(new Gtk::Button(Gtk::Stock::MEDIA_PLAY));
	_stopButton = Gtk::manage(new Gtk::Button(Gtk::Stock::MEDIA_STOP));

	_playButton->signal_clicked().connect(sigc::mem_fun(*this, &AIVocalSetPreview::onPlay));
	_stopButton->signal_clicked().connect(sigc::mem_fun(*this, &AIVocalSetPreview::onStop));

	Gtk::HBox* btnHBox = Gtk::manage(new Gtk::HBox(true, 6));

	btnHBox->pack_start(*_playButton, true, true, 0);
	btnHBox->pack_start(*_stopButton, true, true, 0);

	vbox->pack_end(*btnHBox, false, false, 0);

	_statusLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	vbox->pack_end(*_statusLabel, false, false, 0);

	return *vbox;
}

void AIVocalSetPreview::setVocalSetEclass(const IEntityClassPtr& vocalSetDef)
{
	_vocalSetDef = vocalSetDef;

	update();
}

void AIVocalSetPreview::update()
{
	_setShaders.clear();

	if (_vocalSetDef != NULL)
	{
        eclass::AttributeList sndAttrs = eclass::getSpawnargsWithPrefix(
            *_vocalSetDef, "snd_"
        );

		for (eclass::AttributeList::const_iterator i = sndAttrs.begin();
			i != sndAttrs.end(); ++i)
		{
			_setShaders.push_back(i->getValue());
		}
	}

	// If the soundshader string is empty, desensitise the widgets
	set_sensitive(_vocalSetDef != NULL && !_setShaders.empty());
}

std::string AIVocalSetPreview::getRandomSoundFile()
{
	// get a random sound shader
	std::size_t idx = static_cast<std::size_t>(rand()) % _setShaders.size();

	ISoundShaderPtr soundShader = GlobalSoundManager().getSoundShader(_setShaders[idx]);

	if (soundShader == NULL) return "";

	SoundFileList files = soundShader->getSoundFileList();

	if (files.empty()) return "";

	std::size_t fileIdx = static_cast<std::size_t>(rand()) % files.size();

	return files[fileIdx];
}

void AIVocalSetPreview::onPlay()
{
	_statusLabel->set_text("");

	std::string file = getRandomSoundFile();

	if (!file.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(file))
		{
			_statusLabel->set_markup(_("<b>Error:</b> File not found."));
		}
	}
}

void AIVocalSetPreview::onStop()
{
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();
	_statusLabel->set_text("");
}

} // namespace ui
