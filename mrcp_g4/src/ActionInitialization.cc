#include "ActionInitialization.hh"

#include "Primary_ParticleGun.hh"
#include "Primary_GPS.hh"

#include "RunAction.hh"
#include "TETSteppingAction.hh"

ActionInitialization::ActionInitialization()
: G4VUserActionInitialization()
{}

ActionInitialization::~ActionInitialization()
{}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction);
}

void ActionInitialization::Build() const
{
    SetUserAction(new Primary_ParticleGun);
//    SetUserAction(new Primary_GPS);

    SetUserAction(new RunAction);
    SetUserAction(new TETSteppingAction);
}
