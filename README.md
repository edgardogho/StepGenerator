# StepGenerator
Arduino Stepper Motor pulse generator

This is a very basic sketch that controls a stepper motor (using Pulse/Dir signals) based on a single input. When the input goes from HIGH to LOW it executes a pre-programmed pulse sequence count at a fixed interval.
There is a switch to put it into setup mode where the number of pulses and the period (uS) can be set and tested.
The normal run mode counts how many times the sensor was triggered.

It uses a 16x2 LCD (i2c), a digital encoder with click, a pushbutton for manual control along with several switches to change modes.

This sketch does not handle acceleration ramps for the stepper which might are required for more RPMs.

On a NEMA 34 8Nm 6A Bipolar Stepper motor, with 3200 microsteps, I was able to get 340 RPMs.
