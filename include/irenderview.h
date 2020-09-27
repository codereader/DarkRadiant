#pragma once

#include "ivolumetest.h"
#include <cstddef>
#include <string>

class Frustum;

namespace render
{

// Interface used by camera views, extending the VolumeTest base
// by some accessors needed to set up and run an openGL render pass.
class IRenderView :
	public VolumeTest
{
public:
	virtual ~IRenderView() {}

	virtual void construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height) = 0;

	virtual const Vector3& getViewer() const = 0;

	virtual const Frustum& getFrustum() const = 0;

	virtual std::string getCullStats() const = 0;
};

}
