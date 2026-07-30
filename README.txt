GardenFlow Hotfix 0009b-2

Behebt die leere PROGRAMME-Seite.

Geaendert:
- eigene sichtbare/scrollbare Containerflaeche fuer die Programmliste
- Programmliste wird beim Oeffnen der Seite neu aufgebaut
- Container wird in den Vordergrund gesetzt und neu gezeichnet
- Kompatibilitaetsheader bleibt nur Weiterleitung

Dateien ersetzen:
include/ProgramsPage.h
include/ui/ProgramsPage.h
src/ui/ProgramsPage.cpp
src/ui/DisplayManager.cpp

Danach:
rm -rf .pio
pio run -e esp32-4827S043R
pio run -e esp32-4827S043R -t upload
