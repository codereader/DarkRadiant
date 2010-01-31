#include "EclassModelNode.h"

namespace entity {

EclassModelNode::EclassModelNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_contained(*this, Node::TransformChangedCaller(*this)),
	_updateSkin(true)
{}

EclassModelNode::EclassModelNode(const EclassModelNode& other) :
	EntityNode(other),
	Snappable(other),
	m_contained(other.m_contained, 
				*this, 
				Node::TransformChangedCaller(*this)),
	_updateSkin(true)
{}

EclassModelNode::~EclassModelNode() {
	destroy();
}

void EclassModelNode::construct()
{
	m_contained.construct();

	addKeyObserver("skin", SkinChangedCaller(*this));
}

void EclassModelNode::destroy()
{
	removeKeyObserver("skin", SkinChangedCaller(*this));
}

// Snappable implementation
void EclassModelNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// EntityNode implementation
void EclassModelNode::refreshModel() {
	// Simulate a "model" key change
	m_contained.modelChanged(_entity.getKeyValue("model"));

	// Trigger a skin change
	skinChanged(_entity.getKeyValue("skin"));
}

void EclassModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	// greebo: Check if the skin needs updating before rendering.
	if (_updateSkin) {
		// Instantiate a walker class equipped with the new value
		SkinChangedWalker walker(_entity.getKeyValue("skin"));
		// Update all children
		traverse(walker);

		_updateSkin = false;
	}

	m_contained.renderSolid(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::testSelect(Selector& selector, SelectionTest& test)
{
	m_contained.testSelect(selector, test);
}

scene::INodePtr EclassModelNode::clone() const
{
	EclassModelNodePtr node(new EclassModelNode(*this));
	node->construct();

	return node;
}

void EclassModelNode::skinChanged(const std::string& value) {
	// Instantiate a walker class equipped with the new value
	SkinChangedWalker walker(value);
	// Update all children
	traverse(walker);
}

void EclassModelNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();
		
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.updateTransform();
	}
}

void EclassModelNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.freezeTransform();
	}
}

} // namespace entity
