#include "MD5AnimationChooser.h"

namespace ui
{

MD5AnimationChooser::MD5AnimationChooser(wxWindow* parent) :
	MD5AnimationViewer(parent, RunMode::Selection)
{}

MD5AnimationChooser::Result MD5AnimationChooser::runDialog(const std::string& preselectModel,
	const std::string& preselectAnim)
{
	MD5AnimationChooser::Result result;

	if (!preselectModel.empty())
	{
		setSelectedModel(preselectModel);
	}

	if (!preselectAnim.empty())
	{
		setSelectedAnim(preselectAnim);
	}

	if (MD5AnimationViewer::ShowModal() == wxID_OK)
	{
		result.model = getSelectedModel();
		result.anim = getSelectedAnim();
	}
	else
	{
		result.model.clear();
		result.anim.clear();
	}

	return result;
}

void MD5AnimationChooser::destroyDialog()
{
	Destroy();
}

}
