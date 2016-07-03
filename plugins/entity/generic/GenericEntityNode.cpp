#include "GenericEntityNode.h"

#include "math/Frustum.h"

namespace entity {

GenericEntityNode::GenericEntityNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_contained(*this),
    _solidAABBRenderMode(SolidBoxes)
{}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	Snappable(other),
	m_contained(other.m_contained, *this),
    _solidAABBRenderMode(other._solidAABBRenderMode)
{}

GenericEntityNodePtr GenericEntityNode::Create(const IEntityClassPtr& eclass)
{
	GenericEntityNodePtr instance(new GenericEntityNode(eclass));
	instance->construct();

	return instance;
}

void GenericEntityNode::construct()
{
	EntityNode::construct();

	m_contained.construct();
}

// Snappable implementation
void GenericEntityNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// Bounded implementation
const AABB& GenericEntityNode::localAABB() const {
	return m_contained.localAABB();
}

void GenericEntityNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	m_contained.testSelect(selector, test, localToWorld());
}

scene::INodePtr GenericEntityNode::clone() const
{
	GenericEntityNodePtr node(new GenericEntityNode(*this));
	node->construct();

	return node;
}

GenericEntityNode::SolidAAABBRenderMode GenericEntityNode::getSolidAABBRenderMode() const
{
    return _solidAABBRenderMode;
}

void GenericEntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	m_contained.renderSolid(collector, volume, localToWorld());
}

void GenericEntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld());
}

const Vector3& GenericEntityNode::getDirection() const
{
	// Return the direction as specified by the angle/rotation keys
	return m_contained.getDirection();
}

void GenericEntityNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.updateTransform();
	}
}

void GenericEntityNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.freezeTransform();
	}
}

const Vector3& GenericEntityNode::getUntransformedOrigin()
{
    return m_contained.getUntransformedOrigin();
}

void GenericEntityNode::onChildAdded(const scene::INodePtr& child) 
{
    EntityNode::onChildAdded(child);

    _solidAABBRenderMode = SolidBoxes;

    // Check if this node has any actual models/particles as children
    Node::foreachNode([&](const scene::INodePtr& node)
    {
        // We consider all non-path-connection childnodes as "models"
        if (child->getNodeType() != scene::INode::Type::EntityConnection)
        {
            _solidAABBRenderMode = WireFrameOnly;
            return false; // stop traversal
        }

        return true;
    });  
}

void GenericEntityNode::onChildRemoved(const scene::INodePtr& child)
{
    EntityNode::onChildRemoved(child);

    _solidAABBRenderMode = SolidBoxes;

    // Check if this node has any actual models/particles as children
    Node::foreachNode([&](const scene::INodePtr& node)
    {
        // We consider all non-path-connection childnodes as "models"
        // Ignore the child itself as this event is raised before the node is actually removed.
        if (node != child && child->getNodeType() != scene::INode::Type::EntityConnection)
        {
            _solidAABBRenderMode = WireFrameOnly;
            return false; // stop traversal
        }

        return true;
    });
}

} // namespace entity
