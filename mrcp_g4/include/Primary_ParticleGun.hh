#ifndef Primary_ParticleGun_hh_
#define Primary_ParticleGun_hh_

#include "PrimarySamplingHelper.hh"

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleTable.hh"
#include "G4RandomDirection.hh"
#include "G4SystemOfUnits.hh"

class Primary_ParticleGun: public G4VUserPrimaryGeneratorAction
{
public:
    Primary_ParticleGun();
    virtual ~Primary_ParticleGun();

    virtual void GeneratePrimaries(G4Event*);

    G4String GetPrimaryInfo() const
    {
        std::stringstream ss;
        if(fRadioNuclide)
            ss << fRadioNuclide->GetRadioNuclideName();
        ss << "@" << fPrimary->GetParticlePosition()/cm << "cm";
        return ss.str();
    }

private:
    G4ParticleGun* fPrimary;
    G4GenericMessenger* fMessenger;

    RadioNuclide* fRadioNuclide;

    G4String fAngleBiasingPVName;

    void SetRadioNuclide(const G4String& radioNuclideName);
    G4String fRadioNuclideName;
};

#endif
