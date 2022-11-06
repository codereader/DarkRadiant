#pragma once

#include "icameraview.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "generic/callback.h"
#include "registry/CachedKey.h"

namespace camera
{

class Camera :
	public ICameraView
{
	static Vector3 _prevOrigin;
	static Vector3 _prevAngles;

	Vector3 _origin;
	Vector3 _angles;

	std::function<void(bool)> _requestRedraw;

	float _fieldOfView;
	float _farClipPlane;
	bool _farClipPlaneEnabled;
	int _width;
	int _height;

	Vector3 _forward, _right; // move matrix (TTimo: used to have up but it was not updated)
	Vector3 _vup, _vpn, _vright; // view matrix (taken from the modelview matrix)

	Matrix4 _projection;
	Matrix4 _modelview;

	render::IRenderView& _view;

    registry::CachedKey<bool> _dragSelectionEnabled;

public:
	Camera(render::IRenderView& view, const std::function<void(bool)>& requestRedraw);
	Camera(const Camera& other) = delete;
	Camera& operator=(const Camera& other) = delete;

	void updateVectors();
	void updateModelview();

	float getFarClipPlaneDistance() const override;
	void setFarClipPlaneDistance(float distance) override;
    bool getFarClipPlaneEnabled() const override;
    void setFarClipPlaneEnabled(bool enabled) override;

	void freemoveUpdateAxes();
	void moveUpdateAxes();

	const Vector3& getCameraOrigin() const override;
	void setCameraOrigin(const Vector3& newOrigin) override;

	const Vector3& getCameraAngles() const override;
	void setCameraAngles(const Vector3& newAngles) override;

	void setOriginAndAngles(const Vector3& newOrigin, const Vector3& newAngles) override;

	const Vector3& getRightVector() const override;
	const Vector3& getUpVector() const override;
	const Vector3& getForwardVector() const override;

	const Matrix4& getModelView() const override;
	const Matrix4& getProjection() const override;

	int getDeviceWidth() const override;
	int getDeviceHeight() const override;
	void setDeviceDimensions(int width, int height) override;

	SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
	const VolumeTest& getVolumeTest() const override;
    bool supportsDragSelections() override;
	void queueDraw() override;
	void forceRedraw() override;

private:
	// Set the origin without triggering any nofifications
	void doSetOrigin(const Vector3& origin, bool updateModelView);
	void doSetAngles(const Vector3& angles, bool updateModelView);
	void updateProjection();
};

} // namespace
