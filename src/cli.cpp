// Headless commands for scripts and agents: list, use, refresh, current, and
// skill. Each takes --json when a machine reads the result, exits 0 or 1 with
// errors on stderr, and never needs a display.
#include "cli.h"

#include "backend.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cstdio>

namespace {
void print(FILE *stream, const QString &text) { fprintf(stream, "%s\n", qPrintable(text)); }
void print(const QJsonObject &object) { fprintf(stdout, "%s", QJsonDocument(object).toJson(QJsonDocument::Compact).constData()); }
int fail(const QString &message) {
    print(stderr, message);
    return 1;
}

struct Command {
    QCommandLineParser parser;
    explicit Command(const QString &name, const QString &description) {
        parser.setApplicationDescription(description);
        parser.addHelpOption();
        parser.addPositionalArgument(name, description, "<" + name + ">");
    }
    void json() { parser.addOption({"json", "Print the result as JSON"}); }
    QString argument(int index) const { return parser.positionalArguments().value(index); }
};

int list(const QStringList &arguments) {
    Command command("list", "List the installed Python versions.");
    command.json();
    command.parser.process(arguments);
    Backend backend;
    backend.refresh();
    qDebug() << "Backend version count:" << backend.versionEntries().size();
    const auto entries = backend.versionEntries();
    if (command.parser.isSet("json")) {
        QJsonArray versions;
        for (const auto &entry : entries) {
            const QJsonObject object = QJsonObject::fromVariantMap(entry.toMap());
            versions << QJsonObject{{"version", object.value("version")}, {"source", object.value("source")},
                                    {"path", object.value("path")}};
        }
        print({{"versions", versions}, {"default", backend.defaultVersion()}});
        return 0;
    }
    for (const auto &entry : entries) {
        const QJsonObject object = QJsonObject::fromVariantMap(entry.toMap());
        print(stdout, QString("%1  %2  %3").arg(object.value("version").toString()).arg(object.value("source").toString())
                        .arg(object.value("path").toString()));
    }
    print(stdout, QString("%1 versions").arg(entries.size()));
    return 0;
}

int use(const QStringList &arguments) {
    Command command("use", "Switch the default Python to an installed version.");
    command.json();
    command.parser.process(arguments);
    const QString version = command.argument(1);
    if (version.isEmpty())
        return fail("Use: pyver use <version>.");
    Backend backend;
    backend.refresh();
    const bool ok = backend.useVersion(version);
    if (command.parser.isSet("json"))
        print({{"version", version}, {"ok", ok}, {"status", backend.statusMessage()}});
    else
        print(ok ? stdout : stderr, backend.statusMessage());
    return ok ? 0 : 1;
}

int refresh(const QStringList &arguments) {
    Command command("refresh", "Rescan for installed Python versions.");
    command.json();
    command.parser.process(arguments);
    Backend backend;
    backend.refresh();
    const auto entries = backend.versionEntries();
    if (command.parser.isSet("json"))
        print({{"versions", entries.size()}, {"default", backend.defaultVersion()}});
    else
        print(stdout, QString("%1 versions").arg(entries.size()));
    return 0;
}

int current(const QStringList &arguments) {
    Command command("current", "Print the default Python version.");
    command.json();
    command.parser.process(arguments);
    Backend backend;
    backend.refresh();
    if (command.parser.isSet("json"))
        print({{"default", backend.defaultVersion()}});
    else
        print(stdout, backend.defaultVersion().isEmpty() ? "(none)" : backend.defaultVersion());
    return 0;
}

int skill(const QStringList &arguments) {
    Command command("skill", "Print the agent skill, or install it for coding agents with: skill install.");
    command.parser.addPositionalArgument("install", "Copy to ~/.agents/skills/pyver and link into ~/.claude/skills", "[install]");
    command.parser.process(arguments);
    QFile bundled(":/skill.md");
    if (!bundled.open(QIODevice::ReadOnly))
        return 1;
    const QByteArray text = bundled.readAll();
    if (command.argument(1).isEmpty()) {
        fputs(text.constData(), stdout);
        return 0;
    }
    if (command.argument(1) != "install")
        return fail("Use pyver skill to print the skill, or pyver skill install.");
    const QDir home = QDir::home();
    const QString directory = home.filePath(".agents/skills/pyver");
    QSaveFile file(directory + "/SKILL.md");
    if (!QDir().mkpath(directory) || !file.open(QIODevice::WriteOnly) || file.write(text) < 0 || !file.commit())
        return fail("Could not write " + file.fileName());
    print(stdout, "Installed " + file.fileName());
    if (!home.exists(".claude"))
        return 0;
    const QString link = home.filePath(".claude/skills/pyver");
    const QFileInfo existing(link);
    if (existing.isSymLink())
        QFile::remove(link);
    else if (existing.exists())
        return fail(link + " exists and is not a link; leaving it alone.");
    if (!home.mkpath(".claude/skills") || !QFile::link("../../.agents/skills/pyver", link))
        return fail("Could not link " + link);
    print(stdout, "Linked " + link);
    return 0;
}

int help(const QStringList &arguments) {
    const QString topic = arguments.value(2);
    if (isCliCommand(topic) && topic != "help")
        return runCli({arguments[0], topic, "--help"});
    print(stdout, "Usage: pyver <command> [options]\n\n" + cliSummary());
    return 0;
}
} // namespace

QString cliSummary() {
    return "Python versions:\n"
           "  open                          Open the Python versions window\n\n"
           "Commands that need no display (pyver help <command> for options):\n"
           "  list                          List the installed Python versions\n"
           "  use <version>                 Switch the default Python to a version\n"
           "  refresh                       Rescan for installed Python versions\n"
           "  current                       Print the default Python version\n"
           "  skill [install]               Print the skill for coding agents, or install it";
}

bool isCliCommand(const QString &word) {
    return QStringList{"list", "use", "refresh", "current", "skill", "help"}.contains(word);
}

int runCli(const QStringList &arguments) {
    const QString command = arguments.value(1);
    return command == "list"     ? list(arguments)
           : command == "use"    ? use(arguments)
           : command == "refresh" ? refresh(arguments)
           : command == "current" ? current(arguments)
           : command == "skill"  ? skill(arguments)
                                 : help(arguments);
}
