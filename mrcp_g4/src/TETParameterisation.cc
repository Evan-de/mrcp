#include "TETParameterisation.hh"
#include "TETModelStore.hh"
#include "MRCPModel.hh"

TETParameterisation::TETParameterisation(G4String tetModelName)
: G4VPVParameterisation()
{
    fTETModel = TETModelStore::GetInstance()->GetTETModel(tetModelName);
    if(!fTETModel)
        G4Exception("TETParameterisation::TETParameterisation()", "", FatalErrorInArgument,
            G4String("      invalid tetModel '" + tetModelName + "'" ).c_str());
    for(const auto& subModelID: fTETModel->GetSubModelIDSet())
        subModelVisAttributes_Map[subModelID] =
            new G4VisAttributes(fTETModel->GetSubModelColour(subModelID));
}

TETParameterisation::~TETParameterisation()
{}

G4VSolid* TETParameterisation::ComputeSolid(const G4int copyNo, G4VPhysicalVolume*)
{
    // return G4Tet*
    return fTETModel->GetTetrahedron(copyNo);
}

G4Material* TETParameterisation::ComputeMaterial(
    const G4int copyNo, G4VPhysicalVolume* phy, const G4VTouchable*)
{
    G4int subModelID = fTETModel->GetSubModelID(copyNo);

    // Set VisAttributes
    phy->GetLogicalVolume()->SetVisAttributes(subModelVisAttributes_Map.at(subModelID));

    // If the model is MRCPModel, replace its material to MRCP material.
    MRCPModel* mrcpModel = dynamic_cast<MRCPModel*>(fTETModel);
    if(mrcpModel) return mrcpModel->GetSubModelMaterial(subModelID);
    else return phy->GetLogicalVolume()->GetMaterial();
}


