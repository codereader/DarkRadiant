#pragma once

#include <string>
#include <memory>
#include "wxutil/TreeModel.h"
#include "wxutil/TreeView.h"

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

	// The currently "previewed" soundshader
	std::string _soundShader;

	// Treemodel definition
	struct SoundListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		SoundListColumns() : 
			shader(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column shader; // soundshader name
	};

	SoundListColumns _columns;

public:
	SoundShaderPreview(wxWindow* parent);

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
};

} // namespace ui
