#ifndef DetectorConstruction_hh_
#define DetectorConstruction_hh_

#include "G4VUserDetectorConstruction.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class DetectorConstruction: public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(G4String mainPhantom_FilePath);
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

    void DefineMaterials();

private:
    G4String fMainPhantom_FilePath;
    G4LogicalVolume* fTetLogicalVolume;
};

#endif
