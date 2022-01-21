#pragma once

#include <vector>
#include "irender.h"
#include "imap.h"
#include "icommandsystem.h"
#include "math/Vector3.h"
#include "render/VertexCb.h"
#include "RenderablePointFile.h"

namespace map
{

/// Renderable point trace to identify leak positions
class PointFile
{
	// Vector of point coordinates
	std::vector<VertexCb> _points;

	// Holds the current position in the point file chain
	std::size_t _curPos;

    RenderablePointFile _renderable;

public:
	// Constructor
	PointFile();

	// Destructor
	virtual ~PointFile();

	// Query whether the point path is currently visible
	bool isVisible() const;

	void onMapEvent(IMap::MapEvent ev);

    /// Show the specified pointfile, or hide if the path is empty
	void show(const fs::path& pointfile);

private:

	/**
	 * greebo: This sets the camera position to the next/prev leak spot.
	 * @forward: pass true to set to the next leak spot, false for the previous
	 */
	void advance(bool forward);

	// command targets
	// Toggles visibility of the point file line
	void nextLeakSpot(const cmd::ArgumentList& args);
	void prevLeakSpot(const cmd::ArgumentList& args);

	// Parse the specified pointfile and read the vectors into the point list
	void parse(const fs::path& pointfile);
};

} // namespace map
