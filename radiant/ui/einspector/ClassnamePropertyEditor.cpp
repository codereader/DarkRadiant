#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "ieclass.h"
#include "icommandsystem.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include "wxutil/EntityClassChooser.h"
#include "wxutil/sourceview/DeclarationSourceView.h"

#include "wxutil/Bitmap.h"

namespace ui
{

// Main constructor
ClassnamePropertyEditor::ClassnamePropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
    wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
    mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

    // Register the main widget in the base class
    setMainWidget(mainVBox);

    wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose Entity Class..."));
    browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("classname"));
    browseButton->Bind(wxEVT_BUTTON, &ClassnamePropertyEditor::_onBrowseButton, this);

    wxButton* showDefinition = new wxButton(mainVBox, wxID_ANY, _("Show Definition..."));
    showDefinition->SetBitmap(wxutil::GetLocalBitmap("decl.png"));
    showDefinition->Bind(wxEVT_BUTTON, &ClassnamePropertyEditor::_onShowDefinition, this);

    mainVBox->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
    mainVBox->GetSizer()->Add(showDefinition, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void ClassnamePropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	auto currentEclass = _entities.getSharedKeyValue(_key->getFullKey(), false);

	// Use the EntityClassChooser dialog to get a selection from the user
	auto selectedEclass = wxutil::EntityClassChooser::ChooseEntityClass(
        wxutil::EntityClassChooser::Purpose::SelectClassname, currentEclass);

	// Only apply if the classname has actually changed
	if (!selectedEclass.empty() && selectedEclass != currentEclass)
	{
		// Apply the classname change to the current selection, dispatch the command
		GlobalCommandSystem().executeCommand("SetEntityKeyValue", _key->getFullKey(), selectedEclass);

        signal_keyValueApplied().emit(_key->getFullKey(), selectedEclass);
	}
}

void ClassnamePropertyEditor::_onShowDefinition(wxCommandEvent& ev)
{
    auto currentEclass = _entities.getSharedKeyValue(_key->getFullKey(), false);
    auto eclass = GlobalEntityClassManager().findClass(currentEclass);

    if (eclass)
    {
        auto view = new wxutil::DeclarationSourceView(getWidget());
        view->setDeclaration(eclass);

        view->ShowModal();
        view->Destroy();
    }
}

} // namespace ui
