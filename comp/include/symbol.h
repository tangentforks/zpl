/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _SYMBOL_H_


/* exported interface */

symboltable_t* alloc_st(symbolclass,char*);
symboltable_t* alloc_loc_st(symbolclass,char*);
hashentry_t* getsymtab(int);
void initialize_symtab(void);
symlist_t* alloc_symlist(char*);
symboltable_t* getlevel(int);
hashentry_t* alloc_he(char*);
symboltable_t* lu_pst(char*, ...);
symboltable_t* lu_only(char*);
symboltable_t* lu(char*);
symboltable_t* lu_insert(char*);
void insert_pst(symboltable_t*);
symboltable_t* add_constant(char*);
void unlink_sym(symboltable_t*,int);
void deactivate(symboltable_t*);
initial_t* alloc_initial(void);
symboltable_t* add_unresolved(symboltable_t*);
symboltable_t* insert_function(char*,datatype_t*,int,...);
void initialize_pstack(void);
symboltable_t* get_blank_arrayref_index(int depth,int index);

#endif
