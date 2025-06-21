#/bin/sh
exec 2> /dev/null
set -x
cd /tmp
[ -d spotart ] || mkdir spotart
rm spotart/*
rm graphTFT.cover
wget $1 -P spotart
file=$(basename $1)
echo $file
mv spotart/$file spotart/${file}.png
convert spotart/${file}.png spotart/${file}.jpg
echo "/tmp/spotart/${file}.jpg" > graphTFT.cover
