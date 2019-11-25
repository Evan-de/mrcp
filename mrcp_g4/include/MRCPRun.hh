#ifndef MRCPRun_hh_
#define MRCPRun_hh_

#include "MRCPDoseCalculation.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4THitsMap.hh"
#include "G4SystemOfUnits.hh"

#include <vector>
#include <tuple>

class TETModel;
class Primary_ParticleGun;

class MRCPRun: public G4Run
{
public:
    MRCPRun();
    virtual ~MRCPRun();

    virtual void RecordEvent(const G4Event*);
    virtual void Merge(const G4Run*);

    const std::vector< std::tuple<G4String, G4double, G4double> >& GetProtectionQuantities() const
    { return fProtectionQuantities; }

private:
    G4int fEDep_HCID;
    G4int fRBMDose_HCID;
    G4int fBSDose_HCID;

    MRCPDoseCalculation* doseCalculation;
    const Primary_ParticleGun* fPrimaryGeneratorAction;

    std::vector<ProtQName> fInterestProtectionQuantities;
    std::vector< std::tuple<G4String, G4double, G4double> > fProtectionQuantities; // sum & squared sum
};

#endif
