#pragma once

#include "XYWnd.h"
#include "wxutil/window/TransientWindow.h"

namespace ui
{

/**
 * A floating version of the XYWnd. These are created by the XYWndManager and
 * notify the manager when they are destroyed.
 */
class FloatingOrthoView :
    public wxutil::TransientWindow,
    public XYWnd
{
public:
    /**
     * Construct a floating XY window with the given numeric ID (assigned by
     * the XYWndManager).
     *
     * @param id
     * Unique ID assigned to this window.
     *
     * @param title
     * The displayed title for this window (e.g. "XY Front").
     *
     * @param parent
     * The parent window for which this should be a transient (normally the
     * mainframe).
     */
    FloatingOrthoView(int id, const std::string& title, wxWindow* parent);

    /** Overrides the setViewType method of the XYWnd base class.
     *  Extends the functionality by setting the window title.
     */
    virtual void setViewType(EViewType viewType);

    virtual void SaveWindowState();

protected:
    virtual void _onSetFocus();

    // Post-destroy callback, initiate destruction of this XYWnd
    virtual void _postDestroy();

private:
    void onFocus(wxFocusEvent& ev);
};
typedef boost::shared_ptr<FloatingOrthoView> FloatingOrthoViewPtr;

} // namespace
