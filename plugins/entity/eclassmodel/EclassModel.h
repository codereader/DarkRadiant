#ifndef ECLASSMODEL_H_
#define ECLASSMODEL_H_

#include "editable.h"

#include "entitylib.h"
#include "scenelib.h"
#include "generic/callback.h"
#include "pivot.h"

#include "../keyobservers.h"
#include "../origin.h"
#include "../rotation.h"
#include "../angle.h"
#include "../ModelKey.h"
#include "../namedentity.h"
#include "../SkinChangedWalker.h"
#include "../Doom3Entity.h"
#include "../OptionalRenderedName.h"

namespace entity {

class EclassModelNode;

class EclassModel :
	public Snappable,
    public OptionalRenderedName
{
	EclassModelNode& _owner;

	MatrixTransform m_transform;
	Doom3Entity& m_entity;
	KeyObserverMap m_keyObservers;

	OriginKey m_originKey;
	Vector3 m_origin;
	AngleKey m_angleKey;
	float m_angle;
	RotationKey m_rotationKey;
	Float9 m_rotation;
	ModelKey m_model;

	NamedEntity m_named;
	//NamespaceManager m_nameKeys;
	RenderablePivot m_renderOrigin;
	RenderableNamedEntity m_renderName;

	Callback m_transformChanged;
	Callback m_evaluateTransform;
	
	InstanceCounter m_instanceCounter;
public:
	EclassModel(IEntityClassPtr eclass,
				EclassModelNode& owner,
				const Callback& transformChanged, 
				const Callback& evaluateTransform);
	
	// Copy Constructor
	EclassModel(const EclassModel& other,
				EclassModelNode& owner, 
				const Callback& transformChanged, 
				const Callback& evaluateTransform);

	virtual ~EclassModel();

	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	// Adds the keyobserver to the KeyObserverMap
	void addKeyObserver(const std::string& key, const KeyObserver& observer);
	void removeKeyObserver(const std::string& key, const KeyObserver& observer);

	Doom3Entity& getEntity();
	const Doom3Entity& getEntity() const;

	//Namespaced& getNamespaced();
	NamedEntity& getNameable();
	const NamedEntity& getNameable() const;
	TransformNode& getTransformNode();
	const TransformNode& getTransformNode() const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();
	void transformChanged();
	typedef MemberCaller<EclassModel, &EclassModel::transformChanged> TransformChangedCaller;	

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
