#pragma once

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <boost/shared_ptr.hpp>

class RenderSystem;
typedef boost::shared_ptr<RenderSystem> RenderSystemPtr;

namespace ui
{

/**
 * \brief
 * A treeview that shows a list of materials and a checkbox to enable/disable
 * each material.
 */
class MaterialsList: public Gtk::TreeView
{
    // Data store
    Glib::RefPtr<Gtk::ListStore> _store;

    // Render system to update
    RenderSystemPtr _renderSystem;

    sigc::signal<void> _visibilityChangedSignal;

private:
    void visibleCellClicked(const Glib::ustring& treePath);

public:

    /**
     * \brief
     * Construct a MaterialsList
     *
     * \param renderSystem
     * The render system whose shaders should be made visible or invisible based
     * on clicks in the relevant list column.
     */
    MaterialsList(RenderSystemPtr renderSystem);

    /// Clear all entries
    void clear();

    /// Add a material
    void addMaterial(const Glib::ustring& name);

    /**
     * \brief
     * Signal emitted when material visibility has changed.
     *
     * This indicates that a redraw will be needed based on the new material
     * visibility.
     */
    sigc::signal<void> signal_visibilityChanged() const
    {
        return _visibilityChangedSignal;
    }
};

}
