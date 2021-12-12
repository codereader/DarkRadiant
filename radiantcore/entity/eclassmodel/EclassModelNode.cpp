#include "EclassModelNode.h"

#include <functional>

namespace entity {

EclassModelNode::EclassModelNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
    _originKey(std::bind(&EclassModelNode::originChanged, this)),
    _origin(ORIGINKEY_IDENTITY),
    _rotationKey(std::bind(&EclassModelNode::rotationChanged, this)),
    _angleKey(std::bind(&EclassModelNode::angleChanged, this)),
	_angle(AngleKey::IDENTITY),
    _renderOrigin(_origin),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNode::EclassModelNode(const EclassModelNode& other) :
	EntityNode(other),
	Snappable(other),
    _originKey(std::bind(&EclassModelNode::originChanged, this)),
    _origin(ORIGINKEY_IDENTITY),
    _rotationKey(std::bind(&EclassModelNode::rotationChanged, this)),
    _angleKey(std::bind(&EclassModelNode::angleChanged, this)),
	_angle(AngleKey::IDENTITY),
    _renderOrigin(_origin),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNode::~EclassModelNode()
{
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

    _rotation.setIdentity();

    // Observe position and orientation spawnargs
    static_assert(std::is_base_of<sigc::trackable, OriginKey>::value);
    static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
    observeKey("angle", sigc::mem_fun(_rotationKey, &RotationKey::angleChanged));
	observeKey("rotation", sigc::mem_fun(_rotationKey, &RotationKey::rotationChanged));
    observeKey("origin", sigc::mem_fun(_originKey, &OriginKey::onKeyValueChanged));
}

// Snappable implementation
void EclassModelNode::snapto(float snap)
{
    _originKey.snap(snap);
	_originKey.write(_spawnArgs);
}

const AABB& EclassModelNode::localAABB() const
{
	return _localAABB;
}

void EclassModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

    if (isSelected())
	{
		_renderOrigin.render(collector, volume, localToWorld());
	}
}

void EclassModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	if (isSelected())
	{
		_renderOrigin.render(collector, volume, localToWorld());
	}
}

void EclassModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	_renderOrigin.setRenderSystem(renderSystem);
}

scene::INodePtr EclassModelNode::clone() const
{
	EclassModelNodePtr node(new EclassModelNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

void EclassModelNode::translate(const Vector3& translation)
{
	_origin += translation;
}

void EclassModelNode::rotate(const Quaternion& rotation)
{
	_rotation.rotate(rotation);
}

void EclassModelNode::_revertTransform()
{
	_origin = _originKey.get();
	_rotation = _rotationKey.m_rotation;
}

void EclassModelNode::_freezeTransform()
{
	_originKey.set(_origin);
	_originKey.write(_spawnArgs);

	_rotationKey.m_rotation = _rotation;
	_rotationKey.write(&_spawnArgs, true);
}

void EclassModelNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		_revertTransform();

		translate(getTranslation());
		rotate(getRotation());

		updateTransform();
	}
}

void EclassModelNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		_revertTransform();

		translate(getTranslation());
		rotate(getRotation());

		_freezeTransform();
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

void EclassModelNode::angleChanged()
{
	_angle = _angleKey.getValue();
	updateTransform();
}

} // namespace entity
