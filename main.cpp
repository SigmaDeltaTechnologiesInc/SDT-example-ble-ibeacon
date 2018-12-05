/* SDT-example-ble-ibeacon
 * 
 * Copyright (c) 2018 Sigma Delta Technologies Inc.
 * 
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/iBeacon.h"
#include "events/mbed_events.h"

/* Serial */
#define BAUDRATE 9600
Serial serial_pc(USBTX, USBRX, BAUDRATE);

/* DigitalOut */
#define LED_ON      0
#define LED_OFF     1
DigitalOut do_ledBlue(LED_BLUE, LED_OFF);

/* Ticker */
Ticker ticker;

/* EventQueue */
EventQueue eventQueue(4 * EVENTS_EVENT_SIZE);

/* BLE */
#define BLE_DEVICE_NAME "SDT Device"
BLE& ble_SDTDevice = BLE::Instance();  // Default ID of instance is 'DEFAULT_INSTANCE'

/* iBeacon */
iBeacon* piBeacon;



void callbackTicker(void) {
    serial_pc.printf("LED Toggle\n");
    do_ledBlue = !do_ledBlue;
}

void callbackEventsToProcess(BLE::OnEventsToProcessCallbackContext* context) {
    eventQueue.call(Callback<void()>(&ble_SDTDevice, &BLE::processEvents));
}

void initIBeacon(BLE& ble) {
    /**
    * The Beacon payload has the following composition:
    * 128-Bit / 16byte UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
    * Major/Minor  = 0x1122 / 0x3344
    * Tx Power     = 0xC8 = 200, 2's compliment is 256-200 = (-56dB)
    *
    * Note: please remember to calibrate your beacons TX Power for more accurate results.
    */
    const uint8_t uuid[] = {0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4,
                            0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61};
    uint16_t majorNumber = 1122;
    uint16_t minorNumber = 3344;
    uint16_t txPower     = 0xC8;
    piBeacon = new iBeacon(ble, uuid, majorNumber, minorNumber, txPower);
}

void printMacAddress(BLE& ble) {
    /* Print out device MAC address to the console */
    Gap::AddressType_t addr_type;
    Gap::Address_t address;

    ble.gap().getAddress(&addr_type, address);
    serial_pc.printf("DEVICE MAC ADDRESS = ");
    for (int i = 5; i >= 1; i--){
        serial_pc.printf("%02x:", address[i]);
    }
    serial_pc.printf("%02x\n", address[0]);
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void callbackBleInitComplete(BLE::InitializationCompleteCallbackContext* params) {
    BLE& ble = params->ble;                     // 'ble' equals ble_SDTDevice declared in global
    ble_error_t error = params->error;          // 'error' has BLE_ERROR_NONE if the initialization procedure started successfully.

    if (error == BLE_ERROR_NONE) {
        serial_pc.printf("Initialization completed successfully\n");
    }
    else {
        /* In case of error, forward the error handling to onBleInitError */
        serial_pc.printf("Initialization failled\n");
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        serial_pc.printf("ID of BLE instance is not DEFAULT_INSTANCE\n");
        return;
    }

    /* Setup iBeacon */
    initIBeacon(ble);

    /* Setup and start advertising */
    printMacAddress(ble);                       // Optional function
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, (const uint8_t *)BLE_DEVICE_NAME, sizeof(BLE_DEVICE_NAME) - 1);
    ble.gap().setAdvertisingInterval(1000);     // Advertising interval in units of milliseconds
    ble.gap().startAdvertising();
    serial_pc.printf("Start advertising\n");
}

int main(void) {
    serial_pc.printf("< Sigma Delta Technologies Inc. >\n\r");

    /* Init BLE and iBeacon */
    ble_SDTDevice.onEventsToProcess(callbackEventsToProcess);
    ble_SDTDevice.init(callbackBleInitComplete);

    /* Check whether IC is running or not */
    ticker.attach(callbackTicker, 1);

    eventQueue.dispatch_forever();

    return 0;
}
