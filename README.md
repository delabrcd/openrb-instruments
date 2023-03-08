# OPENRB-INSTRUMENTS
an arduino leonardo based midi pro adapter, emulating a PDP legacy adapter and presenting itself as pro drums, very close in functionality to the [Roll Limitless](https://rolllimitless.com/) but open source. 

# Table of Contents
- [OPENRB-INSTRUMENTS](#openrb-instruments)
- [Table of Contents](#table-of-contents)
- [Parts](#parts)
  - [Necassary Parts](#necassary-parts)
  - [Optional (Debugging) Tools](#optional-debugging-tools)
- [Assembly](#assembly)
  - [Physical Assembly](#physical-assembly)
  - [Flashing the Arduino](#flashing-the-arduino)
    - [AVR-programmer](#avr-programmer)
    - [Manually Flashing With avrdude](#manually-flashing-with-avrdude)
    - [Building The Code](#building-the-code)
- [Usage](#usage)

# Parts
## Necassary Parts 
- [Arduino Leonardo](https://store.arduino.cc/products/arduino-leonardo-with-headers) or any clone & micro-usb -> usb a cable
- [USB Host Shield](https://www.aliexpress.us/item/3256805054675231.html?spm=a2g0o.productlist.main.71.410634f7EOVIeG&algo_pvid=ac99536d-85a8-46b0-94af-9538ab88b9a7&algo_exp_id=ac99536d-85a8-46b0-94af-9538ab88b9a7-35&pdp_ext_f=%7B%22sku_id%22%3A%2212000032330281734%22%7D&pdp_npi=3%40dis%21USD%2115.27%2114.35%21%21%21%21%21%40211bf2da16781320629492357d070e%2112000032330281734%21sea%21US%21821067191&curPageLogUid=P49Bow2d3Lud) [^1]
- [Any arduino midi shield](https://www.aliexpress.us/item/3256803015940184.html?spm=a2g0o.productlist.main.1.781c7e6ar9DaP8&algo_pvid=2f368073-2f0d-4f9c-815a-b900c00a6dae&algo_exp_id=2f368073-2f0d-4f9c-815a-b900c00a6dae-0&pdp_ext_f=%7B%22sku_id%22%3A%2212000024638075909%22%7D&pdp_npi=3%40dis%21USD%2110.01%216.41%21%21%21%21%21%402102160416781384373844470d06f3%2112000024638075909%21sea%21US%21821067191&curPageLogUid=Q3ucbOkF7JJK) [^2]
- XBOX One Controller & coresponding USB-x -> USB-A Cable (newer series ones use USB-C, XBONE controllers use micro) [^3]

## Optional (Debugging) Tools  
- TTL Serial Adapter - there's a debug stream on serial1 at 115200 baud

# Assembly 
## Physical Assembly


Here's the whole system: 

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/block-diagram.jpg?raw=true)


Boards:

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/boards.jpg?raw=true)

Final Product:

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/assembled.jpg?raw=true)

## Flashing the Arduino
### AVR-programmer
If you want the most straightforward experience, go ahead and download the prebuilt executable for [avr-programmer](https://github.com/delabrcd/avr-programmer/releases).  This is a quick UI I built in python to directly facilitate flashing for this project, for windows it bundles its own copy of avrdude to handle the flashing, but it is cross platform with the caveat that you need to have avrdude installed and in your path for it to work.  

Steps: 
1. Download the latest release of [avr-programmer](https://github.com/delabrcd/avr-programmer/releases)
2. Download the latest release of [openrb](https://github.com/delabrcd/rockband-drums-usb/releases)
3. Leave your arduino unplugged
4. Run `avr-programmer.exe`
5. Select "Type" -> atmega32u4 (this is currently the *only* type)
6. Leave "Port" empty (this will auto-detect when you plug your arduino in)
7. Select the firmware file you downloaded earlier
8. Enable "Auto Flash". Here's an example for final settings: 
![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/avr-programmer-general-settings.png?raw=true)
9.  Plug your Arduino in and wait ~10s. The Leonardo may require you to press the reset button before it flashes, if nothing happens within 10s of plugging in, try pressing the physical reset button on the arduino. A successful flash will look something like this: 
![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/avr-programmer-successful-flash.png?raw=true)
1.  Close avr-programmer and unplug your arduino, you're ready to go! Any firmware updates in the future will be done with this method

### Manually Flashing With avrdude 
If you're comfortable using a command line, familiar with avrdude, and don't want to run some random executable on the internet you can just interface with avrdude yourself to flash. The commands are: 

```
avrdude -patmega32u4 -cavr109 -P<device> -b115200 -D-Uflash:w:<path-to-firmware>:a
```

### Building The Code
The project uses the LUFA build system which utilizes GNU Make and requires a unix-style shell (bash or zsh are officially supported).  Install `avr-gcc` and `avr-libc`, then, `make` to build locally.  

# Usage 
When powered, the XBOX controller guide button will light up, then the builtin orange LED on the Leonardo will light up once the authentication is finished. This indicates everything is ready to go. You can then use the controller to navigate menus or turn on drum navigation and unplug the controller altogether. 

Any time it is reconnected / powered, the XBOX controller will need to be plugged in so that it can handle authentication.

This should be a mostly plug and play solution, but there is more polishing that needs to be done - I'm not sure it syncs up perfectly with the timing of the other XBOX wireless instruments and additional support for USB MIDI and XB360 instruments is coming in the future.


[^1]: Host Shields are very finicky - the most popular design that floats around is incorrect, so be sure that the PCB is white **AND** product picture includes the "RoHS" stamp or has "Arduino Meets Android", printed above the "USB Host Shield" text 
[^2]: only serial midi is supported for now, but with more code in the future usb midi is possible using only the host shield and a usb HUB
[^3]: I've only tested this with an older Series S controller, if your controller is plugged in, but the XBOX light doesn't come on, open an issue with the [vendor ID and product ID](https://superuser.com/questions/1106247/how-can-i-get-the-vendor-id-and-product-id-for-a-usb-device) so I can add support 
