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
#include <wx/sizer.h>

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
	constructBrowseButtonPanel(parent, _("Choose texture..."),
		PropertyEditorFactory::getBitmapFor("texture"));
}

// Browse button callback
void TexturePropertyEditor::onBrowseButtonClick()
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
