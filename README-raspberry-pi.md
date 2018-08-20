# How to install on a Raspberry Pi

We need to get Qt 5.9 (the default 5.7 is too old, whereas 5.9 is an LTS release
which Raspbian ought to be using by default now).  Edit /etc/apt/sources.list and add

```deb http://mirrordirector.raspbian.org/raspbian/ testing main contrib non-free rpi```

Then

```
$ sudo apt-get update
$ sudo apt-get install qt5-default libqt5bluetooth5 libqt5svg5-dev qtconnectivity5-dev
$ qmake
$ make -j4
$ bluetoothctl
[bluetooth]# power on
[bluetooth]# exit
$ ./trayble
```

It's still not working reliably enough yet, but you might get lucky.

You can also try the doc/read-scale.sh script.
