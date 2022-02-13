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
    _renderableArrow(*this),
    _renderableBox(*this, localAABB(), worldAABB().getOrigin()),
	_allow3Drotations(_spawnArgs.getKeyValue("editor_rotatable") == "1")
{}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	Snappable(other),
	m_originKey(std::bind(&GenericEntityNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(std::bind(&GenericEntityNode::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_rotationKey(std::bind(&GenericEntityNode::rotationChanged, this)),
    _renderableArrow(*this),
    _renderableBox(*this, localAABB(), worldAABB().getOrigin()),
	_allow3Drotations(_spawnArgs.getKeyValue("editor_rotatable") == "1")
{}

GenericEntityNode::~GenericEntityNode()
{
}

std::shared_ptr<GenericEntityNode> GenericEntityNode::Create(const IEntityClassPtr& eclass)
{
	auto instance = std::make_shared<GenericEntityNode>(eclass);
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
		// Ordinary rotation (2D around z axis), use angle key observer
        static_assert(std::is_base_of<sigc::trackable, AngleKey>::value);
		observeKey("angle", sigc::mem_fun(m_angleKey, &AngleKey::angleChanged));
	}
	else
	{
		// Full 3D rotations allowed, observe both keys using the rotation key observer
        static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
		observeKey("angle", sigc::mem_fun(m_rotationKey, &RotationKey::angleChanged));
        observeKey("rotation", sigc::mem_fun(m_rotationKey, &RotationKey::rotationChanged));
    }

    static_assert(std::is_base_of<sigc::trackable, OriginKey>::value);
	observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));
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
	auto node = std::shared_ptr<GenericEntityNode>(new GenericEntityNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

void GenericEntityNode::onPreRender(const VolumeTest& volume)
{
    EntityNode::onPreRender(volume);

    _renderableBox.update(getColourShader());
    _renderableArrow.update(getColourShader());
}

void GenericEntityNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    EntityNode::renderHighlights(collector, volume);

    collector.addHighlightRenderable(_renderableArrow, Matrix4::getIdentity());
    collector.addHighlightRenderable(_renderableBox, Matrix4::getIdentity());
}

void GenericEntityNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    EntityNode::setRenderSystem(renderSystem);

    // Clear the geometry from any previous shader
    _renderableBox.clear();
    _renderableArrow.clear();
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

void GenericEntityNode::_revertTransform()
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

void GenericEntityNode::_freezeTransform()
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
	setLocalToParent(Matrix4::getTranslation(m_origin));

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

    _renderableBox.queueUpdate();
    _renderableArrow.queueUpdate();
	transformChanged();
}

void GenericEntityNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
        _revertTransform();

		m_origin += getTranslation();
		rotate(getRotation());

		updateTransform();
	}
}

void GenericEntityNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		_revertTransform();

		m_origin += getTranslation();
		rotate(getRotation());

        _freezeTransform();
	}
}

const Vector3& GenericEntityNode::getUntransformedOrigin()
{
    return m_originKey.get();
}

const Vector3& GenericEntityNode::getWorldPosition() const
{
    return m_origin;
}

void GenericEntityNode::onChildAdded(const scene::INodePtr& child)
{
    EntityNode::onChildAdded(child);

    _renderableBox.setFillMode(true);

    // Check if this node has any actual models/particles as children
    Node::foreachNode([&](const scene::INodePtr& node)
    {
        // We consider all non-path-connection childnodes as "models"
        if (child->getNodeType() != scene::INode::Type::EntityConnection)
        {
            _renderableBox.setFillMode(false);
            return false; // stop traversal
        }

        return true;
    });
}

void GenericEntityNode::onChildRemoved(const scene::INodePtr& child)
{
    EntityNode::onChildRemoved(child);

    _renderableBox.setFillMode(true);

    // Check if this node has any actual models/particles as children
    Node::foreachNode([&](const scene::INodePtr& node)
    {
        // We consider all non-path-connection childnodes as "models"
        // Ignore the child itself as this event is raised before the node is actually removed.
        if (node != child && child->getNodeType() != scene::INode::Type::EntityConnection)
        {
            _renderableBox.setFillMode(false);
            return false; // stop traversal
        }

        return true;
    });
}

void GenericEntityNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    // Call the base class first
    EntityNode::onInsertIntoScene(root);

    _renderableBox.queueUpdate();
    _renderableArrow.queueUpdate();
}

void GenericEntityNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    // Call the base class first
    EntityNode::onRemoveFromScene(root);

    _renderableBox.clear();
    _renderableArrow.clear();
}

void GenericEntityNode::onVisibilityChanged(bool isVisibleNow)
{
    EntityNode::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        _renderableBox.queueUpdate();
        _renderableArrow.queueUpdate();
    }
    else
    {
        _renderableBox.clear();
        _renderableArrow.clear();
    }
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
