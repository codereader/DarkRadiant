#define BOOST_TEST_MODULE vfsTest
#include <boost/test/included/unit_test.hpp>

#include "VFSFixture.h"

BOOST_FIXTURE_TEST_CASE(constructFileSystemModule, VFSFixture)
{
    // Confirm its module properties
    BOOST_TEST(fs.getName() == "VirtualFileSystem");
    BOOST_TEST(fs.getDependencies().empty());
}

BOOST_FIXTURE_TEST_CASE(findFilesInVFS, VFSFixture)
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

    // Visit files only under materials/
    typedef std::map<std::string, vfs::FileInfo> FileInfos;
    FileInfos mtrFiles;
    fs.forEachFile(
        "materials/", "mtr",
        [&](const vfs::FileInfo& fi) { mtrFiles.insert(std::make_pair(fi.name, fi)); },
        0
    );
    BOOST_TEST(!mtrFiles.empty());

    // When giving a topdir to visit, the returned file names should be
    // relative to that directory.
    BOOST_TEST(mtrFiles.count("materials/example.mtr") == 0);
    BOOST_TEST(mtrFiles.count("example.mtr") == 1);
    BOOST_TEST(mtrFiles.count("materials/tdm_ai_nobles.mtr") == 0);
    BOOST_TEST(mtrFiles.count("tdm_ai_nobles.mtr") == 1);

    // But we can reconstruct the original path using the FileInfo::fullPath
    // method.
    BOOST_TEST(mtrFiles.find("example.mtr")->second.fullPath()
               == "materials/example.mtr");
    BOOST_TEST(mtrFiles.find("tdm_ai_nobles.mtr")->second.fullPath()
               == "materials/tdm_ai_nobles.mtr");

    // forEachFile() should work the same regardless of whether we have a
    // trailing slash on the base directory name
    FileInfos withoutSlash;
    fs.forEachFile(
        "materials", "mtr",
        [&](const vfs::FileInfo& fi)
        {
            withoutSlash.insert(std::make_pair(fi.name, fi));
        },
        0
    );
    BOOST_TEST(withoutSlash.size() == mtrFiles.size());
    BOOST_TEST(std::equal(withoutSlash.begin(), withoutSlash.end(), mtrFiles.begin()));
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

    // The assets.lst should be converted into visibility information, but NOT
    // returned as an actual file to the calling code.
    BOOST_TEST(fileVis.count("assets.lst") == 0);
}
