#pragma once

#include "iresourcechooser.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeView.h"
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
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			displayName(add(wxutil::TreeModel::Column::IconText)),
			shaderName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column displayName;
		wxutil::TreeModel::Column shaderName;
		wxutil::TreeModel::Column isFolder;
	};

private:
	TreeColumns _columns;

	// Tree store for shaders, and the tree selection
	wxutil::TreeModel::Ptr _treeStore;
	wxutil::TreeView* _treeView;

    class ThreadedSoundShaderLoader;
    std::unique_ptr<ThreadedSoundShaderLoader> _shaderLoader; // PIMPL idiom

	// The preview widget group
	SoundShaderPreview* _preview;

	// Last selected shader
	std::string _selectedShader;

    // For memorising what we need to pre-select once the population is done
    std::string _shaderToSelect;

	// Context menu
	wxutil::PopupMenuPtr _popupMenu;

    bool _loadingShaders;

    sigc::connection _shadersReloaded;

private:

	// Widget construction
	wxWindow* createTreeView(wxWindow* parent);

    void setTreeViewModel();
    void loadSoundShaders();
    void handleSelectionChange();

	// callbacks
	void _onSelectionChange(wxDataViewEvent& ev);
	void _onItemActivated(wxDataViewEvent& ev);
    void _onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);
	void _onContextMenu(wxDataViewEvent& ev);
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
