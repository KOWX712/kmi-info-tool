# KMI Info Tool

A simple tool to extract Kernel Module Interface (KMI) information from Android boot images.

## Usage

```sh
chmod +x kmi
./kmi                       # Show kmi of system active slot, e.g.: /dev/block/by-name/boot_a
./kmi </path/to/boot.img>   # Show kmi of provided image
./kmi --debug               # Show verbose log
```

### Example output

- This tool outputs a KMI string in the format:

```
KMI: android12-5.10.209_2024-09-lz4-boot
```

Where:
- `kernel_version`: Extracted from the kernel version string
- `security_patch`: The security patch level of the boot image
- `compression`: The compression format used for the kernel

- [KernelSU KMI document](https://kernelsu.org/guide/installation.html#kmi)

## Variation

- **kmi.sh** - shell version, no magiskboot built-in, support Android/Linux, path required on Linux, support system boot image on Android (root required).
- **kmi** - Android arm64, built-in magiskboot, support system boot image (root required).
- **kmi-linux** - Linux x86_64, built-in magiskboot, path required.

## Acknowledgement

- [Magisk](https://github.com/topjohnwu/Magisk): powerful magiskboot tool to extract data from boot image.
