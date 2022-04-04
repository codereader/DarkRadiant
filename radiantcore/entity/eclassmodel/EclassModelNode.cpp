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
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)), // minimal AABB, is determined by child bounds anyway
    _noShadowsLit(false)
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
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)), // minimal AABB, is determined by child bounds anyway
    _noShadowsLit(false)
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
    observeKey("noshadows_lit", sigc::mem_fun(this, &EclassModelNode::onNoshadowsLitChanged));
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

void EclassModelNode::onPreRender(const VolumeTest& volume)
{
    EntityNode::onPreRender(volume);

    if (isSelected())
    {
        _renderOrigin.update(_pivotShader);
    }
}

void EclassModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

    if (renderSystem)
    {
        _pivotShader = renderSystem->capture(BuiltInShaderType::Pivot);
    }
    else
    {
        _pivotShader.reset();
    }
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
    _renderOrigin.queueUpdate();
}

void EclassModelNode::rotate(const Quaternion& rotation)
{
	_rotation.rotate(rotation);
}

void EclassModelNode::_revertTransform()
{
	_origin = _originKey.get();
    _renderOrigin.queueUpdate();
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

const Vector3& EclassModelNode::getWorldPosition() const
{
    return _origin;
}

void EclassModelNode::updateTransform()
{
    _renderOrigin.queueUpdate();

    setLocalToParent(Matrix4::getTranslation(_origin) * _rotation.getMatrix4());

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

void EclassModelNode::onNoshadowsLitChanged(const std::string& value)
{
    _noShadowsLit = value == "1";
}

bool EclassModelNode::isShadowCasting() const
{
    // Both noShadowsLit and noShadows should be false
    // It's hard to determine whether a compound entity like wall toches are starting
    // in lit state, so we rather turn off shadow casting for them regardless of their state.
    return !_noShadowsLit && EntityNode::isShadowCasting();
}

void EclassModelNode::onSelectionStatusChange(bool changeGroupStatus)
{
    EntityNode::onSelectionStatusChange(changeGroupStatus);

    if (isSelected())
    {
        _renderOrigin.queueUpdate();
    }
    else
    {
        _renderOrigin.clear();
    }
}

} // namespace entity
