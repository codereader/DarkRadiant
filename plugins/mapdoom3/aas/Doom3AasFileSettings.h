#pragma once

#include "math/AABB.h"
#include "math/Vector3.h"
#include "parser/DefTokeniser.h"

namespace map
{

#define MAX_AAS_BOUNDING_BOXES 4

class Doom3AasFileSettings
{
public:
    Doom3AasFileSettings();

    int         numBoundingBoxes;
	AABB        boundingBoxes[MAX_AAS_BOUNDING_BOXES];
	bool        usePatches;
	bool        writeBrushMap;
	bool        playerFlood;
	bool        noOptimize;
	bool        allowSwimReachabilities;
	bool        allowFlyReachabilities;
	std::string fileExtension;
								// physics settings
	Vector3     gravity;
	Vector3     gravityDir;
	Vector3     invGravityDir;
	float       gravityValue;
	float       maxStepHeight;
	float       maxBarrierHeight;
	float       maxWaterJumpHeight;
	float       maxFallHeight;
	float       minFloorCos;
			
    // fixed travel times
	int		    tt_barrierJump;
	int		    tt_startCrouching;
	int		    tt_waterJump;
	int		    tt_startWalkOffLedge;

    // Parse from token stream. The opening "settings" token should already have been consumed
    void parseFromTokens(parser::DefTokeniser& tok);
};

}
