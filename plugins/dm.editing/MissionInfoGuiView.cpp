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
		gui::IGuiWindowDefPtr bgWindowDef = _gui->findWindowDef(getTargetWindowDefName());

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
	setWindowDefFilter(getTargetWindowDefName());
}

// ---------- Darkmod.xt ----------------

DarkmodTxtGuiView::DarkmodTxtGuiView(wxWindow* parent) :
	MissionInfoGuiView(parent)
{}

void DarkmodTxtGuiView::setMissionInfoFile(const map::DarkmodTxtPtr& file)
{
	_file = file;
}

void DarkmodTxtGuiView::updateGuiState()
{
	const gui::IGuiPtr& gui = getGui();

	if (!_file || !gui) return;

	// This is a localised value, hardcode some for the moment being
	gui->setStateString("details_posx", "100");

	gui->findWindowDef("modTitle")->text.setValue(_file->getTitle());
	gui->findWindowDef("modDescription")->text.setValue(_file->getDescription());
	gui->findWindowDef("modAuthor")->text.setValue(_file->getAuthor());

	// These are internationalised strings in the GUI code, let's hardcode some for the preview
	gui->findWindowDef("modLastPlayedTitle")->text.setValue("Last played:");
	gui->findWindowDef("modCompletedTitle")->text.setValue("Completed:");
	gui->findWindowDef("modLastPlayedValue")->text.setValue("2017-11-19");
	gui->findWindowDef("modCompletedValue")->text.setValue("2017-11-26");
	gui->findWindowDef("modSizeTitle")->text.setValue("Space used:");
	gui->findWindowDef("modSizeValue")->text.setValue("123 MB");

    if (gui->findWindowDef("modSizeEraseFromDiskAction"))
    {
	    gui->findWindowDef("modSizeEraseFromDiskAction")->text.setValue("[Erase from disk]");
    }

	gui->findWindowDef("modLoadN")->text.setValue("Install Mission");
	gui->findWindowDef("modLoadH")->text.setValue("Install Mission");
	gui->findWindowDef("modLoad")->text.setValue("Install Mission");
	gui->findWindowDef("moreInfoH")->text.setValue("Notes");
	gui->findWindowDef("moreInfoN")->text.setValue("Notes");
	gui->findWindowDef("moreInfo")->text.setValue("Notes");

	redraw();
}

std::string DarkmodTxtGuiView::getTargetWindowDefName()
{
	return "ModToInstallParent";
}

// ---------- Readme.xt ----------------

ReadmeTxtGuiView::ReadmeTxtGuiView(wxWindow* parent) :
	MissionInfoGuiView(parent)
{}

void ReadmeTxtGuiView::setMissionInfoFile(const map::ReadmeTxtPtr& file)
{
	_file = file;
}

void ReadmeTxtGuiView::updateGuiState()
{
	const gui::IGuiPtr& gui = getGui();

	if (!_file || !gui) return;

	gui->setStateString("ModNotesText", _file->getContents());
	gui->findWindowDef("ModInstallationNotesButtonOK")->text.setValue("OK");

	redraw();
}

std::string ReadmeTxtGuiView::getTargetWindowDefName()
{
	return "ModInstallationNotesParchment";
}

} // namespace
