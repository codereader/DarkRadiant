#pragma once

#include "RenderPreview.h"

#include "ientity.h"
#include "inode.h"
#include "imap.h"

namespace wxutil
{

/**
 * Preview widget displaying a single entity
 */
class EntityPreview :
    public RenderPreview
{
private:
    // TRUE when the scene, model and skin have been set up
    // is set back to FALSE if the model or skin config is changed
    bool _sceneIsReady;

    scene::IMapRootNodePtr _rootNode;

    // The previewed entity
    IEntityNodePtr _entity;

    AABB _untransformedEntityBounds;

    // The light
    scene::INodePtr _light;

protected:
    float _defaultCamDistanceFactor;

private:
    bool onPreRender() override;
    RenderStateFlags getRenderFlagsFill() override;

protected:
    void setupSceneGraph() override;
    AABB getSceneBounds() override;

    virtual void prepareScene();

    void queueSceneUpdate();

    void onModelRotationChanged() override;

public:
    EntityPreview(wxWindow* parent);

    // Multiply the model node's AABB radius by this factor to get the default camera distance
    // defaults to 6.
    void setDefaultCamDistanceFactor(float factor);

    const IEntityNodePtr& getEntity();
    void setEntity(const IEntityNodePtr& entity);
};

}