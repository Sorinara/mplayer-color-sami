mplayer의 ass기능을 사용해서 sami자막에서도 컬러자막이 가능하도록 해 주는 패치입니다.
mplayer 1.1.1 버전 기준으로, subreader.diff파일을 sub/subreader.c에 패치해 주세요.

~/.mplayer/config에 해당 내용이 있어야 작동합니다.
--------------------
ass=1
ass-color=0xFFFFFF00
--------------------

당장, smi태그의 색깔/루비태그를 지원하는 플레이어를 리눅스에서 사용하시려면, totem을 사용해 보세요)
(아직 작성중이며 완전하지 못합니다. 향후 루비태그도 지원 예정.)
