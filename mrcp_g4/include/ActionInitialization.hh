#ifndef ActionInitialization_hh_
#define ActionInitialization_hh_

#include "G4VUserActionInitialization.hh"

class ActionInitialization: public G4VUserActionInitialization
{
public:
    ActionInitialization();
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;
};

#endif
