#pragma once

#include "EntityPreview.h"

#include "imodel.h"

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <string>

namespace wxutil
{

/**
* \brief
* Preview widget for models and skins.
*
* Subclass of EntityPreview (showing a hidden func_static entity)
* which holds a model and displays it optionally with a skin.
*/
class ModelPreview :
    public EntityPreview
{
private:
    // The name of the model to render
    std::string _model;

    // The name of the skin to render
    std::string _skin;

    // Name of last model, to detect changes in model which require camera
    // recalculation
    std::string _lastModel;

    // Current model to display
    scene::INodePtr _modelNode;

    sigc::signal<void, const model::ModelNodePtr&> _modelLoadedSignal;
    sigc::connection _skinDeclChangedConn;

public:
    ModelPreview(wxWindow* parent);
    ~ModelPreview() override;

    // Returns the name of the current model
    const std::string& getModel() const;

    // Returns the name of the current skin
    const std::string& getSkin() const;

    /**
     * Set the widget to display the given model. If the model name is the
     * empty string, the widget will release the currently displayed model.
     *
     * @param
     * String name of the model to display.
     */
    void setModel(const std::string& model);

    /// Set the skin to apply to the model for rendering
    void setSkin(const std::string& skin);

    /// Return the current model node
    scene::INodePtr getModelNode()
    {
        return _modelNode;
    }

    // Signal emitted when the preview is done loading a new model
    sigc::signal<void, const model::ModelNodePtr&>& signal_ModelLoaded();

protected:
    void prepareScene() override;

    void setupSceneGraph() override;
    AABB getSceneBounds() override;
    void applySkin();
    void onSkinDeclarationChanged();
    void setupInitialViewPosition() override;
};

} // namespace
