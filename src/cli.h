#pragma once

#include <QStringList>

// Headless commands for scripts and agents: pyver list, use, refresh, current,
// and so on. Bare pyver, and any option, are commands too, so they run offscreen.
bool isCliCommand(const QString &word);
QString cliSummary();
int runCli(const QStringList &arguments);
