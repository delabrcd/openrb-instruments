# XBOX ONE ROCKBAND PRO DRUMS USB 
an arduino leonardo based midi pro adapter, emulating a PDP legacy adapter and presenting itself as pro drums, very close in functionality to the [Roll Limitless](https://rolllimitless.com/) but open source. 

# Table of Contents
- [XBOX ONE ROCKBAND PRO DRUMS USB](#xbox-one-rockband-pro-drums-usb)
- [Table of Contents](#table-of-contents)
- [Parts](#parts)
  - [Necassary Parts](#necassary-parts)
  - [Optional (Debugging) Tools](#optional-debugging-tools)
- [Assembly](#assembly)
  - [Physical Assembly](#physical-assembly)
  - [Flashing the Arduino](#flashing-the-arduino)

# Parts
## Necassary Parts 
- Arduino Leonardo or any clone & micro-usb -> usb a cable
- [USB Host Shield](https://www.aliexpress.us/item/3256805054675231.html?spm=a2g0o.productlist.main.71.410634f7EOVIeG&algo_pvid=ac99536d-85a8-46b0-94af-9538ab88b9a7&algo_exp_id=ac99536d-85a8-46b0-94af-9538ab88b9a7-35&pdp_ext_f=%7B%22sku_id%22%3A%2212000032330281734%22%7D&pdp_npi=3%40dis%21USD%2115.27%2114.35%21%21%21%21%21%40211bf2da16781320629492357d070e%2112000032330281734%21sea%21US%21821067191&curPageLogUid=P49Bow2d3Lud) [^1]
- Any arduino midi shield (only serial midi is supported for now, but usb midi is possible with no extra hardware)
- XBOX One Controller & coresponding USB-x -> USB-A Cable (newer series ones use USB-C, XBONE controllers use micro) [^2]

## Optional (Debugging) Tools  
- TTL Serial Adapter 

# Assembly 
## Physical Assembly


Here's the whole system: 

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/block-diagram.jpg?raw=true)


Boards:

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/boards.jpg?raw=true)

Final Product:

![alt text](https://github.com/delabrcd/rockband-drums-usb/blob/master/docs/assembled.jpg?raw=true)

## Flashing the Arduino





[^1]: Host Shields are very finicky - the most popular design that floats around is incorrect, so be sure that the PCB is white **AND** product picture includes the "RoHS" stamp or has "Arduino Meets Android", printed above the "USB Host Shield" text 

[^2]: I've only tested this with an older Series S controller, if your controller is plugged in, but the XBOX light doesn't come on, open an issue with the [vendor ID and product ID](https://superuser.com/questions/1106247/how-can-i-get-the-vendor-id-and-product-id-for-a-usb-device) so I can add support 
