//
//  JsonCustom.c
//  SmartHome
//
//  Created by Tran Xuan on 8/30/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#include "JsonCustom.h"
#include "Lib/Json/cJSON.h"

char* get_Json_String(char* key,char *text){
    char* out;
    cJSON *json;
    json = cJSON_Parse(text);
    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    } else {
        out = cJSON_PrintUnformatted(cJSON_GetObjectItem(json,key));
        out[strlen(out)-1]=out[strlen(out)];
        out = out+1;
        printf("%s\n", out);
        cJSON_Delete(json);
        //free(out);
    }
    return out;
}

void Parse_Json(char *text)
{
    char *out;
    cJSON *json;
    
    json = cJSON_Parse(text);
    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    } else {
        //out = cJSON_Print(json);
        out = cJSON_Print(cJSON_GetObjectItem(json,"ssid"));
        printf("%s\n", out);
        out = cJSON_Print(cJSON_GetObjectItem(json,"pass"));
        cJSON_Delete(json);
        printf("%s\n", out);
        free(out);
    }
}
