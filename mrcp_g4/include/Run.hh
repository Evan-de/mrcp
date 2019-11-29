#ifndef RUN_HH
#define RUN_HH

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4THitsMap.hh"

class Primary_ParticleGun;

class Run: public G4Run
{
public:
    Run();
    virtual ~Run();

    virtual void RecordEvent(const G4Event*);
    virtual void Merge(const G4Run*);

    const std::map< G4String, std::pair<G4double, G4double> >& GetProtQ() const { return fProtQ; }

private:
    G4int fPhantomDose_HCID;
    std::map< G4String, std::pair<G4double, G4double> > fProtQ;

    const Primary_ParticleGun* fPrimaryGeneratorAction;
};

#endif
