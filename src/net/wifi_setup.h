/**
 * wifi_setup.h — WiFi provisioning for FiberQuest HMI
 *
 * On first boot (no saved creds): shows WiFi setup screen (SSID + password entry).
 * Credentials stored in NVS (Preferences).
 * Once connected: transitions to home screen, starts WebSocket client.
 */
#pragma once

void wifi_setup_begin();
bool wifi_is_connected();
const char* wifi_get_ip();
