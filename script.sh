#!/bin/sh
SCALEX=10
SCALEY=25
height=$((SCALEY*15))
width=$((SCALEX*50))

# Management
gnome-terminal -e "./liftMng" --geometry=45x15+0+0
gnome-terminal -e "./liftCtrl" --geometry=45x15+0+$height

# Floor
height=$(((SCALEY+1)*20))
gnome-terminal -e "./opePanel1"	--geometry=45x5+$width+$height #floor 1
# floor 2 to 5
for i in $(seq 2 5)
do
	height=$(((SCALEY+1)*(5-i)*5+5))
	gnome-terminal -e "./opePanelX $i" --geometry=45x5+$width+$height
done

# Elevator
width=$((width*2))
height=$((SCALEY*15))
gnome-terminal -e "./liftBody"  --geometry=50x15+$width+0
gnome-terminal -e "./liftSensor" --geometry=50x15+$width+$height