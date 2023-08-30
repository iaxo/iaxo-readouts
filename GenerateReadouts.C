
#include <TFile.h>
#include <TRestDetectorReadout.h>

using namespace std;

void GenerateReadouts() {
    const string rmlFile = "readoutsIAXO.rml";
    const vector<string> readoutNames = {"iaxoD0Readout", "iaxoD1Readout"};
    const string outputFilename = "readouts.root";

    TFile* file = TFile::Open(outputFilename.c_str(), "RECREATE");

    for (const auto& readoutName : readoutNames) {
        TRestDetectorReadout readout(rmlFile.c_str(), readoutName.c_str());
        readout.Write(readoutName.c_str());

        // print some readout info
        readout.PrintMetadata(3);
    }

    file->Close();
}
