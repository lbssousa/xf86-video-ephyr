# xf86-video-ephyr

This is a new DDX driver designed to launch a Xorg server on top of another Xorg server, just like Xephyr does. It's based on former project [xf86-video-nested](http://cgit.freedesktop.org/xorg/driver/xf86-video-nested/), with one important difference: our project's X11 client backend for nested Xorg is fully based on Xephyr.

In order to achieve it, we've imported Xephyr sources here, removed all unneeded kdrive references and replaced the needed ones with its xfree86 counterparts. The driver frontend implementation comes from xf86-video-nested project.

Our main motivation with this driver is to provide a reliable, display-manager-independent way to configure single-GPU multi-seat systems, combining the latest graphical optimizations from Xephyr with the more robust Xorg input support, which is still missing in kdrive. Thus we're dropping from this driver all code that handles input sharing between host X11 server and nested Xorg one (it may be subject for another Xorg input driver).
