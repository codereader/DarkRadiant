#pragma once

#include <string>

namespace ui
{

namespace algorithm
{

// If the given entity class exists, this shows a DeclarationSourceView dialog with its definition
void showEntityClassDefinition(wxWindow* parent, const std::string& eclass);

// If the given entity class exists, the entity class tree dialog is shown with the given class pre-selected
void showEntityClassInTree(const std::string& eclass);

}

}
