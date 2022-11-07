#include "MD5ModelNode.h"

#include "ivolumetest.h"
#include "ishaders.h"
#include "iscenegraph.h"

namespace md5
{

MD5ModelNode::MD5ModelNode(const MD5ModelPtr& model) :
    _model(new MD5Model(*model)), // create a copy of the incoming model, we need our own instance
    _showSkeleton(RKEY_RENDER_SKELETON),
    _renderableSkeleton(_model->getSkeleton(), localToWorld())
{
    _animationUpdateConnection = _model->signal_ModelAnimationUpdated().connect(
        sigc::mem_fun(*this, &MD5ModelNode::onModelAnimationUpdated)
    );

    _modelShadersChangedConnection = _model->signal_ShadersChanged().connect(
        sigc::mem_fun(*this, &MD5ModelNode::onModelShadersChanged)
    );
}

MD5ModelNode::~MD5ModelNode()
{
    _animationUpdateConnection.disconnect();
}

const model::IModel& MD5ModelNode::getIModel() const
{
    return *_model;
}

model::IModel& MD5ModelNode::getIModel()
{
    return *_model;
}

bool MD5ModelNode::hasModifiedScale()
{
    return false; // not supported
}

Vector3 MD5ModelNode::getModelScale()
{
	return Vector3(1, 1, 1); // not supported
}

void MD5ModelNode::setModel(const MD5ModelPtr& model)
{
    _model = model;
}

const MD5ModelPtr& MD5ModelNode::getModel() const
{
    return _model;
}

// Bounded implementation
const AABB& MD5ModelNode::localAABB() const
{
    return _model->localAABB();
}

std::string MD5ModelNode::name() const
{
    return _model->getFilename();
}

void MD5ModelNode::createRenderableSurfaces()
{
    _model->foreachSurface([&](const MD5Surface& surface)
    {
        if (surface.getVertexArray().empty() || surface.getIndexArray().empty())
        {
            return; // don't handle empty surfaces
        }

        emplaceRenderableSurface(
            std::make_shared<model::RenderableModelSurface>(surface, _renderEntity, localToWorld())
        );
    });
}

void MD5ModelNode::testSelect(Selector& selector, SelectionTest& test)
{
    _model->testSelect(selector, test, localToWorld());
}

bool MD5ModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
    return _model->getIntersection(ray, intersection, localToWorld());
}

void MD5ModelNode::onPreRender(const VolumeTest& volume)
{
    ModelNodeBase::onPreRender(volume);

    if (_showSkeleton.get())
    {
        _renderableSkeleton.queueUpdate();
        _renderableSkeleton.update(_renderEntity->getColourShader());
    }
    else
    {
        _renderableSkeleton.clear();
    }
}

std::string MD5ModelNode::getSkin() const
{
    return _skin;
}

void MD5ModelNode::skinChanged(const std::string& newSkinName)
{
    // greebo: Store the new skin name locally
    _skin = newSkinName;

    // greebo: Acquire the ModelSkin reference from the SkinCache (might return null)
    // Applying the skin might trigger onModelShadersChanged()
    _model->applySkin(GlobalModelSkinCache().findSkin(_skin));

    // Refresh the scene
    GlobalSceneGraph().sceneChanged();
}

void MD5ModelNode::onModelShadersChanged()
{
    // Detach from existing shaders, re-acquire them in onPreRender
    detachFromShaders();
}

void MD5ModelNode::onModelAnimationUpdated()
{
    queueRenderableUpdate();
}

} // namespace md5
