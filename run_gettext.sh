#!/bin/bash

# CrealityPrint gettext
# Created by SoftFever on 27/5/23.

# Check for --full argument
FULL_MODE=0
for arg in "$@"; do
    if [ "$arg" == "--full" ]; then
        FULL_MODE=1
    fi
done

src_dir="$(pwd)/src"
filter_dir="$(pwd)/"
TEMP_FILE="$(pwd)/localization/i18n/list.txt"

if [ $FULL_MODE -eq 1 ]; then
    echo "$(pwd)/src"
    echo "src_dir: $src_dir"
    
    # Create a temporary file to store the list of files
    [ -f "$TEMP_FILE" ] && rm "$TEMP_FILE"
    
    # Traverse src directory and find .cpp and .hpp files
    find "$src_dir" -type f \( -name "*.cpp" -o -name "*.hpp" \) | while read -r file; do
        relativePath="${file#$filter_dir}"
        relativePath="${relativePath#/}"
        relativePath="${relativePath//\\//}"
        echo "$relativePath" >> "$TEMP_FILE"
    done
    
    echo "Get the list of files to .pot"
    xgettext --keyword=L --keyword=_L --keyword=_u8L --keyword=_L_ZH --keyword=L_CONTEXT:1,2c --keyword=_L_PLURAL:1,2 --add-comments=TRN --from-code=UTF-8 --no-location --debug --boost -f "$TEMP_FILE" -o ./localization/i18n/CrealityPrint.pot
    #build/src/hints/Release/hintsToPot ./resources ./localization/i18n
fi

# Print the current directory
echo "$(pwd)"

processFile() {
    file="$1"
    dir="$(dirname "$1")/"
    name="$(basename "$1")"
    lang_tmp="${name#*_}"
    lang="${lang_tmp%.po}"
    part="$2"
    web="$3"
    echo "file:${file}"
    echo "dir:${dir}"
    echo "name:${name}"
    echo "lang:${lang}"
    echo "part:$part"
    if [ $FULL_MODE -eq 1 ]; then
        msgmerge -N -o "$file" "$file" "$pot_file"
    fi
    
    [ ! -d "./resources/i18n/$lang" ] && mkdir -p "./resources/i18n/$lang"
    
    if [ "$web" == "1" ]; then
        po2json "$file" "./resources/i18n/$lang/$part.json"
        echo "processFile: ./resources/i18n/$lang/$part.json"
    else
        msgfmt --check-format -o "./resources/i18n/$lang/$part.mo" "$file"  || exit 1
        echo "processFile: ./resources/i18n/$lang/$part.mo"
    fi
}

# Run the script for each .po file
pot_file="./localization/i18n/CrealityPrint.pot"
echo "Run the script for each .po file"
find "./localization/i18n/" -name "CrealityPrint*.po" | while read -r file; do
    processFile "$file" "CrealityPrint"
done

