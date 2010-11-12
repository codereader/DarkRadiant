#include "ClassnamePropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"
#include "iundo.h"

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>

#include "selection/algorithm/Entity.h"
#include "ui/entitychooser/EntityClassChooser.h"

namespace ui
{

// Main constructor
ClassnamePropertyEditor::ClassnamePropertyEditor(Entity* entity,
									     		 const std::string& name,
									     		 const std::string& options)
: PropertyEditor(entity),
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
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose entity class...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("classname"))));

	browseButton->signal_clicked().connect(
		sigc::mem_fun(*this, &ClassnamePropertyEditor::_onBrowseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);

	mainVBox->pack_start(*vbx, true, true, 0);
}

void ClassnamePropertyEditor::_onBrowseButton()
{
	std::string currentEclass = _entity->getKeyValue(_key);

	// Use the EntityClassChooser dialog to get a selection from the user
	EntityClassChooser& chooser = EntityClassChooser::Instance();

	chooser.setSelectedEntityClass(currentEclass);

	chooser.show(); // enter main loop

	// Check the result and the selected eclass
	const std::string& selection = chooser.getSelectedEntityClass();

	// Only apply if the classname has actually changed
	if (chooser.getResult() == EntityClassChooser::RESULT_OK && selection != currentEclass)
	{
		UndoableCommand cmd("changeEntityClass");

		// Apply the classname change to the entity, this requires some algorithm
		selection::algorithm::setEntityClassname(selection);
	}
}

} // namespace ui
