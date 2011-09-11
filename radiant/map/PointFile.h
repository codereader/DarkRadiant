#ifndef POINTFILE_H_
#define POINTFILE_H_

#include <vector>
#include "irender.h"
#include "icommandsystem.h"
#include "irenderable.h"
#include "math/Vector3.h"

namespace map {

class PointFile :
	public Renderable,
	public OpenGLRenderable
{
	// Vector of point coordinates
	typedef std::vector<Vector3> VectorList;
	VectorList _points;

	// Holds the current position in the point file chain
	VectorList::iterator _curPos;

	// GL display list pointer for rendering the point path
	int _displayList;

	static ShaderPtr _renderstate;

public:
	// Constructor
	PointFile();

	// Destructor
	virtual ~PointFile() {}

	/** greebo: Accessor method containing the singleton instance.
	 */
	static PointFile& Instance();

	/** greebo: This releases the shader and detaches this class from
	 * 			the shadercache.
	 */
	void destroy();

	// Query whether the point path is currently visible
	bool isVisible() const;

  	/*
	 * Toggle the status of the pointfile rendering. If the pointfile must be
	 * shown, the file is parsed automatically.
	 */
	void show(bool show);

	/*
	 * OpenGL render function (back-end).
	 */
	void render(const RenderInfo& info) const;

	/*
	 * Solid renderable submission function (front-end)
	 */
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	/** greebo: This sets the camera position to the next/prev leak spot.
	 *
	 * @forward: pass true to set to the next leak spot, false for the previous
	 */
	void advance(bool forward);

	/** greebo: Clears the point file vector and hides it, if applicable.
	 */
	void clear();

	// Registers the events to the EventManager
	static void registerCommands();

	// Static command targets, these re-route the call to the static instance
	static void nextLeakSpot(const cmd::ArgumentList& args);
	static void prevLeakSpot(const cmd::ArgumentList& args);

	// Toggles visibility of the point file line
	static void toggle(const cmd::ArgumentList& args);

private:
	// Parse the current pointfile and read the vectors into the point list
	void parse();

	// Generates the OpenGL displaylist from the point vector
	void generateDisplayList();
};

} // namespace map

#endif /*POINTFILE_H_*/
