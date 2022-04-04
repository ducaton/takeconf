#!/usr/bin/sh

#Шаблон прячет или делает видимой корзину на рабочем столе XFCE4

path=/home/$1

if [[ $(ps -p $(pidof xfce4-session) -o user=) == $1 ]]; then
	xfce_on=true
else
	xfce_on=false
fi

case "$2" in
	true | on | enable)
		if [ $xfce_on == true ]; then
			runuser -l $1 -c "
				DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u $1)/bus xfconf-query -c xfce4-desktop -p /desktop-icons/file-icons/show-trash -s 'true' || exit 1
			" || exit 1
		else
			xmlstarlet edit --inplace --update "/channel[@name='xfce4-desktop']/property[@name='desktop-icons']/property[@name='file-icons']/property[@name='show-trash']/@value" --value "true" $path/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-desktop.xml || exit 1	
		fi
	;;
	false | off | disable)
		if [ $xfce_on == true ]; then
			runuser -l $1 -c "
				DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u $1)/bus xfconf-query -c xfce4-desktop -p /desktop-icons/file-icons/show-trash -s 'false' || exit 1
			" || exit 1
		else
			xmlstarlet edit --inplace --update "/channel[@name='xfce4-desktop']/property[@name='desktop-icons']/property[@name='file-icons']/property[@name='show-trash']/@value" --value "false" $path/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-desktop.xml || exit 1		
		fi
	;;
	*)
		echo "Usage: ./TrashVisible.sh {user} {true|on|enable|false|off|disable}" >&2
		exit 1
	;;
esac
