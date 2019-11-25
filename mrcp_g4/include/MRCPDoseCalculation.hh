#ifndef MRCPDoseCalculation_hh_
#define MRCPDoseCalculation_hh_

#include "globals.hh"

#include <map>
#include <vector>

class TETModel;

enum class SubModelName_PRIV; // SubModel names of privious MRCP
enum class ProtQName; // Protection quantities

using SubModelName = SubModelName_PRIV;

class MRCPDoseCalculation
{
public:
    MRCPDoseCalculation(G4String tetModelName);

    // Usage
    // 1. Set@@Map(), 2. CalculateProtectionQuantities(), 3. GetProtectionQuantities(). 4. Clear()

    // SetEDepMap: protection quantities calculated by simple average. (incl. Extrathoracic & WholeBody)
    // SetRBMDoseMap: Red bone marrow Dose (by Mass ratio & DRF)
    // SetBSDoseMap: Bone surface Dose (by Mass ratio & DRF)
    void SetEDepMap(std::map<G4int, G4double*>* eDepMap);
    void SetRBMDoseMap(std::map<G4int, G4double*>* RBMDoseMap);
    void SetBSDoseMap(std::map<G4int, G4double*>* BSDoseMap);

    // Calculate protection quantities using the maps set by user
    void CalculateProtectionQuantities();

    // Get Protection quantity
    G4double GetProtectionQuantity(ProtQName protQ)
    {
        if(protQ_Map.find(protQ) == protQ_Map.end())
            return 0.;
        else
            return protQ_Map.at(protQ);
    }

    // Clear data, EDepMap, RBMDoseMap, and BSDoseMap
    void Clear();

    // Convert ProtQName to G4String
    static G4String ProtQNameString(ProtQName protQ);

private:
    void Create_protQ_subModels_Map();
    void Create_SimpleProtQ_Vector();
    G4double CalculateAverageDose(const std::vector<SubModelName>& subModels);

    TETModel* fTETModel;

    std::map<G4int, G4double*>* fEDepMap;
    std::map<G4int, G4double*>* fRBMDoseMap;
    std::map<G4int, G4double*>* fBSDoseMap;
    std::map<ProtQName, G4double> protQ_Map;

    std::map< ProtQName, std::vector<SubModelName> > protQ_subModels_Map;
    std::vector<ProtQName> simpleProtQ_Vector;
};

enum class SubModelName_PRIV // Must be identical to material file
{
    FIRST = 1,

    mHumeriC = 1,
    mHumeriUS = 2,
    mHumeriLS = 3,
    mHumeriM = 4,
    mUlnaeC = 5,
    mUlnaeS = 6,
    mUlnaeM = 7,
    mHandsC = 8,
    mHandsS = 9,
    mClavicleC = 10,
    mClavicleS = 11,
    mCraniumC = 12,
    mCraniumS = 13,
    mFemoraC = 14,
    mFemoraUS = 15,
    mFemoraLS = 16,
    mFemoraM = 17,
    mTibiaeC = 18,
    mTibiaeS = 19,
    mTibiaeM = 20,
    mFootC = 21,
    mFootS = 22,
    mMandibleC = 23,
    mMandibleS = 24,
    mPelvisC = 25,
    mPelvisS = 26,
    mRibsC = 27,
    mRibsS = 28,
    mScapulaeC = 29,
    mScapulaeS = 30,
    mCervicalC = 31,
    mCervicalS = 32,
    mThoracicC = 33,
    mThoracicS = 34,
    mLumbarC = 35,
    mLumbarS = 36,
    mSacrumC = 37,
    mSacrumS = 38,
    mSternumC = 39,
    mSternumS = 40,
    mTeeth = 41,
    mCostalCartilage = 42,
    mDiscsCartilage = 43,
    mTongueUpper_food = 44,
    mTongueLower = 45,
    mTongueLower_OralM = 46,
    mMouthFloor_OralM = 47,
    mMouthLibCheek_OralM = 48,
    mRetentionTeeth = 49,
    mOesophagus_0_190 = 50,
    mOesophagus_190_200 = 51,
    mOesophagus_200_surf = 52,
    mOesophagusC = 53,
    mStomach_0_60 = 54,
    mStomach_60_100 = 55,
    mStomach_100_300 = 56,
    mStomach_300_surf = 57,
    mStomachC = 58,
    mSI_0_130 = 59,
    mSI_130_150 = 60,
    mSI_150_200 = 61,
    mSI_200_surf = 62,
    mSIContent__500_0 = 63,
    mSIContent_center__500 = 64,
    mAscColonW_0_280 = 65,
    mAscColonW_280_300 = 66,
    mAscColonW_300_surf = 67,
    mAscColon_content = 68,
    mTransColonWR_0_280 = 69,
    mTransColonWR_280_300 = 70,
    mTransColonWR_300_surf = 71,
    mTransColonR_content = 72,
    mTransColonWL_0_280 = 73,
    mTransColonWL_280_300 = 74,
    mTransColonWL_300_surf = 75,
    mTransColonL_content = 76,
    mDscColonW_0_280 = 77,
    mDscColonW_280_300 = 78,
    mDscColonW_300_surf = 79,
    mDscColon_content = 80,
    mSigColonW_0_280 = 81,
    mSigColonW_280_300 = 82,
    mSigColonW_300_surf = 83,
    mSigColonW_content = 84,
    mRectumW = 85,
    mSalivary_glandR = 86,
    mSalivary_glandL = 87,
    mTonsils = 88,
    mLiver = 89,
    mGallbladder = 90,
    mGallbladderC = 91,
    mPancreas = 92,
    mHeart = 93,
    mHeartC = 94,
    mBlood_arteries = 95,
    mBlood_veins = 96,
    mLymphET = 97,
    mLymphCer = 98,
    mLymphAxi = 99,
    mLymphBre = 100,
    mLymphTho = 101,
    mLymphCub = 102,
    mLymphMes = 103,
    mLymphIng = 104,
    mLymphPop = 105,
    mKidneyRCortex = 106,
    mKidneyRMedulla = 107,
    mKidneyRPelvis = 108,
    mKidneyLCortex = 109,
    mKidneyLMedulla = 110,
    mKidneyLPelvis = 111,
    mUreterR = 112,
    mUreterL = 113,
    mUrinarybladder = 114,
    mUrinarybladderC = 115,
    mGonadsR = 116,
    mGonadsL = 117,
    mProstate_Uterus = 118,
    mAdrenalR = 119,
    mAdrenalL = 120,
    mBreastRA = 121,
    mBreastLA = 122,
    mBreastRG = 123,
    mBreastLG = 124,
    mSkin_Osurf_50 = 125,
    mSkin_50_100 = 126,
    mSkin_100_Isurf = 127,
    mCorneaR = 128,
    mCorneaL = 129,
    mAqueousR = 130,
    mAqueousL = 131,
    mVitreousR = 132,
    mVitreousL = 133,
    mSensLensR = 134,
    mSensLensL = 135,
    mInsensLensR = 136,
    mInsensLensL = 137,
    mBrain = 138,
    mMuscle = 139,
    mPituitary_gland = 140,
    mSpinal_cord = 141,
    mSpleen = 142,
    mThymus = 143,
    mThyroid = 144,
    mRST = 145,
    mAir = 146,
    mET1_0_8 = 147,
    mET1_8_40 = 148,
    mET1_40_50 = 149,
    mET1_50_Surf = 150,
    mET2__15_0 = 151,
    mET2_0_40 = 152,
    mET2_40_50 = 153,
    mET2_50_55 = 154,
    mET2_55_65 = 155,
    mET2_65_Surf = 156,
    mTrachea = 157,
    mLungR = 158,
    mLungL = 159,
    mBB_gen1_m11_m6 = 160,
    mBB_gen1_m6_0 = 161,
    mBB_gen1_0_10 = 162,
    mBB_gen1_10_35 = 163,
    mBB_gen1_35_40 = 164,
    mBB_gen1_40_50 = 165,
    mBB_gen1_50_60 = 166,
    mBB_gen1_60_70 = 167,
    mBB_gen1_70_Surf = 168,
    mUrinarybladder75 = 169,
    mUrinarybladder118 = 170,

    LAST = 170
};

enum class ProtQName
{
    // wT = .12
    HT_RedBoneMarrow_byDRF, HT_ColonTarget, HT_Lungs,
    HT_StomachTarget, HT_Breast, HT_RemainderTarget,

    // wT = .08
    HT_Gonads, // testes or ovaries

    // wT = .04
    HT_Bladder, HT_OesophagusTarget, HT_Liver, HT_Thyroid,

    // wT = .01
    HT_BoneSurface_byDRF, HT_Brain, HT_SalivaryGlands, HT_SkinTarget,

    // Remainder
    HT_Adrenals, HT_ExtrathoracicTarget, HT_GallBladder,
    HT_Heart, HT_Kidneys, HT_LymphaticNodes, HT_Muscle,
    HT_OralMucosa, HT_Pancreas, HT_ProstateUtreus,
    HT_SmallIntestineTarget, HT_Spleen, HT_Thymus,

    // Sub-region for Extrathoracic
    HT_ET1Target, HT_ET2Target,

    // Bone dose by mass ratio
    HT_RedBoneMarrow_byMassRatio,
    HT_BoneSurface_byMassRatio,

    // Other organs
    HT_EyeLensTarget,

    // Target+@ (thin organs)
    HT_ColonWhole,
    HT_StomachWhole,
    HT_RemainderWhole,
    HT_OesophagusWhole,
    HT_SkinWhole,
    HT_ExtrathoracicWhole,
    HT_SmallIntestineWhole,
    HT_ET1Whole,
    HT_ET2Whole,
    HT_EyeLensWhole,

    // Other Quantities
    EffectiveDose, // DRF for bones, Target layer for thin organs
    EffectiveDose_byDRFWhole, // DRF for bones, Whole organ for thin organs
    EffectiveDose_byMassRatioTarget, // Mass ratio for bones, Target layer for thin organs
    EffectiveDose_byMassRatioWhole, // Mass ratio for bones, whole organ for thin organs
    WholeBodyDose
};

#endif
