# DotOS with MeshCore 1.18

This fork replaces the Meshtastic radio implementation with the official
MeshCore `companion-v1.18.0` core and companion protocol, pinned to commit
`81dbae592ffad40b4212bc0b37e82b38da08f4bc`.

## Build and installation

Use the `codex/meshcore-1.18` branch, not upstream's binary-only `main` branch.

```sh
python3 -m pip install platformio
pio run -e twatch_ultra
```

The output is `.pio/build/twatch_ultra/firmware.bin`. This is the application
image, not a merged factory image. Keep the existing 16 MB DotOS partition
layout. Use `pio run -e twatch_ultra -t upload` for a full source build/upload;
do not use upstream's browser flasher to install this fork because it contains
the upstream Meshtastic build. No firmware has been flashed or tested on a
physical watch during development of this fork.

## Watch operation

- Enable LoRa in the watch's LoRa screen. Messages and contact screens now use
  MeshCore. Channels use MeshCore encryption, not Meshtastic keys or framing.
- Initial US/Pacific Northwest radio preset: 910.525 MHz, BW 62.5 kHz, SF7,
  CR4/5, TX20 dBm. Match your local mesh using Radio Settings in the MeshCore
  phone app; settings persist. Other regional radios need an appropriate
  frequency/profile before use. The build is for the SX1262 T-Watch Ultra.
- Tap a channel message to reply on that channel. Private messages select a
  known contact; full 32-byte public keys address transmissions. Ambiguous
  four-byte display IDs are refused instead of sending to the wrong contact.
- The watch GPS Broadcast Location switch sends MeshCore location adverts at
  its selected interval. Turning it off clears the watch advert location.
  Repeating is managed by the MeshCore phone app and its upstream policies.
- Standalone watch text is limited to 130 UTF-8 bytes. A send succeeds when
  queued; private delivery is confirmed only when an ACK is received.
- Channel names and keys are managed by the MeshCore phone app and are shown
  on the watch's Channels screen. Tap a nonempty channel to select watch TX.
- The stock Meshtastic position-request and traceroute packets are removed.
  Use the MeshCore phone app's diagnostics. Contact positions from adverts
  remain available to the watch map. Meshtastic-specific telemetry fields
  are not fabricated in the watch UI.

## Pair a phone

1. Enable LoRa, turn WiFi and BLE scanners/detectors off.
2. Open the MeshCore screen and **tap its title** to open Phone Sync.
3. Tap **Enable / disable sync**. This switches off notification mirroring.
4. In the official MeshCore Android/iOS app, connect to
   `MeshCore-<watch name>`. Enter the six-digit PIN shown on the watch.
   A fresh identity gets a random PIN, saved in companion preferences.
5. Contacts, channels, identity/radio settings and incoming messages use the
   standard MeshCore v1.18 companion protocol over the Nordic UART BLE service.
   Messages sent by the phone also appear on the watch.

Phone sync is opt-in and the standalone radio works without Bluetooth. The
companion owns the Bluetooth GAP callbacks once initialized. Disabling sync
stops advertising/disconnects the phone, but **reboot before returning to ANCS,
Gadgetbridge, BLE mouse or BLE scanners**. Notification and scan entry points
refuse to take the companion's callbacks. WiFi/BLE coexistence is disabled in
this build to retain the existing watch's memory/ownership guards.

The watch and phone share the radio identity, contacts and channels. The radio
queues incoming messages while the app is disconnected; the app fetches them
on reconnect. This is the standard bounded **RAM queue** (16 frames), not an
unlimited persistent chat-history sync service. Contacts, keys and preferences
persist in internal FAT independently of the removable SD card. Unread watch
messages are not removed when the phone consumes its queue; clearing watch
messages does not consume the phone's queue. The phone's existing sent-message
history is not downloaded to the watch on first pairing. Watch-originated TX
is not retroactively inserted into the phone's sent-message database.

## Integration and provenance

- `lib/MeshCore/src`: upstream MeshCore core source, unchanged.
- `src/meshcore_companion`: upstream MyMesh/DataStore/NodePrefs plus watch
  target and a small `onLocalMessageSent` listener hook after accepted phone
  text transmissions. No duplicate MeshCore command decoder.
- `lib/ed25519`: upstream MeshCore's bundled signing implementation.
- `lib/LilyGoLib`: its existing global SX1262 is specialized to the official
  `CustomSX1262` under `DOTOS_MESHCORE`. The SPI bus and radio are still shared
  with the watch's radio apps, with one owner at a time.
- `src/meshcore.cpp`: adapter to existing DotOS views and the upstream BLE
  transport. `MyMesh` lives in PSRAM when available.
- `src/meshcore_ui.h`: byte limits and collision-safe contact selection.

Retains MIT notices for DotOS, MeshCore and ed25519. See each vendored library's
license. Old Meshtastic SD channel files are not imported into MeshCore.

## Validation

Build the ESP32-S3 firmware with `pio run -e twatch_ultra`; run the pure host
suite with `make -C test test`. New tests cover 130-byte limits including UTF-8,
full-key selection and refusal of colliding display prefixes.

Hardware acceptance still required:

- Boot and verify display/touch/clock/GPS and LoRa power cycling.
- Pair Android and/or iOS, enter PIN, read self-info/contacts/channels.
- Set channel and radio profile from phone; verify on watch and after reboot.
- Exchange public/channel and encrypted private messages with another v1.18
  companion; verify phone/watch views, ACK and timeout states.
- Receive while phone disconnected; reconnect and fetch pending messages.
- Test disconnect/reconnect, screen-off reception and constrained-memory boot.
- Check radio-mode handoff to/from APRS/FSK and Bluetooth ownership guards.

A compiled image is not evidence of BLE or over-air interoperability on the
physical watch. Keep a known-working upstream image for recovery.
