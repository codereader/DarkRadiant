#pragma once

#include "MaterialsList.h"
#include "ModelTreeView.h"

#include <sigc++/connection.h>

#include "modelskin.h"
#include "iradiant.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/dataview/TreeModelFilter.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dataview/KeyValueTable.h"

#include <string>
#include <wx/tglbtn.h>

namespace ui
{

class ModelPopulator;

class ModelSelector;
typedef std::shared_ptr<ModelSelector> ModelSelectorPtr;

/// Dialog for browsing and selecting a model and/or skin
class ModelSelector: public wxutil::DialogBase, private wxutil::XmlResourceBasedWidget
{
public:
    /**
     * Data structure containing the kind (model/eclass),
     * the name of the object and a possible skin name.
     * Als contains the options (whether to create Monsterclip).
     */
    struct Result
    {
        enum class ObjectKind
        {
            Model,
            EntityClass,
        };

        // The object to create
        ObjectKind objectKind;

        // Eclass/Model name
        std::string name;

        // The skin of the model (if not empty)
        std::string skin;

        // Model creation options
        bool createClip = false;
    };

private:
	wxPanel* _dialogPanel;

	// Model preview widget
    std::unique_ptr<wxutil::ModelPreview> _modelPreview;

    // Main tree view with model hierarchy
	ModelTreeView* _treeView;
    wxToggleButton* _showSkinsBtn = nullptr;

    // The model name which the info panels are currently displaying info for
    std::string _infoModel;
    std::string _infoSkin;

    // Key/value table for model information
    wxutil::KeyValueTable* _infoTable;

    // Materials list table
    MaterialsList* _materialsList;

    struct RelatedEntityColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        RelatedEntityColumns() :
            eclassName(add(wxutil::TreeModel::Column::String)),
            skin(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column eclassName;
        wxutil::TreeModel::Column skin;
    };

    RelatedEntityColumns _relatedEntityColumns;
    wxutil::TreeModel::Ptr _relatedEntityStore;
    wxutil::TreeView* _relatedEntityView;
    wxutil::PopupMenuPtr _relatedEntityContextMenu;

	// The window position tracker
	wxutil::WindowPosition _position;
	wxutil::PanedPosition _panedPosition;

    // Whether to show advanced options panel
    bool _showOptions;

	sigc::connection _modelsReloadedConn;
	sigc::connection _skinsReloadedConn;

    Result _result;

private:
	// Private constructor, creates widgets
	ModelSelector();

	// Home of the static instance
	static ModelSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static ModelSelectorPtr& InstancePtr();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	Result showAndBlock(const std::string& curModel, bool showOptions, bool showSkins);

	// Helper functions to configure GUI components
    void setupAdvancedPanel(wxWindow* parent);
    void setupTreeView(wxWindow* parent);
    wxWindow* setupTreeViewToolbar(wxWindow* parent);

	// Populate the tree view with models
	void populateModels();

	void handleSelectionChange();

	void cancelDialog();

	// wx callbacks
	void onOK(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onReloadModels(wxCommandEvent& ev);
	void onReloadSkins(wxCommandEvent& ev);
    void onRescanFolders(wxCommandEvent& ev);
    void onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev);

    // Update the info table with information from the currently-selected model, and
	// update the displayed model.
	void onSelectionChanged(wxDataViewEvent& ev);

	void onRelatedEntitySelectionChange(wxDataViewEvent& ev);
	void onRelatedEntityActivated(wxDataViewEvent& ev);
	void onRelatedEntityContextMenu(wxDataViewEvent& ev);
	void onShowClassDefinition();

	// Connected to the ModelCache/SkinCache signal, fires after the refresh commands are done
	void onSkinsOrModelsReloaded();

	void onModelLoaded(const model::ModelNodePtr& modelNode);

	void onMainFrameShuttingDown();

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
	static Result chooseModel(const std::string& curModel = "", bool showOptions = true, bool showSkins = true);

	// Starts the background population thread
    static void Populate();
};

}
