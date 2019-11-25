#include "MRCPRun.hh"
#include "TETModelStore.hh"
#include "Primary_ParticleGun.hh"

MRCPRun::MRCPRun()
: G4Run(), fEDep_HCID(-1), fRBMDose_HCID(-1), fBSDose_HCID(-1)
{
    doseCalculation = new MRCPDoseCalculation("MainPhantom");

    // --- Set the interest protection quantities --- //
    fInterestProtectionQuantities.push_back(ProtQName::EffectiveDose);
    fInterestProtectionQuantities.push_back(ProtQName::EffectiveDose_byDRFWhole);
    fInterestProtectionQuantities.push_back(ProtQName::EffectiveDose_byMassRatioTarget);
    fInterestProtectionQuantities.push_back(ProtQName::EffectiveDose_byMassRatioWhole);
    fInterestProtectionQuantities.push_back(ProtQName::WholeBodyDose);
    fInterestProtectionQuantities.push_back(ProtQName::HT_EyeLensTarget);
    fInterestProtectionQuantities.push_back(ProtQName::HT_EyeLensWhole);
    fInterestProtectionQuantities.push_back(ProtQName::HT_RedBoneMarrow_byDRF);
    fInterestProtectionQuantities.push_back(ProtQName::HT_RedBoneMarrow_byMassRatio);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Breast);
    fInterestProtectionQuantities.push_back(ProtQName::HT_ColonTarget);
    fInterestProtectionQuantities.push_back(ProtQName::HT_StomachTarget);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Lungs);
    fInterestProtectionQuantities.push_back(ProtQName::HT_RemainderTarget);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Gonads);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Bladder);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Liver);
    fInterestProtectionQuantities.push_back(ProtQName::HT_OesophagusTarget);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Thyroid);
    fInterestProtectionQuantities.push_back(ProtQName::HT_BoneSurface_byDRF);
    fInterestProtectionQuantities.push_back(ProtQName::HT_BoneSurface_byMassRatio);
    fInterestProtectionQuantities.push_back(ProtQName::HT_Brain);
    fInterestProtectionQuantities.push_back(ProtQName::HT_SalivaryGlands);
    fInterestProtectionQuantities.push_back(ProtQName::HT_SkinTarget);

    for(const auto& protQ: fInterestProtectionQuantities)
        fProtectionQuantities.push_back(std::make_tuple(MRCPDoseCalculation::ProtQNameString(protQ), 0., 0.));

    // --- Get PrimaryGeneratorAction --- //
    fPrimaryGeneratorAction = dynamic_cast<const Primary_ParticleGun*>(G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
}

MRCPRun::~MRCPRun()
{
    if(doseCalculation) delete doseCalculation;
}

void MRCPRun::RecordEvent(const G4Event* anEvent)
{
    if(fEDep_HCID==-1)
        fEDep_HCID = G4SDManager::GetSDMpointer()->GetCollectionID("TETModelMFD/EDep");
    if(fRBMDose_HCID==-1)
        fRBMDose_HCID = G4SDManager::GetSDMpointer()->GetCollectionID("TETModelMFD/RBMDose");
    if(fBSDose_HCID==-1)
        fBSDose_HCID = G4SDManager::GetSDMpointer()->GetCollectionID("TETModelMFD/BSDose");

    auto HCE = anEvent->GetHCofThisEvent();
    if(!HCE) return;

    // --- Calculate protection quantities for MRCP --- //
    // Get Map from HitsCollectionOfThisEvent
    auto eDepMap = static_cast<G4THitsMap<G4double>*>(HCE->GetHC(fEDep_HCID))->GetMap();
    auto RBMDoseMap = static_cast<G4THitsMap<G4double>*>(HCE->GetHC(fRBMDose_HCID))->GetMap();
    auto BSDoseMap = static_cast<G4THitsMap<G4double>*>(HCE->GetHC(fBSDose_HCID))->GetMap();

    // MRCPDoseCalculation::Set@@Map()
    doseCalculation->SetEDepMap(eDepMap);
    doseCalculation->SetRBMDoseMap(RBMDoseMap);
    doseCalculation->SetBSDoseMap(BSDoseMap);

    // MRCPDoseCalculation::CalculateProtectionQuantities()
    doseCalculation->CalculateProtectionQuantities();

    // Get primary weight if possible
    G4double primaryWeight = 1.;
    if(fPrimaryGeneratorAction) primaryWeight = fPrimaryGeneratorAction->GetPrimaryWeight();

    // MRCPDoseCalculation::GetProtectionQuantity()
    for(size_t i = 0; i < fInterestProtectionQuantities.size(); ++i)
    {
        G4double protQValue =
            doseCalculation->GetProtectionQuantity(fInterestProtectionQuantities.at(i));
        protQValue *= primaryWeight;
        std::get<1>(fProtectionQuantities[i]) += protQValue;
        std::get<2>(fProtectionQuantities[i]) += protQValue * protQValue;
    }

    // MRCPDoseCalculation::Clear()
    doseCalculation->Clear();

    G4Run::RecordEvent(anEvent);
}

void MRCPRun::Merge(const G4Run* aRun)
{
    const MRCPRun* localRun = static_cast<const MRCPRun*>(aRun);

    for(size_t i = 0; i < fInterestProtectionQuantities.size(); ++i)
    {
        std::get<1>(fProtectionQuantities[i]) +=
            std::get<1>(localRun->fProtectionQuantities.at(i));
        std::get<2>(fProtectionQuantities[i]) +=
            std::get<2>(localRun->fProtectionQuantities.at(i));
    }

    G4Run::Merge(aRun);
}
