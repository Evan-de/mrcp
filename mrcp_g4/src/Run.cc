#include "Run.hh"
#include "MRCPProtQCalculator.hh"

Run::Run()
: G4Run(), fPhantomDose_HCID(-1)
{
    // --- MRCPCalculator --- //
    mainPhantomProtQ = new MRCPProtQCalculator("MainPhantom");
}

Run::~Run()
{
    delete mainPhantomProtQ;
    if(!fProtQ.empty()) fProtQ.clear();
}

void Run::RecordEvent(const G4Event* anEvent)
{
    // --- Obtain dose map of this event --- //
    if(fPhantomDose_HCID==-1)
        fPhantomDose_HCID = G4SDManager::GetSDMpointer()->GetCollectionID("MainPhantom/dose");

    auto HCE = anEvent->GetHCofThisEvent();
    if(!HCE) return;

    auto doseMap = static_cast<G4THitsMap<G4double>*>(HCE->GetHC(fPhantomDose_HCID));

    // Whole body dose
    G4double wholeBodyDose = mainPhantomProtQ->GetWholebodyDose(doseMap);
    fProtQ["01. WholeBodyDose"].first += wholeBodyDose;
    fProtQ["01. WholeBodyDose"].second += wholeBodyDose * wholeBodyDose;

    // Effective dose
    G4double effectiveDose = mainPhantomProtQ->GetEffectiveDose(doseMap);
    fProtQ["02. EffectiveDose"].first += effectiveDose;
    fProtQ["02. EffectiveDose"].second += effectiveDose * effectiveDose;

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
