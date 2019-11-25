#ifndef Primary_GPS_hh_
#define Primary_GPS_hh_

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"

class Primary_GPS: public G4VUserPrimaryGeneratorAction
{
public:
    Primary_GPS();
    virtual ~Primary_GPS();

    virtual void GeneratePrimaries(G4Event*);

private:
    G4GeneralParticleSource* fPrimary;
};

#endif
