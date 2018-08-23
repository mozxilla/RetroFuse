#!/bin/bash
echo Configuring...
FUSE_REF=patch/fuse-1.3.0-ref
./configure  --host=armv5e-poky-linux-gnueabi --with-libspectrum-prefix=patch/libspectrum-1.3.0 --with-sysroot=/opt/poky/vega-one/sysroots/armv5e-poky-linux-gnueabi --prefix=/media/sf_Retro/Fuse/fuse-noglibc --with-fb --without-gpm --without-libxml2 --without-pthread --without-joystick --without-png --without-zlib --without-gtk
echo Patching fuse...
sed 's/\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/#\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/g' Makefile > /tmp/Makefile
cp /tmp/Makefile .
cd ui/sdl && sed 's/\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/#\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/g' Makefile > /tmp/Makefile
cp /tmp/Makefile .
cd ../..
cd ui/fb && sed 's/\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/#\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/g' Makefile > /tmp/Makefile
cp /tmp/Makefile .
cd ../..
cd ui/widget && sed 's/\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/#\/opt\/poky\/vega-one\/sysroots\/x86_64-pokysdk-linux\/usr\/bin\/perl/g' Makefile > /tmp/Makefile
cp /tmp/Makefile .
cd ../..

echo Copying resources...
cp $FUSE_REF/settings.c .
cp $FUSE_REF/settings.h .
cp $FUSE_REF/options.h .
cp $FUSE_REF/ui/fb/keysyms.c ui/fb
cp $FUSE_REF/ui/widget/options.c ui/widget
cp $FUSE_REF/ui/widget/options_internals.h ui/widget
cp $FUSE_REF/ui/widget/fuse.font ui/widget
cp $FUSE_REF/z80/opcodes_base.c z80
cp $FUSE_REF/z80/z80_cb.c z80
cp $FUSE_REF/z80/z80_ddfd.c z80
cp $FUSE_REF/z80/z80_ddfdcb.c z80
cp $FUSE_REF/z80/z80_ed.c z80

echo Patching fuse...
patch -p0 < patch/makefile.patch
touch ui/widget/.deps/virtualkeyboard.Po
cp patch/menu_data.c ui/widget
