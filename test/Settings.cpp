#include "gtest/gtest.h"

#include <fstream>
#include "settings/MajorMinorVersion.h"
#include "settings/SettingsManager.h"
#include "module/ApplicationContextBase.h"
#include "os/dir.h"
#include "os/path.h"

namespace test
{

TEST(MajorMinorVersionTest, ParseFromString)
{
    settings::MajorMinorVersion v1("2.13.0");
    EXPECT_EQ(v1.getMajorVersion(), 2);
    EXPECT_EQ(v1.getMinorVersion(), 13);

    settings::MajorMinorVersion v2("2.14.0pre4");
    EXPECT_EQ(v2.getMajorVersion(), 2);
    EXPECT_EQ(v2.getMinorVersion(), 14);

    settings::MajorMinorVersion v3("2.15.0pre1_rev563abfa");
    EXPECT_EQ(v3.getMajorVersion(), 2);
    EXPECT_EQ(v3.getMinorVersion(), 15);

    settings::MajorMinorVersion v3a("2.15.4_rev563abfa");
    EXPECT_EQ(v3a.getMajorVersion(), 2);
    EXPECT_EQ(v3a.getMinorVersion(), 15);

    settings::MajorMinorVersion v4("3.14.5");
    EXPECT_EQ(v4.getMajorVersion(), 3);
    EXPECT_EQ(v4.getMinorVersion(), 14);

    settings::MajorMinorVersion v5("11.0.0");
    EXPECT_EQ(v5.getMajorVersion(), 11);
    EXPECT_EQ(v5.getMinorVersion(), 0);

    settings::MajorMinorVersion v6("11.0.1");
    EXPECT_EQ(v6.getMajorVersion(), 11);
    EXPECT_EQ(v6.getMinorVersion(), 0);

    settings::MajorMinorVersion v7("11.100.1");
    EXPECT_EQ(v7.getMajorVersion(), 11);
    EXPECT_EQ(v7.getMinorVersion(), 100);

    // Invalid expressions
    EXPECT_THROW(settings::MajorMinorVersion("11.a100.1"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("11"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("11..6"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("11.a"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("x.y"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("2_3_7"), std::invalid_argument);
    EXPECT_THROW(settings::MajorMinorVersion("10.8.9-"), std::invalid_argument);
}

TEST(MajorMinorVersionTest, LessThanOperator)
{
    settings::MajorMinorVersion v1("2.13.0");
    settings::MajorMinorVersion v2("2.14.0pre4");
    EXPECT_TRUE(v1 < v2) << "2.13.0 should be smaller than 2.14.0pre4";
    EXPECT_FALSE(v2 < v1) << "2.14.0pre4 should not be smaller than 2.13.0";

    settings::MajorMinorVersion v3("3.14.5");
    EXPECT_TRUE(v2 < v3) << "2.14.0pre4 should be smaller than 3.14.5";
    EXPECT_TRUE(v1 < v3) << "2.13.0 should be smaller than 3.14.5";
    EXPECT_FALSE(v3 < v1) << "3.14.5 should not be smaller than 2.13.0";
    EXPECT_FALSE(v3 < v2) << "3.14.5 should not be smaller than 2.14.0pre4";

    settings::MajorMinorVersion v5("11.0.0");
    EXPECT_TRUE(v1 < v5) << "2.13.0 should be smaller than 11.0.0";
    EXPECT_TRUE(v2 < v5) << "2.14.0pre4 should be smaller than 11.0.0";
    EXPECT_TRUE(v3 < v5) << "3.14.5 should be smaller than 11.0.0";
    EXPECT_FALSE(v5 < v1) << "11.0.0 should not be smaller than 2.13.0";
    EXPECT_FALSE(v5 < v2) << "11.0.0 should not be smaller than 2.14.0pre4";
    EXPECT_FALSE(v5 < v3) << "11.0.0 should not be smaller than 3.14.5";

    settings::MajorMinorVersion v6("11.0.1");
    EXPECT_FALSE(v5 < v6) << "11.0.0 should not be smaller than 11.0.1 (micro version is ignored)";
    EXPECT_FALSE(v6 < v5) << "11.0.1 should not be smaller than 11.0.0 (micro version is ignored)";

    settings::MajorMinorVersion v7("11.100.1");
    EXPECT_TRUE(v5 < v7) << "11.0.0 should be smaller than 11.100.1";
    EXPECT_TRUE(v6 < v7) << "11.0.1 should be smaller than 11.100.1";
    EXPECT_FALSE(v7 < v5) << "11.100.1 should not be smaller than 11.0.0";
    EXPECT_FALSE(v7 < v6) << "11.100.1 should not be smaller than 11.0.1";

    settings::MajorMinorVersion v8("11.10.3");
    settings::MajorMinorVersion v9("11.1.3");
    EXPECT_TRUE(v9 < v8) << "11.1.3 should be smaller than 11.10.3";
    EXPECT_FALSE(v8 < v9) << "11.10.3 should not be smaller than 11.1.3";
}

TEST(MajorMinorVersionTest, ToString)
{
    EXPECT_EQ(settings::MajorMinorVersion("2.13.0").toString(), "2.13");
    EXPECT_EQ(settings::MajorMinorVersion("2.14.0pre4").toString(), "2.14");
    EXPECT_EQ(settings::MajorMinorVersion("3.14.5").toString(), "3.14");
    EXPECT_EQ(settings::MajorMinorVersion("11.0.0").toString(), "11.0");
    EXPECT_EQ(settings::MajorMinorVersion("11.0.1").toString(), "11.0");
    EXPECT_EQ(settings::MajorMinorVersion("11.100.1").toString(), "11.100");
    EXPECT_EQ(settings::MajorMinorVersion("9.10.3").toString(), "9.10");
    EXPECT_EQ(settings::MajorMinorVersion("11.1.300").toString(), "11.1");
}

// Application Context providing a settings path in a custom folder in the temp path
class SettingsTestContext final :
    public radiant::ApplicationContextBase
{
private:
    std::string _settingsFolder;
    std::string _tempDataPath;

public:
    SettingsTestContext()
    {
        // Set up the temporary settings folder
        auto settingsFolder = os::getTemporaryPath() / "settings_tests";

        _settingsFolder = os::standardPathWithSlash(settingsFolder.string());

        os::removeDirectory(_settingsFolder);
        os::makeDirectory(_settingsFolder);

        auto tempDataFolder = os::getTemporaryPath() / "settings_temp_data";

        _tempDataPath = os::standardPathWithSlash(tempDataFolder.string());

        os::removeDirectory(_tempDataPath);
        os::makeDirectory(_tempDataPath);
    }

    ~SettingsTestContext()
    {
        if (!_settingsFolder.empty())
        {
            os::removeDirectory(_settingsFolder);
        }

        if (!_tempDataPath.empty())
        {
            os::removeDirectory(_tempDataPath);
        }
    }

    std::string getSettingsPath() const override
    {
        return _settingsFolder;
    }
};

namespace
{

void testSettingsPathCreation(const IApplicationContext& context, const std::string& versionString)
{
    settings::MajorMinorVersion version(versionString.empty() ? RADIANT_VERSION : versionString);

    auto expectedFolder = os::standardPathWithSlash(context.getSettingsPath() + version.toString());

    // Let's assume the folder doesn't exist yet
    EXPECT_FALSE(fs::is_directory(expectedFolder)) << "The output path " << expectedFolder << " already exists";

    // Set up a manager and check if it created the settings output folder
    auto manager = versionString.empty() ?
        std::make_unique<settings::SettingsManager>(context) :
        std::make_unique<settings::SettingsManager>(context, versionString);

    EXPECT_EQ(manager->getCurrentVersionSettingsFolder(), expectedFolder) << "Output folder is not what we expected";
    EXPECT_TRUE(fs::is_directory(expectedFolder)) << "Manager should have created the path " << expectedFolder;

    os::removeDirectory(expectedFolder);
}

}

// Checks that the output folder for a specific version is created correctly
TEST(SettingsManagerTest, getSpecificVersionSettingsFolder)
{
    SettingsTestContext context;

    testSettingsPathCreation(context, "2.15.0pre1");
    testSettingsPathCreation(context, "2.15.1");
    testSettingsPathCreation(context, "3.0.0");
    testSettingsPathCreation(context, "3.0.2");
    testSettingsPathCreation(context, "3.1.0");
    testSettingsPathCreation(context, "3.11.0");
}

// Checks that the output folder for the current RADIANT_VERSION is created correctly
TEST(SettingsManagerTest, getCurrentVersionSettingsFolder)
{
    // Test with the current radiant version
    SettingsTestContext context;
    testSettingsPathCreation(context, std::string());
}

// Creates the file in the given version folder, with the full path as file contents. Returns the full path.
inline std::string createSettingsFile(const IApplicationContext& context, const std::string& versionFolder, const std::string& filename)
{
    auto settingsFolder = os::standardPathWithSlash(context.getSettingsPath() + versionFolder);
    os::makeDirectory(settingsFolder);
    auto fullPath = settingsFolder + filename;

    std::ofstream stream(fullPath);

    stream << fullPath;
    stream.flush();
    stream.close();

    return fullPath;
}

inline std::string loadSettingsFile(const IApplicationContext& context, const std::string& applicationVersion, const std::string& relativeFilePath)
{
    settings::SettingsManager manager(context, applicationVersion);
    auto existingFile = manager.getExistingSettingsFile(relativeFilePath);

    if (existingFile.empty()) return {};

    std::ifstream file(existingFile);
    std::stringstream content;
    content << file.rdbuf();

    return content.str();
}

// Checks that the version/base settings folders are respecting the version sort order
TEST(SettingsManagerTest, SettingsFileVersionPrecedence)
{
    SettingsTestContext context;

    // User text is located in the base, 2.13 and 2.14
    auto userTextInBaseFolder = createSettingsFile(context, "", "user.txt");
    auto baseTxtInBaseFolder = createSettingsFile(context, "", "base.txt");
    auto userText21 = createSettingsFile(context, "2.1", "user.txt");
    auto userText213 = createSettingsFile(context, "2.13", "user.txt");
    auto userText215 = createSettingsFile(context, "2.15", "user.txt");
    auto customTxt211 = createSettingsFile(context, "2.11", "custom.txt");
    auto userText30 = createSettingsFile(context, "3.0", "user.txt");

    // Create an unrelated folder, to prove it's not crashing anything
    os::makeDirectory(context.getSettingsPath() + "unrelatedFolder/");

    // Version 2.15 should load the file from the 2.15 folder
    EXPECT_EQ(loadSettingsFile(context, "2.15.66", "user.txt"), userText215) << "2.15 should load user.txt from the 2.15 folder";

    // Version 2.14 should load the file from the 2.13 folder
    EXPECT_EQ(loadSettingsFile(context, "2.14.0", "user.txt"), userText213) << "2.14 should load user.txt from the 2.13 folder";

    // Version 2.13 should load the file from the 2.13 folder
    EXPECT_EQ(loadSettingsFile(context, "2.13.1", "user.txt"), userText213) << "2.13 should load user.txt from the 2.13 folder";

    // Version 2.12 doesn't have any version folder, should load from 2.1
    EXPECT_EQ(loadSettingsFile(context, "2.12.0", "user.txt"), userText21) << "2.12 should load user.txt from the 2.1 folder";

    // Version 2.0.4 should pick the base
    EXPECT_EQ(loadSettingsFile(context, "2.0.4", "user.txt"), userTextInBaseFolder) << "2.04 should load user.txt from the base folder";

    // custom.txt only exists in 2.11
    EXPECT_EQ(loadSettingsFile(context, "2.11.0", "custom.txt"), customTxt211) << "2.15 should load custom.txt from the 2.11 folder";
    EXPECT_EQ(loadSettingsFile(context, "2.15.2", "custom.txt"), customTxt211) << "2.15 should load custom.txt from the 2.11 folder";
    EXPECT_EQ(loadSettingsFile(context, "2.16.99", "custom.txt"), customTxt211) << "2.16 should load custom.txt from the 2.11 folder";
    EXPECT_EQ(loadSettingsFile(context, "3.0.0", "custom.txt"), customTxt211) << "3.0.0 should load custom.txt from the 2.11 folder";
    EXPECT_EQ(loadSettingsFile(context, "3.99.0", "custom.txt"), customTxt211) << "3.99.0 should load custom.txt from the 2.11 folder";

    // File only exists in base folder
    EXPECT_EQ(loadSettingsFile(context, "2.15.2", "base.txt"), baseTxtInBaseFolder) << "2.15 should load custom.txt from the base folder";
    EXPECT_EQ(loadSettingsFile(context, "2.16.99", "base.txt"), baseTxtInBaseFolder) << "2.16 should load custom.txt from the base folder";
    EXPECT_EQ(loadSettingsFile(context, "3.0.0", "base.txt"), baseTxtInBaseFolder) << "3.0.0 should load custom.txt from the base folder";
    EXPECT_EQ(loadSettingsFile(context, "3.99.0", "base.txt"), baseTxtInBaseFolder) << "3.99.0 should load custom.txt from the base folder";

    // Version 3.x should use 3.0
    EXPECT_EQ(loadSettingsFile(context, "3.0.0", "user.txt"), userText30) << "3.0.0 should load user.txt from the 3.0 folder";
    EXPECT_EQ(loadSettingsFile(context, "3.0.2", "user.txt"), userText30) << "3.0.2 should load user.txt from the 3.0 folder";
    EXPECT_EQ(loadSettingsFile(context, "3.0.1", "user.txt"), userText30) << "3.0.1 should load user.txt from the 3.0 folder";
    EXPECT_EQ(loadSettingsFile(context, "3.1.1", "user.txt"), userText30) << "3.1.1 should load user.txt from the 3.0 folder";

    // Nonexisting file should result in empty contents
    EXPECT_EQ(loadSettingsFile(context, "2.12.3", "nonexistent.txt"), std::string()) << "No content expected for nonexistent file";

    // Check what path is returned for the nonexistent settings file
    auto nonexistentPath = settings::SettingsManager(context, "2.16.33").getExistingSettingsFile("nonexistent.txt");
    EXPECT_EQ(nonexistentPath, "") << "No content expected for nonexistent file";

    // Remove the test folders
    os::removeDirectory(context.getSettingsPath());
}

}
