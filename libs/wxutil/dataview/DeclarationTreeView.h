#pragma once

#include "ResourceTreeView.h"

namespace wxutil
{

/**
 * Specialised TreeView used to display lists or trees of Declarations,
 * like Particles, Materials, Skins, etc.
 */
class DeclarationTreeView :
    public ResourceTreeView
{
public:
    // Extends the ResourceTreeView Columns by a declName entry
    struct Columns :
        public wxutil::ResourceTreeView::Columns
    {
        Columns() :
            ResourceTreeView::Columns(),
            declName(add(TreeModel::Column::String))
        {}

        TreeModel::Column declName; // the name used to acquire the IDeclaration reference
    };

private:
    const Columns& _columns;
    decl::Type _declType;

public:
    DeclarationTreeView(wxWindow* parent, decl::Type declType, const Columns& columns, long style = wxDV_SINGLE);
    DeclarationTreeView(wxWindow* parent, decl::Type declType, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);

    // Returns the name of the selected declaration, suitable for lookups in IDeclarationManager
    virtual std::string GetSelectedDeclName();

    virtual void SetSelectedDeclName(const std::string& declName);

protected:
    void PopulateContextMenu(wxutil::PopupMenu& popupMenu) override;

private:
    void _onShowDefinition();
    bool _showDefinitionEnabled();
};

}
