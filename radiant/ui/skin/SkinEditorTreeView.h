#pragma once

#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

/**
 * Specialised skin declaration tree view used in the Skin Editor
 * Monitors the decl manager signals for newly created or renamed/removed skins
 * and automatically adjusts the tree elements.
 */
class SkinEditorTreeView :
    public wxutil::DeclarationTreeView
{
private:
    constexpr static const char* const SKIN_ICON = "icon_skin.png";
    const Columns& _columns;

public:
    SkinEditorTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE) :
        DeclarationTreeView(parent, decl::Type::Skin, columns, style),
        _columns(columns)
    {
        
    }

    virtual void Populate()
    {
        DeclarationTreeView::Populate(
            std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Skin, _columns, SKIN_ICON)
        );
    }
};

}
