#!/bin/bash
# macOS installer for the HAP plugins: After Effects output module + Premiere Pro / Media Encoder
# exporter (whose bundle carries the Media Encoder presets in Resources/Presets).
#
#   installer/packaging/build-mac-pkg.sh <dir holding both .bundle files> <version>
#
# Optional environment:
#   APP_IDENTITY       "Developer ID Application: ..."  signs the plugin bundle
#   INSTALLER_IDENTITY "Developer ID Installer: ..."    signs the .pkg
#   NOTARY_PROFILE     notarytool keychain profile      notarises + staples the .pkg
#   OUT_NAME           output file stem (default HAP-Adobe-<version>-macOS)
# Without them the result is unsigned, for local testing only.
set -euo pipefail

SRC=${1:?dir holding HAPAfterEffectsPlugin.bundle and HAPPremierePlugin.bundle}
VERSION=${2:?version}
BUNDLES="HAPAfterEffectsPlugin.bundle HAPPremierePlugin.bundle"
DEST="Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/HAP"
ID=org.hapcommunity.HapCodecPlugin.adobe
OUT="$PWD/${OUT_NAME:-HAP-Adobe-$VERSION-macOS}.pkg"

# notarising only makes sense for a fully signed build - refuse before doing any work
if [ -n "${NOTARY_PROFILE:-}" ]; then
  MISSING=""
  [ -n "${APP_IDENTITY:-}" ] || MISSING="$MISSING
  APP_IDENTITY (Developer ID Application - signs the plug-ins)"
  [ -n "${INSTALLER_IDENTITY:-}" ] || MISSING="$MISSING
  INSTALLER_IDENTITY (Developer ID Installer - signs the .pkg)"
  if [ -n "$MISSING" ]; then
    echo "error: NOTARY_PROFILE is set, but not:$MISSING" >&2
    exit 1
  fi
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/root/$DEST"
[ -n "${APP_IDENTITY:-}" ] || echo "warning: APP_IDENTITY unset - bundles get an ad-hoc signature"
for NAME in $BUNDLES; do
  [ -d "$SRC/$NAME" ] || { echo "missing $SRC/$NAME" >&2; exit 1; }
  ditto "$SRC/$NAME" "$WORK/root/$DEST/$NAME"
  xattr -cr "$WORK/root/$DEST/$NAME"
  if [ -n "${APP_IDENTITY:-}" ]; then
    codesign --force --timestamp --options runtime --sign "$APP_IDENTITY" "$WORK/root/$DEST/$NAME"
  else
    codesign --force --sign - "$WORK/root/$DEST/$NAME"
  fi
  codesign --verify --strict "$WORK/root/$DEST/$NAME"
done

# install exactly where the Adobe apps look; never "relocate" onto some other copy of a bundle
pkgbuild --analyze --root "$WORK/root" "$WORK/components.plist"
i=0   # one array entry per bundle component; walk until there are no more
while plutil -extract "$i.BundleIsRelocatable" raw "$WORK/components.plist" >/dev/null 2>&1; do
  plutil -replace "$i.BundleIsRelocatable" -bool false "$WORK/components.plist"
  i=$((i + 1))
done
pkgbuild --root "$WORK/root" --component-plist "$WORK/components.plist" \
  --identifier "$ID" --version "$VERSION" --install-location / "$WORK/component.pkg"

cat > "$WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>HAP for Adobe CC $VERSION</title>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <allowed-os-versions><os-version min="11.0"/></allowed-os-versions>
    <license file="license.txt"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default" title="HAP for After Effects, Premiere Pro and Media Encoder"><pkg-ref id="$ID"/></choice>
    <pkg-ref id="$ID" version="$VERSION">component.pkg</pkg-ref>
</installer-gui-script>
EOF
mkdir -p "$WORK/resources"
cp "$(dirname "$0")/../../license.txt" "$WORK/resources/license.txt"

SIGN=()
[ -n "${INSTALLER_IDENTITY:-}" ] && SIGN=(--sign "$INSTALLER_IDENTITY" --timestamp)
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" \
  --package-path "$WORK" ${SIGN[@]+"${SIGN[@]}"} "$OUT"   # bash 3.2-safe empty array

if [ -n "${NOTARY_PROFILE:-}" ]; then
  xcrun notarytool submit "$OUT" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$OUT"
fi
pkgutil --check-signature "$OUT" || true
echo "built $OUT"
