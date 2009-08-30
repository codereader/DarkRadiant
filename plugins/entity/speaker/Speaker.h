#ifndef SPEAKER_H_
#define SPEAKER_H_

#include "ieclass.h"
#include "Bounded.h"
#include "cullable.h"
#include "editable.h"

#include "math/Vector3.h"
#include "entitylib.h"
#include "transformlib.h"
#include "generic/callback.h"

#include "../origin.h"
#include "../Doom3Entity.h"
#include "../EntitySettings.h"

#include "SpeakerRenderables.h"

namespace entity {

class SpeakerNode;

class Speaker :
	public Cullable,
	public Bounded,
	public Snappable
{
	SpeakerNode& _owner;

	Doom3Entity& m_entity;

	OriginKey m_originKey;
	Vector3 m_origin;

	// The current speaker radii (min / max)
	SoundRadii _radii;
	// The "working set" which is used during resize operations
	SoundRadii _radiiTransformed;

	// The default radii as defined on the currently active sound shader
	SoundRadii _defaultRadii;

    // Renderable speaker radii
	RenderableSpeakerRadii _renderableRadii;

	bool m_useSpeakerRadii;
	bool m_minIsSet;
	bool m_maxIsSet;

	AABB m_aabb_local;

	// the AABB that determines the rendering area
	AABB m_aabb_border;

	RenderableSolidAABB m_aabb_solid;
	RenderableWireframeAABB m_aabb_wire;

	Callback m_transformChanged;
	Callback m_boundsChanged;

public:
	// Constructor
	Speaker(SpeakerNode& node,
			const Callback& transformChanged, 
			const Callback& boundsChanged);
	
	// Copy constructor
	Speaker(const Speaker& other, 
			SpeakerNode& node, 
			const Callback& transformChanged, 
			const Callback& boundsChanged);

	~Speaker();

	const AABB& localAABB() const;

	VolumeIntersectionValue intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

    // Render functions (invoked by SpeakerNode)
	void renderSolid(RenderableCollector& collector,
                     const VolumeTest& volume,
                     const Matrix4& localToWorld,
                     bool isSelected) const;
	void renderWireframe(RenderableCollector& collector,
                         const VolumeTest& volume,
                         const Matrix4& localToWorld,
                         bool isSelected) const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();

	// greebo: Modifies the speaker radii according to the passed bounding box
	// this is called during drag-resize operations
	void setRadiusFromAABB(const AABB& aabb);

public:

	void construct();
	void destroy();

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
};

} // namespace entity

#endif /*SPEAKER_H_*/
