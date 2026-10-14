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

**Windows:** download `MD5Tree-0.1.0-win64.zip` from the Releases page, unpack
and run `MD5Tree.exe`. No Qt installation required.

- [Releases on GitHub](https://github.com/sun-demon/MD5Tree/releases)
- [Releases on Mos.Hub](https://hub.mos.ru/sun-demon/md5tree/-/releases)

**Linux/macOS:** build from source (see below).

## Building

```bash
# GitHub
git clone https://github.com/sun-demon/MD5Tree.git

# or Mos.Hub
git clone https://hub.mos.ru/sun-demon/md5tree.git

cd MD5Tree
qmake MD5Tree.pro CONFIG+=release
make          # or mingw32-make on Windows
```

Requirements: Qt 5.14.1+ (Widgets), C++17 compiler.

## Usage

1. **Обзор…** (Browse) — select a directory in the folder tree. Files are visible, but only folders can be selected.
2. **Старт** (Start) — begin scanning.
3. **Стоп** (Stop) — cancel. Already processed rows remain.
4. The table displays paths relative to the selected directory, in lexicographic order.

While scanning, the status bar shows an animated busy indicator because the total
number of folders and files is not known in advance. During hashing, the progress
bar shows the percentage of bytes processed; folder/file counts and total size are
available in its tooltip. Files being hashed appear in the table immediately.
Results use a Qt data model instead of separate cell widgets for every row.

## License

MIT. See [LICENSE](LICENSE).
