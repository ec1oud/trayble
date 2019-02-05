A Qt system tray application to show and record readings from BLE devices such as the
[1byOne body composition scale](https://www.amazon.com/dp/B01FHELB56).
Rhymes with table.

The values are recorded to an
[influxDB](https://github.com/influxdata/influxdb) instance.

It tries to recognize users by their weight: the first time
it receives readings, it will ask for your name; next time,
if your weight isn't too different it will assume you're the
same person.  But if the weight is too different, it will ask
for the name again.  Thus, it can learn to distinguish family
members if they all have different enough weights, and keep
the data separate in Influx.

So far it does not handle other types of scales, but they
could be added.

Another supported device is the
[APlant soil moisture sensor](http://wiki.aprbrother.com/wiki/APlant).
It can now handle multiple plant sensors, and can log the moisture
and temperature to influxDB.  As with unknown users, the first time
it sees an unknown plant it will ask for the name.

Requires Qt 5.12 or newer with extra modules
qtconnectivity and qtsvg.  Qt Bluetooth doesn't support
all [platforms](http://doc.qt.io/qt-5/qtbluetooth-index.html)
yet, but it should get better.

You could try it on a [Raspberry Pi](README-raspberry-pi.md)
(but it needs a bit more work it seems).

