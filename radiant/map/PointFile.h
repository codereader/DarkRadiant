#pragma once

#include <vector>
#include "irender.h"
#include "imap.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "irenderable.h"
#include "math/Vector3.h"
#include "render.h"

namespace map 
{

class PointFile :
	public RegisterableModule,
	public Renderable
{
private:
	// Vector of point coordinates
	RenderablePointVector _points;
	
	// Holds the current position in the point file chain
	std::size_t _curPos;

	ShaderPtr _renderstate;

public:
	// Constructor
	PointFile();

	// Destructor
	virtual ~PointFile() {}

	// Query whether the point path is currently visible
	bool isVisible() const;

  	/*
	 * Solid renderable submission function (front-end)
	 */
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override
	{}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight;
	}

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// Registers the events to the EventManager
	void registerCommands();

	/*
	 * Toggle the status of the pointfile rendering. If the pointfile must be
	 * shown, the file is parsed automatically.
	 */
	void show(bool show);

	/**
	 * greebo: Clears the point file vector, which is the same as hiding it.
	 */
	void clear();

	/** 
	 * greebo: This sets the camera position to the next/prev leak spot.
	 * @forward: pass true to set to the next leak spot, false for the previous
	 */
	void advance(bool forward);

	// command targets
	// Toggles visibility of the point file line
	void toggle(const cmd::ArgumentList& args);
	void nextLeakSpot(const cmd::ArgumentList& args);
	void prevLeakSpot(const cmd::ArgumentList& args);

	// Parse the current pointfile and read the vectors into the point list
	void parse();

	void onMapEvent(IMap::MapEvent ev);
};

} // namespace map
