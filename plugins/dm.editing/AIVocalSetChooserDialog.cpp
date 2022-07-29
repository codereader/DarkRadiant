#include "AIVocalSetChooserDialog.h"

#include "i18n.h"
#include "ieclass.h"
#include "isound.h"

#include "eclass.h"

#include <wx/stattext.h>
#include <wx/sizer.h>

#include "ifavourites.h"
#include "wxutil/Bitmap.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "ThreadedEntityDefPopulator.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose AI Vocal Set");
}

class ThreadedVocalSetLoader :
    public ThreadedEntityDefPopulator
{
public:
    ThreadedVocalSetLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedEntityDefPopulator(columns, "icon_sound.png")
    {}

protected:
    bool ClassShouldBeListed(const IEntityClassPtr& eclass) override
    {
        return eclass->getAttributeValue("editor_vocal_set") == "1";
    }
};

AIVocalSetChooserDialog::AIVocalSetChooserDialog() :
	DialogBase(_(WINDOW_TITLE)),
	_preview(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_preview = new AIVocalSetPreview(this);
	}

    _setView = new wxutil::DeclarationTreeView(this, decl::Type::EntityDef, _columns, wxDV_NO_HEADER);
	_setView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &AIVocalSetChooserDialog::onSetSelectionChanged, this);

	// Head Name column
	_setView->AppendIconTextColumn("", _columns.iconAndName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Allow searching for the name
	_setView->AddSearchColumn(_columns.leafName);

	auto vbox1 = new wxBoxSizer(wxVERTICAL);

	auto label1 = new wxStaticText(this, wxID_ANY, _("Available Sets"));
	label1->SetFont(label1->GetFont().Bold());

    auto treeViewToolbar = new wxutil::ResourceTreeViewToolbar(this, _setView);

	vbox1->Add(label1, 0, wxBOTTOM, 6);
    vbox1->Add(treeViewToolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	vbox1->Add(_setView, 1, wxEXPAND);

	// Right: the description
	auto vbox2 = new wxBoxSizer(wxVERTICAL);

	auto label2 = new wxStaticText(this, wxID_ANY, _("Description"));
	label2->SetFont(label2->GetFont().Bold());

	_description = new wxTextCtrl(this, wxID_ANY, "", 
		wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
	_description->SetMinClientSize(wxSize(-1, 60));

	vbox2->Add(label2, 0, wxBOTTOM, 6);
	vbox2->Add(_description, 1, wxEXPAND | wxBOTTOM, 6);

	// Right: the preview control panel
	if (_preview)
	{
		vbox2->Add(_preview, 0, wxEXPAND);
	}

	// dialog hbox, left is the treeview, right is the preview panel
	auto hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(vbox1, 1, wxEXPAND | wxRIGHT, 6);
	hbox->Add(vbox2, 1, wxEXPAND | wxRIGHT, 6);

	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	FitToScreen(0.7f, 0.6f);

	// Load the found sets into the GtkListStore
	populateSetStore();

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &AIVocalSetChooserDialog::_onItemActivated, this);
}

void AIVocalSetChooserDialog::_onItemActivated(wxDataViewEvent& ev)
{
    EndModal(wxID_OK);
}

void AIVocalSetChooserDialog::setSelectedVocalSet(const std::string& setName)
{
    _setView->SetSelectedDeclName(setName);
}

std::string AIVocalSetChooserDialog::getSelectedVocalSet()
{
	return _setView->GetSelectedDeclName();
}

void AIVocalSetChooserDialog::handleSetSelectionChanged()
{
    _selectedSet = _setView->GetSelectedDeclName();

    // Update sensitivity
    FindWindowById(wxID_OK, this)->Enable(!_selectedSet.empty());
    _description->Enable(!_selectedSet.empty());
    
    if (!_selectedSet.empty())
	{
		// Lookup the IEntityClass instance
		if (auto ecls = GlobalEntityClassManager().findClass(_selectedSet); ecls)
		{
			// Update the preview pane
			if (_preview)
			{
				_preview->setVocalSetEclass(ecls);
			}

			// Update the usage panel
			_description->SetValue(eclass::getUsage(ecls));
		}
	}
	else
	{
		if (_preview)
		{
			_preview->setVocalSetEclass(IEntityClassPtr());
		}
	}
}

void AIVocalSetChooserDialog::onSetSelectionChanged(wxDataViewEvent& ev)
{
	handleSetSelectionChanged();
}

void AIVocalSetChooserDialog::populateSetStore()
{
    _setView->Populate(std::make_shared<ThreadedVocalSetLoader>(_columns));
}

} // namespace
