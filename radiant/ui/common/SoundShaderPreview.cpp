#include "SoundShaderPreview.h"

#include "i18n.h"
#include "isound.h"
#include "iuimanager.h"
#include <iostream>

#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

SoundShaderPreview::SoundShaderPreview(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_listStore(new wxutil::TreeModel(_columns, true)),
	_soundShader("")
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	_treeView = new wxutil::TreeView(this);
	_treeView->SetMinClientSize(wxSize(-1, 130));

	_treeView->AssociateModel(_listStore);
	_listStore->DecRef();

	_treeView->AppendTextColumn(_("Sound Files"), _columns.shader.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Connect the "changed" signal
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(SoundShaderPreview::onSelectionChanged), NULL, this);

	GetSizer()->Add(_treeView, 1, wxEXPAND);
	GetSizer()->Add(createControlPanel(this), 0, wxALIGN_BOTTOM | wxLEFT, 12);

	// Trigger the initial update of the widgets
	update();
}

wxSizer* SoundShaderPreview::createControlPanel(wxWindow* parent)
{
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	// Create the playback button
	_playButton = new wxButton(parent, wxID_ANY, _("Play"));
	_playButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "media-playback-start-ltr.png"));

	_stopButton = new wxButton(parent, wxID_ANY, _("Stop"));
	_stopButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "media-playback-stop.png"));

	_playButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(SoundShaderPreview::onPlay), NULL, this);
	_stopButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(SoundShaderPreview::onStop), NULL, this);

	_playButton->SetMinSize(wxSize(80, -1));
	_stopButton->SetMinSize(wxSize(80, -1));

	_statusLabel = new wxStaticText(parent, wxID_ANY, "");
	_statusLabel->Wrap(100);

	vbox->Add(_statusLabel, 0, wxEXPAND | wxBOTTOM, 12);
	vbox->Add(_playButton, 0, wxBOTTOM, 6);
	vbox->Add(_stopButton, 0);

	return vbox;
}

void SoundShaderPreview::setSoundShader(const std::string& soundShader)
{
	_soundShader = soundShader;
	update();
}

void SoundShaderPreview::update()
{
	// Clear the current treeview model
	_listStore->Clear();

	// If the soundshader string is empty, desensitise the widgets
	Enable(!_soundShader.empty());

	if (!_soundShader.empty())
	{
		// We have a sound shader, update the liststore

		// Get the list of sound files associated to this shader
		const ISoundShaderPtr& shader = GlobalSoundManager().getSoundShader(_soundShader);

		if (!shader->getName().empty())
		{
			// Retrieve the list of associated filenames (VFS paths)
			SoundFileList list = shader->getSoundFileList();

			for (std::size_t i = 0; i < list.size(); ++i)
			{
				wxutil::TreeModel::Row row = _listStore->AddItem();

				row[_columns.shader] = list[i];

				_listStore->ItemAdded(_listStore->GetRoot(), row.getItem());

				// Pre-select the first sound file, for the user's convenience
				if (i == 0)
				{
					_treeView->Select(row.getItem());
				}
			}
		}
		else
		{
			// Not a valid soundshader, switch to inactive
			Enable(false);
		}
	}
}

std::string SoundShaderPreview::getSelectedSoundFile()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_listStore);
		return row[_columns.shader];
	}
	else
	{
		return "";
	}
}

void SoundShaderPreview::onSelectionChanged(wxDataViewEvent& ev)
{
	std::string selectedFile = getSelectedSoundFile();

	// Set the sensitivity of the playbutton accordingly
	_playButton->Enable(!selectedFile.empty());
}

void SoundShaderPreview::onPlay(wxCommandEvent& ev)
{
	_statusLabel->SetLabel("");

	std::string selectedFile = getSelectedSoundFile();

	if (!selectedFile.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(selectedFile))
		{
			_statusLabel->SetLabel(_("Error: File not found."));
		}
	}
}

void SoundShaderPreview::onStop(wxCommandEvent& ev)
{
	// Pass the call to the sound manager
	GlobalSoundManager().stopSound();

	_statusLabel->SetLabel("");
}

} // namespace ui
