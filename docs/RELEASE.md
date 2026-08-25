# Releasing the component

The process for cutting a release of the secure tunneling component. CI builds
the per-architecture bundles; tagging and publishing are manual.

## Versioning

The `version` file at the repo root is the single source of truth, and tags are
`vX.Y.Z` to match. It is read by `CMakeLists.txt`, `gdk-build.sh`, and the
`Set version` step in both `build.yml` and `release.yml`, so that one line
drives the binary, the bundles and the recipe.

## Process

### 1. Prepare

- CI is green on `main`.
- Local `main` is up to date.
- The localproxy release tag to bundle is known, and is later than v3.3.0.

### 2. Open the release PR

From a `dev/<short-kebab-name>` branch, updating just the release notes and the
version. Title it "Release vX.Y.Z".

- Add a new section at the top of `RELEASE_NOTES.md`, headed
  `# Release Notes vX.Y.Z`, covering the changes customers should know about.
  Skip commits with no customer impact.
- Bump the `version` file to `X.Y.Z`.
- Merge the PR once it is green.

### 3. Tag the release commit

- Tag with `git tag -a vX.Y.Z` — annotated, not lightweight.
- Title the tag message "vX.Y.Z release" and use the new `RELEASE_NOTES.md`
  section as the description.
- Push the tag.

### 4. Build the bundles

Run the **Release Pipeline** workflow (`release.yml`) against the `vX.Y.Z` tag.
It is manual-only, by design, and takes one input:

- `localproxy_ref` — the release tag of
  [aws-iot-securetunneling-localproxy](https://github.com/aws-samples/aws-iot-securetunneling-localproxy)
  to build and bundle, for example `v3.3.1`. It must be later than v3.3.0, and a
  malformed ref is rejected before any build starts.

It builds the component and localproxy for each architecture and uploads one
artifact per architecture, each holding the `aws-greengrass-secure-tunnel` and
`localproxy` binaries:

- `GreengrassV2SecureTunnelingComponent-x86_64.zip`
- `GreengrassV2SecureTunnelingComponent-aarch64.zip`
- `GreengrassV2SecureTunnelingComponent-armv7l.zip`

Artifacts are retained for **2 days**, so download them promptly or re-run the
workflow.

### 5. Publish

- Create the GitHub release from the tag, reusing the tag message as the title
  and body.
- Attach the three bundles, zipped as
  `GreengrassV2SecureTunnelingComponent-<arch>.zip`.

Nothing in steps 3 to 5 is automated: `release.yml` is dispatch-only and holds
`contents: read`, so it can neither tag nor create a release.

## Notes

- The zip name, every `{artifacts:decompressedPath}/<name>/` reference in
  `recipe.yaml`, and the S3 artifact URI must all agree, because Greengrass
  extracts `<name>.zip` into a directory named `<name>`. `gdk-build.sh` drives
  all three from one architecture value.
