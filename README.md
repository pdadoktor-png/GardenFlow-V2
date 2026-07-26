# GardenFlow V3.1.1 Fix

Behebt die fehlende Deklaration `setPulseDurationAll()` und die fehlende Theme-Funktion `panelAlt()`.

# GardenFlow V3.1 – funktionierende Navigation

Diese Version erweitert die funktionierende V3-Basis um vier echte Touch-Seiten:

- **MANUELL** – beide Ventilkarten und Schaltimpulse
- **PROGRAMME** – zwei vorbereitete Wochenprogramme mit Ein/Aus-Schaltern
- **STATUS** – Laufzeit, Ventilzustände, Impulszähler und Betriebsart
- **SETUP** – Displayhelligkeit und Ventilimpulsdauer

Die untere Leiste besteht jetzt aus echten LVGL-Schaltflächen und nicht mehr
aus einem Text-Label.

## Einbau in das bestehende Projekt

Da dein lokales PlatformIO-Projekt die Boarddefinition bereits enthält, kannst
du entweder dieses Projekt komplett verwenden oder nur folgende Dateien ersetzen:

- `include/DisplayManager.h`
- `include/ValveManager.h`
- `include/lv_conf.h`
- `src/DisplayManager.cpp`
- `src/ValveManager.cpp`
- `platformio.ini`

Den vorhandenen lokalen Ordner `boards/` bitte beibehalten.

## Neu kompilieren

```bash
rm -rf .pio
python3 -m platformio run
```

## Hochladen

```bash
python3 -m platformio run -t upload
```

## Hinweis

Die Ventile laufen weiterhin im sicheren Simulationsmodus, solange die GPIOs
in `include/AppConfig.h` auf `-1` stehen. Die 35-Ohm-Spulen dürfen nicht direkt
mit ESP32-GPIOs verbunden werden.
