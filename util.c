
int is_empty(char * buffer)
{
  int i;
  for(i = 0; buffer[i] != 0; i++){
    if(buffer[i] != '\n' || buffer[i] != ' ')
      return 0;
  }
  
  return 1;
}
