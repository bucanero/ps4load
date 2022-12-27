# PS4Load

[![Downloads][img_downloads]][app_downloads] [![Release][img_latest]][app_latest] [![License][img_license]][app_license]
[![Build app package](https://github.com/bucanero/ps4load/actions/workflows/build.yml/badge.svg)](https://github.com/bucanero/ps4load/actions/workflows/build.yml)

PS4Load is based on the [PSL1GHT](https://github.com/ps3dev/PSL1GHT/) PS3load sample,
now built using the [Open Orbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/) SDK.

I hope this version helps you build PS4 homebrews easily!

## Features

- You can load SELF files using the network.
- You can upload and extract ZIP files to `/data/` over the network.

![PS4Load](https://user-images.githubusercontent.com/1153055/167851079-209e3ea3-84b3-4485-88ce-50b77294cf67.jpg)

## Usage

Install the package on your PS4. Open the PS4Load host app, and then use the [ps3load client](https://github.com/bucanero/ps4load/tree/main/client) to send your `eboot.bin` or `.zip` file:

### Linux/macOS

```bash
export PS3LOAD=tcp:192.168.x.x
./ps3load /path/to/eboot.bin
```

### Windows

```sh
set PS3LOAD=tcp:192.168.x.x
ps3load.exe \path\to\eboot.bin
```

## Building

You need to have installed:

- [Open Orbis SDK](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/)
- [Zlib](https://github.com/bucanero/oosdk_libraries/tree/master/zlib_partial) library
- [Zip](https://github.com/bucanero/zip) library
- [SDL2](https://github.com/PacBrew/SDL/tree/ps4) library
- [dbglogger](https://github.com/bucanero/dbglogger) library

Run `make` to create a release build.

You can also set the `PS3LOAD` environment variable to the PS4 IP address: `export PS3LOAD=tcp:x.x.x.x`.
This will allow you to use a `ps3load` client and send `eboot.bin` directly to the [PS4Load host](https://github.com/bucanero/ps4load).

PS4Load will also send debug messages to UDP multicast address `239.255.0.100:30000`.
To receive them you can use [socat][] on your computer:

    $ socat udp4-recv:30000,ip-add-membership=239.255.0.100:0.0.0.0 -

## License

[PS4Load](https://github.com/bucanero/ps4load/) - Copyright (C) 2022  [Damian Parrino](https://twitter.com/dparrino)

This program is free software: you can redistribute it and/or modify
it under the terms of the [GNU General Public License](LICENSE) as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

[socat]: http://www.dest-unreach.org/socat/
[app_downloads]: https://github.com/bucanero/ps4load/releases
[app_latest]: https://github.com/bucanero/ps4load/releases/latest
[app_license]: https://github.com/bucanero/ps4load/blob/master/LICENSE
[img_downloads]: https://img.shields.io/github/downloads/bucanero/ps4load/total.svg?maxAge=3600
[img_latest]: https://img.shields.io/github/release/bucanero/ps4load.svg?maxAge=3600
[img_license]: https://img.shields.io/github/license/bucanero/ps4load.svg?maxAge=2592000
