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

#include <QDebug>

#include "razermatrixdevice.h"

bool RazerMatrixDevice::initializeLeds()
{
    foreach(RazerLedId ledId, ledIds) {
        RazerMatrixLED *rled = new RazerMatrixLED(ledId);
        bool ok;
        uchar brightness;
        ok = getBrightness(ledId, &brightness);
        if(!ok) {
            qDebug() << "Error during getBrightness()";
            return false;
        }
        if(!setSpectrumInit(ledId)) {
            qDebug() << "Error during setSpectrumInit()";
            return false;
        }
        rled->brightness = brightness;
        rled->effect = RazerMatrixEffectId::Spectrum;
        leds.insert(ledId, rled);
    }
    return true;
}

bool RazerMatrixDevice::setNone(RazerLedId led)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Off);
}

bool RazerMatrixDevice::setStatic(RazerLedId led, uchar red, uchar green, uchar blue)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Static, red, green, blue);
}

bool RazerMatrixDevice::setBreathing(RazerLedId led, uchar red, uchar green, uchar blue)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Breathing, 0x01, red, green, blue);
}

bool RazerMatrixDevice::setBreathingDual(RazerLedId led, uchar red, uchar green, uchar blue, uchar red2, uchar green2, uchar blue2)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Breathing, 0x02, red, green, blue, red2, green2, blue2);
}

bool RazerMatrixDevice::setBreathingRandom(RazerLedId led)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Breathing, 0x03);
}

bool RazerMatrixDevice::setBlinking(RazerLedId led, uchar red, uchar green, uchar blue)
{
    qDebug() << "setBlinking() not implemented.";
    return false;
}

bool RazerMatrixDevice::setSpectrum(RazerLedId led)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Spectrum);
}

void RazerMatrixDevice::setWave(RazerLedId led, WaveDirection direction)
{
    if(setMatrixEffect(led, RazerMatrixEffectId::Wave, static_cast<uchar>(direction)) != 0)
        sendErrorReply(QDBusError::Failed);
}

bool RazerMatrixDevice::setCustomFrame(RazerLedId led)
{
    return setMatrixEffect(led, RazerMatrixEffectId::Custom);
}

bool RazerMatrixDevice::setBrightness(RazerLedId led, uchar brightness)
{
    razer_report report, response_report;

    report = razer_chroma_standard_set_led_brightness(RazerVarstore::STORE, led, brightness);
    if(sendReport(report, &response_report) != 0) {
        return false;
    }

    // Save state into LED variable
    RazerMatrixLED *rled = dynamic_cast<RazerMatrixLED*>(leds[led]);
    if(rled == NULL) {
        qDebug() << "Error while casting RazerLED into RazerMatrixLED";
        return false;
    }
    rled->brightness = brightness;

    return true;
}

bool RazerMatrixDevice::getBrightness(RazerLedId led, uchar *brightness)
{
    razer_report report, response_report;

    report = razer_chroma_standard_get_led_brightness(RazerVarstore::STORE, led);
    if(sendReport(report, &response_report) != 0) {
        return false;
    }

    *brightness = response_report.arguments[2];

    return true;
}

bool RazerMatrixDevice::setSpectrumInit(RazerLedId led)
{
    razer_report report, response_report;

    report = razer_chroma_standard_matrix_effect(RazerMatrixEffectId::Spectrum);
    if(sendReport(report, &response_report) != 0) {
        return false;
    }

    return true;
}

bool RazerMatrixDevice::setMatrixEffect(RazerLedId led, RazerMatrixEffectId effect, uchar arg1, uchar arg2, uchar arg3, uchar arg4, uchar arg5, uchar arg6, uchar arg7, uchar arg8)
{
    razer_report report, response_report;

    report = razer_chroma_standard_matrix_effect(effect);
    report.arguments[1] = arg1;
    report.arguments[2] = arg2;
    report.arguments[3] = arg3;
    report.arguments[4] = arg4;
    report.arguments[5] = arg5;
    report.arguments[6] = arg6;
    report.arguments[7] = arg7;
    report.arguments[8] = arg8;

    if(sendReport(report, &response_report) != 0) {
        return false;
    }

    // Save state into LED variable
    RazerMatrixLED *rled = dynamic_cast<RazerMatrixLED*>(leds[led]);
    if(rled == NULL) {
        qDebug() << "Error while casting RazerLED into RazerMatrixLED";
        return false;
    }
    rled->effect = effect;

    return true;
}
