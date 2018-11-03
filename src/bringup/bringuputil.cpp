/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Luca Weiss <luca@z3ntu.xyz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bringuputil.h"
#include "../validjsonvalues.h"
#include "../device/razermatrixdevice.h"

#include <QTextStream>

#include <iostream>

BringupUtil::BringupUtil(struct hid_device_info *dev) : dev(dev)
{
}

bool BringupUtil::newDevice()
{
    QTextStream cin(stdin);

    QString input;

    QString name;
    QString vid;
    QString pid;
    QString type;

    qInfo("== razer_test bringup util ==");
    name = QString::fromWCharArray(dev->product_string);
    qInfo("Product: %s", qUtf8Printable(name));
    vid = QString::number(dev->vendor_id, 16).rightJustified(4, '0');
    qInfo("VID: %s", qUtf8Printable(vid));
    pid = QString::number(dev->product_id, 16).rightJustified(4, '0');
    qInfo("PID: %s", qUtf8Printable(pid));
    qInfo("Do you want to bring up this new device? (y/n)");
    std::cout << "> ";
    cin >> input;
    if (input.compare("y", Qt::CaseInsensitive) != 0) {
        qInfo("Exiting bringup util.");
        return false;
    }

    QVector<RazerLedId> ledIds = {RazerLedId::ScrollWheelLED, RazerLedId::BatteryLED, RazerLedId::LogoLED, RazerLedId::BacklightLED, RazerLedId::MacroRecordingLED, RazerLedId::GameModeLED, RazerLedId::KeymapRedLED, RazerLedId::KeymapGreenLED, RazerLedId::KeymapBlueLED, RazerLedId::RightSideLED, RazerLedId::LeftSideLED};
    QVector<RazerDeviceQuirks> quirks = {};
    MatrixDimensions dims = {};
    ushort maxDPI = 0;
    RazerDevice *device = new RazerMatrixDevice(dev->path, dev->vendor_id, dev->product_id, name, type, ledIds, validFx, validFeatures, quirks, dims, maxDPI);
    if (!device->openDeviceHandle()) {
        qCritical("Failed to open device handle.");
        qCritical("You can give your current user permissions to access the hidraw nodes with the following commands:");
        qCritical("$ sudo chmod g+rw /dev/hidraw*");
        qCritical("$ sudo chgrp $(id -g) /dev/hidraw*");
        delete device;
        return true;
    }
    if (!device->initialize()) {
        qCritical("Failed to initialize leds.");
        delete device;
        return true;
    }

    qInfo("What is your device type?");
    qInfo("Valid types: ");
    for (auto value : validType)
        qInfo("- %s", qUtf8Printable(value));
    std::cout << "> ";
    cin >> type;
    if (!validType.contains(type)) {
        qCritical("Invalid device type.");
        return true;
    }

    for (auto led : device->getLeds()) {
        led->setNone();
    }
    qInfo("Now all LEDs should be off. Is that correct? (y/n)");
    std::cout << "> ";
    cin >> input;
    if (input.compare("y", Qt::CaseInsensitive) != 0) {
        qInfo("That's bad :( Exiting for now (TODO: Test out existing quirks if they help)"); // TODO
        return true;
    }
    ledIds.clear();
    for (auto led : device->getLeds()) {
        led->setStatic({0xFF, 0xFF, 0x00});
        qInfo("Did a LED just turn yellow? (y/n)");
        std::cout << "> ";
        cin >> input;
        if (input.compare("y", Qt::CaseInsensitive) == 0) {
            ledIds.append(led->getLedId());
        }
        led->setNone();
    }
    if (ledIds.isEmpty()) {
        qInfo("You said that no LED is supported. Exiting. TODO: quirks"); // TODO
        return true;
    }

    return true;
}
