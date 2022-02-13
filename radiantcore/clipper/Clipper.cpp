#include "Clipper.h"

#include "i18n.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "itexdef.h"

#include "registry/registry.h"
#include "module/StaticModule.h"
#include "ClipPoint.h"
#include "SplitAlgorithm.h"
#include "brush/csg/CSG.h"
#include "debugging/debugging.h"

#include <functional>

namespace
{
	const char* const RKEY_CLIPPER_USE_CAULK = "user/ui/clipper/useCaulk";
}

// Constructor
Clipper::Clipper() :
	_movingClip(NULL),
	_switch(true)
{}

// Update the internally stored variables on registry key change
void Clipper::keyChanged()
{
	_caulkShader = GlobalRegistry().get(RKEY_CLIPPER_CAULK_SHADER);
	_useCaulk = registry::getValue<bool>(RKEY_CLIPPER_USE_CAULK);
}

void Clipper::constructPreferences() {
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Clipper"));

	page.appendCheckBox(_("Clipper tool uses caulk texture"), RKEY_CLIPPER_USE_CAULK);
	page.appendEntry(_("Caulk shader name"), RKEY_CLIPPER_CAULK_SHADER);
}

EViewType Clipper::getViewType() const {
	return _viewType;
}

void Clipper::setViewType(EViewType viewType) {
	_viewType = viewType;
}

ClipPoint* Clipper::getMovingClip() {
	return _movingClip;
}

Vector3& Clipper::getMovingClipCoords() {
	// Check for NULL pointers, one never knows
	if (_movingClip != NULL) {
		return _movingClip->_coords;
	}

	// Return at least anything, i.e. the coordinates of the first clip point
	return _clipPoints[0]._coords;
}

void Clipper::setMovingClip(ClipPoint* clipPoint) {
	_movingClip = clipPoint;
}

bool Clipper::useCaulkForNewFaces() const
{
	return _useCaulk;
}

const std::string& Clipper::getCaulkShader() const
{
	return _caulkShader;
}

// greebo: Cycles through the three possible clip points and returns the nearest to point (for selectiontest)
ClipPoint* Clipper::find(const Vector3& point, EViewType viewtype, float scale) {
	double bestDistance = FLT_MAX;

	ClipPoint* bestClip = NULL;

	for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
		_clipPoints[i].testSelect(point, viewtype, scale, bestDistance, bestClip);
	}

	return bestClip;
}

// Returns true if at least two clip points are set
bool Clipper::valid() const {
	return _clipPoints[0].isSet() && _clipPoints[1].isSet();
}

void Clipper::draw(float scale) {
	// Draw clip points
	for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
		if (_clipPoints[i].isSet())
			_clipPoints[i].Draw(i, scale);
	}
}

void Clipper::getPlanePoints(Vector3 planepts[3], const AABB& bounds) const {
	ASSERT_MESSAGE(valid(), "clipper points not initialised");

	planepts[0] = _clipPoints[0]._coords;
	planepts[1] = _clipPoints[1]._coords;
	planepts[2] = _clipPoints[2]._coords;

	Vector3 maxs(bounds.origin + bounds.extents);
	Vector3 mins(bounds.origin - bounds.extents);

	if (!_clipPoints[2].isSet()) {
		int n = (_viewType == XY) ? 2 : (_viewType == YZ) ? 0 : 1;
		int x = (n == 0) ? 1 : 0;
		int y = (n == 2) ? 1 : 2;

		if (n == 1) // on viewtype XZ, flip clip points
		{
			planepts[0][n] = maxs[n];
			planepts[1][n] = maxs[n];
			planepts[2][x] = _clipPoints[0]._coords[x];
			planepts[2][y] = _clipPoints[0]._coords[y];
			planepts[2][n] = mins[n];
		}
		else {
			planepts[0][n] = mins[n];
			planepts[1][n] = mins[n];
			planepts[2][x] = _clipPoints[0]._coords[x];
			planepts[2][y] = _clipPoints[0]._coords[y];
			planepts[2][n] = maxs[n];
		}
	}
}

const Plane3& Clipper::getClipPlane()
{
    return _clipPlane;
}

void Clipper::setClipPlane(const Plane3& plane)
{
    _clipPlane = plane;
	algorithm::setBrushClipPlane(plane);
}

void Clipper::update() {
	Vector3 planepts[3];
	if (!valid()) {
		planepts[0] = Vector3(0, 0, 0);
		planepts[1] = Vector3(0, 0, 0);
		planepts[2] = Vector3(0, 0, 0);
		setClipPlane(Plane3(0, 0, 0, 0));
	}
	else {
		AABB bounds(Vector3(0, 0, 0), Vector3(64, 64, 64));
		getPlanePoints(planepts, bounds);
		if (_switch) {
			std::swap(planepts[0], planepts[1]);
		}
		setClipPlane(Plane3(planepts));
	}
	SceneChangeNotify();
}

void Clipper::flipClip() {
	_switch = !_switch;
	update();
	SceneChangeNotify();
}

void Clipper::reset() {
	for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
		_clipPoints[i].reset();
	}
}

void Clipper::clip() {
	if (clipMode() && valid()) {
		Vector3 planepts[3];
		AABB bounds(Vector3(0, 0, 0), Vector3(64, 64, 64));
		getPlanePoints(planepts, bounds);

		algorithm::splitBrushesByPlane(planepts, !_switch ? eFront : eBack);

		reset();
		update();
	}
}

void Clipper::splitClip() {
	if (clipMode() && valid()) {
		Vector3 planepts[3];
		AABB bounds(Vector3(0, 0, 0), Vector3(64, 64, 64));
		getPlanePoints(planepts, bounds);

		algorithm::splitBrushesByPlane(planepts, eFrontAndBack);

		reset();
		update();
	}
}

bool Clipper::clipMode() const {
	return GlobalSelectionSystem().getActiveManipulatorType() == selection::IManipulator::Clip;
}

void Clipper::onClipMode(bool enabled) {
	// Revert all clip points to <0,0,0> values
	reset();

	// Revert the _movingClip pointer, if clip mode to be disabled
	if (!enabled && _movingClip) {
		_movingClip = 0;
	}

	update();
}

void Clipper::newClipPoint(const Vector3& point) {
	if (_clipPoints[0].isSet() == false) {
		_clipPoints[0]._coords = point;
		_clipPoints[0].Set(true);
	}
	else if (_clipPoints[1].isSet() == false) {
		_clipPoints[1]._coords = point;
		_clipPoints[1].Set(true);
	}
	else if (_clipPoints[2].isSet() == false) {
		_clipPoints[2]._coords = point;
		_clipPoints[2].Set(true);
	}
	else {
		// All three clip points were already set, restart with the first one
		reset();
		_clipPoints[0]._coords = point;
		_clipPoints[0].Set(true);
	}

	update();
}

// RegisterableModule implementation
const std::string& Clipper::getName() const {
	static std::string _name(MODULE_CLIPPER);
	return _name;
}

const StringSet& Clipper::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void Clipper::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << "Clipper::initialiseModule called\n";

	_useCaulk = registry::getValue<bool>(RKEY_CLIPPER_USE_CAULK);
	_caulkShader = GlobalRegistry().get(RKEY_CLIPPER_CAULK_SHADER);

	GlobalRegistry().signalForKey(RKEY_CLIPPER_USE_CAULK).connect(
        sigc::mem_fun(this, &Clipper::keyChanged)
    );
	GlobalRegistry().signalForKey(RKEY_CLIPPER_CAULK_SHADER).connect(
        sigc::mem_fun(this, &Clipper::keyChanged)
    );

	constructPreferences();

	// Register the clip commands
	GlobalCommandSystem().addCommand("ClipSelected", std::bind(&Clipper::clipSelectionCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("SplitSelected", std::bind(&Clipper::splitSelectedCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("FlipClip", std::bind(&Clipper::flipClipperCmd, this, std::placeholders::_1));
}

void Clipper::clipSelectionCmd(const cmd::ArgumentList& args) {
	if (clipMode()) {
		UndoableCommand undo("clipperClip");
		clip();
	}
}

void Clipper::splitSelectedCmd(const cmd::ArgumentList& args) {
	if (clipMode()) {
		UndoableCommand undo("clipperSplit");
		splitClip();
	}
}

void Clipper::flipClipperCmd(const cmd::ArgumentList& args) {
	flipClip();
}

// Define the static Clipper module
module::StaticModuleRegistration<Clipper> clipperModule;
