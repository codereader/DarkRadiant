#pragma once

#include "dialog/DialogBase.h"
#include "PanedPosition.h"
#include "WindowPosition.h"

#include <sigc++/connection.h>
#include <wx/dataview.h>

namespace wxutil
{

class EntityClassSelector;

/**
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location.
 */
class EntityClassChooser final :
    public DialogBase
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
    EntityClassSelector* _selector;

    WindowPosition _windowPosition;
    PanedPosition _panedPosition;

    sigc::connection _defsReloaded;

    wxButton* _affirmativeButton;

private:
    EntityClassChooser(Purpose purpose);
    ~EntityClassChooser();

    void loadEntityClasses();

    wxWindow* setupSelector(wxWindow * parent);

    // Updates the member variables based on the current tree selection
    void updateSelection();

    // Button callbacks
    void onSelectionChanged(wxDataViewEvent& ev);
    void onDeleteEvent(wxCloseEvent& ev);

    // Sets the tree selection to the given entity class
    void setSelectedEntityClass(const std::string& eclass);

    // Sets the tree selection to the given entity class
    std::string getSelectedEntityClass() const;

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
