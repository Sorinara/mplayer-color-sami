#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "subreader_sami.h"

#define TEXT_LINE_LENGTH_MAX        256
#define ASS_BUFFER_LENGTH_MAX       512
#define ASS_BUFFER_CAT_LENGTH_MAX   1024
#define TAG_PROPERTY_MAX            256
#define TAG_STACK_MAX               32

char *subtitle_line[5000][16] = {{NULL}};

typedef struct _Element{
    void *data;

    unsigned int size;
} Element;

typedef struct _List{
   Element *element;

   unsigned int ep,
                element_max;
} List;

typedef struct _Table{
    List *list;

    unsigned int lp,
                 list_max;
} Table;

typedef struct _Tag_Property{
    char *name,
         *value;

} Tag_Property;

typedef struct _Tag{
    char *name;
    int  name_allocation,
         property_count;

    Tag_Property property[TAG_PROPERTY_MAX];
} Tag;

int element_data_insert(Element *element, void *data, const unsigned int element_size)
{/*{{{*/
    if(element == NULL){
        return -1;
    }

    if(element_size <= 0){
        return -2;
    }

    if((element->data = (void *)calloc(1, element_size)) == NULL){
        return -3;
    }

    memcpy(element->data, data, element_size);

    element->size = element_size;

    return 0;
}/*}}}*/

void element_get(const List list, const unsigned int index, Element *element)
{/*{{{*/
    *element = list.element[index];
}/*}}}*/

void element_print(const List list, const unsigned int index)
{/*{{{*/
    // Warnning: do not check over/underflow
    fprintf(stderr, "List Print { element[%d] = '%p' }\n", index, list.element[index].data);
}/*}}}*/

int element_array_create(List *list, const unsigned int element_max)
{/*{{{*/
    if((list->element = (Element *)calloc(element_max, sizeof(Element))) == NULL){
        return -1;
    }

    return 0;
}/*}}}*/

void element_array_free(List list, const unsigned int index)
{/*{{{*/
    free(list.element);
    list.element = NULL;
}/*}}}*/

int list_init(List *list, const unsigned int element_max)
{/*{{{*/
    if(element_max == 0){
        return -1;
    }

    if(element_array_create(list, element_max) < 0){
        return -2;
    }

    list->ep           = 0;
    list->element_max  = element_max;

    return 0;
}/*}}}*/

int list_check(List list, Element element)
{/*{{{*/
    int list_index;
    
    for(list_index = 0;list_index < list.ep;list_index ++){
        if(list.element[list_index].size != element.size){
            continue;
        }

       if(memcmp(list.element[list_index].data, element.data, element.size) == 0){
          return list_index; 
       }
    }
    
    return -1;
}/*}}}*/

void list_free(List list)
{/*{{{*/
    int i;

    for(i = 0;i < list.ep;i ++){
        //element_print(list, i);
        element_array_free(list, i);
    }

    free(list.element);
}/*}}}*/

// uniq_element_check(list, element, index), 만약 List안에 같은 element가
// list         : list에서 element 와 일치하는것을 찾음
// element      : 검색할 element
// index 0/1    : 0 : 존재하지 않음 / 1 : 1개이상 존재함
// => list에서 한개를 발견하면 바로 루프를 중단한다.
int list_uniq(List list_source, List *list_dest, void (*uniq_element_check)(List *, Element, unsigned int *, int *))
{/*{{{*/
    int i,
        list_dest_ep        = 0,
        element_match_index = 0;
    unsigned int element_match_flag = 0;

    if(list_source.ep == 0){
        return -1;
    }

    if(list_init(list_dest, list_source.element_max) < 0){
        return -2;
    }

    for(i = 0;i < list_source.ep;i ++){
        uniq_element_check(list_dest, list_source.element[i], &element_match_flag, &element_match_index);

        // if element is not exit in list_dest
        if(element_match_flag == 0){
            element_data_insert(&(list_dest->element[list_dest_ep]), list_source.element[i].data, list_source.element_max);
            list_dest_ep ++;
        }
    }

    list_dest->ep = list_dest_ep;

    return 0;
}/*}}}*/

void table_free(Table *table)
{/*{{{*/
    int i;

    for(i = 0;i < table->lp;i ++){
        list_free(table->list[i]);
    }

    free(table->list);
}/*}}}*/

int table_array_create(Table *table, const unsigned int list_max)
{/*{{{*/
    if((table->list = calloc(list_max, sizeof(List))) < 0){
        return -1;
    }

    return 0;
}/*}}}*/

// TODO: 나중을 대비해서 row/col 추가/삭제/검색기능도 가능하도록 설정하기.
int table_init(Table *table, unsigned int key_list_max)
{/*{{{*/
    if(table_array_create(table, key_list_max) < 0){
        return -1;
    }

    table->lp       = 0;
    table->list_max = key_list_max;

    return 0;
}/*}}}*/

int table_col()
{
    return 0;
}

int table_row()
{
    return 0;
}

int table_search_col(Table *table, unsigned int col, void **element)
{
    return 0;
}

int table_search_row(Table *table, unsigned int row, void **element)
{
    return 0;
}

int table_list_uniq_search(Table *table, Element element, unsigned int *list_index)
{
    int i;

    // list element size is equal in table
    *list_index = 0;

    for(i = 0;i < table->lp;i ++){
        if(table->list[i].ep == 0){
            continue;
        }

        if(memcmp(table->list[i].element[0].data, element.data, element.size) == 0){
            *list_index = (unsigned int)i;
            return 1;
        }else{

        }
    }

    return 0;
}

// uniq_element_check() :  만약 검사할 엘리먼트가 table안의 list의 첫번쩨 element와 일치한다면
// return little then 0 : 일치하는것이 없음
// return n    : 일치하는  index
// => 어찌되었건 list를 끝까지 검사(루프)한다
int table_list_uniq_pile(List list_source, Table *table_dest)
{
    int i,
        table_match_flag    = 0,
        list_dest_lp        = 0,
        table_dest_lp       = 0;
    unsigned int list_match_index   = 0,
                 list_element_index = 0;

    if(list_source.ep == 0){
        return -1;
    }

    if(table_init(table_dest, list_source.element_max) < 0){
        return -2;
    }

    for(i = 0;i < list_source.ep;i ++){
        table_match_flag = table_list_uniq_search(table_dest, list_source.element[i], &list_match_index);

        if(table_match_flag == 0){
            list_dest_lp = table_dest->lp;
            table_dest->lp ++;
        }else{
            list_dest_lp = list_match_index;
        }

        list_element_index                                          = table_dest->list[list_dest_lp].ep;
        table_dest->list[list_dest_lp].element[list_element_index]  = list_source.element[i];
    }

    table_dest->lp = table_dest_lp;

    return 0;
}

int stack_top_index_get(const List list)
{/*{{{*/
    // empty
    if(list.ep == 0){
        return -1;
    }

    // full
    if(list.ep == list.element_max){
        return -2;
    }

    return list.ep;
}/*}}}*/

// get pointer in list, not data structure !
int stack_top_get(const List list, Element *element)
{/*{{{*/
    int sp;

    // check overflow
    switch((sp = stack_top_index_get(list))){
        case -1:
            return -1;
        case -2:
            return -2;
    }

    if(element != NULL){
        element_get(list, sp, element);
    }

    return sp;
}/*}}}*/

// top position in list, memory allocation
int stack_push(List list, const void *data, const unsigned int element_size)
{/*{{{*/
    int sp;

    // check overflow (do not check empty !)
    if((sp = stack_top_index_get(list)) == -2){
        return -1;
    }

    if((list.element[sp + 1].data = calloc(1, element_size)) == NULL){
        return -2;
    }

    //fprintf(stderr, "[%s][%s(%d)] DEBUG LINE '%s' (%p)\n", __TIME__,  __FILE__, __LINE__, "Element Allocated", list[sp + 1]);
    memcpy(list.element[sp + 1].data, data, element_size);
    list.element[sp + 1].size = element_size;

    list.ep ++;

    return 0;
}/*}}}*/

// remove only, if you want get element in list, use "stack_top_get()"
int stack_pop(List list)
{/*{{{*/

    // check underflow (do not check empty !)
    if(stack_top_index_get(list) == -1){
        return -1;
    }

    //fprintf(stderr, "[%s][%s(%d)] DEBUG LINE '%s' (%d) (%p)\n", __TIME__,  __FILE__, __LINE__, "List Element Free", index, list[index]);
    //free(list.element[list.ep]);

    //fprintf(stderr, "[%s][%s(%d)] DEBUG LINE '%s'\n", __TIME__,  __FILE__, __LINE__, "List Element Free - Done");
    list.element[list.ep].data = NULL;
    list.element[list.ep].size = 0;
    list.ep --;

    return 0;
}/*}}}*/

char* strescnext(char *start, const char delminiter)
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

int strescmget(char *string, const char start_char, const char end_char, char **middle_string, char **next)
{/*{{{*/
    char *start_string_po,
         *end_string_po;
    int middle_string_size = 0;

    if(start_char == '\0'){
        start_string_po = string;
    }else{
        if((start_string_po = strescnext(string, start_char)) == NULL){
            return -1;
        }
        // eat start char 
        start_string_po += sizeof(char);
    }

    if(end_char == '\0'){
        end_string_po = string + strlen(string);
    }else{
        if((end_string_po = strescnext(start_string_po, end_char)) == NULL){
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

int strstripdup(char *string, char **strip_string)
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

void sami_tag_list_element_free(void *list_element_void)
{/*{{{*/
    int i;
    Tag *element;

    // for list's function
    element = (Tag *)list_element_void;

    if(element->name_allocation == 1){
        //fprintf(stderr, "[%s][%s(%d)] Tag Name Allocation (%d) : '%s' '%p'\n", __TIME__, __FILE__, __LINE__, element->name_allocation, element->name, element->name);
        fprintf(stderr, "[%s][%s(%d)] %s (%p)\n", __TIME__, __FILE__, __LINE__, "Free Tag Name", element->name);
        free(element->name);
    }

    element->name = NULL;

    for(i = 0;i < element->property_count;i ++){
        free(element->property[i].name);
        fprintf(stderr, "[%s][%s(%d)] Free Property (%d) name (%p)\n", __TIME__, __FILE__, __LINE__, i, element->property[i].name);

        free(element->property[i].value);
        fprintf(stderr, "[%s][%s(%d)] Free Property (%d) value (%p)\n", __TIME__, __FILE__, __LINE__, i, element->property[i].value);

        element->property[i].name     = NULL;
        element->property[i].value    = NULL;
    }

    element->property_count = 0;
}/*}}}*/

void sami_tag_list_element_check(List tag_list_accrue, void *tag_list_element_void, unsigned int *tag_name_match_flag, int *tag_list_match_index)
{/*{{{*/
    int tag_list_index;
    Tag *tag_list_element,
        *tag_list_accrue_element;

    tag_list_element        = (Tag *)tag_list_element_void;
    *tag_name_match_flag    = 0;
    *tag_list_match_index   = -1;

    for(tag_list_index = 0;tag_list_index < tag_list_accrue.ep;tag_list_index ++){
        element_get(tag_list_accrue, tag_list_index, (void *)&tag_list_accrue_element);

        if(strcasecmp(tag_list_accrue_element->name, tag_list_element->name) == 0){
            *tag_name_match_flag = 1;
            break;
        }
    }

    if(*tag_name_match_flag == 1){
        *tag_list_match_index = tag_list_index;
    }
}/*}}}*/

void sami_tag_list_element_combine_check_property(const Tag *tag_source, const Tag_Property tag_dest_property, int *property_name_match_flag, int *property_value_match_flag, int *list_top_element, int *list_index_match)
{/*{{{*/
    int property_index;

    *property_name_match_flag   = 0;
    *property_value_match_flag  = 0;

    for(property_index = 0;property_index < tag_source->property_count;property_index ++){
        if(strcasecmp(tag_source->property[property_index].name, tag_dest_property.name) == 0){
            *property_name_match_flag = 1;

            if(strcasecmp(tag_source->property[property_index].value, tag_dest_property.value) == 0){
                *property_value_match_flag = 1;
            }

            break;
        }
    }

    switch(*property_name_match_flag){
        case 0:
            *list_index_match = *list_top_element;
            (*list_top_element) ++;
            break;
        case 1: 
            *list_index_match = property_index;
            break;
    }
}/*}}}*/

int sami_tag_list_print(List list, const char *message)
{/*{{{*/
    int i,
        list_element_property_index;
    Tag *list_element;

    fprintf(stderr, "[%s][%s(%d)] %s\n", __TIME__, __FILE__, __LINE__, message);

    if(list.ep == 0){
        fprintf(stderr, "[%s][%s(%d)] %s\n", __TIME__, __FILE__, __LINE__, "List Empty");
        return -1;
    }

    if(list.ep == list.element_max){
        fprintf(stderr, "[%s][%s(%d)] %s\n", __TIME__, __FILE__, __LINE__, "List Full");
        return -2;
    }

    for(i = 0;i < list.ep;i ++){
        element_get(list, i, (void *)&list_element);

        fprintf(stderr, "[%s][%s(%d)] %s (%d) : %s (%p)\n", __TIME__, __FILE__, __LINE__, "List Tag Name     ", i, list_element->name, list_element->name);

        for(list_element_property_index = 0;list_element_property_index < list_element->property_count;list_element_property_index ++){
            fprintf(stderr, "[%s][%s(%d)] %s (%d/%d)         : { [%s] = '%s' }\n", __TIME__, __FILE__, __LINE__, "-Property",\
                                                                                                                 list_element_property_index,\
                                                                                                                 list_element->property_count,\
                                                                                                                 list_element->property[list_element_property_index].name,\
                                                                                                                 list_element->property[list_element_property_index].value);
        }
    }

    return 0;
}/*}}}*/

int sami_tag_container_get(char *tag_po, char **tag_container, char **next_po)
{/*{{{*/
    // get all tag
    if(strescmget(tag_po, '<', '>', tag_container, next_po) < 0){
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
        if(strstripdup(tag_container + 1, tag_name) < 0){
            return -1;
        }

        tag_type            = 1; // end
        *property_flag      = 0;
        *next_po            = NULL;
    // get tag name "<font color="blue">" (have property)
    }else if(strescmget(tag_container, '\0', ' ', tag_name, next_po) > 0){
        tag_type            = 0;
        *property_flag      = 1;
    // get tag_name "<i>" (not have property)
    }else{
        if(strstripdup(tag_container, tag_name) < 0){
            return -2;
        }

        tag_type            = 0; // start
        *property_flag      = 0;
        *next_po            = NULL;
    }

    return tag_type;
} /*}}}*/

void sami_tag_property_get(const Tag *element, Tag_Property *font_property, const unsigned int property_max)
{/*{{{*/
    int i,
        j;

    for(i = 0;i < (int)property_max;i++){
        font_property[i].value = "";

        for(j = 0;j < element->property_count;j ++){
            if(strcasecmp(font_property[i].name, element->property[j].name) == 0){
                font_property[i].value = element->property[j].value;
            }
        }
    }
}/*}}}*/

int sami_tag_property_name_get(char *string, char **property_name_strip, char **next)
{/*{{{*/
    char *property_name,
         *property_name_end_po;

    if(strescmget(string, '\0', '=', &property_name, &property_name_end_po) < 0){
        return -1;
    }

    if(strstripdup(property_name, property_name_strip) < 0){
        free(property_name);
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

            if((end_po = strescnext(start_po, '\'')) == NULL){
                if((end_po = strescnext(start_po, '"')) == NULL){
                    return -2;
                }
            }

            *next_po = end_po + 1;
            break;
        case '"' :
            start_po   += 1;

            if((end_po = strescnext(start_po, '"')) == NULL){
                if((end_po = strescnext(start_po, '\'')) == NULL){
                    return -2;
                }
            }

            *next_po = end_po + 1;
            break;
        default:
            if((end_po   = strescnext(start_po, ' ')) == NULL){
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

int sami_tag_property_set(char *tag_property_start_po, Tag *element)
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
        // get property name(face/color) - ERROR Stop! (not exception process)
        if(sami_tag_property_name_get(property_po, &property_name, &property_name_next_po) < 0 ){
            ret = 1;
            break;
        }

        // get property value - ERROR Stop! (not exception process)
        if(sami_tag_property_value_get(property_name_next_po, &property_value, &property_value_next_po) < 0){
            free(property_name);
            ret = 2;
            break;
        }

        element->property[property_count].name                = property_name;
        element->property[property_count].value               = property_value;

        property_po = property_value_next_po;
        property_count ++;
    }

    element->property_count = property_count;
    
    /*
    int i;
    for(i = 0;i < element->property_count;i ++){
        fprintf(stderr, "[%s][%s(%d)] %s '%d' '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "Test - Property Loop    :", i, element->property[i].name, element->property[i].value);
    }  */
    return ret;
}/*}}}*/

int sami_tag_ass_font_color_table(const char *color_name, char *color_buffer)
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

        
int sami_tag_ass_font_color_tag(const char *color_value, char *ass_color_buffer, const unsigned int ass_color_buffer_length)
{/*{{{*/
    // must be init 0
    char color_rgb_buffer[7]      = {0},
         color_bgr_buffer[7]      = {0},
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
        if(sami_tag_ass_font_color_table(color_value, color_rgb_buffer) < 0){
            return -1;
        }
    }else{
        sprintf(color_rgb_buffer, color_value);
    }

    if(strcmp(color_rgb_buffer, "") != 0){
        // change sami subtitle color (RGB) to ass color(BGR)
        memcpy(color_bgr_buffer + 0, color_rgb_buffer + 4, 2);
        memcpy(color_bgr_buffer + 2, color_rgb_buffer + 2, 2);
        memcpy(color_bgr_buffer + 4, color_rgb_buffer + 0, 2);
        color_rgb_number = strtol(color_bgr_buffer, NULL, 16);

        memset(ass_color_buffer, 0, ass_color_buffer_length);
        snprintf(ass_color_buffer, ass_color_buffer_length, "{\\r\\c&H%06X&}", color_rgb_number);
    }

    return 0;
}/*}}}*/

// face - only englist name, 한글이름은 바로깨짐.
// 일단 fc-list에서 나오는 영문이름으로 하면 잘 나옴.
// fc-list소스를 분석하던가, ass에서 한글명이 되게 하던가...
// 아마 쉽기는 후자가 쉽겠지만, 전자가 될 가능성은 높은듯.
int sami_tag_ass_font_face_tag(const char *face_value, char **ass_face)
{/*{{{*/
    int face_value_length,
        ass_face_tag_start_length;

    if((face_value_length = strlen(face_value)) < 0){
        return -1;
    }

    // strlen("{\\fn}") + '\0' == 6
    ass_face_tag_start_length = face_value_length + 6;

    if((*ass_face = calloc(sizeof(char), ass_face_tag_start_length)) == NULL){
        return -2;
    }

    snprintf(*ass_face, ass_face_tag_start_length, "{\\fn%s}", face_value);

    return 0;
}/*}}}*/

int sami_ass_tag_set(const char *tag_start, const char *text, const char *tag_end, char **save_buffer)
{/*{{{*/
    int tag_start_length,
        text_length,
        tag_end_length,
        tag_total_length;

    if((tag_start_length = strlen(tag_start)) < 0){
        return -1;
    }

    if((text_length = strlen(text)) < 0){
        return -2;
    }

    if((tag_end_length = strlen(tag_end)) < 0){
        return -3;
    }

    // 7 : save_buffer's basket length
    // 1 : buffer '\0' length
    tag_total_length = tag_start_length + text_length + tag_end_length + 1;

    if((*save_buffer = calloc(sizeof(char), tag_total_length)) == NULL){
        return -4;
    }

    snprintf(*save_buffer, tag_total_length, "%s%s%s", tag_start, text, tag_end);

    return 0;
}/*}}}*/

int sami_tag_ass_combine(const char *tag1, const char *tag2, char **save_buffer)
{/*{{{*/
    int tag1_length,
        tag2_length,
        save_buffer_length = 0;

    if(tag1 != NULL){
        tag1_length = strlen(tag1);
    }

    if(tag2 != NULL){
        tag2_length = strlen(tag2);
    }

    save_buffer_length = tag1_length + tag2_length;

    if(save_buffer_length <= 0){
        return -1;
    }

    if((*save_buffer = calloc(1, save_buffer_length)) == NULL){
        return -2;
    }

    if(tag1 != NULL){
        strcpy(*save_buffer, tag1);
    } 

    if(tag2 != NULL){
        strcat(*save_buffer, tag2);
    }

    return 0;
}/*}}}*/

int sami_tag_ass_convert_tag(Tag *tag)
{/*{{{*/
    int i;
    char ass_color_tag_start[18] = {0}, // ass color tag's size is fix 16 (add null)
         *ass_face_tag_start    = NULL,
         *ass_tag_start         = NULL,
         ass_tag_end_font[9]    = {0},
         *ass_tag_end           = NULL;
    //int ass_tag_start_allocate  = 0;

    if(strcasecmp(tag->name, "font") == 0 ){
        for(i = 0;i < tag->property_count;i ++){
            if(strcasecmp(tag->property[i].name, "color") != 0){
                if(sami_tag_ass_font_color_tag(tag->property[i].value, ass_color_tag_start, sizeof(ass_color_tag_start)) < 0){
                    return -1;
                }

                strcat(ass_tag_end_font, "{\\r}");
            }else if(strcasecmp(tag->property[i].name, "face") != 0){
                if(sami_tag_ass_font_face_tag(tag->property[i].value, &ass_face_tag_start) < 0){
                    return -2;
                }

                strcat(ass_tag_end_font, "{\\r}");
            }
        }

        if(sami_tag_ass_combine(ass_color_tag_start, ass_face_tag_start, &ass_tag_start) < 0){
            free(ass_face_tag_start);
            return -3;
        }

        //ass_tag_start_allocate = 1;
        strcpy(ass_tag_end, ass_tag_end_font);
        free(ass_face_tag_start);
    // bold - operate OK, but 
    }else if(strcasecmp(tag->name, "b") == 0 ){
        ass_tag_start   = "{\\b900}";
        ass_tag_end     = "{\\b0}";
    // ltalic - OK
    }else if(strcasecmp(tag->name, "i") == 0 ){
        ass_tag_start   = "{\\i1}";
        ass_tag_end     = "{\\i0}";
    // underline - OK
    }else if(strcasecmp(tag->name, "u") == 0 ){
        ass_tag_start   = "{\\u1}";
        ass_tag_end     = "{\\u0}";
    // strike - OK
    }else if(strcasecmp(tag->name, "s") == 0 ){
        ass_tag_start   = "{\\s1}";
        ass_tag_end     = "{\\s0}";
    // ruby - OK (TODO: must be check "ruby" in tag stck)
    // UP   : <ruby>주석을 달려는 자막<rt>주석</rt></ruby>
    // DOWN : 주석을 달려는 자막<ruby><rt>주석</ft><ruby>
    }/*  else if(strcasecmp(tag->name, "rt") == 0 ){
        sprintf(ass_tag_buffer, "{\\fs10}%s{\\r}", text);
    } */

    return 0;
}/*}}}*/

int sami_tag_ass_convert(const List tag_list_top, char **ass_tag, char *text)
{/*{{{*/
    //int i;

    /*
    for(i = 0;i < count;i++){
        sami_tag_ass_convert_tag();
    }  */
    
    // TODO: memory free
    /*
    if(sami_ass_tag_set(ass_tag_start, text, ass_tag_end, &ass_tag) < 0){
        //free(ass_tag_font_start);
        return -4;
    }*/

    return 0;
}/*}}}*/

// return 0         : empty text
// return 1         : exist ass text
// return -1 / -2   : only text
int sami_tag_ass(List tag_list, char *plain_text, char *ass_buffer)
{/*{{{*/
    //List tag_list_accrue;
    Table tag_table_dup;

    if(strcmp(plain_text, "") == 0){
        return 0;
    }

    if(table_list_uniq_pile(tag_list, &tag_table_dup) < 0){
        return -3;
    }

    /*
    if(sami_tag_ass_convert(tag_list_accrue, stack_ass_buffer, text) < 0){
        return -3;
    }

    strcpy(ass_buffer, stack_ass_buffer);
  */
    return 1;
} /*}}}*/

int sami_tag_parse_start(List tag_list, char *tag_name, char *tag_property_start_po, const int tag_property_flag)
{/*{{{*/
    Tag  tag_list_element_now;

    // initinal
    tag_list_element_now.name              = strdup(tag_name);
    tag_list_element_now.property_count    = 0;

    if(tag_property_flag == 1){
        if(sami_tag_property_set(tag_property_start_po, &tag_list_element_now) < 0){
            fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Set Tag Property Failed");
            sami_tag_list_element_free(&tag_list_element_now);
            return -1;
        }
    }

    if(stack_push(tag_list, &tag_list_element_now, sizeof(Tag)) < 0){
        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Stack Push Failed");
        sami_tag_list_element_free(&tag_list_element_now);
        return -2;
    }

    return 0;
}
/*}}}*/

// return 0 : no end tag
// return 1 : end tag
int sami_tag_name_match(const char *tag_name, const char *tag_list_pop_name)
{/*{{{*/
    // dismatch tag_push_name
    if((strcasecmp(tag_name, tag_list_pop_name)) != 0 ){
        return -1;
    }

    return 1;
}/*}}}*/

int sami_tag_parse_end(List tag_list, const char *tag_name)
{/*{{{*/
    Tag  *tag_list_top;

    if(stack_top_get(tag_list, (void *)&tag_list_top) < 0){
        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__,  __FILE__, __LINE__, "ERROR : Under flow or Over flow in stack. (end tag)");
        return -1;
    }

    // 바로 이전태그의 시작과 지금태그의 끝이 같아야 함.
    if(sami_tag_name_match(tag_name, tag_list_top->name) < 0){
        fprintf(stderr, "[%s][%s(%d)] '%s' '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : Not match open/close tag name :", tag_name, tag_list_top->name);
        return -2;
    }

    stack_pop(tag_list);

    return 0;
}/*}}}*/

int sami_tag_parse(List tag_list, char *source_string, char **ass_text)
{/*{{{*/
    // ass_buffer는 초기화 되어야 함.
    char *tag_po,
         *tag_container,
         *tag_next_po,
         *tag_name,
         *tag_property_start_po,
         text[TEXT_LINE_LENGTH_MAX]                 = {0},
         ass_buffer[ASS_BUFFER_LENGTH_MAX]          = {0},
         ass_buffer_cat[ASS_BUFFER_CAT_LENGTH_MAX]  = {0};
    int  tag_type,
         tag_property_flag      = 0,
         tag_text_index         = 0;

    tag_po = source_string;

    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<':
                if(sami_tag_container_get(tag_po, &tag_container, &tag_next_po) != 0){
                    // not exist close tag (unable search '>')
                    fprintf(stderr, "[%s][%s(%d)] '%s' '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : get container failed", tag_po);
                    goto END_OF_LINE;
                }

                if((tag_type = sami_tag_name_get(tag_container, &tag_name, &tag_property_flag, &tag_property_start_po)) < 0){
                    fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "ERROR : not have tag name. ignore this tag");
                    goto END_OF_TAG;
                }

                switch(sami_tag_ass(tag_list, text, ass_buffer)){
                    case 0: // not exist text
                        break;
                    case 1: // ass text
                        strcat(ass_buffer_cat, ass_buffer);
                        memset(&text, 0, sizeof(text));
                        tag_text_index  = 0;
                        break;
                    default: // plain text
                        strcat(ass_buffer_cat, text);
                        memset(&text, 0, sizeof(text));
                        tag_text_index  = 0;
                        break;
                }

                switch(tag_type){
                    case 0: // start tag
                        sami_tag_parse_start(tag_list, tag_name, tag_property_start_po, tag_property_flag);
                        sami_tag_list_print(tag_list, "Tag Start Push OK, Stack Result :");
                        break;
                    case 1: // end tag
                        sami_tag_parse_end(tag_list, tag_name);
                        sami_tag_list_print(tag_list, "Tag End   Pop  OK, Stack Result :");
                        break;
                } END_OF_TAG:;

                free(tag_container);
                free(tag_name);
                tag_po = tag_next_po;
                break;
            default :
                // not exist tag (only text), text is accure in buffer
                text[tag_text_index ++] = *tag_po++;
                break;
        }
    } END_OF_LINE:;

    // check last string
    switch(sami_tag_ass(tag_list, text, ass_buffer)){
        case 0:
            fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "A", __FILE__, __LINE__);
            break;
        case 1:
            strcat(ass_buffer_cat, ass_buffer);
            break;
        default:
            // only text
            fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "C", __FILE__, __LINE__);
            strcat(ass_buffer_cat, text);
            break;
    }

    *ass_text = strdup(ass_buffer_cat);

    return 0;
}/*}}}*/

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
{/*{{{*/
    int sync_index,
        line_index,
        sync_time_line_max = -1;
    List tag_list;
    char *ass_text;

    Subtitle_Devide(argv[1], &sync_time_line_max);

    if(list_init(&tag_list, TAG_STACK_MAX) < 0){
        return -1;
    }

    for(sync_index = 0;sync_index < sync_time_line_max;sync_index ++){
        for(line_index = 0; subtitle_line[sync_index][line_index] != NULL;line_index ++){
            fprintf(stderr, "[%s][%s(%d)] (Sync : %d / Line: %d)   : '%s'\n", __TIME__, __FILE__, __LINE__, sync_index, line_index, subtitle_line[sync_index][line_index]);
            sami_tag_parse(tag_list, subtitle_line[sync_index][line_index], &ass_text);
            fprintf(stderr, "[%s][%s(%d)] %s '%s'\n\n", __TIME__, __FILE__, __LINE__, "Test - Final Result     :", ass_text);
            free(ass_text);
            free(subtitle_line[sync_index][line_index]);
        }

        list_free(tag_list);
        fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "-Stack Clean! (Init All Element, EOL)");
        fprintf(stderr, "[%s][%s(%d)] '%s'\n\n\n", __TIME__, __FILE__, __LINE__, "-------------------------------------");
    }

    fprintf(stderr, "[%s][%s(%d)] '%s'\n", __TIME__, __FILE__, __LINE__, "All Finished");
    return 0;
}/*}}}*/
