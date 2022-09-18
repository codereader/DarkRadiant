#pragma once

#include "decl/DeclarationSelectorDialog.h"
#include "ui/iwindowstate.h"

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
    public DeclarationSelectorDialog,
    public ui::IPersistableObject
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

    sigc::connection _defsReloaded;

    bool _restoreSelectionFromRegistry;

private:
    EntityClassChooser(Purpose purpose);
    ~EntityClassChooser() override;

    void loadEntityClasses();

    EntityClassSelector* setupSelector(wxWindow * parent);

public:
    void loadFromPath(const std::string& registryKey) override;
    void saveToPath(const std::string& registryKey) override;

    void SetSelectedDeclName(const std::string& declName) override;

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
