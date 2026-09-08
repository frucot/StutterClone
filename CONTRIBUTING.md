# Contributing

Thanks for helping improve StutterClone.

## Ways to contribute

- Bug reports and feature ideas via [GitHub Issues](https://github.com/frucot/StutterClone/issues)
- Pull requests against `main`
- Documentation fixes

Please read the [developer guide](docs/DEVELOPER.md) ([français](docs/DEVELOPER.fr.md)) before changing audio code.

## Pull requests

1. Fork and branch from `main`.
2. Keep changes focused. Do not mix refactors with behaviour changes.
3. Follow the real-time rules: no allocation, locks, or I/O in `processBlock`.
4. Build at least the Standalone target locally.
5. Update `CHANGELOG.md` under **Unreleased** (or the next version) if the change is user-visible.
6. Use the pull request template and describe how you tested (DAW / OS / format).

## Issues

Use the issue templates. Include OS, DAW, plugin format (VST3 / AU / Standalone), and steps to reproduce.

## License

By contributing you agree that your work is licensed under the [AGPL-3.0](LICENSE), the same license as the rest of the project.

## Code of conduct

See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
