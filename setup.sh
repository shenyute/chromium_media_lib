#!/bin/bash

set -e

cd `dirname $0`

cd .. # to chromium/src

patch -p1 < chromium_media_lib/build.patch

param='ffmpeg_branding="Chrome" proprietary_codecs=true use_debug_fission=false'
gn gen --args="${param}" out/Debug
