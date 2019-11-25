#include "Primary_GPS.hh"

Primary_GPS::Primary_GPS()
: G4VUserPrimaryGeneratorAction()
{
    fPrimary = new G4GeneralParticleSource;
}

Primary_GPS::~Primary_GPS()
{
    delete fPrimary;
}

void Primary_GPS::GeneratePrimaries(G4Event* anEvent)
{
    fPrimary->GeneratePrimaryVertex(anEvent);
}
