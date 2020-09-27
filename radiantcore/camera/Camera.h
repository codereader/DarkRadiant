#pragma once

#include "icameraview.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "generic/callback.h"

namespace camera
{

class Camera :
	public ICameraView
{
	static Vector3 _prevOrigin;
	static Vector3 _prevAngles;

	Vector3 _origin;
	Vector3 _angles;

	Callback _queueDraw;
	Callback _forceRedraw;

	float _fieldOfView;
	float _farClipPlane;
	int _width;
	int _height;

	Vector3 _forward, _right; // move matrix (TTimo: used to have up but it was not updated)
	Vector3 _vup, _vpn, _vright; // view matrix (taken from the modelview matrix)

	Matrix4 _projection;
	Matrix4 _modelview;

	render::IRenderView& _view;

public:
	Camera(render::IRenderView& view, const Callback& queueDraw, const Callback& forceRedraw);
	Camera(const Camera& other) = delete;
	Camera& operator=(const Camera& other) = delete;

	void updateVectors();
	void updateModelview();

	float getFarClipPlaneDistance() const override;
	void setFarClipPlaneDistance(float distance) override;

	void freemoveUpdateAxes();
	void moveUpdateAxes();

	const Vector3& getCameraOrigin() const override;
	void setCameraOrigin(const Vector3& newOrigin) override;

	const Vector3& getCameraAngles() const override;
	void setCameraAngles(const Vector3& newAngles) override;

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

	void queueDraw() override;
	void forceRedraw() override;

private:
	void updateProjection();
};

} // namespace
