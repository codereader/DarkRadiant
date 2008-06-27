#ifndef SPEAKER_H_
#define SPEAKER_H_

#include "ieclass.h"
#include "Bounded.h"
#include "cullable.h"
#include "editable.h"

#include "math/Vector3.h"
#include "entitylib.h"
#include "generic/callback.h"

#include "../origin.h"
#include "../angle.h"
#include "../namedentity.h"
#include "../keyobservers.h"
#include "../Doom3Entity.h"
#include "../OptionalRenderedName.h"

#include "SpeakerRenderables.h"

namespace entity {

class SpeakerNode;

class Speaker :
	public Cullable,
	public Bounded,
	public Snappable,
    public OptionalRenderedName
{
	Doom3Entity& m_entity;
	KeyObserverMap m_keyObservers;
	MatrixTransform m_transform;

	OriginKey m_originKey;
	Vector3 m_origin;
	AngleKey m_angleKey;
	float m_angle;

	NamedEntity m_named;
	//NamespaceManager m_nameKeys;

	entity::RenderSpeakerRadii m_speakerRadii;
	SoundRadii m_stdVal;
	bool m_useSpeakerRadii;
	bool m_minIsSet;
	bool m_maxIsSet;

	AABB m_aabb_local;

	// the AABB that determines the rendering area
	AABB m_aabb_border;
	Ray m_ray;

	RenderableSolidAABB m_aabb_solid;
	RenderableWireframeAABB m_aabb_wire;
	RenderableNamedEntity m_renderName;

	Callback m_transformChanged;
	Callback m_boundsChanged;
	Callback m_evaluateTransform;
public:
	// Constructor
	Speaker(IEntityClassPtr eclass, 
		entity::SpeakerNode& node, 
				  const Callback& transformChanged, 
				  const Callback& boundsChanged,
				  const Callback& evaluateTransform);
	
	// Copy constructor
	Speaker(const Speaker& other, 
				  SpeakerNode& node, 
				  const Callback& transformChanged, 
				  const Callback& boundsChanged,
				  const Callback& evaluateTransform);

	InstanceCounter m_instanceCounter;
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	Doom3Entity& getEntity();
	const Doom3Entity& getEntity() const;

	//Namespaced& getNamespaced();
	NamedEntity& getNameable();
	const NamedEntity& getNameable() const;
	TransformNode& getTransformNode();
	const TransformNode& getTransformNode() const;

	const AABB& localAABB() const;

	VolumeIntersectionValue intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();
	void transformChanged();
	typedef MemberCaller<Speaker, &Speaker::transformChanged> TransformChangedCaller;

public:

	void construct();

	// updates the AABB according to the SpeakerRadii
	void updateAABB();

	void updateTransform();
	typedef MemberCaller<Speaker, &Speaker::updateTransform> UpdateTransformCaller;
	
	void originChanged();
	typedef MemberCaller<Speaker, &Speaker::originChanged> OriginChangedCaller;

	void sShaderChanged(const std::string& value);
	typedef MemberCaller1<Speaker, const std::string&, &Speaker::sShaderChanged> sShaderChangedCaller;

	void sMinChanged(const std::string& value);
	typedef MemberCaller1<Speaker, const std::string&, &Speaker::sMinChanged> sMinChangedCaller;

	void sMaxChanged(const std::string& value);
	typedef MemberCaller1<Speaker, const std::string&, &Speaker::sMaxChanged> sMaxChangedCaller;

	void angleChanged();
	typedef MemberCaller<Speaker, &Speaker::angleChanged> AngleChangedCaller;
};

} // namespace entity

#endif /*SPEAKER_H_*/
