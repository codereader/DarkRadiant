#pragma once

#include <gtkmm/box.h>
#include <boost/shared_ptr.hpp>

class Selectable;

namespace ui
{

class AIEditingPanel;
typedef boost::shared_ptr<AIEditingPanel> AIEditingPanelPtr;

class AIEditingPanel : 
	public Gtk::VBox
{
private:
	sigc::connection _selectionChangedSignal;

public:
	AIEditingPanel();

	static AIEditingPanel& Instance();
	static void Shutdown();

	static void onRadiantStartup();

private:
	static AIEditingPanelPtr& InstancePtr();

	void constructWidgets();

	void onRadiantShutdown();
	void onSelectionChanged(const Selectable& selectable);
};

} // namespace
