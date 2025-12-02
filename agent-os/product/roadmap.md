# Product Roadmap

## Version 1.0 Release

The following features complete the v1.0 release. Current functionality (LoRa forwarding, WiFi connectivity, web interface, OTA updates, GPS, RTC, displays, SD card) is already implemented.

1. [x] Ethernet Connectivity — Enable W5500 Ethernet module support on the ATmega328P bridge, allowing the gateway to connect to the network via wired Ethernet as an alternative to WiFi `M`

2. [x] Network Failover — Implement automatic switching between WiFi and Ethernet connections when the primary connection fails, with configurable primary interface preference and failover timeout settings `M`

3. [x] Failover Status Display — Update LCD/OLED displays and web interface to show current active network interface and failover status `S`

4. [x] Ethernet Configuration UI — Add Ethernet settings page to web interface for configuring static IP/DHCP, DNS, and gateway settings for the wired connection `S`

5. [x] Connection Health Monitoring — Implement periodic connectivity checks to the network server with automatic failover trigger when packet forwarding fails `S`

6. [ ] v1.0 Documentation — Update README and create user guide covering Ethernet setup, failover configuration, and deployment best practices `S`

> Notes
> - Effort scale: XS (1 day), S (2-3 days), M (1 week), L (2 weeks), XL (3+ weeks)
> - Items 1-2 are core v1.0 requirements; items 3-6 complete the feature set
> - Ethernet connectivity (item 1) must be completed before failover (item 2)
> - Total estimated effort: approximately 4 weeks
