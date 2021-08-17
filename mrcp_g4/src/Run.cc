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
    protQMap["11. RedBoneMarrow"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::RedBoneMarrow, doseMap);
    protQMap["12. Colon"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Colon, doseMap);
    protQMap["13. Lungs"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Lungs, doseMap);
    protQMap["14. Stomach"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Stomach, doseMap);
    protQMap["15. Breast"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Breast, doseMap);
    protQMap["16. Gonads"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Gonads, doseMap);
    protQMap["17. Bladder"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Bladder, doseMap);
    protQMap["18. Liver"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Liver, doseMap);
    protQMap["19. Oesophagus"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Oesophagus, doseMap);
    protQMap["20. Thyroid"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Thyroid, doseMap);
    protQMap["21. BoneSurface"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::BoneSurface, doseMap);
    protQMap["22. Brain"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Brain, doseMap);
    protQMap["23. SalivaryGlands"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::SalivaryGlands, doseMap);
    protQMap["24. Skin"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Skin, doseMap);
    protQMap["25. Adrenals"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Adrenals, doseMap);
    protQMap["26. Extrathoracic"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Extrathoracic, doseMap);
    protQMap["27. GallBladder"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::GallBladder, doseMap);
    protQMap["28. Heart"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Heart, doseMap);
    protQMap["29. Kidneys"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Kidneys, doseMap);
    protQMap["30. LymphaticNodes"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::LymphaticNodes, doseMap);
    protQMap["31. Muscle"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Muscle, doseMap);
    protQMap["32. OralMucosa"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::OralMucosa, doseMap);
    protQMap["33. Pancreas"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Pancreas, doseMap);
    protQMap["34. ProstateUterus"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::ProstateUterus, doseMap);
    protQMap["35. SmallIntestine"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::SmallIntestine, doseMap);
    protQMap["36. Spleen"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Spleen, doseMap);
    protQMap["37. Thymus"] = mainPhantomProtQ->GetOrganDose(MRCPProtQCalculator::Organ::Thymus, doseMap);

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
