#include "NullModelNode.h"

#include "math/Frustum.h"
#include "entitylib.h"

namespace model
{

NullModelNode::NullModelNode() :
	_nullModel(new NullModel),
    _renderableBox(localAABB(), localToWorld()),
    _attachedToShaders(false)
{}

NullModelNode::NullModelNode(const NullModelPtr& nullModel) :
	_nullModel(nullModel),
    _renderableBox(localAABB(), localToWorld()),
    _attachedToShaders(false)
{}

std::string NullModelNode::name() const
{
	return "nullmodel";
}

scene::INode::Type NullModelNode::getNodeType() const
{
	return Type::Model;
}

const IModel& NullModelNode::getIModel() const
{
	return *_nullModel;
}

IModel& NullModelNode::getIModel()
{
	return *_nullModel;
}

bool NullModelNode::hasModifiedScale()
{
	return false;
}

Vector3 NullModelNode::getModelScale()
{
	return Vector3(1,1,1);
}

void NullModelNode::testSelect(Selector& selector, SelectionTest& test)
{
    test.BeginMesh(localToWorld());

    SelectionIntersection best;
    aabb_testselect(_nullModel->localAABB(), test, best);

    if (best.isValid())
    {
        selector.addIntersection(best);
    }
}

void NullModelNode::onPreRender(const VolumeTest& volume)
{
    assert(_renderEntity);

    // Attach renderables (or do nothing if everything is up to date)
    attachToShaders();
}

void NullModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addHighlightRenderable(_renderableBox, Matrix4::getIdentity());
}

void NullModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    Node::setRenderSystem(renderSystem);

    // Detach renderables on render system change
    detachFromShaders();

    if (renderSystem)
    {
        _fillShader = renderSystem->capture(BuiltInShaderType::MissingModel);
        _wireShader = renderSystem->capture(ColourShaderType::OrthoviewSolid, { 1.0f, 0, 0, 1});
    }
    else
    {
        _fillShader.reset();
        _wireShader.reset();
    }
}

void NullModelNode::attachToShaders()
{
    if (_attachedToShaders || !_renderEntity) return;

    auto renderSystem = _renderSystem.lock();

    if (!renderSystem) return;

    _renderableBox.attachToShader(_fillShader, _renderEntity);
    _renderableBox.attachToShader(_wireShader, _renderEntity);

    _attachedToShaders = true;
}

void NullModelNode::detachFromShaders()
{
    _renderableBox.detach();
    _attachedToShaders = false;
}

const AABB& NullModelNode::localAABB() const
{
	return _nullModel->localAABB();
}

void NullModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    Node::onInsertIntoScene(root);

    if (_fillShader)
    {
        _fillShader->addSurface(_renderableBox, _renderEntity);
    }

    if (_wireShader)
    {
        _wireShader->addSurface(_renderableBox, _renderEntity);
    }
}

void NullModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    Node::onRemoveFromScene(root);

    _renderableBox.detach();
}

void NullModelNode::onVisibilityChanged(bool isVisibleNow)
{
    Node::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        attachToShaders();
    }
    else
    {
        detachFromShaders();
    }
}

} // namespace model
