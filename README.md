# Focus1

This is software to run a stepper motor based camera focus rail.

The purpose of the rail is to automate the taking of a series of
images that will then be used in "focus stacking".
Typically I am doing photomacrography with fields of view
from 3mm to 30mm on a full frame sensor.

This work was done in 2014 to 2016, but was only placed on Github
on 2020.  I am finding it a challenge to find all the pieces after
letting this sit for 5 years.

It runs on a BBB (Beaglebone Black) under the wretched
Angstrom linux that these boards originally shipped with.
One reason for tracking down all the components and "archiving"
them on Github is that I want to move this to a BBB running
debian.

Some fine day I intend to run all of this under my RTOS Kyu on
the BBB -- the interesting part then will be the GUI.
I call this Focus1 in anticipation of Focus2 being the Kyu
based project.

The front end is a web based GUI implemented using node.js
The machine that handles the user interface need only have a
browser and no special software need be installed.

I use the PRU to generate the pulses to move the stepper motor.
That code is written in assembly and will find its way here
eventually
