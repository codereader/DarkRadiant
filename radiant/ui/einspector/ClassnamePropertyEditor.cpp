#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "iundo.h"
#include "icommandsystem.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "command/ExecutionFailure.h"
#include "ui/entitychooser/EntityClassChooser.h"

namespace ui
{

// Main constructor
ClassnamePropertyEditor::ClassnamePropertyEditor(wxWindow* parent, Entity* entity,
									     		 const std::string& name,
									     		 const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	constructBrowseButtonPanel(parent, _("Choose entity class..."),
		PropertyEditorFactory::getBitmapFor("classname"));
}

void ClassnamePropertyEditor::onBrowseButtonClick()
{
	std::string currentEclass = _entity->getKeyValue(_key);

	// Use the EntityClassChooser dialog to get a selection from the user
	std::string selection = EntityClassChooser::chooseEntityClass(currentEclass);

	// Only apply if the classname has actually changed
	if (!selection.empty() && selection != currentEclass)
	{
		// Apply the classname change to the current selection, dispatch the command
		GlobalCommandSystem().executeCommand("SetEntityKeyValue", _key, selection);
	}
}

} // namespace ui
