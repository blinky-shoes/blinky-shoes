# blinky-shoes
Code for http://blinky.shoes .

blinky.shoes use an Arduino Leonardo compatible microcontroller to detect your steps from the onboard accelerometer, and send light signals to the LED strips. You can modify the Arduino program running on the blinky.shoes to customize your color palettes, or completely change the behavior of the shoes.

Inspired by the [Firewalker](https://learn.adafruit.com/firewalker-led-sneakers/) code and project for Adafruit NeoPixels by Phillip Burgess.

## Install the Arduino IDE
To get going with the blinky.shoes software, you need to set up the Arduino Integrated Development Environment on your computer. This will let you modify the code and upload it to your blinky.shoes. You can set this up by following the instructions at https://www.arduino.cc/en/Guide/HomePage

## Install Arduino Libraries
We use the *Adafruit *Neopixel library to make the LEDs go, and the *elapsedMilis* library to keep track of time. You can use the Arduino Library Manager to install these libraries by following the instructions at https://www.arduino.cc/en/Guide/Libraries

## Upload Software to blinky.shoes
Once you're set up with the Arduino environment, you can test things are working by uploading this program unmodified to your blinky.shoes. Download this code ("Arduino Sketch") to your computer, then follow the instructions at https://www.arduino.cc/en/main/howto to open the sketch and upload it to your shoes with a USB cable.

## Change Things!
The simplest thing to change is the colors of the "user" mode. You can change the values of `user_color1`, `user_color2`, and `user_color3` to build your own color pallette. The colors used in the code are all 8-bit decimal RGB color codes. You can see the codes for many colors at https://en.wikipedia.org/wiki/Web_colors#X11_color_names .

Once you change the colors, upload your new program your blinky.shoes, and turn the power on and off to rotate color modes until you get to your new customized pallette!
