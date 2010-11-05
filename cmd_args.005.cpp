#ifdef _MSC_VER
    #include "gtr/src/lang/gtr_config.h"
#endif
#include <stdio.h>


#pragma runtime_checks("s", off)


int my_ischar(char c)
{
    if (c < '0') return 0;
    if (c > '9') return 0;
    return 1;
}

int my_atoi(char * str)
{
    int result = 0;

    int i = 0;
    while (str[i] != 0) {

        if (!my_ischar(str[i])) break;

        int digit = (int) (str[i] - '0');

        result = result * 10 + digit;

        i++;
    }

    return result;
}

int my_str_cmp(char * s1, char * s2)
{
    int i;
    for (i = 0; (s1[i] != 0) && (s2[i] != 0); i++) {
        if (s1[i] != s2[i]) {
            return 0;
        }
    }
    
    if ((s1[i] != 0) || (s2[i] != 0))
        return 0;

    return 1;
}


// date January 10, 1973 


struct Date {
    int day;
    int month;
    int year;
};


int lookup_month(char * month)
{
    if (my_str_cmp(month, "January")) return 1;
    if (my_str_cmp(month, "February")) return 2;
    if (my_str_cmp(month, "March")) return 3;
    if (my_str_cmp(month, "April")) return 4;
    if (my_str_cmp(month, "May")) return 5;
    if (my_str_cmp(month, "June")) return 6;
    if (my_str_cmp(month, "July")) return 7;
    if (my_str_cmp(month, "August")) return 8;
    if (my_str_cmp(month, "September")) return 9;
    if (my_str_cmp(month, "October")) return 10;
    if (my_str_cmp(month, "November")) return 11;
    if (my_str_cmp(month, "December")) return 12;
    return 0;
}


int parse_date(char * str, Date * d)
{
    int curr_index = 0;

    // extract month
    int i = 0;
    char month[11];
    for(i = 0; (str[curr_index] != ' '); i++, curr_index++) {
        if (str[curr_index] == '\0') 
            return 0;
        if (i >= 10)
            return 0;
        month[i] = str[curr_index];
    }

    // terminate month
    month[i] = '\0';

    // lookup month
    int m = lookup_month(month);
    if (m == 0) return 0;
    d->month = m;

    // skip ' '
    curr_index++;

    char date[3];
    int j = 0;
    while (str[curr_index] != ',') {
        if (!my_ischar(str[curr_index])) return 0;
        if(j >= 2) return 0;
        date[j] = str[curr_index];
        curr_index++;
        j++;
    }

    // terminate date
    date[j] = '\0';

    // get date [check?]
    int day = my_atoi(date);
    d->day = day;

    // skip ',' and ' '
    curr_index++;
    if (str[curr_index++] != ' ') return 0;

    // get year
    char year[5];
    int k = 0;
    while (str[curr_index] != '\0') {
        if (!my_ischar(str[curr_index])) return 0;
        if(k >= 4) return 0;
        year[k] = str[curr_index];
        curr_index++;
        k++;
    }

    // terminate date
    year[k] = '\0';

    // get year
    int y = my_atoi(year);
    d->year = y;

    return 1;
}

int main(int argc, char * argv[])
{
    if (argc != 2) return 0;

    Date d;
    int r;
    if (parse_date(argv[1], &d)) {
        
        // if date is February 13, 2009 do something
        if ((d.day == 13) && (d.month == 2) && (d.year == 2009)) {
//            printf ("Opa");
            r = 2;
        }
        else {
            r = 1;
        }
    }
    else {
        r = 0;
    }

    return r;
}
