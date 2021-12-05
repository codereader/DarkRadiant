#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Line.h"
#include "math/Frustum.h"
#include "entitylib.h"
#include "igl.h"
#include "render/RenderableGeometry.h"
#include "../OriginKey.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]);

namespace entity
{

class LightNode;

class RenderLightRadiiBox : public OpenGLRenderable {
	const Vector3& m_origin;
public:
	mutable Vector3 m_points[8];
	static ShaderPtr m_state;

	RenderLightRadiiBox(const Vector3& origin) : m_origin(origin) {}

	void render(const RenderInfo& info) const;
}; // class RenderLightRadiiBox

class RenderLightProjection : public OpenGLRenderable {
	const Vector3& _origin;
	const Vector3& _start;
	const Frustum& _frustum;
public:
	RenderLightProjection(const Vector3& origin, const Vector3& start, const Frustum& frustum);

	// greebo: Renders the light cone of a projected light (may also be a frustum, when light_start / light_end are set)
	void render(const RenderInfo& info) const;
}; // class RenderLightProjection

// The small diamond representing at the light's origin
// This is using the Triangle geometry type such that we can see 
// the half-transparent (red) overlay when the light is selected
class RenderableLightOctagon :
    public render::RenderableGeometry
{
private:
    const scene::INode& _owner;
    bool _needsUpdate;

public:
    RenderableLightOctagon(const scene::INode& owner) :
        _owner(owner),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override;
};

// The wireframe showing the light volume of the light
// which is either a box (point light) or a frustum/cone (projected)
class RenderableLightVolume :
    public render::RenderableGeometry
{
private:
    const LightNode& _light;
    bool _needsUpdate;

public:
    RenderableLightVolume(const LightNode& light) :
        _light(light),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override;
};

} // namespace entity
