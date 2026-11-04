# DotOS — MeshCore 1.18 fork

MeshCore companion support for the LILYGO T-Watch Ultra (SX1262), replacing
Meshtastic while retaining the DotOS watch interface.

- Official MeshCore companion v1.18.0 core and BLE protocol, pinned by commit.
- Standalone watch channel messages, encrypted DMs, contacts and advert positions.
- MeshCore Android/iOS companion connection, shared settings/contacts/channels,
  incoming-message queue, and phone-sent messages on the watch.
- Pairing PIN and phone-sync controls: tap the **MeshCore screen title**.

Install in desktop Chrome/Edge with the [web flasher](https://corporalmesh.github.io/DotOS-T-Watch-Ultra-Firmware/).
An [offline HTML installer](https://github.com/CorporalMesh/DotOS-T-Watch-Ultra-Firmware/releases/download/meshcore-v1.18.0-dotos-alpha.1/dotos-meshcore-flasher.html) is also attached to the release.

See [MeshCore build, pairing, limitations and hardware checklist](docs/MESHCORE.md).
This fork is experimental and requires physical-watch validation. Use source
builds from this branch; upstream's flasher still contains Meshtastic firmware.

```sh
pio run -e twatch_ultra
```

Bluetooth companion mode is exclusive with notification mirroring and BLE
scanners. Reboot to release its Bluetooth ownership. Chat queue sync uses the
standard bounded RAM queue; it does not replicate unlimited phone chat history.

Based on [Holrigue/DotOS](https://github.com/Holrigue/DotOS-T-Watch-Ultra-Firmware/tree/develop),
[ARGUS](https://github.com/h4d35x0/argus), and [13:37](https://github.com/r3dfish/13-37).
Mesh protocol by [MeshCore](https://github.com/meshcore-dev/MeshCore).
Original project details and credits: [upstream README](docs/UPSTREAM_README.md).
MIT; see [LICENSE](LICENSE) and the vendored library licenses.
