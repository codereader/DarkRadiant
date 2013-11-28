#pragma once

#include <gtkmm/box.h>
#include <map>
#include "ientity.h"
#include "iundo.h"
#include <boost/shared_ptr.hpp>

class Selectable;
class Entity;

namespace ui
{

class AIEditingPanel;
typedef boost::shared_ptr<AIEditingPanel> AIEditingPanelPtr;

class SpawnargLinkedCheckbox;
class SpawnargLinkedSpinButton;

class AIEditingPanel : 
	public Gtk::VBox,
	public Entity::Observer,
	public UndoSystem::Observer
{
private:
	sigc::connection _selectionChangedSignal;

	bool _queueUpdate;

	typedef std::map<std::string, SpawnargLinkedCheckbox*> CheckboxMap;
	CheckboxMap _checkboxes;

	typedef std::map<std::string, SpawnargLinkedSpinButton*> SpinButtonMap;
	SpinButtonMap _spinButtons;

	Entity* _entity;

public:
	AIEditingPanel();

	static AIEditingPanel& Instance();
	static void Shutdown();

	static void onRadiantStartup();

	void onKeyInsert(const std::string& key, EntityKeyValue& value);
    void onKeyChange(const std::string& key, const std::string& val);
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	void postUndo();
	void postRedo();

protected:
	// override Widget's expose event
	bool on_expose_event(GdkEventExpose* event);

private:
	static AIEditingPanelPtr& InstancePtr();

	void constructWidgets();

	void onRadiantShutdown();
	void onSelectionChanged(const Selectable& selectable);

	void rescanSelection();

	Entity* getEntityFromSelection();
	void updateWidgetsFromSelection();
	void updatePanelSensitivity();
};

} // namespace
