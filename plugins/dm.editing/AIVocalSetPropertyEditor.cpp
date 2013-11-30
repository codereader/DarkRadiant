#include "AIVocalSetPropertyEditor.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include "AIVocalSetChooserDialog.h"

namespace ui
{

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor(Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	_widget = Gtk::manage(new Gtk::HBox(false, 0));
	_widget->set_border_width(6);

	// Horizontal box contains the browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Browse button for models
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Select Vocal Set...")));

	browseButton->set_image(
		*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("icon_sound.png")))
	);
	browseButton->signal_clicked().connect(sigc::mem_fun(*this, &AIVocalSetPropertyEditor::onChooseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);
	_widget->pack_start(*vbx, true, true, 0);
}

Gtk::Widget& AIVocalSetPropertyEditor::getWidget()
{
	return *_widget;
}

IPropertyEditorPtr AIVocalSetPropertyEditor::createNew(Entity* entity,
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIVocalSetPropertyEditor(entity, key, options));
}

void AIVocalSetPropertyEditor::onChooseButton()
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog dialog;

	dialog.setSelectedVocalSet(_entity->getKeyValue(DEF_VOCAL_SET_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIVocalSetChooserDialog::RESULT_OK)
	{
		_entity->setKeyValue(DEF_VOCAL_SET_KEY, dialog.getSelectedVocalSet());
	}
}

std::string AIVocalSetPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog dialog;

	std::string oldValue = entity->getKeyValue(DEF_VOCAL_SET_KEY);
	dialog.setSelectedVocalSet(oldValue);

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIVocalSetChooserDialog::RESULT_OK)
	{
		return dialog.getSelectedVocalSet();
	}

	return oldValue;
}

} // namespace ui
