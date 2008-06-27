#ifndef DOOM3GROUP_H_
#define DOOM3GROUP_H_

#include "Bounded.h"
#include "editable.h"
#include "entitylib.h"
#include "pivot.h"

#include "../keyobservers.h"
#include "../ModelKey.h"
#include "../origin.h"
#include "../rotation.h"
#include "../namedentity.h"
#include "../SkinChangedWalker.h"
#include "../Doom3Entity.h"
#include "../curve/CurveCatmullRom.h"
#include "../curve/CurveNURBS.h"
#include "../OptionalRenderedName.h"
#include "scene/TraversableNodeSet.h"

namespace entity {

// Forward declaration
class Doom3GroupNode;

/**
 * An entity that contains brushes or patches, such as func_static.
 */
class Doom3Group 
: public Bounded, 
  public Snappable,
  public OptionalRenderedName
{
	KeyObserverMap m_keyObservers;
	
	Doom3GroupNode& _owner;
	Doom3Entity& _entity;

	MatrixTransform m_transform;

	ModelKey m_model;
	OriginKey m_originKey;
	Vector3 m_origin;
	
	// A separate origin for the renderable names and pivot points
	Vector3 m_nameOrigin;
	 
	RotationKey m_rotationKey;
	Float9 m_rotation;

	NamedEntity m_named;
	RenderablePivot m_renderOrigin;
	RenderableNamedEntity m_renderName;

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
	Doom3Group(IEntityClassPtr eclass, 
			   Doom3GroupNode& owner,
			   const Callback& transformChanged, 
			   const Callback& boundsChanged, 
			   const Callback& evaluateTransform);
	
	// Copy constructor
	Doom3Group(const Doom3Group& other, 
			   Doom3GroupNode& owner,
			   const Callback& transformChanged, 
			   const Callback& boundsChanged, 
			   const Callback& evaluateTransform);
	
	~Doom3Group();

	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	// Adds/removes the keyobserver to/from the KeyObserverMap
	void addKeyObserver(const std::string& key, const KeyObserver& observer);
	void removeKeyObserver(const std::string& key, const KeyObserver& observer);

	Doom3Entity& getEntity();
	const Doom3Entity& getEntity() const;

	scene::Traversable& getTraversable();
	const scene::Traversable& getTraversable() const;
	//Namespaced& getNamespaced();
	NamedEntity& getNameable();
	const NamedEntity& getNameable() const;
	TransformNode& getTransformNode();
	const TransformNode& getTransformNode() const;

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
	
	// Returns TRUE if this D3Group is a model
	bool isModel() const;

	void setTransformChanged(Callback& callback);

	// Attaches keyobservers, etc.
	void construct();

private:
	void destroy();

	void setIsModel(bool newValue);

	/** Determine if this Doom3Group is a model (func_static) or a
	 * brush-containing entity. If the "model" key is equal to the
	 * "name" key, then this is a brush-based entity, otherwise it is
	 * a model entity. The exception to this is for the "worldspawn"
	 * entity class, which is always a brush-based entity.
	 */
	void updateIsModel();

public:

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
};

} // namespace entity

#endif /*DOOM3GROUP_H_*/
