#include "MRCPPSDoseDeposit.hh"
#include "TETModelStore.hh"
#include "MRCPModel.hh"

MRCPPSDoseDeposit::MRCPPSDoseDeposit(G4String name, G4String phantomName)
: G4VPrimitiveScorer(name), fHCID(-1), fEvtMap(nullptr), fDRFFlag(false)
{
    fMRCPModel = dynamic_cast<MRCPModel*>(
        TETModelStore::GetInstance()->GetTETModel(phantomName)
        );
    if(!fMRCPModel)
        G4Exception("MRCPPSDoseDeposit::MRCPPSDoseDeposit()", "", FatalErrorInArgument,
            G4String("      invalid MRCPModel '" + phantomName + "'" ).c_str());
}

G4bool MRCPPSDoseDeposit::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{
    G4double eDep = aStep->GetTotalEnergyDeposit();
    if(0.==eDep) return false;

    G4double particleWeight = aStep->GetPreStepPoint()->GetWeight();

    // --- Averaged subModelDose --- //
    G4int subModelID = GetIndex(aStep);
    G4double mass = fMRCPModel->GetSubModelMass(subModelID);

    G4double dose = eDep / mass;
    dose *= particleWeight;
    fEvtMap->add(subModelID, dose);

    // --- DRF based bone dose --- //
    if(!fDRFFlag) return true; // DRF file has not been imported.
    if(subModelRBMDRF_Map.find(subModelID) == subModelRBMDRF_Map.end())
        return true; // this submodel is not a kind of bone.

    G4double volume = fMRCPModel->GetSubModelVolume(subModelID);
    G4double stepLength = aStep->GetStepLength();
    G4double cellFluence = stepLength / volume;
    G4double kineticEnergy = aStep->GetPreStepPoint()->GetKineticEnergy(); // KE before interaction should be used (prestep).
    G4double rbmDRF = LogInterp(kineticEnergy, energyBin_DRF, subModelRBMDRF_Map.at(subModelID));
    G4double bsDRF = LogInterp(kineticEnergy, energyBin_DRF, subModelBSDRF_Map.at(subModelID));

    G4double rbmDose = cellFluence * rbmDRF;
    rbmDose *= particleWeight;
    fEvtMap->add((-subModelID)-1000, rbmDose); // RBM doses by DRF will be stored at -10xx

    G4double bsDose = cellFluence * bsDRF;
    bsDose *= particleWeight;
    fEvtMap->add((-subModelID)-2000, bsDose); // BS doses by DRF will be stored at -20xx

    return true;
}

void MRCPPSDoseDeposit::Initialize(G4HCofThisEvent* HCE)
{
    fEvtMap = new G4THitsMap<G4double>(GetMultiFunctionalDetector()->GetName(), GetName());

    if(fHCID<0) fHCID = GetCollectionID(0);
    HCE->AddHitsCollection(fHCID, static_cast<G4VHitsCollection*>(fEvtMap));
}

G4int MRCPPSDoseDeposit::GetIndex(G4Step* aStep)
{
    G4int copyNo = aStep->GetPreStepPoint()->GetTouchable()->GetCopyNumber();
    return fMRCPModel->GetSubModelID(copyNo);
}

void MRCPPSDoseDeposit::ImportBoneDRFData(const G4String& boneDRFFilePath)
{
    // --- Open colour file --- //
    std::ifstream ifs(boneDRFFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportBoneDRFData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + boneDRFFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening bone DRF data file '" << boneDRFFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;
    G4double DRFValue;

    // Read data lines
    while(!ifs.eof())
    {
        ifs >> subModelID;

        for(size_t i = 0; i<energyBin_DRF.size(); ++i)
        {
            ifs >> DRFValue;
            DRFValue *= gray*m2;
            subModelRBMDRF_Map[subModelID].push_back(DRFValue);
        }

        for(size_t i = 0; i<energyBin_DRF.size(); ++i)
        {
            ifs >> DRFValue;
            DRFValue *= gray*m2;
            subModelBSDRF_Map[subModelID].push_back(DRFValue);
        }
    }

    // --- Close the file --- //
    ifs.close();

    fDRFFlag = true;
}

