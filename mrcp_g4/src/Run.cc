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

    // Calculate protection quantities of this event
    std::map<G4String, G4double> protQMap;
    protQMap["01. WholeBodyDose"] = mainPhantomProtQ->GetWholebodyDose(doseMap);
    protQMap["02. EffectiveDose"] = mainPhantomProtQ->GetEffectiveDose(doseMap);
    protQMap["03. RBM"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::RedBoneMarrow, doseMap);
    protQMap["04. Liver"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Liver, doseMap);

    // Store the quantities and their squared values
    for(const auto& protQ: protQMap)
    {
        fProtQ[protQ.first].first += protQ.second;
        fProtQ[protQ.first].second += protQ.second * protQ.second;
    }

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
