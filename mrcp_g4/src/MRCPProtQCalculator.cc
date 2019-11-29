#include "MRCPProtQCalculator.hh"
#include "MRCPModel.hh"

MRCPProtQCalculator::MRCPProtQCalculator(const G4String& phantomName, G4VHitsCollection* evtMap)
{
    fMRCPModel = dynamic_cast<MRCPModel*>(
        TETModelStore::GetInstance()->GetTETModel(phantomName)
        );
    if(!fMRCPModel)
        G4Exception("MRCPProtQCalculator::MRCPProtQCalculator()", "", FatalErrorInArgument,
            G4String("      invalid MRCPModel '" + phantomName + "'" ).c_str());

    fEvtMap = dynamic_cast<G4THitsMap<G4double>*>(evtMap);
    if(!fEvtMap)
        G4Exception("MRCPProtQCalculator::MRCPProtQCalculator()", "", FatalErrorInArgument,
            G4String("      invalid HitsCollection" ).c_str());
}

G4double MRCPProtQCalculator::GetWholebodyDose()
{
    std::vector<SubModelName> wholeBody_subModelVec;

    for(G4int i = static_cast<G4int>(SubModelName::FIRST);
        i <= static_cast<G4int>(SubModelName::LAST); ++i)
    {
        SubModelName subModel = static_cast<SubModelName>(i);
        switch(subModel)
        {
        // not to be included to whole body dose calculation
        case SubModelName::mTongueUpper_food:
        case SubModelName::mOesophagusC:
        case SubModelName::mStomachC:
        case SubModelName::mSIContent__500_0:
        case SubModelName::mSIContent_center__500:
        case SubModelName::mAscColon_content:
        case SubModelName::mTransColonR_content:
        case SubModelName::mTransColonL_content:
        case SubModelName::mDscColon_content:
        case SubModelName::mSigColonW_content:
        case SubModelName::mGallbladderC:
//        case SubModelName::mHeartC: // Blood. included in whole body dose
        case SubModelName::mUrinarybladderC:
        case SubModelName::mET2__15_0: // ICRP 66 Fig. 4. Mucus layer
        case SubModelName::mBB_gen1_m11_m6: // ICRP 66 Fig. 5. Cilia & sol layer
        case SubModelName::mBB_gen1_m6_0: // ICRP 66 Fig. 5. Mucus layer
            break;

        // else will be included.
        default:
            wholeBody_subModelVec.push_back(subModel);
        }
    }

    G4double wholeBodyMass{0.};
    for(const auto& subModel: wholeBody_subModelVec)
    {
        G4double subModelMass = fMRCPModel->GetSubModelMass(static_cast<G4int>(subModel));
        wholeBodyMass += subModelMass;
    }

    G4double wholeBodyDose{0.};
    for(const auto& datum: *(fEvtMap->GetMap()))
    {
        G4int subModelID = datum.first;
        if(subModelID<0) continue; // subModelID<0 means dose by DRF (see MRCPPSDoseDeposit::ProcessHits())
        G4double subModelDose = *datum.second;
        G4double subModelMass = fMRCPModel->GetSubModelMass(datum.first);
        wholeBodyDose += subModelDose * (subModelMass / wholeBodyMass);
    }

    return wholeBodyDose;
}
