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

#ifndef RAZERCLASSICDEVICE_H
#define RAZERCLASSICDEVICE_H

#include "razerdevice.h"
#include "../led/razerled.h"

/**
 * @todo write docs
 */
class RazerClassicDevice : public RazerDevice
{
    using RazerDevice::RazerDevice;

    virtual bool setNone(RazerLedId led);
    virtual bool setStatic(RazerLedId led, uchar red, uchar green, uchar blue);
    virtual bool setBreathing(RazerLedId led, uchar red, uchar green, uchar blue);
    virtual bool setSpectrum(RazerLedId led);
    virtual bool setWave(RazerLedId led);

    virtual bool setBrightness(RazerLedId led, uchar brightness);

    bool setLedState(RazerLedId led, RazerClassicLedState state);
    bool ensureLedStateOn(RazerLedId led);
};

#endif // RAZERCLASSICDEVICE_H
