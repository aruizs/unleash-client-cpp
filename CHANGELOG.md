# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- A runnable `quickstart` example under `examples/` (build with
  `-DUNLEASH_BUILD_EXAMPLES=ON`).
- A `-DUNLEASH_SANITIZER=address|thread|undefined` build option, exercised by a
  new sanitizer matrix job in CI.

### Changed
- `ApiClient::metrics()` now has a default implementation, so custom `ApiClient`
  implementations no longer need to override it.

### Fixed
- Removed a data race between the background polling thread and client
  destruction by making the stop flag atomic.

## [1.5.1] - 2026-06-19

### Added
- `UnleashClientBuilder::build()` — explicit, `auto`-friendly construction path
  (the implicit conversion to `UnleashClient` is still supported).
- `isEnabled(flag, defaultValue)` and `isEnabled(flag, context, defaultValue)`
  overloads. The default is returned only for unknown flags; a flag that exists
  is always evaluated on its own merits, matching the official Unleash SDKs.

### Fixed
- Restored source compatibility with 1.3.0 that was inadvertently broken in
  1.5.0:
  - `variant_t::feature_enabled` had been renamed to `featureEnabled`.
  - The builder-to-client conversion had been made `explicit`, forcing
    `static_cast`.
  - `Context::currentTime` had been inserted in the middle of the struct,
    breaking positional aggregate initialization; it is now appended at the end.
  - `Context::properties` and `featuresMap_t` had switched to a transparent
    comparator, changing their public types.
- Made the background polling flag atomic to remove a data race between the
  refresh thread and client destruction.

### Compatibility
- Source-compatible with 1.3.0: existing code recompiles unchanged.

## [1.5.0] - 2026-06-19 — superseded by 1.5.1

Introduced the features below but shipped with inadvertent source-breaking
changes (see 1.5.1). Use 1.5.1 instead.

### Added
- Constraint operators: string (`STR_STARTS_WITH`/`STR_ENDS_WITH`/`STR_CONTAINS`),
  numeric (`NUM_*`), date (`DATE_*`), SemVer (`SEMVER_*`), `REGEX`, and `IN_CIDR`
  (IPv4 + IPv6).
- Segments (global constraints).
- Strategy variants.
- Dependent features.
- UTF-8 feature flag names.
- Delta API support (hydration snapshot and incremental event stream).
- Opt-in usage metrics reporting to `/client/metrics`.

The client passes the full
[Unleash Client Specification](https://github.com/Unleash/client-specification)
test suite as of this release.

## [1.3.0] - 2023-12-27

Baseline published on Conan Center. Feature toggles, built-in strategies,
Unleash context, strategy constraints, application registration, variants,
custom stickiness, and bootstrapping from a local cache file.

[Unreleased]: https://github.com/aruizs/unleash-client-cpp/compare/v1.5.1...HEAD
[1.5.1]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.1
[1.5.0]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.0
[1.3.0]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.3.0
