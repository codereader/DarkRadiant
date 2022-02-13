#include "MeasurementTool.h"

#include "i18n.h"
#include "iclipper.h"
#include "icolourscheme.h"
#include "math/Matrix4.h"
#include "XYMouseToolEvent.h"
#include "selectionlib.h"
#include "string/convert.h"
#include <fmt/format.h>

namespace ui
{

MeasurementTool::MeasurementTool() :
	_points(_vertices),
	_line(_vertices)
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

		Vertex3f clickVertex(clickPos);

		if (_vertices.empty())
		{
			// If we just started, allocate two points
			_vertices.push_back(clickVertex);
            _vertices.push_back(clickVertex);
		}
		else
		{
			// Store the click position and add a new vertex
            _vertices[_vertices.size() - 1] = clickVertex;

			// Add one additional point to the chain
			_vertices.push_back(clickVertex);
		}

        _points.queueUpdate();
        _line.queueUpdate();

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

		assert(!_vertices.empty());

        _vertices[_vertices.size() - 1] = endPos;
        _line.setColour({ 0, 0, 0, 0 });
        _points.setColour({ 0, 0, 0, 0 });
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
	_vertices.clear();
    _line.clear();
    _points.clear();
    _texts.clear();

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
		Vector3 colour = GlobalColourSchemeManager().getColour("drag_selection");

		_colour.x() = colour.x();
		_colour.y() = colour.y();
		_colour.z() = colour.z();
		_colour.w() = 1;

		_wireShader = renderSystem.capture(ColourShaderType::OrthoviewSolid, _colour);
	}

	if (!_pointShader)
	{
		_pointShader = renderSystem.capture(BuiltInShaderType::Point);
	}

    if (!_textRenderer)
    {
        _textRenderer = renderSystem.captureTextRenderer(IGLFont::Style::Mono, 14);
    }
}

void MeasurementTool::render(RenderSystem& renderSystem, IRenderableCollector& collector, const VolumeTest& volume)
{
    ensureShaders(renderSystem);

    _points.update(_pointShader);
    _line.update(_wireShader);
    
    _texts.resize(_vertices.size() - 1);

    // Render distance string
    for (std::size_t i = 1; i < _vertices.size(); ++i)
    {
        auto& text = _texts.at(i - 1);

        if (!text)
        {
            text.reset(new render::StaticRenderableText("", { 0,0,0 }, _colour));
            text->update(_textRenderer);
        }

        const auto& a = _vertices[i - 1];
        const auto& b = _vertices[i];

        text->setColour(_colour);
        text->setText(string::to_string((a - b).getLength()));
        text->setWorldPosition((a + b) * 0.5);
    }
}

} // namespace
