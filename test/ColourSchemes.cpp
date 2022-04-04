#include "RadiantTest.h"

#include "icolourscheme.h"
#include "iregistry.h"
#include "math/Vector3.h"
#include "os/path.h"
#include "string/convert.h"
#include "xmlutil/Document.h"
#include "settings/SettingsManager.h"

namespace test
{

namespace
{
    const char* const SCHEME_DARKRADIANT_DEFAULT = "DarkRadiant Default";
    const char* const SCHEME_QE3 = "QE3Radiant Original";
    const char* const SCHEME_BLACK_AND_GREEN = "Black & Green";
    const char* const SCHEME_MAYA_MAX = "Maya/Max/Lightwave Emulation";
    const char* const SCHEME_SUPER_MAL = "Super Mal";
}

class ColourSchemeTest :
    public RadiantTest
{
protected:
    void copySchemeFileToSettingsPath(const std::string& schemeFile)
    {
        fs::path schemeFilePath = _context.getTestResourcePath();
        schemeFilePath /= "settings/";
        schemeFilePath /= schemeFile;

        settings::SettingsManager manager(_context);
        fs::path targetFile = manager.getCurrentVersionSettingsFolder();
        targetFile /= "colours.xml";

        fs::remove(targetFile);
        fs::copy(schemeFilePath, targetFile);
    }
};

class ColourSchemeTestWithIncompleteScheme :
    public ColourSchemeTest
{
public:
    void preStartup() override
    {
        copySchemeFileToSettingsPath("colours_incomplete.xml");
    }
};

class ColourSchemeTestWithUserColours :
    public ColourSchemeTest
{
protected:
    fs::path _userDefinedSchemePath;

public:
    void preStartup() override
    {
        // Store the value locally, the test case wants to have it
        _userDefinedSchemePath = _context.getTestResourcePath();
        _userDefinedSchemePath /= "settings/colours_userdefined.xml";

        copySchemeFileToSettingsPath("colours_userdefined.xml");
    }
};

class ColourSchemeTestWithEmptySettings :
    public ColourSchemeTest
{
public:
    void preStartup() override
    {
        // Kill any colours.xml file present in the settings folder before regular SetUp
        settings::SettingsManager manager(_context);
        fs::path coloursFile = manager.getCurrentVersionSettingsFolder();
        coloursFile /= "colours.xml";

        fs::remove(coloursFile);
    }
};

namespace
{

std::map<std::string, Vector3> loadSchemeFromXml(const std::string& name, const std::string& xmlPath)
{
    xml::Document doc(xmlPath);

    auto schemes = doc.findXPath("//colourscheme[@name='" + name + "']");
    EXPECT_EQ(schemes.size(), 1) << "Multiple schemes named " << name << " in file " << xmlPath;

    auto schemeColours = doc.findXPath("//colourscheme[@name='" + name + "']//colour");
    EXPECT_NE(schemeColours.size(), 0) << "Scheme " << name << " is empty";

    std::map<std::string, Vector3> result;

    for (auto colourNode : schemeColours)
    {
        result[colourNode.getAttributeValue("name")] = string::convert<Vector3>(colourNode.getAttributeValue("value"));
    }

    return result;
}

std::size_t getItemCountInScheme(const colours::IColourScheme& scheme)
{
    std::size_t result = 0;

    scheme.foreachColour([&](const std::string&, const colours::IColourItem& item) { ++result; });

    return result;
}

}

// With an empty settings folder, the default schemes should be loaded
TEST_F(ColourSchemeTestWithEmptySettings, LoadDefaultSchemes)
{
    auto defaultSchemeNames = {
        SCHEME_DARKRADIANT_DEFAULT,
        SCHEME_QE3,
        SCHEME_BLACK_AND_GREEN,
        SCHEME_MAYA_MAX,
        SCHEME_SUPER_MAL
    };

    // Check the default colours.xml file
    std::string defaultColoursFile = _context.getRuntimeDataPath() + "colours.xml";
    EXPECT_TRUE(fs::exists(defaultColoursFile)) << "Could not find factory colours file: " << defaultColoursFile;

    for (auto name : defaultSchemeNames)
    {
        EXPECT_TRUE(GlobalColourSchemeManager().schemeExists(name)) << "Scheme " << name << " not found";

        auto defaultScheme = loadSchemeFromXml(name, defaultColoursFile);
        auto& loadedScheme = GlobalColourSchemeManager().getColourScheme(name);

        // Number of items must match
        EXPECT_EQ(defaultScheme.size(), getItemCountInScheme(loadedScheme));

        // Each item must match
        for (const auto& defaultColour : defaultScheme)
        {
            EXPECT_EQ(loadedScheme.getColour(defaultColour.first).getColour(), defaultColour.second)
                << "Scheme " << name << ": colour " << defaultColour.first
                << " doesn't match the default value " << defaultColour.second;
        }
    }
}

TEST_F(ColourSchemeTestWithEmptySettings, DefaultSchemeIsActive)
{
    EXPECT_EQ(GlobalColourSchemeManager().getActiveScheme().getName(), SCHEME_DARKRADIANT_DEFAULT);
}

TEST_F(ColourSchemeTestWithEmptySettings, ChangeActiveScheme)
{
    auto newSchemeName = SCHEME_SUPER_MAL;
    GlobalColourSchemeManager().setActive(newSchemeName);
    EXPECT_EQ(GlobalColourSchemeManager().getActiveScheme().getName(), newSchemeName);
}

TEST_F(ColourSchemeTestWithEmptySettings, ActiveSchemePersisted)
{
    // Check the current default
    auto activeSchemes = GlobalRegistry().findXPath("user/ui/colourschemes//colourscheme[@active='1']");
    EXPECT_EQ(activeSchemes.size(), 1);
    EXPECT_EQ(activeSchemes[0].getAttributeValue("name"), SCHEME_DARKRADIANT_DEFAULT);

    auto newSchemeName = SCHEME_SUPER_MAL;
    GlobalColourSchemeManager().setActive(newSchemeName);

    // Export the state to the registry
    GlobalColourSchemeManager().saveColourSchemes();

    // By this time, the colour scheme should be saved to the registry
    // This is checking implementation details, but it might be worth it
    activeSchemes = GlobalRegistry().findXPath("user/ui/colourschemes//colourscheme[@active='1']");
    EXPECT_EQ(activeSchemes.size(), 1);
    EXPECT_EQ(activeSchemes[0].getAttributeValue("name"), newSchemeName);

    // Save to disk
    GlobalRegistry().saveToDisk();

    // Check the XML files if the active flag was set
    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    xml::Document doc(savedColoursFile);
    auto schemes = doc.findXPath("//colourscheme[@active='1']");
    EXPECT_EQ(schemes.size(), 1) << "More than one scheme set to active in " << savedColoursFile;
    EXPECT_EQ(schemes[0].getAttributeValue("name"), newSchemeName);
}

namespace
{

void modifySchemeColour(const std::string& schemeToModify, const std::string& colourName, const Vector3& newValue)
{
    auto& scheme = GlobalColourSchemeManager().getColourScheme(schemeToModify);

    EXPECT_NE(scheme.getColour(colourName).getColour(), newValue) << "Test setup failure: colour is already set to new RGB values";

    // Modify one colour
    scheme.getColour(colourName).getColour() = newValue;
}

}

TEST_F(ColourSchemeTestWithEmptySettings, SystemThemeColourChangePersisted)
{
    Vector3 newValue(0.99, 0.99, 0.99);
    modifySchemeColour(SCHEME_DARKRADIANT_DEFAULT, "default_brush", newValue);

    // Export the state to the registry and save it to disk
    GlobalColourSchemeManager().saveColourSchemes();
    GlobalRegistry().saveToDisk();

    // Check the XML files if the colour was set
    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    auto savedScheme = loadSchemeFromXml(SCHEME_DARKRADIANT_DEFAULT, savedColoursFile);
    EXPECT_EQ(savedScheme["default_brush"], newValue);
}

TEST_F(ColourSchemeTestWithEmptySettings, CopiedSchemePersisted)
{
    auto newSchemeName = "My Copied Scheme";

    // Make a copy of the default scheme
    GlobalColourSchemeManager().copyScheme(SCHEME_DARKRADIANT_DEFAULT, newSchemeName);

    // Export the state to the registry
    GlobalColourSchemeManager().saveColourSchemes();

    // Save to disk
    GlobalRegistry().saveToDisk();

    // Check the XML files if the colour was set
    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    auto savedCopiedScheme = loadSchemeFromXml(newSchemeName, savedColoursFile);
    auto savedDefaultScheme = loadSchemeFromXml(SCHEME_DARKRADIANT_DEFAULT, savedColoursFile);

    EXPECT_EQ(savedCopiedScheme.size(), savedDefaultScheme.size());

    // The values must be equal
    for (const auto& pair : savedDefaultScheme)
    {
        EXPECT_EQ(savedCopiedScheme[pair.first], pair.second);
    }
}

TEST_F(ColourSchemeTestWithIncompleteScheme, SchemeUpgrade)
{
    // The colours.xml should be loaded and the missing colours should be added
    auto missingColour = "selected_group_items";

    auto& activeScheme = GlobalColourSchemeManager().getActiveScheme();

    // Super Mal must be tagged as active
    EXPECT_EQ(activeScheme.getName(), SCHEME_SUPER_MAL);

    // The "selected_group_items" colour definition is missing in that file
    // it should have been set to the defaults of the factory super mal

    // Load the factory defaults
    std::string defaultColoursFile = _context.getRuntimeDataPath() + "colours.xml";
    EXPECT_TRUE(fs::exists(defaultColoursFile)) << "Could not find factory colours file: " << defaultColoursFile;

    auto defaultScheme = loadSchemeFromXml(SCHEME_SUPER_MAL, defaultColoursFile);
    EXPECT_EQ(defaultScheme[missingColour], activeScheme.getColour(missingColour).getColour());

    // When saving, the scheme should now be complete
    GlobalColourSchemeManager().saveColourSchemes();
    GlobalRegistry().saveToDisk();

    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    auto savedScheme = loadSchemeFromXml(SCHEME_SUPER_MAL, savedColoursFile);
    EXPECT_EQ(savedScheme.size(), defaultScheme.size());

    // All colour keys must be present
    for (const auto& pair : savedScheme)
    {
        EXPECT_EQ(savedScheme.count(pair.first), 1);
    }
}

TEST_F(ColourSchemeTestWithUserColours, RestoreActiveSchemeFromUserFile)
{
    auto& activeScheme = GlobalColourSchemeManager().getActiveScheme();

    // The "MyMaya" custom theme is tagged as active in the file
    EXPECT_EQ(activeScheme.getName(), "MyMaya");
}

TEST_F(ColourSchemeTestWithUserColours, RestoreAllSchemesFromUserFile)
{
    // Compare the schemes that we know are stored in the file
    // The colours_userdefined.xml file specifically changes the "camera_background"
    // colour in the SCHEME_BLACK_AND_GREEN theme, plus it adds a custom theme named MyMaya
    auto storedSchemeNames = {
        SCHEME_DARKRADIANT_DEFAULT,
        SCHEME_QE3,
        SCHEME_BLACK_AND_GREEN,
        SCHEME_MAYA_MAX,
        SCHEME_SUPER_MAL,
        "MyMaya" // custom theme
    };

    for (auto schemeName : storedSchemeNames)
    {
        auto& loadedScheme = GlobalColourSchemeManager().getColourScheme(schemeName);
        auto storedScheme = loadSchemeFromXml(schemeName, _userDefinedSchemePath.string());

        // Check colour values
        for (const auto& pair : storedScheme)
        {
            EXPECT_EQ(loadedScheme.getColour(pair.first).getColour(), pair.second);
        }
    }
}

TEST_F(ColourSchemeTestWithUserColours, SavedUserSchemesAreNotReadOnly)
{
    // When saving user-defined schemes the readonly tag should only be present
    // on the non-system themes
    GlobalColourSchemeManager().saveColourSchemes();
    GlobalRegistry().saveToDisk();

    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    std::set<std::string> readOnlySchemes = {
        SCHEME_DARKRADIANT_DEFAULT,
        SCHEME_QE3,
        SCHEME_BLACK_AND_GREEN,
        SCHEME_MAYA_MAX,
        SCHEME_SUPER_MAL
    };

    auto userScheme = "MyMaya";

    xml::Document doc(savedColoursFile);
    auto schemes = doc.findXPath("//colourscheme");
    EXPECT_EQ(schemes.size(), readOnlySchemes.size() + 1); // readOnlySchemes + one user scheme

    for (auto schemeNode : schemes)
    {
        auto name = schemeNode.getAttributeValue("name");

        // Scheme should be read-only for the system ones only
        EXPECT_EQ(schemeNode.getAttributeValue("readonly") == "1", readOnlySchemes.count(name) == 1);
    }
}

TEST_F(ColourSchemeTestWithUserColours, ColourChangePersisted)
{
    // This is the same test as TEST_F(ColourSchemeTestWithEmptySettings, SystemThemeColourChangePersisted) above
    Vector3 newValue(0.99, 0.99, 0.99);
    modifySchemeColour(SCHEME_DARKRADIANT_DEFAULT, "default_brush", newValue);
    modifySchemeColour("MyMaya", "default_brush", newValue);

    // Export the state to the registry and save it to disk
    GlobalColourSchemeManager().saveColourSchemes();
    GlobalRegistry().saveToDisk();

    // Check the XML files if the colour was set
    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    auto savedDefaultScheme = loadSchemeFromXml(SCHEME_DARKRADIANT_DEFAULT, savedColoursFile);
    EXPECT_EQ(savedDefaultScheme["default_brush"], newValue);

    auto savedUserScheme = loadSchemeFromXml("MyMaya", savedColoursFile);
    EXPECT_EQ(savedDefaultScheme["default_brush"], newValue);
}

TEST_F(ColourSchemeTestWithUserColours, DeleteUserTheme)
{
    std::string userThemeName = "MyMaya";
    EXPECT_TRUE(GlobalColourSchemeManager().schemeExists(userThemeName));

    GlobalColourSchemeManager().deleteScheme(userThemeName);

    // Export the state to the registry and save it to disk
    GlobalColourSchemeManager().saveColourSchemes();
    GlobalRegistry().saveToDisk();

    // Check the XML files if the scheme was actually removed
    settings::SettingsManager manager(_context);
    std::string savedColoursFile = manager.getCurrentVersionSettingsFolder() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    xml::Document doc(savedColoursFile);
    auto schemes = doc.findXPath("//colourscheme[@name='" + userThemeName + "']");
    EXPECT_TRUE(schemes.empty());
}

TEST_F(ColourSchemeTestWithUserColours, RestoreDeletedThemeFromRegistry)
{
    std::string userThemeName = "MyMaya";
    EXPECT_TRUE(GlobalColourSchemeManager().schemeExists(userThemeName));

    // Delete the user theme
    GlobalColourSchemeManager().deleteScheme(userThemeName);
    EXPECT_FALSE(GlobalColourSchemeManager().schemeExists(userThemeName));

    // Restore the changes from the registry
    GlobalColourSchemeManager().restoreColourSchemes();

    // The user theme should be there again
    EXPECT_TRUE(GlobalColourSchemeManager().schemeExists(userThemeName));
}

TEST_F(ColourSchemeTestWithUserColours, RestoreChangedColourFromRegistry)
{
    Vector3 newValue(0.99, 0.99, 0.99);
    modifySchemeColour(SCHEME_DARKRADIANT_DEFAULT, "default_brush", newValue);
    modifySchemeColour("MyMaya", "default_brush", newValue);

    EXPECT_EQ(GlobalColourSchemeManager().getColourScheme("MyMaya").getColour("default_brush").getColour(), newValue);
    EXPECT_EQ(GlobalColourSchemeManager().getColourScheme(SCHEME_DARKRADIANT_DEFAULT).getColour("default_brush").getColour(), newValue);

    // Restore the changes from the registry
    GlobalColourSchemeManager().restoreColourSchemes();

    // The changes should be undone
    EXPECT_NE(GlobalColourSchemeManager().getColourScheme("MyMaya").getColour("default_brush").getColour(), newValue);
    EXPECT_NE(GlobalColourSchemeManager().getColourScheme(SCHEME_DARKRADIANT_DEFAULT).getColour("default_brush").getColour(), newValue);
}

TEST_F(ColourSchemeTestWithUserColours, ForeachScheme)
{
    // Use a vector to record visited nodes to catch duplicate names
    std::vector<std::string> visitedSchemes;
    GlobalColourSchemeManager().foreachScheme([&](const std::string& name, colours::IColourScheme&)
    {
        visitedSchemes.push_back(name);
    });

    std::set<std::string> expectedSchemeNames = {
        SCHEME_DARKRADIANT_DEFAULT,
        SCHEME_QE3,
        SCHEME_BLACK_AND_GREEN,
        SCHEME_MAYA_MAX,
        SCHEME_SUPER_MAL,
        "MyMaya" // custom theme
    };

    EXPECT_EQ(expectedSchemeNames.size(), visitedSchemes.size());

    for (auto expectedScheme : expectedSchemeNames)
    {
        EXPECT_EQ(std::count(visitedSchemes.begin(), visitedSchemes.end(), expectedScheme), 1);
    }
}

}
