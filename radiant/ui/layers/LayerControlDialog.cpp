#include "LayerControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "ilayer.h"
#include "imainframe.h"

#include "registry/registry.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtkmm/table.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>

#include "layers/LayerSystem.h"

namespace ui
{
	namespace
	{
		const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

LayerControlDialog::LayerControlDialog() :
	PersistentTransientWindow(_("Layers"), GlobalMainFrame().getTopLevelWindow(), true),
	_controlContainer(Gtk::manage(new Gtk::Table(1, 3, false)))
{
	_controlContainer->set_row_spacings(3);
	_controlContainer->set_col_spacings(3);

	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(this);

	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

void LayerControlDialog::populateWindow()
{
	// Create the "master" vbox
	Gtk::VBox* overallVBox = Gtk::manage(new Gtk::VBox(false, 6));
	overallVBox->set_border_width(12);

	add(*Gtk::manage(new gtkutil::ScrolledFrame(*overallVBox)));

	// Add the LayerControl vbox to the window
	overallVBox->pack_start(*_controlContainer, false, false, 0);

	// Add the option buttons ("Create Layer", etc.) to the window
	overallVBox->pack_start(createButtons(), false, false, 0);
}

Gtk::Widget& LayerControlDialog::createButtons()
{
	Gtk::VBox* buttonVBox = Gtk::manage(new Gtk::VBox(false, 6));

	// Show all / hide all buttons
	Gtk::HBox* hideShowBox = Gtk::manage(new Gtk::HBox(true, 6));

	_showAllLayers = Gtk::manage(new Gtk::Button(_("Show all")));
	_hideAllLayers = Gtk::manage(new Gtk::Button(_("Hide all")));

	_showAllLayers->signal_clicked().connect(sigc::mem_fun(*this, &LayerControlDialog::onShowAllLayers));
	_hideAllLayers->signal_clicked().connect(sigc::mem_fun(*this, &LayerControlDialog::onHideAllLayers));

	hideShowBox->pack_start(*_showAllLayers, true, true, 0);
	hideShowBox->pack_start(*_hideAllLayers, true, true, 0);

	buttonVBox->pack_start(*hideShowBox, false, false, 0);

	// Create layer button
	Gtk::Button* createButton = Gtk::manage(new Gtk::Button(Gtk::Stock::NEW));

	IEventPtr event = GlobalEventManager().findEvent("CreateNewLayer");

	if (event != NULL)
	{
		event->connectWidget(createButton);
	}

	createButton->set_size_request(100, -1);
	buttonVBox->pack_start(*createButton, false, false, 0);

	return *buttonVBox;
}

void LayerControlDialog::refresh()
{
	// Remove the widgets from the vbox first
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i)
	{
		_controlContainer->remove((*i)->getToggle());
		_controlContainer->remove((*i)->getLabelButton());
		_controlContainer->remove((*i)->getButtons());
	}

	// Remove all previously allocated layercontrols
	_layerControls.clear();

	// Local helper class for populating the window
	class LayerControlAccumulator :
		public scene::LayerSystem::Visitor
	{
		typedef std::map<std::string, LayerControlPtr> LayerControlMap;
		LayerControlMap _sortedLayerControls;
	public:
		void visit(int layerID, const std::string& layerName)
		{
			// Create a new layercontrol for each visited layer
			// Store the object in a sorted container
			_sortedLayerControls[layerName] = LayerControlPtr(new LayerControl(layerID));
		}

		// Returns the sorted vector
		LayerControls getVector()
		{
			LayerControls returnValue;

			// Copy the objects over to a linear vector
			for (LayerControlMap::const_iterator i = _sortedLayerControls.begin();
				 i != _sortedLayerControls.end(); ++i)
			{
				returnValue.push_back(i->second);
			}

			return returnValue;
		}
	} populator;

	// Traverse the layers
	scene::getLayerSystem().foreachLayer(populator);

	// Get the sorted vector
	_layerControls = populator.getVector();

	_controlContainer->resize(static_cast<int>(_layerControls.size()), 3);

	int c = 0;
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i, ++c)
	{
		_controlContainer->attach((*i)->getToggle(),
			0, 1, c, c+1, Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

		_controlContainer->attach((*i)->getLabelButton(), 1, 2, c, c+1);

		_controlContainer->attach((*i)->getButtons(),
			2, 3, c, c+1, Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);
	}

	// Make sure the newly added items are visible
	_controlContainer->show_all();

	update();
}

void LayerControlDialog::update()
{
	// Broadcast the update() call
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i)
	{
		(*i)->update();
	}

	// Update the show/hide all button sensitiveness

	class CheckAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		std::size_t numVisible;
		std::size_t numHidden;

		CheckAllLayersWalker() :
			numVisible(0),
			numHidden(0)
		{}

		void visit(int layerID, const std::string& layerName)
		{
			if (GlobalLayerSystem().layerIsVisible(layerID)) {
				numVisible++;
			}
			else {
				numHidden++;
			}
		}
	};

	CheckAllLayersWalker visitor;
	GlobalLayerSystem().foreachLayer(visitor);

	_showAllLayers->set_sensitive(visitor.numHidden > 0);
	_hideAllLayers->set_sensitive(visitor.numVisible > 0);
}

void LayerControlDialog::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleVisibility();
}

void LayerControlDialog::init()
{
	// Lookup the stored window information in the registry
	if (GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "visible") == "1")
	{
		// Show dialog
		Instance().show();
	}
}

void LayerControlDialog::onRadiantShutdown()
{
	rMessage() << "LayerControlDialog shutting down.\n";

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalEventManager().disconnectDialogWindow(this);

	// Write the visibility status to the registry
	GlobalRegistry().setAttribute(RKEY_WINDOW_STATE, "visible", is_visible() ? "1" : "0");

	// Destroy the window (after it has been disconnected from the Eventmanager)
	destroy();

	// Final step: clear the instance pointer
	InstancePtr().reset();
}

LayerControlDialogPtr& LayerControlDialog::InstancePtr()
{
	static LayerControlDialogPtr _instancePtr;
	return _instancePtr;
}

LayerControlDialog& LayerControlDialog::Instance()
{
	LayerControlDialogPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new LayerControlDialog);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &LayerControlDialog::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

// TransientWindow callbacks
void LayerControlDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();
	// Re-populate the dialog
	refresh();
}

void LayerControlDialog::_preHide()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

void LayerControlDialog::onShowAllLayers()
{
	// Local helper class
	class ShowAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, const std::string& layerName)
		{
			GlobalLayerSystem().setLayerVisibility(layerID, true);
		}
	};

	ShowAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

void LayerControlDialog::onHideAllLayers()
{
	// Local helper class
	class HideAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, const std::string& layerName)
		{
			GlobalLayerSystem().setLayerVisibility(layerID, false);
		}
	};

	HideAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

} // namespace ui
