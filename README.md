# Emcee ⛏️

Emcee is a minecraft 1.21.5 server written in C with dependencies only on libuv and cJSON. The inevitable goal of Emcee is to be a highly flexible and friendly way to run custom minecraft servers that can be written in Lua.

# Building

Building Emcee requires `make`, a C99 compiler, `pkg-config`, and `libuv`. To build the test suite, a `C++20` compiler and `gtests` are needed. To build Emcee simply run
```
make
```

To build the tests, run
```
make emcee_tests
```

## ⚠️ Warning ⚠️
Emcee is an exploration of networking and the minecraft protocol, and in its current state doesn't do authentication, compression, and most behavior is hard coded. Furthermore much of the code is not tested thoroughly, so take caution in trusting its full correctness.
