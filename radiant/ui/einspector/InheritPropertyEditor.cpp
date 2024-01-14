#include "InheritPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "wxutil/Bitmap.h"
#include "Algorithm.h"

namespace ui
{

// Main constructor
InheritPropertyEditor::InheritPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
    auto mainVBox = new wxPanel(parent, wxID_ANY);
    mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

    // Register the main widget in the base class
    setMainWidget(mainVBox);

    auto showDefinition = new wxButton(mainVBox, wxID_ANY, _("Show Definition..."));
    showDefinition->SetBitmap(wxutil::GetLocalBitmap("decl.png"));
    showDefinition->Bind(wxEVT_BUTTON, &InheritPropertyEditor::_onShowDefinition, this);

    auto showInDefTree = new wxButton(mainVBox, wxID_ANY, _("Show in Def Tree..."));
    showInDefTree->SetBitmap(PropertyEditorFactory::getBitmapFor("classname"));
    showInDefTree->Bind(wxEVT_BUTTON, &InheritPropertyEditor::_onShowInDefTree, this);

    mainVBox->GetSizer()->Add(showDefinition, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
    mainVBox->GetSizer()->Add(showInDefTree, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void InheritPropertyEditor::_onShowDefinition(wxCommandEvent& ev)
{
    auto parentClass = _entities.getSharedKeyValue(_key->getFullKey(), true);
    algorithm::showEntityClassDefinition(getWidget(), parentClass);
}

void InheritPropertyEditor::_onShowInDefTree(wxCommandEvent& ev)
{
    auto currentEclass = _entities.getSharedKeyValue(_key->getFullKey(), true);

    algorithm::showEntityClassInTree(currentEclass);
}

} // namespace ui
