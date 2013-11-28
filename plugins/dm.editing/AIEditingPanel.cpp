#include "AIEditingPanel.h"

#include "i18n.h"
#include "iselection.h"
#include "ieclass.h"
#include "iradiant.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "selectionlib.h"
#include "scenelib.h"

#include <gtkmm/label.h>

namespace ui
{

AIEditingPanel::AIEditingPanel() :
	_queueVisibilityUpdate(true)
{
	constructWidgets();

	GlobalRadiant().signal_radiantShutdown().connect(
		sigc::mem_fun(*this, &AIEditingPanel::onRadiantShutdown)
	);	

	_selectionChangedSignal = GlobalSelectionSystem().signal_selectionChanged().connect(
		sigc::mem_fun(*this, &AIEditingPanel::onSelectionChanged)
	);
}

void AIEditingPanel::constructWidgets()
{
	Gtk::Label* label = Gtk::manage(new Gtk::Label);
	label->set_text("Test");

	pack_start(*label, false, false, 0);

	show_all();
}

AIEditingPanel& AIEditingPanel::Instance()
{
	AIEditingPanelPtr& instance = InstancePtr();

	if (!instance)
	{
		instance.reset(new AIEditingPanel);
	}

	return *instance;
}

void AIEditingPanel::Shutdown()
{
	if (InstancePtr() != NULL)
	{
		//Instance().shutdown();
		InstancePtr().reset();
	}
} 

void AIEditingPanel::onRadiantStartup()
{
	// Add the Media Browser page
	GlobalGroupDialog().addPage(
    	"aieditingpanel",	// name
    	"AI", // tab title
    	"icon_model.png", // tab icon
    	Instance(), // page widget
    	_("AI"),
		"mediabrowser"
	);
}

void AIEditingPanel::onRadiantShutdown()
{
	_selectionChangedSignal.disconnect();
}

void AIEditingPanel::updatePanelSensitivity()
{
	_queueVisibilityUpdate = false;

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	bool sensitive = false;

	// Make sure we only have entities highlighted
	if (info.entityCount > 0 && info.totalCount == info.entityCount)
	{
		// Check for any AI in the selection
		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			if (entity->isOfType("atdm:ai_base"))
			{
				// One entity is enough, activate the panel
				sensitive = true;
			}
		});
	}

	set_sensitive(sensitive);
}

void AIEditingPanel::onSelectionChanged(const Selectable& selectable)
{
	if (GlobalGroupDialog().getPage() == this)
	{
		updatePanelSensitivity();
	}
	else
	{
		_queueVisibilityUpdate = true;
	}
}

bool AIEditingPanel::on_expose_event(GdkEventExpose* event)
{
	if (_queueVisibilityUpdate)
	{
		updatePanelSensitivity();
	}

	// Propagate the call
	return Gtk::VBox::on_expose_event(event);
}

AIEditingPanelPtr& AIEditingPanel::InstancePtr()
{
	static AIEditingPanelPtr _instancePtr;
	return _instancePtr;
} 

} // namespace
