//
// Created by lobis on 6/13/2023.
//

#include <TRestGeant4GeometryInfo.h>
#include <TRestGeant4Metadata.h>
#include <TRestRun.h>

#include <optional>
#include <regex>
#include <string>

using namespace std;

struct VetoInfo {
    string volume;      // physical volume name
    string lightGuide;  // physical lightGuide volume name
    TVector3 position;  // position of the readout in the world frame. Not the position of the readout volume.
                        // This should be the center of one of the sides of the readout volume.
    TVector3 normal;    // normal to the readout surface.
    double height;      // height of the readout plane: distance between readout and the limit of the veto
};

std::optional<double> extractLength(const std::string& input) {
    // extracts length from veto name such as
    // "VetoSystem_vetoSystemFront_vetoLayerBack3_assembly-22.veto3_scintillatorLightGuideVolume-800.0mm-f1a5df8a"
    // -> 800mm
    std::regex lengthRegex(R"(\d+\.\d+mm)");

    std::smatch matches;
    if (std::regex_search(input, matches, lengthRegex)) {
        std::string lengthString = matches[0];
        try {
            double length = std::stod(lengthString);
            return length;
        } catch (const std::exception& e) {
            // Failed to convert string to double
            return std::nullopt;
        }
    } else {
        // No length found
        return std::nullopt;
    }
}

string VetoInfoToString(const VetoInfo& vetoInfo) {
    ostringstream oss;
    oss << "VetoInfo{volume=" << vetoInfo.volume << ", lightGuide=" << vetoInfo.lightGuide << ", position=("
        << vetoInfo.position.X() << ", " << vetoInfo.position.Y() << ", " << vetoInfo.position.Z() << ")"
        << ", normal=(" << vetoInfo.normal.X() << ", " << vetoInfo.normal.Y() << ", " << vetoInfo.normal.Z()
        << ")"
        << ", height=" << vetoInfo.height << "}";
    return oss.str();
}

vector<TString> GetVolumesFromExpression(const TRestGeant4GeometryInfo& geometryInfo,
                                         const string& expression) {
    // Get all physical volumes matching the expression. The expression can match either the physical volume
    // name or the logical volume name.
    auto volumes = geometryInfo.GetAllPhysicalVolumesMatchingExpression(expression);
    if (volumes.empty()) {
        const auto logicalVolumes = geometryInfo.GetAllLogicalVolumesMatchingExpression(expression);
        for (const auto& logicalVolume : logicalVolumes) {
            for (const auto& physicalVolume : geometryInfo.GetAllPhysicalVolumesFromLogical(logicalVolume)) {
                volumes.push_back(geometryInfo.GetAlternativeNameFromGeant4PhysicalName(physicalVolume));
            }
        }
    }
    return volumes;
}

void GetVetoInfoFromSimulation(const char* simulationFilename1 = "simulation.root") {
    const char* simulationFilename = "simulation.root";
    TRestRun run(simulationFilename);
    const auto metadata = (TRestGeant4Metadata*)run.GetMetadataClass("TRestGeant4Metadata");
    const auto& geometryInfo = metadata->GetGeant4GeometryInfo();

    const string vetoVolumeExpression = "^scintillatorVolume";
    const string vetoLightGuideExpression = "^scintillatorLightGuideVolume";
    const double vetoLightGuideOffset =
        65.0;  // mm. This is the center of the light guide to the center of one of the scintillator sides.

    auto vetoVolumes = GetVolumesFromExpression(geometryInfo, vetoVolumeExpression);
    if (vetoVolumes.empty()) {
        cerr << "No veto volumes found" << endl;
        exit(1);
    }

    auto vetoLightGuides = GetVolumesFromExpression(geometryInfo, vetoLightGuideExpression);
    if (vetoLightGuides.empty()) {
        cerr << "No veto light guides found" << endl;
        exit(1);
    }

    if (vetoVolumes.size() != vetoLightGuides.size()) {
        cerr << "Number of veto volumes and veto light guides do not match" << endl;
        exit(1);
    }

    cout << "Found " << vetoVolumes.size() << " veto volumes" << endl;

    vector<VetoInfo> vetoInfo;
    for (size_t i = 0; i < vetoVolumes.size(); ++i) {
        const auto& volume = vetoVolumes[i];
        const auto& lightGuide = vetoLightGuides[i];
        const auto& vetoPosition = geometryInfo.GetPosition(volume);
        const TVector3 normal = (geometryInfo.GetPosition(lightGuide) - vetoPosition).Unit();
        const auto readoutPosition = geometryInfo.GetPosition(volume) - normal * vetoLightGuideOffset;
        auto h = extractLength(volume.Data());
        const double height = h ? *h : 0;

        vetoInfo.push_back(VetoInfo{volume.Data(), lightGuide.Data(), readoutPosition, normal, height});
    }

    for (const auto& info : vetoInfo) {
        cout << VetoInfoToString(info) << endl;
    }

    cout << "Finished" << endl;
}
