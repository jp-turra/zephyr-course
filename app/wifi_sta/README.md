# Zephyr Wifi Management Flow

Application
   │
   ▼
net_mgmt (events: connect, disconnect, scan)
   │
   ▼
net_if (generic network interface, wifi/eth/etc abstraction)
   │
   ▼
wifi_mgmt driver (offloaded — CYW43/airoc)
   │
   ▼
Hardware

# Setup SSID and Password

Build menuconfig and look for "WiFi Station Configuration" or create "prj.conf.local" and set CONFIG_WIFI_SSID and CONFIG_WIFI_PSK.