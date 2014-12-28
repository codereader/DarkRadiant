#pragma once

#include "iradiant.h"
#include "ieclass.h"

#include "wxutil/preview/ModelPreview.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeModel.h"
#include "wxutil/TreeView.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/PanedPosition.h"

#include <memory>

namespace ui
{

class EntityClassChooser;
typedef std::shared_ptr<EntityClassChooser> EntityClassChooserPtr;

/**
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location.
 */
class EntityClassChooser :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			name(add(wxutil::TreeModel::Column::IconText)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column isFolder;
	};

private:
	TreeColumns _columns;

	// Tree model holding the classnames
	wxutil::TreeModel::Ptr _treeStore;
	wxutil::TreeView* _treeView;

    // Delegated object for loading entity classes in a separate thread
    class ThreadedEntityClassLoader;
    std::unique_ptr<ThreadedEntityClassLoader> _eclassLoader; // PIMPL idiom

	// Last selected classname
	std::string _selectedName;

    // Class we should select when the treemodel is populated
    std::string _classToHighlight;

	// Model preview widget
    wxutil::ModelPreviewPtr _modelPreview;

	wxutil::PanedPosition _panedPosition;

private:
	// Constructor. Creates the GTK widgets.
	EntityClassChooser();

    void setTreeViewModel();
    void getEntityClassesFromLoader();
	void loadEntityClasses();

	// Widget construction helpers
	void setupTreeView();

	// Update the usage panel with information from the provided entityclass
	void updateUsageInfo(const std::string& eclass);

	// Updates the member variables based on the current tree selection
	void updateSelection();

	// Button callbacks
	void onCancel(wxCommandEvent& ev);
	void onOK(wxCommandEvent& ev);
	void onSelectionChanged(wxDataViewEvent& ev);
	void onDeleteEvent(wxCloseEvent& ev);
	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);

	// This is where the static shared_ptr of the singleton instance is held.
	static EntityClassChooserPtr& InstancePtr();

public:
	// Public accessor to the singleton instance
	static EntityClassChooser& Instance();

	// Sets the tree selection to the given entity class
	void setSelectedEntityClass(const std::string& eclass);

	// Sets the tree selection to the given entity class
	const std::string& getSelectedEntityClass() const;

	virtual int ShowModal();

	/**
	 * Convenience function:
	 * Display the dialog and block awaiting the selection of an entity class,
	 * which is returned to the caller. If the dialog is cancelled or no
	 * selection is made, and empty string will be returned.
	 */
	static std::string chooseEntityClass(const std::string& preselectEclass = std::string());

	void onRadiantShutdown();
};

} // namespace ui
