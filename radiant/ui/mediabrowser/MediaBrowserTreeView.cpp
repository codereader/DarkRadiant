#include "MediaBrowserTreeView.h"

#include "i18n.h"
#include "icommandsystem.h"

#include "TextureDirectoryLoader.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

namespace
{
    constexpr const char* const LOAD_TEXTURE_TEXT = N_("Load in Textures view");
    constexpr const char* const LOAD_TEXTURE_ICON = "textureLoadInTexWindow16.png";
     
    constexpr const char* const APPLY_TEXTURE_TEXT = N_("Apply to selection");
    constexpr const char* const APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";
     
    constexpr const char* const OPEN_IN_MATERIAL_EDITOR_TEXT = N_("Open in Material Editor");
    constexpr const char* const OPEN_IN_MATERIAL_EDITOR_ICON = "edit.png";
     
    constexpr const char* const SELECT_ITEMS = N_("Select elements using this shader");
    constexpr const char* const DESELECT_ITEMS = N_("Deselect elements using this shader");

    constexpr const char* const TEXTURE_ICON = "icon_texture.png";
}

MediaBrowserTreeView::MediaBrowserTreeView(wxWindow* parent) :
    MaterialTreeView(parent)
{
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &MediaBrowserTreeView::_onTreeViewItemActivated, this);
}

void MediaBrowserTreeView::PopulateContextMenu(wxutil::PopupMenu& popupMenu)
{
    // Construct the popup context menu
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_(LOAD_TEXTURE_TEXT), LOAD_TEXTURE_ICON),
        std::bind(&MediaBrowserTreeView::_onLoadInTexView, this),
        std::bind(&MediaBrowserTreeView::_testLoadInTexView, this)
    );
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_(APPLY_TEXTURE_TEXT), APPLY_TEXTURE_ICON),
        std::bind(&MediaBrowserTreeView::_onApplyToSel, this),
        std::bind(&MediaBrowserTreeView::_testSingleTexSel, this)
    );
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_(OPEN_IN_MATERIAL_EDITOR_TEXT), OPEN_IN_MATERIAL_EDITOR_ICON),
        [this]()
        {
            GlobalCommandSystem().executeCommand("MaterialEditor", cmd::ArgumentList{ GetSelectedDeclName() });
        },
        std::bind(&MediaBrowserTreeView::_testSingleTexSel, this)
    );
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_(SELECT_ITEMS), TEXTURE_ICON),
        std::bind(&MediaBrowserTreeView::_onSelectItems, this, true),
        std::bind(&MediaBrowserTreeView::_testSingleTexSel, this)
    );
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_(DESELECT_ITEMS), TEXTURE_ICON),
        std::bind(&MediaBrowserTreeView::_onSelectItems, this, false),
        std::bind(&MediaBrowserTreeView::_testSingleTexSel, this)
    );

    MaterialTreeView::PopulateContextMenu(popupMenu);
}

void MediaBrowserTreeView::_onLoadInTexView()
{
    // Use a TextureDirectoryLoader functor to search the directory. This
    // may throw an exception if cancelled by user.
    TextureDirectoryLoader loader(GetSelectedFullname());

    try
    {
        GlobalMaterialManager().foreachShaderName(std::bind(&TextureDirectoryLoader::visit, &loader, std::placeholders::_1));
    }
    catch (wxutil::ModalProgressDialog::OperationAbortedException&)
    {
        // Ignore the error and return from the function normally
    }
}

bool MediaBrowserTreeView::_testLoadInTexView()
{
    // "Load in textures view" requires a directory selection
    if (IsDirectorySelected())
        return true;
    else
        return false;
}

void MediaBrowserTreeView::_onApplyToSel()
{
    // Pass shader name to the selection system
    GlobalCommandSystem().executeCommand("SetShaderOnSelection", GetSelectedDeclName());
}

bool MediaBrowserTreeView::_testSingleTexSel()
{
    return !IsDirectorySelected() && !GetSelectedDeclName().empty();
}

void MediaBrowserTreeView::_onSelectItems(bool select)
{
    std::string shaderName = GetSelectedDeclName();

    if (select)
    {
        GlobalCommandSystem().executeCommand("SelectItemsByShader", shaderName);
    }
    else
    {
        GlobalCommandSystem().executeCommand("DeselectItemsByShader", shaderName);
    }
}

void MediaBrowserTreeView::_onTreeViewItemActivated(wxDataViewEvent& ev)
{
    std::string selection = GetSelectedDeclName();

    if (!IsDirectorySelected() && !selection.empty())
    {
        // Pass shader name to the selection system
        GlobalCommandSystem().executeCommand("SetShaderOnSelection", selection);
    }
}

}
