/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */

#ifndef STRINGUTIL_H
#define STRINGUTIL_H

static char*
concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

static char**
string_split(char* a_str, char a_delim)
{
    char** result       = 0;
    size_t count        = 0;
    char* tmp           = a_str;
    char* last_comma    = 0;
    char delim[2];
    delim[0]            = a_delim;
    delim[1]            = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        //TODO strtok to strsep(3)
        char* token = strtok(a_str, delim);

        while (token) {
            if(idx >= count)
                return NULL;
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }

        if(idx != count -1)
            return NULL;
        *(result + idx) = 0;
    }

    return (char**)result;
}

static char* // NULL not found
string_list_contains(char** list, char* val) {

    for (int i = 0; *(list + i); i++) {

        if(strcmp(*(list + i), val) == 0) {
            return *(list + i);
        }
    }
    return NULL;
}

static void
string_list_dispose(char** list) {

    for (int i = 0; *(list + i); i++) {
        free(*(list + i));
    }
    free(list);
}

#endif /* STRINGUTIL_H */
