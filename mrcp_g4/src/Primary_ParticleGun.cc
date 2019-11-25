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
    fWeight = 1.;

    // Check if the source is defined as a nuclide
    if(fNuclideSource)
    {
        auto decayProduct = fNuclideSource->SampleDecayProduct();
        fPrimary->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(std::get<0>(decayProduct)));
        fPrimary->SetParticleEnergy(std::get<1>(decayProduct));
        fWeight *= std::get<2>(decayProduct);
    }

    fPrimary->SetParticleMomentumDirection(SampleISODirection());
    fPrimary->GeneratePrimaryVertex(anEvent);
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

    fMessenger->DeclareMethod("posFromFloor", &Primary_ParticleGun::SetPositionFromPhantomFloor,
                              "Calculate source position from center of the phantom floor.");
}

void Primary_ParticleGun::SetNuclide(const G4String& nuclideName)
{
    if(fNuclideSource) delete fNuclideSource;
    fNuclideSource = new NuclideSource(fNuclideName = nuclideName);
}

void Primary_ParticleGun::SetPositionFromPhantomFloor(G4ThreeVector position, G4String unitString)
{
    G4double unitDouble = G4UIcommand::ConvertToDimensionedDouble(("1 " + unitString).c_str());
    G4ThreeVector dimensionedPosition = position * unitDouble;

    auto theTETModel = TETModelStore::GetInstance()->GetTETModel("MainPhantom");
    if(!theTETModel)
    {
        G4Exception("Primary_ParticleGun::SetPositionFromPhantomFloor()", "", JustWarning,
            G4String("      no MainPhantom. " ).c_str());
        fPrimary->SetParticlePosition(dimensionedPosition);
        return;
    }

    G4ThreeVector tetModelCenter = theTETModel->GetBoundingBoxCen();
    G4double tetModelHeight = theTETModel->GetBoundingBoxSize().z();
    G4ThreeVector tetModelFloor = tetModelCenter - G4ThreeVector(0., 0., tetModelHeight/2.);

    fPrimary->SetParticlePosition(tetModelFloor + dimensionedPosition);
}

G4ThreeVector Primary_ParticleGun::SampleISODirection()
{
    if(!fAngleBiasingPVName.size()) return G4RandomDirection();

    auto thePhysicalVolume = G4PhysicalVolumeStore::GetInstance()->GetVolume(fAngleBiasingPVName);
    if(!thePhysicalVolume)
    {
        G4Exception("Primary_ParticleGun::SampleISODirection()", "", JustWarning,
                    G4String("      invalid physical volume '" + fAngleBiasingPVName + "'" ).c_str());
        return G4RandomDirection();
    }

    fAngleBiasingPVPosition = thePhysicalVolume->GetObjectTranslation();

    auto theBox = static_cast<const G4Box*>(thePhysicalVolume->GetLogicalVolume()->GetSolid());
    fAngleBiasingPVBoxVerticies =
    {
        G4ThreeVector(-theBox->GetXHalfLength(), -theBox->GetYHalfLength(), -theBox->GetZHalfLength()),
        G4ThreeVector(-theBox->GetXHalfLength(), -theBox->GetYHalfLength(),  theBox->GetZHalfLength()),
        G4ThreeVector(-theBox->GetXHalfLength(),  theBox->GetYHalfLength(), -theBox->GetZHalfLength()),
        G4ThreeVector(-theBox->GetXHalfLength(),  theBox->GetYHalfLength(),  theBox->GetZHalfLength()),
        G4ThreeVector( theBox->GetXHalfLength(), -theBox->GetYHalfLength(), -theBox->GetZHalfLength()),
        G4ThreeVector( theBox->GetXHalfLength(), -theBox->GetYHalfLength(),  theBox->GetZHalfLength()),
        G4ThreeVector( theBox->GetXHalfLength(),  theBox->GetYHalfLength(), -theBox->GetZHalfLength()),
        G4ThreeVector( theBox->GetXHalfLength(),  theBox->GetYHalfLength(),  theBox->GetZHalfLength())
    };
    fAngleBiasingPVBoxMag2 = fAngleBiasingPVBoxVerticies[0].mag2();

    G4ThreeVector targetVector = fAngleBiasingPVPosition - fPrimary->GetParticlePosition();
    // Source position is closer than the size of the box.
    if(targetVector.mag2() <= fAngleBiasingPVBoxMag2) return G4RandomDirection();

    // Calculate cone apex half-angle
    G4double maxTheta = 0.;
    for(const auto& vertex: fAngleBiasingPVBoxVerticies)
    {
        G4double theAngle = targetVector.angle(vertex + targetVector);
        maxTheta = (theAngle>maxTheta) ? theAngle : maxTheta;
    }

    // Sample direction
    G4double solidAngle = CLHEP::twopi * (1 - std::cos(maxTheta));
    fWeight *= solidAngle / (4 * CLHEP::pi);
    G4double rndPhi = G4UniformRand() * CLHEP::twopi;
    G4double rndTheta = std::acos( G4UniformRand()*(1 - std::cos(maxTheta)) + std::cos(maxTheta) );
    G4ThreeVector srcDir;
    srcDir.setRThetaPhi(1., rndTheta, rndPhi);
    if(!(targetVector.isParallel(G4ThreeVector(0., 0., 1.))))
        srcDir.rotate(-targetVector.cross(G4ThreeVector(0., 0., 1.)), targetVector.angle(G4ThreeVector(0., 0., 1.)));
    else
        srcDir *= targetVector.unit().dot(G4ThreeVector(0., 0., 1));

    return srcDir;
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
