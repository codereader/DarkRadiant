#include "SkinEditorTreeView.h"

namespace ui
{

SkinEditorTreeView::SkinEditorTreeView(wxWindow* parent, const Columns& columns, long style) :
    DeclarationTreeView(parent, decl::Type::Skin, columns, style),
    _columns(columns)
{
    _declRenamed = GlobalDeclarationManager().signal_DeclRenamed().connect(
        sigc::mem_fun(this, &SkinEditorTreeView::onDeclarationRenamed));
    // TODO: removed event
}

SkinEditorTreeView::~SkinEditorTreeView()
{
    _declRenamed.disconnect();
}

void SkinEditorTreeView::Populate()
{
    DeclarationTreeView::Populate(
        std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Skin, _columns, SKIN_ICON)
    );
}

void SkinEditorTreeView::onDeclarationRenamed(decl::Type type, const std::string& oldName, const std::string& newName)
{
    if (type != decl::Type::Skin) return;

    wxutil::ThreadedDeclarationTreePopulator populator(type, _columns, SKIN_ICON);
    populator.RemoveSingleDecl(GetTreeModel(), oldName);
    populator.AddSingleDecl(GetTreeModel(), newName);
}

}
