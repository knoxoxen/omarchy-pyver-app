# pyver

One window, one job: which installed Python version should be the default.

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
| `?`               | Show or hide the help    |
| `Space`           | Use the selected version |
| `Up` / `Down`    | Select previous / next   |
| `Q`               | Quit                      |

## Install

```sh
./bin/install
```

## CLI

Commands that need no display, for scripts and agents; every one exits 0 or 1
with errors on stderr, and takes `--json` where a machine reads the result:

```sh
pyver list              # List the installed Python versions
pyver list --json      # ["versions" with version/source/path, "default"]
pyver use <version>    # Switch the default Python to a version
pyver refresh           # Rescan for installed Python versions
pyver current           # Print the default Python version
pyver skill install    # Install the agent skill
```
