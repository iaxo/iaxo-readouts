//
// Created by lobis on 6/13/2023.
//

#include <TEveGeoNode.h>
#include <TEveManager.h>
#include <TEvePointSet.h>
#include <TGLViewer.h>
#include <TGeoManager.h>
#include <TRandom.h>
#include <TRestDetectorReadout.h>
#include <TRestDetectorReadoutChannel.h>
#include <TRestDetectorReadoutModule.h>
#include <TRestDetectorReadoutPixel.h>
#include <TRestDetectorReadoutPlane.h>
#include <TRestGeant4GeometryInfo.h>
#include <TRestGeant4Metadata.h>
#include <TRestGeant4VetoAnalysisProcess.h>  // Method to extract veto info from string
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

const vector<string> readoutNames = {"iaxoD0Readout", "iaxoD1Readout"};
const string fullReadoutFile = "../../readouts/readoutComplete.root";
const string micromegasReadoutFile = "../../readouts/readoutMicromegas.root";
const string vetoSystemReadoutFile = "../../readouts/readoutVetoSystem.root";

std::map<string, int> referenceVetoNameToDaqId;

std::map<string, int> aliasToSignalId = {
    // Top - L1
    {"Top_L1_N4", 4700},
    {"Top_L1_N3", 4701},
    {"Top_L1_N2", 4702},
    {"Top_L1_N1", 4703},

    // Top - L2
    {"Top_L2_N1", 4704},
    {"Top_L2_N2", 4705},
    {"Top_L2_N3", 4706},
    {"Top_L2_N4", 4707},

    // Top - L3
    {"Top_L3_N4", 4708},
    {"Top_L3_N3", 4709},
    {"Top_L3_N2", 4710},
    {"Top_L3_N1", 4711},

    // Bottom - L1
    {"Bottom_L1_N4", 4712},
    {"Bottom_L1_N3", 4713},
    {"Bottom_L1_N2", 4714},
    {"Bottom_L1_N1", 4715},

    // Bottom - L2
    {"Bottom_L2_N1", 4716},
    {"Bottom_L2_N2", 4717},
    {"Bottom_L2_N3", 4718},
    {"Bottom_L2_N4", 4719},

    // Bottom - L3
    {"Bottom_L3_N4", 4720},
    {"Bottom_L3_N3", 4721},
    {"Bottom_L3_N2", 4722},
    {"Bottom_L3_N1", 4723},

    // Right - L1
    {"Right_L1_N3", 4724},
    {"Right_L1_N2", 4725},
    {"Right_L1_N1", 4726},

    // Right - L2
    {"Right_L2_N3", 4727},
    {"Right_L2_N2", 4728},
    {"Right_L2_N1", 4729},

    // Right - L3
    {"Right_L3_N3", 4730},
    {"Right_L3_N2", 4731},
    {"Right_L3_N1", 4732},

    // Left - L1
    {"Left_L1_N3", 4733},
    {"Left_L1_N2", 4734},
    {"Left_L1_N1", 4735},

    // Left - L2
    {"Left_L2_N3", 4736},
    {"Left_L2_N2", 4737},
    {"Left_L2_N1", 4738},

    // Left - L3
    {"Left_L3_N3", 4739},
    {"Left_L3_N2", 4740},
    {"Left_L3_N1", 4741},

    // Back - L1
    {"Back_L1_N1", 4742},
    {"Back_L1_N2", 4743},
    {"Back_L1_N3", 4744},

    // Back - L2
    {"Back_L2_N1", 4745},
    {"Back_L2_N2", 4746},
    {"Back_L2_N3", 4747},

    // Back - L3
    {"Back_L3_N1", 4748},
    {"Back_L3_N2", 4749},
    {"Back_L3_N3", 4750},

    // Front - L1
    {"Front_L1_N3", 4751},
    {"Front_L1_N2", 4752},
    {"Front_L1_N1", 4753},

    // Front - L2
    {"Front_L2_N3", 4754},
    {"Front_L2_N2", 4755},
    {"Front_L2_N1", 4756},

    // Front - L3
    {"Front_L3_N3", 4757},
    {"Front_L3_N2", 4758},
    {"Front_L3_N1", 4759}
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
        auto position = plane->GetPosition() + plane->GetAxisX() * 100;
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

    map<int, TEvePointSet*> vetoNameToPoints;
    for (const auto& [name, daqId] : referenceVetoNameToDaqId) {
        cout << "Creating point set for " << name << " and DAQ ID: " << daqId << endl;
        vetoNameToPoints[daqId] = new TEvePointSet(name.c_str());
    }

    // generate random points to see if they are inside the veto
    for (int i = 0; i < 5000000; i++) {
        double x = gRandom->Uniform(-2000, 2000);
        double y = gRandom->Uniform(-2000, 2000);
        double z = gRandom->Uniform(-2000, 2000);

        Int_t daqId = -1, moduleId, channelId;
        Int_t lastGoodDaqId = -1;
        int uniqueDaqIds = 0;
        for (int p = 0; p < readout->GetNumberOfReadoutPlanes(); p++) {
            auto plane = readout->GetReadoutPlane(p);
            if (plane->GetType() != "veto") {
                continue;
            }
            std::tie(daqId, moduleId, channelId) = readout->GetHitsDaqChannelAtReadoutPlane({x, y, z}, p);
            if (daqId != -1) {
                uniqueDaqIds += 1;
                lastGoodDaqId = daqId;
            }
        }
        if (uniqueDaqIds == 0) {
            // outside all vetoes
            continue;
        }
        if (uniqueDaqIds > 1) {
            cerr << "WARNING: More than one readout plane found for point " << x << " " << y << " " << z
                 << endl;
            exit(1);
        }

        auto pointSet = vetoNameToPoints.at(lastGoodDaqId);
        pointSet->SetNextPoint(x / 10.0, y / 10.0, z / 10.0);
    }

    vector<int> colors = {1, 920, 632, 416, 600, 400, 616, 432, 800, 820, 840, 860, 880, 900};

    int counter = 0;
    for (const auto& [daqId, pointSet] : vetoNameToPoints) {
        counter++;
        pointSet->SetMarkerColor(colors[counter % colors.size()]);
        pointSet->SetMarkerSize(1);
        // pointSet->SetMarkerStyle(23);
        gEve->AddElement(pointSet);
    }

    gEve->Redraw3D(kTRUE);
}

bool IsTopOrBottom(const string& name) {
    // if name contains "Top" or "Bottom" return true
    return name.find("vetoSystemTop") != string::npos || name.find("vetoSystemBottom") != string::npos;
}

TRestDetectorReadout* GenerateReadout(const vector<VetoInfo>& vetoInfo) {
    TRestDetectorReadout readout;

    // verify aliasToSignalId has unique ids
    set<int> signalIds;
    for (const auto& [alias, signalId] : aliasToSignalId) {
        if (signalIds.find(signalId) != signalIds.end()) {
            cerr << "Signal ID " << signalId << " in volume -> signal map is not unique" << endl;
            exit(1);
        }
        signalIds.insert(signalId);
    }
    if (signalIds.size() != aliasToSignalId.size()) {
        cerr << "Signal IDs are not unique" << endl;
        exit(1);
    }

    int i = 0;
    for (const auto& veto : vetoInfo) {
        const auto vetoFromProcess = TRestGeant4VetoAnalysisProcess::GetVetoFromString(veto.volume);
        TRestDetectorReadoutPlane plane;
        plane.SetType("veto");

        // if we do not include this delta, some points in the boundaries are not correctly assigned to the
        // veto this can be a problem if one does not filter hits outside the vetoes. the distance between
        // vetoes should always be greater than this delta
        double delta = 1.0;

        plane.SetPosition(veto.readoutPosition - veto.normal * delta);
        plane.SetNormal(veto.normal);
        plane.SetHeight(veto.height + delta * 2);
        plane.SetID(i++);
        plane.SetAxisX(IsTopOrBottom(veto.volume) ? TVector3(1, 0, 0) : TVector3(0, 1, 0));

        TRestDetectorReadoutModule module;
        module.SetName(veto.volume);
        module.SetModuleID(0);

        TVector2 size = TVector2(200 + delta, 50 + delta);
        module.SetSize(size);
        module.SetOrigin(-1.0 * size / 2.0);

        TRestDetectorReadoutChannel channel;
        int channelId = aliasToSignalId[vetoFromProcess.alias];
        if (channelId == 0) {
            // not found
            cout << "WARNING: Channel ID not found for alias " << vetoFromProcess.alias << endl;
            exit(1);
            channelId = 10000 + i;
            // verify this is not in signalIds
            if (signalIds.find(channelId) != signalIds.end()) {
                cerr << "Signal ID " << channelId << " is not unique" << endl;
                exit(1);
            }
        }
        channel.SetChannelID(channelId);
        channel.SetDaqID(channelId);
        channel.SetChannelName(vetoFromProcess.alias);

        referenceVetoNameToDaqId[veto.volume] = channelId;

        TRestDetectorReadoutPixel pixel;
        pixel.SetSize(size);
        channel.AddPixel(pixel);

        module.AddChannel(channel);
        plane.AddModule(module);
        readout.AddReadoutPlane(plane);
    }

    auto file = TFile::Open(vetoSystemReadoutFile.c_str(), "RECREATE");
    const string readoutName = "vetoSystemReadout";
    readout.Write(readoutName.c_str());
    file->Close();

    file = TFile::Open(vetoSystemReadoutFile.c_str());
    TRestDetectorReadout* readoutFromFile = file->Get<TRestDetectorReadout>(readoutName.c_str());

    return readoutFromFile;
}

void TestReadout(TRestDetectorReadout* readout, const vector<VetoInfo>& vetoInfo) {
    std::map<string, int> volumeToChannelId;
    for (const auto veto : vetoInfo) {
        TVector3 position = veto.readoutPosition + veto.normal * (veto.height / 2.0);  // center of veto

        Int_t daqId, moduleId, channelId;
        for (int p = 0; p < readout->GetNumberOfReadoutPlanes(); p++) {
            std::tie(daqId, moduleId, channelId) = readout->GetHitsDaqChannelAtReadoutPlane(position, p);
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
    bool mismatch = false;
    for (const auto& [volume, daqId] : volumeToChannelId) {
        if (daqId != referenceVetoNameToDaqId[volume]) {
            cout << "Mismatching channel for volume " << volume << " DAQ ID: " << daqId
                 << " Ref DAQ ID: " << referenceVetoNameToDaqId[volume] << endl;
            mismatch = true;
        }
    }
    if (mismatch) {
        cerr << "Mismatching channels found" << endl;
        exit(1);
    }
}

void CheckUniqueChannels(TRestDetectorReadout* readout) {
    set<int> channelIds;
    set<int> channelDaqIds;

    for (int planeIndex = 0; planeIndex < readout->GetNumberOfReadoutPlanes(); planeIndex++) {
        auto plane = readout->GetReadoutPlane(planeIndex);
        for (int moduleIndex = 0; moduleIndex < plane->GetNumberOfModules(); moduleIndex++) {
            auto module = plane->GetModule(moduleIndex);
            for (int channelIndex = 0; channelIndex < module->GetNumberOfChannels(); channelIndex++) {
                auto channel = module->GetChannel(channelIndex);
                auto channelId = channel->GetChannelId();
                auto daqId = channel->GetDaqID();

                channelIds.insert(channelId);
                channelDaqIds.insert(daqId);
            }
        }
    }

    if (channelIds.size() != readout->GetNumberOfChannels() ||
        channelDaqIds.size() != readout->GetNumberOfChannels() || channelIds.size() == 0) {
        cerr << "Number of channels in readout does not match number of unique channels" << endl;
        exit(1);
    }

    cout << "All channel DAQ ids are unique" << endl;
}

void WriteReadoutWithVetoSystem(TRestDetectorReadout* vetoReadout) {
    // TRestDetectorReadout readout(rmlFile.c_str(), readoutName.c_str());
    TFile* readoutFile = TFile::Open(micromegasReadoutFile.c_str());
    TFile* outputFile = TFile::Open(fullReadoutFile.c_str(), "RECREATE");

    for (const auto& readoutName : readoutNames) {
        readoutFile->cd();
        TRestDetectorReadout* readout =
            dynamic_cast<TRestDetectorReadout*>(readoutFile->Get(readoutName.c_str()));
        if (!readout) {
            cerr << "Failed to load readout " << readoutName << endl;
            exit(1);
        }

        for (int i = 0; i < vetoReadout->GetNumberOfReadoutPlanes(); i++) {
            auto plane = vetoReadout->GetReadoutPlane(i);
            readout->AddReadoutPlane(*plane);
        }

        outputFile->cd();
        readout->Write(readoutName.c_str());
    }

    outputFile->Close();

    auto file = TFile::Open(fullReadoutFile.c_str());

    for (const auto& readoutName : readoutNames) {
        TRestDetectorReadout* readoutFromFile = file->Get<TRestDetectorReadout>(readoutName.c_str());
        if (!readoutFromFile) {
            cerr << "Failed to load readout " << readoutName << endl;
            exit(1);
        }
        CheckUniqueChannels(readoutFromFile);
    }

    file->Close();
}

void GenerateReadoutsWithVetoSystem(const char* simulationFilename = "simulation.root") {
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

    const auto vetoReadout = GenerateReadout(vetoInfo);

    TestReadout(vetoReadout, vetoInfo);
    cout << "Done testing readout" << endl;

    WriteReadoutWithVetoSystem(vetoReadout);

    auto file = TFile::Open(fullReadoutFile.c_str());

    for (const auto& readoutName : readoutNames) {
        TRestDetectorReadout* readoutFromFile =
            dynamic_cast<TRestDetectorReadout*>(file->Get<TRestDetectorReadout>(readoutName.c_str()));
        if (!readoutFromFile) {
            cerr << "Failed to load readout " << readoutName << endl;
            exit(1);
        }
        readoutFromFile->PrintMetadata(2);
        // Draw(vetoInfo, readoutFromFile);
    }

    cout << "Finished" << endl;
}
