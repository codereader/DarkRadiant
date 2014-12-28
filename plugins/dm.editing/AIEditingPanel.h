#pragma once

#include <map>
#include "ientity.h"
#include "iundo.h"
#include <memory>

#include <sigc++/connection.h>
#include <wx/event.h>

class Selectable;
class Entity;
class wxStaticText;
class wxScrolledWindow;
class wxSizer;

namespace ui
{

class AIEditingPanel;
typedef std::shared_ptr<AIEditingPanel> AIEditingPanelPtr;

class SpawnargLinkedCheckbox;
class SpawnargLinkedSpinButton;

class AIEditingPanel : 
	public Entity::Observer,
	public UndoSystem::Observer,
	public wxEvtHandler
{
private:
	sigc::connection _selectionChangedSignal;

	wxWindow* _tempParent;
	wxScrolledWindow* _mainPanel;

	bool _queueUpdate;

	typedef std::map<std::string, SpawnargLinkedCheckbox*> CheckboxMap;
	CheckboxMap _checkboxes;

	typedef std::map<std::string, SpawnargLinkedSpinButton*> SpinButtonMap;
	SpinButtonMap _spinButtons;

	typedef std::map<std::string, wxStaticText*> LabelMap;
	LabelMap _labels;

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
	void OnPaint(wxPaintEvent& ev);

	void onBrowseButton(wxCommandEvent& ev, const std::string& key);

private:
	static AIEditingPanelPtr& InstancePtr();

	void constructWidgets();
	wxSizer* createSpinButtonHbox(SpawnargLinkedSpinButton* spinButton);
	wxStaticText* createSectionLabel(const std::string& text);
	void createChooserRow(wxSizer* table, const std::string& rowLabel, 
									  const std::string& buttonLabel, const std::string& buttonIcon,
									  const std::string& key);

	void onRadiantShutdown();
	void onSelectionChanged(const Selectable& selectable);

	void rescanSelection();

	Entity* getEntityFromSelection();
	void updateWidgetsFromSelection();
	void updatePanelSensitivity();
};

} // namespace
