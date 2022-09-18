#pragma once

#include <sigc++/connection.h>
#include "imodel.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include <string>

#include "ui/modelselector/MaterialsList.h"
#include "wxutil/dataview/ResourceTreeView.h"

namespace ui
{

class SkinSelector;

/** Dialog to allow selection of skins for a model. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser :
	public wxutil::DialogBase
{
private:
    SkinSelector* _selector;
    MaterialsList* _materialsList;

	// The model name to use for skin matching
	std::string _model;

	// The last skin selected, and the original (previous) skin
	std::string _lastSkin;
	std::string _prevSkin;

	// Model preview widget
    wxutil::ModelPreviewPtr _preview;

    sigc::connection _modelLoadedConn;

private:
	// Constructor creates widgets
	SkinChooser();

	// Widget creation functions
	void populateWindow();

	// Populate the tree with skins
	void populateSkins();

	// callbacks
	void _onSelChanged(wxDataViewEvent& ev);
    void _onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev);

	// Retrieve the currently selected skin
	std::string getSelectedSkin();
    void setSelectedSkin(const std::string& skin);

    void handleSelectionChange();
    void updateMaterialsList();

    void _onItemActivated( wxDataViewEvent& ev );
    void _onPreviewModelLoaded(const model::ModelNodePtr& model);

public:

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
