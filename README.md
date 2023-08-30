
# IAXO Readouts

This repository contains all the readout related files for IAXO.

It contains the necessary configuration files to generate the readouts, and it also contains the readout itself.

## Readout generation

A readout is generated using the `TRestDetectorReadout` class initialized from a rml file.
This file contains the readout description such as number of channels, pitch, decoding file, etc.
A single rml file may contain multiple readout definitions identified by a name.

The readout generation is done in two steps:

- Initialization using the `TRestDetectorReadout` class.
- Write the readout to a root file as any other root object.

The final file, in this case `readouts.root`, contains all the readouts defined in the rml file.

In order to perform the readout generation the macro `generateReadouts.C` is provided. Calling the macro without any
arguments will generate all the readouts into a single `readouts.root` file.

