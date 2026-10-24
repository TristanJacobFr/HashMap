# HashMap

A small hash map library written in C, inspired by Java's `HashMap`.

This is a personal project made for learning and experimentation. It is still a
work in progress, so the API may change and the library is not suitable for
production use.

## Status

The project is currently in its early stages. The hash map implementation and
its public API are still being designed.

## Build

```sh
make
```

## Tests

Run the complete test suite with:

```sh
make test
```

Run it with AddressSanitizer and UndefinedBehaviorSanitizer with:

```sh
make test-sanitize
```

The runner supports `--list`, `--filter <text>` and `--no-color`. It also
honours the standard `NO_COLOR` environment variable. For example:

```sh
./tests/hashmap_tests --filter collision
NO_COLOR=1 make test
```

The same test suite is run with GCC and Clang by GitHub Actions on every push
and pull request.

## Tagged builds

Pushing a tag beginning with `v` runs the release build workflow. For example:

```sh
git tag v1.0.0
git push origin v1.0.0
```

The workflow runs the tests, builds `libhashmap.a` and `libhashmap.so`, and
uploads a compressed Linux x86-64 package with its SHA-256 checksum as a
GitHub Actions artifact.

## License

No license has been chosen yet.
