#include "TerrainGeneratorDialog.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "imap.h"
#include "ipatch.h"
#include "iselection.h"
#include "icameraview.h"
#include "iscenegraph.h"
#include "ishaderclipboard.h"
#include "iundo.h"

#include "string/convert.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "shaderlib.h"

#include <random>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/statbox.h>

#include "ui/materials/MaterialChooser.h"
#include "ui/materials/MaterialSelector.h"

namespace
{
const char* const WINDOW_TITLE = N_("Terrain Generator");

// Gets the active/selected shader or the default fallback value
inline std::string getSelectedShader()
{
    auto selectedShader = GlobalShaderClipboard().getShaderName();

    if (selectedShader.empty())
    {
        selectedShader = texdef_name_default();
    }

    return selectedShader;
}

// Get spawn position from selection center or camera
Vector3 getSpawnPosition()
{
    if (GlobalSelectionSystem().countSelected() > 0)
    {
        AABB bounds = GlobalSelectionSystem().getWorkZone().bounds;
        if (bounds.isValid())
        {
            return bounds.getOrigin();
        }
    }

    try
    {
        return GlobalCameraManager().getActiveView().getCameraOrigin();
    }
    catch (const std::runtime_error&)
    {
        // No active camera
    }

    return Vector3(0, 0, 0);
}
} // namespace

namespace ui
{

TerrainGeneratorDialog::TerrainGeneratorDialog()
    : Dialog(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow()), _fractalSizer(nullptr),
      _offsetLabel(nullptr), _offsetCtrl(nullptr)
{
    _dialog->GetSizer()->Add(
        loadNamedPanel(_dialog, "TerrainGeneratorMainPanel"), 1, wxEXPAND | wxALL, 12);

    wxStaticText* topLabel = findNamedObject<wxStaticText>(_dialog, "TerrainGeneratorTopLabel");
    topLabel->SetFont(topLabel->GetFont().Bold());

    // Get control references for visibility management
    _offsetLabel = findNamedObject<wxStaticText>(_dialog, "TerrainGeneratorOffsetLabel");
    _offsetCtrl = findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorOffset");

    // Bind events
    wxChoice* algorithmChoice = findNamedObject<wxChoice>(_dialog, "TerrainGeneratorAlgorithm");
    algorithmChoice->Bind(wxEVT_CHOICE, &TerrainGeneratorDialog::onAlgorithmChanged, this);

    wxButton* randomizeBtn = findNamedObject<wxButton>(_dialog, "TerrainGeneratorRandomizeSeed");
    randomizeBtn->Bind(wxEVT_BUTTON, &TerrainGeneratorDialog::onRandomizeSeed, this);

    wxButton* browseBtn = findNamedObject<wxButton>(_dialog, "TerrainGeneratorBrowseMaterial");
    browseBtn->Bind(wxEVT_BUTTON, &TerrainGeneratorDialog::onBrowseMaterial, this);

    // Set a random seed on init
    std::random_device rd;
    findNamedObject<wxSpinCtrl>(_dialog, "TerrainGeneratorSeed")->SetValue(rd() % 1000000000);

    findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorMaterial")->SetValue(getSelectedShader());

    updateControlVisibility();
}

void TerrainGeneratorDialog::onAlgorithmChanged(wxCommandEvent& ev)
{
    updateControlVisibility();
    _dialog->Layout();
    _dialog->Fit();
}

void TerrainGeneratorDialog::onRandomizeSeed(wxCommandEvent& ev)
{
    std::random_device rd;
    findNamedObject<wxSpinCtrl>(_dialog, "TerrainGeneratorSeed")->SetValue(rd() % 1000000000);
}

void TerrainGeneratorDialog::onBrowseMaterial(wxCommandEvent& ev)
{
    wxTextCtrl* materialEntry = findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorMaterial");
    MaterialChooser chooser(_dialog, MaterialSelector::TextureFilter::Regular, materialEntry);
    chooser.ShowModal();
}

void TerrainGeneratorDialog::updateControlVisibility()
{
    noise::Algorithm algo = getAlgorithm();

    // Show fractal parameters only for fBm and Ridged Multifractal
    bool showFractal =
        (algo == noise::Algorithm::FBm || algo == noise::Algorithm::RidgedMultifractal);

    // Show offset only for Ridged Multifractal
    bool showOffset = (algo == noise::Algorithm::RidgedMultifractal);

    // Show/hide fractal parameter controls
    findNamedObject<wxSpinCtrl>(_dialog, "TerrainGeneratorOctaves")->GetParent()->Show(showFractal);

    // Show/hide offset controls
    if (_offsetLabel)
    {
        _offsetLabel->Show(showOffset);
    }
    if (_offsetCtrl)
    {
        _offsetCtrl->Show(showOffset);
    }
}

noise::Algorithm TerrainGeneratorDialog::getAlgorithm()
{
    wxChoice* choice = findNamedObject<wxChoice>(_dialog, "TerrainGeneratorAlgorithm");
    return static_cast<noise::Algorithm>(choice->GetSelection());
}

std::size_t TerrainGeneratorDialog::getColumns()
{
    wxChoice* choice = findNamedObject<wxChoice>(_dialog, "TerrainGeneratorColumns");
    return string::convert<std::size_t>(choice->GetStringSelection().ToStdString(), 11);
}

std::size_t TerrainGeneratorDialog::getRows()
{
    wxChoice* choice = findNamedObject<wxChoice>(_dialog, "TerrainGeneratorRows");
    return string::convert<std::size_t>(choice->GetStringSelection().ToStdString(), 11);
}

float TerrainGeneratorDialog::getPhysicalWidth()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorWidth")->GetValue().ToStdString(),
        512.0f);
}

float TerrainGeneratorDialog::getPhysicalHeight()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorHeight")->GetValue().ToStdString(),
        512.0f);
}

unsigned int TerrainGeneratorDialog::getSeed()
{
    return static_cast<unsigned int>(
        findNamedObject<wxSpinCtrl>(_dialog, "TerrainGeneratorSeed")->GetValue());
}

float TerrainGeneratorDialog::getFrequency()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorFrequency")->GetValue().ToStdString(),
        0.01f);
}

float TerrainGeneratorDialog::getAmplitude()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorAmplitude")->GetValue().ToStdString(),
        64.0f);
}

int TerrainGeneratorDialog::getOctaves()
{
    return findNamedObject<wxSpinCtrl>(_dialog, "TerrainGeneratorOctaves")->GetValue();
}

float TerrainGeneratorDialog::getPersistence()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorPersistence")
            ->GetValue()
            .ToStdString(),
        0.5f);
}

float TerrainGeneratorDialog::getLacunarity()
{
    return string::convert<float>(findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorLacunarity")
                                      ->GetValue()
                                      .ToStdString(),
                                  2.0f);
}

float TerrainGeneratorDialog::getOffset()
{
    return string::convert<float>(
        findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorOffset")->GetValue().ToStdString(),
        1.0f);
}

std::string TerrainGeneratorDialog::getMaterial()
{
    return findNamedObject<wxTextCtrl>(_dialog, "TerrainGeneratorMaterial")
        ->GetValue()
        .ToStdString();
}

void TerrainGeneratorDialog::Show(const cmd::ArgumentList& args)
{
    TerrainGeneratorDialog dialog;

    if (dialog.run() != IDialog::RESULT_OK)
    {
        return;
    }

    noise::NoiseParameters params;
    params.algorithm = dialog.getAlgorithm();
    params.seed = dialog.getSeed();
    params.frequency = dialog.getFrequency();
    params.amplitude = dialog.getAmplitude();
    params.octaves = dialog.getOctaves();
    params.persistence = dialog.getPersistence();
    params.lacunarity = dialog.getLacunarity();
    params.offset = dialog.getOffset();

    std::size_t columns = dialog.getColumns();
    std::size_t rows = dialog.getRows();
    float physicalWidth = dialog.getPhysicalWidth();
    float physicalHeight = dialog.getPhysicalHeight();
    std::string material = dialog.getMaterial();

    Vector3 spawnPos = getSpawnPosition();

    UndoableCommand undo("terrainGeneratorCreate");

    GlobalSelectionSystem().setSelectedAll(false);

    scene::INodePtr node = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);

    GlobalMapModule().findOrInsertWorldspawn()->addChildNode(node);

    IPatch* patch = Node_getIPatch(node);
    if (!patch)
    {
        return;
    }

    patch->setDims(columns, rows);

    // Create noise generator
    noise::NoiseGenerator noiseGen(params);
    float spacingX = physicalWidth / static_cast<float>(columns - 1);
    float spacingY = physicalHeight / static_cast<float>(rows - 1);
    float offsetX = spawnPos.x() - physicalWidth / 2.0f;
    float offsetY = spawnPos.y() - physicalHeight / 2.0f;
    float baseZ = spawnPos.z();

    // Fill in points based on thr noise
    for (std::size_t row = 0; row < rows; ++row)
    {
        for (std::size_t col = 0; col < columns; ++col)
        {
            PatchControl& ctrl = patch->ctrlAt(row, col);

            float worldX = offsetX + col * spacingX;
            float worldY = offsetY + row * spacingY;
            float noiseValue = static_cast<float>(noiseGen.sample(worldX, worldY));

            ctrl.vertex.x() = worldX;
            ctrl.vertex.y() = worldY;
            ctrl.vertex.z() = baseZ + noiseValue;
            ctrl.texcoord.x() = static_cast<float>(col) / static_cast<float>(columns - 1);
            ctrl.texcoord.y() = static_cast<float>(row) / static_cast<float>(rows - 1);
        }
    }

    patch->controlPointsChanged();
    patch->setShader(material);
    patch->scaleTextureNaturally();

    Node_setSelected(node, true);
}

} // namespace ui
