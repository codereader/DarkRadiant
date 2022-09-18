#pragma once

#include "imodel.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/menu/PopupMenu.h"
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

    wxutil::PopupMenuPtr _contextMenu;

private:
    void onShaderToggled(wxDataViewEvent& ev);
    void onContextMenu(wxDataViewEvent& ev);
    void onShowShaderDefinition();
    std::string getSelectedMaterial();

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

    // Populate the table entries using the given model node
    void updateFromModel(const model::IModel& model);

    void clear();

    /**
     * \brief
     * Signal emitted when material visibility has changed.
     *
     * This indicates that a redraw will be needed based on the new material
     * visibility.
     */
    sigc::signal<void>& signal_visibilityChanged()
    {
        return _visibilityChangedSignal;
    }

private:
    void addMaterial(const std::string& name);
};

}
