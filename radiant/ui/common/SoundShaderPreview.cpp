#include "SoundShaderPreview.h"

#include "i18n.h"
#include "isound.h"
#include "itextstream.h"
#include <iostream>
#include <cstdlib>
#include <fmt/format.h>

#include "wxutil/Bitmap.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "ui/UserInterfaceModule.h"

namespace ui
{

namespace
{
    inline std::string getDurationString(float durationInSeconds)
    {
        return fmt::format("{0:0.2f}s", durationInSeconds);
    }
}

SoundShaderPreview::SoundShaderPreview(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_listStore(new wxutil::TreeModel(_columns, true)),
	_soundShader(""),
    _isShuttingDown(false)
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore.get());
	_treeView->SetMinClientSize(wxSize(-1, 130));

	_treeView->AppendTextColumn(_("Sound Files"), _columns.soundFile.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _treeView->AppendTextColumn(_("Duration"), _columns.duration.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Connect the "changed" signal
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SoundShaderPreview::onSelectionChanged, this);
	_treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SoundShaderPreview::onItemActivated, this);

	GetSizer()->Add(_treeView, 1, wxEXPAND);
	GetSizer()->Add(createControlPanel(this), 0, wxALIGN_BOTTOM | wxLEFT, 12);

	// Attach to the close event
	Bind(wxEVT_DESTROY, [&](wxWindowDestroyEvent& ev)
	{
		GlobalSoundManager().stopSound();
		ev.Skip();
	});

	// Trigger the initial update of the widgets
	update();
}

SoundShaderPreview::~SoundShaderPreview()
{
    _isShuttingDown = true;
    _durationQueries.clear();
}

wxSizer* SoundShaderPreview::createControlPanel(wxWindow* parent)
{
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	// Create the playback button
	_playButton = new wxButton(parent, wxID_ANY, _("Play"));
	_playButton->SetBitmap(wxutil::GetLocalBitmap("media-playback-start-ltr.png"));

	_playLoopedButton = new wxButton(parent, wxID_ANY, _("Play and loop"));
	_playLoopedButton->SetBitmap(wxutil::GetLocalBitmap("loop.png"));

	_stopButton = new wxButton(parent, wxID_ANY, _("Stop"));
	_stopButton->SetBitmap(wxutil::GetLocalBitmap("media-playback-stop.png"));

	_playButton->Bind(wxEVT_BUTTON, &SoundShaderPreview::onPlay, this);
	_playLoopedButton->Bind(wxEVT_BUTTON, &SoundShaderPreview::onPlayLooped, this);
	_stopButton->Bind(wxEVT_BUTTON, &SoundShaderPreview::onStop, this);

	_playButton->SetMinSize(wxSize(120, -1));
	_playLoopedButton->SetMinSize(wxSize(120, -1));
	_stopButton->SetMinSize(wxSize(120, -1));

	_statusLabel = new wxStaticText(parent, wxID_ANY, "");
	_statusLabel->Wrap(100);

	vbox->Add(_statusLabel, 0, wxEXPAND | wxBOTTOM, 12);
	vbox->Add(_playButton, 0, wxBOTTOM, 6);
	vbox->Add(_playLoopedButton, 0, wxBOTTOM, 6);
	vbox->Add(_stopButton, 0);

	return vbox;
}

void SoundShaderPreview::ClearPreview()
{
    SetPreviewDeclName({});
}

void SoundShaderPreview::SetPreviewDeclName(const std::string& declName)
{
    _soundShader = declName;
    update();
}

void SoundShaderPreview::playRandomSoundFile()
{
	if (_soundShader.empty() || !_listStore) return;

	// Select a random file from the list
	wxDataViewItemArray children;
	unsigned int numFiles = _listStore->GetChildren(_listStore->GetRoot(), children);

	if (numFiles > 0)
	{
        static int nextFileToPlay = 0;
		int selected = nextFileToPlay++ % numFiles;
		_treeView->Select(children[selected]);
		handleSelectionChange();

		playSelectedFile(false);
	}
}

void SoundShaderPreview::update()
{
	// Clear the current treeview model
	_listStore->Clear();

    _durationQueries.clearPendingTasks();

	// If the soundshader string is empty, desensitise the widgets
	Enable(!_soundShader.empty());

	if (!_soundShader.empty())
	{
		// We have a sound shader, update the liststore

		// Get the list of sound files associated to this shader
		const auto& shader = GlobalSoundManager().getSoundShader(_soundShader);

		if (!shader->getDeclName().empty())
		{
			// Retrieve the list of associated filenames (VFS paths)
			auto list = shader->getSoundFileList();

			for (std::size_t i = 0; i < list.size(); ++i)
			{
				auto row = _listStore->AddItem();
                const auto& soundFile = list[i];

				row[_columns.soundFile] = soundFile;
				row[_columns.duration] = getDurationOrPlaceholder(soundFile);

				row.SendItemAdded();

				// Pre-select the first sound file, for the user's convenience
				if (i == 0)
				{
					_treeView->Select(row.getItem());
				}
			}

			handleSelectionChange();
		}
		else
		{
			// Not a valid soundshader, switch to inactive
			Enable(false);
		}
	}
    else
    {
        GlobalSoundManager().stopSound();
    }
}

std::string SoundShaderPreview::getSelectedSoundFile()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_listStore);
		return row[_columns.soundFile];
	}
	else
	{
		return "";
	}
}

void SoundShaderPreview::onSelectionChanged(wxDataViewEvent& ev)
{
	handleSelectionChange();
}

void SoundShaderPreview::onItemActivated(wxDataViewEvent& ev)
{
	playSelectedFile(false);
}

void SoundShaderPreview::handleSelectionChange()
{
	std::string selectedFile = getSelectedSoundFile();

	// Set the sensitivity of the playbutton accordingly
	_playButton->Enable(!selectedFile.empty());
	_playLoopedButton->Enable(!selectedFile.empty());
}

void SoundShaderPreview::onPlay(wxCommandEvent& ev)
{
	playSelectedFile(false);
}

void SoundShaderPreview::onPlayLooped(wxCommandEvent& ev)
{
	playSelectedFile(true);
}

void SoundShaderPreview::playSelectedFile(bool loop)
{
	_statusLabel->SetLabel("");

	std::string selectedFile = getSelectedSoundFile();

	if (!selectedFile.empty())
	{
		// Pass the call to the sound manager
		if (!GlobalSoundManager().playSound(selectedFile, loop))
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

std::string SoundShaderPreview::getDurationOrPlaceholder(const std::string& soundFile)
{
    {
        std::lock_guard<std::mutex> lock(_durationsLock);

        auto found = _durations.find(soundFile);

        if (found != _durations.end())
        {
            return getDurationString(found->second);
        }
    }

    // No duration known yet, queue a task
    loadFileDurationAsync(soundFile);
    return "--:--";
}

void SoundShaderPreview::loadFileDurationAsync(const std::string& soundFile)
{
    _durationQueries.enqueue([this, soundFile] // copy strings into lambda
    {
        try
        {
            // In case the duration has been calculated in the meantime
            // check if there's still need for this task to run
            float duration = -1.0f;
            {
                std::lock_guard<std::mutex> lock(_durationsLock);

                auto existing = _durations.find(soundFile);
                
                if (existing != _durations.end())
                {
                    duration = existing->second;
                }
            }

            if (duration < 0)
            {
                // Duration still unknown, run the query
                duration = GlobalSoundManager().getSoundFileDuration(soundFile);

                // Store the duration in the local cache
                std::lock_guard<std::mutex> lock(_durationsLock);
                _durations[soundFile] = duration;
            }

            if (_isShuttingDown)
            {
                // Don't dispatch anything if we're shutting down
                return;
            }

            // Dispatch to UI thread when we're done
            GetUserInterfaceModule().dispatch([this, soundFile, duration]()
            {
                // Load into treeview
                auto item = _listStore->FindString(soundFile, _columns.soundFile);

                if (item.IsOk())
                {
                    wxutil::TreeModel::Row row(item, *_listStore);
                    row[_columns.duration] = getDurationString(duration);
                    row.SendItemChanged();
                }
            });
        }
        catch (const std::out_of_range& ex)
        {
            rError() << "Cannot query sound file duration of " << soundFile 
                << ": " << ex.what() << std::endl;
        }
    });
}

} // namespace ui
