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
    // Shaders found
    std::map<std::string, ShaderDefinition> shaderDefs;

    // Required methods for ShaderFileLoader
    bool addTableDefinition(const TableDefinitionPtr& def)
    { return true; }

    bool addDefinition(const std::string& name, const ShaderDefinition& def)
    {
        shaderDefs.insert(std::make_pair(name, def));
        return true;
    }
};

BOOST_FIXTURE_TEST_CASE(loaderShaderFiles, VFSFixture)
{
    static const char* MATERIALS_PATH = "materials/";
    static const char* MATERIALS_EXT = "mtr";

    MockShaderLibrary library;
    shaders::ShaderFileLoader<MockShaderLibrary> loader(
        fs, MATERIALS_PATH, library
    );

    // Walk the filesystem and load .mtr files
    fs.forEachFile(
        MATERIALS_PATH, MATERIALS_EXT,
        [&](const vfs::FileInfo& fileInfo) { loader.addFile(fileInfo); },
        0
    );

    // Instruct the loader to parse MTR files and create ShaderDefinitions
    loader.parseFiles();

    // We should now see our example material definitions in the ShaderLibrary
    auto& defs = library.shaderDefs;
    BOOST_TEST(defs.size() > 0);
    BOOST_TEST(defs.count("textures/orbweaver/drain_grille") == 1);
    BOOST_TEST(defs.count("models/md5/chars/nobles/noblewoman/noblebottom") == 1);
    BOOST_TEST(defs.count("tdm_spider_black") == 1);
}
