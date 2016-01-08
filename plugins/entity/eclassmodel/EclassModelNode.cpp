#include "EclassModelNode.h"

#include <functional>

namespace entity {

EclassModelNode::EclassModelNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
    m_contained(*this, Callback(std::bind(&Node::transformChanged, this))),
    _originKey(std::bind(&EclassModelNode::originChanged, this)),
    _origin(ORIGINKEY_IDENTITY),
    _rotationKey(std::bind(&EclassModelNode::rotationChanged, this)),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNode::EclassModelNode(const EclassModelNode& other) :
	EntityNode(other),
	Snappable(other),
    m_contained(other.m_contained,
				*this,
				Callback(std::bind(&Node::transformChanged, this))),
    _originKey(std::bind(&EclassModelNode::originChanged, this)),
    _origin(ORIGINKEY_IDENTITY),
    _rotationKey(std::bind(&EclassModelNode::rotationChanged, this)),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNode::~EclassModelNode()
{
    removeKeyObserver("origin", _originKey);
}

EclassModelNodePtr EclassModelNode::Create(const IEntityClassPtr& eclass)
{
	EclassModelNodePtr instance(new EclassModelNode(eclass));
	instance->construct();

	return instance;
}

void EclassModelNode::construct()
{
	EntityNode::construct();

    m_contained.construct();

    _rotation.setIdentity();

    addKeyObserver("origin", _originKey);
}

// Snappable implementation
void EclassModelNode::snapto(float snap)
{
    _originKey.snap(snap);
	_originKey.write(_entity);
}

const AABB& EclassModelNode::localAABB() const
{
	return _localAABB;
}

void EclassModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	m_contained.renderSolid(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	m_contained.setRenderSystem(renderSystem);
}

scene::INodePtr EclassModelNode::clone() const
{
	EclassModelNodePtr node(new EclassModelNode(*this));
	node->construct();

	return node;
}

void EclassModelNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		updateTransform();
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

const Vector3& EclassModelNode::getUntransformedOrigin()
{
    return _originKey.get();
}

void EclassModelNode::updateTransform()
{
	localToParent() = Matrix4::getIdentity();
	localToParent().translateBy(_origin);

	localToParent().multiplyBy(_rotation.getMatrix4());

	EntityNode::transformChanged();
}

void EclassModelNode::originChanged()
{
	_origin = _originKey.get();
	updateTransform();
}

void EclassModelNode::rotationChanged()
{
	_rotation = _rotationKey.m_rotation;
	updateTransform();
}

} // namespace entity
