#ifndef ECLASSMODEL_H_
#define ECLASSMODEL_H_

#include "editable.h"

#include "entitylib.h"
#include "scenelib.h"
#include "generic/callback.h"
#include "pivot.h"

#include "../origin.h"
#include "../rotation.h"
#include "../angle.h"
#include "../ModelKey.h"
#include "../NameKey.h"
#include "../SkinChangedWalker.h"
#include "../Doom3Entity.h"
#include "transformlib.h"

namespace entity {

class EclassModelNode;

class EclassModel :
	public Snappable
{
	EclassModelNode& _owner;

	Doom3Entity& m_entity;

	OriginKey m_originKey;
	Vector3 m_origin;
	AngleKey m_angleKey;
	float m_angle;
	RotationKey m_rotationKey;
	Float9 m_rotation;
	ModelKey m_model;

	RenderablePivot m_renderOrigin;

	Callback m_transformChanged;
	
public:
	EclassModel(EclassModelNode& owner,
				const Callback& transformChanged);
	
	// Copy Constructor
	EclassModel(const EclassModel& other,
				EclassModelNode& owner, 
				const Callback& transformChanged);

	~EclassModel();

	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();

	void testSelect(Selector& selector, SelectionTest& test);

public:
	void construct();
	void destroy();

	void updateTransform();
	typedef MemberCaller<EclassModel, &EclassModel::updateTransform> UpdateTransformCaller;

	void originChanged();
	typedef MemberCaller<EclassModel, &EclassModel::originChanged> OriginChangedCaller;
	
	void angleChanged();
	typedef MemberCaller<EclassModel, &EclassModel::angleChanged> AngleChangedCaller;
	
	void rotationChanged();
	typedef MemberCaller<EclassModel, &EclassModel::rotationChanged> RotationChangedCaller;
	
	void modelChanged(const std::string& value);
	typedef MemberCaller1<EclassModel, const std::string&, &EclassModel::modelChanged> ModelChangedCaller;
};

} // namespace entity

#endif /*ECLASSMODEL_H_*/
