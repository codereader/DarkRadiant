#include "MaterialTreeView.h"

#include "ishaders.h"
#include "i18n.h"
#include "MaterialPopulator.h"

namespace ui
{

MaterialTreeView::MaterialTreeView(wxWindow* parent) :
    DeclarationTreeView(parent, decl::Type::Material, Columns(), wxDV_NO_HEADER)
{
    auto* textCol = AppendIconTextColumn(_("Shader"), Columns().iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    SetExpanderColumn(textCol);
    textCol->SetWidth(300);

    AddSearchColumn(Columns().iconAndName);

    // The wxWidgets algorithm sucks at sorting large flat lists of strings,
    // so we do that ourselves
    GetTreeModel()->SetHasDefaultCompare(false);

    _materialCreated = GlobalMaterialManager().signal_materialCreated().connect(
        sigc::mem_fun(this, &MaterialTreeView::onMaterialCreated));
    _materialRenamed = GlobalMaterialManager().signal_materialRenamed().connect(
        sigc::mem_fun(this, &MaterialTreeView::onMaterialRenamed));
    _materialRemoved = GlobalMaterialManager().signal_materialRemoved().connect(
        sigc::mem_fun(this, &MaterialTreeView::onMaterialRemoved));
}

MaterialTreeView::~MaterialTreeView()
{
    _materialCreated.disconnect();
    _materialRenamed.disconnect();
    _materialRemoved.disconnect();
}

const MaterialTreeView::TreeColumns& MaterialTreeView::Columns() const
{
    static TreeColumns _treeColumns;
    return _treeColumns;
}

void MaterialTreeView::Populate()
{
    ResourceTreeView::Populate(std::make_shared<MaterialPopulator>(Columns()));
}

void MaterialTreeView::SetTreeMode(MaterialTreeView::TreeMode mode)
{
    std::string previouslySelectedItem = GetSelectedFullname();

    ResourceTreeView::SetTreeMode(mode);

    // Try to select the same item we had as before
    SetSelectedFullname(previouslySelectedItem);
}

std::string MaterialTreeView::GetSelectedTextureFolderName()
{
    if (!IsDirectorySelected()) return {};

    // The folder name is stored in the decl name column
    auto fullName = GetSelectedFullname();

    auto otherMaterialsPrefix = MaterialPopulator::GetOtherMaterialsName() + "/";

    return string::starts_with(fullName, otherMaterialsPrefix) ? 
        fullName.substr(otherMaterialsPrefix.length()) : fullName;
}

void MaterialTreeView::onMaterialCreated(const std::string& name)
{
    auto populator = MaterialPopulator(Columns());
    populator.AddSingleMaterial(GetTreeModel(), name);
}

void MaterialTreeView::onMaterialRenamed(const std::string& oldName, const std::string& newName)
{
    auto populator = MaterialPopulator(Columns());
    populator.RemoveSingleMaterial(GetTreeModel(), oldName);
    populator.AddSingleMaterial(GetTreeModel(), newName);
}

void MaterialTreeView::onMaterialRemoved(const std::string& name)
{
    auto populator = MaterialPopulator(Columns());
    populator.RemoveSingleMaterial(GetTreeModel(), name);
}

}
