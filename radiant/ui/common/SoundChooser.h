#pragma once

#include "ui/iresourcechooser.h"
#include "wxutil/decl/DeclarationSelectorDialog.h"

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser :
	public wxutil::DeclarationSelectorDialog,
	public IResourceChooser
{
public:
	SoundChooser(wxWindow* parent = nullptr);

	// Run the dialog and return the selected shader - this will be empty if the user clicks cancel
	std::string chooseResource(const std::string& preselected = std::string()) override;

	void destroyDialog() override;
};

} // namespace
