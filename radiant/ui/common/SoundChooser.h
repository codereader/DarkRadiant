#pragma once

#include "iresourcechooser.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/menu/PopupMenu.h"

#include "SoundShaderPreview.h"
#include <memory>
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
    wxutil::ResourceTreeView::Columns _columns;
	wxutil::ResourceTreeView* _treeView;

	// The preview widget group
	SoundShaderPreview* _preview;

	// Last selected shader
	std::string _selectedShader;

    sigc::connection _shadersReloaded;

private:

	// Widget construction
    wxutil::ResourceTreeView* createTreeView(wxWindow* parent);

    void loadSoundShaders();
    void handleSelectionChange();

	// callbacks
	void _onSelectionChange(wxDataViewEvent& ev);
	void _onItemActivated(wxDataViewEvent& ev);
    void _onReloadSounds(wxCommandEvent& ev);

	void onShowShaderDefinition();
	bool testShowShaderDefinition();

    void onShadersReloaded();

public:

	/**
	 * Constructor creates widgets.
	 */
	SoundChooser(wxWindow* parent = nullptr);

	// Retrieve the selected sound shader
	const std::string& getSelectedShader() const;

	// Set the selected sound shader, and focuses the treeview to the new selection
	void setSelectedShader(const std::string& shader);

	virtual int ShowModal() override;

	// Run the dialog and return the selected shader - this will be empty if the user clicks cancel
	virtual std::string chooseResource(const std::string& preselected = std::string()) override;

	virtual void destroyDialog() override;
};

} // namespace
