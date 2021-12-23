#include "NullModelNode.h"

#include "math/Frustum.h"

namespace model
{

NullModelNode::NullModelNode() :
	_nullModel(new NullModel),
    _renderableBox(localAABB(), localToWorld())
{}

NullModelNode::NullModelNode(const NullModelPtr& nullModel) :
	_nullModel(nullModel),
    _renderableBox(localAABB(), localToWorld())
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
	_nullModel->testSelect(selector, test, localToWorld());
}

void NullModelNode::onPreRender(const VolumeTest& volume)
{
    Node::onPreRender(volume);

    _renderableBox.update(_shader);
}

void NullModelNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
}

void NullModelNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
}

void NullModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addHighlightRenderable(_renderableBox, Matrix4::getIdentity());
}

void NullModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    if (renderSystem)
    {
        _shader = renderSystem->capture("");
    }
    else
    {
        _shader.reset();
    }
}

const AABB& NullModelNode::localAABB() const
{
	return _nullModel->localAABB();
}

void NullModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    Node::onInsertIntoScene(root);

    _renderableBox.queueUpdate();
}

void NullModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    Node::onRemoveFromScene(root);

    _renderableBox.clear();
}

void NullModelNode::boundsChanged()
{
    Node::boundsChanged();

    _renderableBox.queueUpdate();
}

void NullModelNode::onVisibilityChanged(bool isVisibleNow)
{
    Node::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        _renderableBox.queueUpdate();
    }
    else
    {
        _renderableBox.clear();
    }
}

} // namespace model
