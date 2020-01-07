#pragma once

#include "RenderPreview.h"

#include "imodel.h"
#include "imap.h"

#include <sigc++/signal.h>
#include <string>
#include <map>

namespace wxutil
{

/** 
 * \brief
 * Preview widget for models and skins.
 *
 * Subclass of RenderPreview which holds a model and displays it optionally with
 * a skin.
 */
class ModelPreview
: public RenderPreview
{
private:
	// TRUE when the scene, model and skin have been set up
	// is set back to FALSE if the model or skin config is changed
	bool _sceneIsReady;

	// The name of the model to render
	std::string _model;

	// The name of the skin to render
	std::string _skin;

    scene::IMapRootNodePtr _rootNode;

    // The parent entity
    scene::INodePtr _entity;

    // Current model to display
    scene::INodePtr _modelNode;

    // The light
    scene::INodePtr _light;

    // Name of last model, to detect changes in model which require camera
    // recalculation
    std::string _lastModel;

	float _defaultCamDistanceFactor;

	sigc::signal<void, const model::ModelNodePtr&> _modelLoadedSignal;

private:

    // Creates parent entity etc.
    void setupSceneGraph() override;
    AABB getSceneBounds() override;
    bool onPreRender() override;
    RenderStateFlags getRenderFlagsFill() override;

	void prepareScene();

protected:
    virtual void onModelRotationChanged() override;

public:

    /// Construct a ModelPreview widget.
    ModelPreview(wxWindow* parent);

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

	// Multiply the model node's AABB radius by this factor to get the default camera distance
	// defaults to 6.
	void setDefaultCamDistanceFactor(float factor);

    /// Return the current model node
    scene::INodePtr getModelNode()
    {
        return _modelNode;
    }

	// Signal emitted when the preview is done loading a new model
	sigc::signal<void, const model::ModelNodePtr&>& signal_ModelLoaded();
};
typedef std::shared_ptr<ModelPreview> ModelPreviewPtr;

} // namespace
