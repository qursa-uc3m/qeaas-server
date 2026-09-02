#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
LIBRARIES_DIR="${QUANTIS_LIBRARIES_DIR:-${SCRIPT_DIR}/dependencies/quantis-libraries}"
DRIVERS_DIR="${QUANTIS_DRIVERS_DIR:-${SCRIPT_DIR}/dependencies/quantis-drivers}"
PCIE_DRIVER="skip"
OS_XOR="ON"

while (($#)); do
  case "$1" in
    --pcie-driver)
      PCIE_DRIVER="$2"
      shift 2
      ;;
    --os-xor)
      OS_XOR="${2^^}"
      shift 2
      ;;
    -h|--help)
      echo "Usage: $0 [--pcie-driver skip|extracted|raw] [--os-xor on|off]"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 2
      ;;
  esac
done

if [[ "${OS_XOR}" != "ON" && "${OS_XOR}" != "OFF" ]]; then
  echo "--os-xor expects on or off" >&2
  exit 2
fi

if [[ ! -x "${LIBRARIES_DIR}/install_quantis_libraries.sh" ]]; then
  echo "Place quantis-libraries in ${LIBRARIES_DIR}, or set QUANTIS_LIBRARIES_DIR" >&2
  exit 1
fi

"${LIBRARIES_DIR}/install_quantis_libraries.sh" --device usb --extraction

case "${PCIE_DRIVER}" in
  skip)
    ;;
  extracted|raw)
    if [[ ! -x "${DRIVERS_DIR}/install_quantis_driver.sh" ]]; then
      echo "Place quantis-drivers in ${DRIVERS_DIR}, or set QUANTIS_DRIVERS_DIR" >&2
      exit 1
    fi
    "${DRIVERS_DIR}/install_quantis_driver.sh" "${PCIE_DRIVER}"
    ;;
  *)
    echo "--pcie-driver expects skip, extracted, or raw" >&2
    exit 2
    ;;
esac

cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_OS_XOR="${OS_XOR}" \
  -DQUANTIS_ROOT=/opt/quantis
cmake --build "${SCRIPT_DIR}/build" --parallel
