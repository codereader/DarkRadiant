#pragma once

#include "modelskin.h"

#include "iradiant.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include <string>

#include "wxutil/TreeModel.h"

namespace wxutil { class TreeView; }

namespace ui
{

class SkinChooser;
typedef std::shared_ptr<SkinChooser> SkinChooserPtr;

/** Dialog to allow selection of skins for a model entity. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser :
	public wxutil::DialogBase
{
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			displayName(add(wxutil::TreeModel::Column::IconText)),
			fullName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column displayName;
		wxutil::TreeModel::Column fullName;
		wxutil::TreeModel::Column isFolder;
	};

private:
	TreeColumns _columns;

	// Tree store, view and selection
	wxutil::TreeModel::Ptr _treeStore;
	wxutil::TreeView* _treeView;

	// The model name to use for skin matching
	std::string _model;

	// The last skin selected, and the original (previous) skin
	std::string _lastSkin;
	std::string _prevSkin;

	// Model preview widget
    wxutil::ModelPreviewPtr _preview;

private:
	// Constructor creates widgets
	SkinChooser();

	// Widget creation functions
	void populateWindow();

	// Show the dialog and block until selection is made
	std::string showAndBlock(const std::string& model, const std::string& prev);

	// Populate the tree with skins
	void populateSkins();

	// callbacks
	void _onSelChanged(wxDataViewEvent& ev);

	// Contains the static instance
	static SkinChooser& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static SkinChooserPtr& InstancePtr();

	// Retrieve the currently selected skin
	std::string getSelectedSkin();
    void setSelectedSkin(const std::string& skin);

    void handleSelectionChange();

public:
	void onRadiantShutdown();

	// Override Dialogbase
	int ShowModal();

	/** Display the dialog and return the skin chosen by the user, or an empty
	 * string if no selection was made. This static method enters are recursive
	 * main loop during skin selection.
	 *
	 * @param model
	 * The full VFS path of the model for which matching skins should be found.
	 *
	 * @param prevSkin
	 * The current skin set on the model, so that the original can be returned
	 * if the dialog is cancelled.
	 */
	static std::string chooseSkin(const std::string& model,
								  const std::string& prevSkin);
};

}
