#pragma once

#include "iobjectrenderer.h"

namespace test
{

// Dummy Object Renderer implementation
class TestObjectRenderer :
    public render::IObjectRenderer
{
public:
    void initAttributePointers() override
    {}

    void submitObject(render::IRenderableObject& object) override
    {}

    void submitGeometry(render::IGeometryStore::Slot slot, GLenum primitiveMode) override
    {}

    void submitInstancedGeometry(render::IGeometryStore::Slot slot, int numInstances, GLenum primitiveMode) override
    {}

    void submitGeometry(const std::set<render::IGeometryStore::Slot>& slots, GLenum primitiveMode) override
    {}

    void submitGeometry(const std::vector<render::IGeometryStore::Slot>& slots, GLenum primitiveMode) override
    {}

    void submitInstancedGeometry(const std::vector<render::IGeometryStore::Slot>& slots, int numInstances, GLenum primitiveMode) override
    {}

    void submitGeometryWithCustomIndices(render::IGeometryStore::Slot slot, GLenum primitiveMode,
        const std::vector<unsigned int>& indices) override
    {}
};

}
