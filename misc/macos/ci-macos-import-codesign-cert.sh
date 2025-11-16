#!/bin/sh

set -e

CERTIFICATE_P12_FILE=certificate.p12

if [ -n "${APPLE_CERTIFICATE_P12_BASE64}" ] && [ -n "${APPLE_CERTIFICATE_PASSWORD}" ]
then
    echo ${APPLE_CERTIFICATE_P12_BASE64} | base64 --decode > ${CERTIFICATE_P12_FILE}

    echo "Creating keychain..."
    KEYCHAIN_PASSWORD=$(openssl rand -hex 12)
    security create-keychain -p ${KEYCHAIN_PASSWORD} build.keychain
    security default-keychain -s build.keychain
    security unlock-keychain -p ${KEYCHAIN_PASSWORD} build.keychain

    echo "Importing certificate into keychain..."
    security import ${CERTIFICATE_P12_FILE} -k build.keychain \
        -P ${APPLE_CERTIFICATE_PASSWORD} -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s \
        -k ${KEYCHAIN_PASSWORD} build.keychain

    rm -rf ${CERTIFICATE_P12_FILE}
fi
