#include "AIHeadPropertyEditor.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include "AIHeadChooserDialog.h"

namespace ui
{

AIHeadPropertyEditor::AIHeadPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIHeadPropertyEditor::AIHeadPropertyEditor(Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	_widget = Gtk::manage(new Gtk::HBox(false, 0));
	_widget->set_border_width(6);

	// Horizontal box contains the browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Browse button for models
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose AI head...")));

	browseButton->set_image(
		*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("icon_model.png")))
	);
	browseButton->signal_clicked().connect(sigc::mem_fun(*this, &AIHeadPropertyEditor::onChooseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);
	_widget->pack_start(*vbx, true, true, 0);
}

Gtk::Widget& AIHeadPropertyEditor::getWidget()
{
	return *_widget;
}

IPropertyEditorPtr AIHeadPropertyEditor::createNew(Entity* entity,
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIHeadPropertyEditor(entity, key, options));
}

void AIHeadPropertyEditor::onChooseButton()
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog dialog;

	dialog.setSelectedHead(_entity->getKeyValue(DEF_HEAD_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIHeadChooserDialog::RESULT_OK)
	{
		_entity->setKeyValue(DEF_HEAD_KEY, dialog.getSelectedHead());
	}
}

std::string AIHeadPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog dialog;

	std::string prevHead = entity->getKeyValue(key);
	dialog.setSelectedHead(prevHead);

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIHeadChooserDialog::RESULT_OK)
	{
		return dialog.getSelectedHead();
	}
	else
	{
		return prevHead;
	}
}

} // namespace ui
