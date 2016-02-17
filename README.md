ass기능을 사용해서 sami자막에서도 컬러자막이 가능하도록 해 주는 패치입니다.

-패치방법

1, applay_patch.sh 파일의 TARGET_MPLAYER_DIRECTORY 항목에 mplayer가 설치된 디렉토리 경로를 삽입.
2, ~/.mplayer/config에 해당 내용(ass, ass-color)이 있어야 작동합니다.

ass=1
ass-color=0xFFFFFF00

당장, smi태그의 색깔/루비태그를 지원하는 플레이어를 리눅스에서 사용하시려면, totem을 사용해 보세요)
(아직 작성중이며 완전하지 못합니다. 향후 루비태그도 지원 예정)

수정: 2014, 11 일부 태그 오류 수정.
수정: 2014, 11 폰트 태그 적용.
수정: 2016, 02 mplayer-1.3.0 버전에 맞게 수정. 패치스크립트 작성.
