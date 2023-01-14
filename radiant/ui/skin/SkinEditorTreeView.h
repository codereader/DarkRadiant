#pragma once

#include <sigc++/connection.h>
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

    // Subscriptions to the material manager
    sigc::connection _declCreated;
    sigc::connection _declRemoved;
    sigc::connection _declRenamed;

public:
    SkinEditorTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    ~SkinEditorTreeView() override;

    virtual void Populate();

private:
    void onDeclarationCreated(decl::Type type, const std::string& name);
    void onDeclarationRenamed(decl::Type type, const std::string& oldName, const std::string& newName);
    void onDeclarationRemoved(decl::Type type, const std::string& name);
};

}
