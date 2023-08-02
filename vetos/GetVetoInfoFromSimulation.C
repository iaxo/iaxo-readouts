//
// Created by lobis on 6/13/2023.
//

#include <TEveGeoNode.h>
#include <TEveGeoShape.h>
#include <TEveManager.h>
#include <TEvePointSet.h>
#include <TGLViewer.h>
#include <TGeoManager.h>
#include <TRestDetectorReadout.h>
#include <TRestDetectorReadoutChannel.h>
#include <TRestDetectorReadoutModule.h>
#include <TRestDetectorReadoutPixel.h>
#include <TRestDetectorReadoutPlane.h>
#include <TRestGeant4GeometryInfo.h>
#include <TRestGeant4Metadata.h>
#include <TRestRun.h>

#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct VetoInfo {
    string volume;             // physical volume name
    string lightGuide;         // physical lightGuide volume name
    TVector3 readoutPosition;  // position of the readout in the world frame. Not the position of the readout
                               // volume. This should be the center of one of the sides of the readout volume.
    TVector3 normal;           // normal to the readout surface.
    double height;  // height of the readout plane: distance between readout and the limit of the veto
};

std::map<string, int> referenceVetoNameToDaqId;

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
    oss << "VetoInfo{volume=" << vetoInfo.volume << ", lightGuide=" << vetoInfo.lightGuide
        << ", readoutPosition=(" << vetoInfo.readoutPosition.X() << ", " << vetoInfo.readoutPosition.Y()
        << ", " << vetoInfo.readoutPosition.Z() << ")"
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

void Draw(const vector<VetoInfo>& vetoInfo, TRestDetectorReadout* readout) {
    cout << "Drawing " << vetoInfo.size() << " veto readouts" << endl;

    TEveManager::Create();

    // Load the TGeoManager from a file or create it programmatically
    TGeoManager* geoManager = gGeoManager;
    if (!geoManager) {
        cerr << "No TGeoManager found" << endl;
    }

    TEveGeoTopNode* volume = new TEveGeoTopNode(geoManager, geoManager->GetTopNode());
    volume->SetVisLevel(5);

    constexpr double transparency = 10.0;
    gEve->AddGlobalElement(volume);
    for (int i = 0; i < geoManager->GetListOfVolumes()->GetEntries(); i++) {
        geoManager->GetVolume(i)->SetTransparency(transparency);
    }

    // set transparency for all elements in the geometry

    // set background color to white
    gEve->GetDefaultGLViewer()->SetClearColor(kWhite);

    TEvePointSet* readoutPositions = new TEvePointSet("VetoInfo");

    for (const auto& veto : vetoInfo) {
        readoutPositions->SetNextPoint(veto.readoutPosition.X() / 10.0, veto.readoutPosition.Y() / 10.0,
                                       veto.readoutPosition.Z() / 10.0);
    }

    readoutPositions->SetMarkerColor(kRed);
    readoutPositions->SetMarkerSize(1);
    readoutPositions->SetMarkerStyle(23);

    gEve->AddElement(readoutPositions);

    // limit of veto

    TEvePointSet* vetoLimits = new TEvePointSet("VetoLimits");

    for (const auto& veto : vetoInfo) {
        TVector3 limit = veto.readoutPosition + veto.normal * veto.height;
        vetoLimits->SetNextPoint(limit.X() / 10.0, limit.Y() / 10.0, limit.Z() / 10.0);
    }

    vetoLimits->SetMarkerColor(kBlue);
    vetoLimits->SetMarkerSize(1);
    vetoLimits->SetMarkerStyle(23);

    gEve->AddElement(vetoLimits);

    // from readout

    TEvePointSet* readoutPlanePositions = new TEvePointSet("ReadoutPlanePositions");
    TEvePointSet* readoutPlaneEnd = new TEvePointSet("ReadoutPlaneEnd");

    for (const auto& veto : vetoInfo) {
        TVector3 limit = veto.readoutPosition + veto.normal * veto.height;
        vetoLimits->SetNextPoint(limit.X() / 10.0, limit.Y() / 10.0, limit.Z() / 10.0);
    }

    for (int p = 0; p < readout->GetNumberOfReadoutPlanes(); p++) {
        auto plane = readout->GetReadoutPlane(p);
        auto position = plane->GetPosition() + plane->GetAxisX() * 50;
        auto normal = plane->GetNormal();
        auto height = plane->GetHeight();

        readoutPlanePositions->SetNextPoint(position.X() / 10.0, position.Y() / 10.0, position.Z() / 10.0);

        TVector3 limit = position + normal * height;
        readoutPlaneEnd->SetNextPoint(limit.X() / 10.0, limit.Y() / 10.0, limit.Z() / 10.0);

        auto module = plane->GetModule(0);
        module->GetOrigin();
    }

    readoutPlanePositions->SetMarkerColor(kGreen);
    readoutPlanePositions->SetMarkerSize(3);
    readoutPlanePositions->SetMarkerStyle(23);

    readoutPlaneEnd->SetMarkerColor(kYellow);
    readoutPlaneEnd->SetMarkerSize(3);
    readoutPlaneEnd->SetMarkerStyle(23);

    gEve->AddElement(readoutPlanePositions);
    gEve->AddElement(readoutPlaneEnd);

    gEve->Redraw3D();
}

bool IsTopOrBottom(const string& name) {
    // if name contains "Top" or "Bottom" return true
    return name.find("vetoSystemTop") != string::npos || name.find("vetoSystemBottom") != string::npos;
}

TRestDetectorReadout* GenerateReadout(const vector<VetoInfo>& vetoInfo) {
    TRestDetectorReadout readout;

    int i = 0;
    for (const auto& veto : vetoInfo) {
        TRestDetectorReadoutPlane plane;
        plane.SetPosition(veto.readoutPosition);
        plane.SetNormal(veto.normal);
        plane.SetHeight(veto.height);
        plane.SetID(i++);
        plane.SetAxisX(IsTopOrBottom(veto.volume) ? TVector3(1, 0, 0) : TVector3(0, 1, 0));

        TRestDetectorReadoutModule module;
        module.SetName(veto.volume);
        module.SetModuleID(0);

        TVector2 size = TVector2(200, 50);
        module.SetSize(size);
        module.SetOrigin(-1.0 * size / 2.0);

        TRestDetectorReadoutChannel channel;
        const int channelId = i + 1000;  // TODO: set correct ID
        channel.SetChannelID(channelId);
        channel.SetDaqID(channelId);
        referenceVetoNameToDaqId[veto.volume] = channelId;

        TRestDetectorReadoutPixel pixel;
        pixel.SetSize(size);
        channel.AddPixel(pixel);

        module.AddChannel(channel);
        plane.AddModule(module);
        readout.AddReadoutPlane(plane);
    }

    const string readoutFilename = "/tmp/vetoReadout.root";
    auto file = TFile::Open(readoutFilename.c_str(), "RECREATE");
    readout.Write("vetoReadout");
    file->Close();

    file = TFile::Open(readoutFilename.c_str());
    TRestDetectorReadout* readoutFromFile = dynamic_cast<TRestDetectorReadout*>(file->Get("vetoReadout"));

    return readoutFromFile;
}

void TestReadout(TRestDetectorReadout* readout, const vector<VetoInfo>& vetoInfo) {
    std::map<string, int> volumeToChannelId;
    for (const auto veto : vetoInfo) {
        Int_t moduleId = -1;
        Int_t channelId = -1;
        Int_t daqId = -1;

        TVector3 position = veto.readoutPosition + veto.normal * (veto.height / 2.0);  // center of veto

        for (int p = 0; p < readout->GetNumberOfReadoutPlanes(); p++) {
            daqId = readout->GetHitsDaqChannelAtReadoutPlane(position, moduleId, channelId, p);
            if (daqId != -1) {
                break;
            }
        }

        volumeToChannelId[veto.volume] = daqId;

        cout << "Name: " << veto.volume << "Position: " << position.X() << ", " << position.Y() << ", "
             << position.Z() << " Module ID: " << moduleId << " Channel ID: " << channelId
             << " DAQ ID: " << daqId << " Ref DAQ ID: " << referenceVetoNameToDaqId[veto.volume] << endl;
    }

    // print mismatching channels
    for (const auto& [volume, daqId] : volumeToChannelId) {
        if (daqId != referenceVetoNameToDaqId[volume]) {
            cout << "Mismatching channel for volume " << volume << " DAQ ID: " << daqId
                 << " Ref DAQ ID: " << referenceVetoNameToDaqId[volume] << endl;
        }
    }
}

void GetVetoInfoFromSimulation(const char* simulationFilename = "simulation.root") {
    TRestRun run(simulationFilename);
    const auto metadata = (TRestGeant4Metadata*)run.GetMetadataClass("TRestGeant4Metadata");
    const auto& geometryInfo = metadata->GetGeant4GeometryInfo();

    vector<VetoInfo> vetoInfo;

    const string vetoVolumeExpression = "^scintillatorVolume";
    const string vetoLightGuideExpression = "^scintillatorLightGuideVolume";
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

    for (size_t i = 0; i < vetoVolumes.size(); ++i) {
        const auto& volume = vetoVolumes[i];
        const auto& lightGuide = vetoLightGuides[i];
        const auto& vetoPosition = geometryInfo.GetPosition(volume);
        const TVector3 normal = (vetoPosition - geometryInfo.GetPosition(lightGuide)).Unit();
        auto h = extractLength(volume.Data());
        const double height = h ? *h : 0;
        const auto readoutPosition = geometryInfo.GetPosition(volume) - normal * (height / 2.0);

        vetoInfo.push_back(VetoInfo{volume.Data(), lightGuide.Data(), readoutPosition, normal, height});
    }

    for (const auto& info : vetoInfo) {
        cout << VetoInfoToString(info) << endl;
    }

    const auto readout = GenerateReadout(vetoInfo);

    TestReadout(readout, vetoInfo);
    cout << "Done testing readout" << endl;

    Draw(vetoInfo, readout);

    cout << "Finished" << endl;
}
