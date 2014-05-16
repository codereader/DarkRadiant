#include "LayerControl.h"

#include "i18n.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "idialogmanager.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/EntryAbortedException.h"

#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/tglbtn.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include "layers/LayerSystem.h"
#include "LayerControlDialog.h"

namespace ui
{

namespace
{
	const char* const ICON_LAYER_VISIBLE("check.png");
	const char* const ICON_LAYER_HIDDEN("empty.png");
	const char* const ICON_LAYER_ACTIVE_VISIBLE("active_layer_visible.png");
	const char* const ICON_LAYER_ACTIVE_HIDDEN("active_layer_invisible.png");
}

LayerControl::LayerControl(wxWindow* parent, int layerID) :
	_layerID(layerID)
{
	// Create the toggle button
	_toggle = new wxBitmapToggleButton(parent, wxID_ANY, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ICON_LAYER_VISIBLE));
	_toggle->SetMaxSize(wxSize(30, -1));
	_toggle->Connect(wxEVT_TOGGLEBUTTON, wxCommandEventHandler(LayerControl::onToggle), NULL, this);

	// Create the label
	_labelButton = new wxButton(parent, wxID_ANY);
	_labelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(LayerControl::onLayerSelect), NULL, this);

	_deleteButton = new wxBitmapButton(parent, wxID_ANY, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "delete.png"));
	_deleteButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(LayerControl::onDelete), NULL, this);

	_renameButton = new wxBitmapButton(parent, wxID_ANY,
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "edit.png"));
	_renameButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(LayerControl::onRename), NULL, this);

	_buttonHBox = new wxBoxSizer(wxHORIZONTAL);

	_buttonHBox->Add(_renameButton, 0, wxEXPAND);
	_buttonHBox->Add(_deleteButton, 0, wxEXPAND | wxLEFT, 3);

	_labelButton->SetToolTip(_("Click to select all in layer, hold SHIFT to deselect, hold CTRL to set as active layer."));
	_renameButton->SetToolTip(_("Rename this layer"));
	_deleteButton->SetToolTip(_("Delete this layer"));
	_toggle->SetToolTip(_("Toggle layer visibility"));

	// Read the status from the Layer
	update();
}

wxButton* LayerControl::getLabelButton()
{
	return _labelButton;
}

wxSizer* LayerControl::getButtons()
{
	return _buttonHBox;
}

wxToggleButton* LayerControl::getToggle()
{
	return _toggle;
}

void LayerControl::update()
{
	_updateActive = true;

	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	bool layerIsVisible = layerSystem.layerIsVisible(_layerID);
	_toggle->SetValue(layerIsVisible);

	_labelButton->SetLabel(layerSystem.getLayerName(_layerID));

	bool isActive = layerSystem.getActiveLayer() == _layerID; 

	std::string imageName;
	
	if (isActive)
	{
		imageName = layerIsVisible ? ICON_LAYER_ACTIVE_VISIBLE : ICON_LAYER_ACTIVE_HIDDEN;
	}
	else
	{
		imageName = layerIsVisible ? ICON_LAYER_VISIBLE : ICON_LAYER_HIDDEN;
	}

	_toggle->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + imageName));

	// Don't allow deleting or renaming layer 0
	_deleteButton->Enable(_layerID != 0);
	_renameButton->Enable(_layerID != 0);

	// Don't allow selection of hidden layers
	_labelButton->Enable(layerIsVisible);

	_updateActive = false;
}

void LayerControl::onToggle(wxCommandEvent& ev)
{
	if (_updateActive) return;

	scene::getLayerSystem().setLayerVisibility(_layerID, _toggle->GetValue());
}

void LayerControl::onDelete(wxCommandEvent& ev)
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

void LayerControl::onRename(wxCommandEvent& ev)
{
	while (true)
	{
		// Query the name of the new layer from the user
		std::string newLayerName;

		try
		{
			newLayerName = gtkutil::Dialog::TextEntryDialog(
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
			wxutil::Messagebox::ShowError(_("Could not rename layer, please try again."));
			continue;
		}
	}
}

void LayerControl::onLayerSelect(wxCommandEvent& ev)
{
	// When holding down CTRL the user sets this as active
	if (wxGetKeyState(WXK_CONTROL))
	{
		GlobalLayerSystem().setActiveLayer(_layerID);

		// Update our icon set
		LayerControlDialog::Instance().refresh();

		return;
	}

	// By default, we SELECT the layer
	bool selected = true;

	// The user can choose to DESELECT the layer when holding down shift
	if (wxGetKeyState(WXK_SHIFT))
	{
		selected = false;
	}

	// Set the entire layer to selected
	GlobalLayerSystem().setSelected(_layerID, selected);
}

} // namespace ui
