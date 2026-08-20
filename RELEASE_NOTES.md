# Release Notes v2.0.2

- Builds and bundles the localproxy binary as part of the gdk build, and
  publishes per-architecture component bundles for x86_64, aarch64, and armv7l
  instead of a single combined archive.

- Adds a recipe manifest for 32-bit arm userspace running on a 64-bit kernel,
  where Debian 13 reports `architecture.detail` as `aarch64`.

- Hardens the localproxy launch path: handles `fork()` failure, closes a
  process-group race window, refuses to follow symlinks when opening the binary,
  and validates numeric CLI arguments.

# Release Notes v2.0.1

- Updates the bundled localproxy binary to v3.3.0, which fixes concurrency bugs
  that could corrupt data across simultaneous tunnel connections.

# Release Notes v2.0.0

- Replaces the Java wrapper with a C wrapper and replaces AWS IoT Device Client
  with
  [AWS IoT Securetunneling Localproxy](https://github.com/aws-samples/aws-iot-securetunneling-localproxy).

- Reduces resource usage through a smaller binary size and lower memory
  footprint.
