DIRECTORY_PATH="$1"

. ./subtitle_line.sh

rm -rf "/tmp/valgrind_dbg_log_*"
BINARY_FILEPATH="./subreader"
SOURCE_FILEPATH="./subreader.c"
COMPILE_LOG_FILEPATH="/tmp/memory_checker_compile_log_$$"
SUBTITLE_FILELIST="/tmp/valgrind_test_filelist_$$"
DEBUGER_LOG_FILEPATH="/tmp/valgrind_dbg_log_$$"
SUBTITLE_VALID_LINE_FILEPATH="./result_memory_checker"
ERROR_FILEPATH=""

function Debug_Valgrind()
{
    local Binary_filepath="$1"
    local Subtitle_valid_line_filepath="$2"
    local Debuger_log_filepath="$3"

    valgrind -v --leak-check=full --show-reachable=yes "$Binary_filepath" "$Subtitle_valid_line_filepath" >/dev/null 2>"$Debuger_log_filepath"
    if [ -z "$(fgrep 'ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)' "$Debuger_log_filepath")" ];then
        return 1
    else
        return 0
    fi
}

if [ -z "$DIRECTORY_PATH" ];then
    echo 'insert directory path'
    exit 1
fi

echo 'Step 1> start compile'
gcc -Wall -g -o "$BINARY_FILEPATH" "$SOURCE_FILEPATH" 2>"$COMPILE_LOG_FILEPATH"
if [ $? != 0 ];then
    echo 'compile failed'
    exit 2
fi

echo -n 'Step 2> select debug mode (v : valgrind, else : normal run) :'
read Mode

if [ "$Mode" == "v" ];then
    DEBUG_FLAG=1
else
    DEBUG_FLAG=0
fi

find "$DIRECTORY_PATH" -iname '*.smi' > "$SUBTITLE_FILELIST"
if [ ! -s "$SUBTITLE_FILELIST" ];then
    echo 'not exist sami file in target directory'
    exit 3
fi

echo 'Step 3> check sami files'

while read LINE;do
    echo -n "Check "$(basename "$LINE")" ..."

    Subtitle_Line_Get "$LINE" "$SUBTITLE_VALID_LINE_FILEPATH"
    if [ $? != 0 ];then
        echo '[FAILED]'
        break
    fi

    if [ "$DEBUG_FLAG" = 1 ];then
        Debug_Valgrind "$BINARY_FILEPATH" "$SUBTITLE_VALID_LINE_FILEPATH" "$DEBUGER_LOG_FILEPATH"
        EXIT_STATUS=$?
    else
        "$BINARY_FILEPATH" "$SUBTITLE_VALID_LINE_FILEPATH" 2>"$DEBUGER_LOG_FILEPATH"
        EXIT_STATUS=$?
    fi

    if [ $EXIT_STATUS = 0 ];then
        rm -rf "$DEBUGER_LOG_FILEPATH" "$SUBTITLE_VALID_LINE_FILEPATH"
        echo "[OK]"
    else
        echo ERROR FILE PATH : "$LINE" >> "$DEBUGER_LOG_FILEPATH"
        ERROR_FILEPATH="$LINE"
        break
    fi
done < "$SUBTITLE_FILELIST"

if [ -f "$DEBUGER_LOG_FILEPATH" ];then
    vim "$DEBUGER_LOG_FILEPATH"
fi

echo "ERROR FILE IS "$LINE""
rm -rf "$SUBTITLE_FILELIST"
