#pragma once

#include "../VertexInstance.h"
#include "../EntitySettings.h"
#include "render/RenderableGeometry.h"

namespace entity
{

class RenderableVertex :
    public render::RenderableGeometry
{
private:
    const VertexInstance& _instance;
    const Matrix4& _localToWorld;

    bool _needsUpdate;

public:
    RenderableVertex(const VertexInstance& instance, const Matrix4& localToWorld) :
        _instance(instance),
        _localToWorld(localToWorld),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<ArbitraryMeshVertex> vertices;
        static std::vector<unsigned int> Indices = { 0 };

        auto colour = entity::EntitySettings::InstancePtr()->getLightVertexColour(
            _instance.isSelected() ? LightEditVertexType::Selected : LightEditVertexType::Deselected
        );

        vertices.push_back(ArbitraryMeshVertex(_localToWorld * _instance.getVertex(), { 0,0,0 }, { 0,0 }, colour));

        RenderableGeometry::updateGeometry(render::GeometryType::Points, vertices, Indices);
    }
};

}
