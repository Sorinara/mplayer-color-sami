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
    int  name_allocation,
         value_allocation;
} Tag_Property;

typedef struct _Tag{
    char *name;
    int  name_allocation,
         property_count;

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

void sami_tag_stack_free(Tag *stack_element, int free_flag)
{/*{{{*/
    int i;

    //fprintf(stderr, "[%s][%s(%d)] Tag Name '%d/%d'\n", __TIME__, __FILE__, __LINE__, stack_element->name_allocation, free_flag);
    if(free_flag == 1 || stack_element->name_allocation == 1){
        free(stack_element->name);
    }

    stack_element->name = NULL;

    for(i = 0;i < stack_element->property_count;i ++){
        //fprintf(stderr, "[%s][%s(%d)] Property '%d/%d'\n", __TIME__, __FILE__, __LINE__, i, stack_element->name_allocation);
        if(free_flag == 1 || stack_element->property[i].name_allocation== 1){
            free(stack_element->property[i].name);
        }

        if(free_flag == 1 || stack_element->property[i].value_allocation == 1){
            free(stack_element->property[i].value);
        }

        stack_element->property[i].name     = NULL;
        stack_element->property[i].value    = NULL;
    }

    stack_element->property_count = 0;
}/*}}}*/

void sami_tag_stack_free_all(Tag **stack, unsigned int stack_max_size)
{/*{{{*/
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        sami_tag_stack_free(stack[i], 1);

        free(stack[i]);
        stack[i]= NULL;
    }
}/*}}}*/

int sami_tag_stack_top_get(Tag **stack, Tag **stack_element, unsigned int stack_max_size)
{/*{{{*/
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

    if(stack_element != NULL){
       // memcpy(stack_element, stack[i], sizeof(Tag));
       *stack_element = stack[i];
    }

    return i;
}/*}}}*/

int sami_tag_stack_name_search(Tag **stack, const char *tag_name, unsigned int stack_max_size) 
{/*{{{*/
    int i,
        match_index = -1,
        match_flag  = 0;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

       if(strcasecmp(stack[i]->name, tag_name) == 0){
            match_flag  = 1;
            match_index = i;    
            // do not "break" statement... because for search top in stack
       }
    }

    if(match_flag != 1){
        return -1;
    }

    return match_index;
}/*}}}*/

int sami_tag_stack_element_combine_property_check(Tag *tag_source, char *target_tag_name, int *stack_top, int *stack_index_match)
{/*{{{*/
    int property_index,
        property_match_flag = 0;

    for(property_index = 0;property_index < tag_source->property_count;property_index ++){
        if(strcasecmp(tag_source->property[property_index].name, target_tag_name) == 0){
            property_match_flag = 1;
            break;
        }
    }

    switch(property_match_flag){
        case 0:
            *stack_index_match = *stack_top;
            (*stack_top) ++;
            break;
        case 1: 
            *stack_index_match = property_index;
            break;
    }

    return property_match_flag;
}/*}}}*/

int sami_tag_stack_element_combine(Tag *tag_source, Tag *tag_target, Tag *tag_new)
{/*{{{*/
    int i,
        new_tag_index,
        new_tag_stack_top = 0;

    if(strcasecmp(tag_source->name, tag_target->name) != 0){
        return -1;
    }

    new_tag_stack_top = tag_source->property_count;

    for(i = 0;i < new_tag_stack_top;i++){
        tag_new->property[i].name               = tag_source->property[i].name;
        tag_new->property[i].value              = tag_source->property[i].value;
        tag_new->property[i].name_allocation    = 0;
        tag_new->property[i].value_allocation   = 0;
    }
    
    for(i = 0;i < tag_target->property_count;i++){
        switch(sami_tag_stack_element_combine_property_check(tag_source, tag_target->property[i].name, &new_tag_stack_top, &new_tag_index)){
            // new
            case 0:
                tag_new->property[new_tag_index].name               = tag_target->property[i].name;
                tag_new->property[new_tag_index].value              = tag_target->property[i].value;
                tag_new->property[new_tag_index].name_allocation    = 0;
                tag_new->property[new_tag_index].value_allocation   = 0;
                break;
            // exist - only change value (don't touch name)
            case 1:
                tag_new->property[new_tag_index].value              = tag_target->property[i].value;
                break;
        }
        //fprintf(stderr, "[%s][%s(%d)] (%d) '%s' : %s\n", __TIME__, __FILE__, __LINE__, new_tag_index, tag_target->property[i].name, tag_target->property[i].value);
    }

    tag_new->property_count     = new_tag_stack_top;
    tag_new->name               = tag_source->name;
    tag_new->name_allocation    = 0;
    return 0;
}/*}}}*/

int sami_tag_stack_push(Tag **stack, Tag stack_element, unsigned int stack_max_size)
{/*{{{*/
    int stack_top;

    // check overflow
    if((stack_top = sami_tag_stack_top_get(stack, NULL, stack_max_size)) == -2){
        return -1;
    }

    stack[stack_top + 1] = (Tag*)calloc(1, sizeof(Tag));
    memcpy(stack[stack_top + 1], &stack_element, sizeof(Tag));
    return 0;
}/*}}}*/

int sami_tag_stack_top_remove(Tag **stack, Tag *stack_element, unsigned int stack_max_size)
{/*{{{*/
    int stack_top;

    // check underflow
    if((stack_top = sami_tag_stack_top_get(stack, &stack_element, stack_max_size)) == -1){
        return -1;
    }

    sami_tag_stack_free(stack[stack_top], 0);
    free(stack[stack_top]);
    stack[stack_top] = NULL;

    return 0;
}/*}}}*/

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

    //fprintf(stderr, "[%s][%s(%d)] '%s' : '%s' '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Property Name : ", string, *property_name_strip, *next);
    free(property_name);
    return 0;
}/*}}}*/

void sami_tag_stack_print(Tag **stack, unsigned int stack_max_size)
{/*{{{*/
    int i,
        j;

    if(stack[0] == NULL){
        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "Stack Empty");
        return;
    }

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        fprintf(stderr, "[%s][%s(%d)] %s (%d) : '%s'\n", __TIME__, __FILE__, __LINE__, "Stack DATA         ", i, stack[i]->name);

        for(j = 0;j < stack[i]->property_count;j ++){
            fprintf(stderr, "[%s][%s(%d)] %s (%d/%d)         : { [%s] = '%s' }\n", __TIME__, __FILE__, __LINE__, "-Property", j, stack[i]->property_count, stack[i]->property[j].name, stack[i]->property[j].value);
        }
    }

    if(i + 1 == stack_max_size){
        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "Stack Full");
    }
}/*}}}*/

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
    //fprintf(stderr, "[%s][%s(%d)]  '%s' '%s' '%s' '%s' %d -> '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Property Value : ", string, start_po, end_po, value_length, *value);
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

int sami_tag_property_set(char *tag_property_start_po, Tag *stack_element)
{/*{{{*/
    char *property_po,
         *property_name,
         *property_name_next_po     = NULL,
         *property_value,
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
        if(sami_tag_property_value_get(property_name_next_po, &property_value, &property_value_next_po) < 0){
            free(property_name);
            ret = -3;
            break;
        }

        stack_element->property[property_count].name                = property_name;
        stack_element->property[property_count].value               = property_value;
        stack_element->property[property_count].name_allocation     = 1;
        stack_element->property[property_count].value_allocation    = 1;

        property_po = property_value_next_po;
        property_count ++;
    }

    stack_element->property_count = property_count;
    
    /*
    int i;
    for(i = 0;i < stack_element->property_count;i ++){
        fprintf(stderr, "[%s][%s(%d)] %s '%d' '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Property Loop    :", i, stack_element->property[i].name, stack_element->property[i].value);
    }  */
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

int sami_tag_name_set(char *tag_container, Tag *tag_stack_element, int *property_flag, char **next_po)
{/*{{{*/
    int tag_type;

    if((tag_type = sami_tag_name_get(tag_container, &(tag_stack_element->name), property_flag, next_po)) < 0){
        return -1;
    }

    tag_stack_element->name_allocation = 1;

    return tag_type;
}/*}}}*/

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

int sami_tag_font_property_get(Tag *stack_element, char **font_face_buffer, char **font_color_buffer)
{/*{{{*/
    int i,
        ret = 0;

    *font_face_buffer  = NULL;
    *font_color_buffer = NULL;

    for(i = 0;i < stack_element->property_count;i ++){
        if(strcasecmp(stack_element->property[i].name, "face") == 0){
            *font_face_buffer = stack_element->property[i].value;
            ret |= 1;
        }else if(strcasecmp(stack_element->property[i].name, "color") == 0){
            *font_color_buffer = stack_element->property[i].value;
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
         *tag_property_start_po,
         *stack_pop_face_po,
         *stack_pop_color_po,
         text[SAMI_LINE_MAX / 2]            = {0},
         ass_buffer[SAMI_LINE_MAX * 2]      = {0},
         ass_buffer_cat[SAMI_LINE_MAX * 3]  = {0};
    int  tag_type,
         tag_property_flag      = 0,
         tag_text_index         = 0,
         tag_stack_top_index    = -1;
    Tag  tag_stack_element_now,
         tag_stack_element_push,
         *tag_stack_top;

    tag_po = font_tag_string;

    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<':
                if(sami_tag_container_get(tag_po, &tag_container, &tag_next_po) != 0){
                    // not exist close tag (unable search '>')
                    tag_po += strlen(tag_po);
                    fprintf(stderr, "[%s][%s(%d)] '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Tag", tag_po);
                    break;
                }

                if((tag_type = sami_tag_name_set(tag_container, &tag_stack_element_now, &tag_property_flag, &tag_property_start_po)) < 0){
                    fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : not have tag name, ignore");
                    tag_po = tag_next_po;
                    free(tag_container);
                    break;
                }

                // if not <font>(</font>) -> ignore
                if(strcasecmp(tag_stack_element_now.name, "font") != 0){
                    tag_po = tag_next_po;
                    free(tag_stack_element_now.name);
                    free(tag_container);
                    break;
                }

                // if exist text
                if(strcmp(text, "") != 0){
                    if(sami_tag_stack_top_get(tag_stack, &tag_stack_top, TAG_STACK_MAX) < 0){
                        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__,  __FILE__, __LINE__, "ERROR : Under flow or Over flow");
                        free(tag_stack_element_now.name);
                        break;
                    }

                    //바로 이전태그의 시작과 지금태그의 끝이 같아야 함.
                    if(sami_tag_check(tag_stack_element_now.name, tag_stack_top->name) < 0){
                        fprintf(stderr, "[%s][%s(%d)] '%s' '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Not match open/close tag name :", tag_stack_element_now.name, tag_stack_top->name);
                        free(tag_stack_element_now.name);
                        break;
                    }

                    // color 정보만 가져옴 (rt, face는 아직...)  동적할당이 아니라, tag_stack에서 pop에서 할당된 주소만 가지고 온다.
                    sami_tag_font_property_get(tag_stack_top, &stack_pop_face_po, &stack_pop_color_po);
                    fprintf(stderr, "[%s][%s(%d)] %s '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Font Tag Result  :", stack_pop_face_po, stack_pop_color_po);
                    sami_tag_ass_font(ass_buffer, stack_pop_face_po, stack_pop_color_po, text);
                    strcat(ass_buffer_cat, ass_buffer);
                    fprintf(stderr, "[%s][%s(%d)] %s '%s' -> '%s' (Ac: '%s')\n", __TIME__, __FILE__, __LINE__, "Test - Ass  Tag Result  :", text, ass_buffer, ass_buffer_cat);

                    memset(&text, 0, sizeof(text));
                    tag_text_index  = 0;
                }

                switch(tag_type){
                    case TAG_START:
                        if(tag_property_flag == 1){
                            if(sami_tag_property_set(tag_property_start_po, &tag_stack_element_now) < 0){
                                free(tag_stack_element_now.name);
                                fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Tag Property");
                                break;
                            }
                        }

                        if((tag_stack_top_index = sami_tag_stack_name_search(tag_stack, tag_stack_element_now.name, TAG_STACK_MAX)) >= 0){
                            sami_tag_stack_element_combine(tag_stack[tag_stack_top_index], &tag_stack_element_now, &tag_stack_element_push);
                        }else{
                            tag_stack_element_push = tag_stack_element_now;
                        }

                        sami_tag_stack_push(tag_stack, tag_stack_element_push, TAG_STACK_MAX);
                        fprintf(stderr, "[%s][%s(%d)] %s (%d)\n", __TIME__, __FILE__, __LINE__, "Push OK, Stack Status   :", tag_stack_top_index);
                        sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        break;
                    case TAG_END:
                        if(sami_tag_stack_top_remove(tag_stack, tag_stack_top, TAG_STACK_MAX) < 0){
                            fprintf(stderr, "[%s][%s(%d)] %s\n", __TIME__, __FILE__, __LINE__, "ERROR : Stack Delete Error");
                            free(tag_stack_element_now.name);
                            break;
                        }

                        fprintf(stderr, "[%s][%s(%d)] %s (%d)\n", __TIME__, __FILE__, __LINE__, "Pop  OK, Stack Status   :", tag_stack_top_index);
                        sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        free(tag_stack_element_now.name);
                        break;
                }

                // do not move free position "tag_container"
                // do not free tag_stack_element_now.name (if push, use stack)
                free(tag_container);
                tag_po = tag_next_po;
                break;
            default :
                // no include tag, accure in buffer
                text[tag_text_index++] = *tag_po++;
                break;
        }
    }

    if(strcmp(text, "") != 0 ){
        // DO NOT USE POP!
        if(sami_tag_stack_top_get(tag_stack, &tag_stack_top, TAG_STACK_MAX) >= 0){
            sami_tag_font_property_get(tag_stack_top, &stack_pop_face_po, &stack_pop_color_po);
            sami_tag_ass_font(ass_buffer, stack_pop_face_po, stack_pop_color_po, text);
        }else{
            sprintf(ass_buffer, "%s", text);
        }

        strcat(ass_buffer_cat, ass_buffer);
        //fprintf(stderr, "[%s][%s(%d)] '%s' '%s' -> '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Accure ass tag :", text, ass_buffer);
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
            fprintf(stderr, "[%s][%s(%d)] (Sync : %d / Line: %d)    : '%s'\n", __TIME__, __FILE__, __LINE__, j, i, subtitle_line[j][i]);
            sami_tag_parse(tag_stack, subtitle_line[j][i], &sami_ass_text);
            fprintf(stderr, "[%s][%s(%d)] %s '%s'\n\n", __TIME__, __FILE__, __LINE__, "Test - Final Result     :", sami_ass_text);
            free(sami_ass_text);
            free(subtitle_line[j][i]);
        }

        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "-Stack Clean! (Init All Element, EOL)");
        fprintf(stderr, "[%s][%s(%d)] '%s'\n\n\n", __TIME__, __FILE__, __LINE__, "-------------------------------------");
        sami_tag_stack_free_all(tag_stack, TAG_STACK_MAX);
    }

    fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "All Finished");
    return 0;
}
