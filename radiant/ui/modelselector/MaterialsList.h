#pragma once

#include "wxutil/TreeView.h"
#include "wxutil/TreeModel.h"
#include <memory>
#include <sigc++/signal.h>

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

namespace ui
{

/**
 * \brief
 * A treeview that shows a list of materials and a checkbox to enable/disable
 * each material.
 */
class MaterialsList : 
	public wxutil::TreeView
{
private:
    // Data store
    wxutil::TreeModel::Ptr _store;

    // Render system to update
    RenderSystemPtr _renderSystem;

    sigc::signal<void> _visibilityChangedSignal;

private:
    void onShaderToggled(wxDataViewEvent& ev);

public:

    /**
     * \brief
     * Construct a MaterialsList
     *
     * \param renderSystem
     * The render system whose shaders should be made visible or invisible based
     * on clicks in the relevant list column.
     */
    MaterialsList(wxWindow* parent, const RenderSystemPtr& renderSystem);

    /// Clear all entries
    void clear();

    /// Add a material
    void addMaterial(const std::string& name);

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
