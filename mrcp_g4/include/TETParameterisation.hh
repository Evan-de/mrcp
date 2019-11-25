#ifndef TETParameterisation_hh_
#define TETParameterisation_hh_

#include "G4VPVParameterisation.hh"
#include "G4Tet.hh"
#include "G4Material.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalVolume.hh"

#include <map>

class TETModel;

class TETParameterisation: public G4VPVParameterisation
{
public:
    TETParameterisation(G4String tetModelName);
    virtual ~TETParameterisation();
    
    virtual void ComputeTransformation(const G4int, G4VPhysicalVolume*) const {}

    virtual G4VSolid* ComputeSolid(const G4int copyNo, G4VPhysicalVolume* );
    virtual G4Material* ComputeMaterial(
        const G4int copyNo, G4VPhysicalVolume* phy, const G4VTouchable*);

private:
    TETModel* fTETModel;
    std::map<G4int, G4VisAttributes*> subModelVisAttributes_Map;
};

#endif
