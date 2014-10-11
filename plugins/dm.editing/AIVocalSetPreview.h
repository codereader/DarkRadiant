#pragma once

#include "ieclass.h"
#include <wx/panel.h>

class wxStaticText;
class wxButton;

namespace ui
{

/**
 * greebo: This class provides the UI elements to listen
 * to a a given AI vocal set. On clicking the playback button
 * a random sound is chosen from the vocal set.
 *
 * Use the getWidget() method to pack this into a
 * parent container.
 */
class AIVocalSetPreview :
	public wxPanel
{
private:
	wxButton* _playButton;
	wxButton* _stopButton;
	wxStaticText* _statusLabel;

	// The currently "previewed" vocal set
	IEntityClassPtr _vocalSetDef;

	typedef std::vector<std::string> SoundShaderList;
	SoundShaderList _setShaders;

public:
	AIVocalSetPreview(wxWindow* parent);

	/**
	 * greebo: Sets the vocal set to preview. Set NULL to disable this panel.
	 */
	void setVocalSetEclass(const IEntityClassPtr& vocalSetDef);

private:
	/**
	 * greebo: Returns a random file from the selected vocal set.
	 * @returns: the filename as defined in one of the shaders or "".
	 */
	std::string getRandomSoundFile();

	/** greebo: Creates the control widgets (play button) and such.
	 */
	void createControlPanel();

	/** greebo: Updates the list according to the active soundshader
	 */
	void update();

	// Callbacks
	void onPlay(wxCommandEvent& ev);
	void onStop(wxCommandEvent& ev);
};

} // namespace ui
