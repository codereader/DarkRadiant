#include "EntityPropertyEditor.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ientity.h"

#include "PropertyEditorFactory.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include "ui/common/EntityChooser.h"

namespace ui
{

// Blank ctor
EntityPropertyEditor::EntityPropertyEditor() :
	PropertyEditor()
{}

// Constructor. Create the widgets here

EntityPropertyEditor::EntityPropertyEditor(wxWindow* parent, Entity* entity, const std::string& name) :
	PropertyEditor(entity),
	_key(name)
{
	constructBrowseButtonPanel(parent, _("Choose target entity..."),
		PropertyEditorFactory::getBitmapFor("entity"));
}

void EntityPropertyEditor::updateFromEntity()
{
	// nothing to do
}

void EntityPropertyEditor::onBrowseButtonClick()
{
	// Use a new dialog window to get a selection from the user
	std::string selection = EntityChooser::ChooseEntity(_entity->getKeyValue(_key));

	// Only apply non-empty selections if the classname has actually changed
	if (!selection.empty() && selection != _entity->getKeyValue(_key))
	{
		UndoableCommand cmd("changeKeyValue");

		// Apply the change
		_entity->setKeyValue(_key, selection);
	}
}

} // namespace ui
