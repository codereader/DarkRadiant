#pragma once

#include "ui/iresourcechooser.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/WindowPosition.h"
#include "ui/common/SoundShaderSelector.h"

#include "SoundShaderPreview.h"
#include <string>
#include <sigc++/connection.h>

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser :
	public wxutil::DialogBase,
	public IResourceChooser
{
private:
    SoundShaderSelector* _selector;

	// The preview widget group
	SoundShaderPreview* _preview;

    sigc::connection _shadersReloaded;

    wxutil::WindowPosition _windowPosition;

private:
    void loadSoundShaders();

	// callbacks
	void _onItemActivated(wxDataViewEvent& ev);
    void _onReloadSounds(wxCommandEvent& ev);

    void onShadersReloaded();

public:
	SoundChooser(wxWindow* parent = nullptr);

	// Retrieve the selected sound shader
	std::string getSelectedShader() const;

	// Set the selected sound shader, and focuses the treeview to the new selection
	void setSelectedShader(const std::string& shader);

	int ShowModal() override;

	// Run the dialog and return the selected shader - this will be empty if the user clicks cancel
	std::string chooseResource(const std::string& preselected = std::string()) override;

	void destroyDialog() override;
};

} // namespace
