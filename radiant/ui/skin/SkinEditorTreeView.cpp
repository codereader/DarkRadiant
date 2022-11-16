#include "SkinEditorTreeView.h"

namespace ui
{

SkinEditorTreeView::SkinEditorTreeView(wxWindow* parent, const Columns& columns, long style) :
    DeclarationTreeView(parent, decl::Type::Skin, columns, style),
    _columns(columns)
{
    _declCreated = GlobalDeclarationManager().signal_DeclCreated().connect(
        sigc::mem_fun(this, &SkinEditorTreeView::onDeclarationCreated));
    _declRenamed = GlobalDeclarationManager().signal_DeclRenamed().connect(
        sigc::mem_fun(this, &SkinEditorTreeView::onDeclarationRenamed));
    _declRemoved = GlobalDeclarationManager().signal_DeclRemoved().connect(
        sigc::mem_fun(this, &SkinEditorTreeView::onDeclarationRemoved));
}

SkinEditorTreeView::~SkinEditorTreeView()
{
    _declCreated.disconnect();
    _declRenamed.disconnect();
    _declRemoved.disconnect();
}

void SkinEditorTreeView::Populate()
{
    DeclarationTreeView::Populate(
        std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Skin, _columns, SKIN_ICON)
    );
}

void SkinEditorTreeView::onDeclarationCreated(decl::Type type, const std::string& name)
{
    if (type != decl::Type::Skin) return;

    wxutil::ThreadedDeclarationTreePopulator populator(type, _columns, SKIN_ICON);
    populator.AddSingleDecl(GetTreeModel(), name);
}

void SkinEditorTreeView::onDeclarationRenamed(decl::Type type, const std::string& oldName, const std::string& newName)
{
    if (type != decl::Type::Skin) return;

    wxutil::ThreadedDeclarationTreePopulator populator(type, _columns, SKIN_ICON);
    populator.RemoveSingleDecl(GetTreeModel(), oldName);
    populator.AddSingleDecl(GetTreeModel(), newName);
}

void SkinEditorTreeView::onDeclarationRemoved(decl::Type type, const std::string& name)
{
    if (type != decl::Type::Skin) return;

    wxutil::ThreadedDeclarationTreePopulator populator(type, _columns, SKIN_ICON);
    populator.RemoveSingleDecl(GetTreeModel(), name);
}

}
