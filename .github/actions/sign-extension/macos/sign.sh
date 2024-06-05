#!/bin/bash

# Based on https://github.com/godot-jolt/godot-jolt/blob/master/scripts/ci_sign_macos.ps1

apple_dev_id="$APPLE_DEV_ID"
apple_dev_app_id="$APPLE_DEV_APP_ID"
apple_dev_team_id="$APPLE_DEV_TEAM_ID"
apple_dev_password="$APPLE_DEV_PASSWORD"

framework_path="$FRAMEWORK_PATH"
archive_path="$ARCHIVE_PATH.zip"

if [ -z "${apple_dev_id}" ]; then
  echo "ERROR: Missing Apple developer ID."
  exit 1
fi
if [ -z "${apple_dev_app_id}" ]; then
  echo "ERROR: Missing Apple developer application ID."
  exit 1
fi
if [ -z "${apple_dev_team_id}" ]; then
  echo "ERROR: Missing Apple team ID."
  exit 1
fi
if [ -z "${apple_dev_password}" ]; then
  echo "ERROR: Missing Apple developer password."
  exit 1
fi
if [ -z "${framework_path}" ]; then
  echo "ERROR: Missing framework path to sign."
  exit 1
fi

# Sign and notarize the framework.

echo "Signing and verifying the framework at '${framework_path}'..."

codesign --timestamp --verbose --deep --sign "${apple_dev_app_id}" "${framework_path}"
codesign --verify "${framework_path}"

echo "Archiving and notarizing the signed framework..."

ditto -ck "${framework_path}" "${archive_path}"
xcrun notarytool submit "${archive_path}" --apple-id ${apple_dev_id} --team-id ${apple_dev_team_id} --password ${apple_dev_password} --wait
