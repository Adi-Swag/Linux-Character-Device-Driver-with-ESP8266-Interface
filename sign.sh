#!/bin/bash

KDIR=/lib/modules/$(uname -r)/build
SIGN_FILE=$KDIR/scripts/sign-file

KEY=~/module-signing/MOK.key
CERT=~/module-signing/MOK.crt

$SIGN_FILE sha256 $KEY $CERT $1
