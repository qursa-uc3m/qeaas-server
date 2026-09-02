# Quantis QRNG API

`GET /random_number/{1..8}` returns a random unsigned integer plus the source
and processing actually used.

## What each device needs

| Device/mode | Required path | API flags |
|---|---|---|
| USB, no extraction | `libusb-1.0` + `libQuantis` | `--source usb --extract off` |
| USB, software extraction | Above + `libQuantis_Extensions` and matrix | `--source usb --extract on` |
| PCIe, hardware-extracted | PCIe driver creates `/dev/qrandom0` in RNG mode | `--source pcie --extract off` |
| PCIe, raw samples | PCIe driver creates `/dev/qrandom0` in sample mode | `--source pcie --extract off` |
| PCIe, software extraction | PCIe driver in sample mode + matrix library | `--source pcie --extract on` |

USB is **not** exposed as `/dev/qrandom0`; on Linux it is accessed through
`libusb-1.0`, here via `libQuantis`. PCIe is read directly from
`/dev/qrandom0`. The PCIe sources originate from the
[Quantis 20.2.4 dependency package](https://github.com/qursa-uc3m/quantis-qrng-tls-pq-bench/tree/master/quantis-qrng-nginx/dependencies/pcie-chip-20.2.4-linux).

OS XOR and `/dev/urandom` fallback are independent of extraction. XOR is
available only when built with `-DENABLE_OS_XOR=ON`.

## Build and run

Place `quantis-libraries/` and optionally `quantis-drivers/` under
[`dependencies/`](dependencies/README.md), then:

```bash
./build.sh --pcie-driver skip --os-xor on
./build/eaas-quantis-qrng-api \
  --source usb \
  --device-number 0 \
  --extract on \
  --xor-os on \
  --fallback on
```

Use `--pcie-driver extracted` for hardware-extracted PCIe output or
`--pcie-driver raw` when the API will return raw samples or apply software
extraction. `build.sh` installs the USB/extraction libraries because one binary
supports both devices. Drogon must already be installed for a host build.

All runtime flags:

```text
--source usb|pcie
--device-number N
--qrandom /dev/qrandom0
--extract on|off
--matrix PATH
--xor-os on|off
--fallback on|off
--port 6065
```

If the primary QRNG read fails and fallback is on, the response uses
`/dev/urandom` and reports `"fallback": true` and a warning.

## Container

The Docker build expects `dependencies/quantis-libraries/` and supports:

```bash
docker build --build-arg OS_XOR=ON -t qrng-api .
docker run --rm --privileged --network host qrng-api \
  --source pcie --extract off --xor-os on --fallback on
```

The PCIe kernel driver must be installed on the host. `--privileged` is the
simple PoC mechanism that exposes either the PCIe node or USB bus to the
container.
