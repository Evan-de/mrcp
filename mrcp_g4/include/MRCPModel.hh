#ifndef MRCPMODEL_HH
#define MRCPMODEL_HH

#include "TETModel.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"

#include <vector>

class MRCPModel: public TETModel
{
public:
    MRCPModel(G4String name,
        const G4String& nodeFilePath, const G4String& eleFilePath,
        const G4String& materialFilePath, const G4String& RBMnBSFilePath,
        const G4String& colourFilePath = "");
    virtual ~MRCPModel() override;

    G4double GetTotalMass() const { return fWholeMass; }

    G4double GetSubModelMass(G4int subModelID) const;
    G4Material* GetSubModelMaterial(G4int subModelID) const;
    G4double GetSubModelRBMMassRatio(G4int subModelID) const;
    G4double GetSubModelBSMassRatio(G4int subModelID) const;

    virtual void Print() const override;

private:
    void ImportMaterialData(const G4String& materialFilePath);
    void ImportRBMnBSMassRatioData(const G4String& RBMnBSFilePath);

    // --- MRCPModel data --- //
    G4double fWholeMass;

    // --- subModel data --- //
    std::map<G4int, G4double> subModelMass_Map;

    // --- material data --- //
    std::map<G4int, G4Material*> subModelMaterial_Map;

    // --- RBM & BS mass ratio data --- //
    std::map<G4int, G4double> subModelRBMMassRatio_Map;
    std::map<G4int, G4double> subModelBSMassRatio_Map;
};

#endif
