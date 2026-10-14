<table>
  <tr>
    <td><img src="icons/md5tree.png" alt="MD5Tree logo" width="64" /></td>
    <td><h1>MD5Tree</h1></td>
  </tr>
</table>

[Русский](README.md) | **English**

A cross-platform Qt Widgets application for recursive directory scanning
and multithreaded MD5 hashing.

![Screenshot](docs/screenshot.png)

> **Note:** The application UI is currently in Russian only.

## Features

- Recursive directory traversal
- Multithreaded MD5 hashing (`QThreadPool`, one `QRunnable` per file)
- Live results table: file name, MD5, thread ID
- Start / Stop with cooperative cancellation
- Cross-platform: Windows, Linux, macOS

## Running

**Windows:** download `MD5Tree-0.1.0-win64.zip` from the
[Releases](https://github.com/sun-demon/MD5Tree/releases) page, unpack
and run `MD5Tree.exe`. No Qt installation required.

**Linux/macOS:** build from source (see below).

## Building

```bash
git clone https://github.com/sun-demon/MD5Tree.git
cd MD5Tree
qmake MD5Tree.pro CONFIG+=release
make          # or mingw32-make on Windows
```

Requirements: Qt 5.14.1+ (Widgets), C++17 compiler.

## Usage

1. **Обзор…** (Browse) — select a directory.
2. **Старт** (Start) — begin scanning.
3. **Стоп** (Stop) — cancel. Already processed rows remain.

## License

MIT. See [LICENSE](LICENSE).
