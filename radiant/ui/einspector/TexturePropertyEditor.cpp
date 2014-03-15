#include "TexturePropertyEditor.h"
#include "LightTextureChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <wx/panel.h>
#include <wx/button.h>

namespace ui
{

// Main constructor
TexturePropertyEditor::TexturePropertyEditor(wxWindow* parent, Entity* entity,
											 const std::string& name,
											 const std::string& options)
: PropertyEditor(entity),
  _prefixes(options),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the browse button
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose texture..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("texture"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(TexturePropertyEditor::_onBrowse), NULL, this);
}

// Browse button callback
void TexturePropertyEditor::_onBrowse(wxCommandEvent& ev)
{
	// Light texture chooser (self-destructs on close)
	LightTextureChooser chooser;
	std::string texture = chooser.chooseTexture();

	if (!texture.empty())
	{
		// Apply the keyvalue immediately
		setKeyValue(_key, texture);
	}
}

} // namespace ui
