SAMI_FILEPATH="$1"

. ./subtitle_line.sh

COMPILE_LOG_FILEPATH="/tmp/run_compile_log_$$"
DEBUGER_LOG_FILEPATH="/tmp/run_dbg_log_$$"
SUBTITLE_LINE_FILEPATH="./result_run"
BINARY_FILEPATH="./subreader"
SOURCE_FILEPATH="./subreader.c"

if [ -z "$SAMI_FILEPATH" ];then
    echo 'Usage :$'"$0" '"Sami filepath"'
    exit 1
fi

clear

echo -n 'Step 1> Source Compile ...'
gcc -Wall -g -o "$BINARY_FILEPATH" "$SOURCE_FILEPATH" 2>"$COMPILE_LOG_FILEPATH"
if [ -s "$COMPILE_LOG_FILEPATH" ];then
    echo
    cat "$COMPILE_LOG_FILEPATH"
    rm -rf "$COMPILE_LOG_FILEPATH";echo '[Failed]'
    exit 1
fi
rm -rf "$COMPILE_LOG_FILEPATH";echo '[OK]'

echo -n 'Step 2> Parse SAMI File ...'
Subtitle_Line_Get "$SAMI_FILEPATH" "$SUBTITLE_LINE_FILEPATH"
if [ $? != 0 ];then
    echo Error sami file "$SAMI_FILEPATH" '(not exist or not ascii text type' 
    exit 2
fi

#cat "$SUBTITLE_LINE_FILEPATH"
echo '[OK]'

# 3, 3에서 생성된 임시파일을 바이너리의 첫번째 파리미터에 넣어서 실행
echo -n 'Select debug mode (run/v/d) :'
read Option

if [ "$Option" = 'v' ];then
    valgrind -v --leak-check=full --show-reachable=yes "$BINARY_FILEPATH" "$SUBTITLE_LINE_FILEPATH" 2>"$DEBUGER_LOG_FILEPATH"
    if [ -z "$(fgrep 'ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)' "$DEBUGER_LOG_FILEPATH")" ];then
        vim "$DEBUGER_LOG_FILEPATH"
        rm -rf "$DEBUGER_LOG_FILEPATH"
    else
        echo valgrind, No ERROR ALL OK!
    fi
elif [ "$Option" = 'd' ];then
    cgdb "$BINARY_FILEPATH"
else
    "$BINARY_FILEPATH" "$SUBTITLE_LINE_FILEPATH"
fi
