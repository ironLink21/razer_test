/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp devicemanager.xml -a devicemanageradaptor.h:devicemanageradaptor.cpp -c DeviceManagerAdaptor
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef DEVICEMANAGERADAPTOR_H
#define DEVICEMANAGERADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface io.github.openrazer1.Manager
 */
class DeviceManagerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.openrazer1.Manager")
    Q_CLASSINFO("D-Bus Introspection", ""
                "  <interface name=\"io.github.openrazer1.Manager\">\n"
                "    <property access=\"read\" type=\"s\" name=\"version\"/>\n"
                "    <method name=\"getVersion\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"getDevices\">\n"
                "      <arg direction=\"out\" type=\"a(o)\"/>\n"
                "      <annotation value=\"QVector&lt;QDBusObjectPath&gt;\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "")
public:
    DeviceManagerAdaptor(QObject *parent);
    virtual ~DeviceManagerAdaptor();

public: // PROPERTIES
    Q_PROPERTY(QString version READ version)
    QString version() const;

public Q_SLOTS: // METHODS
    QVector<QDBusObjectPath> getDevices();
    QString getVersion();
Q_SIGNALS: // SIGNALS
};

#endif
