#ifndef TETPSBoneDose_byDRF_hh_
#define TETPSBoneDose_byDRF_hh_

#include "TETModel.hh"

#include "G4VPrimitiveScorer.hh"
#include "G4THitsMap.hh"
#include "G4ParticleTable.hh"

class TETPSBoneDose_byDRF: public G4VPrimitiveScorer
{
public:
    TETPSBoneDose_byDRF(G4String name, G4String tetModelName, BoneSubModel boneSubModel);
    virtual ~TETPSBoneDose_byDRF();

    virtual void Initialize(G4HCofThisEvent*);
    virtual void EndOfEvent(G4HCofThisEvent*);
    virtual void clear();

protected:
    virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*);
    virtual G4int GetIndex(G4Step*);

private:
    G4int fBoneDose_HCID;

    TETModel* fTETModel;
    BoneSubModel fBoneSubModel;
    G4THitsMap<G4double>* boneDose_HitsMap;
};

#endif
