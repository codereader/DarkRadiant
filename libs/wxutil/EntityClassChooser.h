#pragma once

#include "iradiant.h"
#include "ieclass.h"

#include "preview/ModelPreview.h"
#include "dialog/DialogBase.h"
#include "dataview/ResourceTreeView.h"
#include "XmlResourceBasedWidget.h"
#include "PanedPosition.h"

#include <memory>
#include <sigc++/connection.h>

namespace wxutil
{

class EntityClassChooser;
typedef std::shared_ptr<EntityClassChooser> EntityClassChooserPtr;

/**
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location.
 */
class EntityClassChooser :
    public DialogBase,
    private XmlResourceBasedWidget
{
private:
    ResourceTreeView::Columns _columns;
    ResourceTreeView* _treeView;

    // Last selected classname
    std::string _selectedName;

    // Model preview widget
    ModelPreviewPtr _modelPreview;

    PanedPosition _panedPosition;

    sigc::connection _defsReloaded;

private:
    EntityClassChooser();

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
    
    void onMainFrameShuttingDown();
    
    // This is where the static shared_ptr of the singleton instance is held.
    static EntityClassChooserPtr& InstancePtr();

public:
    // Public accessor to the singleton instance
    static EntityClassChooser& Instance();

    // Sets the tree selection to the given entity class
    void setSelectedEntityClass(const std::string& eclass);

    // Sets the tree selection to the given entity class
    const std::string& getSelectedEntityClass() const;

    int ShowModal() override;

    /**
     * Convenience function:
     * Display the dialog and block awaiting the selection of an entity class,
     * which is returned to the caller. If the dialog is cancelled or no
     * selection is made, and empty string will be returned.
     */
    static std::string chooseEntityClass(const std::string& preselectEclass = std::string());
};

} // namespace
