#!/bin/bash

# Adapted from https://github.com/TheDoctor0/zip-release.
# Create an archive or exit if the command fails.

set -eu
printf "\n📦 Creating zip archive...\n"

if [ "$RUNNER_OS" = "Windows" ]; then
    if [ "$ARCHIVE_SPLIT" = 1 ]; then
        for dir in $ARCHIVE_INCLUDE_PATH/*/ ; do
            7z a -tzip "$dir" $ARCHIVE_INCLUDE_PATH/$dir/ || { printf "\n⛔ Unable to create zip archive.\n"; exit 1; }
            printf "\n✔ Successfully created %s archive.\n" "$dir"
        done
    else
        7z a -tzip "$ARCHIVE_OUTPUT_NAME" $ARCHIVE_INCLUDE_PATH || { printf "\n⛔ Unable to create zip archive.\n"; exit 1; }
        printf "\n✔ Successfully created %s archive.\n" "$ARCHIVE_OUTPUT_NAME"
    fi
else
    if [ "$ARCHIVE_SPLIT" = 1 ]; then
        for dir in $ARCHIVE_INCLUDE_PATH/*/ ; do
            zip -r "$dir" $ARCHIVE_INCLUDE_PATH/$dir/ || { printf "\n⛔ Unable to create zip archive.\n"; exit 1; }
            printf "\n✔ Successfully created %s archive.\n" "$dir"
        done
    else
        zip -r "$ARCHIVE_OUTPUT_NAME" $ARCHIVE_INCLUDE_PATH || { printf "\n⛔ Unable to create zip archive.\n"; exit 1; }
        printf "\n✔ Successfully created %s archive.\n" "$ARCHIVE_OUTPUT_NAME"
    fi
fi

