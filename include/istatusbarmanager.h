#pragma once

#include "imodule.h"

class wxWindow;

namespace ui
{

class IStatusBarManager :
    public RegisterableModule
{
public:
    // Use these positions to place the status bar elements in between
    // the default ones. A position of 31 would put a widget in between
    // POS_BRUSHCOUNT and POS_SHADERCLIPBOARD.
    enum StandardPositions
    {
        POS_FRONT = 0,
        POS_COMMAND = 10,
        POS_POSITION = 20,
        POS_BRUSHCOUNT = 30,
        POS_SHADERCLIPBOARD = 40,
        POS_GRID = 50,
        POS_MAP_EDIT_TIME = 60,
        POS_BACK = 9000,
    };

    /**
        * Destructor
        */
    virtual ~IStatusBarManager() {}

    /**
        * Get the status bar widget, for packing into the main window.
        * The widget will be parented to a temporary wxFrame, so it has to be
        * re-parented before packing.
        */
    virtual wxWindow* getStatusBar() = 0;

    /**
        * greebo: This adds a named element to the status bar. Pass the widget
        * which should be added and specify the position order.
        *
        * @name: the name of the element (can be used for later lookup).
        * @widget: the widget to pack.
        * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
        *       at the front or back of the status bar container.
        */
    virtual void addElement(const std::string& name, wxWindow* widget, int pos) = 0;

    /**
        * greebo: A specialised method, adding a named text element.
        * Use the setText() method to update this element.
        *
        * @name: the name for this element (can be used as key for the setText() method).
        * @icon: the icon file to pack into the element, relative the BITMAPS_PATH. Leave empty
        *        if no icon is desired.
        * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
        *       at the front or back of the status bar container.
        * @description: a description shown when the mouse pointer hovers of this item.
        */
    virtual void addTextElement(const std::string& name, const std::string& icon, int pos,
        const std::string& description) = 0;

    /**
        * Updates the content of the named text element. The name must refer to
        * an element previously added by addTextElement().
        * If immediateUpdate is set to true, the UI will be updated right now. UI updates come with
        * a certain cost, try to avoid it unless it's really necessary.
        */
    virtual void setText(const std::string& name, const std::string& text, bool immediateUpdate = false) = 0;

    /**
        * Returns a named status bar widget, previously added by addElement().
        *
        * @returns: NULL if the named widget does not exist.
        */
    virtual wxWindow* getElement(const std::string& name) = 0;
};

// The name of the command status bar item
#define STATUSBAR_COMMAND "Command"

}

constexpr const char* const MODULE_STATUSBARMANAGER = "StatusBarManager";

inline ui::IStatusBarManager& GlobalStatusBarManager()
{
    static module::InstanceReference<ui::IStatusBarManager> _reference(MODULE_STATUSBARMANAGER);
    return _reference;
}