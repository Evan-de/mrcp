#ifndef TETModel_hh_
#define TETModel_hh_

#include "G4ThreeVector.hh"
#include "G4Tet.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4IntersectionSolid.hh"
#include "Randomize.hh"

#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>

namespace
{
const std::vector<G4double> energyBin_DRF =
{
    0.010*MeV, 0.015*MeV, 0.020*MeV, 0.030*MeV, 0.040*MeV,
    0.050*MeV, 0.060*MeV, 0.080*MeV, 0.10 *MeV, 0.15 *MeV,
    0.20 *MeV, 0.30 *MeV, 0.40 *MeV, 0.50 *MeV, 0.60 *MeV,
    0.80 *MeV, 1.0  *MeV, 1.5  *MeV, 2.0  *MeV, 3.0  *MeV,
    4.0  *MeV, 5.0  *MeV, 6.0  *MeV, 8.0  *MeV, 10.0 *MeV
};
}

enum class BoneSubModel { RBM, BS };

class TETModel
{
public:
    TETModel(G4String name,
        const G4String& nodeFilePath, const G4String& eleFilePath,
        const G4String& materialFilePath = "", const G4String& colourFilePath = "",
        const G4String& boneMassRatioFilePath = "", const G4String& boneDRFFilePath = "");

    // --- TETModel information --- //
    G4String GetName() const { return fTETModelName; }
    G4ThreeVector GetBoundingBoxMin() const { return fBoundingBoxMin; }
    G4ThreeVector GetBoundingBoxMax() const { return fBoundingBoxMax; }
    G4ThreeVector GetBoundingBoxCen() const { return fBoundingBoxCen; }
    G4ThreeVector GetBoundingBoxSize() const { return fBoundingBoxSize; }
    G4int GetNumTets() const { return tet_Vector.size(); }
    G4double GetTotalVolume() const { return fTETModelVolume; }
    G4double GetTotalMass() const { return fTETModelMass; }

    // --- Tetrahedron information --- //
    G4Tet* GetTetrahedron(G4int tetID) const { return tet_Vector.at(tetID); }

    // --- SubModel information --- //
    G4int GetSubModelID(G4int tetID) const { return tetID_subModelID_Map.at(tetID); }
    std::set<G4int> GetSubModelIDSet() const { return subModelID_Set; }
    G4double GetSubModelVolume(G4int subModelID) const;
    G4double GetSubModelMass(G4int subModelID) const;
    G4Material* GetSubModelMaterial(G4int subModelID) const;
    G4Colour GetSubModelColour(G4int subModelID) const;

    // --- Bone Data --- //
    G4double GetSubModelMassRatio(G4int subModelID, BoneSubModel boneSubModel) const;
    G4double GetSubModelDRF(G4int subModelID, BoneSubModel boneSubModel, G4double kineticEnergy) const;

private:
    void ImportNodeData(const G4String& nodeFilePath);
    void ImportEleData(const G4String& eleFilePath);
    void ImportMaterialData(const G4String& materialFilePath);
    void ImportColourData(const G4String& colourFilePath);
    void ImportBoneMassRatioData(const G4String& boneMassRatioFilePath);
    void ImportBoneDRFData(const G4String& boneDRFFilePath);

    void CalculateModelDetails();
    void PrintTETModelInformation() const;

    G4double LogInterp_DRF(G4double energy, const std::vector<G4double>& DRFdata) const;

    // --- TETModel data --- //
    G4String fTETModelName;
    G4ThreeVector fBoundingBoxMin;
    G4ThreeVector fBoundingBoxMax;
    G4ThreeVector fBoundingBoxCen;
    G4ThreeVector fBoundingBoxSize;
    G4double fTETModelVolume;
    G4double fTETModelMass;

    // --- node data --- //
    std::vector<G4ThreeVector> node_Vector;

    // --- ele data --- //
    std::vector<G4Tet*> tet_Vector;
    std::map<G4int, G4int> tetID_subModelID_Map;

    // --- material data --- //
    std::map<G4int, G4Material*> subModelMaterial_Map;

    // --- colour data --- //
    std::map<G4int, G4Colour> subModelColour_Map;

    // --- subModel data --- //
    std::set<G4int> subModelID_Set;
    std::map<G4int, G4double> subModelVolume_Map;
    std::map<G4int, G4double> subModelMass_Map;
    std::map<G4int, G4int> subModelNumTets_Map;

    // --- bone data --- //
    std::map< std::pair<G4int, BoneSubModel> , G4double > subModelMassRatio_Map;
    std::map< std::pair<G4int, BoneSubModel>, std::vector<G4double> > subModelDRFs_Map;
};

#endif
