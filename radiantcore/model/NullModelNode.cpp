#include "NullModelNode.h"

#include "entitylib.h"
#include "NullModelBoxSurface.h"

namespace model
{

NullModelNode::NullModelNode() :
	_nullModel(new NullModel),
    _boxSurface(localAABB(), localToWorld())
{}

NullModelNode::NullModelNode(const NullModelPtr& nullModel) :
	_nullModel(nullModel),
    _boxSurface(localAABB(), localToWorld())
{}

std::string NullModelNode::name() const
{
	return "Nullmodel";
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

void NullModelNode::createRenderableSurfaces()
{
    emplaceRenderableSurface(std::make_shared<NullModelBoxSurface>(_boxSurface, _renderEntity, localToWorld()));
}

void NullModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    ModelNodeBase::setRenderSystem(renderSystem);

    // Detach renderables on render system change
    detachFromShaders();
}

const AABB& NullModelNode::localAABB() const
{
	return _nullModel->localAABB();
}

void NullModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    ModelNodeBase::onInsertIntoScene(root);

    // When inserted into the scene, the localToWorld matrix has to be re-evaluated
    transformChanged();
}

} // namespace
