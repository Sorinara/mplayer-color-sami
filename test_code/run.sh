SAMI_FILEPATH="$1"
TAG_MARKER='<Sync Start=' 
RESULT_FILEPATH="./result_$$"
SOURCE_FILEPATH="./subreader.c"
BINARY_FILEPATH="./subreader"

#1, 소스 컴파일
gcc -o "$BINARY_FILEPATH" "$SOURCE_FILEPATH"
if [ $? != 0 ];then
    exit 1
fi

#2, (이 스크립트의 첫번째 파라미터) sami파일에서 자막부분(+TAG)만 빼온 임시파일 생성 (SYNC시간은 안들어감!!!)
iconv -feuckr -tutf8 "$SAMI_FILEPATH" | fgrep -A1 "$TAG_MARKER" | fgrep -v "$TAG_MARKER" | perl -pe 's/\&nbsp\;//g,s/^\s*$//g' > "$RESULT_FILEPATH"

#3, 2에서 생성된 임시파일을 바이너리의 첫번째 파리미터에 넣어서 실행
#4, 결과적으로 테스트코드에서는 mplayer에서 같은 타입의 데이터를 읽게 됨.
# (바이너리 내부에서 BR태그를 정리해야함)
"$BINARY_FILEPATH" "$RESULT_FILEPATH"
