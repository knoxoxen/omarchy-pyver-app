// The installed Python versions, how to switch the default between them, and
// what to tell the user when a version is missing or a switch fails.
#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <algorithm>

namespace {
bool versionSegmentLess(const QString &a, const QString &b) {
    const bool aNumber = !a.isEmpty() && a[0].isDigit();
    const bool bNumber = !b.isEmpty() && b[0].isDigit();
    if (aNumber && bNumber) {
        const int x = a.toInt(), y = b.toInt();
        if (x != y)
            return x < y;
        return a.length() < b.length();
    }
    return a < b;
}

// Versions compare numerically per segment, so 3.9 sorts before 3.14.
bool versionLess(const QString &a, const QString &b) {
    const QStringList as = a.split('.'), bs = b.split('.');
    const int shared = qMin(as.size(), bs.size());
    for (int i = 0; i < shared; ++i) {
        if (versionSegmentLess(as[i], bs[i]))
            return true;
        if (versionSegmentLess(bs[i], as[i]))
            return false;
    }
    return as.size() < bs.size();
}
} // namespace

Backend::Backend(QObject *parent) : QObject(parent) { setUpDefaultRoots(); refresh(); }

Backend::Backend(const QStringList &roots, QObject *parent, const QString &miseCommand, const QString &pyenvCommand,
                 const QString &asdfCommand, const QString &alternativesCommand)
    : QObject(parent),
      m_miseCommand(miseCommand),
      m_pyenvCommand(pyenvCommand),
      m_asdfCommand(asdfCommand),
      m_alternativesCommand(alternativesCommand) {
    m_roots = roots;
    for (const auto &root : roots)
        m_rootSources << sourceForPath(root);
    refresh();
}

Backend::~Backend() = default;

void Backend::setUpDefaultRoots() {
    m_roots = defaultRootPaths();
    m_rootSources = {sourceForPath("/usr/bin"), sourceForPath("/usr/local/bin"), sourceForPath("/bin"),
                     sourceForPath(QDir::homePath() + "/.pyenv/versions"),
                     sourceForPath(QDir::homePath() + "/.local/share/mise/installs"),
                     sourceForPath(QDir::homePath() + "/.local/share/asdf/installs")};
}

QString Backend::sourceForPath(const QString &path) {
    if (path.contains(".pyenv"))
        return "pyenv";
    if (path.contains(".local/share/mise"))
        return "mise";
    if (path.contains(".local/share/asdf"))
        return "asdf";
    return "system";
}

QStringList Backend::defaultRootPaths() {
    const QString home = QDir::homePath();
    return {"/usr/bin", "/usr/local/bin", "/bin", home + "/.pyenv/versions",
            home + "/.local/share/mise/installs", home + "/.local/share/asdf/installs"};
}

bool Backend::hasVersion(const QString &version) const { return m_installs.contains(version); }

QString Backend::installSource(const QString &version) const {
    const auto found = m_installs.find(version);
    return found == m_installs.end() ? QString() : found.value().source;
}

QString Backend::installPath(const QString &version) const {
    const auto found = m_installs.find(version);
    return found == m_installs.end() ? QString() : found.value().path;
}

void Backend::refresh() { scan(); }

QVariantList Backend::versionEntries() const {
    QVariantList entries;
    for (const auto &version : m_versions) {
        const Backend::Install install = m_installs.value(version);
        QVariantMap entry;
        entry["version"] = version;
        entry["source"] = install.source;
        entry["path"] = install.path;
        entry["current"] = version == m_defaultVersion;
        entries << entry;
    }
    return entries;
}

void Backend::scan() {
    m_scanning = true;
    emit scanningChanged();

    m_versions.clear();
    m_installs.clear();
    for (int i = 0; i < (int)m_roots.size(); ++i) {
        const QString &source = m_rootSources.value(i);
        const QString &path = m_roots.value(i);
        QDir dir(path);
        if (!dir.exists())
            continue;
        const QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
        fprintf(stderr, "DEBUG root %s source %s exists %d entries %d\n", qPrintable(path), qPrintable(source), dir.exists(), entries.size());
        if (source == "system") {
            for (const auto &entry : entries) {
                const QString &name = entry.fileName();
                if (!name.startsWith("python3"))
                    continue;
                const QString version = name.mid(6);
                if (version.isEmpty() || version == "3")
                    continue;
                if (m_installs.contains(version))
                    continue;
                Backend::Install install;
                install.version = version;
                install.source = "system";
                install.path = entry.filePath();
                m_installs.insert(version, install);
                m_versions << version;
            }
        } else if (source == "pyenv") {
            for (const auto &entry : entries) {
                if (!entry.isDir())
                    continue;
                const QString version = entry.fileName();
                if (m_installs.contains(version))
                    continue;
                Backend::Install install;
                install.version = version;
                install.source = "pyenv";
                install.path = entry.filePath() + "/bin/python" + version;
                m_installs.insert(version, install);
                m_versions << version;
            }
        } else if (source == "mise") {
            for (const auto &entry : entries) {
                if (!entry.isDir())
                    continue;
                const QString tool = entry.fileName();
                if (!tool.startsWith("python@"))
                    continue;
                const QString version = tool.mid(8);
                if (version.isEmpty() || m_installs.contains(version))
                    continue;
                Backend::Install install;
                install.version = version;
                install.source = "mise";
                install.path = entry.filePath() + "/" + version + "/bin/python" + version;
                m_installs.insert(version, install);
                m_versions << version;
            }
        } else if (source == "asdf") {
            for (const auto &plugin : entries) {
                if (!plugin.isDir())
                    continue;
                QDir pluginDir(plugin.filePath());
                const QFileInfoList versions = pluginDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
                for (const auto &version : versions) {
                    if (!version.isDir())
                        continue;
                    const QString binPath = version.filePath() + "/bin/python";
                    if (!QFile::exists(binPath))
                        continue;
                    if (m_installs.contains(version.fileName()))
                        continue;
                    Backend::Install install;
                    install.version = version.fileName();
                    install.source = "asdf";
                    install.path = binPath;
                    m_installs.insert(version.fileName(), install);
                    m_versions << version.fileName();
                }
            }
        }
    }

    std::sort(m_versions.begin(), m_versions.end(), [](const QString &a, const QString &b) { return versionLess(a, b); });

    m_scanning = false;
    emit scanningChanged();
    emit versionsChanged();
    refreshDefault();
}

void Backend::refreshDefault() {
    // Version managers hold the default as a global or an alias, which is where a
    // switch writes, so ask each one before falling back to the system symlink.
    QString managerVersion;
    if (!m_miseCommand.isEmpty()) {
        QProcess process;
        process.setProgram(m_miseCommand);
        process.setArguments({"current", "python"});
        process.start();
        process.waitForFinished(-1);
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            const QString firstLine = QString::fromUtf8(process.readAllStandardOutput()).split('\n').first().trimmed();
            if (!firstLine.isEmpty() && firstLine[0].isDigit())
                managerVersion = firstLine;
        }
    }
    if (managerVersion.isEmpty() && !m_pyenvCommand.isEmpty()) {
        QProcess process;
        process.setProgram(m_pyenvCommand);
        process.setArguments({"version-name"});
        process.start();
        process.waitForFinished(-1);
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
            managerVersion = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }
    if (managerVersion.isEmpty() && !m_asdfCommand.isEmpty()) {
        QProcess process;
        process.setProgram(m_asdfCommand);
        process.setArguments({"current", "python"});
        process.start();
        process.waitForFinished(-1);
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            const QString firstLine = QString::fromUtf8(process.readAllStandardOutput()).split('\n').first().trimmed();
            if (!firstLine.isEmpty() && firstLine[0].isDigit())
                managerVersion = firstLine;
        }
    }

    if (managerVersion.isEmpty()) {
        // No manager claimed the default, so read the system interpreter the
        // python3 and python names point at.
        for (int i = 0; i < (int)m_roots.size(); ++i) {
            if (m_rootSources.value(i) != "system")
                continue;
            for (const QString &name : QStringList{"python3", "python"}) {
                const QFileInfo info(m_roots.value(i) + "/" + name);
                if (!info.isSymLink())
                    continue;
                const QString target = info.readSymLink();
                const QString version = QFileInfo(target).fileName();
                if (!version.startsWith("python3"))
                    continue;
                const QString key = version.mid(6);
                if (!key.isEmpty() && m_installs.contains(key)) {
                    managerVersion = key;
                    break;
                }
            }
            if (managerVersion.isEmpty())
                continue;
            break;
        }
    }

    QString defaultVersion;
    if (!managerVersion.isEmpty()) {
        const auto found = m_installs.find(managerVersion);
        if (found != m_installs.end())
            defaultVersion = managerVersion;
    }
    if (defaultVersion.isEmpty())
        defaultVersion = m_defaultVersion;

    if (defaultVersion != m_defaultVersion) {
        m_defaultVersion = defaultVersion;
        emit defaultVersionChanged();
    }
}

bool Backend::useVersion(const QString &version) {
    const auto found = m_installs.find(version);
    if (found == m_installs.end()) {
        m_statusMessage = tr("Python %1 is not installed.").arg(version);
        emit statusMessageChanged();
        return false;
    }
    const Backend::Install install = found.value();

    QString program;
    QStringList arguments;
    if (install.source == "pyenv") {
        program = m_pyenvCommand;
        arguments = {"global", version};
    } else if (install.source == "mise") {
        program = m_miseCommand;
        arguments = {"use", "-g", "python@" + version};
    } else if (install.source == "asdf") {
        program = m_asdfCommand;
        arguments = {"global", "python", version};
    } else if (install.source == "system") {
        program = m_alternativesCommand;
        arguments = {"--set", "python3", install.path};
    }

    if (program.isEmpty()) {
        m_statusMessage = tr("No installer knows how to switch to Python %1.").arg(version);
        emit statusMessageChanged();
        return false;
    }

    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();
    if (!process.waitForFinished(-1)) {
        process.kill();
        process.waitForFinished(-1);
        m_statusMessage = tr("Switching to Python %1 did not finish.").arg(version);
        emit statusMessageChanged();
        return false;
    }
    const bool ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (ok) {
        m_defaultVersion = version;
        emit defaultVersionChanged();
        emit versionsChanged();
        m_statusMessage = tr("Now using Python %1.").arg(version);
        emit statusMessageChanged();
        return true;
    }
    m_statusMessage = tr("Could not switch to Python %1.").arg(version);
    emit statusMessageChanged();
    return false;
}
