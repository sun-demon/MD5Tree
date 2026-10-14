<table>
  <tr>
    <td><img src="icons/md5tree.png" alt="MD5Tree logo" width="64" /></td>
    <td><h1>MD5Tree</h1></td>
  </tr>
</table>

**Русский** | [English](README.en.md)

Кроссплатформенное приложение на Qt Widgets для рекурсивного обхода
директорий и многопоточного вычисления MD5.

![Скриншот](docs/screenshot.png)

## Возможности

- Рекурсивный обход директорий и поддиректорий
- Многопоточное вычисление MD5 (`QThreadPool`, по одному `QRunnable` на файл)
- Таблица результатов: имя файла, MD5, ID потока
- Кнопки «Старт» / «Стоп» с кооперативной отменой
- Кроссплатформенность: Windows, Linux, macOS

## Запуск

**Windows:** скачайте `MD5Tree-0.1.0-win64.zip` со страницы
[Releases](https://github.com/sun-demon/MD5Tree/releases), распакуйте
и запустите `MD5Tree.exe`. Qt не требуется — всё включено.

**Linux/macOS:** соберите из исходников (см. ниже).

## Сборка

```bash
git clone https://github.com/sun-demon/MD5Tree.git
cd MD5Tree
qmake MD5Tree.pro CONFIG+=release
make          # или mingw32-make на Windows
```

Требования: Qt 5.14.1+ (Widgets), компилятор с C++17.

## Использование

1. **Обзор…** — выбрать директорию.
2. **Старт** — начать сканирование.
3. **Стоп** — отменить. Уже обработанные строки остаются.

## Лицензия

MIT. См. [LICENSE](LICENSE).
