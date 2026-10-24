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

## Releases

Pushing a tag beginning with `v` runs the release workflow. For example:

```sh
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

The workflow runs the test suite with GCC, Clang, AddressSanitizer and UBSan.
Only after every test job succeeds does it build `libhashmap.a` and
`libhashmap.so`, package them with the public header, and publish a GitHub
Release containing the Linux x86-64 archive and its SHA-256 checksum.

## License

No license has been chosen yet.
