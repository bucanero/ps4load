# PS4Load

PS4Load is based on the [PSL1GHT](https://github.com/ps3dev/PSL1GHT/) PS3load sample,
now built using the [Open Orbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/) SDK.

I hope this version helps you build PS4 homebrews easily!

## Features

- You can load SELF files using the network.

![PS4Load](https://user-images.githubusercontent.com/1153055/167851079-209e3ea3-84b3-4485-88ce-50b77294cf67.jpg)

## Usage

Install the package on your PS4. Open the PS4Load host app, and then use the `ps3load` client to send your `eboot.bin`:

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
