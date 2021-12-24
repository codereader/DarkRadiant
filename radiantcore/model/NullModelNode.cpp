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
        _fillShader = renderSystem->capture("");
        _wireShader = renderSystem->capture("<1.0 0 0>");

        _renderableBox.clear();
        _renderableBox.attachToShader(_fillShader);
        _renderableBox.attachToShader(_wireShader);
    }
    else
    {
        _renderableBox.clear();
        _fillShader.reset();
        _wireShader.reset();
    }
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
        _fillShader->addSurface(_renderableBox);
    }

    if (_wireShader)
    {
        _wireShader->addSurface(_renderableBox);
    }
}

void NullModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    Node::onRemoveFromScene(root);

    _renderableBox.clear();
}

void NullModelNode::onVisibilityChanged(bool isVisibleNow)
{
    Node::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        if (_fillShader)
        {
            _fillShader->addSurface(_renderableBox);
        }

        if (_wireShader)
        {
            _wireShader->addSurface(_renderableBox);
        }
    }
    else
    {
        _renderableBox.clear();
    }
}

} // namespace model
