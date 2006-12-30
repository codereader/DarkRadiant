#ifndef GLOBALCLIPPOINTS_H_
#define GLOBALCLIPPOINTS_H_

#include "iregistry.h"
#include "preferencesystem.h"

#include "ClipPoint.h"
#include "math/aabb.h"

#include "preferences.h"

	namespace {
		const unsigned int NUM_CLIP_POINTS = 3;
		
		const std::string RKEY_CLIPPER_CAULK_SHADER = "user/ui/clipper/caulkTexture";
		const std::string RKEY_CLIPPER_USE_CAULK = "user/ui/clipper/useCaulk";
	}

class ClipPointManager :
	public RegistryKeyObserver,
	public PreferenceConstructor
{
	// Hold the currently active xy view type
	VIEWTYPE _viewType;
	
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
	ClipPointManager();
	
	// The RegistryKeyObserver implementation, gets called when the observed keys change
	void keyChanged();
	
	// The PreferenceConstructor implementation, used to add the widgets to the dialog
	void constructPreferencePage(PreferenceGroup& group);
	
	VIEWTYPE getViewType() const;
	void setViewType(VIEWTYPE viewType);
	
	ClipPoint* getMovingClip();
	void setMovingClip(ClipPoint* clipPoint);
	
	// greebo: Cycles through the three possible clip points and returns the nearest to point (for selectiontest)
	// @returns: NULL, if no suitable point could be found
	ClipPoint* find(const Vector3& point, VIEWTYPE viewtype, float scale);
	
	// Draws the set clip points, the <scale> argument is needed for drawing the number id of the clip point 
	void draw(float scale);
	
	// Returns true if at least two clip points are set
	bool valid() const;
	
	bool clipMode() const;
	
	void getPlanePoints(Vector3 planepts[3], const AABB& bounds) const;
	
	// Resets all clip points to zero and their "set" property to false
	void reset();
	
	void update();
	
	// Returns the shader name of the texture that gets applied to the surfaces created by the clip operation 
	const std::string getShader() const;
	
	// Switches between front and back clip mode
	void flipClip();
		
	// Clips the selected brushes using the current clip point selection
	void clip();
	
	void splitClip();
	
	// This gets called when the clip mode gets activated/deactivated
	void onClipMode(bool enabled);
	
	// Adds the given clip point coordinates 
	void newClipPoint(const Vector3& point);
};

ClipPointManager* GlobalClipPoints();

#endif /*GLOBALCLIPPOINTS_H_*/
