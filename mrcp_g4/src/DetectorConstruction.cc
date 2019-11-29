#include "DetectorConstruction.hh"
#include "TETModelStore.hh"
#include "TETParameterisation.hh"
#include "MRCPModel.hh"
#include "MRCPPSDoseDeposit.hh"

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
: G4VUserDetectorConstruction(), fMainPhantom_FilePath(mainPhantom_FilePath.c_str())
{}

DetectorConstruction::~DetectorConstruction()
{}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // Materials
    auto mat_Air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");

    // --- Geometry: World --- //
    G4double world_Size = 10.*m;
    auto sol_World = new G4Box("World", 0.5*world_Size, 0.5*world_Size, 0.5*world_Size);
    auto lv_World = new G4LogicalVolume(sol_World, mat_Air, "World");
    lv_World->SetVisAttributes(G4VisAttributes::Invisible);
    auto pv_World = new G4PVPlacement(nullptr, G4ThreeVector(), lv_World, "World", nullptr, false, 0);

    // --- Geometry: Main Phantom --- //
    // Load phantom data
    G4String phantomClassifier = fMainPhantom_FilePath.filename().string().substr(0, 2); // AM-## or AF-##
    auto mainPhantomData = new MRCPModel("MainPhantom",
        fMainPhantom_FilePath.replace_extension(".node").string(),
        fMainPhantom_FilePath.replace_extension(".ele").string(),
        fMainPhantom_FilePath.replace_filename("ICRP-" + phantomClassifier + ".material").string(),
        fMainPhantom_FilePath.replace_filename("ICRP-" + phantomClassifier + ".RBMnBS").string(),
        fMainPhantom_FilePath.replace_filename("colour_OLD.dat").string());
    mainPhantomData->Print();

    // Create phantom box with margin
    // Don't know the specific reason, but the margin will benefit from memory & initialization time.
    G4double phantomBox_Margin = 10.*cm;
    auto sol_PhantomBox = new G4Box("PhantomBox",
        mainPhantomData->GetBoundingBoxSize().x()/2. + phantomBox_Margin,
        mainPhantomData->GetBoundingBoxSize().y()/2. + phantomBox_Margin,
        mainPhantomData->GetBoundingBoxSize().z()/2. + phantomBox_Margin);
    auto lv_PhantomBox = new G4LogicalVolume(sol_PhantomBox, mat_Air, "PhantomBox");
    lv_PhantomBox->SetVisAttributes(G4VisAttributes::Invisible);
    lv_PhantomBox->SetOptimisation(true);
    lv_PhantomBox->SetSmartless(0.5); // for optimization (default=2)
    G4ThreeVector phantomPos(0., 0., mainPhantomData->GetBoundingBoxSize().z()/2);
    new G4PVPlacement(nullptr, phantomPos, lv_PhantomBox, "PhantomBox", lv_World, false, 0);

    // Create tetrahedral phantom (visualization is in the TETParameterisation::ComputeMaterial())
    auto sol_Tet = new G4Tet("Tet",
        G4ThreeVector(),
        G4ThreeVector(1.*cm, 0, 0),
        G4ThreeVector(0, 1.*cm, 0),
        G4ThreeVector(0, 0, 1.*cm));
    fTetLogicalVolume = new G4LogicalVolume(sol_Tet, mat_Air, "Tet");
    new G4PVParameterised("mainPhantomTets", fTetLogicalVolume, lv_PhantomBox,
        kUndefined, static_cast<G4int>(mainPhantomData->GetNumTets()),
        new TETParameterisation("MainPhantom"));

    return pv_World;
}

void DetectorConstruction::ConstructSDandField()
{
    // --- Multi functional detector: MainPhantom --- //
    auto tetMFD = new G4MultiFunctionalDetector("MainPhantom");
    auto ps_MRCPDose = new MRCPPSDoseDeposit("dose", "MainPhantom");
    ps_MRCPDose->ImportBoneDRFData(fMainPhantom_FilePath.replace_filename("ICRP116.DRF").string());
    tetMFD->RegisterPrimitive(ps_MRCPDose);
    G4SDManager::GetSDMpointer()->AddNewDetector(tetMFD);
    SetSensitiveDetector(fTetLogicalVolume, tetMFD);
}
