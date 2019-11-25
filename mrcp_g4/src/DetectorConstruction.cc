#include "DetectorConstruction.hh"
#include "TETModelStore.hh"
#include "TETParameterisation.hh"
#include "TETPSEnergyDeposit.hh"
#include "TETPSBoneDose_byDRF.hh"

#include "G4SystemOfUnits.hh"

#include "G4Box.hh"

#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"

#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"

#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"

DetectorConstruction::DetectorConstruction(G4String mainPhantom_FilePath)
: G4VUserDetectorConstruction(), fMainPhantom_FilePath(mainPhantom_FilePath)
{}

DetectorConstruction::~DetectorConstruction()
{}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // --- Initial setting --- //
    // Dimensions
    G4double world_Size = 10.*m;
    G4double phantomBox_Margin = 10.*cm;

    // Materials
    DefineMaterials();
    auto mat_Air = G4Material::GetMaterial("G4_AIR");

    // --- Geometry: World --- //
    auto sol_World = new G4Box("World", 0.5*world_Size, 0.5*world_Size, 0.5*world_Size);
    auto lv_World = new G4LogicalVolume(sol_World, mat_Air, "World");
//    auto va_World = new G4VisAttributes(G4Colour::White());
//    va_World->SetForceWireframe(true);
//    lv_World->SetVisAttributes(va_World);
    auto pv_World = new G4PVPlacement(0, G4ThreeVector(), lv_World, "World", 0, false, 0);

    // --- Geometry: Main Phantom --- //
    // Load phantom data
    G4String phantomClassifier = fMainPhantom_FilePath.substr(fMainPhantom_FilePath.rfind('/') + 1, 2); // AM or AF
    auto mainPhantomData = new TETModel("MainPhantom",
        fMainPhantom_FilePath + ".node",
        fMainPhantom_FilePath + ".ele",
        "../phantoms/ICRP-" + phantomClassifier + ".material",
        "../phantoms/colour_OLD.dat",
        "../phantoms/ICRP-" + phantomClassifier + ".RBMnBS",
        "../phantoms/ICRP-" + phantomClassifier + ".DRF");

    // Create phantom box with margin
    // Don't know the specific reason, but the margin will benefit from memory & initialization time.
    auto sol_PhantomBox = new G4Box("PhantomBox",
        mainPhantomData->GetBoundingBoxSize().x()/2. + phantomBox_Margin,
        mainPhantomData->GetBoundingBoxSize().y()/2. + phantomBox_Margin,
        mainPhantomData->GetBoundingBoxSize().z()/2. + phantomBox_Margin);
    auto lv_PhantomBox = new G4LogicalVolume(sol_PhantomBox, mat_Air, "PhantomBox");
//    auto va_PhantomBox = new G4VisAttributes(G4Colour::Black());
//    va_PhantomBox->SetForceWireframe(true);
//    lv_PhantomBox->SetVisAttributes(va_PhantomBox);
    new G4PVPlacement(nullptr, mainPhantomData->GetBoundingBoxCen(), lv_PhantomBox, "PhantomBox", lv_World, false, 0);
    lv_PhantomBox->SetOptimisation(true);
    lv_PhantomBox->SetSmartless(0.5); // for optimization (default=2)

    // Create tetrahedral phantom (visualization is in the TETParameterisation::ComputeMaterial())
    auto sol_Tet = new G4Tet("Tet",
        G4ThreeVector(),
        G4ThreeVector(1.*cm, 0, 0),
        G4ThreeVector(0, 1.*cm, 0),
        G4ThreeVector(0, 0, 1.*cm));
    fTetLogicalVolume = new G4LogicalVolume(sol_Tet, mat_Air, "Tet");
    new G4PVParameterised("mainPhantomTets", fTetLogicalVolume, lv_PhantomBox,
        kUndefined, mainPhantomData->GetNumTets(),
        new TETParameterisation("MainPhantom"));

    return pv_World;
}

void DetectorConstruction::ConstructSDandField()
{
    // --- Multi functional detector: Main Phantom --- //
    // --- with EnergyDeposit & BoneDose PS --- //
    auto tetMFD = new G4MultiFunctionalDetector("TETModelMFD");
    G4VPrimitiveScorer* ps_EDep =
        new TETPSEnergyDeposit("EDep", "MainPhantom");
    G4VPrimitiveScorer* ps_RBMDose =
        new TETPSBoneDose_byDRF("RBMDose", "MainPhantom", BoneSubModel::RBM);
    G4VPrimitiveScorer* ps_BSDose =
        new TETPSBoneDose_byDRF("BSDose", "MainPhantom", BoneSubModel::BS);
    tetMFD->RegisterPrimitive(ps_EDep);
    tetMFD->RegisterPrimitive(ps_RBMDose);
    tetMFD->RegisterPrimitive(ps_BSDose);
    G4SDManager::GetSDMpointer()->AddNewDetector(tetMFD);
    SetSensitiveDetector(fTetLogicalVolume, tetMFD);
}

void DetectorConstruction::DefineMaterials()
{
    // Get nist material manager
    G4NistManager* nist = G4NistManager::Instance();

    // NistMaterial definition
    nist->FindOrBuildMaterial("G4_AIR");
    nist->FindOrBuildMaterial("G4_WATER");
}
