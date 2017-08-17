#pragma once

#include "MaterialsList.h"

#include <sigc++/connection.h>

#include "modelskin.h"
#include "iradiant.h"
#include "iuimanager.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/TreeModelFilter.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/KeyValueTable.h"

#include <string>

namespace ui
{

/**
 * Data structure containing the model, the skin name and the options to be returned from
 * the Model Selector.
 */
struct ModelSelectorResult
{
	// Model and skin strings
	std::string model;
	std::string skin;

	// options
	bool createClip;

	// Constructor
	ModelSelectorResult(const std::string& m, const std::string& s, const bool clip)
	: model(m), skin(s), createClip(clip) {}
};

class ModelPopulator;

class ModelSelector;
typedef std::shared_ptr<ModelSelector> ModelSelectorPtr;

/// Dialog for browsing and selecting a model and/or skin
class ModelSelector :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			filename(add(wxutil::TreeModel::Column::IconText)),
			vfspath(add(wxutil::TreeModel::Column::String)),
			skin(add(wxutil::TreeModel::Column::String)),
            isSkin(add(wxutil::TreeModel::Column::Boolean)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column filename;	// e.g. "chair1.lwo"
		wxutil::TreeModel::Column vfspath;	// e.g. "models/darkmod/props/chair1.lwo"
		wxutil::TreeModel::Column skin;		// e.g. "chair1_brown_wood", or "" for no skin
        wxutil::TreeModel::Column isSkin;	// TRUE if this is a skin entry, FALSE if actual model or folder
		wxutil::TreeModel::Column isFolder;	// whether this is a folder
	};

private:
	wxPanel* _dialogPanel;

	TreeColumns _columns;

	// Model preview widget
    wxutil::ModelPreviewPtr _modelPreview;

	// Tree store containing model names (including skins)
	wxutil::TreeModel::Ptr _treeStore;

    // Tree model filter for dynamically excluding the skins
    wxutil::TreeModelFilter::Ptr _treeModelFilter;

    // Main tree view with model hierarchy
	wxutil::TreeView::Ptr _treeView;

    // Key/value table for model information
    wxutil::KeyValueTable* _infoTable;

    // Materials list table
    MaterialsList* _materialsList;

	// The window position tracker
	wxutil::WindowPosition _position;
	wxutil::PanedPosition _panedPosition;

	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	std::string _lastSkin;

	// TRUE if the treeview has been populated
	bool _populated;
    std::unique_ptr<ModelPopulator> _populator;
    bool _showSkins;

    // The model to highlight on show
    std::string _preselectedModel;

    // Whether to show advanced options panel
    bool _showOptions;

    wxIcon _modelIcon;
    wxDataViewItem _progressItem;

	sigc::connection _modelsReloadedConn;
	sigc::connection _skinsReloadedConn;

private:
	// Private constructor, creates widgets
	ModelSelector();

	// Home of the static instance
	static ModelSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static ModelSelectorPtr& InstancePtr();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	ModelSelectorResult showAndBlock(const std::string& curModel,
                                     bool showOptions,
                                     bool showSkins);

	// Helper functions to configure GUI components
    void setupAdvancedPanel(wxWindow* parent);
    void setupTreeView(wxWindow* parent);
    void preSelectModel();

	// Populate the tree view with models
	void populateModels();

	void showInfoForSelectedModel();

	// Return the value from the selected column, or an empty string if nothing selected
	std::string getSelectedValue(const wxutil::TreeModel::Column& col);

	void cancelDialog();

	// wx callbacks
	void onOK(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onReloadModels(wxCommandEvent& ev);
	void onReloadSkins(wxCommandEvent& ev);
	void onIdleReloadTree(wxIdleEvent& ev);
    void onTreeStorePopulationProgress(wxutil::TreeModel::PopulationProgressEvent& ev);
    void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);

	// Update the info table with information from the currently-selected model, and
	// update the displayed model.
	void onSelectionChanged(wxDataViewEvent& ev);

	// Connected to the ModelCache/SkinCache signal, fires after the refresh commands are done
	void onSkinsOrModelsReloaded();

protected:
	void _onDeleteEvent(wxCloseEvent& ev);

public:
	/**
	 * Display the Model Selector instance, constructing it on first use, and
	 * return the VFS path of the model selected by the user. When the
	 * ModelSelector is displayed it will enter a recursive gtk_main loop,
	 * blocking execution of the calling function until destroyed.
	 *
	 * @curModel: the name of the currently selected model the tree will browse to
	 *            Leave this empty to leave the treeview focus where it was when
	 *            the dialog was closed.
	 *
	 * @showOptions: whether to show the advanced options tab.
	 */
	static ModelSelectorResult chooseModel(
			const std::string& curModel = "", bool showOptions = true, bool showSkins = true);

	// Starts the background population thread
    static void Populate();

	void onRadiantShutdown();
};

}
