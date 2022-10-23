#pragma once

#include <map>
#include "ientity.h"
#include <memory>

#include <sigc++/connection.h>
#include <sigc++/trackable.h>

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"

class ISelectable;
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
	public wxutil::DockablePanel,
	public Entity::Observer,
	public sigc::trackable,
    public wxutil::SingleIdleCallback
{
private:
	sigc::connection _selectionChangedSignal;

	wxScrolledWindow* _mainPanel;

	typedef std::map<std::string, SpawnargLinkedCheckbox*> CheckboxMap;
	CheckboxMap _checkboxes;

	typedef std::map<std::string, SpawnargLinkedSpinButton*> SpinButtonMap;
	SpinButtonMap _spinButtons;

	typedef std::map<std::string, wxStaticText*> LabelMap;
	LabelMap _labels;

	Entity* _entity;

	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

    bool _rescanSelectionOnIdle;

public:
	AIEditingPanel(wxWindow* parent);
	~AIEditingPanel() override;

	void onKeyInsert(const std::string& key, EntityKeyValue& value) override;
    void onKeyChange(const std::string& key, const std::string& val) override;
	void onKeyErase(const std::string& key, EntityKeyValue& value) override;

	void postUndo();
	void postRedo();

protected:
    void onIdle() override;
	void onBrowseButton(wxCommandEvent& ev, const std::string& key);

    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

	void constructWidgets();
	wxSizer* createSpinButtonHbox(SpawnargLinkedSpinButton* spinButton);
	wxStaticText* createSectionLabel(const std::string& text);
	void createChooserRow(wxSizer* table, const std::string& rowLabel, 
									  const std::string& buttonLabel, const std::string& buttonIcon,
									  const std::string& key);

	void onSelectionChanged(const ISelectable& selectable);

	void rescanSelection();

	Entity* getEntityFromSelection();
	void updateWidgetsFromSelection();
	void updatePanelSensitivity();
};

} // namespace
