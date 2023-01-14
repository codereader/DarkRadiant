#pragma once

#include "decl/DeclarationSelectorDialog.h"

namespace wxutil
{

/**
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location.
 */
class EntityClassChooser final :
    public DeclarationSelectorDialog
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
    EntityClassChooser(Purpose purpose);

    static std::string GetDialogTitle(Purpose purpose);

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
