#include "SkinPropertyEditor.h"
#include "SkinChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "ientity.h"

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(wxWindow* parent, Entity* entity,
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
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose skin..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("skin"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(SkinPropertyEditor::_onBrowseButton), NULL, this);

	mainVBox->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL);
}

void SkinPropertyEditor::_onBrowseButton(wxCommandEvent& ev)
{
	// Display the SkinChooser to get a skin from the user
	std::string modelName = _entity->getKeyValue("model");
	std::string prevSkin = _entity->getKeyValue(_key);
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);

	// Apply the key to the entity
	setKeyValue(_key, skin);
}

std::string SkinPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	std::string modelName = entity->getKeyValue("model");
	std::string prevSkin = entity->getKeyValue(key);
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);

	// return the new value
	return skin;
}

} // namespace
