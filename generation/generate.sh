
# Generate micromegas readouts

cd ./micromegas/
restRoot -q -b GenerateReadoutsMicromegas.C

# Generate veto readouts

cd ../vetos/
bash simulation.sh
restRoot -q -b GenerateReadoutsWithVetoSystem.C

cd ..
ls -lht ../readouts/
