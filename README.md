# CS144: Spring 2023

## Lab

- [x] Checkpoint 0: networking warmup
- [x] Checkpoint 1: stitching substrings into a byte stream
- [x] Checkpoint 2: the TCP receiver
- [x] Checkpoint 3: the TCP sender
- [x] Checkpoint 4: down the stack
- [x] Checkpoint 5: building an IP router
- [ ] Checkpoint 6: putting it all together

## Note

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`
