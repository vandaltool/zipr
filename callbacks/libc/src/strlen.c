
int strlen(char *str)
{
        const char *s;

        for (s = str; *s; ++s)
                ;
        return (s - str);
}

