# How to generate the readout

The readouts are defined in a RML file, in this case in `readouts_IAXOD0.rml`. This file contains definitions for multiple readouts which correspond to the section called `TRestDetectorReadout` with name given by the field `name`.

In order to generate the `readout.root` file follow this instructions. We use a RML file called `readouts_IAXOD0.rml` which contains the readout named `IAXOD0_Readout`.

* Open a ROOT prompt and load REST libraries (type `restRoot`).

* From the ROOT shell create a new `TRestDetectorReadout` object referencing the RML file and the `name` of the readout to use 

```TRestDetectorReadout *readout = new TRestDetectorReadout("readouts_IAXOD0.rml", "IAXOD0_Readout")```.

* Create a ROOT file to save the readout into

```TFile *f = new TFile("readouts.root", "RECREATE")```.

* Save the readout with a name of our choice which will be used to reference this readout inside the file (in this case "iaxo_readout"). One can create multiple readouts and save them all to the same file, then we can choose to load the one we need

```readout->Write("iaxo_readout")```.

* Safely close the file

```f->Close()```
