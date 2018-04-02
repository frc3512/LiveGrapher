#!/bin/bash

PREFIX=x86_64-w64-mingw32
DATE=`date +"%Y%m%d"`

# Cross-compile LiveGrapher
rm -rf build
mkdir -p build && cd build
$PREFIX-qmake-qt5 ..
make -j$(nproc)
cd ..

# Create destination folder
rm -r LiveGrapher-$DATE LiveGrapher-$DATE.zip
mkdir -p LiveGrapher-$DATE/platforms

# Copy files into folder
cp build/release/LiveGrapher.exe /usr/$PREFIX/bin/{Qt5Core,Qt5Gui,Qt5PrintSupport,Qt5Network,Qt5Widgets,libbz2-1,libfreetype-6,libgcc_s_seh-1,libglib-2.0-0,libgraphite2,libharfbuzz-0,libiconv-2,libintl-8,libpcre-1,libpcre2-16-0,libpng16-16,libstdc++-6,libwinpthread-1,zlib1}.dll IPSettings.txt LiveGrapher-$DATE
cp /usr/$PREFIX/lib/qt/plugins/platforms/qwindows.dll LiveGrapher-$DATE/platforms

# Create archive
zip -r LiveGrapher-$DATE.zip LiveGrapher-$DATE
