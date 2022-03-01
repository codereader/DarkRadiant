#pragma once

#include <map>
#include "imousetool.h"
#include "imousetoolmanager.h"
#include "MouseToolGroup.h"
#include <wx/event.h>
#include <wx/timer.h>
#include "wxutil/event/SingleIdleCallback.h"

namespace ui
{

class ModifierHintPopup;

/**
* Implementation of the IMouseToolManager interface.
* This is used by the GlobalXYWnd and GlobalCamera instances.
*/
class MouseToolManager :
    public wxutil::SingleIdleCallback,
    public IMouseToolManager
{
protected:
    typedef std::map<IMouseToolGroup::Type, MouseToolGroupPtr> GroupMap;
    GroupMap _mouseToolGroups;

    unsigned int _activeModifierState;

    wxTimer _hintCloseTimer;
    ModifierHintPopup* _hintPopup;
    bool _shouldClosePopup;

public:
    MouseToolManager();

    // RegisterableModule implementation
    const std::string& getName() const;
    const StringSet& getDependencies() const;
    void initialiseModule(const IApplicationContext& ctx);
    void shutdownModule();

    // Get the group defined by the given enum. This always succeeds, if the group
    // is not existing yet, a new one will be created internally.
    MouseToolGroup& getGroup(IMouseToolGroup::Type group);

    // Iterate over each group using the given visitor function
    void foreachGroup(const std::function<void(IMouseToolGroup&)>& functor);

    MouseToolStack getMouseToolsForEvent(IMouseToolGroup::Type group, unsigned int mouseState);

    void updateStatusbar(unsigned int newState);

    void resetBindingsToDefault();
    void closeHintPopup();

protected:
    void onIdle() override;

private:
    void onMainFrameConstructed();

    void loadToolMappings();
	void loadGroupMapping(MouseToolGroup::Type type);

    void saveToolMappings();

    void onCloseTimerIntervalReached(wxTimerEvent& ev);
};

} // namespace
