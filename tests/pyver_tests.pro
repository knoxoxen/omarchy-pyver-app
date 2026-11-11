QT += core gui quick quickcontrols2 testlib dbus

CONFIG += c++17 testcase
TEMPLATE = app
TARGET = pyver_tests

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/theme.h ../src/translator.h
SOURCES += pyver_tests.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp
