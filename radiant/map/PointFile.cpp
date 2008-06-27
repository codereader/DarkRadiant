#include "PointFile.h"

#include "igl.h"
#include "iscenegraph.h"
#include "ieventmanager.h"
#include <fstream>
#include <iostream>

#include "generic/callback.h"
#include "gtkutil/dialog.h"
#include "math/matrix.h"
#include "math/Vector3.h"
#include "map/Map.h"
#include "mainframe.h"
#include "camera/CamWnd.h"
#include "xyview/GlobalXYWnd.h"

namespace map {

// Constructor
PointFile::PointFile() :
	_curPos(_points.begin()), 
	_displayList(0)
{
	_renderstate = GlobalShaderCache().capture("$POINTFILE");
	GlobalShaderCache().attachRenderable(*this);
}

void PointFile::destroy() {
	GlobalShaderCache().detachRenderable(*this);
	_renderstate = ShaderPtr();
}

// Static accessor method
PointFile& PointFile::Instance() {
	static PointFile _instance;
	return _instance;
}

// Query whether the point path is currently visible
bool PointFile::isVisible() const {
	return _displayList != 0;
}

/*
 * Toggle the status of the pointfile rendering. If the pointfile must be
 * shown, the file is parsed automatically.
 */
void PointFile::show(bool show) {
	
	// Update the status if required
	if(show && _displayList == 0) {
		// Parse the pointfile from disk
		parse();
		if (_points.size() > 0) {
			generateDisplayList();
		}
	}
	else if(!show && _displayList != 0) {
		glDeleteLists (_displayList, 1);
		_displayList = 0;
		_points.clear();
	}
	
	// Regardless whether hide or show, we reset the current position
	_curPos = _points.begin();
	
	// Redraw the scene
	SceneChangeNotify();
}

/*
 * OpenGL render function (back-end).
 */
void PointFile::render(RenderStateFlags state) const {
	glCallList(_displayList);
}

/*
 * Solid renderable submission function (front-end)
 */
void PointFile::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	if(isVisible()) {
		renderer.SetState(_renderstate, Renderer::eWireframeOnly);
		renderer.SetState(_renderstate, Renderer::eFullMaterials);
		renderer.addRenderable(*this, g_matrix4_identity);
	}
}

/*
 * Wireframe renderable submission function (front-end).
 */
void PointFile::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

// Parse the current pointfile and read the vectors into the point list
void PointFile::parse() {

	// Pointfile is the same as the map file but with a .lin extension
	// instead of .map
	std::string mapName = GlobalMap().getMapName();
	std::string pfName = mapName.substr(0, mapName.rfind(".")) + ".lin";
	
	// Open the pointfile and get its input stream if possible
	std::ifstream inFile(pfName.c_str());
	if (!inFile) {
		gtkutil::errorDialog("Could not open pointfile:\n\n" + pfName,
							 MainFrame_getWindow());
		return;
	}

	// Pointfile is a list of float vectors, one per line, with components
	// separated by spaces.
	while (inFile.good()) {
		float x, y, z;
		inFile >> x; inFile >> y; inFile >> z;
		_points.push_back(Vector3(x, y, z));			
	}
}

// create the display list at the end
void PointFile::generateDisplayList() {
	_displayList = glGenLists(1);

	glNewList (_displayList,  GL_COMPILE);

	glBegin(GL_LINE_STRIP);
	for (VectorList::iterator i = _points.begin();
		 i != _points.end();
		 ++i)
	{
		glVertex3dv(*i);
	}
	glEnd();
	glLineWidth (1);
	
	glEndList();
}

// Static shader
ShaderPtr PointFile::_renderstate;

// advance camera to previous point
void PointFile::advance(bool forward) {
	if (!isVisible()) {
		return;
	}

	if (forward) {
		if (_curPos+2 == _points.end())	{
			globalOutputStream() << "End of pointfile\n";
			return;
		}

		_curPos++;
	}
	else {
		// Backward movement
		if (_curPos == _points.begin()) {
			globalOutputStream() << "Start of pointfile\n";
			return;
		}
	
		_curPos--;
	}

	CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	camwnd.setCameraOrigin(*_curPos);
	GlobalXYWnd().getActiveXY()->setOrigin(*_curPos);
	{
		Vector3 dir((*(_curPos+1) - camwnd.getCameraOrigin()).getNormalised());
		Vector3 angles(camwnd.getCameraAngles());
		angles[CAMERA_YAW] = static_cast<double>(radians_to_degrees(atan2(dir[1], dir[0])));
		angles[CAMERA_PITCH] = static_cast<double>(radians_to_degrees(asin(dir[2])));
		camwnd.setCameraAngles(angles);
	}
	
	// Redraw the scene
	SceneChangeNotify();
}

void PointFile::nextLeakSpot() {
	Instance().advance(true);
}

void PointFile::prevLeakSpot() {
	Instance().advance(false);
}

void PointFile::clear() {
	show(false);
}

void PointFile::toggle() {
	Instance().show(!Instance().isVisible());
}

void PointFile::registerCommands() {
	GlobalEventManager().addCommand("TogglePointfile", FreeCaller<PointFile::toggle>());
	GlobalEventManager().addCommand("NextLeakSpot", FreeCaller<PointFile::nextLeakSpot>());
	GlobalEventManager().addCommand("PrevLeakSpot", FreeCaller<PointFile::prevLeakSpot>());
}

} // namespace map
