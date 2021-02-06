#pragma once

#include "imodule.h"
class wxToolBar;
class wxWindow;

namespace ui
{

/**
 * \brief
 * Manager object which constructs toolbars from XML files.
 *
 * The IToolbarManager is responsible for parsing toolbar declarations from
 * user.xml and constructing wxToolBar objects containing the specified tool
 * buttons.
 *
 * Note however that the IToolbarManager does not own or keep track of the
 * constructed wxToolBars; these are packed into and owned by the IMainFrame
 * and made available with IMainFrame::getToolbar().
 */
class IToolbarManager :
    public RegisterableModule
{
public:
    virtual ~IToolbarManager() {}

    /**
     * \brief
     * Create a wxToolBar corresponding to the given named toolbar declaration
     * in the user.xml file.
     *
     * The toolbar's tool buttons will be registered with the event manager on
     * construction, and deregistered when the wxToolbar widget is destroyed.
     *
     * Calling this method more than once with the same toolbarName will create
     * a new wxToolBar each time, which will possibly result in duplicate event
     * manager registrations unless the first toolbar widget has already been
     * destroyed.
     *
     * \param name
     * Name of the toolbar to create. This must correspond to a <toolbar>
     * section in the user.xml file.
     *
     * \param parent
     * Parent window for the constructed wxToolBar widget.
     */
    virtual wxToolBar* createToolbar(const std::string& name, wxWindow* parent) = 0;
};

}

constexpr const char* const MODULE_TOOLBARMANAGER = "ToolBarManager";

inline ui::IToolbarManager& GlobalToolBarManager()
{
    static module::InstanceReference<ui::IToolbarManager> _reference(MODULE_TOOLBARMANAGER);
    return _reference;
}
