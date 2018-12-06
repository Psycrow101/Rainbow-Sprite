# Raibow Sprite

**Raibow Sprite** is a simple console application that allows you to animate
the Half-Life sprite, adding the effect of color transfusion.

[![Video tutorial](http://img.youtube.com/vi/maYiBrBojwk/0.jpg)](http://www.youtube.com/watch?v=maYiBrBojwk)

## How to use

Using the command line, launch the application with the following
arguments: `sprite_name.spr` `color_row_value` `"red green blue"` `"red green blue"` ...

* `color_row_value` is the number of gray colors for one frame of animation.
Simply put, this is picture quality. It is better to leave in the range
from 5 to 10. Too much value is better not to set - it depends on
the number of received frames.
* `"red green blue"` is the keyframe color. To loop the animation, repeating the first frame at the end is unnecessary.

## Examples

* *RainbowSprite my_sprite1.spr 8 "255 0 255" "128 0 0"*
* *RainbowSprite my_sprite2.spr 5 "255 0 0" "0 0 64" "0 255 0"*
