#!/usr/bin/sh

#Шаблон добавляет файл в автозагрузку

#path должен быть каталогом
path=/home/$1
#создаём каталоги до файла назачения
mkdir -p $path/.config/autostart && cp $2 $path/.config/autostart || exit 1