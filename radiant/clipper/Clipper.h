#ifndef CLIPPER_H
#define CLIPPER_H

#include "iclipper.h"
#include "iregistry.h"
#include "ClipPoint.h"
#include "math/aabb.h"

namespace {
	const unsigned int NUM_CLIP_POINTS = 3;
}

class Clipper : 
	public IClipper,
	public RegistryKeyObserver
{
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
	Clipper();

	// Update the internally stored variables on registry key change
	void keyChanged(const std::string& key, const std::string& val);

	void constructPreferences();
	
	EViewType getViewType() const;
	void setViewType(EViewType viewType);
	ClipPoint* getMovingClip();
	
	Vector3& getMovingClipCoords();
	void setMovingClip(ClipPoint* clipPoint);
	
	const std::string getShader() const;
	
	// greebo: Cycles through the three possible clip points and returns the nearest to point (for selectiontest)
	ClipPoint* find(const Vector3& point, EViewType viewtype, float scale);
	
	// Returns true if at least two clip points are set
	bool valid() const;
	void draw(float scale);
	void getPlanePoints(Vector3 planepts[3], const AABB& bounds) const;
	
	void splitBrushes(const Vector3& p0, 
					const Vector3& p1, const Vector3& p2, 
					const std::string& shader, EBrushSplit split);
	void setClipPlane(const Plane3& plane);
	
	void update();
	void flipClip();
	void reset();
	void clip();
	
	void splitClip();
	bool clipMode() const;
	void onClipMode(bool enabled);
	void newClipPoint(const Vector3& point);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

}; // class Clipper

#endif /* CLIPPER_H */
