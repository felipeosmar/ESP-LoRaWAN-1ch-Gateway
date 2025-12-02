# Product Mission

## Pitch

ESP-LoRaWAN-1ch-Gateway is an open-source single-channel LoRaWAN gateway that helps small businesses and industrial/agricultural operations create their own private LoRaWAN network by providing affordable, customizable hardware and software that integrates seamlessly with ChirpStack network servers.

## Users

### Primary Customers

- **Small Businesses**: Companies needing affordable IoT connectivity for monitoring assets, environmental conditions, or equipment status without enterprise-level gateway costs
- **Agricultural Operations**: Farms and agricultural businesses requiring long-range, low-power sensor networks for soil monitoring, livestock tracking, or irrigation control
- **Industrial Facilities**: Manufacturing plants and warehouses deploying sensor networks for environmental monitoring, asset tracking, or process automation

### User Personas

**Operations Manager** (35-55)
- **Role:** Facility or farm manager responsible for operational efficiency
- **Context:** Managing multiple sensors across a large physical area with limited IT budget
- **Pain Points:** Commercial LoRaWAN gateways are expensive; cloud-dependent solutions have recurring costs; proprietary systems lock them into vendor ecosystems
- **Goals:** Deploy a reliable, low-cost sensor network that can be maintained and customized in-house

**Technical Integrator** (25-45)
- **Role:** System integrator or technical consultant implementing IoT solutions
- **Context:** Building custom IoT solutions for clients with specific requirements
- **Pain Points:** Closed-source gateways limit customization; need to support various hardware configurations
- **Goals:** Deploy flexible gateway solutions that can be adapted to client-specific hardware and requirements

## The Problem

### Expensive and Inflexible LoRaWAN Infrastructure

Commercial LoRaWAN gateways cost hundreds to thousands of dollars and often require ongoing subscription fees. Small businesses and agricultural operations need affordable alternatives that provide essential functionality without unnecessary complexity.

**Our Solution:** An open-source gateway running on low-cost ESP32 hardware that provides core LoRaWAN packet forwarding with web-based configuration, supporting multiple connectivity options (WiFi and Ethernet) for reliable operation in various environments.

## Differentiators

### Open-Source and Hardware-Agnostic

Unlike proprietary gateways, ESP-LoRaWAN-1ch-Gateway provides fully open-source firmware that can be customized for any requirement. While JVTech hardware is available for purchase, the software supports multiple ESP32 boards (Heltec V2, TTGO LoRa32, custom configurations). This results in lower total cost of ownership and freedom from vendor lock-in.

### Built-in Reliability Features

Unlike basic DIY gateway projects, this gateway includes enterprise-grade features such as network failover (WiFi to Ethernet), RTC backup for time accuracy during network outages, GPS for precise positioning, and comprehensive web-based configuration. This results in reliable operation suitable for production deployments.

### ChirpStack Integration

Full compatibility with ChirpStack network server using the Semtech UDP Packet Forwarder v2 protocol. This enables integration with a mature, open-source LoRaWAN network server ecosystem without proprietary dependencies.

## Key Features

### Core Features

- **LoRa Packet Forwarding:** Receive LoRa packets from end devices and forward them to the network server using standard Semtech UDP protocol
- **Dual Connectivity:** Support for both WiFi and Ethernet connections with automatic failover for reliable network access
- **Web Configuration Interface:** Browser-based interface for configuring WiFi, LoRa parameters, server settings, NTP, and system options

### Reliability Features

- **Network Failover:** Automatic switching between WiFi and Ethernet to maintain connectivity during network issues
- **GPS Time/Location:** L80-M39 GPS module provides accurate time synchronization and gateway positioning
- **RTC Backup:** DS1307 RTC maintains accurate time during GPS signal loss or power cycles

### Management Features

- **OTA Updates:** Over-the-air firmware updates via web interface for easy maintenance
- **REST API:** Programmatic access to gateway status and configuration
- **Multi-Display Support:** LCD 16x2 and OLED display options for local status monitoring
- **SD Card Logging:** Optional storage for packet logs and configuration backup
