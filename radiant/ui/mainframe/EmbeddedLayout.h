#pragma once

#include "wxutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"
#include "ui/widgets/Splitter.h"

namespace ui
{

#define EMBEDDED_LAYOUT_NAME "Embedded"

class EmbeddedLayout;
typedef std::shared_ptr<EmbeddedLayout> EmbeddedLayoutPtr;

class EmbeddedLayout :
	public IMainFrameLayout
{
private:
	// The camera view
	CamWndPtr _camWnd;

	Splitter* _horizPane;
	Splitter* _groupCamPane;

public:
	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;

	// The creation function, needed by the mainframe layout manager
	static EmbeddedLayoutPtr CreateInstance();

private:
	void maximiseCameraSize();
	void restorePanePositions();
};

} // namespace ui
