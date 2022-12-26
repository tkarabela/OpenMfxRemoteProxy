# OpenMfx Remote Proxy

Work in progress. Proof-of-concept for https://openmfx.org/Rfc/009-dynamic-linking-hell.html via running
plugins in a separate process.

## Compilation

### Ubuntu 22.04

```sh
git submodule update --init --recursive

sudo apt-get install curl zip unzip tar
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install
```
