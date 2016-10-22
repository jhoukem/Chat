
int is_empty(char * text)
{
  int i;
  for(i = 0; text[i] != 0; i++){
    if(text[i] != ' ')
      return 0;
  }
  
  return 1;
}
