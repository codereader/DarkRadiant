#pragma once

#include "Bounded.h"
#include "editable.h"
#include "entitylib.h"
#include "pivot.h"

#include "../ModelKey.h"
#include "../OriginKey.h"
#include "../rotation.h"
#include "../Doom3Entity.h"
#include "../curve/CurveCatmullRom.h"
#include "../curve/CurveNURBS.h"
#include "../KeyObserverDelegate.h"
#include "scene/TraversableNodeSet.h"
#include "transformlib.h"

namespace entity {

// Forward declaration
class Doom3GroupNode;

/**
 * An entity that contains brushes or patches, such as func_static.
 */
class Doom3Group
: public Bounded,
  public Snappable
{
	Doom3GroupNode& _owner;
	Doom3Entity& _entity;

	OriginKey m_originKey;
	Vector3 m_origin;

	// A separate origin for the renderable pivot points
	Vector3 m_nameOrigin;

	RotationKey m_rotationKey;
	Float9 m_rotation;

	RenderablePivot m_renderOrigin;

	mutable AABB m_curveBounds;

	// The value of the "name" key for this Doom3Group.
	std::string m_name;

	// The value of the "model" key for this Doom3Group.
	std::string m_modelKey;

	// Flag to indicate this Doom3Group is a model (i.e. does not contain
	// brushes).
	bool m_isModel;

	KeyObserverDelegate _rotationObserver;
	KeyObserverDelegate _angleObserver;
	KeyObserverDelegate _nameObserver;

public:
	CurveNURBS m_curveNURBS;
	std::size_t m_curveNURBSChanged;
	CurveCatmullRom m_curveCatmullRom;
	std::size_t m_curveCatmullRomChanged;

	/** greebo: The constructor takes the Node as argument
	 * as well as some callbacks for transformation and bounds changes.
	 */
	Doom3Group(Doom3GroupNode& owner,
			   const Callback& boundsChanged);

	// Copy constructor
	Doom3Group(const Doom3Group& other,
			   Doom3GroupNode& owner,
			   const Callback& boundsChanged);

	~Doom3Group();

	const AABB& localAABB() const;

	Vector3& getOrigin();

	// Curve-related methods
	void appendControlPoints(unsigned int numPoints);
	void convertCurveType();

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best);

	/** greebo: Translates this Doom3Group
	 *
	 * @translation: The translation vector
	 * @rotation: TRUE, if the translation was due to a rotation
	 * 			  (this inhibits the movement of the origin)
	 * @scale: TRUE if the translation is due to a pivoted scale
	 */
	void translate(const Vector3& translation, bool rotation = false, bool scale = false);
	void rotate(const Quaternion& rotation);
	void scale(const Vector3& scale);

	void snapto(float snap);

	void revertTransform();
	void freezeTransform();

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
	void modelChanged(const std::string& value);

	void updateTransform();

	void originChanged();

	void rotationChanged();
};

} // namespace entity
