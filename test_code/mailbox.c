/*
 * =====================================================================================
 *
 *       Filename:  aa.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013년 09월 16일 15시 10분 02초
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#define _GNU_SOURCE   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_PROPERTY_MAX    256

typedef struct _Pr{
    char *name,
         *value;
} Tag_Property;

typedef struct _Tag{
    char *name;
    int  property_count;

    Tag_Property property[TAG_PROPERTY_MAX];
} Tag;

int sami_tag_stack_property_check(Tag *tag_source,char *name, int *stack_top)
{
    int i,
        flag = 0;

    for(i = 0;i < tag_source->property_count;i ++){
        if(strcasecmp(tag_source->property[i].name, name) == 0){
            flag = 1;
            break;
        }
    }

    if(flag == 0){
        i = *stack_top;
        (*stack_top) ++;
    }

    return i;
}

int sami_tag_stack_property_combine(Tag *tag_source, Tag *tag_target, Tag *tag_new)
{
    int i,
        new_tag_i,
        new_tag_stack_top = 0;

    if(strcasecmp(tag_source->name, tag_target->name) != 0){
        return -1;
    }

    new_tag_stack_top = tag_source->property_count;
    memcpy(tag_new->property, tag_source->property, new_tag_stack_top * sizeof(Tag_Property));

    for(i = 0;i < tag_target->property_count;i++){
        // if match in tag_target->property[i].name, return matched index
        // not, return to new_tag_stack_top (and ++, for future)
        new_tag_i = sami_tag_stack_property_check(tag_source, tag_target->property[i].name, &new_tag_stack_top);
        //fprintf(stderr, "[%s] DEBUG LINE (%d) '%s' : %s (%s, Line:%d)\n", __TIME__, new_tag_i, tag_target->property[i].name, tag_target->property[i].value, __FILE__, __LINE__);
        tag_new->property[new_tag_i].name  = tag_target->property[i].name;
        tag_new->property[new_tag_i].value = tag_target->property[i].value;
    }

    tag_new->property_count = new_tag_stack_top;
    return 0;
}

int main(int argc, const char *argv[])
{
    int i;
    char *dong_a[][2]  = {{"mangpo",        "0"},
                          {"youngtong",     "1"},
                          {"shin",          "2"},
                          {"omockchoun",    "3"},
                          {"gyo",           "4"},
                          {"gocksun",       "5"},
                          {NULL,            NULL}},
         *dong_b[][2]  = {{"gocksun",       "10"},
                          {"mangpo",        "11"},
                          {"saeryu",        "12"},
                          {"gwangkyo",      "13"},
                          {"shin",          "14"},
                          {"maekyo",        "15"},
                          {NULL,            NULL}};
    Tag *tag_font_a     = NULL,
        *tag_font_b     = NULL,
        *tag_font_new   = NULL;
    
    if((tag_font_a = malloc(sizeof(Tag))) == NULL) {
        fprintf(stderr, "\ndynamic memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    if((tag_font_b = malloc(sizeof(Tag))) == NULL) {
        fprintf(stderr, "\ndynamic memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    if((tag_font_new = malloc(sizeof(Tag))) == NULL) {
        fprintf(stderr, "\ndynamic memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for(i = 0;i < 7;i++){
        tag_font_a->property[i].name  = dong_a[i][0];
        tag_font_a->property[i].value = dong_a[i][1];
        tag_font_b->property[i].name  = dong_b[i][0];
        tag_font_b->property[i].value = dong_b[i][1];
    }

    tag_font_a->name = "font";
    tag_font_a->property_count = 6;
    tag_font_b->name = "font";
    tag_font_b->property_count = 6;

    sami_tag_stack_property_combine(tag_font_a, tag_font_b, tag_font_new);

    for(i = 0;i < tag_font_new->property_count;i++){
        //fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "Data", __FILE__, __LINE__);
        printf("Data (%d): '%s' '%s'\n", i, tag_font_new->property[i].name, tag_font_new->property[i].value);
    }
    
    free(tag_font_a);
    tag_font_a = NULL;

    free(tag_font_b);
    tag_font_b = NULL;

    free(tag_font_new);
    tag_font_new = NULL;

    return 0;
}
