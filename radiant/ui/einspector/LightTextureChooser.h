#pragma once

#include "ui/common/ShaderSelector.h"
#include "wxutil/dialog/DialogBase.h"

#include <string>

// Forward decls
class Material;

namespace ui
{

/**
 * A dialog containing a ShaderSelector widget combo and OK/Cancel
 * buttons.
 */
class LightTextureChooser :
	public wxutil::DialogBase,
	public ShaderSelector::Client
{
private:
	// The ShaderSelector widget group, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector* _selector;

	// The user's texture selection
	std::string _selectedTexture;

private:
	// Widget construction helpers
	void createButtons(wxPanel* mainPanel, wxBoxSizer* dialogVBox);

public:
	/**
	 * Construct the dialog window and its contents.
	 */
	LightTextureChooser();

	/**
	 * Get the currently selected shader.
	 *
	 * @returns
	 * The selected shader name, or "" if there was no selection or the dialog
	 * was cancelled.
	 */
	std::string getSelectedTexture();

    /**
     * Highlights the given texture in the tree.
     */
    void setSelectedTexture(const std::string& textureName);

	/** greebo: Gets called upon selection change, updates the infostore
	 * 			of the contained ShaderSelector helper class accordingly.
	 */
	void shaderSelectionChanged(const std::string& shaderName,
								wxutil::TreeModel& listStore);

};

} // namespace ui
