# Release Notes v2.0.1

- Updates the bundled localproxy binary to v3.3.0, which fixes concurrency bugs
  that could corrupt data across simultaneous tunnel connections.

# Release Notes v2.0.0

- Replaces the Java wrapper with a C wrapper and replaces AWS IoT Device Client
  with
  [AWS IoT Securetunneling Localproxy](https://github.com/aws-samples/aws-iot-securetunneling-localproxy).

- Reduces resource usage through a smaller binary size and lower memory
  footprint.
