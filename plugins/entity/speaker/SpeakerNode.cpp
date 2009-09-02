#include "SpeakerNode.h"

#include "math/frustum.h"

namespace entity {

SpeakerNode::SpeakerNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	_speaker(*this, 
		Node::TransformChangedCaller(*this), 
		Node::BoundsChangedCaller(*this)),
	_dragPlanes(SelectedChangedComponentCaller(*this))
{}

SpeakerNode::SpeakerNode(const SpeakerNode& other) :
	EntityNode(other),
	Snappable(other),
	SelectionTestable(other),
	Cullable(other),
	Bounded(other),
	_speaker(other._speaker, 
		*this, 
		Node::TransformChangedCaller(*this), 
		Node::BoundsChangedCaller(*this)),
	_dragPlanes(SelectedChangedComponentCaller(*this))
{}

void SpeakerNode::construct()
{
	_speaker.construct();
}

// Snappable implementation
void SpeakerNode::snapto(float snap) {
	_speaker.snapto(snap);
}

// Bounded implementation
const AABB& SpeakerNode::localAABB() const {
	return _speaker.localAABB();
}

// Cullable implementation
VolumeIntersectionValue SpeakerNode::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return _speaker.intersectVolume(test, localToWorld);
}

// EntityNode implementation
Entity& SpeakerNode::getEntity() {
	return _entity;
}

void SpeakerNode::refreshModel() {
	// Nothing to do
}

void SpeakerNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
{
	test.BeginMesh(localToWorld());

	_dragPlanes.selectPlanes(localAABB(), selector, test, selectedPlaneCallback);
}

void SpeakerNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	_dragPlanes.selectReversedPlanes(localAABB(), selector, selectedPlanes);
}

void SpeakerNode::testSelect(Selector& selector, SelectionTest& test) {
	_speaker.testSelect(selector, test, localToWorld());
}

void SpeakerNode::selectedChangedComponent(const Selectable& selectable)
{
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool SpeakerNode::isSelectedComponents() const
{
	return _dragPlanes.isSelected();
}

void SpeakerNode::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode)
{
	if (mode == SelectionSystem::eFace)
	{
		_dragPlanes.setSelected(false);
	}
}

void SpeakerNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
{
	// nothing, planes are selected via selectPlanes()
}

scene::INodePtr SpeakerNode::clone() const
{
	SpeakerNodePtr node(new SpeakerNode(*this));
	node->construct();

	return node;
}

/* Renderable implementation */

void SpeakerNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
{
	EntityNode::renderSolid(collector, volume);

	_speaker.renderSolid(collector, volume, localToWorld(), isSelected());
}
void SpeakerNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
{
	EntityNode::renderWireframe(collector, volume);

	_speaker.renderWireframe(collector, volume, localToWorld(), isSelected());
}

void SpeakerNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		_speaker.translate(getTranslation());
		_speaker.rotate(getRotation());
	}
	else
	{
		// This seems to be a drag operation
		_dragPlanes.m_bounds = _speaker.localAABB();
		
		// Let the dragplanes helper resize our local AABB
		AABB resizedAABB = _dragPlanes.evaluateResize(getTranslation(), Matrix4::getIdentity());

		// Let the speaker do the rest of the math
		_speaker.setRadiusFromAABB(resizedAABB);
	}
}

void SpeakerNode::_onTransformationChanged()
{
	_speaker.revertTransform();
	evaluateTransform();
	_speaker.updateTransform();
}

void SpeakerNode::_applyTransformation()
{
	_speaker.revertTransform();
	evaluateTransform();
	_speaker.freezeTransform();
}

} // namespace entity
