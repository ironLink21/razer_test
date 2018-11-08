#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <hidapi.h>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>

#include <QDBusConnection>

#ifdef ENABLE_BRINGUP_UTIL
#include "bringup/bringuputil.h"
#endif
#include "device/razerdevice.h"
#include "device/razerclassicdevice.h"
#include "device/razermatrixdevice.h"
#include "device/razerfakedevice.h"
#include "led/razerled.h"
#include "led/razerclassicled.h"
#include "dbus/razerdeviceadaptor.h"
#include "dbus/devicemanageradaptor.h"
#include "dbus/razerledadaptor.h"
#include "manager/devicemanager.h"
#include "config.h"

#define ANSI_BOLD          "\x1b[1m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_RESET         "\x1b[0m"

// Used to tell myMessageOutput if --verbose was given on the command line
bool verbose = false;

void myMessageOutput(QtMsgType type, const QMessageLogContext &/*context*/, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        if (verbose)
            fprintf(stderr, ANSI_BOLD ANSI_COLOR_GREEN "[debug]" ANSI_RESET " %s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, ANSI_BOLD ANSI_COLOR_GREEN "[info]" ANSI_RESET " %s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, ANSI_BOLD ANSI_COLOR_YELLOW "[warning]" ANSI_RESET " %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, ANSI_BOLD ANSI_COLOR_RED "[critical]" ANSI_RESET " %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, ANSI_BOLD ANSI_COLOR_RED "[fatal]" ANSI_RESET " %s\n", localMsg.constData());
        break;
    }
}

QJsonArray loadDevicesFromJson(bool devel)
{
    QJsonArray devices;

    QDir datadir;
    if (devel)
        datadir = QDir("../data/devices");
    else
        datadir = QDir(RAZER_TEST_DATADIR);

    QStringList filters;
    filters << "*.json";
    datadir.setNameFilters(filters);

    QListIterator<QFileInfo> i(datadir.entryInfoList());
    while (i.hasNext()) {
        QFile f(i.next().absoluteFilePath());
        f.open(QFile::ReadOnly);
        QJsonArray a = QJsonDocument::fromJson(f.readAll()).array();
        foreach (const QJsonValue &value, a) {
            devices.append(value);
        }
    }
    return devices;
}

bool getVidPidFromJson(QJsonObject deviceObj, ushort *vid, ushort *pid)
{
    bool ok;
    *vid = deviceObj.value("vid").toString().toUShort(&ok, 16);
    if (!ok) {
        qCritical() << "Error converting vid: " << deviceObj.value("vid");
        return false;
    }
    *pid = deviceObj.value("pid").toString().toUShort(&ok, 16);
    if (!ok) {
        qCritical() << "Error converting pid: " << deviceObj.value("pid");
        return false;
    }
    return true;
}

bool registerDeviceOnDBus(RazerDevice *device, QDBusConnection &connection)
{
    // D-Bus
    new RazerDeviceAdaptor(device);
    if (!connection.registerObject(device->getObjectPath().path(), device)) {
        qCritical("Failed to register D-Bus object at \"%s\".", qUtf8Printable(device->getObjectPath().path()));
        delete device;
        return false;
    }
    foreach (RazerLED *led, device->getLeds()) {
        new RazerLEDAdaptor(led);
        if (!connection.registerObject(led->getObjectPath().path(), led)) {
            qCritical("Failed to register D-Bus object at \"%s\".", qUtf8Printable(led->getObjectPath().path()));
            delete device;
            return false;
        }
    }
    return true;
}

bool getDeviceInfoFromJson(QJsonObject deviceObj, QString *name, QString *type, QString *pclass, QVector<RazerLedId> *leds, QStringList *fx, QStringList *features, QVector<RazerDeviceQuirks> *quirks, MatrixDimensions *matrixDimensions, ushort *maxDPI)
{
    // TODO: Check everything for sanity
    *name = deviceObj["name"].toString();
    *type = deviceObj["type"].toString();
    *pclass = deviceObj["pclass"].toString();
    foreach (const QJsonValue &ledVal, deviceObj["leds"].toArray()) {
        leds->append(static_cast<RazerLedId>(ledVal.toInt()));
    }
    foreach (const QJsonValue &fxVal, deviceObj["fx"].toArray()) {
        fx->append(fxVal.toString());
    }
    foreach (const QJsonValue &featureVal, deviceObj["features"].toArray()) {
        features->append(featureVal.toString());
    }
    foreach (const QJsonValue &quirkVal, deviceObj["quirks"].toArray()) {
        if (StringToQuirks.contains(quirkVal.toString())) {
            quirks->append(StringToQuirks.value(quirkVal.toString()));
        } else {
            qCritical("Unhandled quirks string!");
        }
    }
    *matrixDimensions = {deviceObj["matrix_dimensions"].toArray()[0].toVariant().value<uchar>(),
                         deviceObj["matrix_dimensions"].toArray()[1].toVariant().value<uchar>()
                        };
    *maxDPI = deviceObj["max_dpi"].toInt();
    return true;
}

/**
 * Initializes the RazerDevice object with the data provided.
 * If dev_path is NULL, a fake device is created, otherwise the matching device based on the input JSON is created.
 * Returns NULL on error (error message is printed with qCritical), or a valid RazerDevice*.
 */
RazerDevice *initializeDevice(QString dev_path, QJsonObject deviceObj)
{
    qInfo().noquote().nospace() << "Initializing device: " << deviceObj["name"].toString() << " (" << deviceObj["vid"].toString() << ":" << deviceObj["pid"].toString() << ")";

    ushort vid, pid;
    if (!getVidPidFromJson(deviceObj, &vid, &pid))
        return nullptr; // Message is printed in that method

    QString name, type, pclass;
    QVector<RazerLedId> leds;
    QStringList fx;
    QStringList features;
    QVector<RazerDeviceQuirks> quirks;
    MatrixDimensions matrixDimensions;
    ushort maxDPI;
    if (!getDeviceInfoFromJson(deviceObj, &name, &type, &pclass, &leds, &fx, &features, &quirks, &matrixDimensions, &maxDPI)) {
        qCritical("Failed to get device info from JSON");
        return nullptr;
    }

    RazerDevice *device;
    if (dev_path == nullptr) { // create a fake device
        device = new RazerFakeDevice(dev_path, vid, pid, name, type, leds, fx, features, quirks, matrixDimensions, maxDPI);
    } else if (pclass == "classic") {
        device = new RazerClassicDevice(dev_path, vid, pid, name, type, leds, fx, features, quirks, matrixDimensions, maxDPI);
    } else if (pclass == "matrix") {
        device = new RazerMatrixDevice(dev_path, vid, pid, name, type, leds, fx, features, quirks, matrixDimensions, maxDPI);
    } else {
        qCritical("Unknown device class: %s", qUtf8Printable(pclass));
        return nullptr;
    }
    if (!device->openDeviceHandle()) {
        qCritical("Failed to open device handle, skipping device.");
        delete device;
        return nullptr;
    }
    if (!device->initialize()) {
        qCritical("Failed to initialize leds, skipping device.");
        delete device;
        return nullptr;
    }
    return device;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Set the directory of the application to where the application is located. Needed for relative paths.
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QCoreApplication::setApplicationName("razer_test");
    QCoreApplication::setApplicationVersion(RAZER_TEST_VERSION);

    // Setup up the command line parser
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"devel", QString("Uses data files at ../data/devices instead of %1.").arg(RAZER_TEST_DATADIR)});
    parser.addOption({"fake-devices", "Adds fake devices instead of real ones."});
    parser.addOption({"verbose", "Print debug messages."});
    parser.process(app);

    verbose = parser.isSet("verbose");
    qInstallMessageHandler(myMessageOutput);

#ifdef ENABLE_BRINGUP_UTIL
    const char *bringup = " (bringup util)";
#else
    const char *bringup = "";
#endif
    qInfo("razer_test%s - version %s", bringup, RAZER_TEST_VERSION);
    if (parser.isSet("devel"))
        qInfo("Running in development mode and using development data files.");

    // Register the enums with the Qt system
    razer_test::registerMetaTypes();

    // Get the D-Bus system bus
    QDBusConnection connection = QDBusConnection::systemBus();

    // Load the supported devices from the json files
    QJsonArray supportedDevices = loadDevicesFromJson(parser.isSet("devel"));
    if (supportedDevices.isEmpty()) {
        qFatal("JSON device definition files were not found. Exiting.");
    }

    QVector<RazerDevice *> devices;

    // Use the real devices
    if (!parser.isSet("fake-devices")) {
        if (hid_init())
            return -1;

        struct hid_device_info *devs, *cur_dev;
        devs = hid_enumerate(0x1532, 0x0000);
        cur_dev = devs;

        while (cur_dev) {
            // TODO maybe needs https://github.com/cyanogen/uchroma/blob/2b8485e5ac931980bacb125b8dff7b9a39ea527f/uchroma/server/device_manager.py#L141-L155
            if (cur_dev->interface_number != 0) {
                cur_dev = cur_dev->next;
                continue;
            }

            bool supported = false;
            // Check if device is supported
            foreach (const QJsonValue &deviceVal, supportedDevices) {
                QJsonObject deviceObj = deviceVal.toObject();

                ushort vid, pid;
                if (!getVidPidFromJson(deviceObj, &vid, &pid))
                    break;

                if (cur_dev->vendor_id == vid && cur_dev->product_id == pid) {
                    RazerDevice *device = initializeDevice(QString(cur_dev->path), deviceObj);
                    if (device == nullptr)
                        break;

                    devices.append(device);

                    // D-Bus
                    registerDeviceOnDBus(device, connection);

                    supported = true;
                    break;
                }
            }
#ifdef ENABLE_BRINGUP_UTIL
            if (!supported) {
                BringupUtil bringupUtil = BringupUtil(cur_dev);
                if (bringupUtil.newDevice()) {
                    return 0;
                }
            }
#else
            Q_UNUSED(supported);
#endif
            cur_dev = cur_dev->next;
        }

        // Free devs and cur_dev pointers
        hid_free_enumeration(devs);
    } else { // Handle fake devices
        // Check if device is supported
        foreach (const QJsonValue &deviceVal, supportedDevices) {
            RazerDevice *device = initializeDevice(nullptr, deviceVal.toObject());
            if (device == nullptr)
                continue;

            devices.append(device);

            // D-Bus
            registerDeviceOnDBus(device, connection);
        }
    }

    DeviceManager *manager = new DeviceManager(devices);
    new DeviceManagerAdaptor(manager);
    if (!connection.registerObject(manager->getObjectPath().path(), manager)) {
        qFatal("Failed to register D-Bus object at \"%s\".", qUtf8Printable(manager->getObjectPath().path()));
    }

#ifdef ENABLE_BRINGUP_UTIL

    if (devices.isEmpty()) {
        qFatal("No device found. Exiting.");
    }
    foreach (RazerDevice *razerDevice, devices) {
        qInfo() << "Device:" << razerDevice->getName();
        qInfo() << "Serial:" << razerDevice->getSerial();
        qInfo() << "Firmware version:" << razerDevice->getFirmwareVersion();

        foreach (RazerLED *led, razerDevice->getLeds()) {
            qInfo() << "Setting LED to static with color #FFFF00";
            qDebug() << "LED object path:" << led->getObjectPath().path();
            led->setStatic({0xFF, 0xFF, 0x00});
            led->setBrightness(255);
        }
    }
    return 0;

#else

    if (!connection.registerService("io.github.openrazer1")) {
        qCritical("Failed to register D-Bus service at \"io.github.openrazer1\".");
        if (connection.lastError().isValid()) {
            qCritical("Additional information:");
            qCritical("%s", qUtf8Printable(connection.lastError().name()));
            qCritical("%s", qUtf8Printable(connection.lastError().message()));
        } else {
            qCritical("Maybe it's already running?");
        }
        return 1;
    }

    qInfo("Initialization finished. D-Bus interface is active...");

    return app.exec();

#endif
}
