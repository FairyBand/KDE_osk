#!/usr/bin/env bash
set -euo pipefail

prefix="${1:-/usr/local}"
source_conf="${prefix}/share/kde-osk/sddm/kde-osk-sddm-wayland.conf"
target_conf="${KDE_OSK_SDDM_CONF:-/etc/sddm.conf.d/kde-osk-wayland.conf}"

if [[ ! -f "${source_conf}" ]]; then
    echo "KDE OSK SDDM config sample is not installed: ${source_conf}" >&2
    exit 1
fi

if [[ ${EUID} -ne 0 && "${target_conf}" == /etc/* ]]; then
    echo "Writing to ${target_conf} requires root. Re-run with sudo." >&2
    exit 1
fi

mkdir -p "$(dirname "${target_conf}")"
install -m 0644 "${source_conf}" "${target_conf}"

echo "Installed KDE OSK SDDM Wayland input-method config:"
echo "  ${target_conf}"
echo
echo "This affects only the SDDM greeter KWin instance. It does not change the"
echo "logged-in Plasma session virtual keyboard backend, so fcitx5 can remain the"
echo "desktop virtual keyboard/input method."
