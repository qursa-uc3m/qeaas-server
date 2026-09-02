# Quantis QRNG API

`GET /random_number/{1..8}` returns a random unsigned integer plus the source
and processing actually used.

## What each device needs

| Device/mode | Host setting | API flags |
|---|---|---|
| USB, device output | None; use `libusb-1.0` + `libQuantis` | `--source usb --extract off` |
| USB, software extraction | Same USB setup | `--source usb --extract on` |
| PCIe, normal use | Driver RNG mode (`default_qrng_mode=0`) | `--source pcie --pcie-output extracted --extract off` |
| PCIe, raw samples | Driver SAMPLE mode (`default_qrng_mode=1`) | `--source pcie --pcie-output raw --extract off` |
| PCIe raw + software extraction | Driver SAMPLE mode (`default_qrng_mode=1`) | `--source pcie --pcie-output raw --extract on` |

USB is **not** exposed as `/dev/qrandom0`; on Linux it is accessed through
`libusb-1.0`, here via `libQuantis`. PCIe is read directly from
`/dev/qrandom0`. The PCIe sources originate from the
[Quantis 20.2.4 dependency package](https://github.com/qursa-uc3m/quantis-qrng-tls-pq-bench/tree/master/quantis-qrng-nginx/dependencies/pcie-chip-20.2.4-linux).

The PCIe card is different from USB: its driver configures the FPGA. RNG mode
enables the card's hardware post-processing; SAMPLE mode disables it and
returns raw samples. Do not combine PCIe RNG mode with `--extract on`: the API
rejects that double-processing configuration. Raw PCIe output without software
extraction is allowed for measurements, but the response carries a warning and
must not be treated as cryptographic randomness.

OS XOR and `/dev/urandom` fallback are independent of extraction. XOR is
available only when built with `-DENABLE_OS_XOR=ON`.

## PCIe kernel mode

The mode is a **host kernel-driver setting**, not a container setting. The
installer selects it and reloads the module so it takes effect:

```bash
sudo ./dependencies/quantis-drivers/install_quantis_driver.sh extracted  # RNG, parameter 0
sudo ./dependencies/quantis-drivers/install_quantis_driver.sh raw        # SAMPLE, parameter 1
cat /sys/module/quantis_chip_pcie/parameters/default_qrng_mode
```

Writing the sysfs parameter after the card is initialized is insufficient; use
the installer to unload/reload the module. Stop readers of `/dev/qrandom0`
first. The API's `--pcie-output` value states what the host driver supplies and
must match it; the container deliberately does not load or reconfigure kernel
modules.

## Extraction matrix

The default 1024x768-bit matrix is installed at:

```text
/opt/quantis/share/quantis/default_idq_matrix.dat
```

It is loaded only with `--extract on`. It can be used for USB output or for
PCIe **raw/SAMPLE** output. It is not used for normal PCIe RNG output, because
that output is already post-processed by the card. The Docker image includes
the default matrix at the same path. A custom file must be compatible with the
API's 1024-input-bit/768-output-bit dimensions. Mount it read-only and pass its
container path:

```bash
docker run --rm \
  --device /dev/bus/usb:/dev/bus/usb \
  --mount type=bind,src=/host/custom_matrix.dat,dst=/matrix.dat,readonly \
  qrng-api --source usb --extract on --matrix /matrix.dat
```

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
--pcie-output extracted|raw
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
`"extraction"` reports software-matrix use for backward compatibility;
`"extraction_mode"` explicitly reports `none`, `software-matrix`, or
`pcie-hardware`. PCIe responses also report the configured `"pcie_output"`.

## Container

The Docker build expects `dependencies/quantis-libraries/` and supports:

```bash
docker build --build-arg OS_XOR=ON -t qrng-api .
docker run --rm --network host --device /dev/qrandom0:/dev/qrandom0:r qrng-api \
  --source pcie --pcie-output extracted --extract off --xor-os on --fallback on
```

The PCIe kernel driver must be installed on the host. The container needs only
the device, not `--privileged`: map `/dev/qrandom0` for PCIe or `/dev/bus/usb`
for USB. Compose uses `QRNG_DOCKER_DEVICE`; its PCIe default is
`/dev/qrandom0:/dev/qrandom0:r`. For USB set:

```bash
QRNG_SOURCE=usb \
QRNG_DOCKER_DEVICE=/dev/bus/usb:/dev/bus/usb \
docker compose up --build qrng-api
```
