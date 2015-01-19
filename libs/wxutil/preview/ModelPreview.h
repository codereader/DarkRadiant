#pragma once

#include "RenderPreview.h"

#include "imodel.h"
#include "imap.h"

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

private:

    // Creates parent entity etc.
    void setupSceneGraph();
    AABB getSceneBounds();
    bool onPreRender();
    RenderStateFlags getRenderFlagsFill();

protected:
    virtual void onModelRotationChanged();

public:

    /// Construct a ModelPreview widget.
    ModelPreview(wxWindow* parent);

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
};
typedef std::shared_ptr<ModelPreview> ModelPreviewPtr;

} // namespace
