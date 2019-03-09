#pragma once

#include "ieventmanager.h"

#include <wx/event.h>
#include <sigc++/connection.h>
#include "Event.h"

namespace ui
{

/* greebo: A Statement is an object that executes a command string when invoked.
 *
 * Trigger the Statement via the execute() method (usually done by the associated accelerator).
 *
 * Connect the statement to a wxToolBarToolBase / wxButton / wxMenuItem via the connectWidget method.
 */
class Statement :
    public Event,
    public wxEvtHandler
{
private:
    // The statement to execute
    std::string _statement;

    // Whether this Statement reacts on keyup or keydown
    bool _reactOnKeyUp;

    typedef std::set<wxMenuItem*> MenuItems;
    MenuItems _menuItems;

    typedef std::set<wxToolBarToolBase*> ToolItems;
    ToolItems _toolItems;

    typedef std::set<wxButton*> Buttons;
    Buttons _buttons;

public:
    Statement(const std::string& statement, bool reactOnKeyUp = false);

    virtual ~Statement();

    // Invoke the registered callback
    virtual void execute();

    // Override the derived keyDown/keyUp method
    virtual void keyUp();
    virtual void keyDown();

    // Connect the given menuitem/toolbutton to this Statement
    virtual void connectMenuItem(wxMenuItem* item);
    virtual void disconnectMenuItem(wxMenuItem* item);

    virtual void connectToolItem(wxToolBarToolBase* item);
    virtual void disconnectToolItem(wxToolBarToolBase* item);

    virtual void connectButton(wxButton* button);
    virtual void disconnectButton(wxButton* button);

    virtual bool empty() const;

    virtual void connectAccelerator(IAccelerator& accel);
    virtual void disconnectAccelerators();

private:
    // The allback methods that can be connected to a ToolButton or a MenuItem
    void onMenuItemClicked(wxCommandEvent& ev);
    void onToolItemClicked(wxCommandEvent& ev);
    void onButtonClicked(wxCommandEvent& ev);

}; // class Statement

}
