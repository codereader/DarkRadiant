#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/Dialog.h"

namespace ui
{

class CreateTrimDialog :
	public wxutil::Dialog
{
public:
	enum class FitTo { Bottom, Top, Left, Right };

	struct TrimParams
	{
		double height;
		double depth;
		FitTo fitTo;
		bool mitered;
	};

private:
	Handle _heightHandle;
	Handle _depthHandle;
	Handle _fitToHandle;
	Handle _miteredHandle;

public:
	CreateTrimDialog();

	static bool QueryTrimParams(TrimParams& params);

	static void CreateTrimCmd(const cmd::ArgumentList& args);
};

} // namespace ui
