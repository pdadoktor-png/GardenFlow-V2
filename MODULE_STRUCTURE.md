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
