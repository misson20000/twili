# Twili USB Driver Package

This driver package associates the libtransistor USB device, and its interfaces, with libusbK for use with Twili.

## Usage

Before the driver package can be installed, its files must be cataloged and digitally signed.

You have two options here:

1. Use the contributed self-signed driver package.
2. Catalog and sign the driver package yourself.

### Using the provided self-signed driver package

1. Double-click `twili.cer` and click the Install Certificate button.
2. Follow the Certificate Import Wizard steps to install the certificate into the Local Machine `Trusted Root Certification Authorities` store.
3. Open Device Manager and double-click any of the `TransistorUSB` devices listed under the `Other devices` heading.
4. Click the Update Driver button and browse to the folder containing the provided driver package.
5. Follow on-screen instructions to complete the install.
6. Repeat steps 3-5 for any remaining `TransistorUSB` devices.

### Catalog and sign the driver package yourself

1. Download the [Enterprise Windows Driver Kit (EWDK)](https://docs.microsoft.com/en-us/windows-hardware/drivers/develop/using-the-enterprise-wdk) (13GB)
2. Mount the Enterprise WDK image and start a new **elevated** build environment (`LaunchBuildEnv.cmd`).
3. In that environment, navigate to the driver package location in the source tree and execute the signing script (`SignDrivers.cmd`).
4. Follow self-signed driver package installation steps 4-6.
