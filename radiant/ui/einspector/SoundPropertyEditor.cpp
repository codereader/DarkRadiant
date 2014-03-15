#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "ui/common/SoundChooser.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(wxWindow* parent, Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the browse button
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose sound..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("sound"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(SoundPropertyEditor::_onBrowseButton), NULL, this);
}

void SoundPropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	// Use a SoundChooser dialog to get a selection from the user
	SoundChooser chooser;
	chooser.setSelectedShader(getKeyValue(_key));

	chooser.show(); // blocks

	const std::string& selection = chooser.getSelectedShader();

	// Selection will be empy if user clicked cancel or X
	if (!selection.empty())
	{
		// Apply the change to the entity
		setKeyValue(_key, selection);
	}
}

} // namespace ui
