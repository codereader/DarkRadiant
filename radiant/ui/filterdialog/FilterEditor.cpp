#include "FilterEditor.h"

#include "i18n.h"

#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "shaderlib.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE_EDIT = N_("Edit Filter");
    const char* const WINDOW_TITLE_VIEW = N_("View Filter");

    enum Columns
    {
        INDEX,          // integer
        //TYPE,           // integer
        TYPE_STRING,    // string
        ENTITY_KEY,     // string
        MATCH,          // string
        SHOWHIDE,       // string (representing boolean)
        N_COLUMNS
    };

    enum
    {
        WIDGET_NAME_ENTRY,
        WIDGET_ADD_RULE_BUTTON,
        WIDGET_MOVE_RULE_UP_BUTTON,
        WIDGET_MOVE_RULE_DOWN_BUTTON,
        WIDGET_DELETE_RULE_BUTTON,
        WIDGET_HELP_TEXT,
    };
}

FilterEditor::FilterEditor(Filter& filter, wxWindow* parent, bool viewOnly) :
    DialogBase(viewOnly ? _(WINDOW_TITLE_VIEW) : _(WINDOW_TITLE_EDIT), parent),
    _originalFilter(filter),
    _filter(_originalFilter), // copy-construct
    _viewOnly(viewOnly)
{
    // Create the widgets
    populateWindow();

    // Update the widget contents
    update();

    FitToScreen(0.66f, 0.4f);
}

void FilterEditor::populateWindow()
{
    loadNamedPanel(this, "FilterEditorMainPanel");

    // Create the name entry box
    wxStaticText* topLabel = findNamedObject<wxStaticText>(this, "FilterEditorTopLabel");
    topLabel->SetFont(topLabel->GetFont().Bold());

    wxStaticText* ruleLabel = findNamedObject<wxStaticText>(this, "FilterEditorRuleLabel");
    ruleLabel->SetFont(ruleLabel->GetFont().Bold());

    wxTextCtrl* nameEntry = findNamedObject<wxTextCtrl>(this, "FilterEditorNameEntry");
    nameEntry->Connect(wxEVT_TEXT, wxCommandEventHandler(FilterEditor::onNameEdited), NULL, this);

    createCriteriaPanel();

    // Add the help text
    if (_viewOnly)
    {
        findNamedObject<wxStaticText>(this, "FilterEditorHelpText")->Hide();
    }

    wxButton* okButton = findNamedObject<wxButton>(this, "FilterEditorOkButton");
    wxButton* cancelButton = findNamedObject<wxButton>(this, "FilterEditorCancelButton");
    wxButton* saveButton = findNamedObject<wxButton>(this, "FilterEditorSaveButton");

    okButton->Show(_viewOnly);
    cancelButton->Show(!_viewOnly);
    saveButton->Show(!_viewOnly);

    // Connect the OK button to the "CANCEL" event
    okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onCancel), NULL, this);

    saveButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onSave), NULL, this);
    cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onCancel), NULL, this);
}

void FilterEditor::update()
{
    // Avoid callback loops
    _updateActive = true;

    // Populate the criteria store
    _ruleList->DeleteAllItems();

    _selectedRule = -1;

    // Traverse the criteria of the Filter to be edited
    for (std::size_t i = 0; i < _filter.rules.size(); ++i)
    {
        const FilterRule& rule = _filter.rules[i];

        // Allocate a new list store item
        wxVector<wxVariant> row(Columns::N_COLUMNS);

        row[Columns::INDEX] = wxVariant(std::to_string(i));
        //row[_columns.type] = static_cast<int>(rule.type);
        row[Columns::TYPE_STRING] = getStringForType(rule.type);
        row[Columns::MATCH] = rule.match;
        row[Columns::ENTITY_KEY] = rule.entityKey;
        row[Columns::SHOWHIDE] = rule.show ? std::string(_("show")) : std::string(_("hide"));

        _ruleList->AppendItem(row);
    }

    findNamedObject<wxTextCtrl>(this, "FilterEditorNameEntry")->SetValue(_filter.name);

    updateWidgetSensitivity();

    _updateActive = false;
}

void FilterEditor::createCriteriaPanel()
{
    // Create a new treeview
    wxPanel* parent = findNamedObject<wxPanel>(this, "FilterEditorTreeViewPanel");

    _ruleList = new wxDataViewListCtrl(parent, wxID_ANY);
    parent->GetSizer()->Add(_ruleList, 1, wxEXPAND | wxLEFT, 12);

    static const int FLAGS = wxDATAVIEW_COL_SORTABLE
                           | wxDATAVIEW_COL_RESIZABLE
                           | wxDATAVIEW_COL_REORDERABLE;

    // INDEX (col 0)
    _ruleList->AppendTextColumn(_("Index"), wxDATAVIEW_CELL_INERT,
                                -1, wxALIGN_CENTER, FLAGS);

    // TYPE_STRING (col 1)
    wxArrayString typeChoices;
    typeChoices.Add("entityclass");
    typeChoices.Add("texture");
    typeChoices.Add("object");
    typeChoices.Add("entitykeyvalue");

    wxDataViewChoiceRenderer* typeChoiceRenderer = 
        new wxDataViewChoiceRenderer(typeChoices, wxDATAVIEW_CELL_EDITABLE, wxALIGN_RIGHT);
    
    wxDataViewColumn* typeColumn = new wxDataViewColumn(
        _("Type"), typeChoiceRenderer, Columns::TYPE_STRING,
        -1, wxALIGN_LEFT,
        wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE
    );
    
    _ruleList->AppendColumn(typeColumn);

    // ENTITY_KEY (col 2)
    _ruleList->AppendTextColumn(_("Entity Key"), wxDATAVIEW_CELL_EDITABLE,
                                -1, wxALIGN_NOT, FLAGS);

    // MATCH (col 3)
    _ruleList->AppendTextColumn(_("Match"), wxDATAVIEW_CELL_EDITABLE,
                                -1, wxALIGN_NOT, FLAGS);

    // SHOWHIDE (col 4)
    wxArrayString actionChoices;
    actionChoices.Add(_("show"));
    actionChoices.Add(_("hide"));

    wxDataViewChoiceRenderer* actionChoiceRenderer = new wxDataViewChoiceRenderer(
        actionChoices, wxDATAVIEW_CELL_EDITABLE, wxALIGN_LEFT
    );
    
    wxDataViewColumn* actionColumn = new wxDataViewColumn(
        _("Action"), actionChoiceRenderer, Columns::SHOWHIDE,
        -1, wxALIGN_LEFT, FLAGS
    );
    
    _ruleList->AppendColumn(actionColumn);

    // Get notified when the user edits an item
    _ruleList->Connect(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                       wxDataViewEventHandler(FilterEditor::onItemEdited),
                       NULL, this);
    _ruleList->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
                       wxDataViewEventHandler(FilterEditor::onRuleSelectionChanged),
                       NULL, this);

    // Action buttons
    wxButton* addRuleButton =       findNamedObject<wxButton>(this, "FilterEditorAddButton");
    wxButton* moveRuleUpButton =    findNamedObject<wxButton>(this, "FilterEditorUpButton");
    wxButton* moveRuleDownButton =  findNamedObject<wxButton>(this, "FilterEditorDownButton");
    wxButton* deleteRuleButton =    findNamedObject<wxButton>(this, "FilterEditorDeleteButton");

    addRuleButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onAddRule), NULL, this);
    moveRuleUpButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onMoveRuleUp), NULL, this);
    moveRuleDownButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onMoveRuleDown), NULL, this);
    deleteRuleButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterEditor::onDeleteRule), NULL, this);
}

std::string FilterEditor::getStringForType(const FilterRule::Type type)
{
    switch (type)
    {
    case FilterRule::TYPE_TEXTURE: return "texture";
    case FilterRule::TYPE_OBJECT: return "object";
    case FilterRule::TYPE_ENTITYCLASS: return "entityclass";
    case FilterRule::TYPE_ENTITYKEYVALUE: return "entitykeyvalue";
    default: return "";
    };
}

FilterRule::Type FilterEditor::getTypeForString(const std::string& typeStr)
{
    if (typeStr == "texture")
    {
        return FilterRule::TYPE_TEXTURE;
    }
    else if (typeStr == "object")
    {
        return FilterRule::TYPE_OBJECT;
    }
    else if (typeStr == "entityclass")
    {
        return FilterRule::TYPE_ENTITYCLASS;
    }
    else // "entitykeyvalue"
    {
        return FilterRule::TYPE_ENTITYKEYVALUE;
    }
}

void FilterEditor::save()
{
    _filter.name = findNamedObject<wxTextCtrl>(this, "FilterEditorNameEntry")->GetValue();

    // Copy the working set over the actual Filter
    _originalFilter = _filter;
}

void FilterEditor::updateWidgetSensitivity() {

    if (_viewOnly)
    {
        findNamedObject<wxWindowBase>(this, "FilterEditorNameEntry")->Enable(false);

        _ruleList->Enable(false);

        findNamedObject<wxButton>(this, "FilterEditorAddButton")->Enable(false);
        findNamedObject<wxButton>(this, "FilterEditorUpButton")->Enable(false);
        findNamedObject<wxButton>(this, "FilterEditorDownButton")->Enable(false);
        findNamedObject<wxButton>(this, "FilterEditorDeleteButton")->Enable(false);

        return;
    }

    if (_selectedRule != -1)
    {
        bool lastSelected = (_selectedRule + 1 >= static_cast<int>(_filter.rules.size()) || _filter.rules.size() <= 1);
        bool firstSelected = (_selectedRule <= 0 || _filter.rules.size() <= 1);

        findNamedObject<wxButton>(this, "FilterEditorUpButton")->Enable(!firstSelected);
        findNamedObject<wxButton>(this, "FilterEditorDownButton")->Enable(!lastSelected);
        findNamedObject<wxButton>(this, "FilterEditorDeleteButton")->Enable(true);
    }
    else
    {
        // no rule selected
        findNamedObject<wxButton>(this, "FilterEditorUpButton")->Enable(false);
        findNamedObject<wxButton>(this, "FilterEditorDownButton")->Enable(false);
        findNamedObject<wxButton>(this, "FilterEditorDeleteButton")->Enable(false);
    }
}

void FilterEditor::onSave(wxCommandEvent& ev)
{
    save();
    EndModal(wxID_OK);
}

void FilterEditor::onCancel(wxCommandEvent& ev)
{
    EndModal(wxID_CANCEL);
}

void FilterEditor::onItemEdited(wxDataViewEvent& ev)
{
    // Determine which row and column was edited and retrieve the current value
    // (note that ev.GetValue() does not return anything on Linux).
    int row = _ruleList->ItemToRow(ev.GetItem());
    int col = ev.GetColumn();
    wxVariant value;
    _ruleList->GetValue(value, row, col);

    // The rule index is in column INDEX
    wxVariant ruleIndexV;
    _ruleList->GetValue(ruleIndexV, row, INDEX);
    int ruleIndex = ruleIndexV.GetLong();

    // Update the criterion
    assert(ruleIndex >= 0 && ruleIndex < static_cast<int>(_filter.rules.size()));

    if (col == MATCH)
    {
        _filter.rules[ruleIndex].match = value.GetString();
    }
    else if (col == ENTITY_KEY)
    {
        _filter.rules[ruleIndex].entityKey = value.GetString();
    }
    else if (col == TYPE_STRING)
    {
        // Look up the type index for "new_text"
        FilterRule::Type type = getTypeForString(std::string(value.GetString()));
        _filter.rules[ruleIndex].type = type;
    }
    else if (col == SHOWHIDE)
    {
        // Update the bool flag
        _filter.rules[ruleIndex].show = (value.GetString() == _("show"));
    }
}

void FilterEditor::onNameEdited(wxCommandEvent& ev)
{
    if (_updateActive) return;

    _filter.name = findNamedObject<wxTextCtrl>(this, "FilterEditorNameEntry")->GetValue();
}

void FilterEditor::onRuleSelectionChanged(wxDataViewEvent& ev)
{
    int selectedRow = _ruleList->GetSelectedRow();

    if (selectedRow != -1)
    {
        wxVariant value;
        _ruleList->GetValue(value, selectedRow, Columns::INDEX);
        _selectedRule = value.GetLong();
    }
    else
    {
        _selectedRule = -1;
    }

    updateWidgetSensitivity();
}

void FilterEditor::onAddRule(wxCommandEvent& ev)
{
    FilterRule newRule = FilterRule::Create(FilterRule::TYPE_TEXTURE, GlobalTexturePrefix_get(), false);
    _filter.rules.push_back(newRule);

    update();
}

void FilterEditor::onMoveRuleUp(wxCommandEvent& ev)
{
    if (_selectedRule >= 1)
    {
        FilterRule temp = _filter.rules[_selectedRule - 1];
        _filter.rules[_selectedRule - 1] = _filter.rules[_selectedRule];
        _filter.rules[_selectedRule] = temp;

        update();
    }
}

void FilterEditor::onMoveRuleDown(wxCommandEvent& ev)
{
    if (_selectedRule < static_cast<int>(_filter.rules.size()) - 1)
    {
        FilterRule temp = _filter.rules[_selectedRule + 1];
        _filter.rules[_selectedRule + 1] = _filter.rules[_selectedRule];
        _filter.rules[_selectedRule] = temp;

        update();
    }
}

void FilterEditor::onDeleteRule(wxCommandEvent& ev)
{
    if (_selectedRule != -1)
    {
        // Let the rules slip down one index each
        for (std::size_t i = _selectedRule; i+1 < _filter.rules.size(); ++i)
        {
            _filter.rules[i] = _filter.rules[i+1];
        }

        // Remove one item, it is the superfluous one now
        _filter.rules.pop_back();

        update();
    }
}

} // namespace ui
