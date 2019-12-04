#include "TETModel.hh"
#include "TETModelStore.hh"

TETModel::TETModel(G4String name,
    const G4String& nodeFilePath, const G4String& eleFilePath,
    const G4String& colourFilePath)
: fModelName(name)
{
    G4cout << "================================================================================"<<G4endl;
    G4cout << "\t" << fModelName << " was implemented in this CODE!!   "<< G4endl;
    G4cout << "================================================================================"<<G4endl;

    ImportNodeData(nodeFilePath);
    ImportEleData(eleFilePath);
    ImportColourData(colourFilePath);

    CalculateModelDetails();

    TETModelStore::GetInstance()->Register(this);
}

// --- Get* functions --- //
G4int TETModel::GetSubModelNumTet(G4int subModelID) const
{
    if(subModelNumTets_Map.find(subModelID) == subModelNumTets_Map.end())
        return 0;
    else
        return subModelNumTets_Map.at(subModelID);
}

G4double TETModel::GetSubModelVolume(G4int subModelID) const
{
    if(subModelVolume_Map.find(subModelID) == subModelVolume_Map.end())
        return 0.;
    else
        return subModelVolume_Map.at(subModelID);
}

G4Colour TETModel::GetSubModelColour(G4int subModelID) const
{
    if(subModelColour_Map.find(subModelID) == subModelColour_Map.end())
        return G4Colour(1., .752941, .627451, .01);
    else
        return subModelColour_Map.at(subModelID);
}

void TETModel::Print() const
{
    // --- Print the overall information for each subModel --- //
    G4cout << G4endl
           << std::setw(9)  << "SubModel ID"
           << std::setw(11) << "# of Tets"
           << std::setw(11) << "vol [cm3]" << G4endl;
    G4cout << "--------------------------------------------------------------------------------" << G4endl;

    G4cout << std::setiosflags(std::ios::fixed);
    G4cout.precision(3);

    for(auto subModelID: subModelID_Set)
    {
        G4cout << std::setw(9)  << subModelID
               << std::setw(11) << subModelNumTets_Map.at(subModelID)
               << std::setw(11) << GetSubModelVolume(subModelID)/cm3 << G4endl;
    }

    // --- Print information for whole TETModel --- //
    G4cout << G4endl;
    G4cout << "   TETModel name              " << fModelName << G4endl;
    G4cout << "   TETModelBox size           "
           << fBoundingBoxSize.x()/cm << " * "
           << fBoundingBoxSize.y()/cm << " * "
           << fBoundingBoxSize.z()/cm << " cm3" << G4endl;
    G4cout << "   TETModelBox Min. position  " << fBoundingBoxMin/cm << " cm" << G4endl;
    G4cout << "   TETModelBox Max. position  " << fBoundingBoxMax/cm << " cm" << G4endl;
    G4cout << "   TETModelBox Cen. position  " << fBoundingBoxCen/cm << " cm" << G4endl;
    G4cout << "   TETModel Volume            " << fWholeVolume/cm3 << " cm3" << G4endl;
    G4cout << "   Number of tetrahedrons     " << tet_Vector.size() << G4endl << G4endl;
}

G4bool TETModel::IsInside(const G4ThreeVector pt)
{
    for(const auto& tet: tet_Vector)
        if(tet->Inside(pt - fBoundingBoxCen)==kInside) return true;
    return false;
}

// --- Private functions --- //
void TETModel::ImportNodeData(const G4String& nodeFilePath)
{
    // --- Open node file --- //
    std::ifstream ifs(nodeFilePath.c_str());
    if(!ifs.is_open())
        G4Exception("TETModel::ImportNodeData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + nodeFilePath + "'").c_str());

    G4cout << "  Opening TETGEN node (vertex points: x y z) file '"
           << nodeFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4double xMin(DBL_MAX), yMin(DBL_MAX), zMin(DBL_MAX);
    G4double xMax(-DBL_MAX), yMax(-DBL_MAX), zMax(-DBL_MAX);

    G4int nNodes;
    G4double x, y, z;
    G4int dummyValue;

    // Read header line
    ifs >> nNodes >> dummyValue >> dummyValue >> dummyValue;

    // Read data lines
    for(G4int i = 0; i < nNodes; ++i)
    {
        ifs >> dummyValue >> x >> y >> z;
        x *= cm;
        y *= cm;
        z *= cm;

        node_Vector.push_back(G4ThreeVector(x, y, z));

        // Update Min & Max
        if (x < xMin) xMin = x;
        if (x > xMax) xMax = x;
        if (y < yMin) yMin = y;
        if (y > yMax) yMax = y;
        if (z < zMin) zMin = z;
        if (z > zMax) zMax = z;
    }

    // Set Min, Max, Cen, Size
    fBoundingBoxMin = G4ThreeVector(xMin, yMin, zMin);
    fBoundingBoxMax = G4ThreeVector(xMax, yMax, zMax);
    fBoundingBoxCen = (fBoundingBoxMin + fBoundingBoxMax)/2.;
    fBoundingBoxSize = fBoundingBoxMax - fBoundingBoxMin;

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportEleData(const G4String& eleFilePath)
{
    // --- Open ele file --- //
    std::ifstream ifs(eleFilePath.c_str());
    if(!ifs.is_open())
        G4Exception("TETModel::ImportEleData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + eleFilePath + "'").c_str());

    G4cout << "  Opening TETGEN elements (tetrahedron with node No.) file '"
           << eleFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int nTets;
    size_t nodeID0, nodeID1, nodeID2, nodeID3;
    G4int subModelID;
    G4int dummyValue;

    // Read header line
    ifs >> nTets >> dummyValue >> dummyValue;

    // Read data lines
    for(G4int i = 0; i < nTets; ++i)
    {
        ifs >> dummyValue >> nodeID0 >> nodeID1 >> nodeID2 >> nodeID3;

        if(ifs.get() == '\n') subModelID = -1; // NULL ID. when there is no subModelID in ele file.
        else ifs >> subModelID;

        tet_Vector.push_back(
            new G4Tet("Tet_Solid",
            node_Vector[nodeID0]-fBoundingBoxCen,
            node_Vector[nodeID1]-fBoundingBoxCen,
            node_Vector[nodeID2]-fBoundingBoxCen,
            node_Vector[nodeID3]-fBoundingBoxCen)
            );

        subModelID_Set.insert(subModelID);
        tetID_subModelID_Map[i] = subModelID;
    }

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportColourData(const G4String& colourFilePath)
{
    // --- Open colour file --- //
    std::ifstream ifs(colourFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportColourData()", "", JustWarning,
            G4String("      There is no file '" + colourFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening colour data file '" << colourFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;
    G4double rValue, gValue, bValue, aValue;

    // Read data lines
    while(ifs >> subModelID >> rValue >> gValue >> bValue >> aValue)
        subModelColour_Map[subModelID] = G4Colour(rValue, gValue, bValue, aValue);

    // --- Close the file --- //
    ifs.close();
}

void TETModel::CalculateModelDetails()
{
    G4cout << "  Calculating model details... " <<G4endl;

    // --- Initialize --- //
    fWholeVolume = 0.;
    for(auto subModelID: subModelID_Set)
    {
        subModelVolume_Map[subModelID] = 0.;
        subModelNumTets_Map[subModelID] = 0;
    }

    // --- Calculate submodel volume, mass, nTets --- //
    for(size_t i = 0; i < tet_Vector.size(); ++i)
    {
        G4double tetVolume = tet_Vector[i]->GetCubicVolume();
        G4int subModelID = tetID_subModelID_Map[static_cast<G4int>(i)];

        subModelVolume_Map[subModelID] += tetVolume;
        ++subModelNumTets_Map[subModelID];

        fWholeVolume += tetVolume;
    }
}

// --- Additional Functions --- //
G4ThreeVector SampleRndPointInTet(const G4Tet* tet)
{
    // sample random point in a tetrahedron (ref. http://vcg.isti.cnr.it/jgt/tetra.htm)
    G4double c1 = G4UniformRand();
    G4double c2 = G4UniformRand();
    G4double c3 = G4UniformRand();

    if((c1 + c2)>1.)
    {
        c1 = 1. - c1;
        c2 = 1. - c2;
    }

    if((c2 + c3)>1.)
    {
        G4double c3_prv = c3;
        c3 = 1. - c1 - c2;
        c2 = 1. - c3_prv;
    }
    else if((c1 + c2 + c3)>1.)
    {
        G4double c3_prv = c3;
        c3 = c1 + c2 + c3 - 1.;
        c1 = 1. - c2 - c3_prv;
    }

    G4double c0 = 1. - c1 - c2 - c3;
    G4ThreeVector rndPoint =
            tet->GetVertices().at(0) * c0 +
            tet->GetVertices().at(1) * c1 +
            tet->GetVertices().at(2) * c2 +
            tet->GetVertices().at(3) * c3;

    return rndPoint;
}
