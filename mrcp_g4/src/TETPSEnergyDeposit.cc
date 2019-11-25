#include "TETPSEnergyDeposit.hh"
#include "TETModelStore.hh"

TETPSEnergyDeposit::TETPSEnergyDeposit(G4String name, G4String tetModelName)
: G4PSEnergyDeposit(name)
{
    fTETModel = TETModelStore::GetInstance()->GetTETModel(tetModelName);
    if(!fTETModel)
        G4Exception("TETPSEnergyDeposit::TETPSEnergyDeposit()", "", FatalErrorInArgument,
            G4String("      invalid tetModel '" + tetModelName + "'" ).c_str());
}

TETPSEnergyDeposit::~TETPSEnergyDeposit()
{}

G4int TETPSEnergyDeposit::GetIndex(G4Step* aStep)
{
    G4int copyNo = aStep->GetPreStepPoint()->GetTouchable()->GetCopyNumber();
    return fTETModel->GetSubModelID(copyNo);
}
