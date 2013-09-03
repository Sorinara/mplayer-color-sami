#define _GNU_SOURCE   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include <assert.h>
#include "subreader_sami.h"

#define SAMI_LINE_MAX       512
#define TAG_PROPERTY_MAX    256
#define TAG_STACK_MAX       32

char *subtitle_line[5000][16] = {NULL};

typedef struct _Tag_Property{
    char *name,
         *value;
} Tag_Property;

typedef struct _Tag{
    char *name;
    int property_count;

    Tag_Property property[TAG_PROPERTY_MAX];
} Tag;

/* Remove leading and trailing space */
void trail_space(char *s) {
	int i = 0;
	while (isspace(s[i])) ++i;
	if (i) strcpy(s, s + i);
	i = strlen(s) - 1;
	while (i > 0 && isspace(s[i])) s[i--] = '\0';
}

char* strstrip(char *s)
{
    trail_space(s);
    return strdup(s);
}

void sami_tag_stack_free(Tag *stack)
{
    int i;

    free(stack->name);

    for(i = 0;i < stack->property_count;i ++){
        free(stack->property[i].name);
        free(stack->property[i].value);
    }
}

void sami_tag_stack_free_all(Tag **stack, unsigned int stack_max_size)
{
    int i;

    for(i = 0;i < stack_max_size;i ++){
        if(stack[i] == NULL){
            break;
        }

        sami_tag_stack_free(stack[i]);

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
        puts("Stack Empty");
    }

    if(i + 1 == stack_max_size){
        puts("Stack Full");
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

// dynamic allocation
int sami_tag_stack_push(Tag **stack, unsigned int stack_max_size, Tag *element)
{
    int stack_top;

    // check overflow
    if((stack_top = sami_tag_stack_top(stack, stack_max_size)) == -2){
        return -1;
    }

    stack[stack_top + 1] = (Tag*)calloc(1, sizeof(Tag));
    memcpy(stack[stack_top + 1], element, sizeof(Tag));
    return 0;
}

int sami_tag_stack_pop(Tag **stack, unsigned int stack_max_size, Tag *element)
{
    int stack_top;

    // check underflow
    if((stack_top = sami_tag_stack_top(stack, stack_max_size)) == -1){
        puts("Under flow");
        return -1;
    }
    

    memcpy(element, stack[stack_top], sizeof(Tag));
    free(stack[stack_top]);
    stack[stack_top] = NULL;
    return 0;
}

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

void sami_tag_value_color(char *value, char *save_buffer, char **last_po)
{
    char *start,
         *end;
    int valid_string_length = 0;

    start       = value;
    end         = start + strlen(value);
    *last_po    = end;

    switch(*start){
        case '"' :
            start   += 1;
            end      = strchr(start, '"');
            *last_po = end + 1;
            break;
    }

    if(*start == '#' ){
        start ++;
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

int sami_tag_parse_property(char *tag_body, char *tag_name, Tag *tag_data)
{
    char property_name_buffer[TAG_PROPERTY_MAX], 
         value_parse_buffer[TAG_PROPERTY_MAX],
         *property_po,
         *property_name_last_po     = NULL,
         *property_value_last_po    = NULL;
    int property_count   = 0;

    // 먼저 태그명 부터 복사하자
    tag_data->name = strdup(tag_name);

    property_po = tag_body;

    // <font face='abc' color="0x123456">
    while(*property_po != '\0'){
        // get property (face/color)
        if(strmget(property_po, NULL,  "=",  property_name_buffer, &property_name_last_po) < 0 ){
            // free 
            return -1;
        }

        sami_tag_value_color(property_name_last_po, value_parse_buffer, &property_value_last_po);

        tag_data->property[property_count].name  = strstrip(property_name_buffer);
        tag_data->property[property_count].value = strstrip(value_parse_buffer);
        //printf("N: '%s' '%s'\n", tag_data->property[property_count].name, tag_data->property[property_count].value);
        property_po = property_value_last_po;
        property_count ++;
    }

    tag_data->property_count = property_count;
    return 0;
}

int sami_tag_get(char *tag_po, char *tag_body, char **last_po)
{
    // get all tag
    strmget(tag_po, "<", ">", tag_body, last_po);

    if(strcmp(tag_body, "") == 0){
        return 1;
    }

    return 0;
}

int sami_tag_name_get(char *tag_body, char *tag_name, char **last_po)
{
    // first charecter is '/' -> end(close) tag
    if(*tag_body == '/'){
        sprintf(tag_name, "%s", tag_body);
        return 1;
    }

    // get tag name (font)
    if(strmget(tag_body, NULL, " ", tag_name, last_po) < 0){
        sprintf(tag_name, "%s", tag_body);
    }

    while(**last_po == ' '){
        *last_po++;
    }

    return 0;
} 

// return 0 : no end tag
// return 1 : end tag
int sami_tag_check(char *tag_push_name, char *tag_pop_name, int tag_type)
{
    // dismatch tag_push_name
    if((strcasecmp(tag_push_name, tag_pop_name)) == 0 ){
        return 0;
    }

    return 1;
}

int sami_tag_parse(char *font_tag_string, Tag **tag_stack)
{
    // 이해 데이터들은 초기화 되어야 함.
    char tag_body[SAMI_LINE_MAX]                = {0},
         tag_name[16]                           = {0},
         tag_text[SAMI_LINE_MAX / 2]            = {0},
         ass_buffer_cat_temp[SAMI_LINE_MAX * 3] = {0},
         *tag_po = font_tag_string,
         *tag_last_po,
         *tag_name_last_po;
    int  tag_type        = 0,
         tag_text_index  = 0,
         i;
    Tag  tag_push,
         tag_pop;

    //puts("Intro");
    //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
    //puts("");

    printf("Line    : %s\n", tag_po);
    while(*tag_po != '\0'){
        switch(*tag_po){
            case '<' :
                if(sami_tag_get(tag_po, tag_body, &tag_last_po) != 0){
                    puts("ER1");
                    break;
                }

                if((tag_type = sami_tag_name_get(tag_body, tag_name, &tag_name_last_po)) < 0){
                    puts("ER2");
                    break;
                }

                /*
                //printf("Ha? : '%s' '%s' '%s'\n", tag_body, tag_name, tag_last_po);
                if(strcasecmp(tag_name, "font") != 0){
                    break;
                }  */

                switch(tag_type){
                    case 0:
                        if(strcmp(tag_text, "") != 0){

                        }

                        if(sami_tag_parse_property(tag_name_last_po, tag_name, &tag_push) < 0){
                            puts("ER3");
                            break;
                        }

                        //printf("Push     : '%s' '%s'\n", tag_push.name, tag_body);

                        // ignore overflow
                        //puts("Stack push");
                        //puts("Before");
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        sami_tag_stack_push(tag_stack, TAG_STACK_MAX, &tag_push);
                        printf("After (add '%s')\n", tag_name);
                        //sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        //puts("");
                        break;
                    case 1:
                        puts("Stack pop");
                        puts("Before");
                        sami_tag_stack_print(tag_stack, TAG_STACK_MAX);
                        sami_tag_stack_pop(tag_stack, TAG_STACK_MAX, &tag_pop);
                        printf("After (delete '%s')\n", tag_name);
                        sami_tag_stack_print(tag_stack, TAG_STACK_MAX);

                        printf("Next: '%s'\n", tag_last_po);
                        puts("");

                        //바로 이전태그의 시작과 지금태그(끝)이 같아야 함.
                        if(sami_tag_check(tag_name, tag_pop.name, tag_type) < 1){
                            printf("DF : '%s' '%s'\n", tag_push.name, tag_pop.name);
                            assert(1);
                            exit(100);
                            break;
                        }

                        /*
                        for(i = 0;i < tag_pop.property_count;i ++){
                            printf("Property : (%d/%d) '%s' '%s'\n", i, tag_pop.property_count, tag_pop.property[i].name, tag_pop.property[i].value);
                        }   */

                        // color 정보 가져옴 (rt, face는 아직...)
                        //sami_tag_font_get();
                        /*
                        sami_tag_ass(ass_buffer, tag_font_face, tag_font_color, tag_text);
                        memset(&tag_font_face, 0, sizeof(tag_font_face));
                        memset(&tag_font_color, 0, sizeof(tag_font_color));
                        memset(&tag_text, 0, sizeof(tag_text));
                        tag_text_index  = 0; */
                        break;
                    case 2:
                        puts("Type Error 2?");
                        break;
                }

                // 밑에 tag_po++가 있어서 미리 한번 빼줘야 함.
                tag_po = tag_last_po - 1;
                break;
            default :
                // no include tag, accure in buffer
                tag_text[tag_text_index++] = *tag_last_po++;
                break;
        }
        tag_po++;
    }

    puts("- BR Line -");
    // for no tag string
    /*
    if(strcmp(tag_text, "") != 0){
        sami_tag_ass(ass_buffer_cat_temp, tag_font_face, tag_font_color, tag_text);
    } */

    //free(font_tag_string);
    //*ass_buffer_cat = strdup(ass_buffer_cat_temp);
    return 0;
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
    Tag *tag_stack[TAG_STACK_MAX] = {NULL};

    Subtitle_Devide(argv[1], &sync_time_line_max);

    for(j = 0;j < sync_time_line_max;j ++){
        for(i = 0; subtitle_line[j][i] != NULL;i ++){
            sami_tag_parse(subtitle_line[j][i], tag_stack);
        }
        sami_tag_stack_free_all(tag_stack, TAG_STACK_MAX);
    }

    return 0;
}
