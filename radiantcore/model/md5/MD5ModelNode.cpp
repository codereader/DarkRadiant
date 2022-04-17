#include "MD5ModelNode.h"

#include "ivolumetest.h"
#include "imodelcache.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include <functional>

namespace md5
{

MD5ModelNode::MD5ModelNode(const MD5ModelPtr& model) :
    _model(new MD5Model(*model)), // create a copy of the incoming model, we need our own instance
    _attachedToShaders(false),
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

scene::INode::Type MD5ModelNode::getNodeType() const
{
    return Type::Model;
}

void MD5ModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    // Renderables will acquire their shaders in onPreRender
    _model->foreachSurface([&](const MD5Surface& surface)
    {
        if (surface.getVertexArray().empty() || surface.getIndexArray().empty())
        {
            return; // don't handle empty surfaces
        }

        _renderableSurfaces.emplace_back(
            std::make_shared<model::RenderableModelSurface>(surface, _renderEntity, localToWorld())
        );
    });

    Node::onInsertIntoScene(root);
}

void MD5ModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    Node::onRemoveFromScene(root);

    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _renderableSurfaces.clear();
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
    assert(_renderEntity);

    // Attach renderables (or do nothing if everything is up to date)
    attachToShaders();

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

void MD5ModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    auto identity = Matrix4::getIdentity();

    for (const auto& surface : _renderableSurfaces)
    {
        collector.addHighlightRenderable(*surface, identity);
    }
}

void MD5ModelNode::detachFromShaders()
{
    // Detach any existing surfaces. In case we need them again,
    // the node will re-attach in the next pre-render phase
    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _attachedToShaders = false;
}

void MD5ModelNode::attachToShaders()
{
    if (_attachedToShaders || !_renderEntity) return;

    auto renderSystem = _renderSystem.lock();

    if (!renderSystem) return;

    for (auto& surface : _renderableSurfaces)
    {
        auto shader = renderSystem->capture(surface->getSurface().getActiveMaterial());
        surface->attachToShader(shader);

        // For orthoview rendering we need the entity's wireframe shader
        surface->attachToShader(_renderEntity->getWireShader());

        // Attach to the render entity for lighting mode rendering
        surface->attachToEntity(_renderEntity, shader);
    }

    _attachedToShaders = true;
}

std::string MD5ModelNode::getSkin() const
{
    return _skin;
}

void MD5ModelNode::skinChanged(const std::string& newSkinName)
{
    // greebo: Store the new skin name locally
    _skin = newSkinName;

    // greebo: Acquire the ModelSkin reference from the SkinCache
    // Note: This always returns a valid reference
    auto& skin = GlobalModelSkinCache().capture(_skin);

    // Applying the skin might trigger onModelShadersChanged()
    _model->applySkin(skin);

    // Refresh the scene
    GlobalSceneGraph().sceneChanged();
}

void MD5ModelNode::onModelShadersChanged()
{
    // Detach from existing shaders, re-acquire them in onPreRender
    detachFromShaders();
}

void MD5ModelNode::onVisibilityChanged(bool isVisibleNow)
{
    if (isVisibleNow)
    {
        attachToShaders();
    }
    else
    {
        detachFromShaders();
    }
}

void MD5ModelNode::onModelAnimationUpdated()
{
    for (auto& surface : _renderableSurfaces)
    {
        surface->queueUpdate();
    }
}

void MD5ModelNode::transformChangedLocal()
{
    Node::transformChangedLocal();

    for (auto& surface : _renderableSurfaces)
    {
        surface->boundsChanged();
    }
}

} // namespace md5
