#include "MaterialSelector.h"

#include <vector>
#include <string>

#include "ishaders.h"

#include "texturelib.h"
#include "gamelib.h"
#include "string/predicate.h"
#include "registry/registry.h"

#include "../common/TexturePreviewCombo.h"
#include "MaterialThumbnailBrowser.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/Bitmap.h"

#include <wx/tglbtn.h>
#include <wx/sizer.h>

namespace ui
{

/* CONSTANTS */

namespace
{
	constexpr const char* const TEXTURE_ICON = "icon_texture.png";
	constexpr const char* const RKEY_MATERIAL_SELECTOR_VIEW_MODE = "user/ui/materialSelector/viewMode";
}

/**
 * Visitor class to retrieve material names and add them to folders.
 */
class ThreadedMaterialLoader final :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    std::vector<std::string> _prefixes;

public:
    ThreadedMaterialLoader(const wxutil::DeclarationTreeView::Columns& columns, MaterialSelector::TextureFilter filter) :
        ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON)
    {
        switch (filter)
        {
        case MaterialSelector::TextureFilter::Lights:
            _prefixes = game::current::getLightTexturePrefixes();
            break;

        case MaterialSelector::TextureFilter::Regular:
            _prefixes = std::vector<std::string>{ GlobalMaterialManager().getTexturePrefix() };
            break;

        case MaterialSelector::TextureFilter::All:
            _prefixes = std::vector<std::string>();
            break;
        };
    }

    ~ThreadedMaterialLoader() override
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        wxutil::VFSTreePopulator populator(model);

        GlobalMaterialManager().foreachShaderName([&](const std::string& materialName)
        {
            if (_prefixes.empty()) // no filter?
            {
                AddMaterial(populator, materialName);
                return;
            }

            for (const std::string& prefix : _prefixes)
            {
                if (string::istarts_with(materialName, prefix))
                {
                    AddMaterial(populator, materialName);
                    break; // don't consider any further prefixes
                }
            }
        });
    }

    void AddMaterial(wxutil::VFSTreePopulator& populator, const std::string& materialName)
    {
        populator.addPath(materialName, [&](wxutil::TreeModel::Row& row,
            const std::string& path, const std::string& leafName, bool isFolder)
        {
            AssignValuesToRow(row, path, path, leafName, isFolder);
        });
    }
};

MaterialSelector::MaterialSelector(wxWindow* parent, TextureFilter textureFilter)
: DeclarationSelector(parent, decl::Type::Material),
  _textureFilter(textureFilter),
  _thumbnailBrowser(nullptr),
  _viewToggleBtn(nullptr),
  _showingThumbnails(false)
{
    createThumbnailBrowser();
    createViewToggleButton();
    connectFilterSignal();

    AddPreviewToBottom(new TexturePreviewCombo(this));

    std::string savedMode = GlobalRegistry().get(RKEY_MATERIAL_SELECTOR_VIEW_MODE);
    if (savedMode == "thumbnails")
    {
        _showingThumbnails = true;
        _viewToggleBtn->SetValue(true);
        switchView(true);
    }

    Populate();
}

MaterialSelector::~MaterialSelector()
{
    GlobalRegistry().set(RKEY_MATERIAL_SELECTOR_VIEW_MODE,
                         _showingThumbnails ? "thumbnails" : "tree");

    _thumbnailSelectionConn.disconnect();
    _thumbnailActivatedConn.disconnect();
    _filterTextChangedConn.disconnect();
}

void MaterialSelector::createThumbnailBrowser()
{
    MaterialThumbnailBrowser::TextureFilter thumbnailFilter;
    switch (_textureFilter)
    {
    case TextureFilter::Lights:
        thumbnailFilter = MaterialThumbnailBrowser::TextureFilter::Lights;
        break;
    case TextureFilter::Regular:
        thumbnailFilter = MaterialThumbnailBrowser::TextureFilter::Regular;
        break;
    case TextureFilter::All:
    default:
        thumbnailFilter = MaterialThumbnailBrowser::TextureFilter::All;
        break;
    }

    _thumbnailBrowser = new MaterialThumbnailBrowser(GetLeftPanel(), thumbnailFilter);
    _thumbnailBrowser->Hide();

    GetTreeViewSizer()->Insert(2, _thumbnailBrowser, 1, wxEXPAND);

    _thumbnailSelectionConn = _thumbnailBrowser->signal_selectionChanged().connect(
        sigc::mem_fun(this, &MaterialSelector::onThumbnailSelectionChanged));
    _thumbnailActivatedConn = _thumbnailBrowser->signal_itemActivated().connect(
        sigc::mem_fun(this, &MaterialSelector::onThumbnailItemActivated));
}

void MaterialSelector::createViewToggleButton()
{
    auto* sizer = GetTreeViewSizer();
    if (sizer->GetItemCount() > 0)
    {
        auto* toolbarItem = sizer->GetItem(static_cast<size_t>(0));
        if (toolbarItem && toolbarItem->GetWindow())
        {
            auto* toolbar = dynamic_cast<wxutil::ResourceTreeViewToolbar*>(toolbarItem->GetWindow());
            if (toolbar)
            {
                _viewToggleBtn = new wxBitmapToggleButton(toolbar, wxID_ANY,
                    wxutil::GetLocalBitmap("bgimage16.png"));
                _viewToggleBtn->SetToolTip(_("Toggle between tree view and thumbnail grid view"));
                toolbar->GetRightSizer()->Add(_viewToggleBtn, wxSizerFlags().Border(wxLEFT, 6));
                _viewToggleBtn->Bind(wxEVT_TOGGLEBUTTON, &MaterialSelector::onViewToggle, this);
            }
        }
    }
}

void MaterialSelector::connectFilterSignal()
{
    auto* sizer = GetTreeViewSizer();
    if (sizer->GetItemCount() > 0)
    {
        auto* toolbarItem = sizer->GetItem(static_cast<size_t>(0));
        if (toolbarItem && toolbarItem->GetWindow())
        {
            auto* toolbar = dynamic_cast<wxutil::ResourceTreeViewToolbar*>(toolbarItem->GetWindow());
            if (toolbar)
            {
                _filterTextChangedConn = toolbar->signal_filterTextChanged().connect(
                    sigc::mem_fun(this, &MaterialSelector::onFilterTextChanged));
            }
        }
    }
}

void MaterialSelector::onFilterTextChanged(const std::string& filterText)
{
    if (_thumbnailBrowser)
    {
        _thumbnailBrowser->setExternalFilterText(filterText);
    }
}

void MaterialSelector::switchView(bool showThumbnails)
{
    if (showThumbnails)
    {
        _thumbnailBrowser->setSelectedShader(DeclarationSelector::GetSelectedDeclName());
        GetTreeView()->Hide();
        _thumbnailBrowser->Show();
        _thumbnailBrowser->queueUpdate();
    }
    else
    {
        std::string selectedShader = _thumbnailBrowser->getSelectedShader();
        if (!selectedShader.empty())
        {
            SetSelectedDeclName(selectedShader);
        }
        _thumbnailBrowser->Hide();
        GetTreeView()->Show();
    }

    _showingThumbnails = showThumbnails;
    GetLeftPanel()->Layout();
}

void MaterialSelector::onViewToggle(wxCommandEvent& ev)
{
    switchView(ev.IsChecked());
}

void MaterialSelector::onThumbnailSelectionChanged()
{
    UpdatePreviewsWithDeclaration(_thumbnailBrowser->getSelectedShader());
    _selectionChanged();
}

void MaterialSelector::onThumbnailItemActivated()
{
    onTreeViewItemActivated();
}

void MaterialSelector::Populate()
{
    PopulateTreeView(std::make_shared<ThreadedMaterialLoader>(GetColumns(), _textureFilter));

    if (_thumbnailBrowser)
    {
        _thumbnailBrowser->queueUpdate();
    }
}

std::string MaterialSelector::GetSelectedDeclName() const
{
    if (_showingThumbnails && _thumbnailBrowser)
    {
        return _thumbnailBrowser->getSelectedShader();
    }
    return DeclarationSelector::GetSelectedDeclName();
}

MaterialPtr MaterialSelector::getSelectedShader()
{
    return GlobalMaterialManager().getMaterial(GetSelectedDeclName());
}

void MaterialSelector::onTreeViewSelectionChanged()
{
    if (!_showingThumbnails && _thumbnailBrowser)
    {
        _thumbnailBrowser->setSelectedShader(DeclarationSelector::GetSelectedDeclName());
    }
    _selectionChanged();
}

bool MaterialSelector::onTreeViewItemActivated()
{
    return false;
}

} // namespace
