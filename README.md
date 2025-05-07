# KMI Info Tool

A simple tool to extract Kernel Module Interface (KMI) information from Android boot images.

## Usage

```sh
chmod +x kmi
./kmi                       # Show kmi of system active slot, e.g.: /dev/block/by-name/boot_a
./kmi </path/to/boot.img>   # Show kmi of provided image
./kmi debug                 # Show verbose log
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

## Acknowledgement

- [Magisk](https://github.com/topjohnwu/Magisk): powerful magiskboot tool to extract data from boot image.
