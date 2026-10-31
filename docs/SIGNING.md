# Signing and notarization

Status: **not implemented in this revision.** There is no packaging step in the repository yet.
This page records the intended design so that the packaging work can follow it.

## Planned behaviour

- **macOS.** `cmake/Signing.cmake` will codesign every bundle with the hardened runtime and
  `scripts/installer/macos/entitlements.plist` when `MAC_CERT_P12` is set. Builds are universal
  (`arm64;x86_64`) with deployment target 11.0. Notarization runs `xcrun notarytool submit --wait`
  and `xcrun stapler staple` only when `MAC_CERT_P12`, `MAC_CERT_PASSWORD`, `APPLE_ID`,
  `APPLE_TEAM_ID` and `APPLE_APP_PASSWORD` are all present. Otherwise the build is logged as
  unsigned and continues.
- **Windows.** `signtool` signs the installer and the plugin binaries when `WIN_CERT_PFX` and
  `WIN_CERT_PASSWORD` are set. Otherwise the build is unsigned.
- **Checksums.** Release artifacts ship with `SHA256SUMS`.

## Secrets

Secrets are read from the repository's GitHub Actions secrets. They are never written to the
repository or to logs. `config.json`, `*.p12` and `*.pfx` are ignored by `.gitignore`.
