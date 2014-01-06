#pragma once

#include "editable.h"

#include "entitylib.h"
#include "generic/callback.h"
#include "pivot.h"

#include "../OriginKey.h"
#include "../rotation.h"
#include "../angle.h"
#include "../ModelKey.h"
#include "../NameKey.h"
#include "../Doom3Entity.h"
#include "../KeyObserverDelegate.h"
#include "transformlib.h"

namespace entity {

class EclassModelNode;

class EclassModel :
	public Snappable
{
private:
	EclassModelNode& _owner;

	Doom3Entity& m_entity;

	OriginKey m_originKey;
	Vector3 m_origin;
	AngleKey m_angleKey;
	float m_angle;
	RotationKey m_rotationKey;
	Float9 m_rotation;

	RenderablePivot m_renderOrigin;

	Callback m_transformChanged;

	KeyObserverDelegate _rotationObserver;
	KeyObserverDelegate _angleObserver;

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
	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void snapto(float snap);

	void revertTransform();
	void freezeTransform();

public:
	void construct();
	void destroy();

	void updateTransform();

	void originChanged();

	void angleChanged();

	void rotationChanged();
};

} // namespace entity
