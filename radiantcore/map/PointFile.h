#pragma once

#include <vector>
#include "irender.h"
#include "imap.h"
#include "icommandsystem.h"
#include "irenderable.h"
#include "math/Vector3.h"
#include "render.h"

namespace map
{

/// Renderable point trace to identify leak positions
class PointFile: public Renderable
{
	// Vector of point coordinates
	RenderablePointVector _points;

	// Holds the current position in the point file chain
	std::size_t _curPos;

    // The shader for rendering the line
	ShaderPtr _shader;

public:
	// Constructor
	PointFile();

	// Destructor
	virtual ~PointFile();

	// Query whether the point path is currently visible
	bool isVisible() const;

  	/*
	 * Solid renderable submission function (front-end)
	 */
	void renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const override;

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override
	{}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight;
	}

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
