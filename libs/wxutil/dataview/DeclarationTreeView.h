#pragma once

#include "ResourceTreeView.h"

namespace wxutil
{

class DeclarationTreeView :
    public ResourceTreeView
{
public:
    DeclarationTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    DeclarationTreeView(wxWindow* parent, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);
};

}
