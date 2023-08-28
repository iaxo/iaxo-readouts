restManager --c analysis.rml --i simulation.root --o simulation.analysis.root --j 8

FILENAME=iaxo-d1-20aug-2023-4layer-1000h
restManager --c /tmp/framework/projects/iaxo/iaxo-readouts/vetos/analysis.rml --i $FILENAME.root --o $FILENAME.analysis.root --j 16
