#include "RadiantTest.h"

#include "icolourscheme.h"
#include "iregistry.h"
#include "math/Vector3.h"
#include "os/path.h"
#include "string/convert.h"
#include "xmlutil/Document.h"

namespace test
{

class ColourSchemeTest :
    public RadiantTest
{};

class ColourSchemeTestWithEmptySettings :
    public ColourSchemeTest
{
public:
    void SetUp() override
    {
        // Kill any colours.xml file present in the settings folder before regular SetUp
        fs::path coloursFile = _context.getSettingsPath();
        coloursFile /= "colours.xml";
        
        fs::remove(coloursFile);

        RadiantTest::SetUp();
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
        "DarkRadiant Default",
        "QE3Radiant Original",
        "Black & Green",
        "Maya/Max/Lightwave Emulation",
        "Super Mal"
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
    EXPECT_EQ(GlobalColourSchemeManager().getActiveScheme().getName(), "DarkRadiant Default");
}

TEST_F(ColourSchemeTestWithEmptySettings, ChangeActiveScheme)
{
    auto newSchemeName = "Super Mal";
    GlobalColourSchemeManager().setActive(newSchemeName);
    EXPECT_EQ(GlobalColourSchemeManager().getActiveScheme().getName(), newSchemeName);
}

TEST_F(ColourSchemeTestWithEmptySettings, ActiveSchemePersisted)
{
    // Check the current default
    auto activeSchemes = GlobalRegistry().findXPath("user/ui/colourschemes//colourscheme[@active='1']");
    EXPECT_EQ(activeSchemes.size(), 1);
    EXPECT_EQ(activeSchemes[0].getAttributeValue("name"), "DarkRadiant Default");

    auto newSchemeName = "Super Mal";
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
    std::string savedColoursFile = _context.getSettingsPath() + "colours.xml";
    EXPECT_TRUE(fs::exists(savedColoursFile)) << "Could not find saved colours file: " << savedColoursFile;

    xml::Document doc(savedColoursFile);
    auto schemes = doc.findXPath("//colourscheme[@active='1']");
    EXPECT_EQ(schemes.size(), 1) << "More than one scheme set to active in " << savedColoursFile;
    EXPECT_EQ(schemes[0].getAttributeValue("name"), newSchemeName);
}

}
