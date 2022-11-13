#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ui/iresourcechooser.h"
#include "ui/idialogmanager.h"
#include "i18n.h"
#include "ientity.h"
#include "isound.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	constructBrowseButtonPanel(parent, _("Choose sound..."),
		PropertyEditorFactory::getBitmapFor("sound"));

    if (!module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
    {
        getWidget()->Disable();
    }
}

void SoundPropertyEditor::onBrowseButtonClick()
{
	IResourceChooser* chooser = GlobalDialogManager().createSoundShaderChooser(wxGetTopLevelParent(getWidget()));

	// Use a SoundChooser dialog to get a selection from the user
	std::string picked = chooser->chooseResource(getKeyValueFromSelection(_key->getFullKey()));

	// Selection will be empy if user clicked cancel or X
	if (!picked.empty())
	{
		// Apply the change to the entity
        setKeyValueOnSelection(_key->getFullKey(), picked);
	}

	chooser->destroyDialog();
}

} // namespace ui
