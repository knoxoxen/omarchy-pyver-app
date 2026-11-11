#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantList>

// Owns the installed Python versions, which of them the user can switch between,
// and how to switch: through a version manager (mise, pyenv, asdf) or through
// the system alternatives. Exposes the known versions to QML as a list of
// entries, and the actions as invokables.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString defaultVersion READ defaultVersion NOTIFY defaultVersionChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    Backend(QObject *parent = nullptr);
    Backend(const QStringList &roots, QObject *parent = nullptr, const QString &miseCommand = {},
            const QString &pyenvCommand = {}, const QString &asdfCommand = {}, const QString &alternativesCommand = {});
    ~Backend() override;

    QString defaultVersion() const { return m_defaultVersion; }
    bool scanning() const { return m_scanning; }
    QString statusMessage() const { return m_statusMessage; }
    QStringList versions() const { return m_versions; }
    bool hasVersion(const QString &version) const;

    QString installSource(const QString &version) const;
    QString installPath(const QString &version) const;

    Q_INVOKABLE QVariantList versionEntries() const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool useVersion(const QString &version);

signals:
    void versionsChanged();
    void defaultVersionChanged();
    void scanningChanged();
    void statusMessageChanged();

private:
    struct Install {
        QString version;
        QString source;
        QString path;
    };
    void scan();
    void refreshDefault();
    void setUpDefaultRoots();

    static QString sourceForPath(const QString &path);
    static QStringList defaultRootPaths();

    QStringList m_roots;
    QStringList m_rootSources;
    QStringList m_versions;
    QHash<QString, Install> m_installs;
    QString m_defaultVersion;
    bool m_scanning = false;
    QString m_statusMessage;
    QString m_miseCommand, m_pyenvCommand, m_asdfCommand, m_alternativesCommand;
};
