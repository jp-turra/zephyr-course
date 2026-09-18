# Zephyr Training Environment

Welcome to the Zephyr RTOS training! This repository includes a ready-to-use
development environment based on Zephyr 4.3.0, which you can set up in one of
three ways:

---

## Manual Zephyr Setup

Follow the following guide:
- [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#).

Make sure to select appropriate OS and to perform all steps till
[Build the Blinky Sample](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#build-the-blinky-sample).


## Running Applications

### Source environment

```sh
source .venv/bin/activate
```

### Configuring

```sh
west build -t menuconfig -b rpi_pico/rp2040/w app/blink_kconfig/
```

### Build the application

```sh
west build -b \<board\> \<app/path\>
```

```sh
west build -b rpi_pico/rp2040/w app/blink_kconfig/
```

### Flashing

runner = `uf2`, `jlink` and more...

```sh
west flash --runner \<runner\>
```