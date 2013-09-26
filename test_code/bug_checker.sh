DIRECTORY_PATH="$1"

. ./subtitle_line.sh

BINARY_FILEPATH="./subreader"
SOURCE_FILEPATH="./subreader.c"
COMPILE_LOG_FILEPATH="/tmp/memory_checker_compile_log_$$"
SUBTITLE_FILELIST="/tmp/valgrind_test_filelist_$$"
DEBUGER_LOG_FILEPATH="/tmp/valgrind_dbg_log_$$"
SUBTITLE_VALID_LINE_FILEPATH="./result_memory_checker"
ERROR_FILEPATH=""

rm -rf "/tmp/valgrind_dbg_log_*"

if [ -z "$DIRECTORY_PATH" ];then
    echo 'insert directory path'
    exit 1
fi

gcc -Wall -g -o "$BINARY_FILEPATH" "$SOURCE_FILEPATH" 2>"$COMPILE_LOG_FILEPATH"

find "$DIRECTORY_PATH" -iname '*.smi' > "$SUBTITLE_FILELIST"

while read LINE;do
    echo -n "Check "$(basename "$LINE")" ..."

    Subtitle_Line_Get "$LINE" "$SUBTITLE_VALID_LINE_FILEPATH"
    if [ $? != 0 ];then
        echo '[FAILED'
        break
    fi

    valgrind -v --leak-check=full --show-reachable=yes "$BINARY_FILEPATH" "$SUBTITLE_VALID_LINE_FILEPATH" >/dev/null 2>"$DEBUGER_LOG_FILEPATH"

    if [ -z "$(fgrep 'ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)' "$DEBUGER_LOG_FILEPATH")" ];then
        echo ERROR FILE PATH : "$LINE" >> "$DEBUGER_LOG_FILEPATH"
        ERROR_FILEPATH="$LINE"
        echo "[FAILED]"
        break
    else
        rm -rf "$DEBUGER_LOG_FILEPATH" "$SUBTITLE_VALID_LINE_FILEPATH"
        echo "[OK]"
    fi
done < "$SUBTITLE_FILELIST"

if [ -f "$DEBUGER_LOG_FILEPATH" ];then
    vim "$DEBUGER_LOG_FILEPATH"
fi

echo "ERROR FILE IS "$LINE""
rm -rf "$SUBTITLE_FILELIST"
