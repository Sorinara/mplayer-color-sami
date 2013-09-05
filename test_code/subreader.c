#define _GNU_SOURCE   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include "subreader_sami.h"

#define TAG_START           0
#define TAG_START_PROPERTY  1
#define TAG_END             2

#define SAMI_LINE_MAX       512
#define TAG_PROPERTY_MAX    256
#define TAG_STACK_MAX       32

char *subtitle_line[5000][16] = {{NULL}};

typedef struct _Tag_Property{
    char *name,
         *value;
} Tag_Property;

typedef struct _Tag{
    char *name;
    int property_count;

    Tag_Property property[TAG_PROPERTY_MAX];
} Tag;

char* next_delimiter(char *start, char delminiter)
{
    char *next;

    next = start;

    while((next = strchr(next, delminiter)) != NULL){

        if(*(next - 1) != '\\'){
           break; 
        }

        next ++;
    }

    return next;
}

int strstripdup(char *string, char **strip_string)
{
    int i = 0,
        start_index,
        end_index,
        string_length,
        strip_string_length;

    string_length = strlen(string);

    // delete white space
	for(i = 0;i < string_length;i ++){
        if(isspace(string[i]) == 0){
            break;
        }
    }

    start_index = i;

    // search non white space end
	for(i = 0;i < string_length;i ++){
        if(isspace(string[i]) != 0){
            break;
        }
    }

    end_index    = i;

    // delete white space
	for(i = 0;i < string_length;i ++){
        if(isspace(string[i]) == 0){
            break;
        }
    }

    strip_string_length = end_index - start_index;
    *strip_string       = strndup(string + start_index, strip_string_length);
    return 0;
}

int strmdup(char *string, const char *start_string, const char *end_string, char **middle_string, char **next)
{
    char *start_string_po,
         *end_string_po;
    int middle_string_size = 0;

    if(start_string == NULL){
        start_string_po = string;
    }else{
        if((start_string_po = strstr(string, start_string)) == NULL){
            return -1;
        }
        // eat start string_
        start_string_po += strlen(start_string);
    }

    if(end_string == NULL){
        end_string_po = string + strlen(string);
    }else{
        if((end_string_po = strstr(start_string_po, end_string)) == NULL){
            return -2;
        }
    }

    middle_string_size = end_string_po - start_string_po;

    if(middle_string != NULL){
        *middle_string = strndup(start_string_po, middle_string_size);
    }

    if(next != NULL){
        // eat start string_
        *next = end_string_po + strlen(end_string);
    }

    // do not eat "start_string"
    return middle_string_size;
}

int strmstrip(char *string, const char *start_string, const char *end_string, char **middle_string, char **next)
{
    char *start_po,
         *end_po,
         *string_valid;
    int string_valid_length = 0;

    start_po = string;
	while(isspace(*start_po) != 0) ++start_po;

    end_po = start_po + strlen(start_po);

    // delete end space character
	while(isspace(*end_po) != 0) --end_po;

    string_valid_length = end_po - start_po;
    string_valid = strndup(start_po, string_valid_length);
    strmdup(string_valid, start_string, end_string, middle_string, next);
    free(string_valid);

    return 0;
    //return strmdup(string, start_string, end_string, middle_string, next);
}

void sami_tag_stack_free(Tag stack)
{
    int i;

    free(stack.name);

    for(i = 0;i < stack.property_count;i ++){
        free(stack.property[i].name);
        free(stack.property[i].value);
    }
}

void sami_tag_stack_free_all(Tag **stack, unsigned int stack_max_size)
{
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        sami_tag_stack_free(*(stack[i]));

        free(stack[i]);
        stack[i]= NULL;
    }
}

void sami_tag_stack_print(Tag **stack, unsigned int stack_max_size)
{
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        printf("ST(%d): %s\n", i, stack[i]->name);
    }

    if(i == 0){
        printf("Stack Empty\n");
    }

    if(i + 1 == stack_max_size){
        printf("Stack Full\n");
    }
}

int sami_tag_stack_top(Tag **stack, unsigned int stack_max_size)
{
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }
    }

    // -1 : empty
    if(i == 0){
        return -1;
    }

    // -2 : full
    if(i + 1 == stack_max_size){
        return -2;
    }

    return --i;
}

int sami_tag_stack_push(Tag **stack, unsigned int stack_max_size, Tag element)
{
    int stack_top;

    // check overflow
    if((stack_top = sami_tag_stack_top(stack, stack_max_size)) == -2){
        return -1;
    }

    stack[stack_top + 1] = (Tag*)calloc(1, sizeof(Tag));
    memcpy(stack[stack_top + 1], &element, sizeof(Tag));
    return 0;
}

int sami_tag_stack_pop(Tag **stack, unsigned int stack_max_size, Tag *element)
{
    int stack_top;

    // check underflow
    if((stack_top = sami_tag_stack_top(stack, stack_max_size)) == -1){
        return -1;
    }

    memcpy(element, stack[stack_top], sizeof(Tag));
    free(stack[stack_top]);

    stack[stack_top] = NULL;
    return 0;
}

int sami_tag_value_get(const char *property_name, char *head_po, char **value_parse, char **last_po)
{
    char *start_po,
         *end_po;
    int valid_string_length = 0;

    // delete first white space
    start_po = head_po;
	while(isspace(*start_po) != 0) ++start_po;

    // "font" tag, "color" or "face" property
    switch(*start_po){
        case '"' :
            start_po   += 1;
            if((end_po = next_delimiter(start_po, '"')) == NULL){
                return -1;
            }
            break;
    }

    if(strcasecmp(property_name, "color") == 0){
        if(*start_po == '#' ){
            start_po ++;
        }
    }

    // EOP
    *last_po = end_po;

    // white search?!
    // delete end space character
	while(isspace(*end_po) != 0) --end_po;

    valid_string_length = end_po - start_po;
    *value_parse = strndup(start_po, valid_string_length);

    return 0;
}

int sami_tag_ass_colors(const char *color_name, char *color_buffer)
{
    char *color_name_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN]  = SAMI_COLORSET_ARRAY_NAME;
    unsigned int color_value_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN] = SAMI_COLORSET_ARRAY_VALUE;
    char first_char;
    int table_number,
        i,
        color_value = -1;
    
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

void sami_tag_ass(char *ass_tag_buffer, const char *font_face, const char *color_value, const char *text)
{
    // must be init 0
    char color_value_buffer[7]      = {0},
         color_rgb_buffer[7]        = {0},
         *strtol_po;
    unsigned int color_rgb_number   = 0;
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

int sami_tag_parse_property(Tag *tag_data, char *tag_body, char *tag_name)
{
    char *property_name,
         *value,
         *property_po               = tag_body,
         *property_name_last_po     = NULL,
         *property_value_last_po    = NULL;
    int property_count   = 0;

    // 먼저 태그명 부터 복사하자
    tag_data->name  = strdup(tag_name);

    // <font face='abc' color="0x123456">
    while(*property_po != '\0'){
        // get property (face/color)
        if(strmstrip(property_po, NULL,  "=",  &property_name, &property_name_last_po) < 0 ){
            // free 
            return -1;
        }

        sami_tag_value_get(property_name, property_name_last_po, &value, &property_value_last_po);
        tag_data->property[property_count].name  = property_name;
        tag_data->property[property_count].value = value;
        //printf("PRP: '%s' '%s'\n", tag_data->property[property_count].name, tag_data->property[property_count].value);
        property_po = property_value_last_po;
        property_count ++;
    }

    tag_data->property_count = property_count;
    return 0;
}

int sami_tag_get(char *tag_head, char **tag_body, char **last_po)
{
    // get all tag
    if(strmdup(tag_head, "<", ">", tag_body, last_po) < 0){
        return -1;
    }

    if(strcmp(*tag_body, "") == 0){
        return 1;
    }

    return 0;
}

// return 0 : start tag => <font>
// return 1 : end   tag => </font>
int sami_tag_name_get(char *tag_body, char **tag_name, char **last_po)
{
    int tag_type;

    // first charecter is '/' -> end(close) tag (</font>)
    if(*tag_body == '/'){
        strstripdup(tag_body + 1, tag_name);
        *last_po    = NULL;
        tag_type    = TAG_END;
    // get tag name "<font color="blue">" (have property)
    }else if(strmdup(tag_body, NULL, " ", tag_name, last_po) > 0){

        // delete space
        while(**(last_po++) == ' ');
        tag_type = TAG_START_PROPERTY;
    // get tag_name "<ruby>" (not have property)
    }else{
        strstripdup(tag_body, tag_name);
        *last_po    = NULL;
        tag_type = TAG_START;
    }

    return tag_type;
} 

// return 0 : no end tag
// return 1 : end tag
int sami_tag_check(const char *tag_push_name, const char *tag_pop_name, int tag_type)
{
    // dismatch tag_push_name
    if((strcasecmp(tag_push_name, tag_pop_name)) == 0 ){
        return 0;
    }

    return 1;
}

int sami_tag_font_property_get(Tag tag, char **font_face_buffer, char **font_color_buffer)
{
    int i,
        ret = 0;

    for(i = 0;i < tag.property_count;i ++){
        if(strcasecmp(tag.property[i].name, "face") == 0){
            *font_face_buffer = strdup(tag.property[i].value);
            ret |= 1;
        }else if(strcasecmp(tag.property[i].name, "color") == 0){
            *font_color_buffer = strdup(tag.property[i].value);
            ret |= 2;
        }
    }

    return ret;
}

int sami_tag_parse(Tag **tag_stack, char *font_tag_string, char **sami_ass_text)
{
    // 이해 데이터들은 초기화 되어야 함.
    char *tag_body,
         *tag_name,
         text[SAMI_LINE_MAX / 2]            = {0},
         ass_buffer[SAMI_LINE_MAX * 2]      = {0},
         ass_buffer_cat[SAMI_LINE_MAX * 3]  = {0},
         *tag_font_face,
         *tag_font_color,
         *tag_po      = font_tag_string,
         *tag_last_po = font_tag_string,
         *tag_name_last_po;
    int  tag_type           = 0,
         tag_property_type  = 0,
         tag_text_index     = 0,
         stack_index;
    Tag  tag_push,
         tag_pop;

    //printf("Line    : %s\n", tag_po);
    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<':
                if(sami_tag_get(tag_po, &tag_body, &tag_last_po) != 0){
                    // '>'로 닫히지 않는 태그 - 현재 태그 무시함
                    fprintf(stderr, "ERROR Tag '%s'\n", tag_po);
                    break;
                }

                if((tag_type = sami_tag_name_get(tag_body, &tag_name, &tag_name_last_po)) < 0){
                    fprintf(stderr, "ERROR Tag name\n");
                    tag_po = tag_last_po;
                    free(tag_body);
                    break;
                }

                // if not <fomt> -> ignore
                if(strcasecmp(tag_name, "font") != 0){
                    tag_po = tag_last_po;
                    free(tag_name);
                    free(tag_body);
                    break;
                }

                switch(tag_type){
                    case TAG_START_PROPERTY:
                        if(sami_tag_parse_property(&tag_push, tag_name_last_po, tag_name) < 0){
                            free(tag_name);
                            free(tag_body);
                            fprintf(stderr, "Tag Property Error\n");
                            break;
                        } // no break!
                    case TAG_START:
                        //puts("Stack push - Before");
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        sami_tag_stack_push(tag_stack, TAG_STACK_MAX, tag_push);
                        //printf("After (add '%s')\n", tag_name);
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        break;
                    case TAG_END:
                        //puts("Stack pop - Before");
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        if(sami_tag_stack_pop(tag_stack, TAG_STACK_MAX, &tag_pop) < 0){
                            fprintf(stderr, "Under flow\n");
                            break;
                        }
                        //printf("After (delete '%s')\n", tag_name);
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        //바로 이전태그의 시작과 지금태그(끝)이 같아야 함.
                        if(sami_tag_check(tag_name, tag_pop.name, tag_type) < 0){
                            sami_tag_stack_free(tag_pop);
                            printf("Not match tag sequence : '%s' '%s'\n", tag_push.name, tag_pop.name);
                            exit(300);
                            break;
                        }
                        
                        // color 정보만 가져옴 (rt, face는 아직...)
                        tag_property_type = sami_tag_font_property_get(tag_pop, &tag_font_face, &tag_font_color);
                        sami_tag_ass(ass_buffer, tag_font_face, tag_font_color, text);
                        strcat(ass_buffer_cat, ass_buffer);

                        if((tag_property_type & 0x01) == 1){
                            free(tag_font_face);
                        }

                        if((tag_property_type & 0x02) == 1){
                            free(tag_font_color);
                        }

                        sami_tag_stack_free(tag_pop);
                        memset(&text, 0, sizeof(text));
                        tag_text_index  = 0;

                        // if text is empty, go last
                        /*
                        if(strcmp(text, "") != 0){
                            printf("'%s' => face : '%s', color : '%s'\n", text, tag_font_face, tag_font_color);
                            printf("'%s'\n", text, ass_buffer);
                        } */
                        break;
                }

                // do not move free position "tag_body"
                free(tag_body);
                free(tag_name);
                tag_po = tag_last_po;
                break;
            default :
                // no include tag, accure in buffer
                text[tag_text_index++] = *tag_po++;
                break;
        }
    }

    // for no tag string
    //puts("- BR Line -");
    if(strcmp(text, "") != 0 ){
        if((stack_index = sami_tag_stack_pop(tag_stack, TAG_STACK_MAX, &tag_pop)) >= 0){
            tag_property_type = sami_tag_font_property_get(tag_pop, &tag_font_face, &tag_font_color);
            sami_tag_ass(ass_buffer, tag_font_face, tag_font_color, text);
            if((tag_property_type & 0x01) == 1){
                free(tag_font_face);
            }

            if((tag_property_type & 0x02) == 1){
                free(tag_font_color);
            }

            sami_tag_stack_free(tag_pop);
        }else{
            sprintf(ass_buffer, "%s", text);
        }

        strcat(ass_buffer_cat, ass_buffer);
        //printf("k : '%s' -> '%s'\n", text, ass_buffer);
    }

    *sami_ass_text = strdup(ass_buffer_cat);
    return 0;
}

int Subtitle_Devide(const char *sami_filepath, int *sync_time_line_max)/*{{{*//*}}}*/
{/*{{{*/
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
}/*}}}*/

int main(int argc, char *argv[])
{
    int i,
        j,
        sync_time_line_max = -1;
    Tag *tag_stack[TAG_STACK_MAX] = {NULL};
    char *sami_ass_text;

    Subtitle_Devide(argv[1], &sync_time_line_max);

    for(j = 0;j < sync_time_line_max;j ++){
        for(i = 0; subtitle_line[j][i] != NULL;i ++){
            sami_tag_parse(tag_stack, subtitle_line[j][i], &sami_ass_text);
            printf("%s\n", sami_ass_text);
            free(sami_ass_text);
            free(subtitle_line[j][i]);
        }
        sami_tag_stack_free_all(tag_stack, TAG_STACK_MAX);
    }

    return 0;
}
