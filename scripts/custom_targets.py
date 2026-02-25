# Register custom PlatformIO/SCons targets for CYD_DataDisplay
# - `-t merged`: Creates CYD_DataDisplay_v<version>[_debug]_FULL.bin for Web Serial flashing
#                (bootloader + partition table + boot_app0 + app)
# - `-t ota`:    Creates CYD_DataDisplay_v<version>[_debug]_OTA.bin for OTA updates
#                (app only, same binary the OTA updater applies)
#
# Binaries are placed in CYD_DataDisplay/bin/:
#   bin/CYD_DataDisplay_v1.4.1_FULL.bin       (release, full flash image)
#   bin/CYD_DataDisplay_v1.4.1_OTA.bin        (release, OTA app image)
#   bin/CYD_DataDisplay_v1.4.1_debug_FULL.bin (debug variant)
#   bin/CYD_DataDisplay_v1.4.1_debug_OTA.bin
#
# Any existing binary of the same type/version is removed before creating a new one.
# Usage:
#   pio run -e release -t merged
#   pio run -e release -t ota
#   pio run -e debug   -t merged
#   pio run -e debug   -t ota

import os
import re
import glob
import shutil
import subprocess
from SCons.Script import Import  # type: ignore

Import("env")

PRODUCT_NAME = "CYD_DataDisplay"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _ensure_dir(path: str):
    os.makedirs(path, exist_ok=True)


def _read(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read()


def _get_version(project_dir: str) -> str:
    """Extract FIRMWARE_VERSION from src/main.cpp.

    Looks for:  const char *FIRMWARE_VERSION = "x.y.z";
    """
    main_cpp = os.path.join(project_dir, "src", "main.cpp")
    try:
        with open(main_cpp, "r") as f:
            for line in f:
                m = re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]+)"', line)
                if m:
                    return m.group(1)
    except Exception as e:
        print(f"[custom_targets] Warning: could not read version from {main_cpp}: {e}")
    return "UNKNOWN"


def _build_suffix(env_name: str) -> str:
    """Return filename suffix based on environment name.

    release → ""         → "CYD_DataDisplay_v1.4.1_FULL.bin"
    debug   → "_debug"   → "CYD_DataDisplay_v1.4.1_debug_FULL.bin"
    """
    return "_debug" if "debug" in env_name.lower() else ""


def _merge_segments(segments):
    """Python fallback: stitch binary segments into a single flash image."""
    if not segments:
        return b""
    end = max(off + len(data) for off, data in segments)
    image = bytearray(b"\xFF" * end)
    for off, data in segments:
        image[off: off + len(data)] = data
    return bytes(image)


# ---------------------------------------------------------------------------
# Merged-binary action  (bootloader + partitions + boot_app0 + app)
# ---------------------------------------------------------------------------

def _do_merge(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir   = env.subst("$BUILD_DIR")
    env_name    = env.subst("$PIOENV")

    version  = _get_version(project_dir)
    suffix   = _build_suffix(env_name)
    filename = f"{PRODUCT_NAME}_v{version}{suffix}_FULL.bin"

    # ESP32 classic: bootloader at 0x1000
    OFF_BOOTLOADER = 0x1000
    OFF_PARTITIONS = 0x8000
    OFF_BOOT_APP0  = 0xE000
    OFF_APP        = 0x10000

    firmware_path    = os.path.join(build_dir, env.subst("${PROGNAME}.bin"))
    bootloader_path  = os.path.join(build_dir, "bootloader.bin")
    partitions_path  = os.path.join(build_dir, "partitions.bin")
    boot_app0_path   = os.path.join(build_dir, "boot_app0.bin")

    # Validate inputs
    missing = [n for n, p in [
        ("firmware.bin",    firmware_path),
        ("bootloader.bin",  bootloader_path),
        ("partitions.bin",  partitions_path),
    ] if not os.path.exists(p)]
    if missing:
        print(f"[merged target] ERROR: missing required files: {', '.join(missing)}")
        return 1

    out_dir = os.path.join(project_dir, "bin")
    _ensure_dir(out_dir)

    # Remove stale binaries of the same type/suffix before writing new one
    for old in glob.glob(os.path.join(out_dir, f"{PRODUCT_NAME}_*{suffix}_FULL.bin")):
        try:
            os.remove(old)
            print(f"[merged target] Removed old binary: {os.path.basename(old)}")
        except Exception as e:
            print(f"[merged target] Warning: could not remove {old}: {e}")

    merged_tmp = os.path.join(build_dir, filename)
    merged_dst = os.path.join(out_dir, filename)

    # Try system esptool first, fall back to PlatformIO's bundled esptool.py, then Python merge
    chip = "esp32"
    segments_args = [
        f"0x{OFF_BOOTLOADER:X}", bootloader_path,
        f"0x{OFF_PARTITIONS:X}", partitions_path,
    ]
    if os.path.exists(boot_app0_path):
        segments_args += [f"0x{OFF_BOOT_APP0:X}", boot_app0_path]
    segments_args += [f"0x{OFF_APP:X}", firmware_path]

    merged_ok = False

    esptool_exe = shutil.which("esptool")
    if esptool_exe:
        cmd = [esptool_exe, "--chip", chip, "merge-bin", "-o", merged_tmp] + segments_args
        print("[merged target] Using system esptool:", " ".join(cmd))
        try:
            subprocess.run(cmd, check=True)
            merged_ok = True
        except Exception as e:
            print(f"[merged target] system esptool failed: {e}")

    if not merged_ok:
        try:
            platform   = env.PioPlatform()
            esptool_dir = platform.get_package_dir("tool-esptoolpy")
            if esptool_dir:
                pyexe      = env.subst("$PYTHONEXE") or "python3"
                esptool_py = os.path.join(esptool_dir, "esptool.py")
                cmd = [pyexe, esptool_py, "--chip", chip, "merge_bin", "-o", merged_tmp] + segments_args
                print("[merged target] Using bundled esptool.py:", " ".join(cmd))
                subprocess.run(cmd, check=True)
                merged_ok = True
        except Exception as e:
            print(f"[merged target] bundled esptool failed: {e}")

    if not merged_ok:
        print("[merged target] Falling back to Python segment merge")
        segs = [
            (OFF_BOOTLOADER, _read(bootloader_path)),
            (OFF_PARTITIONS, _read(partitions_path)),
            (OFF_APP,        _read(firmware_path)),
        ]
        if os.path.exists(boot_app0_path):
            segs.append((OFF_BOOT_APP0, _read(boot_app0_path)))
        with open(merged_tmp, "wb") as f:
            f.write(_merge_segments(segs))

    shutil.copy2(merged_tmp, merged_dst)
    size = os.path.getsize(merged_dst)
    print(f"[merged target] Created: {merged_dst} ({size:,} bytes)")
    return 0


# ---------------------------------------------------------------------------
# OTA-binary action  (app only — what the HTTP OTA updater flashes)
# ---------------------------------------------------------------------------

def _do_ota(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir   = env.subst("$BUILD_DIR")
    env_name    = env.subst("$PIOENV")

    version  = _get_version(project_dir)
    suffix   = _build_suffix(env_name)
    filename = f"{PRODUCT_NAME}_v{version}{suffix}_OTA.bin"

    firmware_path = os.path.join(build_dir, env.subst("${PROGNAME}.bin"))
    if not os.path.exists(firmware_path):
        print(f"[ota target] ERROR: firmware.bin not found: {firmware_path}")
        return 1

    out_dir = os.path.join(project_dir, "bin")
    _ensure_dir(out_dir)

    # Remove stale OTA binaries of the same type/suffix
    for old in glob.glob(os.path.join(out_dir, f"{PRODUCT_NAME}_*{suffix}_OTA.bin")):
        try:
            os.remove(old)
            print(f"[ota target] Removed old binary: {os.path.basename(old)}")
        except Exception as e:
            print(f"[ota target] Warning: could not remove {old}: {e}")

    ota_dst = os.path.join(out_dir, filename)
    shutil.copy2(firmware_path, ota_dst)
    size = os.path.getsize(ota_dst)
    print(f"[ota target] Created: {ota_dst} ({size:,} bytes)")
    return 0


# ---------------------------------------------------------------------------
# Register targets
# ---------------------------------------------------------------------------

env.AddCustomTarget(
    name="merged",
    dependencies=["$BUILD_DIR/${PROGNAME}.bin"],
    actions=[_do_merge],
    title="Export merged firmware (FULL flash image)",
    description=(
        f"Create {PRODUCT_NAME}_v<version>[_debug]_FULL.bin in ./bin/ "
        "for Web Serial / esptool full flashing"
    ),
)

env.AddCustomTarget(
    name="ota",
    dependencies=["$BUILD_DIR/${PROGNAME}.bin"],
    actions=[_do_ota],
    title="Export OTA firmware (app only)",
    description=(
        f"Create {PRODUCT_NAME}_v<version>[_debug]_OTA.bin in ./bin/ "
        "for HTTP OTA updates"
    ),
)
