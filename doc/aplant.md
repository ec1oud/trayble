From http://wiki.aprbrother.com/en/APlant_Payload_Format.html:

APlant advertises sensor data with minor value in iBeacon protocol. Let's say you use app AprilBeacon scan a beacon with minor=11548.

Decimal 11548 = Heximal 0x2D1C

    Temperature data at Low byte = 0x1C = 28℃
    Soil Moisture Sensor data at High byte = 0x2D = 45, higher value means more wet

From https://en.wikipedia.org/wiki/IBeacon#Packet_Structure_Byte_Map:

Byte 0-2: Standard BLE Flags

 Byte 0: Length :  0x02
 Byte 1: Type: 0x01 (Flags)
 Byte 2: Value: 0x06 (Typical Flags)

Byte 3-29: Apple Defined iBeacon Data

 Byte 3: Length: 0x1a
 Byte 4: Type: 0xff (Custom Manufacturer Packet)
 Byte 5-6: Manufacturer ID : 0x4c00 (Apple)
 Byte 7: SubType: 0x02 (iBeacon)
 Byte 8: SubType Length: 0x15
 Byte 9-24: Proximity UUID
 Byte 25-26: Major
 Byte 27-28: Minor
 Byte 29: Signal Power

After it's been discovered and manufacturer data is updated,
QBluetoothDeviceInfo::manufacturerData(4c) provides stuff like

0215b5b182c7eab14988aa99b5c1517008d93a261d15c5 (in balcony planter)
0215b5b182c7eab14988aa99b5c1517008d93a260816c5 (indoors, clean and dry)

so I think we've got temperature 0x15/0x16 and moisture 0x1d/0x08

