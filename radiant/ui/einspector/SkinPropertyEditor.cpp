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
	constructBrowseButtonPanel(parent, _("Choose skin..."),
		PropertyEditorFactory::getBitmapFor("skin"));
}

void SkinPropertyEditor::onBrowseButtonClick()
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
