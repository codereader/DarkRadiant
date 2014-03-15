#include "EntityPropertyEditor.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ientity.h"

#include "PropertyEditorFactory.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include "ui/common/EntityChooser.h"

namespace ui
{

// Blank ctor
EntityPropertyEditor::EntityPropertyEditor() :
	PropertyEditor()
{}

// Constructor. Create the widgets here

EntityPropertyEditor::EntityPropertyEditor(wxWindow* parent, Entity* entity, const std::string& name) :
	PropertyEditor(entity),
	_key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Browse button
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose target entity..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("entity"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityPropertyEditor::_onBrowseButton), NULL, this);

	mainVBox->GetSizer()->Add(browseButton, 0, wxEXPAND | wxALIGN_CENTER);
}

void EntityPropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	// Use a new dialog window to get a selection from the user
	std::string selection = EntityChooser::ChooseEntity(_entity->getKeyValue(_key));

	// Only apply non-empty selections if the classname has actually changed
	if (!selection.empty() && selection != _entity->getKeyValue(_key))
	{
		UndoableCommand cmd("changeKeyValue");

		// Apply the change
		_entity->setKeyValue(_key, selection);
	}
}

} // namespace ui
