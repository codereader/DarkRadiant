#pragma once

#include "irenderview.h"
#include "NopVolumeTest.h"
#include "math/Frustum.h"
#include "math/Viewer.h"

namespace render
{

class NopRenderView :
    public IRenderView
{
private:
    NopVolumeTest _volumeTest;
    Frustum _frustum;
    Viewer _viewer;

public:
    bool TestPoint(const Vector3& point) const { return _volumeTest.TestPoint(point); }
    bool TestLine(const Segment& segment) const { return _volumeTest.TestLine(segment); }
    bool TestPlane(const Plane3& plane) const { return _volumeTest.TestPlane(plane); }
    bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const { return _volumeTest.TestPlane(plane, localToWorld); }

    VolumeIntersectionValue TestAABB(const AABB& aabb) const
    {
        return _volumeTest.TestAABB(aabb);
    }

    VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const
    { 
        return _volumeTest.TestAABB(aabb, localToWorld);
    }

    bool fill() const { return _volumeTest.fill(); }

    const Matrix4& GetViewProjection() const { return _volumeTest.GetViewProjection(); }
    const Matrix4& GetViewport() const { return _volumeTest.GetViewport(); }
    const Matrix4& GetProjection() const { return _volumeTest.GetProjection(); }
    const Matrix4& GetModelview() const { return _volumeTest.GetModelview(); }

    void construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height) override
    {
        _volumeTest.setProjection(projection);
        _volumeTest.setModelView(modelview);

        _frustum = Frustum::createFromViewproj(_volumeTest.GetViewProjection());
        _viewer = Viewer::createFromViewProjection(_volumeTest.GetViewProjection());
    }

    Vector3 getViewer() const override
    {
        return _viewer.getVector3();
    }

    void setViewer(const Vector3& viewer)
    {
        _viewer = Vector4(viewer, 1.0);
    }

    const Frustum& getFrustum() const override
    {
        return _frustum;
    }

    std::string getCullStats() const override
    {
        return {};
    }
};

}
