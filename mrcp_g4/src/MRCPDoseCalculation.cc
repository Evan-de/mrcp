#include "MRCPDoseCalculation.hh"
#include "TETModelStore.hh"

MRCPDoseCalculation::MRCPDoseCalculation(G4String tetModelName)
{
    fTETModel = TETModelStore::GetInstance()->GetTETModel(tetModelName);
    if(!fTETModel)
        G4Exception("MRCPDoseCalculation::MRCPDoseCalculation()", "", FatalErrorInArgument,
            G4String("      invalid tetModel. '" + tetModelName + "'" ).c_str());

    Create_protQ_subModels_Map();
    Create_SimpleProtQ_Vector();
}

void MRCPDoseCalculation::CalculateProtectionQuantities()
{
    if(fEDepMap->size())
    {
        // --- Calculate simple protection quantities by Average dose --- //
        for(const auto& protQ: simpleProtQ_Vector)
            protQ_Map[protQ] = CalculateAverageDose(protQ_subModels_Map.at(protQ));

        // --- Calculate Extrathoracic dose --- //
        protQ_Map[ProtQName::HT_ExtrathoracicTarget] =
            protQ_Map[ProtQName::HT_ET1Target] * 0.001
            + protQ_Map[ProtQName::HT_ET2Target] * 0.999;
        protQ_Map[ProtQName::HT_ExtrathoracicWhole] =
            protQ_Map[ProtQName::HT_ET1Whole] * 0.001
            + protQ_Map[ProtQName::HT_ET2Whole] * 0.999;

        // --- Calculate RBM dose by mass ratio --- //
        G4double RBMDose_byMassRatio = 0.;
        for(const auto& subModel: protQ_subModels_Map.at(ProtQName::HT_RedBoneMarrow_byMassRatio))
        {
            G4int subModelID = static_cast<G4int>(subModel);
            if(fEDepMap->find(subModelID) != fEDepMap->end())
            {
                G4double eDep = *(fEDepMap->at(subModelID));
                G4double subModelMass = fTETModel->GetSubModelMass(subModelID);
                G4double RBMMassRatio = fTETModel->GetSubModelMassRatio(subModelID, BoneSubModel::RBM);

                RBMDose_byMassRatio += (eDep/subModelMass) * RBMMassRatio;
            }
        }
        protQ_Map[ProtQName::HT_RedBoneMarrow_byMassRatio] = RBMDose_byMassRatio;

        // --- Calculate BS dose by mass ratio --- //
        G4double BSDose_byMassRatio = 0.;
        for(const auto& subModel: protQ_subModels_Map.at(ProtQName::HT_BoneSurface_byMassRatio))
        {
            G4int subModelID = static_cast<G4int>(subModel);
            if(fEDepMap->find(subModelID) != fEDepMap->end())
            {
                G4double eDep = *(fEDepMap->at(subModelID));
                G4double subModelMass = fTETModel->GetSubModelMass(subModelID);
                G4double BSMassRatio = fTETModel->GetSubModelMassRatio(subModelID, BoneSubModel::BS);

                BSDose_byMassRatio += (eDep/subModelMass) * BSMassRatio;
            }
        }
        protQ_Map[ProtQName::HT_BoneSurface_byMassRatio] = BSDose_byMassRatio;
    }

    if(fRBMDoseMap->size())
    {
        // --- Calculate RBM dose by DRF --- //
        G4double RBMDose_byDRF = 0.;
        for(const auto& subModel: protQ_subModels_Map.at(ProtQName::HT_RedBoneMarrow_byDRF))
        {
            G4int subModelID = static_cast<G4int>(subModel);
            if(fRBMDoseMap->find(subModelID) != fRBMDoseMap->end())
            {
                G4double RBMDoseInSubModel = *(fRBMDoseMap->at(subModelID));
                G4double RBMMassRatio = fTETModel->GetSubModelMassRatio(subModelID, BoneSubModel::RBM);

                RBMDose_byDRF += RBMDoseInSubModel * RBMMassRatio;
            }
        }
        protQ_Map[ProtQName::HT_RedBoneMarrow_byDRF] = RBMDose_byDRF;
    }

    if(fBSDoseMap->size())
    {
        // --- Calculate BS dose by DRF --- //
        G4double BSDose_byDRF = 0.;
        for(const auto& subModel: protQ_subModels_Map.at(ProtQName::HT_BoneSurface_byDRF))
        {
            G4int subModelID = static_cast<G4int>(subModel);
            if(fBSDoseMap->find(subModelID) != fBSDoseMap->end())
            {
                G4double BSDoseInSubModel = *(fBSDoseMap->at(subModelID));
                G4double BSMassRatio = fTETModel->GetSubModelMassRatio(subModelID, BoneSubModel::BS);

                BSDose_byDRF += BSDoseInSubModel * BSMassRatio;
            }
        }
        protQ_Map[ProtQName::HT_BoneSurface_byDRF] = BSDose_byDRF;
    }

    // --- Calculate Remainder dose --- //
    protQ_Map[ProtQName::HT_RemainderTarget] =
        (
        GetProtectionQuantity(ProtQName::HT_Adrenals) +
        GetProtectionQuantity(ProtQName::HT_ExtrathoracicTarget) +
        GetProtectionQuantity(ProtQName::HT_GallBladder) +
        GetProtectionQuantity(ProtQName::HT_Heart) +
        GetProtectionQuantity(ProtQName::HT_Kidneys) +
        GetProtectionQuantity(ProtQName::HT_LymphaticNodes) +
        GetProtectionQuantity(ProtQName::HT_Muscle) +
        GetProtectionQuantity(ProtQName::HT_OralMucosa) +
        GetProtectionQuantity(ProtQName::HT_Pancreas) +
        GetProtectionQuantity(ProtQName::HT_ProstateUtreus) +
        GetProtectionQuantity(ProtQName::HT_SmallIntestineTarget) +
        GetProtectionQuantity(ProtQName::HT_Spleen) +
        GetProtectionQuantity(ProtQName::HT_Thymus)
        )
        / 13.;
    protQ_Map[ProtQName::HT_RemainderWhole] =
        (
        GetProtectionQuantity(ProtQName::HT_Adrenals) +
        GetProtectionQuantity(ProtQName::HT_ExtrathoracicWhole) +
        GetProtectionQuantity(ProtQName::HT_GallBladder) +
        GetProtectionQuantity(ProtQName::HT_Heart) +
        GetProtectionQuantity(ProtQName::HT_Kidneys) +
        GetProtectionQuantity(ProtQName::HT_LymphaticNodes) +
        GetProtectionQuantity(ProtQName::HT_Muscle) +
        GetProtectionQuantity(ProtQName::HT_OralMucosa) +
        GetProtectionQuantity(ProtQName::HT_Pancreas) +
        GetProtectionQuantity(ProtQName::HT_ProstateUtreus) +
        GetProtectionQuantity(ProtQName::HT_SmallIntestineWhole) +
        GetProtectionQuantity(ProtQName::HT_Spleen) +
        GetProtectionQuantity(ProtQName::HT_Thymus)
        )
        / 13.;

    // --- Calculate Effective dose --- //
    protQ_Map[ProtQName::EffectiveDose] =
        (
        GetProtectionQuantity(ProtQName::HT_RedBoneMarrow_byDRF) +
        GetProtectionQuantity(ProtQName::HT_ColonTarget) +
        GetProtectionQuantity(ProtQName::HT_Lungs) +
        GetProtectionQuantity(ProtQName::HT_StomachTarget) +
        GetProtectionQuantity(ProtQName::HT_Breast) +
        GetProtectionQuantity(ProtQName::HT_RemainderTarget)
        ) * .12
        +
        GetProtectionQuantity(ProtQName::HT_Gonads)
        * .08
        +
        (
        GetProtectionQuantity(ProtQName::HT_Bladder) +
        GetProtectionQuantity(ProtQName::HT_Liver) +
        GetProtectionQuantity(ProtQName::HT_OesophagusTarget) +
        GetProtectionQuantity(ProtQName::HT_Thyroid)
        ) * .04
        +
        (
        GetProtectionQuantity(ProtQName::HT_BoneSurface_byDRF) +
        GetProtectionQuantity(ProtQName::HT_Brain) +
        GetProtectionQuantity(ProtQName::HT_SalivaryGlands) +
        GetProtectionQuantity(ProtQName::HT_SkinTarget)
        ) * .01;

    protQ_Map[ProtQName::EffectiveDose_byDRFWhole] =
        (
        GetProtectionQuantity(ProtQName::HT_RedBoneMarrow_byDRF) +
        GetProtectionQuantity(ProtQName::HT_ColonWhole) +
        GetProtectionQuantity(ProtQName::HT_Lungs) +
        GetProtectionQuantity(ProtQName::HT_StomachWhole) +
        GetProtectionQuantity(ProtQName::HT_Breast) +
        GetProtectionQuantity(ProtQName::HT_RemainderWhole)
        ) * .12
        +
        GetProtectionQuantity(ProtQName::HT_Gonads)
        * .08
        +
        (
        GetProtectionQuantity(ProtQName::HT_Bladder) +
        GetProtectionQuantity(ProtQName::HT_Liver) +
        GetProtectionQuantity(ProtQName::HT_OesophagusWhole) +
        GetProtectionQuantity(ProtQName::HT_Thyroid)
        ) * .04
        +
        (
        GetProtectionQuantity(ProtQName::HT_BoneSurface_byDRF) +
        GetProtectionQuantity(ProtQName::HT_Brain) +
        GetProtectionQuantity(ProtQName::HT_SalivaryGlands) +
        GetProtectionQuantity(ProtQName::HT_SkinWhole)
        ) * .01;

    protQ_Map[ProtQName::EffectiveDose_byMassRatioTarget] =
        (
        GetProtectionQuantity(ProtQName::HT_RedBoneMarrow_byMassRatio) +
        GetProtectionQuantity(ProtQName::HT_ColonTarget) +
        GetProtectionQuantity(ProtQName::HT_Lungs) +
        GetProtectionQuantity(ProtQName::HT_StomachTarget) +
        GetProtectionQuantity(ProtQName::HT_Breast) +
        GetProtectionQuantity(ProtQName::HT_RemainderTarget)
        ) * .12
        +
        GetProtectionQuantity(ProtQName::HT_Gonads)
        * .08
        +
        (
        GetProtectionQuantity(ProtQName::HT_Bladder) +
        GetProtectionQuantity(ProtQName::HT_Liver) +
        GetProtectionQuantity(ProtQName::HT_OesophagusTarget) +
        GetProtectionQuantity(ProtQName::HT_Thyroid)
        ) * .04
        +
        (
        GetProtectionQuantity(ProtQName::HT_BoneSurface_byMassRatio) +
        GetProtectionQuantity(ProtQName::HT_Brain) +
        GetProtectionQuantity(ProtQName::HT_SalivaryGlands) +
        GetProtectionQuantity(ProtQName::HT_SkinTarget)
        ) * .01;

    protQ_Map[ProtQName::EffectiveDose_byMassRatioWhole] =
        (
        GetProtectionQuantity(ProtQName::HT_RedBoneMarrow_byMassRatio) +
        GetProtectionQuantity(ProtQName::HT_ColonWhole) +
        GetProtectionQuantity(ProtQName::HT_Lungs) +
        GetProtectionQuantity(ProtQName::HT_StomachWhole) +
        GetProtectionQuantity(ProtQName::HT_Breast) +
        GetProtectionQuantity(ProtQName::HT_RemainderWhole)
        ) * .12
        +
        GetProtectionQuantity(ProtQName::HT_Gonads)
        * .08
        +
        (
        GetProtectionQuantity(ProtQName::HT_Bladder) +
        GetProtectionQuantity(ProtQName::HT_Liver) +
        GetProtectionQuantity(ProtQName::HT_OesophagusWhole) +
        GetProtectionQuantity(ProtQName::HT_Thyroid)
        ) * .04
        +
        (
        GetProtectionQuantity(ProtQName::HT_BoneSurface_byMassRatio) +
        GetProtectionQuantity(ProtQName::HT_Brain) +
        GetProtectionQuantity(ProtQName::HT_SalivaryGlands) +
        GetProtectionQuantity(ProtQName::HT_SkinWhole)
        ) * .01;
}

G4double MRCPDoseCalculation::CalculateAverageDose(const std::vector<SubModelName>& subModels)
{
    G4double totalEDep = 0.;
    G4double totalMass = 0.;
    for(const auto& subModel: subModels)
    {
        G4int subModelID = static_cast<G4int>(subModel);
        totalMass += fTETModel->GetSubModelMass(subModelID);

        if(fEDepMap->find(subModelID) != fEDepMap->end())
            totalEDep += *(fEDepMap->at(subModelID));
    }

    return (totalMass == 0.) ? 0. : totalEDep/totalMass;
}

void MRCPDoseCalculation::Create_protQ_subModels_Map()
{
    // 1. Red bone marrow (Active marrow)
    protQ_subModels_Map[ProtQName::HT_RedBoneMarrow_byDRF] =
    {
        SubModelName::mHumeriUS,
        SubModelName::mClavicleS,
        SubModelName::mCraniumS,
        SubModelName::mFemoraUS,
        SubModelName::mMandibleS,
        SubModelName::mPelvisS,
        SubModelName::mRibsS,
        SubModelName::mScapulaeS,
        SubModelName::mCervicalS,
        SubModelName::mThoracicS,
        SubModelName::mLumbarS,
        SubModelName::mSacrumS,
        SubModelName::mSternumS
    };
    protQ_subModels_Map[ProtQName::HT_RedBoneMarrow_byMassRatio] =
        protQ_subModels_Map.at(ProtQName::HT_RedBoneMarrow_byDRF);

    // 2. Colon
    protQ_subModels_Map[ProtQName::HT_ColonTarget] =
    {
        SubModelName::mAscColonW_280_300,
        SubModelName::mTransColonWR_280_300,
        SubModelName::mTransColonWL_280_300,
        SubModelName::mDscColonW_280_300,
        SubModelName::mSigColonW_280_300
    };
    // 2*. Colon (whole)
    protQ_subModels_Map[ProtQName::HT_ColonWhole] =
    {
        SubModelName::mAscColonW_0_280,
        SubModelName::mAscColonW_280_300,
        SubModelName::mAscColonW_300_surf,
        SubModelName::mTransColonWR_0_280,
        SubModelName::mTransColonWR_280_300,
        SubModelName::mTransColonWR_300_surf,
        SubModelName::mTransColonWL_0_280,
        SubModelName::mTransColonWL_280_300,
        SubModelName::mTransColonWL_300_surf,
        SubModelName::mDscColonW_0_280,
        SubModelName::mDscColonW_280_300,
        SubModelName::mDscColonW_300_surf,
        SubModelName::mSigColonW_0_280,
        SubModelName::mSigColonW_280_300,
        SubModelName::mSigColonW_300_surf,
        SubModelName::mRectumW
    };

    // 3. Lung
    protQ_subModels_Map[ProtQName::HT_Lungs] =
    {
        SubModelName::mLungR,
        SubModelName::mLungL
    };

    // 4. Stomach
    protQ_subModels_Map[ProtQName::HT_StomachTarget] =
    {
        SubModelName::mStomach_60_100
    };
    // 4*. Stomach (whole)
    protQ_subModels_Map[ProtQName::HT_StomachWhole] =
    {
        SubModelName::mStomach_0_60,
        SubModelName::mStomach_60_100,
        SubModelName::mStomach_100_300,
        SubModelName::mStomach_300_surf
    };

    // 5. Breast
    protQ_subModels_Map[ProtQName::HT_Breast] =
    {
        SubModelName::mBreastRA,
        SubModelName::mBreastLA,
        SubModelName::mBreastRG,
        SubModelName::mBreastLG
    };

    // 6. Gonads
    protQ_subModels_Map[ProtQName::HT_Gonads] =
    {
        SubModelName::mGonadsR,
        SubModelName::mGonadsL
    };

    // 7. Bladder
    protQ_subModels_Map[ProtQName::HT_Bladder] =
    {
        SubModelName::mUrinarybladder
    };

    // 8. Oesophagus
    protQ_subModels_Map[ProtQName::HT_OesophagusTarget] =
    {
        SubModelName::mOesophagus_190_200
    };
    // 8*. Oesophagus (whole)
    protQ_subModels_Map[ProtQName::HT_OesophagusWhole] =
    {
        SubModelName::mOesophagus_0_190,
        SubModelName::mOesophagus_190_200,
        SubModelName::mOesophagus_200_surf
    };

    // 9. Liver
    protQ_subModels_Map[ProtQName::HT_Liver] =
    {
        SubModelName::mLiver
    };

    // 10. Thyroid
    protQ_subModels_Map[ProtQName::HT_Thyroid] =
    {
        SubModelName::mThyroid
    };

    // 11. Bone surface
    protQ_subModels_Map[ProtQName::HT_BoneSurface_byDRF] =
    {
        SubModelName::mHumeriUS,
        SubModelName::mHumeriLS,
        SubModelName::mHumeriM,
        SubModelName::mUlnaeS,
        SubModelName::mUlnaeM,
        SubModelName::mHandsS,
        SubModelName::mClavicleS,
        SubModelName::mCraniumS,
        SubModelName::mFemoraUS,
        SubModelName::mFemoraLS,
        SubModelName::mFemoraM,
        SubModelName::mTibiaeS,
        SubModelName::mTibiaeM,
        SubModelName::mFootS,
        SubModelName::mMandibleS,
        SubModelName::mPelvisS,
        SubModelName::mRibsS,
        SubModelName::mScapulaeS,
        SubModelName::mCervicalS,
        SubModelName::mThoracicS,
        SubModelName::mLumbarS,
        SubModelName::mSacrumS,
        SubModelName::mSternumS
    };
    protQ_subModels_Map[ProtQName::HT_BoneSurface_byMassRatio] =
        protQ_subModels_Map.at(ProtQName::HT_BoneSurface_byDRF);

    // 12. Brain
    protQ_subModels_Map[ProtQName::HT_Brain] =
    {
        SubModelName::mBrain
    };

    // 13. Salivary glands
    protQ_subModels_Map[ProtQName::HT_SalivaryGlands] =
    {
        SubModelName::mSalivary_glandR,
        SubModelName::mSalivary_glandL
    };

    // 14. Skin
    protQ_subModels_Map[ProtQName::HT_SkinTarget] =
    {
        SubModelName::mSkin_50_100
    };
    // 14*. Skin (whole)
    protQ_subModels_Map[ProtQName::HT_SkinWhole] =
    {
        SubModelName::mSkin_Osurf_50,
        SubModelName::mSkin_50_100,
        SubModelName::mSkin_100_Isurf
    };

    // 15. Adrenals
    protQ_subModels_Map[ProtQName::HT_Adrenals] =
    {
        SubModelName::mAdrenalR,
        SubModelName::mAdrenalL
    };

    // 16-1. Extrathracic region 1
    protQ_subModels_Map[ProtQName::HT_ET1Target] =
    {
        SubModelName::mET1_40_50
    };
    // 16-2. Extrathracic region 2
    protQ_subModels_Map[ProtQName::HT_ET2Target] =
    {
        SubModelName::mET2_40_50
    };
    // 16-1*. Extrathracic region 1 (whole)
    protQ_subModels_Map[ProtQName::HT_ET1Whole] =
    {
        SubModelName::mET1_0_8,
        SubModelName::mET1_8_40,
        SubModelName::mET1_40_50,
        SubModelName::mET1_50_Surf
    };
    // 16-2*. Extrathracic region 2 (whole)
    protQ_subModels_Map[ProtQName::HT_ET2Whole] =
    {
        SubModelName::mET2_0_40,
        SubModelName::mET2_40_50,
        SubModelName::mET2_50_55,
        SubModelName::mET2_55_65,
        SubModelName::mET2_65_Surf
    };

    // 17. Gall bladder
    protQ_subModels_Map[ProtQName::HT_GallBladder] =
    {
        SubModelName::mGallbladder
    };

    // 18. Heart
    protQ_subModels_Map[ProtQName::HT_Heart] =
    {
        SubModelName::mHeart
    };

    // 19. Kidneys
    protQ_subModels_Map[ProtQName::HT_Kidneys] =
    {
        SubModelName::mKidneyRCortex,
        SubModelName::mKidneyRMedulla,
        SubModelName::mKidneyRPelvis,
        SubModelName::mKidneyLCortex,
        SubModelName::mKidneyLMedulla,
        SubModelName::mKidneyLPelvis
    };

    // 20. Lymphatic nodes
    protQ_subModels_Map[ProtQName::HT_LymphaticNodes] =
    {
        SubModelName::mLymphET,
        SubModelName::mLymphCer,
        SubModelName::mLymphAxi,
        SubModelName::mLymphBre,
        SubModelName::mLymphTho,
        SubModelName::mLymphCub,
        SubModelName::mLymphMes,
        SubModelName::mLymphIng,
        SubModelName::mLymphPop
    };

    // 21. Muscle
    protQ_subModels_Map[ProtQName::HT_Muscle] =
    {
        SubModelName::mMuscle
    };

    // 22. Oral mucosa
    protQ_subModels_Map[ProtQName::HT_OralMucosa] =
    {
        SubModelName::mTongueLower_OralM,
        SubModelName::mMouthFloor_OralM,
        SubModelName::mMouthLibCheek_OralM
    };

    // 23. Pancreas
    protQ_subModels_Map[ProtQName::HT_Pancreas] =
    {
        SubModelName::mPancreas
    };

    // 24. Prostate/Uterus
    protQ_subModels_Map[ProtQName::HT_ProstateUtreus] =
    {
        SubModelName::mProstate_Uterus
    };

    // 25. Small intestine
    protQ_subModels_Map[ProtQName::HT_SmallIntestineTarget] =
    {
        SubModelName::mSI_130_150
    };
    // 25*. Small intestine (whole)
    protQ_subModels_Map[ProtQName::HT_SmallIntestineWhole] =
    {
        SubModelName::mSI_0_130,
        SubModelName::mSI_130_150,
        SubModelName::mSI_150_200,
        SubModelName::mSI_200_surf
    };

    // 26. Spleen
    protQ_subModels_Map[ProtQName::HT_Spleen] =
    {
        SubModelName::mSpleen
    };

    // 27. Thymus
    protQ_subModels_Map[ProtQName::HT_Thymus] =
    {
        SubModelName::mThymus
    };

    // 28. Eye lens
    protQ_subModels_Map[ProtQName::HT_EyeLensTarget] =
    {
        SubModelName::mSensLensR,
        SubModelName::mSensLensL
    };
    // 28*. Eye lens (whole)
    protQ_subModels_Map[ProtQName::HT_EyeLensWhole] =
    {
        SubModelName::mSensLensR,
        SubModelName::mSensLensL,
        SubModelName::mInsensLensR,
        SubModelName::mInsensLensL,
    };

    // 29. Whole body
    for(G4int i = static_cast<G4int>(SubModelName::FIRST);
        i <= static_cast<G4int>(SubModelName::LAST); ++i)
    {
        SubModelName subModel = static_cast<SubModelName>(i);
        switch(subModel)
        {
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
        default:
            protQ_subModels_Map[ProtQName::WholeBodyDose].push_back(subModel);
        }
    }
}

void MRCPDoseCalculation::Create_SimpleProtQ_Vector()
{
    simpleProtQ_Vector =
    {
        ProtQName::HT_ColonTarget,
        ProtQName::HT_Lungs,
        ProtQName::HT_StomachTarget,
        ProtQName::HT_Breast,
        ProtQName::HT_Gonads,
        ProtQName::HT_Bladder,
        ProtQName::HT_OesophagusTarget,
        ProtQName::HT_Liver,
        ProtQName::HT_Thyroid,
        ProtQName::HT_Brain,
        ProtQName::HT_SalivaryGlands,
        ProtQName::HT_SkinTarget,
        ProtQName::HT_Adrenals,
        ProtQName::HT_GallBladder,
        ProtQName::HT_Heart,
        ProtQName::HT_Kidneys,
        ProtQName::HT_LymphaticNodes,
        ProtQName::HT_Muscle,
        ProtQName::HT_OralMucosa,
        ProtQName::HT_Pancreas,
        ProtQName::HT_ProstateUtreus,
        ProtQName::HT_SmallIntestineTarget,
        ProtQName::HT_Spleen,
        ProtQName::HT_Thymus,
        ProtQName::HT_ET1Target,
        ProtQName::HT_ET2Target,
        ProtQName::HT_EyeLensTarget,
        ProtQName::HT_ColonWhole,
        ProtQName::HT_StomachWhole,
        ProtQName::HT_OesophagusWhole,
        ProtQName::HT_SkinWhole,
        ProtQName::HT_SmallIntestineWhole,
        ProtQName::HT_ET1Whole,
        ProtQName::HT_ET2Whole,
        ProtQName::HT_EyeLensWhole,
        ProtQName::WholeBodyDose
    };
}

G4String MRCPDoseCalculation::ProtQNameString(ProtQName protQ)
{
    switch (protQ) {
    case ProtQName::HT_RedBoneMarrow_byDRF: return "HT_RedBoneMarrow_byDRF";
    case ProtQName::HT_ColonTarget: return "HT_ColonTarget";
    case ProtQName::HT_Lungs: return "HT_Lungs";
    case ProtQName::HT_StomachTarget: return "HT_StomachTarget";
    case ProtQName::HT_Breast: return "HT_Breast";
    case ProtQName::HT_RemainderTarget: return "HT_RemainderTarget";
    case ProtQName::HT_Gonads: return "HT_Gonads";
    case ProtQName::HT_Bladder: return "HT_Bladder";
    case ProtQName::HT_OesophagusTarget: return "HT_OesophagusTarget";
    case ProtQName::HT_Liver: return "HT_Liver";
    case ProtQName::HT_Thyroid: return "HT_Thyroid";
    case ProtQName::HT_BoneSurface_byDRF: return "HT_BoneSurface_byDRF";
    case ProtQName::HT_Brain: return "HT_Brain";
    case ProtQName::HT_SalivaryGlands: return "HT_SalivaryGlands";
    case ProtQName::HT_SkinTarget: return "HT_SkinTarget";
    case ProtQName::HT_Adrenals: return "HT_Adrenals";
    case ProtQName::HT_ExtrathoracicTarget: return "HT_ExtrathoracicTarget";
    case ProtQName::HT_GallBladder: return "HT_GallBladder";
    case ProtQName::HT_Heart: return "HT_Heart";
    case ProtQName::HT_Kidneys: return "HT_Kidneys";
    case ProtQName::HT_LymphaticNodes: return "HT_LymphaticNodes";
    case ProtQName::HT_Muscle: return "HT_Muscle";
    case ProtQName::HT_OralMucosa: return "HT_OralMucosa";
    case ProtQName::HT_Pancreas: return "HT_Pancreas";
    case ProtQName::HT_ProstateUtreus: return "HT_ProstateUtreus";
    case ProtQName::HT_SmallIntestineTarget: return "HT_SmallIntestineTarget";
    case ProtQName::HT_Spleen: return "HT_Spleen";
    case ProtQName::HT_Thymus: return "HT_Thymus";
    case ProtQName::HT_ET1Target: return "HT_ET1Target";
    case ProtQName::HT_ET2Target: return "HT_ET2Target";
    case ProtQName::HT_RedBoneMarrow_byMassRatio: return "HT_RedBoneMarrow_byMassRatio";
    case ProtQName::HT_BoneSurface_byMassRatio: return "HT_BoneSurface_byMassRatio";
    case ProtQName::HT_EyeLensTarget: return "HT_EyeLensTarget";
    case ProtQName::HT_ColonWhole: return "HT_ColonWhole";
    case ProtQName::HT_StomachWhole: return "HT_StomachWhole";
    case ProtQName::HT_RemainderWhole: return "HT_RemainderWhole";
    case ProtQName::HT_OesophagusWhole: return "HT_OesophagusWhole";
    case ProtQName::HT_SkinWhole: return "HT_SkinWhole";
    case ProtQName::HT_ExtrathoracicWhole: return "HT_ExtrathoracicWhole";
    case ProtQName::HT_SmallIntestineWhole: return "HT_SmallIntestineWhole";
    case ProtQName::HT_ET1Whole: return "HT_ET1Whole";
    case ProtQName::HT_ET2Whole: return "HT_ET2Whole";
    case ProtQName::HT_EyeLensWhole: return "HT_EyeLensWhole";
    case ProtQName::EffectiveDose: return "EffectiveDose";
    case ProtQName::EffectiveDose_byDRFWhole: return "EffectiveDose_byDRFWhole";
    case ProtQName::EffectiveDose_byMassRatioTarget: return "EffectiveDose_byMassRatioTarget";
    case ProtQName::EffectiveDose_byMassRatioWhole: return "EffectiveDose_byMassRatioWhole";
    case ProtQName::WholeBodyDose: return "WholeBodyDose";
    default: return "INVALID_PROTQNAME";
    }
}

void MRCPDoseCalculation::SetEDepMap(std::map<G4int, G4double*>* eDepMap)
{
    if(!eDepMap)
    {
        G4Exception("MRCPDoseCalculation::SetEDepMap()", "", JustWarning,
            G4String("      invalid eDepMap." ).c_str());
        return;
    }
    fEDepMap = eDepMap;
}

void MRCPDoseCalculation::SetRBMDoseMap(std::map<G4int, G4double*>* RBMDoseMap)
{
    if(!RBMDoseMap)
    {
        G4Exception("MRCPDoseCalculation::SetRBMDoseMap()", "", JustWarning,
            G4String("      invalid RBMDoseMap." ).c_str());
        return;
    }
    fRBMDoseMap = RBMDoseMap;
}

void MRCPDoseCalculation::SetBSDoseMap(std::map<G4int, G4double*>* BSDoseMap)
{
    if(!BSDoseMap)
    {
        G4Exception("MRCPDoseCalculation::SetBSDoseMap()", "", JustWarning,
            G4String("      invalid BSDoseMap." ).c_str());
        return;
    }
    fBSDoseMap = BSDoseMap;
}

void MRCPDoseCalculation::Clear()
{
    protQ_Map.clear();

    if(fEDepMap) fEDepMap = nullptr;
    if(fRBMDoseMap) fRBMDoseMap = nullptr;
    if(fBSDoseMap) fBSDoseMap = nullptr;
}
