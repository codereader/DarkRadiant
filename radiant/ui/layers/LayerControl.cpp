#include "LayerControl.h"

#include "i18n.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "idialogmanager.h"
#include "gtkutil/dialog.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/EntryAbortedException.h"

#include <gtkmm/togglebutton.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "layers/LayerSystem.h"
#include "LayerControlDialog.h"

namespace ui
{
	namespace
	{
		const std::string ICON_LAYER_VISIBLE("check.png");
		const std::string ICON_LAYER_HIDDEN("empty.png");
	}

LayerControl::LayerControl(int layerID) :
	_layerID(layerID)
{
	// Create the toggle button
	_toggle = Gtk::manage(new Gtk::ToggleButton);
	_toggle->signal_toggled().connect(sigc::mem_fun(*this, &LayerControl::onToggle));

	// Create the label
	_labelButton = Gtk::manage(new Gtk::Button);
	_labelButton->signal_clicked().connect(sigc::mem_fun(*this, &LayerControl::onLayerSelect));
	
	_deleteButton = Gtk::manage(new Gtk::Button);
	_deleteButton->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_SMALL_TOOLBAR)));
	_deleteButton->signal_clicked().connect(sigc::mem_fun(*this, &LayerControl::onDelete));
	
	_renameButton = Gtk::manage(new Gtk::Button);
	_renameButton->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::EDIT, Gtk::ICON_SIZE_SMALL_TOOLBAR)));
	_renameButton->signal_clicked().connect(sigc::mem_fun(*this, &LayerControl::onRename));

	_buttonHBox = Gtk::manage(new Gtk::HBox(false, 6));
	
	_buttonHBox->pack_start(*_renameButton, false, false, 0);
	_buttonHBox->pack_start(*_deleteButton, false, false, 0);

	_labelButton->set_tooltip_text(_("Click to select all in layer, hold SHIFT to deselect"));
	_renameButton->set_tooltip_text(_("Rename this layer"));
	_deleteButton->set_tooltip_text(_("Delete this layer"));
	_toggle->set_tooltip_text(_("Toggle layer visibility"));

	// Read the status from the Layer
	update();
}

Gtk::Button& LayerControl::getLabelButton()
{
	return *_labelButton;
}

Gtk::HBox& LayerControl::getButtons()
{
	return *_buttonHBox;
}

Gtk::ToggleButton& LayerControl::getToggle()
{
	return *_toggle;
}

void LayerControl::update()
{
	_updateActive = true;

	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	bool layerIsVisible = layerSystem.layerIsVisible(_layerID);
	_toggle->set_active(layerIsVisible);

	_labelButton->set_label(layerSystem.getLayerName(_layerID));

	std::string imageName = layerIsVisible ? ICON_LAYER_VISIBLE : ICON_LAYER_HIDDEN;
	_toggle->set_image(*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbufWithMask(imageName))));
	
	// Don't allow deleting or renaming layer 0
	_deleteButton->set_sensitive(_layerID != 0);
	_renameButton->set_sensitive(_layerID != 0);

	// Don't allow selection of hidden layers
	_labelButton->set_sensitive(layerIsVisible);

	_updateActive = false;
}

void LayerControl::onToggle()
{
	if (_updateActive) return;

	scene::getLayerSystem().setLayerVisibility(_layerID, _toggle->get_active());
}

void LayerControl::onDelete()
{
	// Ask the about the deletion
	std::string msg = _("Do you really want to delete this layer?");
	msg += "\n<b>" + scene::getLayerSystem().getLayerName(_layerID) + "</b>";

	IDialogPtr box = GlobalDialogManager().createMessageBox(
		_("Confirm Layer Deletion"), msg, IDialog::MESSAGE_ASK
	);

	if (box->run() == IDialog::RESULT_YES)
	{
		scene::getLayerSystem().deleteLayer(
			scene::getLayerSystem().getLayerName(_layerID)
		);
		LayerControlDialog::Instance().refresh();
	}
}

void LayerControl::onRename()
{
	while (true)
	{
		// Query the name of the new layer from the user
		std::string newLayerName;

		try
		{
			newLayerName = gtkutil::textEntryDialog(
				_("Rename Layer"), 
				_("Enter new Layer Name"), 
				scene::getLayerSystem().getLayerName(_layerID),
				Glib::RefPtr<Gtk::Window>()
			);
		}
		catch (gtkutil::EntryAbortedException&)
		{
			break;
		}

		// Attempt to rename the layer, this will return -1 if the operation fails
		bool success = scene::getLayerSystem().renameLayer(_layerID, newLayerName);

		if (success)
		{
			// Reload the widgets, we're done here
			update();
			break;
		}
		else
		{
			// Wrong name, let the user try again
			gtkutil::errorDialog(_("Could not rename layer, please try again."), Glib::RefPtr<Gtk::Window>());
			continue; 
		}
	}
}

void LayerControl::onLayerSelect()
{
	// By default, we SELECT the layer
	bool selected = true;

	// The user can choose to DESELECT the layer when holding down shift
	if ((GlobalEventManager().getModifierState() & GDK_SHIFT_MASK) != 0)
	{
		selected = false;
	}

	// Set the entire layer to selected
	GlobalLayerSystem().setSelected(_layerID, selected);
}

} // namespace ui
