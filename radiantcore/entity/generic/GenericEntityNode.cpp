#include "GenericEntityNode.h"
#include "../EntitySettings.h"

#include "math/Frustum.h"

namespace entity
{

GenericEntityNode::GenericEntityNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_originKey(std::bind(&GenericEntityNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(std::bind(&GenericEntityNode::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_rotationKey(std::bind(&GenericEntityNode::rotationChanged, this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_allow3Drotations(_spawnArgs.getKeyValue("editor_rotatable") == "1"),
    _solidAABBRenderMode(SolidBoxes)
{}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	Snappable(other),
	m_originKey(std::bind(&GenericEntityNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(std::bind(&GenericEntityNode::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_rotationKey(std::bind(&GenericEntityNode::rotationChanged, this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_allow3Drotations(_spawnArgs.getKeyValue("editor_rotatable") == "1"),
    _solidAABBRenderMode(other._solidAABBRenderMode)
{}

GenericEntityNode::~GenericEntityNode()
{
	if (!_allow3Drotations)
	{
		// Ordinary rotation (2D around z axis), use angle key observer
		removeKeyObserver("angle", _angleObserver);
	}
	else
	{
		// Full 3D rotations allowed, observe both keys using the rotation key observer
		removeKeyObserver("angle", _angleObserver);
		removeKeyObserver("rotation", _rotationObserver);
	}

	removeKeyObserver("origin", m_originKey);
}

GenericEntityNodePtr GenericEntityNode::Create(const IEntityClassPtr& eclass)
{
	GenericEntityNodePtr instance(new GenericEntityNode(eclass));
	instance->construct();

	return instance;
}

void GenericEntityNode::construct()
{
	EntityNode::construct();

	m_aabb_local = _spawnArgs.getEntityClass()->getBounds();
	m_ray.origin = m_aabb_local.getOrigin();
	m_ray.direction = Vector3(1, 0, 0);
	m_rotation.setIdentity();

	if (!_allow3Drotations)
	{
		_angleObserver.setCallback(std::bind(&AngleKey::angleChanged, &m_angleKey, std::placeholders::_1));

		// Ordinary rotation (2D around z axis), use angle key observer
		addKeyObserver("angle", _angleObserver);
	}
	else
	{
		_angleObserver.setCallback(std::bind(&RotationKey::angleChanged, &m_rotationKey, std::placeholders::_1));
		_rotationObserver.setCallback(std::bind(&RotationKey::rotationChanged, &m_rotationKey, std::placeholders::_1));

		// Full 3D rotations allowed, observe both keys using the rotation key observer
		addKeyObserver("angle", _angleObserver);
		addKeyObserver("rotation", _rotationObserver);
	}

	addKeyObserver("origin", m_originKey);
}

void GenericEntityNode::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
}

const AABB& GenericEntityNode::localAABB() const
{
	return m_aabb_local;
}

void GenericEntityNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());

	SelectionIntersection best;
	aabb_testselect(m_aabb_local, test, best);
	if(best.isValid()) {
		selector.addIntersection(best);
	}
}

scene::INodePtr GenericEntityNode::clone() const
{
	GenericEntityNodePtr node(new GenericEntityNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

GenericEntityNode::SolidAAABBRenderMode GenericEntityNode::getSolidAABBRenderMode() const
{
    return _solidAABBRenderMode;
}

void GenericEntityNode::renderArrow(const ShaderPtr& shader, RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (EntitySettings::InstancePtr()->getShowEntityAngles())
	{
		collector.addRenderable(*shader, m_arrow, localToWorld);
	}
}

void GenericEntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	const ShaderPtr& shader = getSolidAABBRenderMode() == GenericEntityNode::WireFrameOnly ?
		getWireShader() : getFillShader();

	collector.addRenderable(*shader, m_aabb_solid, localToWorld());
	renderArrow(shader, collector, volume, localToWorld());
}

void GenericEntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	collector.addRenderable(*getWireShader(), m_aabb_wire, localToWorld());
	renderArrow(getWireShader(), collector, volume, localToWorld());
}

const Vector3& GenericEntityNode::getDirection() const
{
	return m_ray.direction;
}

void GenericEntityNode::rotate(const Quaternion& rotation)
{
	if (_allow3Drotations)
	{
        m_rotation.rotate(rotation);
	}
	else
	{
		m_angle = AngleKey::getRotatedValue(m_angle, rotation);
	}
}

void GenericEntityNode::revertTransform()
{
	m_origin = m_originKey.get();

	if (_allow3Drotations)
	{
		m_rotation = m_rotationKey.m_rotation;
	}
	else
	{
		m_angle = m_angleKey.getValue();
	}
}

void GenericEntityNode::freezeTransform()
{
	m_originKey.set(m_origin);
	m_originKey.write(_spawnArgs);

	if (_allow3Drotations)
	{
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.m_rotation.writeToEntity(&_spawnArgs);
	}
	else
	{
		m_angleKey.setValue(m_angle);
		m_angleKey.write(&_spawnArgs);
	}
}

void GenericEntityNode::updateTransform()
{
	localToParent() = Matrix4::getTranslation(m_origin);

	if (_allow3Drotations)
	{
		// greebo: Use the z-direction as base for rotations
		m_ray.direction = m_rotation.getMatrix4().transformDirection(Vector3(0,0,1));
	}
	else
	{
        m_ray.direction = Matrix4::getRotationAboutZ(math::Degrees(m_angle))
                                  .transformDirection(Vector3(1, 0, 0));
    }

	transformChanged();
}

void GenericEntityNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		revertTransform();

		m_origin += getTranslation();
		rotate(getRotation());

		updateTransform();
	}
}

void GenericEntityNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		revertTransform();

		m_origin += getTranslation();
		rotate(getRotation());

		freezeTransform();
	}
}

const Vector3& GenericEntityNode::getUntransformedOrigin()
{
    return m_originKey.get();
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

void GenericEntityNode::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();
}

void GenericEntityNode::angleChanged()
{
	// Ignore the angle key when 3D rotations are enabled
	if (_allow3Drotations) return;

	m_angle = m_angleKey.getValue();
	updateTransform();
}

void GenericEntityNode::rotationChanged()
{
	// Ignore the rotation key, when in 2D "angle" mode
	if (!_allow3Drotations) return;

	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}


} // namespace entity
