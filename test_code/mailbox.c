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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Pr{
    char *name,
         *value;
} Property;

int MailBox_Check(char *name, Property *property_array, int property_count)
{
    int i,
        flag = 0;

    for(i = 0;i < property_count;i++){
        if(strcmp(name, property_array[i].name) == 0){
            flag = 1;
            break;
        }
    }

    if(flag == 0){
       return -1; 
    }
    
    return i;
}

int main(int argc, const char *argv[])
{
    int i,
        j,
        mailbox_i,
        name_flag   = 0,
        value_flag  = 0,
        new_counter = 0;
    char *dong_a[3][2]  = {{"mangpo",    "0"},
                           {"youngtong", "1"},
                           {"shin",      "2"}},
         *dong_b[3][2]  = {{"gocksun",   "3"},
                           {"mangpo",    "4"},
                           {"saeryu",    "5"}};
    Property property_array_a[30],
             property_array_b[30],
             property_array_new[30];

    for(i = 0;i < 3;i++){
        property_array_a[i].name  = dong_a[i][0];
        property_array_a[i].value = dong_a[i][1];
        property_array_b[i].name  = dong_b[i][0];
        property_array_b[i].value = dong_b[i][1];
    }

    for(i = 0;i < 3;i++){
        for(j = 0;j <3;j++){
            name_flag = 0;
            fprintf(stderr, "'%s(%s)' '%s(%s)'\n", property_array_a[i].name, property_array_a[i].value, property_array_b[j].name, property_array_b[j].value);

            if(strcmp(property_array_a[i].name, property_array_b[j].name) == 0){
                if(strcmp(property_array_a[i].value, property_array_b[j].value) != 0){
                    fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "Match in", __FILE__, __LINE__);
                    mailbox_i = MailBox_Check(property_array_b[j].name, property_array_new, new_counter);

                    if( mailbox_i < 0){
                        property_array_new[new_counter].name  = property_array_b[j].name;
                        property_array_new[new_counter].value = property_array_b[j].value;
                        new_counter ++;
                    }else{
                        property_array_new[mailbox_i].value = property_array_b[j].value;
                    }

                    value_flag = 1; 
                }
               name_flag = 1; 
            }
           
            if(name_flag == 0){
                mailbox_i = MailBox_Check(property_array_a[i].name, property_array_new, new_counter);
                fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "Not Match Add", __FILE__, __LINE__);

                if( mailbox_i < 0){
                    property_array_new[new_counter].name  = property_array_b[j].name; 
                    property_array_new[new_counter].value = property_array_b[j].value; 
                    new_counter ++;
                }else{
                    property_array_new[mailbox_i].value = property_array_b[j].value;
                }
            }
        }
    }
    
    for(i = 0;i < new_counter;i++){
        fprintf(stderr, "[%s] DEBUG LINE '%s' - (%s, Line:%d)\n", __TIME__, "Data", __FILE__, __LINE__);
        printf("Data (%d): '%s' '%s'\n", new_counter, property_array_new[i].name, property_array_new[i].value);
    }
    
    return 0;
}
