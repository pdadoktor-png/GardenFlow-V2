#!/usr/bin/env bash
set -euo pipefail

# GardenFlow Commit 0008 – sichere Modulstruktur
# Im Wurzelverzeichnis des PlatformIO-Projekts ausführen.

require_file() {
  if [[ ! -f "$1" ]]; then
    echo "FEHLER: Datei fehlt: $1"
    exit 1
  fi
}

for file in \
  include/AppConfig.h \
  include/Theme.h \
  include/ValveManager.h \
  include/DisplayManager.h \
  include/Scheduler.h \
  include/ProgramsPage.h \
  src/ValveManager.cpp \
  src/DisplayManager.cpp \
  src/Scheduler.cpp \
  src/ProgramsPage.cpp
do
  require_file "$file"
done

backup=".gardenflow-backup-0008"
if [[ -e "$backup" ]]; then
  echo "FEHLER: $backup existiert bereits."
  echo "Bitte zuerst umbenennen oder löschen."
  exit 1
fi

mkdir -p "$backup/include" "$backup/src"
cp include/AppConfig.h include/Theme.h include/ValveManager.h \
   include/DisplayManager.h include/Scheduler.h include/ProgramsPage.h \
   "$backup/include/"
cp src/ValveManager.cpp src/DisplayManager.cpp src/Scheduler.cpp \
   src/ProgramsPage.cpp "$backup/src/"

mkdir -p \
  include/app \
  include/hardware \
  include/scheduler \
  include/ui \
  include/storage \
  include/network \
  src/app \
  src/hardware \
  src/scheduler \
  src/ui \
  src/storage \
  src/network

# Echte Header in Modulordner verschieben.
mv include/AppConfig.h include/app/AppConfig.h
mv include/Theme.h include/ui/Theme.h
mv include/ValveManager.h include/hardware/ValveManager.h
mv include/DisplayManager.h include/ui/DisplayManager.h
mv include/Scheduler.h include/scheduler/Scheduler.h
mv include/ProgramsPage.h include/ui/ProgramsPage.h

# Implementierungen verschieben. PlatformIO kompiliert src rekursiv.
mv src/ValveManager.cpp src/hardware/ValveManager.cpp
mv src/DisplayManager.cpp src/ui/DisplayManager.cpp
mv src/Scheduler.cpp src/scheduler/Scheduler.cpp
mv src/ProgramsPage.cpp src/ui/ProgramsPage.cpp

# Kompatibilitätsheader erhalten die bisherigen Include-Namen.
cat > include/AppConfig.h <<'EOF'
#pragma once
#include "app/AppConfig.h"
EOF

cat > include/Theme.h <<'EOF'
#pragma once
#include "ui/Theme.h"
EOF

cat > include/ValveManager.h <<'EOF'
#pragma once
#include "hardware/ValveManager.h"
EOF

cat > include/DisplayManager.h <<'EOF'
#pragma once
#include "ui/DisplayManager.h"
EOF

cat > include/Scheduler.h <<'EOF'
#pragma once
#include "scheduler/Scheduler.h"
EOF

cat > include/ProgramsPage.h <<'EOF'
#pragma once
#include "ui/ProgramsPage.h"
EOF

cat > include/storage/ProgramStorage.h <<'EOF'
#pragma once

// Reservierter Modulanschluss.
// Die vorhandene Preferences-Speicherung bleibt vorerst im Scheduler.
// Sie wird erst in einem eigenen, getesteten Commit hierher verschoben.
class ProgramStorage;
EOF

cat > include/network/NetworkManager.h <<'EOF'
#pragma once

// Reservierter Modulanschluss für WLAN/NTP.
// Noch keine Implementierung, damit der aktuelle Build unverändert bleibt.
class NetworkManager;
EOF

cat > include/app/GardenFlowApp.h <<'EOF'
#pragma once

// Reservierter Modulanschluss für die spätere zentrale App-Klasse.
// setup()/loop() und die bestehende Initialisierung bleiben vorerst unverändert.
class GardenFlowApp;
EOF

cat > MODULE_STRUCTURE.md <<'EOF'
# GardenFlow Modulstruktur

## Aktive Module

- `include/app/AppConfig.h`
- `include/hardware/ValveManager.h`
- `src/hardware/ValveManager.cpp`
- `include/scheduler/Scheduler.h`
- `src/scheduler/Scheduler.cpp`
- `include/ui/Theme.h`
- `include/ui/DisplayManager.h`
- `src/ui/DisplayManager.cpp`
- `include/ui/ProgramsPage.h`
- `src/ui/ProgramsPage.cpp`

## Kompatibilität

Die bisherigen Header in `include/` bleiben als Weiterleitungen bestehen.
Deshalb funktionieren vorhandene Includes wie:

```cpp
#include "Scheduler.h"
#include "DisplayManager.h"
```

weiterhin unverändert.

## Bewusst noch nicht verschoben

- Preferences-Speicherung aus `Scheduler`
- Initialisierung aus `main.cpp`
- Netzwerk- und Zeitverwaltung

Diese Änderungen betreffen Verhalten und Schnittstellen. Sie werden nicht mit
der reinen Ordnerumstellung vermischt.
EOF

echo
echo "Modulstruktur erfolgreich angelegt."
echo "Backup: $backup"
echo
echo "Jetzt testen:"
echo "  pio run -e esp32-4827S043R"
