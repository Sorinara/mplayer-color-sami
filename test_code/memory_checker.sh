DIRECTORY_PATH="$1"
SUBTITLE_FILELIST="/tmp/valgrind_test_filelist_$"
BINARY_FILEPATH="./subreader"
DEBUGER_LOG_FILEPATH="/tmp/valgrind_dbg_log_$$"
ERROR_FILEPATH=""

rm -rf "/tmp/valgrind_dbg_log_*"

if [ -z "$DIRECTORY_PATH" ];then
    echo 'insert directory path'
    exit 1
fi

find "$DIRECTORY_PATH" -iname '*.smi' > "$SUBTITLE_FILELIST"

while read LINE;do
    echo -n "Check "$(basename "$LINE")" ..."

    valgrind -v --leak-check=full --show-reachable=yes "$BINARY_FILEPATH" "$LINE" >/dev/null 2>"$DEBUGER_LOG_FILEPATH"

    if [ -z "$(fgrep 'ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)' "$DEBUGER_LOG_FILEPATH")" ];then
        ERROR_FILEPATH="$LINE"
        echo "[FAILED]"
        break
    else
        rm -rf "$DEBUGER_LOG_FILEPATH"
        echo "[OK]"
    fi
done < "$SUBTITLE_FILELIST"

if [ -f "$DEBUGER_LOG_FILEPATH" ];then
    vim "$DEBUGER_LOG_FILEPATH"
fi

echo "ERROR FILE IS "$LINE""
rm -rf "$SUBTITLE_FILELIST"
