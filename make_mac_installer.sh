#!/bin/bash

IDENTIFIER=org.hapcommunity.HapEncoderPlugin
VERSION=1
PRODUCT=HapEncoderPlugin
INSTALLER_TMP_DIR=$(mktemp -d)
INSTALLER_TMP_RESOURCES_DIR=$INSTALLER_TMP_DIR/Resources
DISTRIBUTION=$PRODUCT-macOS-installer.dist

pushd Release
pkgbuild --identifier $IDENTIFIER --version $VERSION --install-location "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore" --component "source/premiere_CC2018/$PRODUCT.bundle" "$PRODUCT.pkg"
# The following creates a starter distribution from pkgbuild output
#productbuild --synthesize --package "$PRODUCT.pkg" "$DISTRIBUTION"
mkdir "$INSTALLER_TMP_RESOURCES_DIR"
cp ../license.txt $INSTALLER_TMP_RESOURCES_DIR/
productbuild --distribution ../$DISTRIBUTION --resources "$INSTALLER_TMP_RESOURCES_DIR" "$PRODUCT-unsigned.pkg"
# An installer must be signed, requires Apple Developer Program membership
productsign --sign "Developer ID Installer" "$PRODUCT-unsigned.pkg" "$PRODUCT Installer.pkg"
rm "$PRODUCT-unsigned.pkg"
rm "$PRODUCT.pkg"
popd
