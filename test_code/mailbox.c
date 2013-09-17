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

typedef struct _Pr{
    char *name,
         *value;
} Property;

int Property_Check(Property *property, char *name)
{
    int i,
        flag = 0;

    for(i = 0;property[i].name != NULL;i ++){
        if(strcasecmp(name, property[i].name) == 0){
            flag = 1;
            break;
        }
    }

    if(flag == 0){
       return -1; 
    }
    
    return i;
}

int Property_Update(Property *mailbox_source, Property *mailbox_new, int *mailbox_new_counter)
{
    int i,
        mailbox_i;

    for(i = 0;mailbox_source[i].name != NULL;i++){
        //fprintf(stderr, "[%s] DEBUG LINE '(%d)%s' - (%s, Line:%d)\n", __TIME__, i, mailbox_source[i].name, __FILE__, __LINE__);
        
        // new
        if((mailbox_i = Property_Check(mailbox_new, mailbox_source[i].name)) < 0){
            mailbox_new[(*mailbox_new_counter)].name  = mailbox_source[i].name;
            mailbox_new[(*mailbox_new_counter)].value = mailbox_source[i].value;
            (*mailbox_new_counter) ++;
        // overwrite
        }else{
            mailbox_new[mailbox_i].value = mailbox_source[i].value;
        }
    }

    return *mailbox_new_counter;
}

int main(int argc, const char *argv[])
{
    int i,
        mailbox_new_max = 0;
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
    Property mailbox_a[30],
             mailbox_b[30],
             mailbox_new[30] = {{NULL, NULL}};

    for(i = 0;i < 7;i++){
        mailbox_a[i].name  = dong_a[i][0];
        mailbox_a[i].value = dong_a[i][1];
        mailbox_b[i].name  = dong_b[i][0];
        mailbox_b[i].value = dong_b[i][1];
    }

    //mailbox_new_max = Property_Update(mailbox_a, mailbox_new, &mailbox_new_max);
    memcpy(mailbox_new, mailbox_a, sizeof(mailbox_a));
    mailbox_new_max = 7;
    mailbox_new_max = Property_Update(mailbox_b, mailbox_new, &mailbox_new_max);

    for(i = 0;i < mailbox_new_max;i++){
        fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "Data", __FILE__, __LINE__);
        printf("Data (%d): '%s' '%s'\n", mailbox_new_max, mailbox_new[i].name, mailbox_new[i].value);
    }
    
    return 0;
}
