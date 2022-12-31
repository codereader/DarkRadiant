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

#include "wxutil/Bitmap.h"

namespace ui
{

constexpr const char* const SILENCE_SHADER = "silence";

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	constructBrowseButtonPanel(parent, _("Choose sound..."),
		PropertyEditorFactory::getBitmapFor("sound"));

    if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
    {
        // Check if there's a silence shader to display the button
        auto button = new wxButton(getWidget(), wxID_ANY, _("Assign Silence"));
        button->SetBitmap(wxutil::GetLocalBitmap("icon_sound_mute.png"));
        button->Bind(wxEVT_BUTTON, &SoundPropertyEditor::onAssignSilence, this);
        button->SetToolTip(_("Assigns the 'silence' sound shader (if available)"));

        getWidget()->GetSizer()->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

        button->Enable(GlobalSoundManager().getSoundShader(SILENCE_SHADER) != nullptr);
    }
    else
    {
        getWidget()->Disable();
    }
}

void SoundPropertyEditor::onAssignSilence(wxCommandEvent& ev)
{
    setKeyValueOnSelection(_key->getFullKey(), SILENCE_SHADER);
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
