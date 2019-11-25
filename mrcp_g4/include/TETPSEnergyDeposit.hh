#ifndef TETPSEnergyDeposit_hh_
#define TETPSEnergyDeposit_hh_

#include "G4PSEnergyDeposit.hh"

class TETModel;

class TETPSEnergyDeposit: public G4PSEnergyDeposit
{
public:
    TETPSEnergyDeposit(G4String name, G4String tetModelName);
    virtual ~TETPSEnergyDeposit();

protected:
    virtual G4int GetIndex(G4Step*);

private:
    TETModel* fTETModel;
};

#endif

