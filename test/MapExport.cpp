#include "RadiantTest.h"

#include "imap.h"
#include "imapformat.h"
#include "ibrush.h"
#include "iselection.h"
#include "scenelib.h"
#include "string/predicate.h"
#include "xmlutil/Document.h"

namespace test
{

using MapExportTest = RadiantTest;

TEST_F(MapExportTest, exportSelectedWithFormat)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brushNode = GlobalBrushCreator().createBrush();
    worldspawn->addChildNode(brushNode);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brushNode, true);

    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    std::ostringstream output;
    GlobalMapModule().exportSelected(output, format);

    output.flush();

    // Minimal assertion: we got a string that appears to start like an XML document
    auto result = output.str();
    ASSERT_TRUE(string::starts_with(result, "<?xml version=\"1.0\" encoding=\"utf-8\"?>"));
    ASSERT_TRUE(result.find("<map") != std::string::npos);
}

}
