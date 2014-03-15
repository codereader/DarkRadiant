#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "iundo.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "selection/algorithm/Entity.h"
#include "ui/entitychooser/EntityClassChooser.h"

namespace ui
{

// Main constructor
ClassnamePropertyEditor::ClassnamePropertyEditor(wxWindow* parent, Entity* entity,
									     		 const std::string& name,
									     		 const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Browse button
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose entity class..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("classname"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ClassnamePropertyEditor::_onBrowseButton), NULL, this);

	mainVBox->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL);
}

void ClassnamePropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	std::string currentEclass = _entity->getKeyValue(_key);

	// Use the EntityClassChooser dialog to get a selection from the user
	EntityClassChooser& chooser = EntityClassChooser::Instance();

	chooser.setSelectedEntityClass(currentEclass);

	chooser.show(); // enter main loop

	// Check the result and the selected eclass
	const std::string& selection = chooser.getSelectedEntityClass();

	// Only apply if the classname has actually changed
	if (chooser.getResult() == EntityClassChooser::RESULT_OK && selection != currentEclass)
	{
		UndoableCommand cmd("changeEntityClass");

		// Apply the classname change to the entity, this requires some algorithm
		selection::algorithm::setEntityClassname(selection);
	}
}

} // namespace ui
