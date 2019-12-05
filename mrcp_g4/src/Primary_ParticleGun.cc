#include "Primary_ParticleGun.hh"
#include "TETModelStore.hh"

Primary_ParticleGun::Primary_ParticleGun()
: G4VUserPrimaryGeneratorAction(), fWeight(1.), fNuclideSource(nullptr)
{
    fPrimary = new G4ParticleGun;

    DefineCommands();
}

Primary_ParticleGun::~Primary_ParticleGun()
{
    delete fPrimary;
    delete fMessenger;
    if(fNuclideSource) delete fNuclideSource;
}

void Primary_ParticleGun::GeneratePrimaries(G4Event* anEvent)
{
    // Weight initialization
    G4double particleWeight{1.};

    // Check if the source is defined as a nuclide
    if(fNuclideSource)
    {
        auto decayProduct = fNuclideSource->SampleDecayProduct();
        fPrimary->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(std::get<0>(decayProduct)));
        fPrimary->SetParticleEnergy(std::get<1>(decayProduct));
        particleWeight *= std::get<2>(decayProduct);
    }

    auto dirVec = RandomDirectionFromTo(
                fPrimary->GetParticlePosition(),
                fAngleBiasingPVName,
                particleWeight
                );
    fPrimary->SetParticleMomentumDirection(dirVec);
    fPrimary->GeneratePrimaryVertex(anEvent);

    // Weight setting
    anEvent->GetPrimaryVertex()->SetWeight(particleWeight);
}

void Primary_ParticleGun::DefineCommands()
{
    fMessenger = new G4GenericMessenger(this, "/gun/");

    auto& nuclideCmd = fMessenger->DeclareMethod("nuclide", &Primary_ParticleGun::SetNuclide, "Primary source nuclide.");
    nuclideCmd.SetParameterName("nucl", true);
    nuclideCmd.SetDefaultValue("");

    auto& angleBiasingCmd =
        fMessenger->DeclareProperty("angleBiasing", fAngleBiasingPVName, "Bias source direction to the G4Box shape physical volume.");
    angleBiasingCmd.SetParameterName("physicalVolume", true);
    angleBiasingCmd.SetDefaultValue("");
}

void Primary_ParticleGun::SetNuclide(const G4String& nuclideName)
{
    if(fNuclideSource) delete fNuclideSource;
    fNuclideSource = new NuclideSource(fNuclideName = nuclideName);
}

NuclideSource::NuclideSource(G4String nuclideName)
: fNormalized(false), fTotalYield(0.)
{
    // --- Open source file --- //
    std::ifstream ifs( ("./nuclides/" + nuclideName).c_str() );
    if(!ifs.is_open())
        G4Exception("NuclideSource::NuclideSource()", "", FatalErrorInArgument,
            G4String("      No nuclide source file '" + nuclideName + "' in './nuclides/.'" ).c_str());

    G4cout << "  Opening NuclideSource file '"
           << nuclideName << "'" <<G4endl;

    // --- Get data --- //
    while(!ifs.eof())
    {
        // Get each line
        G4String thisLine;
        std::getline(ifs, thisLine);

        // Ignore the comment lines which starts with '#'
        if(thisLine.c_str()[0] == '#') continue;

        // Store data as a tuple
        G4String particleName;
        G4double kineticEnergy;
        G4double yield;
        std::stringstream ss(thisLine);
        ss >> particleName >> kineticEnergy >> yield;
        fYield_Vector.push_back(std::make_tuple(particleName, kineticEnergy, yield));
    }

    // --- Close the file --- //
    ifs.close();
}

std::tuple<G4String, G4double, G4double> NuclideSource::SampleDecayProduct() // ParticleName, Energy, Weight
{
    if(!fNormalized) Normalize();

    G4double rnd = G4UniformRand();
    size_t i = 0;
    while(rnd > fCumulativeProbability_Vector.at(i)) ++i;
    return std::make_tuple(std::get<0>(fYield_Vector.at(i)),
                           std::get<1>(fYield_Vector.at(i)),
                           fTotalYield);
}

void NuclideSource::Normalize()
{
    // Calculate total yield
    for(const auto& yieldData: fYield_Vector)
        fTotalYield += std::get<2>(yieldData);

    // Normalize probabilities and calculate weights
    G4double currentProbability = 0.;
    fCumulativeProbability_Vector.clear();
    for(const auto& yieldData: fYield_Vector)
        fCumulativeProbability_Vector.push_back(currentProbability += std::get<2>(yieldData) / fTotalYield);

    fNormalized = true;
}

G4ThreeVector RandomDirectionFromTo(const G4ThreeVector& referencePoint,
                                    const G4String& physicalVolumeName,
                                    G4double& particleWeight,
                                    G4double margin)
{
    auto physicalVolume = G4PhysicalVolumeStore::GetInstance()->GetVolume(physicalVolumeName);
    if(!physicalVolume)
    {
        G4Exception("Primary_ParticleGun::SampleISODirection()", "", JustWarning,
                    G4String("      invalid physical volume '" + physicalVolumeName + "'" ).c_str());
        return G4RandomDirection();
    }

    G4ThreeVector pvPosition, pvMin, pvMax;
    pvPosition = physicalVolume->GetObjectTranslation();
    physicalVolume->GetLogicalVolume()->GetSolid()->BoundingLimits(pvMin, pvMax);
    if(margin>0.)
    {
        pvMin = pvMin - G4ThreeVector(margin, margin, margin);
        pvMax = pvMax + G4ThreeVector(margin, margin, margin);
    }

    G4ThreeVector targetVector = pvPosition - referencePoint;
    G4ThreeVectorList pvBoxVerticies =
    {
        G4ThreeVector(pvMin.x(), pvMin.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMin.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMax.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMax.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMin.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMin.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMax.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMax.y(), pvMax.z()) + targetVector
    };

    G4double maxTheta = 0.;
    for(const auto& vertex: pvBoxVerticies)
    {
        G4double target2VertexAngle = targetVector.angle(vertex);
        maxTheta = (target2VertexAngle>maxTheta) ? target2VertexAngle : maxTheta;
    }
    // If the ref point is in the bounding box of the physical volume, return 4pi dir.
    G4double cosTheta = std::cos(maxTheta);
    if(cosTheta<=0.) return G4RandomDirection();

    // Multiply particle weight as its solid angle reduction
    G4double solidAngle = CLHEP::twopi * (1 - cosTheta);
    particleWeight *= solidAngle / (4 * CLHEP::pi);

    // Sample direction and rotate to targetVector
    G4ThreeVector dirVec = G4RandomDirection(cosTheta);
    G4ThreeVector zUnit = G4ThreeVector(0., 0., 1);
    if(!(targetVector.isParallel(zUnit)))
        dirVec.rotate(zUnit.cross(targetVector), zUnit.angle(targetVector));
    else
        dirVec *= targetVector.unit().dot(zUnit);

    return dirVec;
}
