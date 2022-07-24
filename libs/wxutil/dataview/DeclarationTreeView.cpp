#include "DeclarationTreeView.h"

namespace wxutil
{

DeclarationTreeView::DeclarationTreeView(wxWindow* parent, const Columns& columns, long style) :
    DeclarationTreeView(parent, TreeModel::Ptr(), columns, style)
{}

DeclarationTreeView::DeclarationTreeView(wxWindow* parent, const TreeModel::Ptr& model, const Columns& columns, long style) :
    ResourceTreeView(parent, model, columns, style)
{}

}
