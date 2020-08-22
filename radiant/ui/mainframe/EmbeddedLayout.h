#pragma once

#include <memory>
#include "wxutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"
#include "wxutil/Splitter.h"

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

	wxutil::Splitter* _horizPane;
	wxutil::Splitter* _groupCamPane;

	struct SavedPositions
	{
		int horizSashPosition;
		int groupCamSashPosition;
	};
	std::unique_ptr<SavedPositions> _savedPositions;

public:
	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;

	// The creation function, needed by the mainframe layout manager
	static EmbeddedLayoutPtr CreateInstance();
};

} // namespace ui
