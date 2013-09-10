#define _GNU_SOURCE   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include "subreader_sami.h"

#define TAG_START               0
#define TAG_END                 1

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
    int  property_count;

    Tag_Property property[TAG_PROPERTY_MAX];
} Tag;

char* next_delimiter(char *start, char delminiter)
{/*{{{*/
    char *next;

    next = start;

    while((next = strchr(next, delminiter)) != NULL){
        // next와 start가 같을경우 (맨처음부터 일치하는 경우)
        // 아래 *(next - 1)이 잘못된 참조를 하게되므로 이를 예방하기 위한 식
        if(next == start){
            break;
        }

        if(*(next - 1) != '\\'){
           break; 
        }

        next ++;
    }

    // add 'delminter' size
    return next;
}/*}}}*/

int strstrip(char *string, char **start_po, int *string_strip_length, char **next)
{/*{{{*/
    int i,
        start_index,
        end_index,
        string_length;

    string_length = strlen(string);

    // delete white space
	for(i = 0;i < string_length;i ++){
        if(isspace(string[i]) == 0){
            break;
        }
    }

    start_index = i;

    // search non white space end
	for(;i < string_length;i ++){
        if(isspace(string[i]) != 0){
            break;
        }
    }

    end_index  = i;

    // delete white space
	for(;i < string_length;i ++){
        if(isspace(string[i]) == 0){
            break;
        }
    }

    if(next != NULL){
        *next = string + i;
    }

    if(start_index == end_index){
        return -1;
    }

    *start_po            = string + start_index;
    *string_strip_length = end_index - start_index;

    return 0;
}/*}}}*/

int tagmdup(char *string, char start_char, char end_char, char **middle_string, char **next)
{/*{{{*/
    char *start_string_po,
         *end_string_po;
    int middle_string_size = 0;

    if(start_char == '\0'){
        start_string_po = string;
    }else{
        if((start_string_po = next_delimiter(string, start_char)) == NULL){
            return -1;
        }
        // eat start char 
        start_string_po += sizeof(char);
    }

    if(end_char == '\0'){
        end_string_po = string + strlen(string);
    }else{
        if((end_string_po = next_delimiter(start_string_po, end_char)) == NULL){
            return -2;
        }
    }

    if((middle_string_size = end_string_po - start_string_po) <= 0){
        return -3;
    }

    if(middle_string != NULL){
        *middle_string = strndup(start_string_po, middle_string_size);
    }

    if(next != NULL){
        // eat end char (over end char)
        *next = end_string_po + sizeof(char);
    }

    // do not eat "start_string"
    return middle_string_size;
}/*}}}*/

int tagstripdup(char *string, char **strip_string)
{/*{{{*/
    int  strip_string_length;
    char *start_po;
    
    // DO NOT USE fourth parameter
    if(strstrip(string, &start_po, &strip_string_length, NULL) < 0){
        return -1;
    }

    *strip_string   = strndup(start_po, strip_string_length);
    
    return strip_string_length;
}/*}}}*/

void sami_tag_stack_free(Tag *stack)
{/*{{{*/
    int i;

    free(stack->name);

    for(i = 0;i < stack->property_count;i ++){
        free(stack->property[i].name);
        free(stack->property[i].value);
    }
}/*}}}*/

void sami_tag_stack_free_all(Tag **stack, unsigned int stack_max_size)
{/*{{{*/
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        sami_tag_stack_free(stack[i]);

        free(stack[i]);
        stack[i]= NULL;
    }
}/*}}}*/

int sami_tag_stack_top(Tag **stack, Tag **element, unsigned int stack_max_size)
{
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }
    }

    --i;

    // empty
    if(i == -1){
        return -1;
    }

    // full
    if(i == stack_max_size){
        return -2;
    }

    if(element != NULL){
       // memcpy(element, stack[i], sizeof(Tag));
       *element = stack[i];
    }

    return i;
}

int sami_tag_stack_search(Tag **stack, const char *tag_name, unsigned int stack_max_size) 
{
    int i,
        match_flag = 0;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

       if(strcasecmp(stack[i]->name, tag_name) == 0){
            match_flag = 1;
            break;
       }
    }

    if(match_flag != 1){
        return -1;
    }

    return i;
}

int sami_tag_stack_update(Tag **stack, char *tag_name, int stack_index, Tag *element)
{
    int i,
        j,
        property_count,
        update_flag = 0;

    if(strcasecmp(stack[stack_index]->name, tag_name) != 0){
        return -1;
    }

    stack[stack_index]->name = element->name;

    for(i = 0;i < element->property_count;i ++){
        property_count = stack[stack_index]->property_count;

        for(j = 0;j < property_count;j ++){

            fprintf(stderr, "Test - Property Compare : '%s'('%s') '%s'('%s')\n", element->property[i].name, element->property[i].value, stack[stack_index]->property[j].name, stack[stack_index]->property[j].value);

            // match property, update value
            if(strcasecmp(element->property[i].name,  stack[stack_index]->property[j].name)  == 0 && \
               strcasecmp(element->property[i].value, stack[stack_index]->property[j].value) != 0){
                free(stack[stack_index]->property[j].value);
                stack[stack_index]->property[j].value = element->property[i].value;
                fprintf(stderr, "Test - Property Update  : '%s' '%s'\n", stack[stack_index]->property[j].name, stack[stack_index]->property[j].value);
                update_flag = 1;
            }

            fprintf(stderr, "Test - Property Compare : END\n");
        }

        // not exist, add property
        if(update_flag == 0){
            stack[stack_index]->property[property_count].name  = element->property[i].name;
            stack[stack_index]->property[property_count].value = element->property[i].value;
            fprintf(stderr, "Test - Proerty Add      : '%s' '%s'\n", stack[stack_index]->property[property_count].name, stack[stack_index]->property[property_count].value);
            (stack[stack_index]->property_count) ++;
        }

        update_flag = 0;
    }

    return 0;
}

int sami_tag_stack_push(Tag **stack, Tag element, unsigned int stack_max_size)
{
    int stack_top;

    // check overflow
    if((stack_top = sami_tag_stack_top(stack, NULL, stack_max_size)) == -2){
        return -1;
    }

    stack[stack_top + 1] = (Tag*)calloc(1, sizeof(Tag));
    memcpy(stack[stack_top + 1], &element, sizeof(Tag));
    return 0;
}

int sami_tag_stack_top_remove(Tag **stack, Tag *element, unsigned int stack_max_size)
{
    int stack_top;

    // check underflow
    if((stack_top = sami_tag_stack_top(stack, &element, stack_max_size)) == -1){
        return -1;
    }

    sami_tag_stack_free(stack[stack_top]);
    free(stack[stack_top]);
    stack[stack_top] = NULL;

    return 0;
}

int sami_tag_property_name_get(char *string, char **property_name_strip, char **next)
{/*{{{*/
    char *property_name,
         *property_name_end_po;

    if(tagmdup(string, '\0', '=', &property_name, &property_name_end_po) < 0){
        return -1;
    }

    if(tagstripdup(property_name, property_name_strip) < 0){
        return -2;
    }

    // IF YOU DON'T KNOWH THIS, DO NOT MODIFY
	while(isspace(*property_name_end_po) != 0){
        property_name_end_po ++;
    }

    *next = property_name_end_po;

    //fprintf(stderr, "Test - Property Name : '%s' '%s' '%s'\n", string, *property_name_strip, *next);
    free(property_name);
    return 0;
}/*}}}*/

void sami_tag_stack_print(Tag **stack, unsigned int stack_max_size)
{
    int i,
        j;

    if(stack[0] == NULL){
        fprintf(stderr, "Stack Empty\n");
        return;
    }

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        // stack[i]에서 변수를 참조하는게 에러?!
        fprintf(stderr, "ST(%d): %s\n", i, stack[i]->name);

        for(j = 0;j < stack[i]->property_count;j ++){
            fprintf(stderr, "-Property(%d): %s [%s]\n", j, stack[i]->property[j].name, stack[i]->property[j].value);
        }
    }

    if(i + 1 == stack_max_size){
        fprintf(stderr, "Stack Full\n");
    }
}

int sami_tag_property_value_get(char *string, char **value, char **next_po)
{/*{{{*/
    char *start_po,
         *end_po;
    int  value_length = 0;

    if(string == NULL){
        return -1;
    }

    start_po = string;

    // delete first white space
	while(isspace(*start_po) != 0){
        start_po ++;
    }

    // "font" tag, "color" or "face" property
    switch(*start_po){
        case '\'' :
            start_po   += 1;

            if((end_po = next_delimiter(start_po, '\'')) == NULL){
                if((end_po = next_delimiter(start_po, '"')) == NULL){
                    return -2;
                }
            }

            *next_po = end_po + 1;
            break;
        case '"' :
            start_po   += 1;

            if((end_po = next_delimiter(start_po, '"')) == NULL){
                if((end_po = next_delimiter(start_po, '\'')) == NULL){
                    return -2;
                }
            }

            *next_po = end_po + 1;
            break;
        default:
            if((end_po   = next_delimiter(start_po, ' ')) == NULL){
                end_po   = start_po + strlen(start_po);
                *next_po = end_po;
                break;
            }

            *next_po = end_po;
            break;
    }

    value_length = end_po - start_po;
    *value       = strndup(start_po, value_length);
    //fprintf(stderr, "Test - Property Value : '%s' '%s' '%s' %d -> '%s'\n", string, start_po, end_po, value_length, *value);
    return 0;
}/*}}}*/

int sami_tag_ass_colors(const char *color_name, char *color_buffer)
{/*{{{*/
    char first_char,
         *color_name_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN]  = SAMI_COLORSET_ARRAY_NAME;
    unsigned int color_value_table[SAMI_COLORSET_ARRAY_MAX_ROW][SAMI_COLORSET_ARRAY_MAX_COLUMN] = SAMI_COLORSET_ARRAY_VALUE;
    int color_table_index,
        i,
        color_value,
        color_flag = 0;
    
    first_char = *color_name;

    if(isalpha(first_char) == 0){
        return -1;
    }

    color_table_index = (char)tolower(first_char) - 'a';

    for(i = 0;color_name_table[color_table_index][i] != NULL;i ++){
        if(strcasecmp(color_name, color_name_table[color_table_index][i]) == 0){
            color_value = color_value_table[color_table_index][i];
            sprintf(color_buffer, "%06x", color_value);
            color_flag = 1;
            break;
        }
    }

    if(color_flag == 0){
        return -2;
    }

    return 0;
}/*}}}*/

void sami_tag_ass_color(char *ass_buffer, const char *color_value)
{/*{{{*/
    // must be init 0
    char color_value_buffer[7]      = {0},
         color_rgb_buffer[7]        = {0},
         *strtol_po;
    unsigned int color_rgb_number   = 0;
    int color_value_length          = 0,
        color_value_length_valid    = 0;

    if(*color_value == '#'){
        color_value ++;
    }

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
        snprintf(ass_buffer, 16, "{\\r\\c&H%06X&}", color_rgb_number);
    }
}/*}}}*/

void sami_tag_ass_font(char *ass_buffer, const char *face_value, const char *color_value, const char *text)
{/*{{{*/
    if(color_value != NULL){
        sami_tag_ass_color(ass_buffer, color_value);
        sprintf(ass_buffer + 15, "%s{\\r}", text);
    }else{
        // change to strcat...
        sprintf(ass_buffer, "%s", text);
    }
}/*}}}*/

int sami_tag_parse_property(char *tag_property_start_po, Tag *tag_data)
{/*{{{*/
    char *property_po,
         *property_name,
         *property_name_next_po     = NULL,
         *value,
         *property_value_next_po    = NULL;
    int property_count  = 0,
        ret             = 0;

    if(tag_property_start_po == NULL){
        return -1;
    }

    property_po = tag_property_start_po;

    // <font face='abc' color="0x123456">
    while(*property_po != '\0'){
        // get property name(face/color)
        if(sami_tag_property_name_get(property_po, &property_name, &property_name_next_po) < 0 ){
            ret = -2;
            break;
        }

        // get property value
        if(sami_tag_property_value_get(property_name_next_po, &value, &property_value_next_po) < 0){
            free(property_name);
            ret = -3;
            break;
        }

        tag_data->property[property_count].name  = property_name;
        tag_data->property[property_count].value = value;
        property_count ++;

        property_po = property_value_next_po;
    }

    tag_data->property_count = property_count;

    int i;
    for(i = 0;i < tag_data->property_count;i ++){
        fprintf(stderr, "Test - Property Loop    : '%d' '%s' '%s'\n", i, tag_data->property[i].name, tag_data->property[i].value);
    }

    return ret;
}/*}}}*/

int sami_tag_container_get(char *tag_po, char **tag_container, char **next_po)
{/*{{{*/
    // get all tag
    if(tagmdup(tag_po, '<', '>', tag_container, next_po) < 0){
        return -1;
    }

    return 0;
}/*}}}*/

// return 0 : start tag => <font>
// return 1 : end   tag => </font>
int sami_tag_name_get(char *tag_container, char **tag_name, int *property_flag, char **next_po)
{/*{{{*/
    int tag_type;

    // first charecter is '/' -> end(close) tag (</font>)
    if(*tag_container == '/'){
        if(tagstripdup(tag_container + 1, tag_name) < 0){
            return -1;
        }

        tag_type            = TAG_END;
        *property_flag      = 0;
        *next_po            = NULL;
    // get tag name "<font color="blue">" (have property)
    }else if(tagmdup(tag_container, '\0', ' ', tag_name, next_po) > 0){
        tag_type            = TAG_START;
        *property_flag      = 1;
    // get tag_name "<ruby>" (not have property)
    }else{
        if(tagstripdup(tag_container, tag_name) < 0){
            return -2;
        }

        tag_type            = TAG_START;
        *property_flag      = 0;
        *next_po            = NULL;
    }

    return tag_type;
} /*}}}*/

// return 0 : no end tag
// return 1 : end tag
int sami_tag_check(char *tag_name, char *tag_stack_pop_name)
{/*{{{*/
    // dismatch tag_push_name
    if((strcasecmp(tag_name, tag_stack_pop_name)) != 0 ){
        return -1;
    }

    return 1;
}/*}}}*/

int sami_tag_font_property_get(Tag *tag, char **font_face_buffer, char **font_color_buffer)
{/*{{{*/
    int i,
        ret = 0;

    *font_face_buffer  = NULL;
    *font_color_buffer = NULL;

    for(i = 0;i < tag->property_count;i ++){
        if(strcasecmp(tag->property[i].name, "face") == 0){
            *font_face_buffer = tag->property[i].value;
            ret |= 1;
        }else if(strcasecmp(tag->property[i].name, "color") == 0){
            *font_color_buffer = tag->property[i].value;
            ret |= 2;
        }
    }

    return ret;
}/*}}}*/

int sami_tag_parse(Tag **tag_stack, char *font_tag_string, char **sami_ass_text)
{
    // ass buffer는 초기화 되어야 함.
    char *tag_po,
         *tag_container,
         *tag_next_po,
         *tag_name,
         *tag_property_start_po,
         *stack_pop_face_po,
         *stack_pop_color_po,
         text[SAMI_LINE_MAX / 2]            = {0},
         ass_buffer[SAMI_LINE_MAX * 2]      = {0},
         ass_buffer_cat[SAMI_LINE_MAX * 3]  = {0};
    int  tag_open_flag,
         tag_property_flag      = 0,
         tag_text_index         = 0,
         tag_stack_push_index;
    Tag  tag_stack_push,
         *tag_stack_top;

    tag_po      = font_tag_string;

    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<':
                fprintf(stderr, "Test - Subtitle (TEXT)  : '%s'\n", text);

                if(strcmp(text, "") != 0 ){
                    if(sami_tag_stack_top(tag_stack, &tag_stack_top, TAG_STACK_MAX) >= 0){
                        sami_tag_font_property_get(tag_stack_top, &stack_pop_face_po, &stack_pop_color_po);
                        sami_tag_ass_font(ass_buffer, stack_pop_face_po, stack_pop_color_po, text);

                        if(sami_tag_stack_top_remove(tag_stack, tag_stack_top, TAG_STACK_MAX) < 0){
                            fprintf(stderr, "ERROR : Stack Delete Error\n");
                            free(tag_name);
                            break;
                        }
                    }else{
                        sprintf(ass_buffer, "%s", text);
                    }

                    strcat(ass_buffer_cat, ass_buffer);
                    fprintf(stderr, "Test - Accure ass tag : '%s' -> '%s' next: '%s'\n", text, ass_buffer, tag_po);
                    memset(&text, 0, sizeof(text));
                }

                if(sami_tag_container_get(tag_po, &tag_container, &tag_next_po) != 0){
                    // not close '>' tag
                    tag_po += strlen(tag_po);
                    fprintf(stderr, "ERROR : Tag '%s'\n", tag_po);
                    break;
                }

                if((tag_open_flag = sami_tag_name_get(tag_container, &tag_name, &tag_property_flag, &tag_property_start_po)) < 0){
                    fprintf(stderr, "ERROR : Tag name\n");
                    tag_po = tag_next_po;
                    free(tag_container);
                    break;
                }

                // if not <fomt> -> ignore
                if(strcasecmp(tag_name, "font") != 0){
                    tag_po = tag_next_po;
                    free(tag_name);
                    free(tag_container);
                    break;
                }

                switch(tag_open_flag){
                    case TAG_START:
                        if(tag_property_flag == 1){
                            if(sami_tag_parse_property(tag_property_start_po, &tag_stack_push) < 0){
                                free(tag_name);
                                fprintf(stderr, "ERROR : Tag Property\n");
                                break;
                            }
                        }

                        // do not free tag_name
                        tag_stack_push.name = tag_name;

                        if((tag_stack_push_index = sami_tag_stack_search(tag_stack, tag_name, TAG_STACK_MAX)) >= 0){
                            sami_tag_stack_update(tag_stack, tag_name, tag_stack_push_index, &tag_stack_push);
                        }else{
                            sami_tag_stack_push(tag_stack, tag_stack_push, TAG_STACK_MAX);
                        }

                        sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        break;
                    case TAG_END:
                        if(sami_tag_stack_top(tag_stack, &tag_stack_top, TAG_STACK_MAX) < 0){
                            fprintf(stderr, "ERROR : Under flow or Over flow\n");
                            free(tag_name);
                            break;
                        }

                        //fprintf(stderr, "(END  ) Stack pop\n");
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        //바로 이전태그의 시작과 지금태그(끝)이 같아야 함.
                        if(sami_tag_check(tag_name, tag_stack_top->name) < 0){
                            fprintf(stderr, "ERROR : Not match tag sequence : '%s' '%s'\n", tag_name, tag_stack_top->name);
                            free(tag_name);
                            break;
                        }
                        
                        // color 정보만 가져옴 (rt, face는 아직...)
                        // 다시 동적할당을 하는게 아니라, tag_stack에서 pop에서 할당된 주소만 가지고 온다.
                        sami_tag_font_property_get(tag_stack_top, &stack_pop_face_po, &stack_pop_color_po);
                        fprintf(stderr, "Test - Font Tag Result  : '%s' '%s' \n", stack_pop_face_po, stack_pop_color_po);
                        sami_tag_ass_font(ass_buffer, stack_pop_face_po, stack_pop_color_po, text);
                        strcat(ass_buffer_cat, ass_buffer);
                        fprintf(stderr, "Test - Ass  Tag Result  : '%s' -> '%s' (Ac: '%s')\n", text, ass_buffer, ass_buffer_cat);

                        if(sami_tag_stack_top_remove(tag_stack, tag_stack_top, TAG_STACK_MAX) < 0){
                            fprintf(stderr, "ERROR : Stack Delete Error\n");
                            free(tag_name);
                            break;
                        }

                        free(tag_name);
                        memset(&text, 0, sizeof(text));
                        tag_text_index  = 0;
                        break;
                }

                // do not move free position "tag_container"
                // do not free tag_name (if push, use stack)
                free(tag_container);
                tag_po = tag_next_po;
                break;
            default :
                // no include tag, accure in buffer
                text[tag_text_index++] = *tag_po++;
                break;
        }
    }

    // for no tag string
    //fprintf(stderr, "- BR Line -\n");
    if(strcmp(text, "") != 0 ){
        // DO NOT USE POP!
        if(sami_tag_stack_top(tag_stack, &tag_stack_top, TAG_STACK_MAX) >= 0){
            sami_tag_font_property_get(tag_stack_top, &stack_pop_face_po, &stack_pop_color_po);
            sami_tag_ass_font(ass_buffer, stack_pop_face_po, stack_pop_color_po, text);
        }else{
            sprintf(ass_buffer, "%s", text);
        }

        strcat(ass_buffer_cat, ass_buffer);
        //fprintf(stderr, "Test - Accure ass tag : '%s' -> '%s'\n", text, ass_buffer);
    }

    *sami_ass_text = strdup(ass_buffer_cat);
    return 0;
}

int Subtitle_Devide(const char *sami_filepath, int *sync_time_line_max)
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
            fprintf(stderr, "(count: %d / Line: %d)   : '%s'\n", j, i, subtitle_line[j][i]);
            sami_tag_parse(tag_stack, subtitle_line[j][i], &sami_ass_text);
            fprintf(stderr, "Test - Final Result     : '%s'\n\n", sami_ass_text);
            free(sami_ass_text);
            free(subtitle_line[j][i]);
        }
        fprintf(stderr, "Stack Free!\n");
        sami_tag_stack_free_all(tag_stack, TAG_STACK_MAX);
        fprintf(stderr, "EOL\n");
    }

    fprintf(stderr, "END\n");

    return 0;
}
