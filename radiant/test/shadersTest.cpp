#define BOOST_TEST_MODULE shadersTest
#include <boost/test/included/unit_test.hpp>

#include "VFSFixture.h"

#include "radiant/shaders/ShaderFileLoader.h"
#include "radiant/shaders/textures/GLTextureManager.h"

namespace shaders
{

// Provide a local implementation of GetTextureManager since the application
// version calls the module registry.
GLTextureManager& GetTextureManager()
{
    static GLTextureManager manager;
    return manager;
}

}

using namespace shaders;

// Replacement for ShaderLibrary used in tests
struct MockShaderLibrary
{
    bool addTableDefinition(const TableDefinitionPtr& def)
    { return true; }

    bool addDefinition(const std::string& name, const ShaderDefinition& def)
    { return true; }
};

BOOST_FIXTURE_TEST_CASE(loaderShaderFiles, VFSFixture)
{
    MockShaderLibrary library;
    shaders::ShaderFileLoader<MockShaderLibrary> loader("materials", library);

    loader.parseFiles();
}
