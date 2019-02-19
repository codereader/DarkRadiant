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

