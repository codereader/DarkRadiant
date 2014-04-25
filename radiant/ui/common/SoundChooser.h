#pragma once

#include "gtkutil/dialog/DialogBase.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/TreeView.h"

#include "ui/common/SoundShaderPreview.h"
#include <string>

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser :
	public wxutil::DialogBase
{
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			displayName(add(wxutil::TreeModel::Column::IconText)),
			shaderName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Bool))
		{}

		wxutil::TreeModel::Column displayName;
		wxutil::TreeModel::Column shaderName;
		wxutil::TreeModel::Column isFolder;
	};

private:
	TreeColumns _columns;

	// Tree store for shaders, and the tree selection
	wxutil::TreeModel* _treeStore;
	wxutil::TreeView* _treeView;

	// The preview widget group
	SoundShaderPreview* _preview;

	// Last selected shader
	std::string _selectedShader;

private:

	// Widget construction
	wxWindow* createTreeView(wxWindow* parent);

	// callbacks
	void _onSelectionChange(wxDataViewEvent& ev);

public:

	/**
	 * Constructor creates widgets.
	 */
	SoundChooser();

	// Retrieve the selected sound shader
	const std::string& getSelectedShader() const;

	// Set the selected sound shader, and focuses the treeview to the new selection
	void setSelectedShader(const std::string& shader);

	virtual int ShowModal();

	// Run the dialog and return the selected shader - this will be empty if the user clicks cancel
	static std::string ChooseSound(const std::string& preSelectedShader = std::string());
};

} // namespace
