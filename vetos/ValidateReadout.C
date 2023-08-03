
#include <string>

#include "TRestDetectorSignalEvent.h"
#include "TRestRun.h"

using namespace std;

int ValidateReadout(const string& simulationWithAnalysisFilename = "simulation.analysis.root") {
    TRestRun run(simulationWithAnalysisFilename);

    TRestDetectorSignalEvent* event = nullptr;

    // run.SetInputEvent(event); // why not working?

    // EventTree->SetBranchAddress("TRestDetectorSignalEventBranch", &event);

    run.GetEntry(0);

    // TODO: why not working?

    return 0;
}
