#pragma once

#include "iradiant.h"
#include "ieclass.h"

#include "preview/ModelPreview.h"
#include "dialog/DialogBase.h"
#include "dataview/ResourceTreeView.h"
#include "dataview/ResourceTreeViewToolbar.h"
#include "XmlResourceBasedWidget.h"
#include "PanedPosition.h"
#include "WindowPosition.h"

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
class EntityClassChooser final :
    public DialogBase,
    private XmlResourceBasedWidget
{
public:
    // Enumeration of possible modes of this dialog
    // Influences the dialog title and button labels
    enum class Purpose
    {
        AddEntity,
        ConvertEntity,
        SelectClassname,
    };

private:
    ResourceTreeView::Columns _columns;
    ResourceTreeView* _treeView;
    ResourceTreeViewToolbar* _treeViewToolbar;

    // Last selected classname
    std::string _selectedName;

    // Model preview widget
    ModelPreviewPtr _modelPreview;

    WindowPosition _windowPosition;
    PanedPosition _panedPosition;

    sigc::connection _defsReloaded;

private:
    EntityClassChooser(Purpose purpose);
    ~EntityClassChooser();

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

    // Sets the tree selection to the given entity class
    void setSelectedEntityClass(const std::string& eclass);

    // Sets the tree selection to the given entity class
    const std::string& getSelectedEntityClass() const;

    void _onItemActivated( wxDataViewEvent& ev );

    // Overridden from wxDialog
    int ShowModal() override;

public:

    /**
     * \brief Construct and show the dialog to choose an entity class,
     * returning the result.
     * 
     * \param purpose
     * The scenario this dialog has been requested for
     *
     * \param preselectEclass
     * Optional initial class to locate and highlight in the tree after the
     * dialog is shown.
     */
    static std::string ChooseEntityClass(Purpose purpose, const std::string& preselectEclass = {});
};

} // namespace
