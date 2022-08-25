#include "TexturePropertyEditor.h"
#include "ui/common/ShaderChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

// Main constructor
TexturePropertyEditor::TexturePropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	constructBrowseButtonPanel(parent, _("Choose texture..."),
		PropertyEditorFactory::getBitmapFor("texture"));
}

// Browse button callback
void TexturePropertyEditor::onBrowseButtonClick()
{
	auto dialog = new ShaderChooser(getWidget(), ShaderSelector::TextureFilter::Lights);

    dialog->setSelectedTexture(getKeyValue(_key->getFullKey()));

	if (dialog->ShowModal() == wxID_OK)
	{
		// Return the last selection to calling process
		std::string texture = dialog->getSelectedTexture();

		if (!texture.empty())
		{
			// Apply the keyvalue immediately
			setKeyValue(_key->getFullKey(), texture);
		}
	}

	dialog->Destroy();
}

} // namespace ui
