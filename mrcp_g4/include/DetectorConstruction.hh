#ifndef DetectorConstruction_hh_
#define DetectorConstruction_hh_

#include "G4VUserDetectorConstruction.hh"

#include <experimental/filesystem>

class G4LogicalVolume;
class G4VPhysicalVolume;

class DetectorConstruction: public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(G4String mainPhantom_FilePath);
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

private:
    std::experimental::filesystem::v1::path fMainPhantom_FilePath;
    G4LogicalVolume* fTetLogicalVolume;
};

#endif
