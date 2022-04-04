#!/usr/bin/sh

# Шаблон изменяет количество допустимого использования памяти для хранения журнала journalctl

path=/etc/systemd/journald.conf

if grep -Fq "SystemMaxUse=" $path; then
	sed 's/.*SystemMaxUse=.*/SystemMaxUse='"$1"'/g' -i $path || exit 1
else
	sed 's/\[Journal\]/\[Journal\]\nSystemMaxUse='"$1"'/g' -i $path || exit 1
fi