#include "SkinPropertyEditor.h"
#include "ui/common/SkinChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "wxutil/dialog/MessageBox.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "ientity.h"

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	constructBrowseButtonPanel(parent, _("Choose skin..."),
		PropertyEditorFactory::getBitmapFor("skin"));
}

void SkinPropertyEditor::onBrowseButtonClick()
{
    auto modelKey = _key->clone();
    modelKey->setAffectedKey("model");

    auto model = getKeyValueFromSelection(modelKey->getFullKey());

    if (model.empty())
    {
        wxutil::Messagebox::ShowError(
            _("The model key values of the selection are ambiguous, cannot choose a skin."), getWidget());
        return;
    }

	// Display the SkinChooser to get a skin from the user
	std::string prevSkin = getKeyValueFromSelection(_key->getFullKey());
	std::string skin = SkinChooser::ChooseSkin(model, prevSkin);

	// Apply the key to the entity
    setKeyValueOnSelection(_key->getFullKey(), skin);
}

std::string SkinChooserDialogWrapper::runDialog(Entity* entity, const std::string& key)
{
    std::string modelName = entity->getKeyValue("model");
    std::string prevSkin = entity->getKeyValue(key);
    std::string skin = SkinChooser::ChooseSkin(modelName, prevSkin);

    // return the new value
    return skin;
}

} // namespace
