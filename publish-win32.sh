#!/bin/bash

PROJECT=LiveGrapher
PREFIX=x86_64-w64-mingw32
DATE=`date +"%Y%m%d"`

# Build project
rm -rf build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Create destination folder
rm -rf $PROJECT-$DATE $PROJECT-$DATE.zip
mkdir -p $PROJECT-$DATE/platforms

# Copy files into folder
cp build/release/$PROJECT.exe /usr/$PREFIX/bin/{Qt5Core,Qt5Gui,Qt5PrintSupport,Qt5Network,Qt5Widgets,libbz2-1,libfreetype-6,libgcc_s_seh-1,libglib-2.0-0,libgraphite2,libharfbuzz-0,libiconv-2,libintl-8,libpcre-1,libpcre2-16-0,libpng16-16,libstdc++-6,libwinpthread-1,zlib1}.dll IPSettings.txt $PROJECT-$DATE
cp /usr/$PREFIX/lib/qt/plugins/platforms/qwindows.dll $PROJECT-$DATE/platforms
$PREFIX-strip $PROJECT-$DATE/$PROJECT.exe

# Create archive
zip -r $PROJECT-$DATE.zip $PROJECT-$DATE
