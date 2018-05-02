#include "MeasurementTool.h"

#include "i18n.h"
#include "iclipper.h"
#include "iuimanager.h"
#include "math/Matrix4.h"
#include "XYMouseToolEvent.h"
#include "selectionlib.h"
#include "string/convert.h"
#include <fmt/format.h>

namespace ui
{

MeasurementTool::MeasurementTool() :
	_points(GL_POINTS),
	_lines(GL_LINE_STRIP)
{}

const std::string& MeasurementTool::getName()
{
    static std::string name("MeasurementTool");
    return name;
}

const std::string& MeasurementTool::getDisplayName()
{
    static std::string displayName(_("Measure"));
    return displayName;
}

MouseTool::Result MeasurementTool::onMouseDown(Event& ev)
{
    try
    {
        if (GlobalClipper().clipMode())
        {
            return Result::Ignored; // no measurement in clip mode
        }

        // We only operate on XY view events, so attempt to cast
        XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

		Vector3 clickPos = xyEvent.getWorldPos();
		xyEvent.getView().snapToGrid(clickPos);

		assert(_lines.size() == _points.size());

		VertexCb clickVertex(clickPos, Colour4b());

		if (_points.empty())
		{
			// If we just started, allocate two points
			_points.push_back(clickVertex);
			_points.push_back(clickVertex);

			// Copy the vertices to the lines vector
			_lines.push_back(clickVertex);
			_lines.push_back(clickVertex);
		}
		else
		{
			// Store the click position and add a new vertex
			_points[_points.size() - 1].vertex = clickVertex.vertex;
			_lines[_lines.size() - 1].vertex = clickVertex.vertex;

			// Add one additional point to the chain
			_points.push_back(clickVertex);
			_lines.push_back(clickVertex);
		}

		return Result::Activated;
    }
    catch (std::bad_cast&)
    {
    }

    return Result::Ignored; // not handled
}

MouseTool::Result MeasurementTool::onMouseMove(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

        Vector3 endPos = xyEvent.getWorldPos();
        xyEvent.getView().snapToGrid(endPos);

		assert(!_points.empty());

		_points[_points.size() - 1].vertex = endPos;
		_points.setColour(Colour4b(0, 0, 0, 0));

		_lines[_lines.size() -1] = _points[_points.size() - 1];
    }
    catch (std::bad_cast&)
    {
        return Result::Ignored;
    }

    return Result::Continued;
}

MouseTool::Result MeasurementTool::onMouseUp(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        dynamic_cast<XYMouseToolEvent&>(ev).getScale();

		// We continue until the user hits ESC
        return Result::Continued;
    }
    catch (std::bad_cast&)
    {
        return Result::Ignored;
    }
}

MeasurementTool::Result MeasurementTool::onCancel(IInteractiveView& view)
{
	_points.clear();
	_lines.clear();

    return Result::Finished;
}

void MeasurementTool::onMouseCaptureLost(IInteractiveView& view)
{
    onCancel(view);
}

unsigned int MeasurementTool::getPointerMode()
{
    return PointerMode::Normal;
}

unsigned int MeasurementTool::getRefreshMode()
{
    return RefreshMode::Force | RefreshMode::ActiveView;
}

void MeasurementTool::ensureShaders(RenderSystem& renderSystem)
{
	if (!_wireShader)
	{
		Vector3 colour = ColourSchemes().getColour("drag_selection");

		_colour.x() = colour.x();
		_colour.y() = colour.y();
		_colour.z() = colour.z();
		_colour.w() = 1;

		_wireShader = renderSystem.capture(fmt::format("<{0:f} {1:f} {2:f}>", _colour[0], _colour[1], _colour[2]));
	}

	if (!_pointShader)
	{
		_pointShader = renderSystem.capture("$POINT");
	}
}

void MeasurementTool::render(RenderSystem& renderSystem, RenderableCollector& collector, const VolumeTest& volume)
{
	ensureShaders(renderSystem);

	// Render lines
	collector.addRenderable(_wireShader, _lines, Matrix4::getIdentity());

	// Render points
	collector.addRenderable(_pointShader, _points, Matrix4::getIdentity());

	// Render distance string
	for (std::size_t i = 1; i < _points.size(); ++i)
	{
		const Vector3& a = _points[i-1].vertex;
		const Vector3& b = _points[i].vertex;

		glColor4dv(_colour);
		glRasterPos3dv((a+b)*0.5);
		GlobalOpenGL().drawString(string::to_string((a-b).getLength()));
	}
}

} // namespace
