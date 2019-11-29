#ifndef TETModel_hh_
#define TETModel_hh_

#include "G4ThreeVector.hh"
#include "G4Tet.hh"
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

class TETModel
{
public:
    TETModel(G4String name,
        const G4String& nodeFilePath, const G4String& eleFilePath,
        const G4String& colourFilePath = "");
    virtual ~TETModel(){}

    // --- TETModel information --- //
    G4String GetName() const { return fModelName; }
    G4ThreeVector GetBoundingBoxMin() const { return fBoundingBoxMin; }
    G4ThreeVector GetBoundingBoxMax() const { return fBoundingBoxMax; }
    G4ThreeVector GetBoundingBoxCen() const { return fBoundingBoxCen; }
    G4ThreeVector GetBoundingBoxSize() const { return fBoundingBoxSize; }
    size_t GetNumTets() const { return tet_Vector.size(); }
    G4double GetTotalVolume() const { return fWholeVolume; }

    // --- Tetrahedron information --- //
    G4Tet* GetTetrahedron(G4int tetID) const { return tet_Vector.at(static_cast<size_t>(tetID)); }

    // --- SubModel information --- //
    G4int GetSubModelID(G4int tetID) const { return tetID_subModelID_Map.at(tetID); }
    std::set<G4int> GetSubModelIDSet() const { return subModelID_Set; }
    G4int GetSubModelNumTet(G4int subModelID) const;
    G4double GetSubModelVolume(G4int subModelID) const;
    G4Colour GetSubModelColour(G4int subModelID) const;

    virtual void Print() const;

private:
    void ImportNodeData(const G4String& nodeFilePath);
    void ImportEleData(const G4String& eleFilePath);
    void ImportColourData(const G4String& colourFilePath);

    void CalculateModelDetails();

    // --- TETModel data --- //
    G4String fModelName;
    G4ThreeVector fBoundingBoxMin;
    G4ThreeVector fBoundingBoxMax;
    G4ThreeVector fBoundingBoxCen;
    G4ThreeVector fBoundingBoxSize;
    G4double fWholeVolume;

    // --- node data --- //
    std::vector<G4ThreeVector> node_Vector;

    // --- ele data --- //
    std::vector<G4Tet*> tet_Vector;
    std::map<G4int, G4int> tetID_subModelID_Map;

    // --- colour data --- //
    std::map<G4int, G4Colour> subModelColour_Map;

    // --- subModel data --- //
    std::set<G4int> subModelID_Set;
    std::map<G4int, G4double> subModelVolume_Map;
    std::map<G4int, G4int> subModelNumTets_Map;
};

#endif
