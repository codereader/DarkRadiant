#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "iundo.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include "wxutil/dialog/MessageBox.h"

#include "selection/algorithm/Entity.h"
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
		UndoableCommand cmd("changeEntityClass");

		try
		{
			// Apply the classname change to the entity, this requires some algorithm
			selection::algorithm::setEntityClassname(selection);
		}
		catch (std::runtime_error& ex)
		{
			wxutil::Messagebox::ShowError(ex.what());
		}
	}
}

} // namespace ui
