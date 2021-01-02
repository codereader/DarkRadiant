#include "MediaBrowserTreeView.h"

#include "i18n.h"
#include "ifavourites.h"
#include "iuimanager.h"
#include "icommandsystem.h"
#include "ui/common/MaterialDefinitionView.h"
#include "TextureDirectoryLoader.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "string/string.h"
#include "shaderlib.h"

#include <wx/thread.h>
#include <wx/artprov.h>

namespace ui
{

namespace
{
    const char* const OTHER_MATERIALS_FOLDER = N_("Other Materials");

    const char* const LOAD_TEXTURE_TEXT = N_("Load in Textures view");
    const char* const LOAD_TEXTURE_ICON = "textureLoadInTexWindow16.png";

    const char* const APPLY_TEXTURE_TEXT = N_("Apply to selection");
    const char* const APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";

    const char* const SHOW_SHADER_DEF_TEXT = N_("Show Shader Definition");
    const char* const SHOW_SHADER_DEF_ICON = "icon_script.png";

    const char* const SELECT_ITEMS = N_("Select elements using this shader");
    const char* const DESELECT_ITEMS = N_("Deselect elements using this shader");

    const char* const ADD_TO_FAVOURITES = N_("Add to Favourites");
    const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");

    const char* FOLDER_ICON = "folder16.png";
    const char* TEXTURE_ICON = "icon_texture.png";
}

/* Callback functor for processing shader names */
struct ShaderNameCompareFunctor
{
    bool operator()(const std::string& s1, const std::string& s2) const
    {
        // return boost::algorithm::ilexicographical_compare(s1, s2); // slow!
        return string_compare_nocase(s1.c_str(), s2.c_str()) < 0;
    }
};

struct ShaderNameFunctor
{
    // TreeStore to populate
    wxutil::TreeModel& _store;
    const MediaBrowserTreeView::TreeColumns& _columns;
    const std::set<std::string>& _favourites;
    wxDataViewItem _root;

    std::string _otherMaterialsPath;

    // Maps of names to corresponding treemodel items, for both intermediate
    // paths and explicitly presented paths
    typedef std::map<std::string, wxDataViewItem, ShaderNameCompareFunctor> NamedIterMap;
    NamedIterMap _iters;

    wxIcon _folderIcon;
    wxIcon _textureIcon;

    ShaderNameFunctor(wxutil::TreeModel& store, const MediaBrowserTreeView::TreeColumns& columns, const std::set<std::string>& favourites) :
        _store(store),
        _columns(columns),
        _favourites(favourites),
        _root(_store.GetRoot()),
        _otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
    {
        _folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
        _textureIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + TEXTURE_ICON));
    }

    // Recursive add function
    wxDataViewItem& addRecursive(const std::string& path, bool isOtherMaterial)
    {
        // Look up candidate in the map and return it if found
        auto it = _iters.find(path);

        if (it != _iters.end())
        {
            return it->second;
        }

        /* Otherwise, split the path on its rightmost slash, call recursively on the
         * first half in order to add the parent node, then add the second half as
         * a child. Recursive bottom-out is when there is no slash (top-level node).
         */
         // Find rightmost slash
        std::size_t slashPos = path.rfind("/");

        // Call recursively to get parent iter, leaving it at the toplevel if
        // there is no slash
        wxDataViewItem& parIter =
            slashPos != std::string::npos ? addRecursive(path.substr(0, slashPos), isOtherMaterial) : _root;

        // Append a node to the tree view for this child
        wxutil::TreeModel::Row row = _store.AddItem(parIter);

        std::string name = slashPos != std::string::npos ? path.substr(slashPos + 1) : path;

        row[_columns.iconAndName] = wxVariant(wxDataViewIconText(name, _folderIcon));
        row[_columns.leafName] = name;
        row[_columns.fullName] = path;
        row[_columns.isFolder] = true;
        row[_columns.isOtherMaterialsFolder] = isOtherMaterial;
        row[_columns.isFavourite] = false; // folders are not favourites

        // Add a copy of the wxDataViewItem to our hashmap and return it
        std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
            NamedIterMap::value_type(path, row.getItem()));

        return result.first->second;
    }

    void visit(const std::string& name)
    {
        // Find rightmost slash
        std::size_t slashPos = name.rfind("/");

        wxDataViewItem parent;

        if (string::istarts_with(name, GlobalTexturePrefix_get()))
        {
            // Regular texture, ensure parent folder
            parent = slashPos != std::string::npos ? addRecursive(name.substr(0, slashPos), false) : _root;
        }
        else
        {
            // Put it under "other materials", ensure parent folder
            parent = slashPos != std::string::npos ?
                addRecursive(_otherMaterialsPath + "/" + name.substr(0, slashPos), true) :
                addRecursive(_otherMaterialsPath, true);
        }

        // Insert the actual leaf
        wxutil::TreeModel::Row row = _store.AddItem(parent);

        std::string leafName = slashPos != std::string::npos ? name.substr(slashPos + 1) : name;

        bool isFavourite = _favourites.count(name) > 0;

        row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon));
        row[_columns.leafName] = leafName;
        row[_columns.fullName] = name;
        row[_columns.isFolder] = false;
        row[_columns.isOtherMaterialsFolder] = false;
        row[_columns.isFavourite] = isFavourite;

        // Formatting
        row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
    }
};

class MediaPopulator final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const MediaBrowserTreeView::TreeColumns& _columns;

    // The set of favourites
    std::set<std::string> _favourites;

public:
    // Construct and initialise variables
    MediaPopulator(const MediaBrowserTreeView::TreeColumns& columns, wxEvtHandler* finishedHandler) :
        ThreadedResourceTreePopulator(columns, finishedHandler),
        _columns(columns)
    {
        _favourites = GlobalFavouritesManager().getFavourites(decl::Type::Material);
    }

    ~MediaPopulator()
    {
        // Stop the worker while the class is still intact
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        model->SetHasDefaultCompare(false);

        ShaderNameFunctor functor(*model, _columns, _favourites);
        GlobalMaterialManager().foreachShaderName(std::bind(&ShaderNameFunctor::visit, &functor, std::placeholders::_1));
    }

    // Special sort algorithm to sort the "Other Materials" separately
    void SortModel(const wxutil::TreeModel::Ptr& model) override
    {
        // Sort the model while we're still in the worker thread
        model->SortModel([&](const wxDataViewItem& a, const wxDataViewItem& b)
        {
            // Check if A or B are folders
            wxVariant aIsFolder, bIsFolder;
            model->GetValue(aIsFolder, a, _columns.isFolder.getColumnIndex());
            model->GetValue(bIsFolder, b, _columns.isFolder.getColumnIndex());

            if (aIsFolder)
            {
                // A is a folder, check if B is as well
                if (bIsFolder)
                {
                    // A and B are both folders
                    wxVariant aIsOtherMaterialsFolder, bIsOtherMaterialsFolder;

                    model->GetValue(aIsOtherMaterialsFolder, a, _columns.isOtherMaterialsFolder.getColumnIndex());
                    model->GetValue(bIsOtherMaterialsFolder, b, _columns.isOtherMaterialsFolder.getColumnIndex());

                    // Special treatment for "Other Materials" folder, which always comes last
                    if (aIsOtherMaterialsFolder)
                    {
                        return false;
                    }

                    if (bIsOtherMaterialsFolder)
                    {
                        return true;
                    }

                    // Compare folder names
                    // greebo: We're not checking for equality here, shader names are unique
                    wxVariant aName, bName;
                    model->GetValue(aName, a, _columns.leafName.getColumnIndex());
                    model->GetValue(bName, b, _columns.leafName.getColumnIndex());

                    return aName.GetString().CmpNoCase(bName.GetString()) < 0;
                }
                else
                {
                    // A is a folder, B is not, A sorts before
                    return true;
                }
            }
            else
            {
                // A is not a folder, check if B is one
                if (bIsFolder)
                {
                    // A is not a folder, B is, so B sorts before A
                    return false;
                }
                else
                {
                    // Neither A nor B are folders, compare names
                    // greebo: We're not checking for equality here, shader names are unique
                    wxVariant aName, bName;
                    model->GetValue(aName, a, _columns.leafName.getColumnIndex());
                    model->GetValue(bName, b, _columns.leafName.getColumnIndex());

                    return aName.GetString().CmpNoCase(bName.GetString()) < 0;
                }
            }
        });
    }
};

MediaBrowserTreeView::MediaBrowserTreeView(wxWindow* parent) :
    ResourceTreeView(parent, _columns, wxDV_NO_HEADER)
{
    auto* textCol = AppendIconTextColumn(_("Shader"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    SetExpanderColumn(textCol);
    textCol->SetWidth(300);

    AddSearchColumn(_columns.iconAndName);

    // The wxWidgets algorithm sucks at sorting large flat lists of strings,
    // so we do that ourselves
    getTreeModel()->SetHasDefaultCompare(false);

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &MediaBrowserTreeView::_onTreeViewItemActivated, this);
}

const MediaBrowserTreeView::TreeColumns& MediaBrowserTreeView::getColumns() const
{
    return _columns;
}

void MediaBrowserTreeView::setTreeMode(MediaBrowserTreeView::TreeMode mode)
{
    std::string previouslySelectedItem = getSelection();
    
    ResourceTreeView::setTreeMode(mode);

    // Try to select the same item we had as before
    setSelection(previouslySelectedItem);
}

void MediaBrowserTreeView::populate()
{
    ResourceTreeView::populate(std::make_shared<MediaPopulator>(_columns, this));
}

void MediaBrowserTreeView::populateContextMenu(wxutil::PopupMenu& popupMenu)
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
        new wxutil::IconTextMenuItem(_(SHOW_SHADER_DEF_TEXT), SHOW_SHADER_DEF_ICON),
        std::bind(&MediaBrowserTreeView::_onShowShaderDefinition, this),
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

    ResourceTreeView::populateContextMenu(popupMenu);
}

void MediaBrowserTreeView::_onLoadInTexView()
{
    // Use a TextureDirectoryLoader functor to search the directory. This
    // may throw an exception if cancelled by user.
    TextureDirectoryLoader loader(getSelection());

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
    if (isDirectorySelected())
        return true;
    else
        return false;
}

void MediaBrowserTreeView::_onApplyToSel()
{
    // Pass shader name to the selection system
    GlobalCommandSystem().executeCommand("SetShaderOnSelection", getSelection());
}

bool MediaBrowserTreeView::_testSingleTexSel()
{
    if (!isDirectorySelected() && !getSelection().empty())
        return true;
    else
        return false;
}

void MediaBrowserTreeView::_onShowShaderDefinition()
{
    std::string shaderName = getSelection();

    // Construct a shader view and pass the shader name
    auto view = new MaterialDefinitionView(shaderName);
    view->ShowModal();
    view->Destroy();
}

void MediaBrowserTreeView::_onSelectItems(bool select)
{
    std::string shaderName = getSelection();

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
    std::string selection = getSelection();

    if (!isDirectorySelected() && !selection.empty())
    {
        // Pass shader name to the selection system
        GlobalCommandSystem().executeCommand("SetShaderOnSelection", selection);
    }
}

}
