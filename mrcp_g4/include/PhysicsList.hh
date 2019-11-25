#ifndef PhysicsList_hh_
#define PhysicsList_hh_

#include "G4VModularPhysicsList.hh"

class G4VPhysicsConstructor;

class PhysicsList: public G4VModularPhysicsList
{
public:
    PhysicsList();
    virtual ~PhysicsList();

    virtual void SetCuts();
};

#endif
