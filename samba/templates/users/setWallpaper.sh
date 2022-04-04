#!/usr/bin/sh

#Шаблон меняет обои рабочего стола и меняет параметры XFCE4, указывая расположение обоев

path="/home/$1"
mkdir -p "$path" && cp "$2" "$path/.wallpaper.jpg" || exit 1

if [[ $(ps -p $(pidof xfce4-session) -o user=) == "$1" ]]; then
	runuser -l "$1" -c '
		export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u "$USER")/bus
		path="/home/$USER"
		xfconf-query --channel xfce4-desktop --list | grep last-image | while read setting; do
			xfconf-query --channel xfce4-desktop --property "$setting" --set "$path/.wallpaper.jpg"
		done
	' || exit 1
else
	sed 's|<property name="last-image" type="string" value=.*|<property name="last-image" type="string" value="'"$path"'\/.wallpaper.jpg">|g' -i "$path/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-desktop.xml" || exit 1
fi
