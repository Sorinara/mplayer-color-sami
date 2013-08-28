#define _GNU_SOURCE   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include "subreader_sami.h"

#define LINE_LEN 512

char *subtitle_line[5000][16] = {NULL};

int strmget(char *string, const char *start_string, const char *end_string, char *middle_string_buffer, char **next)
{
    char *start_string_p,
         *end_string_p;
    int middle_string_size = 0;

    if(start_string == NULL){
        start_string_p = string;
    }else{
        if((start_string_p = strstr(string, start_string)) == NULL){
            return -1;
        }
        // eat start string_
        start_string_p += strlen(start_string);
    }

    if(end_string == NULL){
        end_string_p = string + strlen(string);
    }else{
        if((end_string_p= strstr(start_string_p, end_string)) == NULL){
            return -2;
        }
    }

    middle_string_size = end_string_p - start_string_p + 1;

    if(middle_string_buffer != NULL){
        snprintf(middle_string_buffer, middle_string_size, "%s", start_string_p);
    }

    if(next != NULL){
        // eat start string_
        *next = end_string_p + strlen(end_string);
    }

    // do not eat "start_string"
    return middle_string_size;
}

void sami_tag_value_parse(char *save_buffer, char *value)
{
    char *start,
         *end;
    int value_length = 0,
        valid_string_length = 0;

    value_length = strlen(value);
    start        = value;
    end          = start + value_length;

    switch(*value){
        case '"' :
            start += 1;
            end   -= 1;
            if(*start == '#' ){
                start++;
            }
            break;
        default :
            break;
    }

    valid_string_length = end - start + 1;
    snprintf(save_buffer, valid_string_length, "%s", start);
}

int sami_tag_ass_colors(const char *color_name, char *color_buffer)
{
    char *color_name_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN]  = SAMI_COLORSET_ARRAY_NAME;
    unsigned int color_value_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN] = SAMI_COLORSET_ARRAY_VALUE;
    char first_char;
    int table_number,
        i;
    int color_value = -1;
    
    first_char      = *color_name;
    table_number    = (char)tolower(first_char) - 'a';

    for(i = 0;color_name_table[table_number][i] != NULL;i ++){
        if(strcasecmp(color_name, color_name_table[table_number][i]) == 0){
            color_value = color_value_table[table_number][i];
            break;
        }
    }

    if(color_value > 0 ){
        sprintf(color_buffer, "%06x", color_value);
    }

    return color_value;
}

void sami_tag_ass(char *ass_tag_buffer, const char *font_name, const char *color_value, const char *text)
{
    // must be init 0
    char color_value_buffer[7]      = {0},
         color_rgb_buffer[7]        = {0};
    unsigned int color_rgb_number   = 0;
    char *strtol_po;
    int color_value_length          = 0,
        color_value_length_valid    = 0;

    // 앞부분 color가 A-F이면 문제가 발생하니까, 
    // 리턴값으로 결과를 확인하는게 아니라, 처리된 갯수로 확인해야 함.
    strtol(color_value, &strtol_po, 16);
    color_value_length          = strlen(color_value);
    color_value_length_valid    = strtol_po - color_value;

    if(color_value_length != color_value_length_valid){
        sami_tag_ass_colors(color_value, color_value_buffer);
    }else{
        sprintf(color_value_buffer, color_value);
    }

    if(strcmp(color_value_buffer, "") != 0){
        // sami자막의 color순서(RGB)와 ass간의 color(BGR)순서가 다르다.
        memcpy(color_rgb_buffer + 0, color_value_buffer + 4, 2);
        memcpy(color_rgb_buffer + 2, color_value_buffer + 2, 2);
        memcpy(color_rgb_buffer + 4, color_value_buffer + 0, 2);
        color_rgb_number = strtol(color_rgb_buffer, NULL, 16);

        snprintf(ass_tag_buffer, 16, "{\\r\\c&H%06X&}", color_rgb_number);
        sprintf(ass_tag_buffer + 15, "%s{\\r}", text);
    }else{
        sprintf(ass_tag_buffer, "%s", text);
    }
}

void sami_tag_parse_property_font(char *tag_property_buffer)
{
    while(1){
        // <font face='abc' color="0x123456">
        // get property (face/color)
        if(strmget(tag_body, " ",  "=",  tag_property, NULL) < 0 ){
            break;
        }

        // face, color
        if((strcasecmp(tag_property_buffer, "face")) == 0){
            sami_tag_value_parse(tag_font_face, tag_value);
        }

        // get value (font name or color rgb)
        if(strmget(tag_body, "=",  " ", tag_value,    NULL) < 0){
            if(strmget(tag_body, "=",  NULL, tag_value,    NULL) < 0){
                break;
            }
        }
    }
}

int sami_tag_parse_get(char *tag_po, char *tag_body_buffer, char **last_po)
{
    // get all tag
    strmget(tag_po, "<", ">", tag_body_buffer, last_po);

    if(strcmp(tag_body_buffer, "") == 0){
        return 1;
    }

    return 0;
}
/*
int sami_tag_parse_name_get(char *tag_body_buffer, char *tag_name_buffer, char *last_po)
{
    // get tag name (font)
    strmget(tag_body, NULL, " ", tag_name, last_po);
} 

int sami_tag_parse_start(char *tag_name_source, char *tag_name_target, int *tag_start_flag)
{
    if((strcasecmp(tag_name_source, tag_name_target)) == 0 ){
        *tag_start_flag = 1;
        return 0;
    }

    // printf("Body       : %s\n", tag_body);
    // printf("Name       : %s\n", tag_name_source);
    // printf("Property   : %s\n", tag_property_buffer);
    // printf("Value      : %s\n", tag_value);
    // printf("Next       : %s\n\n", s);
    return 1;
} */

// return 0 : no end tag
// return 1 : end tag
int sami_tag_check(char *tag_body, char *tag_name)
{
    // first charecter is '/' -> end(close) tag
    if(*tag_body != '/'){
        return 0;
    }

    // dismatch tag_name
    if((strcasecmp(tag_body + 1, tag_name)) != 0 ){
        return 0;
    }

    return 1;
}

int sami_tag_parse(char *font_tag_string)
{
    // 이해 데이터들은 반드시 초기화 되어야 함.
    char tag_body[LINE_LEN]                 = {0},
         tag_name[16]                       = {0},
         tag_property[256]                  = {0},
         tag_value[256]                     = {0},
         tag_text[LINE_LEN / 2]             = {0},
         ass_buffer_cat_temp[LINE_LEN * 3]  = {0};
    int  tag_text_index = 0;
    char *tag_po = font_tag_string,
         *last_po;

    char *input_array[]  = {"font", "face", "color", 0};
    char *output_array[] = {"font", "face", "color", 0};
    static char tag_font_face_old[32],
                tag_font_color_old[9];
    char *tag_property_p;

    // input  {"font", "face", "color", 0}
    // output {0,      "굴림", "0x928374, "텍스트입니다"
    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<' :
                if(sami_tag_parse_get(tag_po, tag_body, &last_po) != 0){
                    break;
                }

                printf("%s-%s\n", font_tag_string, tag_body);
                sami_tag_parse_property_font(tag_body);

                switch(sami_tag_check(tag_body, input_array[0]);
                    // 값 넣기
                    case 0:
                        sami_tag_ass(ass_buffer, tag_font_face, tag_font_color, tag_text);
                        continue;
                    // 초기화
                    case 1:
                        memset(&tag_font_face_old, 0, sizeof(tag_font_face_old));
                        memset(&tag_font_color_old, 0, sizeof(tag_font_color_old));
                        memset(&tag_font_face, 0, sizeof(tag_font_face));
                        memset(&tag_font_color, 0, sizeof(tag_font_color));
                        memset(&tag_text, 0, sizeof(tag_text));
                        tag_text_index  = 0;
                        tag_start_flag  = 0;
                        tag_end_flag    = 0;
                        continue;
                }

                /*
                sami_tag_parse_name_get(tag_body, tag_name);

                if(sami_tag_parse_start(tag_name, input_array[0], &font_start_flag) != 0){
                    break;
                } */
                break;
            default :
                // no include tag, accure in buffer
                tag_text[tag_text_index++] = *last_po++;
                break;
        }
        tag_po++;
    }

    // for no tag string
    if(strcmp(tag_text, "") != 0){
        sami_tag_ass(ass_buffer_cat_temp, tag_font_face, tag_font_color, tag_text);
    }

    //free(font_tag_string);
    //*ass_buffer_cat = strdup(ass_buffer_cat_temp);
    return return_value;
}

int Subtitle_Devide(const char *sami_filepath, int *sync_time_line_max)
{
    FILE *fp;
    int br_line,
        br_length,
        sync_time_line;
    char line_buf[1024],
         *br_start_po,
         *br_end_po;

    if((fp = fopen(sami_filepath, "r")) == NULL){
        perror("Subtitle Open Error");
        return -1;
    }

    sync_time_line = 0;

    while(fgets(line_buf, sizeof(line_buf), fp) != NULL){
        br_start_po = line_buf;
        br_end_po   = NULL;
        br_line     = 0;

        // if exist br tag
        while((br_end_po = strcasestr(br_start_po, "<br>")) != NULL){
            br_length = br_end_po - br_start_po;

            subtitle_line[sync_time_line][br_line] = strndup(br_start_po, br_length);

            br_start_po = br_end_po + strlen("<br>");
            br_line++;

        }
        
        // last line
        subtitle_line[sync_time_line][br_line] = strndup(br_start_po, strlen(br_start_po) - 1);
        sync_time_line ++;
    }

    *sync_time_line_max = sync_time_line;
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    int i,
        j,
        sync_time_line_max = -1;

    Subtitle_Devide(argv[1], &sync_time_line_max);

    for(j = 0;j < sync_time_line_max;j ++){
        for(i = 0; subtitle_line[j][i] != NULL;i ++){
            //printf("%d %s\n", i, subtitle_line[j][i]);
            sami_tag_parse(subtitle_line[j][i]);
        }
    }

    return 0;
}
