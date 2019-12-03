#include "MRCPProtQCalculator.hh"
#include "MRCPModel.hh"

MRCPProtQCalculator::MRCPProtQCalculator(const G4String& phantomName)
{
    fMRCPModel = dynamic_cast<MRCPModel*>(
            TETModelStore::GetInstance()->GetTETModel(phantomName)
            );
    if(!fMRCPModel)
        G4Exception("MRCPProtQCalculator::MRCPProtQCalculator()", "", FatalErrorInArgument,
                G4String("      invalid MRCPModel '" + phantomName + "'" ).c_str());

    // Protection quantity preset
    Preset_WholeBodyDose();
    Preset_OrganDose();
}

G4double MRCPProtQCalculator::GetWholebodyDose(const G4THitsMap<G4double>* subModelDoseMap)
{

    G4double wholeBodyDose{0.};
    for(const auto& datum: *(subModelDoseMap->GetMap()))
    {
        // If the subModel is not a part of whole body, skip it.
        G4int subModelID = datum.first;
        if(protQ_subModelWeights_Map[Organ::WholeBody].find(subModelID) ==
                protQ_subModelWeights_Map[Organ::WholeBody].end())
            continue;

        // Calculate
        G4double subModelDose = *datum.second;
        G4double subModelWeightFactor =
                protQ_subModelWeights_Map.at(Organ::WholeBody).at(subModelID);

        wholeBodyDose += subModelDose * subModelWeightFactor;
    }

    return wholeBodyDose;
}

G4double MRCPProtQCalculator::GetOrganDose(Organ organName, const G4THitsMap<G4double>* subModelDoseMap)
{
    G4double organDose{0.};
    for(const auto& datum: *(subModelDoseMap->GetMap()))
    {
        // If the subModel is not a part of whole body, skip it.
        G4int subModelID = datum.first;
        if(protQ_subModelWeights_Map[organName].find(subModelID) ==
                protQ_subModelWeights_Map[organName].end())
            continue;

        // Calculate
        G4double subModelDose = *datum.second;
        G4double subModelWeightFactor =
                protQ_subModelWeights_Map.at(organName).at(subModelID);

        organDose += subModelDose * subModelWeightFactor;
    }

    return organDose;
}

G4double MRCPProtQCalculator::GetEffectiveDose(const G4THitsMap<G4double>* subModelDoseMap)
{
    // Remainder dose
    G4double remainderDose = (
                GetOrganDose(Organ::Adrenals, subModelDoseMap) +
                GetOrganDose(Organ::Extrathoracic, subModelDoseMap) +
                GetOrganDose(Organ::GallBladder, subModelDoseMap) +
                GetOrganDose(Organ::Heart, subModelDoseMap) +
                GetOrganDose(Organ::Kidneys, subModelDoseMap) +
                GetOrganDose(Organ::LymphaticNodes, subModelDoseMap) +
                GetOrganDose(Organ::Muscle, subModelDoseMap) +
                GetOrganDose(Organ::OralMucosa, subModelDoseMap) +
                GetOrganDose(Organ::Pancreas, subModelDoseMap) +
                GetOrganDose(Organ::ProstateUterus, subModelDoseMap) +
                GetOrganDose(Organ::SmallIntestine, subModelDoseMap) +
                GetOrganDose(Organ::Spleen, subModelDoseMap) +
                GetOrganDose(Organ::Thymus, subModelDoseMap)
                ) / 13.;

    G4double effectiveDose =
            (
                GetOrganDose(Organ::RedBoneMarrow, subModelDoseMap) +
                GetOrganDose(Organ::Colon, subModelDoseMap) +
                GetOrganDose(Organ::Lungs, subModelDoseMap) +
                GetOrganDose(Organ::Stomach, subModelDoseMap) +
                GetOrganDose(Organ::Breast, subModelDoseMap) +
                remainderDose
            ) * .12
            +
            (
                GetOrganDose(Organ::Gonads, subModelDoseMap)
            ) * .08
            +
            (
                GetOrganDose(Organ::Bladder, subModelDoseMap) +
                GetOrganDose(Organ::Liver, subModelDoseMap) +
                GetOrganDose(Organ::Oesophagus, subModelDoseMap) +
                GetOrganDose(Organ::Thyroid, subModelDoseMap)
            ) * .04
            +
            (
                GetOrganDose(Organ::BoneSurface, subModelDoseMap) +
                GetOrganDose(Organ::Brain, subModelDoseMap) +
                GetOrganDose(Organ::SalivaryGlands, subModelDoseMap) +
                GetOrganDose(Organ::Skin, subModelDoseMap)
            ) * .01;

    return effectiveDose;
}

void MRCPProtQCalculator::Preset_WholeBodyDose()
{
    std::vector<SubModel> wholeBody_subModelVec;

    for(G4int i = static_cast<G4int>(SubModel::FIRST);
            i <= static_cast<G4int>(SubModel::LAST); ++i)
    {
        SubModel subModel = static_cast<SubModel>(i);
        switch(subModel)
        {
        // not to be included to whole body dose calculation
        case SubModel::mTongueUpper_food:
        case SubModel::mOesophagusC:
        case SubModel::mStomachC:
        case SubModel::mSIContent__500_0:
        case SubModel::mSIContent_center__500:
        case SubModel::mAscColon_content:
        case SubModel::mTransColonR_content:
        case SubModel::mTransColonL_content:
        case SubModel::mDscColon_content:
        case SubModel::mSigColonW_content:
        case SubModel::mGallbladderC:
//        case SubModel::mHeartC: // Blood. included in whole body dose
        case SubModel::mUrinarybladderC:
        case SubModel::mET2__15_0: // ICRP 66 Fig. 4. Mucus layer
        case SubModel::mBB_gen1_m11_m6: // ICRP 66 Fig. 5. Cilia & sol layer
        case SubModel::mBB_gen1_m6_0: // ICRP 66 Fig. 5. Mucus layer
            break;

        // else will be included.
        default:
            wholeBody_subModelVec.push_back(subModel);
        }
    }

    // Calculate mass of the whole body
    G4double wholeBodyMass{0.};
    for(const auto& subModel: wholeBody_subModelVec)
    {
        G4int subModelID = static_cast<G4int>(subModel);
        G4double subModelMass = fMRCPModel->GetSubModelMass(subModelID);

        wholeBodyMass += subModelMass;
    }

    // Calculate & record mass ratio (weight factor) of each subModel
    for(const auto& subModel: wholeBody_subModelVec)
    {
        G4int subModelID = static_cast<G4int>(subModel);
        G4double subModelMass = fMRCPModel->GetSubModelMass(subModelID);

        protQ_subModelWeights_Map[Organ::WholeBody][subModelID] = subModelMass / wholeBodyMass;
    }
}

void MRCPProtQCalculator::Preset_OrganDose()
{
    std::map< Organ, std::vector<SubModel> > organSubModelVec_Map;

    // 1. Red bone marrow (Active marrow)
    organSubModelVec_Map[Organ::RedBoneMarrow] =
    {
        SubModel::mHumeriUS,
        SubModel::mClavicleS,
        SubModel::mCraniumS,
        SubModel::mFemoraUS,
        SubModel::mMandibleS,
        SubModel::mPelvisS,
        SubModel::mRibsS,
        SubModel::mScapulaeS,
        SubModel::mCervicalS,
        SubModel::mThoracicS,
        SubModel::mLumbarS,
        SubModel::mSacrumS,
        SubModel::mSternumS
    };

    // 2. Colon
    organSubModelVec_Map[Organ::Colon] =
    {
        SubModel::mAscColonW_280_300,
        SubModel::mTransColonWR_280_300,
        SubModel::mTransColonWL_280_300,
        SubModel::mDscColonW_280_300,
        SubModel::mSigColonW_280_300
    };
    // 2*. Colon (whole)
    organSubModelVec_Map[Organ::ColonWhole] =
    {
        SubModel::mAscColonW_0_280,
        SubModel::mAscColonW_280_300,
        SubModel::mAscColonW_300_surf,
        SubModel::mTransColonWR_0_280,
        SubModel::mTransColonWR_280_300,
        SubModel::mTransColonWR_300_surf,
        SubModel::mTransColonWL_0_280,
        SubModel::mTransColonWL_280_300,
        SubModel::mTransColonWL_300_surf,
        SubModel::mDscColonW_0_280,
        SubModel::mDscColonW_280_300,
        SubModel::mDscColonW_300_surf,
        SubModel::mSigColonW_0_280,
        SubModel::mSigColonW_280_300,
        SubModel::mSigColonW_300_surf,
        SubModel::mRectumW
    };

    // 3. Lung
    organSubModelVec_Map[Organ::Lungs] =
    {
        SubModel::mLungR,
        SubModel::mLungL
    };

    // 4. Stomach
    organSubModelVec_Map[Organ::Stomach] =
    {
        SubModel::mStomach_60_100
    };
    // 4*. Stomach (whole)
    organSubModelVec_Map[Organ::StomachWhole] =
    {
        SubModel::mStomach_0_60,
        SubModel::mStomach_60_100,
        SubModel::mStomach_100_300,
        SubModel::mStomach_300_surf
    };

    // 5. Breast
    organSubModelVec_Map[Organ::Breast] =
    {
        SubModel::mBreastRA,
        SubModel::mBreastLA,
        SubModel::mBreastRG,
        SubModel::mBreastLG
    };

    // 6. Gonads
    organSubModelVec_Map[Organ::Gonads] =
    {
        SubModel::mGonadsR,
        SubModel::mGonadsL
    };

    // 7. Bladder
    organSubModelVec_Map[Organ::Bladder] =
    {
        SubModel::mUrinarybladder
    };

    // 8. Oesophagus
    organSubModelVec_Map[Organ::Oesophagus] =
    {
        SubModel::mOesophagus_190_200
    };
    // 8*. Oesophagus (whole)
    organSubModelVec_Map[Organ::OesophagusWhole] =
    {
        SubModel::mOesophagus_0_190,
        SubModel::mOesophagus_190_200,
        SubModel::mOesophagus_200_surf
    };

    // 9. Liver
    organSubModelVec_Map[Organ::Liver] =
    {
        SubModel::mLiver
    };

    // 10. Thyroid
    organSubModelVec_Map[Organ::Thyroid] =
    {
        SubModel::mThyroid
    };

    // 11. Bone surface
    organSubModelVec_Map[Organ::BoneSurface] =
    {
        SubModel::mHumeriUS,
        SubModel::mHumeriLS,
        SubModel::mHumeriM,
        SubModel::mUlnaeS,
        SubModel::mUlnaeM,
        SubModel::mHandsS,
        SubModel::mClavicleS,
        SubModel::mCraniumS,
        SubModel::mFemoraUS,
        SubModel::mFemoraLS,
        SubModel::mFemoraM,
        SubModel::mTibiaeS,
        SubModel::mTibiaeM,
        SubModel::mFootS,
        SubModel::mMandibleS,
        SubModel::mPelvisS,
        SubModel::mRibsS,
        SubModel::mScapulaeS,
        SubModel::mCervicalS,
        SubModel::mThoracicS,
        SubModel::mLumbarS,
        SubModel::mSacrumS,
        SubModel::mSternumS
    };

    // 12. Brain
    organSubModelVec_Map[Organ::Brain] =
    {
        SubModel::mBrain
    };

    // 13. Salivary glands
    organSubModelVec_Map[Organ::SalivaryGlands] =
    {
        SubModel::mSalivary_glandR,
        SubModel::mSalivary_glandL
    };

    // 14. Skin
    organSubModelVec_Map[Organ::Skin] =
    {
        SubModel::mSkin_50_100
    };
    // 14*. Skin (whole)
    organSubModelVec_Map[Organ::SkinWhole] =
    {
        SubModel::mSkin_Osurf_50,
        SubModel::mSkin_50_100,
        SubModel::mSkin_100_Isurf
    };

    // 15. Adrenals
    organSubModelVec_Map[Organ::Adrenals] =
    {
        SubModel::mAdrenalR,
        SubModel::mAdrenalL
    };

    // 16. Extrathoracic
    // Exceptional one. See below.
    // 16*. Extrathracic (whole)
    // Exceptional one. See below.

    // 17. Gall bladder
    organSubModelVec_Map[Organ::GallBladder] =
    {
        SubModel::mGallbladder
    };

    // 18. Heart
    organSubModelVec_Map[Organ::Heart] =
    {
        SubModel::mHeart
    };

    // 19. Kidneys
    organSubModelVec_Map[Organ::Kidneys] =
    {
        SubModel::mKidneyRCortex,
        SubModel::mKidneyRMedulla,
        SubModel::mKidneyRPelvis,
        SubModel::mKidneyLCortex,
        SubModel::mKidneyLMedulla,
        SubModel::mKidneyLPelvis
    };

    // 20. Lymphatic nodes
    organSubModelVec_Map[Organ::LymphaticNodes] =
    {
        SubModel::mLymphET,
        SubModel::mLymphCer,
        SubModel::mLymphAxi,
        SubModel::mLymphBre,
        SubModel::mLymphTho,
        SubModel::mLymphCub,
        SubModel::mLymphMes,
        SubModel::mLymphIng,
        SubModel::mLymphPop
    };

    // 21. Muscle
    organSubModelVec_Map[Organ::Muscle] =
    {
        SubModel::mMuscle
    };

    // 22. Oral mucosa
    organSubModelVec_Map[Organ::OralMucosa] =
    {
        SubModel::mTongueLower_OralM,
        SubModel::mMouthFloor_OralM,
        SubModel::mMouthLibCheek_OralM
    };

    // 23. Pancreas
    organSubModelVec_Map[Organ::Pancreas] =
    {
        SubModel::mPancreas
    };

    // 24. Prostate/Uterus
    organSubModelVec_Map[Organ::ProstateUterus] =
    {
        SubModel::mProstate_Uterus
    };

    // 25. Small intestine
    organSubModelVec_Map[Organ::SmallIntestine] =
    {
        SubModel::mSI_130_150
    };
    // 25*. Small intestine (whole)
    organSubModelVec_Map[Organ::SmallIntestineWhole] =
    {
        SubModel::mSI_0_130,
        SubModel::mSI_130_150,
        SubModel::mSI_150_200,
        SubModel::mSI_200_surf
    };

    // 26. Spleen
    organSubModelVec_Map[Organ::Spleen] =
    {
        SubModel::mSpleen
    };

    // 27. Thymus
    organSubModelVec_Map[Organ::Thymus] =
    {
        SubModel::mThymus
    };

    // 28. Eye lens
    organSubModelVec_Map[Organ::EyeLens] =
    {
        SubModel::mSensLensR,
        SubModel::mSensLensL
    };
    // 28*. Eye lens (whole)
    organSubModelVec_Map[Organ::EyeLensWhole] =
    {
        SubModel::mSensLensR,
        SubModel::mSensLensL,
        SubModel::mInsensLensR,
        SubModel::mInsensLensL,
    };

    // --- subModel weighting factor calculation --- //
    // For all organs (except Organ::WholeBody)
    for(const auto& organSubModelVec: organSubModelVec_Map)
    {
        // Calculate mass of the organ
        G4double organMass{0.};
        for(const auto& subModel: organSubModelVec.second)
        {
            G4int subModelID = static_cast<G4int>(subModel);
            G4double subModelMass = fMRCPModel->GetSubModelMass(subModelID);

            organMass += subModelMass;
        }

        // Calculate & record mass ratio (weighting factor) of each subModel
        for(const auto& subModel: organSubModelVec.second)
        {
            G4int subModelID = static_cast<G4int>(subModel);
            G4double subModelMass = fMRCPModel->GetSubModelMass(subModelID);

            protQ_subModelWeights_Map[organSubModelVec.first][subModelID] = subModelMass / organMass;
        }
    }

    // --- Organs that which weighting factors are not mass ratio --- //
    // 1. Red Bone Marrow
    for(const auto& subModel: organSubModelVec_Map.at(Organ::RedBoneMarrow))
    {
        G4int subModelID = static_cast<G4int>(subModel);
        G4double subModelRBMMassRatio = fMRCPModel->GetSubModelRBMMassRatio(subModelID);

        // RBM dose by using DRF. subModelIDs are -10xx (see MRCPPSDoseDeposit::ProcessHits())
        protQ_subModelWeights_Map[Organ::RedBoneMarrow][(-subModelID)-1000] = subModelRBMMassRatio;
        // RBM dose by using only mass ratio.
        protQ_subModelWeights_Map[Organ::RedBoneMarrow_byMassRatio][subModelID] = subModelRBMMassRatio;
    }

    // 2. Bone Surface
    for(const auto& subModel: organSubModelVec_Map.at(Organ::BoneSurface))
    {
        G4int subModelID = static_cast<G4int>(subModel);
        G4double subModelBSMassRatio = fMRCPModel->GetSubModelBSMassRatio(subModelID);

        // BS dose by using DRF. subModelIDs are -20xx (see MRCPPSDoseDeposit::ProcessHits())
        protQ_subModelWeights_Map[Organ::BoneSurface][(-subModelID)-2000] = subModelBSMassRatio;
        // BS dose by using only mass ratio.
        protQ_subModelWeights_Map[Organ::BoneSurface_byMassRatio][subModelID] = subModelBSMassRatio;
    }

    // 3. Extrathoracic
    protQ_subModelWeights_Map[Organ::Extrathoracic][static_cast<G4int>(SubModel::mET1_40_50)] = .001;
    protQ_subModelWeights_Map[Organ::Extrathoracic][static_cast<G4int>(SubModel::mET2_40_50)] = .999;

    // 3*. Extrathoracic (whole)
    // ET1 region
    G4double ET1Mass =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_0_8)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_8_40)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_40_50)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_50_Surf));
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET1_0_8)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_0_8)) / ET1Mass * .001;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET1_8_40)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_8_40)) / ET1Mass * .001;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET1_40_50)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_40_50)) / ET1Mass * .001;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET1_50_Surf)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET1_40_50)) / ET1Mass * .001;
    // ET2 region
    G4double ET2Mass =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_0_40)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_40_50)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_50_55)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_55_65)) +
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_65_Surf));
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET2_0_40)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_0_40)) / ET2Mass * .999;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET2_40_50)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_40_50)) / ET2Mass * .999;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET2_50_55)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_50_55)) / ET2Mass * .999;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET2_55_65)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_55_65)) / ET2Mass * .999;
    protQ_subModelWeights_Map[Organ::ExtrathoracicWhole][static_cast<G4int>(SubModel::mET2_65_Surf)] =
            fMRCPModel->GetSubModelMass(static_cast<G4int>(SubModel::mET2_65_Surf)) / ET2Mass * .999;
}


