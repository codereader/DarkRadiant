#include "RadiantTest.h"

#include "ifilesystem.h"
#include "os/path.h"
#include "os/file.h"

namespace test
{

using VfsTest = RadiantTest;

TEST_F(VfsTest, FileSystemModule)
{
    // Confirm its module properties
    
    EXPECT_EQ(GlobalFileSystem().getName(), "VirtualFileSystem");
    EXPECT_TRUE(GlobalFileSystem().getDependencies().empty());
}

TEST_F(VfsTest, FilePrerequisites)
{
    // Check presence of some files
    EXPECT_EQ(GlobalFileSystem().getFileCount("nothere"), 0);
    EXPECT_EQ(GlobalFileSystem().getFileCount("materials/example.mtr"), 1);
    EXPECT_EQ(GlobalFileSystem().getFileCount("models/darkmod/test/unit_cube.ase"), 1);
    EXPECT_EQ(GlobalFileSystem().getFileCount("models/darkmod/test/unit_cube_blah.ase"), 0);
    EXPECT_EQ(GlobalFileSystem().getFileCount("models/darkmod/test/unit_cube.lwo"), 1);
}

TEST_F(VfsTest, VisitEntireTree)
{
    // Use a visitor to walk the tree
    std::map<std::string, vfs::FileInfo> foundFiles;
    GlobalFileSystem().forEachFile(
        "", "*",
        [&](const vfs::FileInfo& fi) { foundFiles.emplace(fi.name, fi); },
        0
    );
    EXPECT_EQ(foundFiles.count("dummy"), 0);
    EXPECT_EQ(foundFiles.count("materials/example.mtr"), 1);
    EXPECT_EQ(foundFiles.count("models/darkmod/test/unit_cube.ase"), 1);
}

TEST_F(VfsTest, GetArchiveFileInfo)
{
    // Use a visitor to walk the tree
    std::map<std::string, vfs::FileInfo> foundFiles;
    GlobalFileSystem().forEachFile(
        "", "*",
        [&](const vfs::FileInfo& fi) { foundFiles.emplace(fi.name, fi); },
        0
    );

    // Inspect a physical file that is in the test resources
    std::string physicalFile = "materials/example.mtr";
    std::string fileInPak = "materials/tdm_bloom_afx.mtr"; // this is in tdm_example_mtrs.pk4

    EXPECT_EQ(foundFiles.count(physicalFile), 1); // physical file
    EXPECT_EQ(foundFiles.count(fileInPak), 1); // file in pk4
    
    // Get the file size of example.mtr
    fs::path physicalFilePath = _context.getTestResourcePath();
    physicalFilePath /= physicalFile;

    EXPECT_EQ(foundFiles.find(physicalFile)->second.getSize(), os::getFileSize(physicalFilePath.string()));
    EXPECT_EQ(foundFiles.find(physicalFile)->second.getIsPhysicalFile(), true);

    EXPECT_EQ(foundFiles.find(fileInPak)->second.getSize(), 1096); // that file should have 1096 bytes
    EXPECT_EQ(foundFiles.find(fileInPak)->second.getIsPhysicalFile(), false);
}

TEST_F(VfsTest, VisitMaterialsFolderOnly)
{
    // Visit files only under materials/
    typedef std::map<std::string, vfs::FileInfo> FileInfos;
    FileInfos mtrFiles;
    GlobalFileSystem().forEachFile(
        "materials/", "mtr",
        [&](const vfs::FileInfo& fi) { mtrFiles.insert(std::make_pair(fi.name, fi)); },
        0
    );
    EXPECT_TRUE(!mtrFiles.empty());
}

TEST_F(VfsTest, LeafNamesVsFullPath)
{
    // Visit files only under materials/
    typedef std::map<std::string, vfs::FileInfo> FileInfos;
    FileInfos mtrFiles;
    GlobalFileSystem().forEachFile(
        "materials/", "mtr",
        [&](const vfs::FileInfo& fi) { mtrFiles.insert(std::make_pair(fi.name, fi)); },
        0
    );
    
    // When giving a topdir to visit, the returned file names should be
    // relative to that directory.
    EXPECT_EQ(mtrFiles.count("materials/example.mtr"), 0);
    EXPECT_EQ(mtrFiles.count("example.mtr"), 1);
    EXPECT_EQ(mtrFiles.count("materials/tdm_ai_nobles.mtr"), 0);
    EXPECT_EQ(mtrFiles.count("tdm_ai_nobles.mtr"), 1);

    // But we can reconstruct the original path using the FileInfo::fullPath
    // method.
    EXPECT_EQ(mtrFiles.find("example.mtr")->second.fullPath(), "materials/example.mtr");
    EXPECT_EQ(mtrFiles.find("tdm_ai_nobles.mtr")->second.fullPath(), "materials/tdm_ai_nobles.mtr");
}

TEST_F(VfsTest, forEachFileTrailingSlashInsensitive)
{
    // forEachFile() should work the same regardless of whether we have a
    // trailing slash on the base directory name
    typedef std::map<std::string, vfs::FileInfo> FileInfos;

    FileInfos mtrFiles;
    GlobalFileSystem().forEachFile(
        "materials/", "mtr",
        [&](const vfs::FileInfo& fi) { mtrFiles.insert(std::make_pair(fi.name, fi)); },
        0
    );

    FileInfos withoutSlash;
    GlobalFileSystem().forEachFile(
        "materials", "mtr",
        [&](const vfs::FileInfo& fi) { withoutSlash.insert(std::make_pair(fi.name, fi)); },
        0
    );

    EXPECT_EQ(withoutSlash.size(), mtrFiles.size());
    EXPECT_TRUE(std::equal(withoutSlash.begin(), withoutSlash.end(), mtrFiles.begin()));
}

TEST_F(VfsTest, assetsLstFileHandling)
{
    // Visit models dir and store visibility information
    std::map<std::string, vfs::Visibility> fileVis;
    GlobalFileSystem().forEachFile(
        "models/", "*", [&](const vfs::FileInfo& fi) { fileVis[fi.name] = fi.visibility; },
        0
    );

    EXPECT_TRUE(!fileVis.empty());

    EXPECT_EQ(fileVis.count("darkmod/test/unit_cube.ase"), 1);
    EXPECT_EQ(fileVis["darkmod/test/unit_cube.ase"], vfs::Visibility::HIDDEN);
    EXPECT_EQ(fileVis["darkmod/test/unit_cube.lwo"], vfs::Visibility::NORMAL);

    // The assets.lst should be converted into visibility information, but NOT
    // returned as an actual file to the calling code.
    EXPECT_EQ(fileVis.count("assets.lst"), 0);
}

TEST_F(VfsTest, openArchiveInAbsolutePath)
{
    fs::path pk4Path = _context.getTestResourcePath();
    pk4Path /= "tdm_example_mtrs.pk4";

    auto archive = GlobalFileSystem().openArchiveInAbsolutePath(pk4Path.string());

    EXPECT_TRUE(archive) << "Could not open " << pk4Path.string();

    // Check file existence
    ASSERT_TRUE(archive->containsFile("materials/tdm_bloom_afx.mtr"));
    
    // Check file read access
    auto file = archive->openTextFile("materials/tdm_bloom_afx.mtr");

    std::istream fileStream(&(file->getInputStream()));
    std::string contents(std::istreambuf_iterator<char>(fileStream), {});
    ASSERT_NE(contents.find("textures/AFX/AFXmodulate"), std::string::npos);
}

TEST_F(VfsTest, VisitEachFileInArchive)
{
    fs::path pk4Path = _context.getTestResourcePath();
    pk4Path /= "tdm_example_mtrs.pk4";
    
    // Use a visitor to walk the tree
    std::set<std::string> foundFiles;
    GlobalFileSystem().forEachFileInArchive(
        pk4Path.string(), "*",
        [&](const vfs::FileInfo& fi) { foundFiles.insert(fi.name); },
        0
    );

    EXPECT_EQ(foundFiles.count("materials/tdm_ai_monsters_spiders.mtr"), 1);
    EXPECT_EQ(foundFiles.count("materials/tdm_ai_nobles.mtr"), 1);
    EXPECT_EQ(foundFiles.count("materials/tdm_bloom_afx.mtr"), 1);
}

TEST_F(VfsTest, VisitEachFileInAbsolutePath)
{
    // Use a visitor to walk the tree
    std::set<std::string> foundFiles;
    GlobalFileSystem().forEachFileInAbsolutePath(
        _context.getTestResourcePath(), "*",
        [&](const vfs::FileInfo& fi) { foundFiles.insert(fi.name); },
        0
    );

    EXPECT_EQ(foundFiles.count("tdm_example_mtrs.pk4"), 1);
    EXPECT_EQ(foundFiles.count("models/moss_patch.ase"), 1);
    EXPECT_EQ(foundFiles.count("materials/___NONEXISTENTFILE.mtr"), 0);
}

}
