#include "Doom3Group.h"

#include "iregistry.h"
#include "selectable.h"
#include "render.h"
#include "transformlib.h"

namespace entity {

class InstanceFunctor
{
public:
	virtual void operator() (scene::Instance& instance) const = 0;
};

class InstanceVisitor :
	public scene::Instantiable::Visitor
{
	const InstanceFunctor& _functor;
public:
	InstanceVisitor(const InstanceFunctor& functor) :
		_functor(functor)
	{}

	void visit(scene::Instance& instance) const {
		_functor(instance);
	}
};

class ChildTranslator : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
	const Vector3& _translation;
public:
	ChildTranslator(const Vector3& translation) :
		_translation(translation)
	{}

	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}
	
	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);
		
		if (transformable != NULL) {
			transformable->setTranslation(_translation);
		}
	}
};

class ChildRotator : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
	const Quaternion& _rotation;
public:
	ChildRotator(const Quaternion& rotation) :
		_rotation(rotation)
	{}

	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}
	
	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);
		
		if (transformable != NULL) {
			transformable->setRotation(_rotation);
		}
	}
};

class ChildTransformReverter : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
public:
	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}
	
	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);
		
		if (transformable != NULL) {
			transformable->revertTransform();
		}
	}
};

class ChildTransformFreezer : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
public:
	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}
	
	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);
		
		if (transformable != NULL) {
			transformable->freezeTransform();
		}
	}
};


inline void PointVertexArray_testSelect(PointVertex* first, std::size_t count, 
	SelectionTest& test, SelectionIntersection& best) 
{
	test.TestLineStrip(
	    VertexPointer(
	        reinterpret_cast<VertexPointer::pointer>(&first->vertex),
	        sizeof(PointVertex)
	    ),
	    IndexPointer::index_type(count),
	    best
	);
}

Doom3Group::Doom3Group(IEntityClassPtr eclass, scene::Node& node, 
		const Callback& transformChanged, 
		const Callback& boundsChanged, 
		const Callback& evaluateTransform) :
	m_entity(eclass),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_named(m_entity),
	m_nameKeys(m_entity),
	m_funcStaticOrigin(m_traverse, m_origin),
	m_renderOrigin(m_origin),
	m_renderName(m_named, m_origin),
	m_skin(SkinChangedCaller(*this)),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform),
	m_traversable(0),
	m_curveNURBS(boundsChanged),
	m_curveCatmullRom(boundsChanged)
{
	construct();
}

Doom3Group::Doom3Group(const Doom3Group& other, scene::Node& node, 
		const Callback& transformChanged, 
		const Callback& boundsChanged, 
		const Callback& evaluateTransform) :
	m_entity(other.m_entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_named(m_entity),
	m_nameKeys(m_entity),
	m_funcStaticOrigin(m_traverse, m_origin),
	m_renderOrigin(m_origin),
	m_renderName(m_named, g_vector3_identity),
	m_skin(SkinChangedCaller(*this)),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform),
	m_traversable(0),
	m_curveNURBS(boundsChanged),
	m_curveCatmullRom(boundsChanged)
{
	construct();
}

Doom3Group::~Doom3Group() {
	destroy();
}

void Doom3Group::instanceAttach(const scene::Path& path) {
	if (++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_traverse.instanceAttach(path_find_mapfile(path.begin(), path.end()));

		m_funcStaticOrigin.enable();
	}
}

void Doom3Group::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		m_funcStaticOrigin.disable();

		m_traverse.instanceDetach(path_find_mapfile(path.begin(), path.end()));
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

EntityKeyValues& Doom3Group::getEntity() {
	return m_entity;
}
const EntityKeyValues& Doom3Group::getEntity() const {
	return m_entity;
}

scene::Traversable& Doom3Group::getTraversable() {
	return *m_traversable;
}

Namespaced& Doom3Group::getNamespaced() {
	return m_nameKeys;
}

Nameable& Doom3Group::getNameable() {
	return m_named;
}

TransformNode& Doom3Group::getTransformNode() {
	return m_transform;
}

ModelSkin& Doom3Group::getModelSkin() {
	return m_skin.get();
}

void Doom3Group::attach(scene::Traversable::Observer* observer) {
	m_traverseObservers.attach(*observer);
}

void Doom3Group::detach(scene::Traversable::Observer* observer) {
	m_traverseObservers.detach(*observer);
}

const AABB& Doom3Group::localAABB() const {
	m_curveBounds = m_curveNURBS.m_bounds;
	m_curveBounds.includeAABB(m_curveCatmullRom.m_bounds);
	return m_curveBounds;
}

void Doom3Group::renderSolid(Renderer& renderer, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	if (selected) {
		m_renderOrigin.render(renderer, volume, localToWorld);
	}

	renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
	renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eFullMaterials);

	if (!m_curveNURBS.m_renderCurve.m_vertices.empty()) {
		renderer.addRenderable(m_curveNURBS.m_renderCurve, localToWorld);
	}
	if (!m_curveCatmullRom.m_renderCurve.m_vertices.empty()) {
		renderer.addRenderable(m_curveCatmullRom.m_renderCurve, localToWorld);
	}
}

void Doom3Group::renderWireframe(Renderer& renderer, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	renderSolid(renderer, volume, localToWorld, selected);
	if (GlobalRegistry().get("user/ui/xyview/showEntityNames") == "1") {
		renderer.addRenderable(m_renderName, localToWorld);
	}
}

void Doom3Group::testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best) {
	PointVertexArray_testSelect(&m_curveNURBS.m_renderCurve.m_vertices[0], m_curveNURBS.m_renderCurve.m_vertices.size(), test, best);
	PointVertexArray_testSelect(&m_curveCatmullRom.m_renderCurve.m_vertices[0], m_curveCatmullRom.m_renderCurve.m_vertices.size(), test, best);
}

void Doom3Group::translate(const Vector3& translation) {
	std::cout << "Doom3Group::translate...\n";
	//m_origin = origin_translated(m_origin, translation);
	m_renderOrigin.updatePivot();
	
	if (m_instanceCounter.m_count > 0) {
		if (m_traversable != NULL) {
			std::cout << "Translating children..." << translation << "\n";
			m_traversable->traverse(ChildTranslator(translation));
		}
	}
}

void Doom3Group::rotate(const Quaternion& rotation) {
	std::cout << "Doom3Group::rotate...\n";
	if (m_instanceCounter.m_count > 0) {
		if (m_traversable != NULL) {
			std::cout << "Rotating children..." << rotation[0] << "," << rotation[1] << "," << rotation[2] << "," << rotation[3] << "\n";
			m_traversable->traverse(ChildRotator(rotation));
		}
	}
	else {
		rotation_rotate(m_rotation, rotation);
	}
}

void Doom3Group::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void Doom3Group::revertTransform() {
	m_origin = m_originKey.m_origin;
	if (m_instanceCounter.m_count == 0) {
		rotation_assign(m_rotation, m_rotationKey.m_rotation);
	}
	m_curveNURBS.m_controlPointsTransformed = m_curveNURBS.m_controlPoints;
	m_curveCatmullRom.m_controlPointsTransformed = m_curveCatmullRom.m_controlPoints;
}

void Doom3Group::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);
	
	if (m_instanceCounter.m_count > 0) {
		if (m_traversable != NULL) {
			std::cout << "Freezing children...\n";
			m_traversable->traverse(ChildTransformFreezer());
		}
	}
	
	if (m_instanceCounter.m_count == 0) {
		rotation_assign(m_rotationKey.m_rotation, m_rotation);
	}
	m_rotationKey.write(&m_entity);
	m_curveNURBS.m_controlPoints = m_curveNURBS.m_controlPointsTransformed;
	ControlPoints_write(m_curveNURBS.m_controlPoints, curve_Nurbs, m_entity);
	m_curveCatmullRom.m_controlPoints = m_curveCatmullRom.m_controlPointsTransformed;
	ControlPoints_write(m_curveCatmullRom.m_controlPoints, curve_CatmullRomSpline, m_entity);
}

void Doom3Group::transformChanged() {
	std::cout << "Doom3Group::transformChanged() called\n";
	
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (m_instanceCounter.m_count > 0) {
		if (m_traversable != NULL) {
			std::cout << "Traversing children...\n";
			
			m_traversable->traverse(ChildTransformReverter());
			m_evaluateTransform();
			//m_traversable->traverse(ChildTransformEvaluator());
			//m_traversable->traverse(ChildRotator(rotation));
		}
	}
	else {
		revertTransform();
		m_evaluateTransform();
		updateTransform();
		m_curveNURBS.curveChanged();
		m_curveCatmullRom.curveChanged();
	}
}

void Doom3Group::construct() {
	default_rotation(m_rotation);

	m_keyObservers.insert(Static<KeyIsName>::instance().m_nameKey, NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("model", Doom3Group::ModelChangedCaller(*this));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("name", NameChangedCaller(*this));
	m_keyObservers.insert(curve_Nurbs, NURBSCurve::CurveChangedCaller(m_curveNURBS));
	m_keyObservers.insert(curve_CatmullRomSpline, CatmullRomSpline::CurveChangedCaller(m_curveCatmullRom));
	m_keyObservers.insert("skin", ModelSkinKey::SkinChangedCaller(m_skin));

	m_traverseObservers.attach(m_funcStaticOrigin);
	m_isModel = false;
	m_nameKeys.setKeyIsName(keyIsNameDoom3Doom3Group);
	attachTraverse();

	m_entity.attach(m_keyObservers);
}

void Doom3Group::destroy() {
	m_entity.detach(m_keyObservers);

	if (isModel()) {
		detachModel();
	}
	else {
		detachTraverse();
	}

	m_traverseObservers.detach(m_funcStaticOrigin);
}

void Doom3Group::attachModel() {
	m_traversable = &m_model.getTraversable();
	m_model.attach(&m_traverseObservers);
}
void Doom3Group::detachModel() {
	m_traversable = 0;
	m_model.detach(&m_traverseObservers);
}

void Doom3Group::attachTraverse() {
	m_traversable = &m_traverse;
	m_traverse.attach(&m_traverseObservers);
}

void Doom3Group::detachTraverse() {
	m_traversable = 0;
	m_traverse.detach(&m_traverseObservers);
}

bool Doom3Group::isModel() const {
	return m_isModel;
}

void Doom3Group::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		detachTraverse();
		attachModel();

		m_nameKeys.setKeyIsName(Static<KeyIsName>::instance().m_keyIsName);
		m_model.modelChanged(m_modelKey.c_str());
	}
	else if (!newValue && m_isModel) {
		detachModel();
		attachTraverse();

		m_nameKeys.setKeyIsName(keyIsNameDoom3Doom3Group);
	}
	m_isModel = newValue;
	updateTransform();
}

/** Determine if this Doom3Group is a model (func_static) or a
 * brush-containing entity. If the "model" key is equal to the
 * "name" key, then this is a brush-based entity, otherwise it is
 * a model entity. The exception to this is for the "worldspawn"
 * entity class, which is always a brush-based entity.
 */
void Doom3Group::updateIsModel() {
	if (m_modelKey != m_name &&
	        std::string(m_entity.getKeyValue("classname")) != "worldspawn") {
		setIsModel(true);
	}
	else {
		setIsModel(false);
	}
}

void Doom3Group::nameChanged(const char* value) {
	m_name = value;
	updateIsModel();
}

void Doom3Group::modelChanged(const char* value) {
	m_modelKey = value;
	updateIsModel();
	if (isModel()) {
		m_model.modelChanged(value);
	}
	else {
		m_model.modelChanged("");
	}
}

void Doom3Group::updateTransform() {
	m_transform.localToParent() = g_matrix4_identity;
	if (isModel()) {
		matrix4_translate_by_vec3(m_transform.localToParent(), m_origin);
		matrix4_multiply_by_matrix4(m_transform.localToParent(), rotation_toMatrix(m_rotation));
	}
	
	// Notify the InstanceSet of the Doom3GroupNode about this transformation change	 
	m_transformChanged();
	
	if (!isModel()) {
		m_funcStaticOrigin.originChanged();
	}
}

void Doom3Group::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
	m_renderOrigin.updatePivot();
}

void Doom3Group::rotationChanged() {
	rotation_assign(m_rotation, m_rotationKey.m_rotation);
	updateTransform();
}

void Doom3Group::skinChanged() {
	if (isModel()) {
		scene::Node* node = m_model.getNode();
		if (node != 0) {
			Node_modelSkinChanged(*node);
		}
	}
}

} // namespace entity
