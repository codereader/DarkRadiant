#include "MissionInfoGuiView.h"

#include "math/Vector4.h"

namespace ui
{

MissionInfoGuiView::MissionInfoGuiView(wxWindow* parent) :
	GuiView(parent)
{}

void MissionInfoGuiView::setGLViewPort()
{
	double width = _windowDims[0];
	double height = _windowDims[1];

	double aspectRatio = _bgDims[0] / _bgDims[1];

	if (width / height > aspectRatio)
	{
		width = height * aspectRatio;
	}
	else
	{
		height = width / aspectRatio;
	}

	SetSize(static_cast<int>(width), -1);

	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void MissionInfoGuiView::setGui(const gui::IGuiPtr& gui)
{
	// Call the base class first
	GuiView::setGui(gui);

	Vector2 topLeft(0, 0);
	Vector2 bottomRight(640, 480);

	if (_gui != NULL)
	{
		gui::IGuiWindowDefPtr bgWindowDef = _gui->findWindowDef("ModToInstallParent");

		if (bgWindowDef)
		{
			const Vector4& rect = bgWindowDef->rect;
			topLeft = Vector2(rect[0], rect[1]);
			bottomRight = Vector2(rect[0] + rect[2], rect[1] + rect[3]);
		}
	}

	_bgDims = bottomRight - topLeft;

	_renderer.setVisibleArea(topLeft, bottomRight);

	// Only draw a certain windowDef
	setWindowDefFilter("ModToInstallParent");
}

void MissionInfoGuiView::setMissionInfoFile(const map::DarkmodTxtPtr& file)
{
	_file = file;
}

void MissionInfoGuiView::update()
{
	const gui::IGuiPtr& gui = getGui();

	if (!_file || !gui) return;

	gui->findWindowDef("modTitle")->setText(_file->getTitle());
	gui->findWindowDef("modDescription")->setText(_file->getDescription());
	gui->findWindowDef("modAuthor")->setText(_file->getAuthor());

	// These are internationalised strings in the GUI code, let's hardcode some for the preview
	gui->findWindowDef("modLastPlayedTitle")->setText("Last played:");
	gui->findWindowDef("modCompletedTitle")->setText("Completed:");
	gui->findWindowDef("modLastPlayedValue")->setText("2017-11-19");
	gui->findWindowDef("modCompletedValue")->setText("2017-11-26");
	gui->findWindowDef("modSizeTitle")->setText("Space used:");
	gui->findWindowDef("modSizeValue")->setText("123 MB");
	gui->findWindowDef("modSizeEraseFromDiskAction")->setText("[Erase from disk]");
	
	gui->findWindowDef("modLoadN")->setText("Install Mission");
	gui->findWindowDef("modLoadH")->setText("Install Mission");
	gui->findWindowDef("modLoad")->setText("Install Mission");
	gui->findWindowDef("moreInfoH")->setText("Notes");
	gui->findWindowDef("moreInfoN")->setText("Notes");
	gui->findWindowDef("moreInfo")->setText("Notes");

	redraw();
}

} // namespace
