#include "AIVocalSetPreview.h"

#include "i18n.h"
#include "isound.h"
#include "eclass.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <wx/sizer.h>
#include "wxutil/Bitmap.h"
#include <wx/button.h>
#include <wx/stattext.h>

namespace ui
{

AIVocalSetPreview::AIVocalSetPreview(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	createControlPanel();

	// Trigger the initial update of the widgets
	update();

	srand(static_cast<unsigned int>(time(NULL)));
}

void AIVocalSetPreview::createControlPanel()
{
	SetMinClientSize(wxSize(200, -1));
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the playback button
	_playButton = new wxButton(this, wxID_ANY);
	_playButton->SetBitmap(wxutil::GetLocalBitmap("media-playback-start-ltr.png"));

	_stopButton = new wxButton(this, wxID_ANY);
	_stopButton->SetBitmap(wxutil::GetLocalBitmap("media-playback-stop.png"));

	_playButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AIVocalSetPreview::onPlay), NULL, this);
	_stopButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AIVocalSetPreview::onStop), NULL, this);

	wxBoxSizer* btnHBox = new wxBoxSizer(wxHORIZONTAL);

	btnHBox->Add(_playButton, 1, wxRIGHT, 6);
	btnHBox->Add(_stopButton, 1);

	_statusLabel = new wxStaticText(this, wxID_ANY, "");

	GetSizer()->Add(_statusLabel);
	GetSizer()->Add(btnHBox);
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
            _vocalSetDef, "snd_"
        );

		for (eclass::AttributeList::const_iterator i = sndAttrs.begin();
			i != sndAttrs.end(); ++i)
		{
			_setShaders.push_back(i->getValue());
		}
	}

	// If the soundshader string is empty, desensitise the widgets
	Enable(_vocalSetDef != NULL && !_setShaders.empty());
}

std::string AIVocalSetPreview::getRandomSoundFile()
{
	// get a random sound shader
	std::size_t idx = static_cast<std::size_t>(rand()) % _setShaders.size();

	auto soundShader = GlobalSoundManager().getSoundShader(_setShaders[idx]);

	if (!soundShader) return "";

	SoundFileList files = soundShader->getSoundFileList();

	if (files.empty()) return "";

	std::size_t fileIdx = static_cast<std::size_t>(rand()) % files.size();

	return files[fileIdx];
}

void AIVocalSetPreview::onPlay(wxCommandEvent& ev)
{
	_statusLabel->SetLabelMarkup("");

	std::string file = getRandomSoundFile();

	if (!file.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(file))
		{
			_statusLabel->SetLabelMarkup(_("<b>Error:</b> File not found."));
		}
	}
}

void AIVocalSetPreview::onStop(wxCommandEvent& ev)
{
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();
	_statusLabel->SetLabelMarkup("");
}

} // namespace ui
