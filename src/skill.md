---
name: pyver
description: |
  Switch the default Python version and refresh the list of installed Python
  versions. Use for ANY request to change which Python version runs as the
  default, or to find out which Python versions are installed.
---

# pyver

One window, one job: which installed Python version should be the default.

Run `pyver list` to see the installed versions, `pyver use <version>` to switch
the default, and `pyver refresh` to rescan. Every command that changes state
takes no display, needs no arguments beyond its options, exits 0 on success and
1 on failure, and writes errors to stderr; `pyver list --json` prints the
versions as JSON.

## Workflow

1. `pyver list` names the installed versions and the current default.
2. `pyver use <version>` switches the default to the chosen version, and
   `pyver refresh` rescans after a new version shows up.

## Working with the person

- The window (`pyver open`) shows every installed version, marks the default,
  and refreshes itself on launch and on demand.
- A version the user just installed (through a version manager or a package) is
  only known once `pyver refresh` runs, so suggest refreshing before switching.
- Do not work around a failed switch by quietly picking another version; report
  the error the tool prints.
