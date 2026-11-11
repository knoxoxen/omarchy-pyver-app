# pyver

One window, one job: which installed Python version should be default.

`pyver` lists the Python versions known to be installed on this machine, lets
you switch the default between them, and refreshes the list when a new version
shows up.

## What it does

- Scans the system interpreter directories and the version manager roots
  (pyenv, mise, asdf) for installed Python versions.
- Marks the default and lets you switch to any installed version.
- Refreshes the list on launch and on demand.

## Shortcuts

| Keys              | Action                    |
| ----------------- | ------------------------- |
| `?`               | Show or hide the help     |
| `Space`           | Use the selected version  |
| `Up` / `Down`    | Select previous / next    |
| `Q`               | Quit                      |

## Install

```sh
./bin/install
```

This builds the binary with `qmake6 + make` and installs it via `makepkg -fsi`,
placing the app in the Omarchy app launcher (`Super + Space`).

## CLI

Commands that need no display, for scripts and agents; every one exits 0 or 1
with errors on stderr, and takes `--json` when a machine reads the result:

```
pyver list              # List the installed Python versions
pyver list --json       # ["versions" with version/source/path, "default"]
pyver use <version>     # Switch the default Python to a version
pyver refresh           # Rescan for installed Python versions
pyver current           # Print the default Python version
pyver skill install     # Install the agent skill
```

## Omarchy Conventions

### Theme Live

Reads `accent` from
`~/.local/state/omarchy/current/theme/colors.toml` and watches it; a theme
switch replaces files and symlinks, so re-arm the watcher on every change.
Fallback to `#FFD60A` when Omarchy isn't there.

### Language Support

Follows the language picked under Omarchy's Setup > Language, live, like the
theme. Every visible string uses `qsTr("Save")` in QML or `tr("Save")` in C++,
with `%1` placeholders filled by `.arg()` rather than sentences glued together,
and a counted string, `qsTr("%n clips", "", count)`, maps to an object of plural
forms keyed by CLDR category ({"one": "%n klip", "other": "%n klip"}).

Ship translations as `src/i18n/<code>.json` (English text to translation) compiled
into the binary. The `Translator` reads `~/.config/omarchy/language`, falls back
from `pt-BR` to `pt` to English per string, and sets the default `QLocale` and
layout direction from Omarchy's registry: write numbers and dates through
`Qt.locale()` (`value.toLocaleString(Qt.locale(), 'f', 1)`) so they take
the language's separators and digits, and keep the window's content under the
template's mirrored root so Arabic, Hebrew, Persian, and Urdu read right to
left. A binding that formats a date or number should read `translator.language`
so it re-renders on a switch. Translate the `.desktop` file's `Name` and
`Comment` too (`Name[da]=…`). Keep brand names, units, and keyboard keys as
they are.

### Corners Match the Windows

Hyprland's `decoration:rounding` is 0 on Omarchy's default, so corners are
sharp unless the theme or the user rounds windows. The template `Theme` asks
`hyprctl` for the effective value and exposes it as `theme.radius`; use it for
every panel, dialog, and button, set `Material.roundedScale: theme.radius` on the
window and on Material controls in other files, and cap small elements with
`Math.min(4, theme.radius)`. Only true circles keep a fixed radius.

### Place Things Where the Pointer Is

Anything the user inserts lands under the mouse when it's over the content (a
`HoverHandler` tracks it), and in the middle of what's visible otherwise. Never
at a fixed corner.

### Keyboard First

`?` shows every shortcut in an overlay, `Q` quits (and asks first when there's
unsaved or unexported work), `Space` is the main action, Ctrl+Z / Ctrl+Shift+Z
undo and redo. The README lists the shortcuts in a table.

### Careful with Data

Never modify the user's originals. Save atomically (write beside the target,
then rename). Ask before discarding work.

### Say What's Wrong

A missing tool, device, or permission is reported in the window, never worked
around by quietly doing less.

### Agents Can Drive It

If the app makes or changes files, give it commands that need no display, as
Hype does (`hype check`, `hype render`, `hype export`): each takes `--json`,
exits 0 or 1 with errors on stderr, and `<name> skill install` installs a short
agent skill bundled in the binary. Set `QT_QPA_PLATFORM=offscreen` before
`QGuiApplication` for those commands, and keep the GUI behind `<name> open`.

## Development Loop

1. `qmake6 && make`, then run `./pyver` to try it. Pass a file argument if
   the app opens files, so it's quick to test.

2. `make test` after each change to the backend. Tests compile `src/*.cpp`
   straight in, run offscreen, and pass temp directories to anything that
   reads the disk (the `Theme` takes the theme directory as an argument).

3. Look at it. Take a screenshot with `omarchy capture screenshot fullscreen
   save` and check the window against the theme before calling it done.
   Switch the language once (`omarchy language da`) and look again: longer
   words must not clip, and nothing should still be in English.

4. Drive the real QML offscreen when a change is about interaction. A small
   harness in the scratchpad that loads `qrc:/Main.qml` with
   `QT_QPA_PLATFORM=offscreen`, sends mouse and key events, and saves
   `window->grabWindow()` shows what the user would see without touching
   their desktop. Set `QT_FORCE_STDERR_LOGGING=1`, or QML warnings go to the
   journal instead of the terminal.

5. `./bin/install` builds the package with `makepkg -fsi`, which installs the
   binary, the `.desktop` entry, and the icon. The app then appears in the
   Omarchy app launcher (`Super + Space`). Run it again after every change the
   user should see there.

## Qt Quick Traps (don't do these!)

- Don't name a component file after a Qt Quick type: `ItemView.qml` fails with
  "abstract base class". Prefix it with what it shows (`PlacedItem.qml`).
- A `MouseArea` inside a `ListView` or `Flickable` loses its drag after a few
  pixels unless it sets `preventStealing: true`.
- Every file that sets a `Material.*` attached property must import
  `QtQuick.Controls.Material`, or the file fails to load.
- `font.pixelSize` is an int. Standard keys go in `sequences:
  [StandardKey.Undo]`, not `sequence:`.
- Text that must look the same on screen and in an exported file is measured
  and painted by one C++ function, used by a `QQuickPaintedItem` on screen and
  by the exporter, rather than laid out twice.

## Build & Install

```sh
cd pyver
qmake6
make              # builds ./pyver
./bin/install     # builds package with makepkg -fsi
```

## Version

0.1.0