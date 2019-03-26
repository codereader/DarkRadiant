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

void parseShadersFromPath(vfs::Doom3FileSystem& fs, const std::string& path,
                          MockShaderLibrary& library)
{
    static const char* MATERIALS_EXT = "mtr";

    ShaderFileLoader<MockShaderLibrary> loader(fs, library);

    // Walk the filesystem and load .mtr files
    fs.forEachFile(
        path, MATERIALS_EXT,
        [&](const vfs::FileInfo& fileInfo) { loader.addFile(fileInfo); },
        0
    );

    // Instruct the loader to parse MTR files and create ShaderDefinitions
    loader.parseFiles();
}

BOOST_FIXTURE_TEST_CASE(loadShaderFiles, VFSFixture)
{
    static const char* MATERIALS_PATH = "materials/";

    MockShaderLibrary library;
    parseShadersFromPath(fs, MATERIALS_PATH, library);

    // We should now see our example material definitions in the ShaderLibrary
    auto& defs = library.shaderDefs;
    BOOST_TEST(defs.size() > 0);
    BOOST_TEST(defs.count("textures/orbweaver/drain_grille") == 1);
    BOOST_TEST(defs.count("models/md5/chars/nobles/noblewoman/noblebottom") == 1);
    BOOST_TEST(defs.count("tdm_spider_black") == 1);

    // Parsing should work both with or without a trailing slash on the
    // directory name
    MockShaderLibrary libNoSlash;
    parseShadersFromPath(fs, "materials", libNoSlash);
    BOOST_TEST(libNoSlash.shaderDefs.size() == defs.size());
    for (auto i1 = defs.begin(), i2 = libNoSlash.shaderDefs.begin();
         i1 != defs.end() && i2 != libNoSlash.shaderDefs.end();
         ++i1, ++i2)
    {
        BOOST_TEST(i1->first == i2->first);
    }
}
