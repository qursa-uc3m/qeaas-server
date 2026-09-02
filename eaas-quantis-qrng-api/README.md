# Quantis QRNG API

`GET /random_number/{1..8}` returns a random unsigned integer and reports the
source and processing used.

## Select a mode

| Source | Requirement | Flags |
|---|---|---|
| USB, direct | Quantis library + USB access | `--source usb --extract off` |
| USB, software-extracted | Same, plus bundled matrix | `--source usb --extract on` |
| PCIe, hardware-extracted | Host driver in RNG mode | `--source pcie --pcie-output extracted --extract off` |
| PCIe, raw samples | Host driver in SAMPLE mode | `--source pcie --pcie-output raw --extract off` |
| PCIe, software-extracted | SAMPLE mode + bundled matrix | `--source pcie --pcie-output raw --extract on` |
| Linux kernel CRNG | Nothing beyond Linux | `--source os --extract off --xor-os off` |

USB uses `libQuantis`/`libusb`; it does **not** create `/dev/qrandom0`. PCIe
requires the host kernel driver and is read from `/dev/qrandom0`. In normal RNG
mode the PCIe FPGA already applies hardware post-processing. SAMPLE mode is raw;
use it only for measurements or with `--extract on`.

If a QRNG is fed into the Linux entropy pool using `rngd` or equivalent, select
`--source os`. The API uses `getrandom()`, so its output comes from the Linux
kernel CRNG and may include that QRNG contribution; it is not attributable only
to the QRNG. OS XOR is therefore rejected for this source as redundant.

## Matrix installation is automatic

Yes: use the matrix already bundled in this repository:

```text
quantis-libraries/Libs-Apps/QuantisExtensions/default_idq_matrix.dat
```

It appears as `dependencies/quantis-libraries/...` inside the API build context.

`build.sh` and the Dockerfile call the library installer with `--extraction`.
That installer copies the matrix automatically to:

```text
/opt/quantis/share/quantis/default_idq_matrix.dat
```

Nothing is assumed to be preinstalled and no manual copy is needed. The API
loads the file only with `--extract on`. It is valid for USB or PCIe raw/SAMPLE
data, never for already-processed PCIe RNG data. `--matrix PATH` is only an
override for a custom compatible matrix.

## Build and run

Place the dependencies as described in
[`dependencies/README.md`](dependencies/README.md), then:

```bash
./build.sh --pcie-driver skip --os-xor on
./build/eaas-quantis-qrng-api --source os
```

For PCIe, `--pcie-driver extracted` installs/reloads the driver in normal RNG
mode; `--pcie-driver raw` selects SAMPLE mode. This is a host kernel setting,
not a container setting. The driver source is from the
[Quantis 20.2.4 package](https://github.com/qursa-uc3m/quantis-qrng-tls-pq-bench/tree/master/quantis-qrng-nginx/dependencies/pcie-chip-20.2.4-linux).

```text
--source usb|pcie|os
--pcie-output extracted|raw
--device-number N
--qrandom /dev/qrandom0
--extract on|off
--matrix PATH
--xor-os on|off
--fallback on|off
--port 6065
```

`--xor-os on` XORs hardware bytes with `getrandom()` and is available only when
built with `--os-xor on`. If a hardware read fails and `--fallback on`, the API
uses `getrandom()` and reports the fallback and original error. `getrandom()`
and `/dev/urandom` use the same initialized Linux CSPRNG, but `getrandom()` is
used everywhere here because it waits for initialization and needs no device
file. OpenSSL `RAND_bytes()` would add another wrapper around the OS RNG without
improving this Linux-only PoC's entropy source.

## Container

```bash
docker build --build-arg OS_XOR=ON -t qrng-api .

# No hardware access needed:
docker run --rm --network host qrng-api --source os

# PCIe driver must already be installed on the host:
docker run --rm --network host --device /dev/qrandom0:/dev/qrandom0:r qrng-api \
  --source pcie --pcie-output extracted
```

For USB, expose the required host USB device(s). Compose defaults to
`QRNG_PRIVILEGED=false`, which is sufficient for `--source os` and fallback.
For the simplest PoC hardware setup use `QRNG_PRIVILEGED=true`; direct
`docker run --device ...` mappings are preferable when the exact device is
known.
