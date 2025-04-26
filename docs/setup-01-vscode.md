# 01 Setup nRF Connect SDK with VSCode

I would recommend [this tutorial series](https://youtu.be/EAJdOqsL9m8?si=0kT2u3M6l1WH-PJW) for setting up the nRF Connect SDK with VSCode.


---


## nrfutil

To my suprise, the VSCode extension (and the toolchain that it install) did not include the `nrfutil` package. This is required for flashing the firmware. Without it, the "flash" action button of the VSCode extension will not work.

Simply head to [this website](https://www.nordicsemi.com/Products/Development-tools/nRF-Util) and download the latest version of `nrfutil`. Then add it to your PATH. Open a terminal and run the following command to see what commands are available:

```bash
nrfutil search
```

You should see something like this:

```bash
Command           Installed Latest Status
91                          0.5.0  Not installed
ble-sniffer                 0.16.2 Not installed
completion                  1.5.0  Not installed
device                      2.9.0  Not installed
npm                         0.3.0  Not installed
nrf5sdk-tools     1.1.0     1.1.0  Installed
sdk-manager                 1.1.0  Not installed
suit                        0.9.0  Not installed
toolchain-manager           0.15.0 Not installed
trace                       3.3.1  Not installed
```

You should install the `device` command. This is required for flashing the firmware. To install it, run the following command:

```bash
nrfutil install device
```