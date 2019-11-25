#include "TETPSBoneDose_byDRF.hh"
#include "TETModelStore.hh"

TETPSBoneDose_byDRF::TETPSBoneDose_byDRF(G4String name, G4String tetModelName, BoneSubModel boneSubModel)
: G4VPrimitiveScorer(name), fBoneDose_HCID(-1), fBoneSubModel(boneSubModel), boneDose_HitsMap(nullptr)
{
    fTETModel = TETModelStore::GetInstance()->GetTETModel(tetModelName);
    if(!fTETModel)
        G4Exception("TETPSBoneDose_byDRF::TETPSBoneDose_byDRF()", "", FatalErrorInArgument,
            G4String("      invalid tetModel '" + tetModelName + "'" ).c_str());
}

TETPSBoneDose_byDRF::~TETPSBoneDose_byDRF()
{}

G4int TETPSBoneDose_byDRF::GetIndex(G4Step* aStep)
{
    G4int copyNo = aStep->GetPreStepPoint()->GetTouchable()->GetCopyNumber();
    return fTETModel->GetSubModelID(copyNo);
}

G4bool TETPSBoneDose_byDRF::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{
    // --- Check if the particle is indirectly ionizing radiation --- //
    auto pd_Gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    auto pd_Neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    auto pd_thisStep = aStep->GetTrack()->GetParticleDefinition();
    if(pd_thisStep!=pd_Gamma && pd_thisStep!=pd_Neutron) return false;

    // --- Get fluence & kinetic energy --- //
    G4int subModelID = GetIndex(aStep);
    G4double cellFluence =
        aStep->GetStepLength() / fTETModel->GetSubModelVolume(subModelID);
    G4double kineticEnergy =
        aStep->GetPreStepPoint()->GetKineticEnergy(); // Must obtain KE before interaction (prestep).

    // --- Calculate dose --- //
    G4double boneDose =
        cellFluence * fTETModel->GetSubModelDRF(subModelID, fBoneSubModel, kineticEnergy);

    // --- Store the dose --- //
    boneDose_HitsMap->add(subModelID, boneDose);

    return true;
}

void TETPSBoneDose_byDRF::Initialize(G4HCofThisEvent* HCE)
{
    boneDose_HitsMap = new G4THitsMap<G4double>(detector->GetName(), GetName());
    if(fBoneDose_HCID<0) fBoneDose_HCID = GetCollectionID(0);
    HCE->AddHitsCollection(fBoneDose_HCID, boneDose_HitsMap);
}

void TETPSBoneDose_byDRF::EndOfEvent(G4HCofThisEvent*)
{}

void TETPSBoneDose_byDRF::clear()
{
    boneDose_HitsMap->clear();
}
