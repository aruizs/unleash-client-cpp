# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.5.4] - 2026-06-22

### Fixed
- Build on MSVC toolchains older than `_MSC_VER` 1950 (VS 2022 v17.50), which do
  not provide `std::regex_constants::multiline`. The `(?m)` inline regex flag is
  ignored on those compilers so the library still builds; it works as before on
  newer MSVC, libstdc++, and libc++.

## [1.5.3] - 2026-06-22

### Added
- Custom activation strategies: subclass `unleash::Strategy` and register a factory with
  `UnleashClientBuilder::registerStrategy()`. A registered name overrides the built-in
  strategy of the same name.

### Fixed
- Use `std::regex_constants::multiline` instead of the `std::regex::multiline` static
  member, which some MSVC toolchains do not provide — the `REGEX` operator failed to
  compile with `'multiline': is not a member of 'std::basic_regex'`.
- Make `Strategy` explicitly non-copyable. Exporting the class (for custom strategies)
  made MSVC emit the implicit copy constructor for a DLL build, which failed to compile
  because `Strategy` owns a `std::vector<std::unique_ptr<Variant>>`.

## [1.5.2] - 2026-06-19

### Added
- Doxygen API documentation for the public headers, published to GitHub Pages
  by a new docs workflow.
- A vcpkg overlay port under `vcpkg/ports/unleash-client-cpp`, validated by a
  new CI job, ready for submission to the upstream vcpkg registry.
- A runnable `quickstart` example under `examples/` (build with
  `-DUNLEASH_BUILD_EXAMPLES=ON`).
- A `-DUNLEASH_SANITIZER=address|thread|undefined` build option, exercised by a
  new sanitizer matrix job in CI.

### Changed
- `ApiClient::metrics()` now has a default implementation, so custom `ApiClient`
  implementations no longer need to override it.

### Fixed
- Link `ws2_32` on Windows so a shared/DLL build resolves `inet_pton` (used by
  the `IN_CIDR` operator) without relying on a consumer to pull it in.
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

[Unreleased]: https://github.com/aruizs/unleash-client-cpp/compare/v1.5.4...HEAD
[1.5.4]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.4
[1.5.3]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.3
[1.5.2]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.2
[1.5.1]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.1
[1.5.0]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.5.0
[1.3.0]: https://github.com/aruizs/unleash-client-cpp/releases/tag/v1.3.0
