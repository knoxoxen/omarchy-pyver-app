#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class PyverTests : public QObject {
    Q_OBJECT

private slots:
    void backendScansRootsForVersions()
    {
        QTemporaryDir dir;
        QFile python3(dir.path() + "/python3.12");
        QVERIFY(python3.open(QIODevice::WriteOnly));
        python3.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.12", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QFile python3b(dir.path() + "/python3.13");
        QVERIFY(python3b.open(QIODevice::WriteOnly));
        python3b.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.13", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend backend(QStringList{dir.path()}, nullptr, "", "", "", "");
        backend.refresh();
        QCOMPARE(backend.versions().size(), 2);
        QVERIFY(backend.versions().contains("3.12"));
        QVERIFY(backend.versions().contains("3.13"));
        QCOMPARE(backend.versions().first(), QString("3.13"));
        QCOMPARE(backend.defaultVersion(), QString());
        QVERIFY(backend.statusMessage().isEmpty());
        QVERIFY(!backend.scanning());
    }

    void backendSkipsBarePythonNames()
    {
        QTemporaryDir dir;
        QFile bare(dir.path() + "/python3");
        QVERIFY(bare.open(QIODevice::WriteOnly));
        bare.close();
        QFile real(dir.path() + "/python3.14");
        QVERIFY(real.open(QIODevice::WriteOnly));
        real.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.14", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend backend(QStringList{dir.path()}, nullptr, "", "", "", "");
        backend.refresh();
        QCOMPARE(backend.versions().size(), 1);
        QCOMPARE(backend.versions().first(), QString("3.14"));
    }

    void backendFindsDefaultFromSystemSymlink()
    {
        QTemporaryDir dir;
        QFile real(dir.path() + "/python3.12");
        QVERIFY(real.open(QIODevice::WriteOnly));
        real.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.12", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(QFile::link(dir.path() + "/python3.12", dir.path() + "/python3"));

        Backend backend(QStringList{dir.path()}, nullptr, "", "", "", "");
        backend.refresh();
        QCOMPARE(backend.defaultVersion(), QString("3.12"));
    }

    void backendFindsDefaultFromManagerGlobal()
    {
        QTemporaryDir dir;
        QFile real(dir.path() + "/python3.12");
        QVERIFY(real.open(QIODevice::WriteOnly));
        real.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.12", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QTemporaryDir scripts;
        QFile mise(scripts.path() + "/mise");
        QVERIFY(mise.open(QIODevice::WriteOnly));
        mise.write("#!/bin/sh\necho 3.12\n");
        mise.close();
        QVERIFY(QFile::setPermissions(scripts.path() + "/mise", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend backend(QStringList{dir.path()}, nullptr, scripts.path() + "/mise", "", "", "");
        backend.refresh();
        QCOMPARE(backend.defaultVersion(), QString("3.12"));
    }

    void backendUseVersionUnknownVersionFails()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("bin");
        QFile real(dir.path() + "/bin/python3.12");
        QVERIFY(real.open(QIODevice::WriteOnly));
        real.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/bin/python3.12", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend backend(QStringList{dir.path()}, nullptr, "", "", "", "");
        backend.refresh();
        QCOMPARE(backend.useVersion("9.9"), false);
        QVERIFY(!backend.statusMessage().isEmpty());
    }

    void backendUseVersionSwitchesSystemDefault()
    {
        QTemporaryDir dir;
        QFile real(dir.path() + "/python3.12");
        QVERIFY(real.open(QIODevice::WriteOnly));
        real.close();
        QVERIFY(QFile::setPermissions(dir.path() + "/python3.12", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QTemporaryDir scripts;
        QFile alternatives(scripts.path() + "/update-alternatives");
        QVERIFY(alternatives.open(QIODevice::WriteOnly));
        alternatives.write("#!/bin/sh\nexit 0\n");
        alternatives.close();
        QVERIFY(QFile::setPermissions(scripts.path() + "/update-alternatives", QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend backend(QStringList{dir.path()}, nullptr, "", "", "", scripts.path() + "/update-alternatives");
        backend.refresh();
        QCOMPARE(backend.useVersion("3.12"), true);
        QCOMPARE(backend.defaultVersion(), QString("3.12"));
        QVERIFY(backend.statusMessage().contains("3.12"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(PyverTests)
#include "pyver_tests.moc"
