# SDK 04: Custom Driver

**Author:** Tony Fu  
**Date:** 2025/06/08  
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## Understanding Zephyr Driver Architecture Through Bare-Metal Comparison

Let’s start by forgetting about Zephyr for a moment.

In traditional embedded development (bare-metal or close to it) when we connect an external component like a temperature sensor, we typically write our own driver. This is especially true for devices connected over common buses like SPI or I2C. We usually don’t write the SPI driver itself; that’s provided by the chip vendor through a HAL or SDK.

Suppose we have an SPI temperature sensor. To use it, we’d typically:

1. Define some register addresses and configuration constants.
2. Use vendor-provided SPI functions to read/write data.
3. Wrap those in our own driver API like this:

```c
#define REG_TEMP     0x01
#define CONFIG_VAL   0x7F

extern struct my_temp_sensor;

void temp_sensor_init();
int temp_sensor_get_reading();
```

Then, in our application code, we’d instantiate the sensor and call those functions to get temperature data.

In this approach, all the information needed to "drive" the sensor (e.g., address, bus config, access logic) is bundled directly into the firmware. It's simple and works well for small, self-contained systems.

This tightly couples our application to:

* A specific sensor model
* A particular interface (SPI/I2C)
* Custom driver logic

In a large codebase with many components or developers, this makes maintenance difficult. Changing a sensor requires touching both the driver and any application code using it. There's no clear separation between *what* the application wants and *how* the driver fulfills it.

---

### The Zephyr Way: Decoupling with Devicetree + Bindings

Zephyr introduces a clean abstraction. Instead of hardcoding driver specifics in the application:

* The **application** refers to devices by name (via devicetree).
* The **driver** is separate from the application.
* The **binding file** matches the device name to a driver implementation.
* The **devicetree system** links everything together.

In Zephyr, your C code just does something like:

```c
const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));
sensor_sample_fetch(dev);
sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
```

No mention of SPI, register addresses, or the specific part number.

---

### How Does It Work?

1. **Devicetree (DTS)** describes your hardware in a structured way:

   ```dts
   temp_sensor: temp@0 {
       compatible = "vendor,temp-sensor";
       reg = <0>;
       spi-max-frequency = <1000000>;
   };
   ```

2. A **binding file (YAML)** describes what this node means:

   ```yaml
   compatible: "vendor,temp-sensor"
   include: [sensor-device.yaml, spi-device.yaml]
   ```

3. The **build system** sees:

   * This node has `compatible = "vendor,temp-sensor"`.
   * It matches a binding that tells it this is a SPI sensor.
   * It generates C macros to pass config into your driver.
   * It includes the correct driver source file, thanks to `DT_DRV_COMPAT`.

So, the **application talks to the sensor via a generic API**, and **the build system injects the right implementation** based on the hardware described in the devicetree.

---

