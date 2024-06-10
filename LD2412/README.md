# Homeassistant support for HLK-LD2412
This is my quick-and-dirty effort to support the new HLK-LD2412 sensor

## Into
In my testing, the LD2412 is much better than the LD2410, but there isn't an official support for it in ESPHome. 
The serial interface is similar to the LD2410, but not quite the same, so I used the existing LD2410 code and adapted it a bit so 
I can test it properly.

## Credits
Well, I mainly used other people's code and adapted it to my needs, most notibly
* https://github.com/rainchi/ESPHome-LD2410

## Approach
I needed something quickly, so my approach was that I do all the configuration of the LD2412 with the HLKRadarTool app on iOS via bluetooth and this custom ESPHome component
will mainly support the sensor values, so I can use it within Homeassistant.

So pretty much no configuration is supported at this time.

## Usage
* Place ld2412.h into your esphome configuration folder
* Add the ld2412.yaml to your YAML file for your device

## Screenshot
It will look something like this
![image](https://github.com/abaumhof/homeassistant_public/assets/9362029/107d43c0-a383-4ae9-80e1-061fe71c298f)
