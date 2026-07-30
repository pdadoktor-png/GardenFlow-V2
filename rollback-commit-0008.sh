#!/usr/bin/env bash
set -euo pipefail

backup=".gardenflow-backup-0008"

if [[ ! -d "$backup" ]]; then
  echo "FEHLER: Backup $backup nicht gefunden."
  exit 1
fi

rm -rf \
  include/app include/hardware include/scheduler include/ui \
  include/storage include/network \
  src/app src/hardware src/scheduler src/ui src/storage src/network

cp "$backup/include/"*.h include/
cp "$backup/src/"*.cpp src/

rm -f MODULE_STRUCTURE.md

echo "Stand vor Commit 0008 wurde wiederhergestellt."
