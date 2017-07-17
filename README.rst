About
=====

This is the C++ library which wrapper chromium media module to create media player.
Now we use chromium version 59.0.3071.117.

Features
========

 * Support playing local file
 * Normal media player API (ex: play, pause, seek, set volume, set play rate)
 * Not coupling with chromium multi-process architecture

How to build
============

1. Please reference https://chromium.googlesource.com/chromium/src/+/master/docs/linux_build_instructions.md#Update-your-checkout to sync chromium code base. And install necessary library for build chromium::

    $ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    $ export PATH="$PATH:/path/to/depot_tools"
    $ mkdir ~/chromium && cd ~/chromium
    $ fetch --nohooks chromium
    $ cd src

2. Checkout to the tag we use and sync relative repository. ::

    $ cd src
    $ git checkout tags/59.0.3071.117 -b 59.0.3071.117
    $ gclient sync


3. Sync chromium_media_lib. ::

   $ cd src
   $ git clone https://github.com/shenyute/chromium_media_lib.git
   $ cd chromium_media_lib
   $ ./setup.sh

4. Build media example. ::

   $ cd src
   $ ./chromium_media_lib/setup.sh


5. Now you can execute basic sample as following. ::

   $ ./out/Default/media_example <media file path>


Reference
=========

 * Architecture_.

 .. _Architecture: https://docs.google.com/document/d/1uqU2GVcr8a60sUkP3KKB_XyShqZhJvuEHOiQDSBz7hE/edit?usp=sharing

TODO
====

 * Support network media file (with network flow control)
