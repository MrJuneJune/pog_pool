#include "pog_pool/auto_generate.h"
#include "pog_pool/crud_header_template.h"
#include "pog_pool/crud_src_template.h"


char* SanitizeHexForJSON(const char* input)
{
  if (!input) return "none";

  if (strncmp(input, "\\x", 2) == 0) {
    return strdup(input + 2);  
  }

  return strdup(input); 
}

char* ArrayToString_int(const int* arr, int len, int is_json)
{
  if (!arr || len <= 0) return strdup("{}");

  size_t buf_size = len * 12 + 2;
  char* out = malloc(buf_size);
  if (!out) return NULL;

  out[0] = is_json==1 ? '[': '{';
  size_t offset = 1;

  for (int i = 0; i < len; ++i)
    offset += snprintf(out + offset, buf_size - offset, "%d%s", arr[i], (i < len - 1) ? "," : is_json==1 ? "]": "}");

  return out;
}

char* ArrayToString_str(char** arr, int len, int is_json)
{
  if (!arr || len <= 0) return strdup("{}");

  size_t buf_size = 2;  // for '{' and '}'
  for (int i = 0; i < len; ++i) {
    const char* val = arr[i] ? arr[i] : "null";
    buf_size += strlen(val) + 4;  // quotes + comma
  }

  char* out = malloc(buf_size);
  if (!out) return NULL;

  out[0] = is_json==1 ? '[': '{';
  size_t offset = 1;

  for (int i = 0; i < len; ++i) {
    const char* val = arr[i] ? arr[i] : "null";
    offset += snprintf(out + offset, buf_size - offset, "\"%s\"%s", val, (i < len - 1) ? "," : is_json==1 ? "]": "}");
  }

  out[offset] = '\0'; // make sure it's null-terminated
  return out;
}


void* ParseArray(const char* value, ArrayElementType type, size_t* out_len)
{
  if (!value || value[0] != '{') {
    *out_len = 0;
    return NULL;
  }

  char* tmp = strdup(value + 1);  // skip '{'
  char* end_brace = strchr(tmp, '}');
  if (end_brace) *end_brace = '\0';

  size_t capacity = 8;
  size_t count = 0;
  void* result = NULL;

  if (type == ARRAY_TYPE_INT) {
    int* arr = malloc(capacity * sizeof(int));
    char* token = strtok(tmp, ",");

    while (token) {
      if (count >= capacity) {
        capacity *= 2;
        arr = realloc(arr, capacity * sizeof(int));
      }
      arr[count++] = atoi(token);
      token = strtok(NULL, ",");
    }

    result = arr;

  } else if (type == ARRAY_TYPE_STRING) {
    char** arr = malloc(capacity * sizeof(char*));
    char* token = strtok(tmp, ",");

    while (token) {
      if (count >= capacity) {
        capacity *= 2;
        arr = realloc(arr, capacity * sizeof(char*));
      }

      // Strip surrounding quotes if any
      size_t len = strlen(token);
      if (token[0] == '"') {
        token[len - 1] = '\0';
        token++;
      }

      arr[count++] = strdup(token);
      token = strtok(NULL, ",");
    }

    result = arr;
  }

  *out_len = count;
  free(tmp);
  return result;
}

const char* SqlToCType(const char *sql_type)
{
  // Handle arrays first
  if (strstr(sql_type, "[]") != NULL) {
    if (strncmp(sql_type, "INT", 3) == 0)
      return "int*";
    if (strncmp(sql_type, "TEXT", 4) == 0 || strncmp(sql_type, "VARCHAR", 7) == 0)
      return "char**";
    return "char*"; // fallback for unknown array types
  }

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

size_t GetFileSize(FILE *file)
{
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  return file_size;
}

char* ReadTemplate(const char *file_path)
{
  FILE *file = fopen(file_path, "r");
  if (!file)
  {
    fprintf(stderr, "Error opening file: %s\n", file_path);
    return NULL;
  }

  size_t file_size = GetFileSize(file);
  char *buffer = malloc(file_size*3 + 1);
  if (!buffer)
  {
    fprintf(stderr, "Memory allocation failed\n");
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, file_size, file);
  buffer[file_size] = '\0';
  fclose(file);
  return buffer;
}

char* ReplaceAllChar(const char* buffer, const char* placeholder, const char* value)
{
  size_t buffer_len = strlen(buffer);
  size_t placeholder_len = strlen(placeholder);
  size_t value_len = strlen(value);

  size_t count = 0;
  const char* tmp = buffer;

  // Count occurrences
  while ((tmp = strstr(tmp, placeholder)))
  {
    count++;
    tmp += placeholder_len;
  }

  // Compute new size
  size_t new_size = buffer_len + count * (value_len - placeholder_len) + 1;
  char* result = malloc(new_size);
  if (!result) return NULL;

  const char* src = buffer;
  char* dst = result;
  while ((tmp = strstr(src, placeholder)))
  {
    size_t chunk_len = tmp - src;
    memcpy(dst, src, chunk_len);
    dst += chunk_len;
    memcpy(dst, value, value_len);
    dst += value_len;
    src = tmp + placeholder_len;
  }
  strcpy(dst, src); // copy remaining tail

  return result; // caller must free
}


void ReplaceAll(char **buffer, const char *placeholder, const char *value)
{
  char *new_str = ReplaceAllChar(*buffer, placeholder, value);
  free(*buffer);
  *buffer = new_str;
}

char* GetDefaultValue(const char *c_type)
{
  if 
  (
    strcmp(c_type, "int") == 0 || 
    strcmp(c_type, "short") == 0 || 
    strcmp(c_type, "long long") == 0 || 
    strcmp(c_type, "float") == 0 || 
    strcmp(c_type, "double") == 0
  )
    return "0";
  return "\"null\"";
}

void BuildAllCodegenPieces(const Column* columns, size_t count, const char* table_name, CodegenOutput* out)
{
  strcpy(out->column_names, "(");
  out->format_parts[0] = '\0';
  out->value_args[0] = '\0';
  out->set_clause[0] = '\0';
  out->update_args[0] = '\0';
  out->field_assignments[0] = '\0';
  strcpy(out->serialize_format, "{");
  out->serialize_args[0] = '\0';

  int first_val = 1;
  int first_col = 1;
  int first_update = 1;
  int first_json = 1;

  for (size_t i = 0; i < count; i++) {
    const char* name = columns[i].name;
    const char* sql_type = columns[i].type;
    const char* c_type = SqlToCType(sql_type);

    // Skip SERIAL columns
    if (strcmp(sql_type, "SERIAL") == 0)
      continue;

    // --- column_names ---
    if (!first_col) strcat(out->column_names, ", ");
    strcat(out->column_names, name);

    // --- format_parts ---
    if (!first_col) strcat(out->format_parts, ", ");
    if (strcmp(c_type, "short") == 0 || strcmp(c_type, "int") == 0)
      strcat(out->format_parts, "%d");
    else if (strcmp(c_type, "long long") == 0)
      strcat(out->format_parts, "%lld");
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
      strcat(out->format_parts, "%f");
    else if (strcmp(c_type, "bool") == 0)
      strcat(out->format_parts, "%s");
    else
      strcat(out->format_parts, "'%s'");

    // --- value_args ---
    if (!first_val) strcat(out->value_args, ", ");
    if (strcmp(c_type, "bool") == 0)
      sprintf(out->value_args + strlen(out->value_args), "u.%s ? \"TRUE\" : \"FALSE\"", name);
    else if (strcmp(c_type, "void*") == 0)
      sprintf(out->value_args + strlen(out->value_args), "(char*)u.%s", name);
    else if (strcmp(c_type, "int*") == 0 || strcmp(c_type, "char**") == 0)
      sprintf(out->value_args + strlen(out->value_args), "u.%s ? ArrayToString(u.%s, u.%s_len, 0) : \"null\"", name, name, name);
    else
      sprintf(out->value_args + strlen(out->value_args), "u.%s", name);

    // --- set_clause ---
    if (!first_update) strcat(out->set_clause, ", ");
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0)
      sprintf(out->set_clause + strlen(out->set_clause), "%s=%%d", name);
    else if (strcmp(c_type, "long long") == 0)
      sprintf(out->set_clause + strlen(out->set_clause), "%s=%%lld", name);
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
      sprintf(out->set_clause + strlen(out->set_clause), "%s=%%f", name);
    else if (strcmp(c_type, "bool") == 0)
      sprintf(out->set_clause + strlen(out->set_clause), "%s=%%s", name);
    else
      sprintf(out->set_clause + strlen(out->set_clause), "%s='%%s'", name);

    // --- update_args ---
    if (out->update_args[0] != '\0') strcat(out->update_args, ", ");
    if (strcmp(c_type, "bool") == 0)
      sprintf(out->update_args + strlen(out->update_args), "u.%s ? \"TRUE\" : \"FALSE\"", name);
    else if (strcmp(c_type, "void*") == 0)
      sprintf(out->update_args + strlen(out->update_args), "(char*)u.%s", name);
    else if (strcmp(c_type, "int*") == 0 || strcmp(c_type, "char**") == 0)
      sprintf(out->update_args + strlen(out->update_args), "u.%s ? ArrayToString(u.%s, u.%s_len, 0) : \"null\"", name, name, name);
    else
      sprintf(out->update_args + strlen(out->update_args), "u.%s", name);

    // --- field_assignments ---
    char line[512];
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = atoi(value);\n",
        name, name);
    }
    else if (strcmp(c_type, "long long") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = atoll(value);\n",
        name, name);
    }
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = atof(value);\n",
        name, name);
    }
    else if (strcmp(c_type, "bool") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = (strcmp(value, \"t\") == 0);\n",
        name, name);
    } 
    else if (strcmp(c_type, "int*") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = (int*) ParseArray(value, ARRAY_TYPE_INT, &list[i].%s_len);\n",
        name, name, name);
    }
    else if (strcmp(c_type, "char**") == 0)
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = (char**) ParseArray(value, ARRAY_TYPE_STRING, &list[i].%s_len);\n",
        name, name, name);
    }
    else
    {
      snprintf(line, sizeof(line),
        "      if (strcmp(colname, \"%s\") == 0) list[i].%s = strdup(value);\n",
        name, name);
    }
    strcat(out->field_assignments, line);

    // --- serialize_format ---
    if (!first_json) strcat(out->serialize_format, ", ");
    sprintf(out->serialize_format + strlen(out->serialize_format), "\\\"%s\\\":", name);
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0)
      strcat(out->serialize_format, "%d");
    else if (strcmp(c_type, "long long") == 0)
      strcat(out->serialize_format, "%lld");
    else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0)
      strcat(out->serialize_format, "%f");
    else if (strcmp(c_type, "bool") == 0 || strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0)
      strcat(out->serialize_format, "%s");
    else if (strstr(sql_type, "[]") != NULL)
      strcat(out->serialize_format, "%s");
    else
      strcat(out->serialize_format, "\\\"%s\\\"");

    // --- serialize_args ---    
    if (out->serialize_args[0] != '\0') strcat(out->serialize_args, ",\n    ");
    if (strcmp(c_type, "bool") == 0)
    {
      sprintf(out->serialize_args + strlen(out->serialize_args), "u.%s ? \"true\" : \"false\"", name);
    }
    else if (strstr(sql_type, "[]") != NULL)
    {
      sprintf(out->serialize_args + strlen(out->serialize_args),
        "ArrayToString(u.%s, u.%s_len, 1)",
        name,
        name
      );
    }
    else if (strcmp(c_type, "void*") == 0 || strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0)
    {
      sprintf(out->serialize_args + strlen(out->serialize_args), "(u.%s ? (char*)u.%s : \"null\")", 
        name,
        name
      );
    } 
    else if (strncmp(sql_type, "BYTEA", 5) == 0)
    {
      sprintf(out->serialize_args + strlen(out->serialize_args), "(u.%s ? SanitizeHexForJSON(u.%s) : %s)", name, name, GetDefaultValue(c_type));
    }
    else
    {
      sprintf(out->serialize_args + strlen(out->serialize_args), "(u.%s ? u.%s : %s)", name, name, GetDefaultValue(c_type));
    }

    first_col = 0;
    first_val = 0;
    first_update = 0;
    first_json = 0;
  }

  strcat(out->serialize_format, "}");
  strcat(out->column_names, ")");
}

void CreateHeader(
  FILE* file_out_p,
  const char* table_name,
  const Column *columns,
  const size_t column_sizes
)
{
  char struct_fields[2048] = {0};
  for (size_t i = 0; i < column_sizes; i++) {
    const char* c_type = SqlToCType(columns[i].type);
    char line[128];
    char second_line[128];
    second_line[0] = '\0';
    
    snprintf(line, sizeof(line), "  %s %s;\n", c_type, columns[i].name);
    
    if (strcmp(c_type, "int*") == 0 || strcmp(c_type, "char**") == 0)
    {
      snprintf(second_line, sizeof(second_line), "  size_t %s_len;\n", columns[i].name);
    }
    
    strcat(struct_fields, line);
    if (second_line[0] != '\0')
    {
      strcat(struct_fields, second_line);
    }
  }

  char *template = strdup(CRUD_HEADER_TEMPLATE);
  ReplaceAll(&template, "{{TABLE_NAME}}", table_name);
  ReplaceAll(&template, "{{STRUCT_FIELDS}}", struct_fields);
  fprintf(file_out_p, "%s", template);
  fclose(file_out_p);
}

void CreateSrc(
  FILE* file_out_p,
  const char* table_name,
  const Column *columns,
  const size_t column_sizes
)
{
  CodegenOutput code = {0};
  BuildAllCodegenPieces(columns, column_sizes, table_name, &code);

  // Load and replace template
  char *template = strdup(CRUD_SRC_TEMPLATE);

  ReplaceAll(&template, "{{TABLE_NAME}}", table_name);
  ReplaceAll(&template, "{{STRUCT_NAME}}", table_name);
  ReplaceAll(&template, "{{QUERY_BUFFER}}", "4096");
  ReplaceAll(&template, "{{SERIALIZE_BUFFER}}", "4096");

  ReplaceAll(&template, "{{COLUMN_NAMES}}", code.column_names);
  ReplaceAll(&template, "{{FORMAT_PARTS}}", code.format_parts);
  ReplaceAll(&template, "{{VALUE_ARGS}}", code.value_args);
  ReplaceAll(&template, "{{SET_CLAUSE}}", code.set_clause);
  ReplaceAll(&template, "{{UPDATE_ARGS}}", code.update_args);
  ReplaceAll(&template, "{{FIELD_ASSIGNMENTS}}", code.field_assignments);
  ReplaceAll(&template, "{{SERIALIZE_JSON_FORMAT}}", code.serialize_format);
  ReplaceAll(&template, "{{SERIALIZE_JSON_ARGS}}", code.serialize_args);

  fprintf(file_out_p, "%s", template);
  fclose(file_out_p);
}

void CreateCRUDFiles(
  const char *table_name,
  const Column *columns,
  size_t column_sizes,
  const char *output_folder
) {
  char header_file_name[1024];
  char src_file_name[1024];
  FILE *src_file_out_p;
  FILE *header_file_out_p;

  snprintf(header_file_name, sizeof(header_file_name), "%s/model_%s.h", output_folder, table_name);
  header_file_out_p = fopen(header_file_name, "w");
  if (!header_file_out_p)
  {
    perror("fopen header");
    return;
  }
  CreateHeader(header_file_out_p, table_name, columns, column_sizes);

  snprintf(src_file_name, sizeof(src_file_name), "%s/model_%s.c", output_folder, table_name);
  src_file_out_p = fopen(src_file_name, "w");
  if (!src_file_out_p) {
    perror("fopen source");
    return;
  }
  CreateSrc(src_file_out_p, table_name, columns, column_sizes);
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
  fprintf(file, "database_url: db_urls\n");
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
  const char *config_path = "pog_pool_config.yml";
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
