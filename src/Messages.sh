#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT *.cpp dialogs/*.cpp cells/*.cpp io/*.cpp library/*.cpp -o $podir/krossword.pot
