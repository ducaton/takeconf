#!/usr/bin/sh

#Шаблон устанавливает пакеты

err=0

for package in "$@"; do
	if pacman -Q "$package" > /dev/null 2>&1; then
		continue
	else
		pacman -S --noconfirm "$package" || err=1
	fi
done

if [ $err -eq 1 ]; then
	exit 1
fi