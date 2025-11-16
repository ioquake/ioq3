#!/bin/sh

set -e

if [ -z "${APPLE_NOTARIZATION_USERNAME}" ]
then
    echo "No notarization credentials supplied, skipping..."
    exit 0
fi

echo "Creating NotarizationProfile..."
xcrun notarytool store-credentials --apple-id "${APPLE_NOTARIZATION_USERNAME}" \
    --password "${APPLE_NOTARIZATION_PASSWORD}" \
    --team-id "${APPLE_TEAM_ID}" "NotarizationProfile"

if [ "$#" -eq 0 ]
then
    echo "Error: Please provide one or more .dmg files"
    exit 1
fi

for FILE in "$@"; do
    case ${FILE} in
        *.dmg)
            if [ ! -f "${FILE}" ]
            then
                echo "Error: '${FILE}' does not exist or is not a regular file"
                exit 1
            fi

            echo "Submitting notarization request..."
            xcrun notarytool submit "${FILE}" \
                --keychain-profile "NotarizationProfile" --wait

            echo "Stapling..."
            xcrun stapler staple "${FILE}"
            ;;

        *)
            echo "Error: '${FILE}' does not have a .dmg extension"
            exit 1
            ;;
    esac
done
