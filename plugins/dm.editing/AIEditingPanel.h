#pragma once

#include <gtkmm/box.h>
#include <map>
#include <boost/shared_ptr.hpp>

class Selectable;
class Entity;

namespace ui
{

class AIEditingPanel;
typedef boost::shared_ptr<AIEditingPanel> AIEditingPanelPtr;

class SpawnargLinkedCheckbox;

class AIEditingPanel : 
	public Gtk::VBox
{
private:
	sigc::connection _selectionChangedSignal;

	bool _queueUpdate;

	typedef std::map<std::string, SpawnargLinkedCheckbox*> CheckboxMap;
	CheckboxMap _checkboxes;

public:
	AIEditingPanel();

	static AIEditingPanel& Instance();
	static void Shutdown();

	static void onRadiantStartup();

protected:
	// override Widget's expose event
	bool on_expose_event(GdkEventExpose* event);

private:
	static AIEditingPanelPtr& InstancePtr();

	void constructWidgets();

	void onRadiantShutdown();
	void onSelectionChanged(const Selectable& selectable);

	Entity* getSelectedEntity();

	void updateWidgetsFromSelection();
	void updatePanelSensitivity();
};

} // namespace
