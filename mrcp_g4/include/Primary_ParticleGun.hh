#ifndef Primary_ParticleGun_hh_
#define Primary_ParticleGun_hh_

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Event.hh"

#include "G4GenericMessenger.hh"

#include "G4ParticleTable.hh"
#include "G4RandomDirection.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"

#include <fstream>

class NuclideSource;

class Primary_ParticleGun: public G4VUserPrimaryGeneratorAction
{
public:
    Primary_ParticleGun();
    virtual ~Primary_ParticleGun();

    virtual void GeneratePrimaries(G4Event*);

    G4double GetPrimaryWeight() const { return fWeight; }
    G4String GetPrimaryInfo() const
    {
        std::stringstream ss;
        ss << fNuclideName << "@" << fPrimary->GetParticlePosition()/cm << "cm";
        return ss.str();
    }

private:
    void DefineCommands();

    void SetNuclide(const G4String& nuclideName);

    G4ParticleGun* fPrimary;
    G4GenericMessenger* fMessenger;
    G4double fWeight;

    G4String fNuclideName;
    NuclideSource* fNuclideSource;

    G4String fAngleBiasingPVName;
    G4ThreeVector fAngleBiasingPVPosition;
    std::vector<G4ThreeVector> fAngleBiasingPVBoxVerticies;
    G4double fAngleBiasingPVBoxMag2;
};

G4ThreeVector RandomDirectionFromTo(const G4ThreeVector& referencePoint,
                                    const G4String& physicalVolumeName,
                                    G4double& particleWeight,
                                    G4double margin = 0.);
// --- Nuclide source --- //
// There should be a file "./nuclides/{nuclideName}".
// The file should include data "{ParticleName} {KineticEnergy in MeV} {YieldPerDecay}" in a row.
// The line starting with a '#' character in the file will be commented.
class NuclideSource
{
public:
    NuclideSource(G4String nuclideName);
    ~NuclideSource() {}

    std::tuple<G4String, G4double, G4double> SampleDecayProduct(); // ParticleName, Energy, Weight

private:
    void Normalize();

    G4bool fNormalized;
    G4double fTotalYield;

    std::vector< std::tuple<G4String, G4double, G4double> > fYield_Vector; // ParticleName, Energy, Yield
    std::vector<G4double> fCumulativeProbability_Vector; // normalized
};

#endif
