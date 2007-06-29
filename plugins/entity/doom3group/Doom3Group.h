#ifndef DOOM3GROUP_H_
#define DOOM3GROUP_H_

#include "Bounded.h"
#include "editable.h"
#include "entitylib.h"
#include "traverselib.h"
#include "pivot.h"

#include "../keyobservers.h"
#include "../model.h"
#include "../origin.h"
#include "../rotation.h"
#include "../namedentity.h"
#include "../namekeys.h"
#include "../modelskinkey.h"
#include "../Doom3Entity.h"
#include "../curve/CurveCatmullRom.h"
#include "../curve/CurveNURBS.h"

namespace entity {

class Doom3Group :
	public Bounded,
	public Snappable 
{
	Doom3Entity _entity;
	KeyObserverMap m_keyObservers;
	TraversableNodeSet m_traverse;
	MatrixTransform m_transform;

	SingletonModel m_model;
	OriginKey m_originKey;
	Vector3 m_origin;
	
	// A separate origin for the renderable names and pivot points
	Vector3 m_nameOrigin;
	 
	RotationKey m_rotationKey;
	Float9 m_rotation;

	NamedEntity m_named;
	NameKeys m_nameKeys;
	//TraversableObserverPairRelay m_traverseObservers;
	scene::Traversable::Observer* _traverseObserver; // This is the "surrounding" Doom3GroupNode
	RenderablePivot m_renderOrigin;
	RenderableNamedEntity m_renderName;
	ModelSkinKey m_skin;

	mutable AABB m_curveBounds;

	Callback m_transformChanged;
	Callback m_evaluateTransform;

	// The value of the "name" key for this Doom3Group.
	std::string m_name;

	// The value of the "model" key for this Doom3Group.
	std::string m_modelKey;

	// Flag to indicate this Doom3Group is a model (i.e. does not contain
	// brushes).
	bool m_isModel;

	scene::Traversable* m_traversable;
	
public:
	CurveNURBS m_curveNURBS;
	SignalHandlerId m_curveNURBSChanged;
	CurveCatmullRom m_curveCatmullRom;
	SignalHandlerId m_curveCatmullRomChanged;
	InstanceCounter m_instanceCounter;

	/** greebo: The constructor takes the Entity class and the Node as argument
	 * as well as some callbacks for transformation and bounds changes.
	 * 
	 * These callbacks point to and InstanceSet::transformChangedCaller(), for example.  
	 */
	Doom3Group(IEntityClassPtr eclass, scene::Node& node, 
			   const Callback& transformChanged, 
			   const Callback& boundsChanged, 
			   const Callback& evaluateTransform);
	
	// Copy constructor
	Doom3Group(const Doom3Group& other, scene::Node& node, 
			   const Callback& transformChanged, 
			   const Callback& boundsChanged, 
			   const Callback& evaluateTransform);
	
	~Doom3Group();

	/** greebo: Called right before map save to recalculate
	 * the child brush position.
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	Doom3Entity& getEntity();
	const Doom3Entity& getEntity() const;

	scene::Traversable& getTraversable();
	const scene::Traversable& getTraversable() const;
	Namespaced& getNamespaced();
	NamedEntity& getNameable();
	const NamedEntity& getNameable() const;
	TransformNode& getTransformNode();
	const TransformNode& getTransformNode() const;
	ModelSkin& getModelSkin();
	const ModelSkin& getModelSkin() const;

	// This gets called by the Doom3GroupNode constructor and attaches the
	// Node class as observer of this Doom3Group
	void attach(scene::Traversable::Observer* observer);
	void detach(scene::Traversable::Observer* observer);

	const AABB& localAABB() const;
	
	Vector3& getOrigin();
	
	// Curve-related methods
	void appendControlPoints(unsigned int numPoints);
	void convertCurveType();
	
	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;

	void testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best);

	/** greebo: Translates this Doom3Group
	 * 
	 * @translation: The translation vector
	 * @rotation: TRUE, if the translation was due to a rotation
	 * 			  (this inhibits the movement of the origin)
	 */
	void translate(const Vector3& translation, bool rotation = false);
	void rotate(const Quaternion& rotation);
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();
	void transformChanged();
	typedef MemberCaller<Doom3Group, &Doom3Group::transformChanged> TransformChangedCaller;
	
	// Translates the origin only (without the children)
	void translateOrigin(const Vector3& translation);
	// Snaps the origin to the grid
	void snapOrigin(float snap);
	
	void translateChildren(const Vector3& childTranslation);
	
private:
	void construct();
	void destroy();

	void attachModel();
	void detachModel();
	
	void attachTraverse();
	void detachTraverse();

	bool isModel() const;

	void setIsModel(bool newValue);

	/** Determine if this Doom3Group is a model (func_static) or a
	 * brush-containing entity. If the "model" key is equal to the
	 * "name" key, then this is a brush-based entity, otherwise it is
	 * a model entity. The exception to this is for the "worldspawn"
	 * entity class, which is always a brush-based entity.
	 */
	void updateIsModel();

	void nameChanged(const std::string& value);
	typedef MemberCaller1<Doom3Group, const std::string&, &Doom3Group::nameChanged> NameChangedCaller;

	void modelChanged(const std::string& value);
	typedef MemberCaller1<Doom3Group, const std::string&, &Doom3Group::modelChanged> ModelChangedCaller;

	void updateTransform();
	typedef MemberCaller<Doom3Group, &Doom3Group::updateTransform> UpdateTransformCaller;

	void originChanged();
	typedef MemberCaller<Doom3Group, &Doom3Group::originChanged> OriginChangedCaller;

	void rotationChanged();
	typedef MemberCaller<Doom3Group, &Doom3Group::rotationChanged> RotationChangedCaller;

	void skinChanged();
	typedef MemberCaller<Doom3Group, &Doom3Group::skinChanged> SkinChangedCaller;
};

} // namespace entity

#endif /*DOOM3GROUP_H_*/
