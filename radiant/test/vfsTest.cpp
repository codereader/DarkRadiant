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
    vfs::VirtualFileSystem::ExtensionSet exts;
    exts.insert("pk4");

    vfs::SearchPaths paths;
    paths.insertIfNotExists(std::string(getenv("srcdir")) + "/test/data/vfs_root");

    vfs::Doom3FileSystem fs;
    fs.initialise(paths, exts);

    // Check presence of some files
    BOOST_TEST(fs.getFileCount("nothere") == 0);
}

