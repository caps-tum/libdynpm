%{
#include "dynpm_config.h"

extern int yylex();
int yyerror(const char *error) { printf("error: %s\n", error); }
int yywrap() { return 1; }
extern int yyparse (void);
%}

%union {
	int  value;
	char symbol;
	char conf_string[16];
};

%token <value> number
%token <conf_string> string_val
%token <symbol> equals left right comma colon

%%
configurations:
	      | configurations configuration
	      | configurations wrapping
	      | configurations configuration_size
              ;

configuration:
	      string_val equals left number comma number comma string_val right
              {
               static int process_entry = 0;
               strcpy(dynpm_configuration->process_entries[process_entry].name,$1);
               dynpm_configuration->process_entries[process_entry].port = $4;
               dynpm_configuration->process_entries[process_entry].threads = $6;
               strcpy(dynpm_configuration->process_entries[process_entry].host,$8);
               process_entry++;
              }
              ;

wrapping:
	 string_val colon string_val
         {
         static int wrapping_entry = 0;
         strcpy(dynpm_configuration->plugin_entries[wrapping_entry].plugin_type,$1);
		 strcpy(dynpm_configuration->plugin_entries[wrapping_entry].plugin_name,$3);
         wrapping_entry++;
         }
         ;

configuration_size:
		   left string_val  equals number comma string_val equals number right
                   {
                   allocate_dynpm_conf($4, $8);
                   }

%%
