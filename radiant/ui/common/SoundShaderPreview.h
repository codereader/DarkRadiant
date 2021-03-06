#pragma once

#include <string>
#include <mutex>
#include <map>
#include <memory>
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeView.h"
#include "SequentialTaskQueue.h"

#include <wx/panel.h>

class wxButton;
class wxStaticText;
class wxSizer;

namespace ui
{

/**
 * greebo: This class provides the UI elements to inspect a given
 * sound shader with playback option.
 */
class SoundShaderPreview :
	public wxPanel
{
private:
	// Tree store and view for available sound files, and the tree selection
	wxutil::TreeModel::Ptr _listStore;

	wxutil::TreeView* _treeView;

	wxButton* _playButton;
	wxButton* _playLoopedButton;
	wxButton* _stopButton;
	wxStaticText* _statusLabel;

	wxSizer* _shaderDescriptionSizer;
	wxStaticText* _shaderNameLabel;
	wxStaticText* _shaderFileLabel;

	// The currently "previewed" soundshader
	std::string _soundShader;

	// Treemodel definition
	struct SoundListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		SoundListColumns() : 
            soundFile(add(wxutil::TreeModel::Column::String)),
			duration(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column soundFile; // soundFile path
		wxutil::TreeModel::Column duration; // duration
	};

	SoundListColumns _columns;

    std::mutex _durationsLock;

    // Already known durations
    std::map<std::string, float> _durations;

    // Sound file lengths are queried asynchronously
    util::SequentialTaskQueue _durationQueries;

    bool _isShuttingDown;

public:
	SoundShaderPreview(wxWindow* parent);

    ~SoundShaderPreview();

	/** greebo: Sets the soundshader to preview.
	 * 			This updates the preview liststore and treeview.
	 */
	void setSoundShader(const std::string& soundShader);

	/**
	 * Provided a sound shader is assigned, this will pick
	 * a random file from the list and start its playback.
	 */
	void playRandomSoundFile();

private:
	/** greebo: Returns the currently selected sound file (file list)
	 *
	 * @returns: the filename as defined in the shader or "" if nothing selected.
	 */
	std::string getSelectedSoundFile();

	/** greebo: Creates the control widgets (play button) and such.
	 */
	wxSizer* createControlPanel(wxWindow* parent);

	/** greebo: Updates the list according to the active soundshader
	 */
	void update();

	// Callbacks
	void onPlay(wxCommandEvent& ev);
	void onPlayLooped(wxCommandEvent& ev);
	void onStop(wxCommandEvent& ev);
	void onSelectionChanged(wxDataViewEvent& ev);
	void onItemActivated(wxDataViewEvent& ev);

	void playSelectedFile(bool loop);
	void handleSelectionChange();

    void loadFileDurationAsync(const std::string& soundFile);
    std::string getDurationOrPlaceholder(const std::string& soundFile);
};

} // namespace ui
