
#include "iclipper.h"		// The Abstract Base Class
#include "iradiant.h"

#include "iregistry.h"
#include "ipreferencesystem.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "itexdef.h"

#include "debugging/debugging.h"
#include "math/aabb.h"

#include "ClipPoint.h"
#include "csg.h"
#include "ui/texturebrowser/TextureBrowser.h"

const unsigned int NUM_CLIP_POINTS = 3;
		
const std::string RKEY_CLIPPER_CAULK_SHADER = "user/ui/clipper/caulkTexture";
const std::string RKEY_CLIPPER_USE_CAULK = "user/ui/clipper/useCaulk";

class BrushClipper : 
	public Clipper,
	public RegistryKeyObserver
{
public:
	// Radiant Module stuff
	typedef Clipper Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	Clipper* getTable() {
		return this;
	}

private:

	// Hold the currently active xy view type
	EViewType _viewType;
	
	// The array holding the three possible clip points
	ClipPoint _clipPoints[NUM_CLIP_POINTS];
	
	// The pointer to the currently moved clip point
	ClipPoint* _movingClip;
	
	bool _switch;
	
	// Whether to use the _caulkShader texture for new brush faces
	bool _useCaulk;
	
	// The shader name used for new faces when _useCaulk is true
	std::string _caulkShader;
	
public:

	// Constructor
	BrushClipper() :
		_movingClip(NULL),
		_switch(true),
		_useCaulk(GlobalRegistry().get(RKEY_CLIPPER_USE_CAULK) == "1"),
		_caulkShader(GlobalRegistry().get(RKEY_CLIPPER_CAULK_SHADER))
	{
		GlobalRegistry().addKeyObserver(this, RKEY_CLIPPER_USE_CAULK);
		GlobalRegistry().addKeyObserver(this, RKEY_CLIPPER_CAULK_SHADER);
		
		constructPreferences();
	}

	// Update the internally stored variables on registry key change
	void keyChanged() {
		_caulkShader = GlobalRegistry().get(RKEY_CLIPPER_CAULK_SHADER);
		_useCaulk = (GlobalRegistry().get(RKEY_CLIPPER_USE_CAULK) == "1");
	}

	void constructPreferences() {
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Clipper");
		
		page->appendCheckBox("", "Clipper tool uses caulk texture", RKEY_CLIPPER_USE_CAULK);
		page->appendEntry("Caulk shader name", RKEY_CLIPPER_CAULK_SHADER);
	}
	
	EViewType getViewType() const {
		return _viewType;
	}
	
	void setViewType(EViewType viewType) {
		_viewType = viewType;
	}
	
	ClipPoint* getMovingClip() {
		return _movingClip;
	}
	
	Vector3& getMovingClipCoords() {
		// Check for NULL pointers, one never knows
		if (_movingClip != NULL) {
			return _movingClip->_coords;
		}
		
		// Return at least anything, i.e. the coordinates of the first clip point
		return _clipPoints[0]._coords;
	}
	
	void setMovingClip(ClipPoint* clipPoint) {
		_movingClip = clipPoint;
	}
	
	const std::string getShader() const {
		return (_useCaulk) ? _caulkShader : GlobalTextureBrowser().getSelectedShader();
	}
	
	// greebo: Cycles through the three possible clip points and returns the nearest to point (for selectiontest)
	ClipPoint* find(const Vector3& point, EViewType viewtype, float scale) {
		double bestDistance = FLT_MAX;
		
		ClipPoint* bestClip = NULL;
		
		for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
			_clipPoints[i].testSelect(point, viewtype, scale, bestDistance, bestClip);
		}
		
		return bestClip;
	}
	
	// Returns true if at least two clip points are set
	bool valid() const {
		return _clipPoints[0].isSet() && _clipPoints[1].isSet();
	}
	
	void draw(float scale) {
		// Draw clip points
		for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
			if (_clipPoints[i].isSet())
				_clipPoints[i].Draw(i, scale);
		}
	}
	
	void getPlanePoints(Vector3 planepts[3], const AABB& bounds) const {
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
	
	void splitBrushes(const Vector3& p0, const Vector3& p1, const Vector3& p2, const std::string& shader, EBrushSplit split) {
		Vector3 planePoints[3] = {p0, p1, p2};
		
		Scene_BrushSplitByPlane(planePoints, shader, split);
		GlobalRadiant().updateAllWindows();
	}
	
	void setClipPlane(const Plane3& plane) {
		Scene_BrushSetClipPlane(plane);
	}
	
	void update() {
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
		GlobalRadiant().updateAllWindows();
	}
	
	void flipClip() {
		_switch = !_switch;
		update();
		GlobalRadiant().updateAllWindows();
	}
	
	void reset() {
		for (unsigned int i = 0; i < NUM_CLIP_POINTS; i++) {
			_clipPoints[i].reset();
		}	
	}
	
	void clip() {
		if (clipMode() && valid()) {
			Vector3 planepts[3];
			AABB bounds(Vector3(0, 0, 0), Vector3(64, 64, 64));
			getPlanePoints(planepts, bounds);
			
			splitBrushes(planepts[0], planepts[1], planepts[2], getShader().c_str(), (!_switch) ? eFront : eBack);
			
			reset();
			update();
			GlobalRadiant().updateAllWindows();
		}
	}
	
	void splitClip() {
		if (clipMode() && valid()) {
			Vector3 planepts[3];
			AABB bounds(Vector3(0, 0, 0), Vector3(64, 64, 64));
			getPlanePoints(planepts, bounds);
			
			splitBrushes(planepts[0], planepts[1], planepts[2], getShader().c_str(), eFrontAndBack);
			
			reset();
			update();
			GlobalRadiant().updateAllWindows();
		}
	}
	
	bool clipMode() const {
		return GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip;
	}
	
	void onClipMode(bool enabled) {
		// Revert all clip points to <0,0,0> values
		reset();
		
		// Revert the _movingClip pointer, if clip mode to be disabled
		if (!enabled && _movingClip) {
			_movingClip = 0;
		}
		
		update();
		GlobalRadiant().updateAllWindows();
	}
	
	void newClipPoint(const Vector3& point) {
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
		GlobalRadiant().updateAllWindows();
	}

}; // class BrushClipper


/* BrushClipper dependencies class. 
 */
 
class BrushClipperDependencies :
	public GlobalRegistryModuleRef,
	public GlobalRadiantModuleRef,
	public GlobalPreferenceSystemModuleRef,
	public GlobalSelectionModuleRef,
	public GlobalSceneGraphModuleRef
{
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<BrushClipper, BrushClipperDependencies> BrushClipperModule;
typedef Static<BrushClipperModule> StaticBrushClipperModule;
StaticRegisterModule staticRegisterDefaultClipper(StaticBrushClipperModule::instance());
