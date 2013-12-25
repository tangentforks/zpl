/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __BUILD3_H_
#define __BUILD3_H_

#include "parsetree.h"
#include "struct.h"
#include "symtab.h"

#define SIGN(x)   ((x)>0 ? 1 : ((x)==0 ? 0 : -1))


/* function prototypes */

int equiv_datatypes(datatype_t *, datatype_t *);
statement_t *build_comm_statement(commtype_t,int,int,expr_t*,expr_t*,
				  genlist_t*,genlist_t*,int,char*);
comm_info_t* add_comm_info(statement_t*,expr_t*,genlist_t*,genlist_t*);
statement_t* build_commstat_statement(commstattype, int, char*);
dimension_t* build_dimension(dimtypeclass);
dimension_t* copy_dimension(dimension_t*);
dimension_t* build_dim_list(dimension_t*,expr_t*,expr_t*);
datatype_t* build_ensemble_type(datatype_t*,expr_t*,int, int,int,char*);
symboltable_t* lookup(symbolclass,char*);
expr_t* define_quote_region(void);
symboltable_t* build_grid_decl_list(expr_t* expr, int lineno, char* filename);
symboltable_t* build_distribution_decl_list(expr_t* expr, int lineno, char* filename);
symboltable_t* build_region_decl_list(expr_t* expr, int lineno, char* filename);
statement_t* build_reg_mask_scope(expr_t*,expr_t*,masktype,statement_t*,int,char*);
dimension_t *build_rgrid_dim_list(dimension_t *list);
dimension_t* append_dim_list(dimension_t*,dimension_t*);
symboltable_t* process_region_for_type(symboltable_t*,int);
datatype_t* build_array_type(dimension_t*,datatype_t*);
datatype_t* build_record_type(symbolclass);
datatype_t* HandleUnknownTypes(char*);
expr_t *build_usrreduction_op(char*, expr_t*, expr_t*);
expr_t* build_reduction_op(binop_t,expr_t*,expr_t*);
expr_t* build_flood_op(exprtype,expr_t*,expr_t*);
expr_t* build_usrscan_op(char*, int*, expr_t*);
expr_t* build_scan_op(binop_t,int*,expr_t*);
expr_t* build_randacc_op(expr_t* expr, expr_t* map);
expr_t* build_permute_op(permutetype_t,expr_t*,expr_t*);
statement_t* build_halt_statement(expr_t*,int,char*);
statement_t* build_wrap_statement(stmnttype,expr_t*,int,char*);
statement_t* build_io_statements(iotype,expr_t*,int,char*);
symlist_t* build_sym_list(symlist_t*,symlist_t*);
int* build_intlist(int*,int);
int* build_constantlist(int*,expr_t*);
statement_t* build_1_io_statement(iotype,expr_t*,expr_t*,expr_t*,statement_t*,
				  int,char*);
statement_t* build_mloop_statement(expr_t*,statement_t*,int,expr_t*,int,
				   int,char*);
statement_t* build_nloop_statement(int,dimension_t*,statement_t*,int,int,int,
				   char*);
void define_componentlist(symlist_t*,datatype_t*);
symboltable_t* make_direction_multi(expr_t*,char*,dimension_t*,expr_t*,
				    int,char*);
void define_parameterlist(subclass,int,symlist_t*,datatype_t*);

symboltable_t* define_functionhead(char*, datatype_t*, int, char*, int, int);
symboltable_t* define_functionbody(symboltable_t*, statement_t*, int, char*);

void HandleAnonymousRecords(datatype_t*);
void test_mloop_vars_pre_post(mloop_t*);
symboltable_t* define_varconst(symbolclass,int,int,int,int,int,char*,datatype_t*,
			       initial_t*,int,char*);
void define_varconstlist(symbolclass,int,int,int,int,int,symlist_t*,
			 datatype_t*,initial_t*, int,char*);


void check_name_list_conflict(symlist_t*);
void check_name_conflict(char*);
symboltable_t* define_direction(char*, initial_t*, int, char*);
void define_type(char*,datatype_t*,int,int,char*);
dimension_t* find_region_multirange(symboltable_t*);

symboltable_t* create_grid(char*, initial_t*, int, char*);
symboltable_t* create_distribution(char*, char*, initial_t*, int, char*);

/** chooses an overloaded function name from all possibilities based on the
    parameters and return type, silent != 0 means no errors are reported **/
char* get_function_name(symboltable_t*, expr_t*, int silent);

expr_t* build_sbe_expr(dimension_t*);
expr_t* convert_sbe_to_direction(expr_t*);
expr_t* convert_sbe_to_region(expr_t*, char*,int,char*);
datatype_t* convert_sbe_to_region_type(expr_t*, char*, char*,int,char*);
statement_t* convert_sbe_to_region_spec(expr_t*);
statement_t* build_region_scope(expr_t*, int, char*);
statement_t* insert_statements_into_region_scope(statement_t*, statement_t*);
expr_t* buildqregexpr(int);

symboltable_t* find_distribution_pst_in_expr(expr_t*);
symboltable_t* find_grid_pst_in_expr(expr_t*);

#endif
