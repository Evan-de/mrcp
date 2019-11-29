#ifndef MRCPPSDOSEDEPOSIT_HH
#define MRCPPSDOSEDEPOSIT_HH

#include "G4VPrimitiveScorer.hh"
#include "G4THitsMap.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

class MRCPModel;

class MRCPPSDoseDeposit: public G4VPrimitiveScorer
{
public:
    MRCPPSDoseDeposit(G4String name, G4String phantomName);
    virtual ~MRCPPSDoseDeposit() override {}

    void ImportBoneDRFData(const G4String& boneDRFFilePath);

protected:
    virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
    virtual G4int GetIndex(G4Step*) override;

public:
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override {}
    virtual void clear() override { fEvtMap->clear(); }

private:
    G4int fHCID;
    G4THitsMap<G4double>* fEvtMap;

    MRCPModel* fMRCPModel;

    std::map< G4int, std::vector<G4double> > subModelRBMDRF_Map;
    std::map< G4int, std::vector<G4double> > subModelBSDRF_Map;

    const std::vector<G4double> energyBin_DRF =
    {
        0.010*MeV, 0.015*MeV, 0.020*MeV, 0.030*MeV, 0.040*MeV,
        0.050*MeV, 0.060*MeV, 0.080*MeV, 0.10 *MeV, 0.15 *MeV,
        0.20 *MeV, 0.30 *MeV, 0.40 *MeV, 0.50 *MeV, 0.60 *MeV,
        0.80 *MeV, 1.0  *MeV, 1.5  *MeV, 2.0  *MeV, 3.0  *MeV,
        4.0  *MeV, 5.0  *MeV, 6.0  *MeV, 8.0  *MeV, 10.0 *MeV
    };
    G4bool fDRFFlag; // Has DRF file been imported
};

inline G4double LogInterp(G4double xq, const std::vector<G4double>& sorted_x, const std::vector<G4double>& sorted_y)
{
    if(xq<sorted_x.front()) return sorted_y.front();
    if(xq>sorted_x.back()) return sorted_y.back();

    size_t i1 = 0, i2 = 1;
    for(; i2 < (sorted_x.size() - 1); ++i1, ++i2)
        if(xq <= sorted_x.at(i2)) break;

    G4double log10_yq = // log(y1) + (log(x)-log(x1)) * (log(y2)-log(y1))/(log(x2)-log(x1))
        std::log10(sorted_y.at(i1)) +
        std::log10(xq/sorted_x.at(i1)) *
        std::log10(sorted_y.at(i2)/sorted_y.at(i1)) / std::log10(sorted_x.at(i2)/sorted_x.at(i1));

    return std::pow(10., log10_yq);
}


#endif
