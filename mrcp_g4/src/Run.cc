#include "Run.hh"
#include "Primary_ParticleGun.hh"
#include "MRCPProtQCalculator.hh"

Run::Run()
: G4Run(), fPhantomDose_HCID(-1)
{
    // --- Get PrimaryGeneratorAction --- //
    fPrimaryGeneratorAction = dynamic_cast<const Primary_ParticleGun*>(G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
}

Run::~Run()
{
    if(!fProtQ.empty()) fProtQ.clear();
}

void Run::RecordEvent(const G4Event* anEvent)
{
    G4double primaryWeight = 1.;
    if(fPrimaryGeneratorAction) primaryWeight = fPrimaryGeneratorAction->GetPrimaryWeight();

    if(fPhantomDose_HCID==-1)
        fPhantomDose_HCID = G4SDManager::GetSDMpointer()->GetCollectionID("MainPhantom/dose");

    auto HCE = anEvent->GetHCofThisEvent();
    if(!HCE) return;

    auto evtMap = HCE->GetHC(fPhantomDose_HCID);

    MRCPProtQCalculator mrcpProtQCalc("MainPhantom", evtMap);

    G4double wholeBodyDose = mrcpProtQCalc.GetWholebodyDose();
    wholeBodyDose *= primaryWeight;
    fProtQ["WholeBodyDose"].first += wholeBodyDose;
    fProtQ["WholeBodyDose"].second += wholeBodyDose * wholeBodyDose;

    G4Run::RecordEvent(anEvent);
}

void Run::Merge(const G4Run* aRun)
{
    const Run* localRun = static_cast<const Run*>(aRun);

    for(const auto& protQ: localRun->GetProtQ())
    {
        this->fProtQ[protQ.first].first += std::get<0>(protQ.second);
        this->fProtQ[protQ.first].second += std::get<1>(protQ.second);
    }

    G4Run::Merge(aRun);
}
