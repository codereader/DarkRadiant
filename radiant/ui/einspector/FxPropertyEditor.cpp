#include "FxPropertyEditor.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "ientity.h"

#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "ui/fx/FxChooser.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>

namespace ui
{

FxPropertyEditor::FxPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key) :
    PropertyEditor(entities),
    _key(key)
{
    auto mainVBox = new wxPanel(parent, wxID_ANY);
    mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

    // Register the main widget in the base class
    setMainWidget(mainVBox);

    auto browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose FX..."));
    browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("fx"));
    browseButton->Bind(wxEVT_BUTTON, &FxPropertyEditor::_onBrowseButton, this);

    mainVBox->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void FxPropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	auto currentDecl = _entities.getSharedKeyValue(_key->getFullKey(), false);

	// Use the EntityClassChooser dialog to get a selection from the user
	auto selectedDecl = FxChooser::ChooseDeclaration(currentDecl);

	// Only apply if the classname has actually changed
	if (!selectedDecl.empty() && selectedDecl != currentDecl)
	{
		// Apply the change to the current selection, dispatch the command
		GlobalCommandSystem().executeCommand("SetEntityKeyValue", _key->getFullKey(), selectedDecl);

        signal_keyValueApplied().emit(_key->getFullKey(), selectedDecl);
	}
}

}
