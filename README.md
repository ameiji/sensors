DESCRIPTION:

'Sensors' is a simple GTK+ application which monitors:

 - CPU temperature (acpi and coretemp sensors supported)
 - CPU core frequency
 - CPU usage (user,sys,total)


'Sensors' is written specifically for FreeBSD systems and makes use of specific kernel modules and sysctl variables.




INSTALLATION:

You'll need to have the following apps installed to build and run 'sensors':

- make
- cc/clang
- pkgconf
- pango
- cairo
- gtk-2


Unpack and run: 

```
 $ make 
```
Load coretemp.ko module if it supports your CPU:

```
 # kldload coretemp
```

USAGE:

```
 $ ./sensors [delay]
```

 delay - is a refresh time in seconds.



BUGS:

- [ ] Tray status icon is a stub. Yet. Bear with it :)
- [ ] Support for different hardware needs testing.



SEE ALSO:

coretemp(4), sysctl(8)



LICENSE:

Please see the LICENSE file provided with this code.




