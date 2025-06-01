#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LINE_LEN 1024
#define TABLE_LEN 64 
#define COLUNM_LEN 64 
#define MAX_COLUNM_NUM 64 
#define IS_CREATE_TABLE(str) (strncmp(str, "CREATE TABLE", 12) == 0)

typedef struct {
  char name[COLUNM_LEN];
  char type[COLUNM_LEN];
} Column;

const char* SqlToCType(const char* sql_type) {
  if (strncmp(sql_type, "INT", 3) == 0 || strncmp(sql_type, "INTEGER", 7) == 0) return "int";
  if (strncmp(sql_type, "SMALLINT", 8) == 0) return "short";
  if (strncmp(sql_type, "BIGINT", 6) == 0) return "long long";
  if (strncmp(sql_type, "UUID", 4) == 0) return "char*";
  if (strncmp(sql_type, "VARCHAR", 7) == 0) return "char*";
  if (strncmp(sql_type, "CHAR", 4) == 0) return "char*";
  if (strncmp(sql_type, "TEXT", 4) == 0) return "char*";
  if (strncmp(sql_type, "TIMESTAMP", 9) == 0) return "char*";
  if (strncmp(sql_type, "DATE", 4) == 0) return "char*";
  if (strncmp(sql_type, "TIME", 4) == 0) return "char*";
  if (strncmp(sql_type, "BOOLEAN", 7) == 0 || strncmp(sql_type, "BOOL", 4) == 0) return "bool";
  if (strncmp(sql_type, "FLOAT", 5) == 0) return "float";
  if (strncmp(sql_type, "DOUBLE", 6) == 0) return "double";
  if (strncmp(sql_type, "DECIMAL", 7) == 0 || strncmp(sql_type, "NUMERIC", 7) == 0) return "double";
  if (strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0) return "void*";
  if (strncmp(sql_type, "BYTEA", 5) == 0) return "void*";
  if (strncmp(sql_type, "ARRAY", 5) == 0) return "void*";

  // Default fallback
  return "void*";
}

char* SkipSpaces(char* p) {
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

int GetWord(char* line, char* word)
{
  int res = 0;
  while (*line != ' ' && *line != ',' && *line != '\0')
  {
    *word = *line;
    line++; 
    word++;
    res++;
  }
  *word = '\0';
  return res;
}

int main()
{
  FILE *fp; 
  char table_name[TABLE_LEN];
  char line[LINE_LEN];
  Column columns[MAX_COLUNM_NUM];
  int curr_column_num = 0;
  char *curr;
  int8_t is_in_table = 0;

  fp = fopen("test.sql", "r");

  while(fgets(line, LINE_LEN, fp))
  {
   if (IS_CREATE_TABLE(line))
   {
     GetWord(line+13, table_name);
     printf("table name: %s\n", table_name);
     is_in_table = 1;
     continue;
   }

   if (is_in_table)
   {
     char* skipped = SkipSpaces(line);
     if (*skipped=='\n' || *skipped==')') continue;
     int length = GetWord(skipped, columns[curr_column_num].name);
     if (strncmp(columns[curr_column_num].name, "--", 2)==0) continue;
     GetWord(SkipSpaces(skipped+length), columns[curr_column_num].type);
     curr_column_num++;
   }
  }

  printf("struct %s {\n", table_name);
  for (int i = 0; i < curr_column_num; i++) {
    printf("    %s %s;\n", SqlToCType(columns[i].type), columns[i].name);
  }
  printf("};\n");
  return 0;
}
