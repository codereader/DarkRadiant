#include "RadiantTest.h"

#include "imap.h"
#include "imapformat.h"
#include "ibrush.h"
#include "iselection.h"
#include "scenelib.h"
#include "string/predicate.h"
#include "xmlutil/Document.h"
#include "messages/MapFileOperation.h"
#include "algorithm/XmlUtils.h"

namespace test
{

namespace
{

void createAndSelectSingleBrush()
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brushNode = GlobalBrushCreator().createBrush();
    worldspawn->addChildNode(brushNode);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brushNode, true);
}

}

using MapExportTest = RadiantTest;

TEST_F(MapExportTest, exportSelectedWithFormat)
{
    createAndSelectSingleBrush();

    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    std::ostringstream output;
    GlobalMapModule().exportSelected(output, format);

    output.flush();

    // Minimal assertion: we got a string that appears to start like an XML document
    algorithm::assertStringIsMapxFile(output.str());
}

TEST_F(MapExportTest, exportSelectedDoesNotSendMessages)
{
    createAndSelectSingleBrush();

    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    bool messageReceived = false;

    // Subscribe to the messages potentially sent by the MapExporter
    auto listener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::MapFileOperation,
        radiant::TypeListener<map::FileOperation>([&](map::FileOperation& msg)
    { 
        messageReceived = true; 
    }));

    std::ostringstream output;
    GlobalMapModule().exportSelected(output, format);
    output.flush();

    GlobalRadiantCore().getMessageBus().removeListener(listener);

    EXPECT_FALSE(messageReceived) << "Received a FileOperation message while exporting the selection";
}

}
