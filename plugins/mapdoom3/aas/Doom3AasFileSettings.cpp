#include "Doom3AasFileSettings.h"

#include "string/convert.h"
#include "string/trim.h"
#include "Util.h"

namespace map
{

Doom3AasFileSettings::Doom3AasFileSettings() :
    numBoundingBoxes(1),
    usePatches(false),
    writeBrushMap(false),
    playerFlood(false),
    noOptimize(false),
    allowSwimReachabilities(false),
    allowFlyReachabilities(false),
	fileExtension("aas48"),
	gravity(0, 0, -1066),
	gravityDir(gravity.getNormalised()),
	invGravityDir(-gravityDir),
    gravityValue(gravity.getLength()),
	maxStepHeight(14.0f),
	maxBarrierHeight(32.0f),
	maxWaterJumpHeight(20.0f),
	maxFallHeight(64.0f),
	minFloorCos(0.7f),
	tt_barrierJump(100),
	tt_startCrouching(100),
	tt_waterJump(100),
	tt_startWalkOffLedge(100)
{
    boundingBoxes[0] = AABB::createFromMinMax(Vector3(-16, -16, 0), Vector3(16, 16, 72));
}

void Doom3AasFileSettings::parseFromTokens(parser::DefTokeniser& tok)
{
    tok.assertNextToken("{");

    while (tok.hasMoreTokens())
    {
        std::string token = tok.nextToken();

        if (token == "}")
        {
            break;
        }
        else if (token == "bboxes")
        {
            // Parse bboxes
            tok.assertNextToken("{");

            std::size_t index = 0;

            while (tok.hasMoreTokens() && index < MAX_AAS_BOUNDING_BOXES)
            {
                if (tok.peek() == "}")
                {
                    tok.nextToken();
                    break;
                }

                // Parse bbox
                boundingBoxes[index].origin = parseVector3(tok);
                tok.assertNextToken("-");
                boundingBoxes[index].extents = parseVector3(tok);

                ++index;
            }
        }
        else if (token == "usePatches")
        {
            tok.assertNextToken("=");
            usePatches = string::convert<bool>(tok.nextToken());
        }
        else if (token == "writeBrushMap")
        {
            tok.assertNextToken("=");
            writeBrushMap = string::convert<bool>(tok.nextToken());
        }
        else if (token == "playerFlood")
        {
            tok.assertNextToken("=");
            playerFlood = string::convert<bool>(tok.nextToken());
        }
        else if (token == "allowSwimReachabilities")
        {
            tok.assertNextToken("=");
            allowSwimReachabilities = string::convert<bool>(tok.nextToken());
        }
        else if (token == "allowFlyReachabilities")
        {
            tok.assertNextToken("=");
            allowFlyReachabilities = string::convert<bool>(tok.nextToken());
        }
        else if (token == "fileExtension")
        {
            tok.assertNextToken("=");
            fileExtension = string::trim_copy(tok.nextToken(), "\"");
        }
        else if (token == "gravity")
        {
            tok.assertNextToken("=");
            gravity = parseVector3(tok);

            gravityDir = gravity.getNormalised();
			gravityValue = gravity.getLength();
			invGravityDir = -gravityDir;
        }
        else if (token == "maxStepHeight")
        {
            tok.assertNextToken("=");
            maxStepHeight = string::convert<float>(tok.nextToken());
        }
        else if (token == "maxBarrierHeight")
        {
            tok.assertNextToken("=");
            maxBarrierHeight = string::convert<float>(tok.nextToken());
        }
        else if (token == "maxWaterJumpHeight")
        {
            tok.assertNextToken("=");
            maxWaterJumpHeight = string::convert<float>(tok.nextToken());
        }
        else if (token == "maxFallHeight")
        {
            tok.assertNextToken("=");
            maxFallHeight = string::convert<float>(tok.nextToken());
        }
        else if (token == "minFloorCos")
        {
            tok.assertNextToken("=");
            minFloorCos = string::convert<float>(tok.nextToken());
        }
        else if (token == "tt_barrierJump")
        {
            tok.assertNextToken("=");
            tt_barrierJump = string::convert<int>(tok.nextToken());
        }
        else if (token == "tt_startCrouching")
        {
            tok.assertNextToken("=");
            tt_startCrouching = string::convert<int>(tok.nextToken());
        }
        else if (token == "tt_waterJump")
        {
            tok.assertNextToken("=");
            tt_waterJump = string::convert<int>(tok.nextToken());
        }
        else if (token == "tt_startWalkOffLedge")
        {
            tok.assertNextToken("=");
            tt_startWalkOffLedge = string::convert<int>(tok.nextToken());
        }
        else
        {
            throw parser::ParseException("Unknown settings token: " + token);
        }
    }
}

}
