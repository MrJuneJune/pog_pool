#include "pog_pool/auto_generate.h"


const char* SqlToCType(const char *sql_type)
{
  // Handle arrays first
  if (strstr(sql_type, "[]") != NULL)
    return "char*"; // Use char* to hold PostgreSQL array like "{1,2,3}"

  // Prefix matches for known types
  if (strncmp(sql_type, "SMALLINT", 8) == 0) return "short";
  if (strncmp(sql_type, "INTEGER", 7) == 0 || strncmp(sql_type, "INT", 3) == 0) return "int";
  if (strncmp(sql_type, "BIGINT", 6) == 0) return "long long";

  if (strncmp(sql_type, "DECIMAL", 7) == 0 || strncmp(sql_type, "NUMERIC", 7) == 0) return "double";
  if (strncmp(sql_type, "REAL", 4) == 0) return "float";
  if (strncmp(sql_type, "DOUBLE", 6) == 0) return "double";
  if (strncmp(sql_type, "SERIAL", 6) == 0) return "void*";  // PostgreSQL auto-increment

  if (strncmp(sql_type, "UUID", 4) == 0) return "char*";

  if (strncmp(sql_type, "CHAR", 4) == 0 || strncmp(sql_type, "VARCHAR", 7) == 0 || strncmp(sql_type, "TEXT", 4) == 0)
    return "char*";

  if (strncmp(sql_type, "DATE", 4) == 0 || strncmp(sql_type, "TIME", 4) == 0 ||
      strncmp(sql_type, "TIMESTAMP", 9) == 0 || strncmp(sql_type, "TIMESTAMPTZ", 11) == 0)
    return "char*";

  if (strncmp(sql_type, "BOOLEAN", 7) == 0 || strncmp(sql_type, "BOOL", 4) == 0) return "bool";

  if (strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0) return "char*";
  if (strncmp(sql_type, "BYTEA", 5) == 0) return "char*";

  // Treat enums and unknown types as strings
  return "char*";
}

char* SkipSpaces(char *p)
{
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

int GetWord(char *line, char *word)
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

void CreateCRUDFiles(
  const char *table_name,
  const Column *columns,
  size_t column_sizes,
  const char *output_folder
)
{
  // TODO: Make this dynamic by counting length of the file?
  char file_name[1024];

  // Create header file
  snprintf(file_name, sizeof(file_name), "%s/model_%s.h", output_folder, table_name);
  printf("%s\n", file_name);
  FILE *file_out_p = fopen(file_name, "w");
  if (!file_out_p)
  {
    perror("fopen");
    return;
  }

  fprintf(file_out_p, "#ifndef MODEL_%s\n", table_name);
  fprintf(file_out_p, "#define MODEL_%s\n\n", table_name);
  fprintf(file_out_p, "#include <postgresql/libpq-fe.h>\n#include <stdbool.h>\n\n");
  fprintf(file_out_p, "typedef struct {\n");
  for (size_t i = 0; i < column_sizes; i++)
  {
    const char *c_type = SqlToCType(columns[i].type);
    fprintf(file_out_p, "  %s %s;\n", c_type, columns[i].name);
  }
  fprintf(file_out_p, "} %s;\n\n",table_name);

  fprintf(file_out_p, "typedef struct {\n");
  fprintf(file_out_p, "  %s* %s;\n", table_name, table_name);
  fprintf(file_out_p, "  ExecStatusType status;\n");
  fprintf(file_out_p, "} %sQuery;\n\n", table_name);

  fprintf(file_out_p, "%sQuery Query%s(PGconn *conn, const char *where_clause);\n", table_name, table_name);
  fprintf(file_out_p, "void Insert%s(PGconn *conn, %s u);\n", table_name, table_name);
  fprintf(file_out_p, "void Update%s(PGconn *conn, %s u, const char *where_clause);\n", table_name, table_name);
  fprintf(file_out_p, "void Delete%s(PGconn *conn, const char *where_clause);\n\n", table_name);
  fprintf(file_out_p, "char* Serialize%s(%s u);\n", table_name, table_name);
  fprintf(file_out_p, "#endif // MODEL_%s\n", table_name);
  fclose(file_out_p);

  // Create source file 
  snprintf(file_name, sizeof(file_name), "%s/model_%s.c", output_folder, table_name);
  file_out_p = fopen(file_name, "w");
  if (!file_out_p)
  {
    perror("fopen");
    return;
  }

  // Build column list and format string
  char column_names[512] = {0};
  char format_parts[512] = {0};
  char value_args[512] = {0};

  strcat(column_names, "(");
  for (size_t i = 0; i < column_sizes; i++)
  {
    const char *name = columns[i].name;
    const char *c_type = SqlToCType(columns[i].type);
  
    // Skip SERIAL (Postgres will auto-generate)
    if (strcmp(columns[i].type, "SERIAL") == 0)
      continue;
  
    strcat(column_names, name);
  
    // Format string
    if (strcmp(c_type, "short") == 0 || strcmp(c_type, "int") == 0)
    {
      strcat(format_parts, "%d");
    }
    else if (strcmp(c_type, "long long") == 0)
    {
      strcat(format_parts, "%lld");
    }
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
    {
      strcat(format_parts, "%f");
    }
    else if (strcmp(c_type, "bool") == 0)
    {
      strcat(format_parts, "%s");
    }
    else
    {
      strcat(format_parts, "'%s'");
    }
  
    // Add value argument
    char tmp[64];
    if (strcmp(c_type, "void*") == 0)
    {
      snprintf(tmp, sizeof(tmp), "(char*)u.%s", name);
    }
    else if (strcmp(c_type, "bool") == 0)
    {
      snprintf(tmp, sizeof(tmp), "u.%s ? \"TRUE\" : \"FALSE\"", name);
    }
    else
    {
      snprintf(tmp, sizeof(tmp), "u.%s", name);
    }
  
    strcat(value_args, tmp);
  
    // Trailing commas if not last
    if (i != column_sizes - 1)
    {
      strcat(column_names, ", ");
      strcat(format_parts, ", ");
      strcat(value_args, ", ");
    }
  }
  strcat(column_names, ")");

  // Build SET clause and format string
  char set_clause[1024] = {0};
  char update_args[1024] = {0};
  
  for (size_t i = 0, written = 0; i < column_sizes; i++)
  {
    const char *name = columns[i].name;
    const char *c_type = SqlToCType(columns[i].type);
  
    // skip auto-generated
    if (strncmp(columns[i].type, "SERIAL", 6) == 0) continue;
  
    // --- SET clause formatting ---
    char set_line[64];
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%d", name);
    } else if (strcmp(c_type, "long long") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%lld", name);
    } else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%f", name);
    } else if (strcmp(c_type, "bool") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%s", name);  // will pass "TRUE"/"FALSE"
    } else {
      snprintf(set_line, sizeof(set_line), "%s='%%s'", name);
    }
  
    // --- Value argument formatting ---
    char arg_line[64];
    if (strcmp(c_type, "bool") == 0) {
      snprintf(arg_line, sizeof(arg_line), "u.%s ? \"TRUE\" : \"FALSE\"", name);
    } else if (strcmp(c_type, "void*") == 0) {
      snprintf(arg_line, sizeof(arg_line), "(char*)u.%s", name);
    } else {
      snprintf(arg_line, sizeof(arg_line), "u.%s", name);
    }
  
    // --- Append to buffer ---
    if (written > 0) {
      strcat(set_clause, ", ");
      strcat(update_args, ", ");
    }
  
    strcat(set_clause, set_line);
    strcat(update_args, arg_line);
    written++;
  }

  fprintf(file_out_p, "#include \"model_%s.h\"\n", table_name);
  fprintf(file_out_p, "#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n\n");

  // QUERY function
  fprintf(file_out_p, "%sQuery Query%s(PGconn *conn, const char *where_clause)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  %sQuery query_result;\n", table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query), \"SELECT * FROM %s WHERE %%s;\", where_clause);\n", table_name);
  fprintf(file_out_p, "  PGresult *res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  ExecStatusType status = PQresultStatus(res);");
  fprintf(file_out_p, "  query_result.%s = NULL;\n", table_name);
  fprintf(file_out_p, "  query_result.status = status;");
  fprintf(file_out_p, "  if (status != PGRES_TUPLES_OK)\n  {\n");
  fprintf(file_out_p, "    fprintf(stderr, \"SELECT failed: %%s\\n\", PQerrorMessage(conn));\n");
  fprintf(file_out_p, "    PQclear(res);\n");
  fprintf(file_out_p, "    return query_result;\n");
  fprintf(file_out_p, "  }\n");
  
  fprintf(file_out_p, "  int rows = PQntuples(res);\n");

  fprintf(file_out_p, "   if (rows == 0)\n");
  fprintf(file_out_p, "   {\n");
  fprintf(file_out_p, "     return query_result;\n");
  fprintf(file_out_p, "   }\n");
  fprintf(file_out_p, "  %s* list = malloc(rows * sizeof(%s));\n", table_name, table_name);
  fprintf(file_out_p, "  for (int i = 0; i < rows; ++i)\n  {\n");
  
  for (size_t i = 0; i < column_sizes; i++) {
    const char *name = columns[i].name;
    const char *c_type = SqlToCType(columns[i].type);
  
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0 || strcmp(c_type, "long long") == 0)
    {
      fprintf(file_out_p, "    list[i].%s = atoi(PQgetvalue(res, i, %zu));\n", name, i);
    } 
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
    {
      fprintf(file_out_p, "    list[i].%s = atof(PQgetvalue(res, i, %zu));\n", name, i);
    }
    else
    {
      fprintf(file_out_p, "    list[i].%s = strdup(PQgetvalue(res, i, %zu));\n", name, i);
    }
  }
  
  fprintf(file_out_p, "  }\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "  query_result.%s = list;", table_name);
  fprintf(file_out_p, "  return query_result;");
  fprintf(file_out_p, "}\n\n");

  // INSERT function
  fprintf(file_out_p, "void Insert%s(PGconn *conn, %s u)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"INSERT INTO %s %s \"\n    \"VALUES (%s);\",\n", table_name, column_names, format_parts);
  fprintf(file_out_p, "    %s);\n", value_args);
  fprintf(file_out_p, "  PGresult *res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  if (PQresultStatus(res) != PGRES_COMMAND_OK) {\n");
  fprintf(file_out_p, "    fprintf(stderr, \"Insert failed: %%s\\n\", PQerrorMessage(conn));\n");
  fprintf(file_out_p, "  }\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n\n");

  // UPDATE function
  fprintf(file_out_p, "void Update%s(PGconn *conn, %s u, const char *where_clause)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"UPDATE %s SET %s WHERE %%s;\",\n", table_name, set_clause);
  fprintf(file_out_p, "    %s, where_clause);\n", update_args);
  fprintf(file_out_p, "  PGresult *res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  if (PQresultStatus(res) != PGRES_COMMAND_OK) {\n");
  fprintf(file_out_p, "    fprintf(stderr, \"Update failed: %%s\\n\", PQerrorMessage(conn));\n");
  fprintf(file_out_p, "  }\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n\n");

  // DELETE stub
  fprintf(file_out_p, "void Delete%s(PGconn *conn, const char *where_clause)\n{\n", table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"DELETE FROM %s WHERE %%s;\",\n", table_name);
  fprintf(file_out_p, "    where_clause);\n");
  fprintf(file_out_p, "  PGresult *res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  if (PQresultStatus(res) != PGRES_COMMAND_OK) {\n");
  fprintf(file_out_p, "    fprintf(stderr, \"Delete failed: %%s\\n\", PQerrorMessage(conn));\n");
  fprintf(file_out_p, "  }\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n\n");

  // Serialize stub
  fprintf(file_out_p, "char* Serialize%s(%s u)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  char *buffer = malloc(%d);\n", QUERY_BUFFER * 2);
  fprintf(file_out_p, "  if (!buffer) return NULL;\n");
  fprintf(file_out_p, "  snprintf(buffer, %d,\n", QUERY_BUFFER * 2);
  fprintf(file_out_p, "    \"{");
  for (size_t i = 0; i < column_sizes; i++) {
    const char *name = columns[i].name;
    const char *c_type = SqlToCType(columns[i].type);
    const char *sql_type = columns[i].type;
  
    // JSON key
    fprintf(file_out_p, "\\\"%s\\\":", name);
  
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0) {
      fprintf(file_out_p, "%%d");
    } else if (strcmp(c_type, "long long") == 0) {
      fprintf(file_out_p, "%%lld");
    } else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0) {
      fprintf(file_out_p, "%%f");
    } else if (strcmp(c_type, "bool") == 0) {
      fprintf(file_out_p, "%%s"); // will insert "true" or "false"
    } else if (strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0) {
      fprintf(file_out_p, "%%s"); // inline raw json
    } else if (strstr(sql_type, "[]") != NULL) {
      fprintf(file_out_p, "%%s"); // formatted as a JSON array
    } else {
      fprintf(file_out_p, "\\\"%%s\\\""); // wrap plain strings
    }
  
    if (i != column_sizes - 1) fprintf(file_out_p, ", ");
  }
  fprintf(file_out_p, "}\",\n");
  for (size_t i = 0; i < column_sizes; i++) {
    const char *name = columns[i].name;
    const char *c_type = SqlToCType(columns[i].type);
    const char *sql_type = columns[i].type;
  
    if (strcmp(c_type, "bool") == 0) {
      fprintf(file_out_p, "    u.%s ? \"true\" : \"false\"", name);
    } else if (strstr(sql_type, "[]") != NULL) {
      // inline conversion: {1,2,3} -> [1,2,3]
      fprintf(file_out_p, "    ({ const char *src = u.%s; size_t len = strlen(src); char *tmp = malloc(len + 3); if (!tmp) tmp = \"[]\"; else { tmp[0] = '['; strncpy(tmp + 1, src + 1, len - 2); tmp[len - 1] = ']'; tmp[len] = '\\0'; } tmp; })", name);
    } else if (strcmp(c_type, "void*") == 0 || strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0) {
      fprintf(file_out_p, "    (char*)u.%s", name);
    } else {
      fprintf(file_out_p, "    u.%s", name);
    }
  
    if (i != column_sizes - 1) fprintf(file_out_p, ",\n");
  }
  fprintf(file_out_p, "  );\n  return buffer;\n}\n");
  fclose(file_out_p);
} 

void CreateCRUDFilesFromSQL(
  const char *sql_file_name,
  const char *output_folder
)
{
  FILE *fp; 
  char table_name[TABLE_LEN];
  char line[LINE_LEN];
  Column columns[MAX_COLUNM_NUM];
  int curr_column_num = 0;
  char *curr;
  int8_t is_in_table = 0;

  fp = fopen(sql_file_name, "r");

  while(fgets(line, LINE_LEN, fp))
  {
   if (IS_CREATE_TABLE(line))
   { 
     GetWord(line+CREATE_TABLE+14, table_name);
     is_in_table = 1;
     continue;
   }

   if (is_in_table)
   {
     char *skipped = SkipSpaces(line);
     if (*skipped=='\n' || *skipped==')') continue;
     int length = GetWord(skipped, columns[curr_column_num].name);
     if (strncmp(columns[curr_column_num].name, "--", 2)==0) continue;
     GetWord(SkipSpaces(skipped+length), columns[curr_column_num].type);
     curr_column_num++;
   }
  }
  CreateCRUDFiles(table_name, columns, curr_column_num, output_folder);
}

void SetModelHeader(FILE *models_header)
{
  fprintf(models_header, "#ifndef MODELS_H\n#define MODELS_H\n\n");
}

pid_t* PIDStatus(
  const char *model_dir,
  const struct dirent *entry,
  FILE *models_header,
  const char *output_folder
)
{
   char full_path[512];
   snprintf(full_path, sizeof(full_path), "%s/%s", model_dir, entry->d_name);
   pid_t pid = fork();

	 switch (pid)
   {
	   case -1:
	       perror("fork");
	       exit(EXIT_FAILURE);
	   case 0:
	       puts("Child exiting.");
         char table_name[TABLE_LEN];
         printf("%s: legnth: %d\n", entry->d_name, strcspn(entry->d_name, "."));
         size_t len_name = strcspn(entry->d_name, ".");
         strncpy(table_name, entry->d_name, len_name);
         table_name[len_name] = '\0';
         printf("%s\n", table_name);
         fprintf(models_header, "#include \"model_%s.h\"\n", table_name);
	       exit(3);
	   default:
	       printf("Child is PID %jd\n", (int) pid);
         CreateCRUDFilesFromSQL(full_path, output_folder);
	       puts("Parent exiting.");
	 }
}

void EnsureDefaultConfig(const char *config_path)
{
  FILE *file = fopen(config_path, "w");
  if (file == NULL) {
    perror("Failed to create config");
    exit(1);
  }
  fprintf(file, "input_model_folder: ./models\n");
  fprintf(file, "output_model_folder: ./models\n");
  fclose(file);
}

void ParseConfig(const char *config_path, PogPoolConfig *config)
{
  FILE *file = fopen(config_path, "r");
  if (!file)
  {
    EnsureDefaultConfig(config_path);
    file = fopen(config_path, "r");
    if (!file)
    {
      perror("Cannot open config file after creating it");
      exit(1);
    }
  }

  char line[512];
  while (fgets(line, sizeof(line), file)) {
    char *key = strtok(line, ":");
    char *value = SkipSpaces(strtok(NULL, "\n"));
    if (key && value)
    {
      if (strcmp(key, "input_model_folder") == 0)
      {
        strncpy(config->input_dir, value, DIR_LEN);
      }
      else if (strcmp(key, "output_model_code_folder") == 0)
      {
        strncpy(config->output_dir, value, DIR_LEN);
        sprintf(config->output_file, "%s/models.h", config->output_dir);
      }
      else if (strcmp(key, "database_url") == 0)
      {
        strncpy(config->database_url, value, DATABASE_URL_LEN);
      }
    }
  }

  fclose(file);
}

void GenerateModelFilesFromConfig()
{
  const char *config_path = "example/pog_pool_config.yml";
  PogPoolConfig pogPoolConfig = {0};

  ParseConfig(config_path, &pogPoolConfig);

  DIR *dir = opendir(pogPoolConfig.input_dir);
  if (!dir)
  {
    perror("Failed to open input directory");
    return;
  }

  FILE *models_header = fopen(pogPoolConfig.output_file, "w");
  if (!models_header)
  {
    printf("%s\n", pogPoolConfig.output_file);
    perror("Failed to open output file");
    closedir(dir);
    return;
  }

  SetModelHeader(models_header);
  fclose(models_header);
  models_header = fopen(pogPoolConfig.output_file, "a");

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (entry->d_type == DT_REG && strstr(entry->d_name, ".swp")) continue;
    if (entry->d_type == DT_REG && strstr(entry->d_name, ".sql")) {
      PIDStatus(pogPoolConfig.input_dir, entry, models_header, pogPoolConfig.output_dir);
    }
  }

  // Wait for all child process to finish..
  while (wait(NULL) > 0)
  {
    continue;
  }

  fprintf(models_header, "\n#endif // MODELS_H\n");
  fclose(models_header);
  closedir(dir);
}
