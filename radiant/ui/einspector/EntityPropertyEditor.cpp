#include "EntityPropertyEditor.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ientity.h"

#include "PropertyEditorFactory.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "ui/common/EntityChooser.h"

namespace ui {

// Blank ctor
EntityPropertyEditor::EntityPropertyEditor() :
	PropertyEditor()
{}

// Constructor. Create the GTK widgets here

EntityPropertyEditor::EntityPropertyEditor(Entity* entity, const std::string& name) :
	PropertyEditor(entity),
	_key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 0);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Horizontal box contains the browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Browse button
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose target entity...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("entity"))));

	browseButton->signal_clicked().connect(
		sigc::mem_fun(*this, &EntityPropertyEditor::_onBrowseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);

	mainVBox->pack_start(*vbx, true, true, 0);
}

void EntityPropertyEditor::_onBrowseButton()
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
