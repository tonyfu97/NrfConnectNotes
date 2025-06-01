# SDK 03: Custom Board Support using VS Code GUI

**Author:** Tony Fu  
**Date:** 2025/06/01  
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

We will go over how to create a simple custom board definition for the nRF52840 DK in the nRF Connect SDK using the GUI in VS Code. Then, we will use the board definition to build a simple blinky application to run on nRF52840 DK.

---

## Creating Custom Board support using West

In Zephyr, a "board target" correspond to an image of a specific hardware. I know, it sounds a bit abstract. But here is a breakdown. The name of a board target consists of the following parts:


- **Board Name (Must have)**: The name of the board, such as `nrf5340dk`.
- **Version (Optional)**: The version of the board, such as `0.1.0`.
- **SoC (Must have)**: The name of the SoC, such as `nrf5340`.
- **CPU Cluster (Optional)**: The CPU cluster of the SoC, such as `cpunet` for network processor or `cpuapp` for application processor.
- **Variant (Optional)**: The variant of the board, such as `ns` for Trusted Firmware-M support.

For example, the board target `nrf5340dk@0.1.0/nrf5340/cpunet/ns` corresponds to the nRF5340 DK board with the nRF5340 SoC, using the network processor cluster, and supporting Trusted Firmware-M. Notice that they are formatted as a path-like structure, which is how the build tool "west" organizes board targets. So, to create the above board target in your designated directory where you store all your custom boards, you would create a directory structure like this:

```bash
west build -b nrf5340dk@0.1.0/nrf5340/cpunet/ns -- -DBOARD_ROOT=C:/nordic/myBoards
```

You need to create all the board targets required by your board, so this mean you need to also call `west build` for the application processor cluster:

```bash
west build -b nrf5340dk@0.1.0/nrf5340/cpuapp -- -DBOARD_ROOT=C:/nordic/myBoards
```

This is the traditional way to create a custom board target in Zephyr. However, we will use the GUI in VS Code to make it easier.


---


## Creating Custom Board Support using VS Code GUI

On the left side of the VS Code, open the "nRF Connect" extension view. Then click on "Create a new board" button. This will open a window to create a new board target. You will need to fill in the following fields:

- **Board Name**: The name of the board, such as `my52840dk`.
- **Description**: A short description of the board. This is usually just the board name formatted in a more human readable way, such as `My 52840 DK`.
- **Vendor name**: The name of the vendor. Since I am an individual hobbyist, I will use my github username `tonyfu97`.
- **SoC**: This is a dropdown list of all the SoCs supported by the nRF Connect SDK. For our example, select `nRF52840-QIAA` (same package used in nRF52840). The GUI will automatically create all the required board targets for the SoC. 
- **Board root**: This is the directory where you want to store your custom board targets. You can use the default value, which is `C:/nordic/myBoards` in this example.

Then click on the "Create Board" button. The GUI will create the following directory structure in your designated board root directory:

```
C:
└───myBoards
    └───tonyfu97
        └───my52840dk
                board.cmake
                board.yml
                Kconfig.my52840dk
                my52840dk-pinctrl.dtsi
                my52840dk.dts
                my52840dk.yml
                my52840dk_defconfig
                pre_dt_board.cmake
```

You can see that the GUI has created all the required files for the board target. We will go over the important files in the next section.

---

## Default Configuration File: `my52840dk_defconfig`

This configuration file will be applied to the board target when building the application. However, the configuration in `prj.conf` will override the configuration in this file. This is useful for setting default configurations for the board target, such as enabling hardware features or disabling unused features.

The auto-generated `my52840dk_defconfig` file already contains the following configuration options:

```kconfig
CONFIG_ARM_MPU=y
CONFIG_HW_STACK_PROTECTION=y
```

We can foresee the following configurations will be needed for our custom board, so we will append them to the `my52840dk_defconfig` file:

```kconfig
# Enable RTT
CONFIG_USE_SEGGER_RTT=y

# enable GPIO
CONFIG_GPIO=y

# enable uart driver
CONFIG_SERIAL=y

# enable console
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
```

---

## Device Tree Source File: `my52840dk.dts`

While the kconfig files handle software configuration, the device tree source (DTS) file describes the hardware layout of the board. This is crucial for Zephyr to understand how to interact with the hardware components.

The device tree source file is used to describe the hardware of the board. The GUI has already created a basic device tree source file for us, which includes the following:

### 1. **Header**

```dts
/dts-v1/;
#include <nordic/nrf52840_qiaa.dtsi>
#include "my52840dk-pinctrl.dtsi"
```

* Includes common definitions for the **nRF52840 QIAA** variant.
* Includes your board-specific pin control configuration. We will go over the pin control file later.

### 2. **Root Node**

```dts
/ {
	model = "My 52840 DK";
	compatible = "tonyfu97,my52840dk";

	chosen {
		zephyr,sram = &sram0;
		zephyr,flash = &flash0;
		zephyr,code-partition = &slot0_partition;
	};
};
```

* Declares board name and unique ID string.
* `chosen` tells Zephyr the default hardware configurations. In this case, it specifies the SRAM and flash memory regions, as well as the primary code partition.

### 3. **Flash and Partition Layout**

```dts
&flash0 {
	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;
```

* `&flash0` refers to the main flash device from the included `.dtsi`.
* Defines fixed partitions with absolute addresses and sizes.

Partition definitions:

```dts
		boot_partition: partition@0 {
			label = "mcuboot";
			reg = <0x00000000 DT_SIZE_K(48)>;
		};

		slot0_partition: partition@c000 {
			label = "image-0";
			reg = <0x0000c000 DT_SIZE_K(472)>;
		};

		slot1_partition: partition@82000 {
			label = "image-1";
			reg = <0x00082000 DT_SIZE_K(472)>;
		};

		storage_partition: partition@f8000 {
			label = "storage";
			reg = <0x000f8000 DT_SIZE_K(32)>;
		};
```

* `mcuboot`: reserved for the bootloader.
* `image-0` / `image-1`: main and backup firmware images (for DFU).
* `storage`: typically used for settings or filesystem.

However, **this autogenerated DTS file is not sufficient**. We have to also specify the peripherals (e.g., GPIO, UART, I2C, SPI, and PWM) and their configuration. 

First, we need to turn them on outside of the root node. Add this to the end of the DTS file:

```dts
&gpiote {
	status = "okay";
};

&gpio0 {
	status = "okay";
};

&gpio1 {
	status = "okay";
};

&uart0 {
	compatible = "nordic,nrf-uarte";
	status = "okay";
	current-speed = <115200>;
	pinctrl-0 = <&uart0_default>;
	pinctrl-1 = <&uart0_sleep>;
	pinctrl-names = "default", "sleep";
};

&i2c0 {
	compatible = "nordic,nrf-twi";
	status = "okay";
	pinctrl-0 = <&i2c0_default>;
	pinctrl-1 = <&i2c0_sleep>;
	pinctrl-names = "default", "sleep";
};

&spi1 {
	compatible = "nordic,nrf-spi";
	status = "okay";
	pinctrl-0 = <&spi1_default>;
	pinctrl-1 = <&spi1_sleep>;
	pinctrl-names = "default", "sleep";
};

&pwm0 {
	status = "okay";
	pinctrl-0 = <&pwm0_default>;
	pinctrl-1 = <&pwm0_sleep>;
	pinctrl-names = "default", "sleep";
};
```

Notice that for the serial protocols (UART, I2C, SPI), we have specified the pin control configurations. These configurations are defined in the `my52840dk-pinctrl.dtsi` file, which we will go over next. Briefly, it is used to tell which pins are used for which function, such as UART TX/RX, I2C SCL/SDA, SPI MOSI/MISO/SCK, and PWM output. Also, the "compatible" property is used to specify the driver for the peripheral.

Then, inside of the root node (`/ { ... }`), we need to add the following to support the LEDs, buttons, and PWM LEDs. This is done by exposing them using aliases and defining their properties. The following is the complete device tree source file for our custom board:

```dts
/ {
	model = "My 52840 DK";
	compatible = "tonyfu97,my52840dk";

	chosen {
		zephyr,sram = &sram0;
		zephyr,flash = &flash0;
		zephyr,code-partition = &slot0_partition;
		zephyr,console =  &uart0 ;
		zephyr,shell-uart =  &uart0 ;
		zephyr,uart-mcumgr = &uart0;
	};

	leds {
		compatible = "gpio-leds";
		led0: led_0 {
			gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
			label = "Green LED 0";
		};
		led1: led_1 {
			gpios = <&gpio0 14 GPIO_ACTIVE_LOW>;
			label = "Green LED 1";
		};
		led2: led_2 {
			gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;
			label = "Green LED 2";
		};
		led3: led_3 {
			gpios = <&gpio0 16 GPIO_ACTIVE_LOW>;
			label = "Green LED 3";
		};
	};	

	buttons {
		compatible = "gpio-keys";
		button0: button_0 {
			gpios = <&gpio0 11 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "Push button switch 0";
			zephyr,code = <INPUT_KEY_0>;
		};
		button1: button_1 {
			gpios = <&gpio0 12 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "Push button switch 1";
			zephyr,code = <INPUT_KEY_1>;
		};
		button2: button_2 {
			gpios = <&gpio0 24 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "Push button switch 2";
			zephyr,code = <INPUT_KEY_2>;
		};
		button3: button_3 {
			gpios = <&gpio0 25 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "Push button switch 3";
			zephyr,code = <INPUT_KEY_3>;
		};
	};

	pwmleds {
		compatible = "pwm-leds";
		pwm_led0: pwm_led_0 {
			pwms = <&pwm0 0 PWM_MSEC(20) PWM_POLARITY_INVERTED>;
		};
	};
	
	/* These aliases are provided for compatibility with samples */
	aliases {
		led0 = &led0;
		led1 = &led1;
		led2 = &led2;
		led3 = &led3;
		pwm-led0 = &pwm_led0;
		sw0 = &button0;
		sw1 = &button1;
		sw2 = &button2;
		sw3 = &button3;
		bootloader-led0 = &led0;
		mcuboot-button0 = &button0;
		mcuboot-led0 = &led0;
		watchdog0 = &wdt0;
	};		
};		
```

You also need to include the input event codes header file to define the button codes. This is done by adding the following line at the top of the DTS file:

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>
```

---

## Pin Control File: `my52840dk-pinctrl.dtsi`

The "Device Tree Source Include" (dsti) file is used to define the pin control configurations for the board. This file is included in the main DTS file and contains the pin configurations for the peripherals.

The auto-generated `my52840dk-pinctrl.dtsi` file is almost empty:

```dts
&pinctrl {
};
```


