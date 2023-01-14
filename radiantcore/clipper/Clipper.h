#pragma once

#include "iclipper.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "ClipPoint.h"
#include "math/AABB.h"

namespace
{
	const unsigned int NUM_CLIP_POINTS = 3;
}

class Clipper final : 
	public IClipper
{
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

    // The plane defined by the clip points (might be invalid)
    Plane3 _clipPlane;

private:
	// Update the internally stored variables on registry key change
	void keyChanged();

public:
	// Constructor
	Clipper();

	void constructPreferences();

	EViewType getViewType() const override;
	void setViewType(EViewType viewType) override;
	ClipPoint* getMovingClip() override;

	Vector3& getMovingClipCoords() override;
	void setMovingClip(ClipPoint* clipPoint) override;

	bool useCaulkForNewFaces() const override;
	const std::string& getCaulkShader() const override;

	// greebo: Cycles through the three possible clip points and returns the nearest to point (for selectiontest)
	ClipPoint* find(const Vector3& point, EViewType viewtype, float scale) override;

	// Returns true if at least two clip points are set
	bool valid() const;
	void draw(float scale) override;
	void getPlanePoints(Vector3 planepts[3], const AABB& bounds) const;

    const Plane3& getClipPlane() override;
	void setClipPlane(const Plane3& plane);

	void update() override;
	void flipClip() override;
	void reset();
	void clip() override;

	void splitClip() override;
	bool clipMode() const override;
	void onClipMode(bool enabled) override;
	void newClipPoint(const Vector3& point) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

	// Command targets
	void clipSelectionCmd();
	void splitSelectedCmd();

}; // class Clipper

