function Subtitle_Line_Scope_Get()
{
    local Parse_filepath="$1"
    local Parse_result_filepath="$2"
    local Temp_filepath="/tmp/Subtitle_Line_Valid_$$"
    local Subtitle_line_valid_start=0
    local Subtitle_line_valid_end=0

    fgrep -n -i '<sync start' "$Parse_filepath" > "$Temp_filepath"

    if [ ! -s "$Temp_filepath" ];then
        rm -rf "$Temp_filepath"
        return 1
    fi

    Subtitle_line_valid_start="$(head -1 "$Temp_filepath" | cut -d ':' -f1)"
    Subtitle_line_valid_end="$(tail   -1 "$Temp_filepath" | cut -d ':' -f1)"

    sed -n "$Subtitle_line_valid_start"','"$Subtitle_line_valid_end"'p' "$Parse_filepath" > "$Parse_result_filepath"

    rm -rf "$Temp_filepath"
    return 0
}

function Subtitle_Line_Get()
{
    local Sami_filepath="$1"
    local Subtitle_Line_filepath="$2"
    local Subtitle_default_locale="euckr"
    local Parse_valid_filepath="/tmp/Subtitle_Line_Get_$$"
    local Parse_filepath="/tmp/Subtitle_Line_Iconv_$$"
    local Line=""
    local Sync_tag_flag=2
    local Sync_tag_old_flag=2

    if [ ! -f "$Sami_filepath" ];then
        return 1
    fi

    if [ -n "$(file "$Sami_filepath" | grep "UTF-16")" ];then
        Subtitle_default_locale="utf16"
    fi

    # Change Sami Locale (EUCKR -> UTF8)
    iconv -c -f"$Subtitle_default_locale" -tutf8 "$Sami_filepath" | perl -pe 's/\&nbsp\;//g,s/^\s*$//g,s///g' > "$Parse_filepath"

    # get "<Sync Start" Min/Max Line
    Subtitle_Line_Scope_Get "$Parse_filepath" "$Parse_valid_filepath"
    if [ $? != 0 ];then
        rm -rf "$Parse_filepath"
        return 2
    fi

    rm -rf "$Subtitle_Line_filepath" "$Parse_filepath" 

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
    done < "$Parse_valid_filepath"
    rm -rf "$Parse_valid_filepath"

    return 0
}
