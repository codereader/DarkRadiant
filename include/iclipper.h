#pragma once

#include "imodule.h"
#include "iorthoview.h"
#include "math/Vector3.h"

// The possible split modes
enum EBrushSplit {
	eFront,
	eBack,
	eFrontAndBack,
};

enum PlaneClassification {
    ePlaneFront = 0,
    ePlaneBack = 1,
    ePlaneOn = 2,
};

struct BrushSplitType {
	std::size_t counts[3];

	BrushSplitType() {
		counts[0] = 0;
		counts[1] = 0;
		counts[2] = 0;
	}
	BrushSplitType& operator+=(const BrushSplitType& other) {
		counts[0] += other.counts[0];
		counts[1] += other.counts[1];
		counts[2] += other.counts[2];
		return *this;
	}
};

class ClipPoint;

const char* const MODULE_CLIPPER("Clipper");
const char* const RKEY_CLIPPER_CAULK_SHADER("user/ui/clipper/caulkTexture");

/* greebo: This is the interface the clipper module has to provide.
 */
class IClipper :
	public RegisterableModule
{
public:
	// Gets called if the clip mode is toggled on/off
	virtual void onClipMode(bool enabled) = 0;

	// Returns true if the clip mode is enabled
	virtual bool clipMode() const = 0;

	// Methods to clip the selected brush or to split it (keeping both parts)
	virtual void clip() = 0;
	virtual void splitClip() = 0;

	// Inverts the clip plane "direction" to determine which part of the brush is kept after clipping
	virtual void flipClip() = 0;

	// True when new faces should get caulked
	virtual bool useCaulkForNewFaces() const = 0;

	// Returns the name of the caulk shader to be used for clip-created planes
	virtual const std::string& getCaulkShader() const = 0;

	// Return or set the view type of the xy view (needed for the projections).
	virtual EViewType getViewType() const = 0;
	virtual void setViewType(EViewType viewType) = 0;

	// Returns the pointer to the currently moved clip point
	virtual ClipPoint* getMovingClip() = 0;
	virtual void setMovingClip(ClipPoint* clipPoint) = 0;

	// Retrieves the reference to the coordinates of the currently "selected" clip point
	virtual Vector3& getMovingClipCoords() = 0;

	virtual ClipPoint* find(const Vector3& point, EViewType viewtype, float scale) = 0;

	// Adds the given point as new clip point.
	virtual void newClipPoint(const Vector3& point) = 0;

	// Draws the clip points into the XYView
	virtual void draw(float scale) = 0;

	// Updates the clip plane information
	virtual void update() = 0;
};

// The accessor for the clipper module
inline IClipper& GlobalClipper() 
{
	// Cache the reference locally
	static IClipper&  _clipper(
		*std::static_pointer_cast<IClipper>(
			module::GlobalModuleRegistry().getModule(MODULE_CLIPPER)
		)
	);
	return _clipper;
}
