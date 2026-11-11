/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file            subject                              =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 121192 hua    paramreader.hc  .params postfix for param file name  =*/
/*=                               is extended implicitly               =*/
/*=                                                                    =*/
/*= 011292 hua    paramreader.hc  added check params                   =*/
/*=                                                                    =*/
/*======================================================================*/

#include "paramreader.h"
#include "xmath.h"

/*----------------------------------------------------------------------*/
/* parameter check functionsons)                                        */
/*----------------------------------------------------------------------*/

void check_params(int num_params)
{
   if (num_params < 2)
   { // handle_param_error
      system("sound /home/hua/sound/laughter");
      errorstop(2, "paramreader", "Oh boy, better play golf");
   }
}

/*----------------------------------------------------------------------*/
/* CLASS paramreader (functions)                                        */
/*----------------------------------------------------------------------*/

void paramreader::read_sym(char sym[], bool &is_eof)
{
   is_eof = false;
   fscanf(f[num_includes - 1], "%79s", sym);
   while ((sym[0] == '#' && strcmp(sym, "#include") != 0))
   { // skip_comment
      char tc;

      while ((tc = getc(f[num_includes - 1])) != '\n' && tc != '#')
      {
      };
      fscanf(f[num_includes - 1], "%79s", sym);
   }
   if ((strcmp(name[num_params], "//") == 0))
   { // pop_f
      num_includes--;
      fclose(f[num_includes]);
      if (num_includes == 0)
         is_eof = true;
      else
         read_sym(sym, is_eof);
   }
   if (strcmp(sym, "#include") == 0)
   { // push_f
      fscanf(f[num_includes - 1], "%79s", sym);
      num_includes++;
      f[num_includes - 1] = fopen(sym, "r");
      read_sym(sym, is_eof);
   }
}

paramreader::paramreader(const char param_file_name[])
{
   bool is_eof = false;

   num_params = 0;
   { // open_param_file
      char file_name[128];

      strcpy(file_name, param_file_name);
      strcat(file_name, ".params");
      f[0] = fopen(file_name, "r");
      num_includes = 1;
   }
   if (f[0] == NULL)
      errorstop(1, "PARAMREADER", param_file_name, "parameter file unknown");
   { // read_operation
      read_sym(name[num_params], is_eof);
      if (!is_eof)
         read_sym(value[num_params], is_eof);
   }
   while (!is_eof)
   { // read_one_param
      num_params++;
      { // read_operation
         read_sym(name[num_params], is_eof);
         if (!is_eof)
            read_sym(value[num_params], is_eof);
      }
   }
}

void paramreader::dump()
{
   for (int i = 0; i < num_params; i++)
      printf(">%s< = >%s<\n", name[i], value[i]);
}

int paramreader::param_no(const char p_name[])
{
   for (int no = 0; no < num_params; no++)
      if (strcmp(name[no], p_name) == 0)
         return no;
   printf("paramreader error, symbol '%s' unknown\n", p_name);
   exit(1);
   return (0);
}

char *paramreader::s_param(const char name[])
{
   return value[param_no(name)];
}

double paramreader::d_param(char name[])
{
   return atof(value[param_no(name)]);
}

int paramreader::i_param(const char name[])
{
   return atoi(value[param_no(name)]);
}

int paramreader::max_i_name()
{
   int max = -INT_MAX;

   for (int i = 0; i < num_params; i++)
      max = i_max(max, atoi(name[i]));
   return max;
}

void paramreader::set(char p_name[], char p_value[])
{
   int pno = param_no(p_name);

   strcpy(value[pno], p_value);
}
