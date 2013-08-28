SAMI_FILEPATH="$1"

PARSE_FILEPATH="/tmp/mplayer_run_parse_$$"
SUBTITLE_LINE_FILEPATH="./result"
SOURCE_FILEPATH="./subreader.c"
BINARY_FILEPATH="./subreader"

function Subtitle_Line_Get()
{
    local Parse_filepath="$1"
    local Subtitle_Line_filepath="$2"
    local Line=""
    local Sync_tag_flag=2
    local Sync_tag_old_flag=2

    while read Line;do
        case "$Line" in 
            *[Ss][Yy][Nn][Cc]' '[Ss][Tt][Aa][Rr][Tt]* )
                Sync_tag_flag=1
                ;;
            * )
                Sync_tag_flag=0
                ;;
        esac

        if [ "$Sync_tag_flag" = 0 ] && [ "$Sync_tag_old_flag" = 0 ];then
            echo -n "$Line" >> "$Subtitle_Line_filepath"
        elif [ "$Sync_tag_flag" = 0 ] && [ "$Sync_tag_old_flag" = 1 ];then
            echo -n "$Line" >> "$Subtitle_Line_filepath"
        elif [ "$Sync_tag_flag" = 1 ] && [ "$Sync_tag_old_flag" = 0 ];then
            echo >> "$Subtitle_Line_filepath"
        fi

        Sync_tag_old_flag="$Sync_tag_flag"
    done < "$Parse_filepath"
}

rm -rf "$SUBTITLE_LINE_FILEPATH"

if [ -z "$SAMI_FILEPATH" ];then
    echo 'Usage :$'"$0" '"Sami filepath"'
    exit 1
fi

# 1, (이 스크립트의 첫번째 파라미터) sami파일에서 자막부분(+TAG)만 빼온 임시파일 생성 (SYNC시간은 안들어감!!!)
iconv -c -feuckr -tutf8 "$SAMI_FILEPATH" | perl -pe 's/\&nbsp\;//g,s/^\s*$//g,s///g' | egrep -v -i '<sami>|<\/sami\>|<body>|<\/body>' > "$PARSE_FILEPATH"

Subtitle_Line_Get "$PARSE_FILEPATH" "$SUBTITLE_LINE_FILEPATH"
#cat "$SUBTITLE_LINE_FILEPATH"
rm -rf "$PARSE_FILEPATH" 2>/dev/null

# 2, 소스 컴파일
gcc -g -o "$BINARY_FILEPATH" "$SOURCE_FILEPATH"
if [ $? != 0 ];then
    exit 1
fi

# 3, 2에서 생성된 임시파일을 바이너리의 첫번째 파리미터에 넣어서 실행
"$BINARY_FILEPATH" "$SUBTITLE_LINE_FILEPATH"
