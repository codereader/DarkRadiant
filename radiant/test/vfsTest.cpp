#define BOOST_TEST_MODULE vfsTest
#include <boost/test/included/unit_test.hpp>

#include "radiant/vfs/Doom3FileSystem.h"

BOOST_AUTO_TEST_CASE(constructFileSystemModule)
{
    // Construct a filesystem module
    vfs::Doom3FileSystem fs;

    // Confirm its module properties
    BOOST_TEST(fs.getName() == "VirtualFileSystem");
    BOOST_TEST(fs.getDependencies().empty());

    // The count for any file should be 0
    BOOST_TEST(fs.getFileCount("anything") == 0);
    BOOST_TEST(fs.getFileCount("*") == 0);
    BOOST_TEST(fs.getFileCount("") == 0);
}

BOOST_AUTO_TEST_CASE(readFilesFromVFS)
{
    GlobalOutputStream().setStream(std::cout);

    vfs::VirtualFileSystem::ExtensionSet exts;
    exts.insert("pk4");

    vfs::SearchPaths paths;
    paths.insertIfNotExists(std::string(getenv("srcdir")) + "/test/data/vfs_root");

    vfs::Doom3FileSystem fs;
    fs.initialise(paths, exts);

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

