#ifndef UTIL
#define UTIL

char *strstrip(char *s);
void echo_on();
void echo_off();
int is_empty(char * text);
void clear_stdin();
int get_line(char *buffer, int buffer_size);

#endif
