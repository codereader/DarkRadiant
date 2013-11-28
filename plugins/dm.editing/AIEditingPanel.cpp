#include "AIEditingPanel.h"

#include "i18n.h"
#include "iselection.h"
#include "iradiant.h"
#include "igroupdialog.h"
#include "iuimanager.h"

namespace ui
{

AIEditingPanel::AIEditingPanel()
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

void AIEditingPanel::onSelectionChanged(const Selectable& selectable)
{
}

AIEditingPanelPtr& AIEditingPanel::InstancePtr()
{
	static AIEditingPanelPtr _instancePtr;
	return _instancePtr;
} 

} // namespace
