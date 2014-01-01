#!/bin/sh
#
# script to build the menu bar using ImageMagick
#
rm -f *.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Esc -frame 4 01_esc.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Ret -frame 4 02_ret.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Inv -frame 4 03_inv.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Drop -frame 4 04_drop.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Wear -frame 4 05_wear.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Wield -frame 4 06_wield.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Quaff -frame 4 07_quaff.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Remove -frame 4 08_remove.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Help -frame 4 09_help.png
convert -background white -fill black -font Helvetica -size x20 -gravity center label:Quit -frame 4 10_quit.png

convert *.png +append ularn_menu.bmp
