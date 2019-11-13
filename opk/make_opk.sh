#!/bin/sh

cd $(dirname $0)

OPK_NAME=snes9x.opk

echo Building ${OPK_NAME}...

# create opk
FLIST="../build/snes9x default.gcw0.desktop sfc.png bg.png"
if [ -f ../translations/locales ]; then
  FLIST="${FLIST} ../translations/locales"
  if [ -d ../build/locale ]; then
    FLIST="${FLIST} ../build/locale"
  fi
fi

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
