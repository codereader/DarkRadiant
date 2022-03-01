#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "iundo.h"
#include "icommandsystem.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include "wxutil/EntityClassChooser.h"

#include "command/ExecutionFailure.h"

namespace ui
{

// Main constructor
ClassnamePropertyEditor::ClassnamePropertyEditor(wxWindow* parent, IEntitySelection& entities,
									     		 const std::string& name,
									     		 const std::string& options)
: PropertyEditor(entities),
  _key(name)
{
	constructBrowseButtonPanel(parent, _("Choose Entity Class..."),
		PropertyEditorFactory::getBitmapFor("classname"));
}

void ClassnamePropertyEditor::onBrowseButtonClick()
{
	std::string currentEclass = _entities.getSharedKeyValue(_key, false);

	// Use the EntityClassChooser dialog to get a selection from the user
	std::string selection = wxutil::EntityClassChooser::ChooseEntityClass(
        wxutil::EntityClassChooser::Purpose::SelectClassname, currentEclass);

	// Only apply if the classname has actually changed
	if (!selection.empty() && selection != currentEclass)
	{
		// Apply the classname change to the current selection, dispatch the command
		GlobalCommandSystem().executeCommand("SetEntityKeyValue", _key, selection);
	}
}

} // namespace ui
