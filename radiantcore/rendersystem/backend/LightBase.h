#pragma once

namespace render
{

/**
 * Common light type used in the backend renderer.
 * It only lives through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 *
 * Provides base methods shared by BlendLights and RegularLights.
 */ 
class LightBase
{
protected:

};

}
