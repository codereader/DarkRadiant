#define BOOST_TEST_MODULE vfsTest
#include <boost/test/included/unit_test.hpp>

#include "radiant/vfs/Doom3FileSystem.h"

struct VFSFixture
{
    // The Doom3FileSystem under test
    vfs::Doom3FileSystem fs;

    // Initialisation parameters for the Doom3FileSystem
    vfs::VirtualFileSystem::ExtensionSet pakExtensions;
    vfs::SearchPaths searchPaths;

    VFSFixture()
    {
        // Setup the output stream
        GlobalOutputStream().setStream(std::cout);

        // Configure search paths and extensions
        pakExtensions.insert("pk4");
        searchPaths.insertIfNotExists(
            std::string(getenv("srcdir")) + "/test/data/vfs_root"
        );

        // Initialise the VFS
        fs.initialise(searchPaths, pakExtensions);
    }
};

BOOST_FIXTURE_TEST_CASE(constructFileSystemModule, VFSFixture)
{
    // Confirm its module properties
    BOOST_TEST(fs.getName() == "VirtualFileSystem");
    BOOST_TEST(fs.getDependencies().empty());
}

BOOST_FIXTURE_TEST_CASE(readFilesFromVFS, VFSFixture)
{
    // Check presence of some files
    BOOST_TEST(fs.getFileCount("nothere") == 0);
    BOOST_TEST(fs.getFileCount("materials/example.mtr") == 1);
    BOOST_TEST(fs.getFileCount("models/darkmod/test/unit_cube.ase") == 1);
    BOOST_TEST(fs.getFileCount("models/darkmod/test/unit_cube_blah.ase") == 0);
    BOOST_TEST(fs.getFileCount("models/darkmod/test/unit_cube.lwo") == 1);

    // Use a visitor to walk the tree
    std::set<std::string> foundFiles;
    fs.forEachFile(
        "", "*",
        [&](const vfs::FileInfo& fi) { foundFiles.insert(fi.name); },
        0
    );
    BOOST_TEST(foundFiles.count("dummy") == 0);
    BOOST_TEST(foundFiles.count("materials/example.mtr") == 1);
    BOOST_TEST(foundFiles.count("models/darkmod/test/unit_cube.ase") == 1);
}

BOOST_FIXTURE_TEST_CASE(handleAssetsLst, VFSFixture)
{
    // Visit models dir and store visibility information
    std::map<std::string, vfs::Visibility> fileVis;
    fs.forEachFile(
        "models/", "*", [&](const vfs::FileInfo& fi) { fileVis[fi.name] = fi.visibility; },
        0
    );

    BOOST_TEST(!fileVis.empty());

    BOOST_TEST(fileVis.count("darkmod/test/unit_cube.ase") == 1);
    BOOST_TEST(fileVis["darkmod/test/unit_cube.ase"] == vfs::Visibility::HIDDEN);
    BOOST_TEST(fileVis["darkmod/test/unit_cube.lwo"] == vfs::Visibility::NORMAL);
}
