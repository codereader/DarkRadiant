#pragma once

#include "imousetool.h"
#include "i18n.h"
#include "../GlobalCamera.h"
#include "../CameraSettings.h"

namespace ui
{

class PanViewTool :
	public MouseTool
{
public:
	const std::string& getName() override
	{
		static std::string name("PanViewTool");
		return name;
	}

	const std::string& getDisplayName() override
	{
		static std::string displayName(_("Pan Camera View"));
		return displayName;
	}

	unsigned int getPointerMode() override
	{
		return PointerMode::Capture | PointerMode::Freeze |
			PointerMode::Hidden | PointerMode::MotionDeltas;
	}

	Result onMouseDown(Event& ev) override
	{
		try
		{
			CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

			// Don't operate when the camera is already in free look mode
			if (camEvent.getView().freeMoveEnabled())
			{
				return Result::Ignored;
			}

			return Result::Activated; 
		}
		catch (std::bad_cast&)
		{
		}

		return Result::Ignored; // not handled
	}

	Result onMouseMove(Event& ev) override
	{
		try
		{
			// We use capture mode, so xy event will contain the delta only
			CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);
			ICameraView& view = camEvent.getView();

			const float strafespeed = GlobalCamera().getCameraStrafeSpeed();

			double dx = camEvent.getDeviceDelta().x();
			double dy = camEvent.getDeviceDelta().y();

			Vector3 delta(0, 0, 0);

			delta += view.getRightVector() * strafespeed * dx;
			delta += view.getUpVector() * strafespeed * dy * (-1); // invert y direction

			view.setCameraOrigin(view.getCameraOrigin() + delta);

			return Result::Continued;
		}
		catch (std::bad_cast&)
		{
		}

		return Result::Ignored;
	}

	Result onMouseUp(Event& ev) override
	{
		return Result::Finished;
	}
};

}
