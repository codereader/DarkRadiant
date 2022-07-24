#pragma once

#include "ResourceTreeView.h"

namespace wxutil
{

class DeclarationTreeView :
    public ResourceTreeView
{
private:
    decl::Type _declType;

public:
    DeclarationTreeView(wxWindow* parent, decl::Type declType, const Columns& columns, long style = wxDV_SINGLE);
    DeclarationTreeView(wxWindow* parent, decl::Type declType, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);

protected:
    void PopulateContextMenu(wxutil::PopupMenu& popupMenu) override;

private:
    void _onShowDefinition();
    bool _showDefinitionEnabled();
};

}
