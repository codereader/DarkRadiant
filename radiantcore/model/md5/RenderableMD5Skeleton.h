#pragma once

#include "irender.h"
#include "MD5Skeleton.h"
#include "render/RenderableGeometry.h"

namespace md5
{

class RenderableMD5Skeleton :
    public render::RenderableGeometry
{
private:
	const MD5Skeleton& _skeleton;
    const Matrix4& _localToWorld;

    bool _updateNeeded;

public:
	RenderableMD5Skeleton(const MD5Skeleton& skeleton, const Matrix4& localToWorld) :
		_skeleton(skeleton),
        _localToWorld(localToWorld),
        _updateNeeded(true)
	{}

    void queueUpdate()
    {
        _updateNeeded = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_updateNeeded) return;

        _updateNeeded = false;

        if (_skeleton.size() == 0)
        {
            clear();
            return;
        }

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        auto numJoints = _skeleton.size();

        for (auto i = 0; i < numJoints; ++i)
        {
            const auto& bone = _skeleton.getKey(i);
            const auto& joint = _skeleton.getJoint(i);

            if (joint.parentId != -1)
            {
                const auto& parentBone = _skeleton.getKey(joint.parentId);

                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(toVertex(parentBone.origin));

                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(toVertex(bone.origin));
            }
            else
            {
                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(toVertex({ 0, 0, 0 }));

                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(toVertex(bone.origin));
            }
        }

        for (auto i = 0; i < numJoints; ++i)
        {
            const auto& joint = _skeleton.getKey(i);

            Vector3 x(2, 0, 0);
            Vector3 y(0, 2, 0);
            Vector3 z(0, 0, 2);

            x = joint.orientation.transformPoint(x);
            y = joint.orientation.transformPoint(y);
            z = joint.orientation.transformPoint(z);

            const auto& origin = joint.origin;

            // x axis
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin, { 1, 0, 0, 1 }));
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin + x, { 1, 0, 0, 1 }));

            // y axis
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin, { 0, 1, 0, 1 }));
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin + y, { 0, 1, 0, 1 }));

            // z axis
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin, { 0, 0, 1, 1 }));
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(toVertex(origin + z, { 0, 0, 1, 1 }));
        }

        // Move the skeleton to world space
        for (auto& v : vertices)
        {
            v.vertex = math::transformVector3f(_localToWorld, v.vertex);
        }

        updateGeometryWithData(render::GeometryType::Lines, vertices, indices);
    }

private:
    render::RenderVertex toVertex(const Vector3& vertex, const Vector4& colour = { 1, 1, 1, 1 })
    {
        return { vertex, {0, 0, 0}, {1, 0}, colour };
    }
};

} // namespace
