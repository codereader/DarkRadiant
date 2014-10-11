#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "ui/common/SoundChooser.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(wxWindow* parent, Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	constructBrowseButtonPanel(parent, _("Choose sound..."),
		PropertyEditorFactory::getBitmapFor("sound"));
}

void SoundPropertyEditor::onBrowseButtonClick()
{
	// Use a SoundChooser dialog to get a selection from the user
	std::string selection = SoundChooser::ChooseSound();

	// Selection will be empy if user clicked cancel or X
	if (!selection.empty())
	{
		// Apply the change to the entity
		setKeyValue(_key, selection);
	}
}

} // namespace ui
