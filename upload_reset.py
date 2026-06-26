Import("env")
import glob
import os
import serial.tools.list_ports
import subprocess
import sys
import time


def _find_esptool():
    base = os.path.join(os.path.expanduser("~"), ".platformio", "packages")
    candidate = os.path.join(base, "tool-esptoolpy", "esptool.py")
    if os.path.exists(candidate):
        return candidate
    hits = glob.glob(os.path.join(base, "tool-esptoolpy*", "esptool.py"))
    return hits[0] if hits else None


def _find_boot_app0():
    base = os.path.join(os.path.expanduser("~"), ".platformio", "packages")
    for pattern in [
        os.path.join(base, "framework-arduinoespressif32", "tools", "partitions", "boot_app0.bin"),
        os.path.join(base, "framework-arduinoespressif32*", "tools", "partitions", "boot_app0.bin"),
    ]:
        hits = glob.glob(pattern)
        if hits:
            return hits[0]
    return None


def _find_esp32_port():
    """Findet den ESP32-Port automatisch über Espressif VID 0x303A."""
    for p in serial.tools.list_ports.comports():
        if p.vid == 0x303A:
            return p.device
    return None


def upload_with_reset(source, target, env):
    app_port  = env.get("UPLOAD_PORT", "")
    build_dir = env.subst("$BUILD_DIR")
    firmware  = str(source[0])

    boot_app0 = _find_boot_app0()
    if not boot_app0:
        print("[reset] FEHLER: boot_app0.bin nicht gefunden")
        sys.stdout.flush()
        env.Exit(1)
        return

    esptool = _find_esptool()
    if not esptool:
        print("[reset] FEHLER: esptool.py nicht gefunden")
        sys.stdout.flush()
        env.Exit(1)
        return

    # ── 1. App-Port bestimmen ──────────────────────────────────────
    # Konfigurierter Port prüfen, sonst automatisch per VID suchen
    ports_now = {p.device for p in serial.tools.list_ports.comports()}
    if app_port not in ports_now:
        detected = _find_esp32_port()
        if detected:
            print(f"[reset] Konfigurierter Port {app_port} nicht gefunden, "
                  f"verwende automatisch erkannten Port {detected}")
            app_port = detected
        else:
            print(f"[reset] Kein ESP32 (VID 0x303A) gefunden — verbunden?")
            sys.stdout.flush()
            env.Exit(1)
            return

    # ── 2. 1200bps Touch ───────────────────────────────────────────
    print(f"\n[reset] Sende 1200bps Touch an {app_port} ...")
    sys.stdout.flush()
    try:
        s = serial.Serial(app_port, 1200, timeout=1)
        time.sleep(0.1)
        s.close()
    except Exception as e:
        print(f"[reset] Fehler beim Öffnen von {app_port}: {e}")
        print("[reset] -> SimHub geschlossen?")
        sys.stdout.flush()
        env.Exit(1)
        return

    # ── 3. Warte auf Bootloader-Port ───────────────────────────────
    # Szenario A: gleicher Port (COM5) verschwindet und kommt wieder
    # Szenario B: neuer Port erscheint (andere VID/PID im Bootloader)
    print("[reset] Warte auf Bootloader ...")
    sys.stdout.flush()

    before = {p.device for p in serial.tools.list_ports.comports()}
    bl_port = None

    # Warte max 3s bis app_port verschwunden ist
    for _ in range(30):
        time.sleep(0.1)
        if app_port not in {p.device for p in serial.tools.list_ports.comports()}:
            break

    # Warte max 5s auf Rückkehr desselben Ports ODER neuen Port
    for _ in range(50):
        time.sleep(0.1)
        after = {p.device for p in serial.tools.list_ports.comports()}
        new = after - before
        if new:                          # Szenario B: neuer Port
            bl_port = sorted(new)[0]
            print(f"[reset] Neuer Bootloader-Port: {bl_port}")
            break
        if app_port in after:            # Szenario A: gleicher Port zurück
            bl_port = app_port
            print(f"[reset] Bootloader auf {bl_port} (gleicher Port)")
            break

    if not bl_port:
        print("[reset] Kein Bootloader-Port gefunden — ESP32 manuell resetten?")
        sys.stdout.flush()
        env.Exit(1)
        return

    time.sleep(0.3)  # kurze Pause für USB-Stabilität
    sys.stdout.flush()

    # ── 4. esptool ─────────────────────────────────────────────────
    cmd = [
        env.subst("$PYTHONEXE"), esptool,
        "--chip", "esp32s2",
        "--port", bl_port,
        "--baud", "921600",
        "--before", "no_reset",
        "--after", "hard_reset",
        "write_flash", "-z",
        "0x1000",  os.path.join(build_dir, "bootloader.bin"),
        "0x8000",  os.path.join(build_dir, "partitions.bin"),
        "0xe000",  boot_app0,
        "0x10000", firmware,
    ]

    result = subprocess.run(cmd)
    if result.returncode != 0:
        env.Exit(result.returncode)


env.Replace(UPLOADCMD=upload_with_reset)
