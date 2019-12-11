#ifndef PRIMARYSAMPLINGHELPER_HH
#define PRIMARYSAMPLINGHELPER_HH

#include "G4RandomDirection.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4VSolid.hh"
#include "G4SystemOfUnits.hh"

#include <list>

// Direction sampling
G4ThreeVector SampleDirectionFromTo(const G4ThreeVector& referencePoint,
                                    const G4String& physicalVolumeName,
                                    G4double& particleWeight,
                                    G4double margin = 0.);

// Particle & energy sampling
class RadioNuclide
{
/************************************************************
 * WARNING                                                  *
 * USE CAREFULLY WHEN YOU SAMPLE PARTICLES WITH CONTINUOUS  *
 * E SPECTRA, SUCH AS BETA DECAY PARTICLE, NEUTRON, ETC.    *
 * This class is available only for *.RAD file format data. *
 * Energy sampling of particles with continuous E spectra   *
 * is not supported yet.                                    *
 * Future development will enable the use of other data     *
 * files provided by ICRP107.                               *
 ***********************************************************/
public:
    RadioNuclide(G4String name, const G4String& decayDataFilePath, G4double branchingRatio = 1.);
    ~RadioNuclide();

    G4String GetRadioNuclideName() const { return fRadioNuclideName; }
    G4double GetBranchingRatio() const { return fBranchingRatio; }

    enum class Radiation;
    struct DecayProduct
    {
        G4String particleName;
        G4double energy;
    };
    using EnergyYieldData = std::map<G4double, G4double>;
    using DecayData = std::map<Radiation, EnergyYieldData>;

    DecayData GetDecayDataInteresting() { return fDecayDataInteresting; }

    void AddRadioactiveDaughter(RadioNuclide* radioNuclide)
    { fRadioactiveDaughters.push_back(radioNuclide); }

private:
    G4String fRadioNuclideName;
    G4double fBranchingRatio;

    std::vector<RadioNuclide*> fRadioactiveDaughters;

    DecayData fDecayData;
    DecayData fDecayDataInteresting;
    std::vector< std::pair<G4double, DecayProduct> > fNormalizedDecayData_Vector;

    void Normalize();
    G4double fTotalYield;
    G4bool fNormalized;

    G4String ICode2G4ParticleName(Radiation radiation);

public:
    void AddInterestingRadiation(Radiation radiation);
    void AddInterestingRadiation(std::vector<Radiation> radiations);
    void RemoveInterestingRadiation(Radiation radiation);
    void SetRadiationEnergyThreshold(Radiation radiation, G4double energy);
    void SetRadiationYieldThreshold(Radiation radiation, G4double yield);
    void ClearInterestingRadiation() { fDecayDataInteresting.clear(); }

    DecayProduct SampleDecayProduct(G4double& particleWeight);
};

enum class RadioNuclide::Radiation // ICODE
{
    Gamma = 1, // gamma rays (G) (including prompt (PG) & delayed gamma (DG) of spontaneous fission)
    Xray = 2, // X rays (X)
    AnnihilationPhoton = 3, // Annihilation photons (AQ)
    BetaPlus = 4, // Beta-plus particles (B+)
    BetaMinus = 5, // Beta-minus particles (B-) (including delayed beta (BD) of spontaneous fission)
    InternalConversionElectron = 6, // Internal conversion electrons (IE)
    AugerElectron = 7, // Auger Electrons (AE)
    Alpha = 8, // Alpha particles (A)
    AlphaRecoilNuclei = 9, // AlphaRecoilNuclei (AR)
    FissionFragment = 10, // Fission fragments (FF)
    Neutron = 11 // Neutrons (N)
};

#endif // PRIMARYSAMPLINGHELPER_HH
