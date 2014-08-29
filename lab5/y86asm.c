/*Code： SJTU->Cece*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y86asm.h"

line_t *y86bin_listhead = NULL;   /* the head of y86 binary code line list*/
line_t *y86bin_listtail = NULL;   /* the tail of y86 binary code line list*/
int y86asm_lineno = 0; /* the current line number of y86 assemble code */

#define err_print(_s, _a ...) do { \
  if (y86asm_lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", y86asm_lineno, ## _a); \
} while (0);

/* macro for parsing y86 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

int vmaddr = 0;    /* vm addr */

/* register table */
reg_t reg_table[REG_CNT] = {
    {"%eax", REG_EAX},
    {"%ecx", REG_ECX},
    {"%edx", REG_EDX},
    {"%ebx", REG_EBX},
    {"%esp", REG_ESP},
    {"%ebp", REG_EBP},
    {"%esi", REG_ESI},
    {"%edi", REG_EDI},
};

regid_t find_register(char *name)
{
    int i;
    for (i = 0; i < REG_CNT; i++)
        if (!strncmp(name, reg_table[i].name, 4))
            return reg_table[i].id;
    return REG_ERR;
}

/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovl", 6,HPACK(I_RRMOVL, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVL, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVL, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVL, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVL, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVL, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVL, C_G), 2 },
    {"irmovl", 6,HPACK(I_IRMOVL, F_NONE), 6 },
    {"rmmovl", 6,HPACK(I_RMMOVL, F_NONE), 6 },
    {"mrmovl", 6,HPACK(I_MRMOVL, F_NONE), 6 },
    {"addl", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subl", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andl", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorl", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 5 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 5 },
    {"jl", 2,    HPACK(I_JMP, C_L), 5 },
    {"je", 2,    HPACK(I_JMP, C_E), 5 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 5 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 5 },
    {"jg", 2,    HPACK(I_JMP, C_G), 5 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 5 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushl", 5, HPACK(I_PUSHL, F_NONE), 2 },
    {"popl", 4,  HPACK(I_POPL, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name)
{
    int i;
    for (i = 0; instr_set[i].name; i++)
		if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
	    	return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
	symbol_t *next_buff;
	next_buff = symtab -> next;
	while ( (next_buff != NULL)&&(next_buff->name!=NULL)){
		if (strncmp(next_buff->name,name,strlen(name)) == 0)
			return next_buff;
		next_buff = next_buff->next;
	}
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name)
{    
    /* check duplicate */
	if (find_symbol(name) != NULL)
		return -1;

    /* create new symbol_t (don't forget to free it)*/
	symbol_t *tmp = (symbol_t *)malloc(sizeof(symbol_t));
	memset(tmp,0,sizeof(symbol_t));
	tmp->name = name;
	tmp->addr = vmaddr;
	tmp->next = NULL;

    /* add the new symbol_t to symbol table */
	symbol_t *next_buff = symtab;
	if (symtab->next == NULL)
		symtab->next = tmp;
	else {
	while ( next_buff->next != NULL){
		next_buff = next_buff->next;
	}
	next_buff ->next = tmp;
	}
	//vmaddr += strlen(name+1);
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
void add_reloc(char *name, bin_t *bin)
{
	int i = 0;
	char *name_buff;
	while (*(name + i) != ' '){
		i++;
		if (i == strlen(name))
			break;
	}
	name_buff = (char *)malloc(i*sizeof(char));
	strncpy(name_buff,name,i);
    /* create new reloc_t (don't forget to free it)*/
	reloc_t *re_buff = (reloc_t *)malloc(sizeof(reloc_t));    
	re_buff->y86bin = bin;
	re_buff->name = name_buff;
	re_buff->next = NULL;

    /* add the new reloc_t to relocation table */
	reloc_t *next_reloc = (reloc_t *)malloc(sizeof(reloc_t));
	next_reloc = reltab;
	if (reltab->next == NULL)
		reltab->next = re_buff;
	else {
	while (next_reloc->next != NULL){

		next_reloc = next_reloc->next;
	}
	next_reloc->next = re_buff;
	}
}



/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovl')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t **inst)
{
    char *cur = *ptr;
    instr_t *tmp;

    /* skip the blank */
    SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* find_instr and check end */
    tmp = find_instr(cur);
    if (tmp == NULL)
        return PARSE_ERR;

    cur += tmp->len;
    if (!IS_END(cur) && !IS_BLANK(cur))
        return PARSE_ERR;

    /* set 'ptr' and 'inst' */
    *inst = tmp;
    *ptr = cur;
    return PARSE_INSTR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr, char delim)
{
	char *cur = *ptr;

    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

	/*Comments: whether the delim is the ","*/
	if (*cur != delim){
		if (IS_REG(cur+1) || IS_LETTER(cur+1) || IS_DIGIT(cur+1))
		err_print("Invalid ','");
		return PARSE_ERR;
	}	
	cur ++;
	
    /* set 'ptr' */
	*ptr = cur;
    return PARSE_DELIM;
}

/*
 * parse_reg: parse an expected register token (e.g., '%eax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid)
{
	char *cur = *ptr;
	regid_t tmp;

    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* find register */
	tmp = find_register(cur);
	cur += 4;
	if (tmp == REG_ERR){
        return PARSE_ERR;
	}

    /* set 'ptr' and 'regid' */
	*ptr = cur;
	*regid = tmp;
    return PARSE_REG;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name)
{
	char *cur = *ptr;
	int result = 0,i = 0;
	char *cur_buff = "p";
    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;
	int k = 0;
	char *name_buff;
	while (*(cur + k) != ' ' && *(cur+k) != ',' && *(cur+k) != '\t'){
		k++;
		if (k == strlen(cur))
			break;
	}
	name_buff = (char *)malloc(k*sizeof(char)+1);
	memset(name_buff,0,(k*sizeof(char))+1);
	strncpy(name_buff,cur,k);
    /* allocate name and copy to it */
	for (i = 0;i<strlen(name_buff);i++){
		if (*(name_buff+i) == ','){
			result = 1;
			break;
			}
	}
	if (result == 1){
		i = i-1;
		memset(cur_buff,0,(i*sizeof(char)));
		strncmp(cur_buff,name_buff,i);
	}
	else if (result == 0){
		i = strlen(name_buff);
		cur_buff = name_buff;
	}

	if (!IS_LETTER(cur_buff))
		return PARSE_ERR;

	cur += i;

    /* set 'ptr' and 'name' */
	*ptr = cur;
	*name = cur_buff;
    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, long *value)
{
	char *cur = *ptr;
	char *err;
	long tmp;
    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* calculate the digit, (NOTE: see strtoll()) */
	if (!IS_DIGIT(cur)){
		return PARSE_ERR;
	}
	tmp = strtoll(cur,&err,0);

	cur = err;

    /* set 'ptr' and 'value' */
	*ptr = cur;
	*value = tmp;
    return PARSE_DIGIT;

}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char **name, long *value)
{
	char *cur = *ptr;
	parse_t result = PARSE_ERR;
	long *val_buff = NULL;
	char *name_buff = NULL;
	val_buff = value;

    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* if IS_IMM, then parse the digit */
	if (IS_IMM(cur)){
		cur++;
		result = parse_digit(&(cur),val_buff);
	}

    /* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(cur))
		result = parse_symbol(&(cur),&(name_buff));

	else return PARSE_ERR;
	if (result == PARSE_ERR){
		err_print("Invalid Immediate");
		return result;
	}
    /* set 'ptr' and 'name' or 'value' */
	*ptr = cur;
	*name = name_buff;
	value = val_buff;
    return result;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%ebp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
	char *cur = *ptr;
	long val_buff = 0;
	regid_t regid_buff;
    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* calculate the digit and register, (ex: (%ebp) or 8(%ebp)) */
	parse_digit(&cur,&val_buff);
	if (*(cur) == '('){
		cur++;
		parse_reg(&(cur),&regid_buff);
		if (*(cur) != ')'){
			cur++;	
			err_print("Invalid MEM");			
			return PARSE_ERR;
		}
		else cur++;	
	}

    /* set 'ptr', 'value' and 'regid' */
	*ptr = cur;
	*value = val_buff;
	*regid = regid_buff;
    return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char **name, long *value)
{
	char *cur = *ptr;
	long val_buff;
	char *name_buff = *name;
	parse_t result = PARSE_ERR;
    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;

    /* if IS_DIGIT, then parse the digit */
	if (IS_DIGIT(cur))
		result = parse_digit(&cur,&val_buff);

    /* if IS_LETTER, then parse the symbol */
	if (IS_LETTER(cur))
		result = parse_symbol(&cur,name);
    /* set 'ptr', 'name' and 'value' */
	*ptr = cur;
	name = name_buff;
	*value = val_buff;
    return result;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char **ptr, char **name)
{
	int result = 0;
	char *cur = *ptr;
	char *name_buff;
	int i = 0;
	unsigned j = 0;
    /* skip the blank and check */
	SKIP_BLANK(cur);
    if (IS_END(cur))
        return PARSE_ERR;
    /* allocate name and copy to it */
	if (IS_LETTER(cur)){
		for (j = 0;j<strlen(cur);j++){
			if (*(cur+j) == ':'){
				for (i = 0;i<j;i++)
					if (*(cur + i) == '#'){
						result = 0;
						break;
						}
					else result = 1;
				break;
				}
		}
	i = 0;
		if (result == 1){
		while (*(cur+i) != ':')
			i++;
		 name_buff = (char *)malloc(i*sizeof(char));
		memset(name_buff,0,i*sizeof(char));
		strncpy(name_buff,cur,i);
		}
		else return PARSE_ERR;
	}
	else return PARSE_ERR;
    /* set 'ptr' and 'name' */
	*ptr = cur+i+1;
	*name = name_buff;
    return PARSE_LABEL;
}

/*
 * parse_line: parse a line of y86 code (e.g., 'Loop: mrmovl (%ecx), %esi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y86 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y86 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
type_t parse_line(line_t *line)
{
    bin_t *y86bin;
    char * y86asm;
    char *label = NULL;
    instr_t *inst = NULL;

    char *cur;
    int ret;

    y86bin = &line->y86bin;
    y86asm = (char *)
        malloc(sizeof(char) * (strlen(line->y86asm) + 1));
    strcpy(y86asm, line->y86asm);
    cur = y86asm;

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%ebp), %ecx
*           call SUM  #invoke SUM function */
cont:

    /* skip blank and check IS_END */
    SKIP_BLANK(cur);
    if (IS_END(cur))
        goto out; /* done */
    
    /* is a comment ? */
    if (IS_COMMENT(cur)) {
        goto out; /* skip rest */
    }


    /* is a label ? */
    ret = parse_label(&cur, &label);
    if (ret == PARSE_LABEL) {
        /* add new symbol */
        if (add_symbol(label) < 0) {
            line->type = TYPE_ERR;
            err_print("Dup symbol:%s", label);
            goto out;
        }

        /* set type and y86bin */
        line->type = TYPE_INS;
        line->y86bin.addr = vmaddr;

        /* continue */
        goto cont;
    }

    /* is an instruction ? */
    ret = parse_instr(&cur, &inst);
    if (ret == PARSE_ERR) {
        line->type = TYPE_ERR;
        err_print("Invalid instr");
        goto out;
    }

    /* set type and y86bin */
    line->type = TYPE_INS;
    y86bin->addr = vmaddr;
    y86bin->codes[0] = inst->code;
    y86bin->bytes = inst->bytes;

    /* update vmaddr */    
    vmaddr += inst->bytes;

    /* parse the rest of instruction according to the itype */
    switch (HIGH(inst->code)) {
      /* further partition the y86 instructions according to the format */
      case I_HALT:  /* 0:0 - e.g., halt */
      case I_NOP:   /* 1:0 - e.g., nop */
      case I_RET: { /* 9:0 - e.g., ret" */
        goto cont;
      }

      case I_PUSHL: /* A:0 regA:F - e.g., pushl %esp */
      case I_POPL: {/* B:0 regA:F - e.g., popl %ebp */
		regid_t reg_buff;
        /* parse register */
		if (parse_reg(&cur,&reg_buff) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
        /* set y86bin codes */
		y86bin->codes[1] = (reg_buff<<4)+0xf;
        goto cont;
      }
   
      case I_RRMOVL:/* 2:x regA,regB - e.g., rrmovl %esp, %ebp */
      case I_ALU: { /* 6:x regA,regB - e.g., xorl %eax, %eax */
		regid_t regA,regB;
		char de = ',';
		if (parse_reg(&cur,&regA) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
		if (parse_delim(&cur,de) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_reg(&cur,&regB) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
		y86bin->codes[1] = (regA<<4)+regB;
        goto cont;
      }
      
      case I_IRMOVL: {  /* 3:0 Imm, regB - e.g., irmovl $-1, %ebx */
		regid_t regB;
		char de = ',';
		char *name;
		long val = 10;
		parse_t result = parse_imm(&cur,&name,&val);
		if (result == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_delim(&cur,de) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_reg(&cur,&regB) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
		if (result == PARSE_DIGIT){
		y86bin->codes[1] = 0xf0 + regB;
		y86bin->codes[2] = (val & 0xff);
		y86bin->codes[3] = (val>>8) & 0xff;
		y86bin->codes[4] = (val>>16) & 0xff;
		y86bin->codes[5] = (val>>24) & 0xff;
		}
		else if (result == PARSE_SYMBOL){
		add_reloc(name,y86bin);
		y86bin->codes[1] = 0xf0 + regB;
		}
        goto cont;
      }
      
      case I_RMMOVL: {  /* 4:0 regA, D(regB) - e.g., rmmovl %eax, 8(%esp)  */
		regid_t regA,regB;
		long val = 0;
		char de = ',';
		if (parse_reg(&cur,&regA) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
		if (parse_delim(&cur,de) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_mem(&cur,&val,&regB) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		y86bin->codes[1] = (regA << 4) + regB;
		y86bin->codes[2] = (val & 0xff);
		y86bin->codes[3] = (val>>8) & 0xff;
		y86bin->codes[4] = (val>>16) & 0xff;
		y86bin->codes[5] = (val>>24) & 0xff;
        goto cont;
      }
      
      case I_MRMOVL: {  /* 5:0 D(regB), regA - e.g., mrmovl 8(%ebp), %ecx */
		regid_t regA,regB;
		long val = 0;
		char de = ',';
		if (parse_mem(&cur,&val,&regB) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_delim(&cur,de) == PARSE_ERR){
			line->type = TYPE_ERR;
			goto out;
		}
		if (parse_reg(&cur,&regA) == PARSE_ERR){
			line->type = TYPE_ERR;
			err_print("Invalid REG");
			goto out;
		}
		y86bin->codes[1] = (regA << 4) + regB;
		y86bin->codes[2] = (val & 0xff);
		y86bin->codes[3] = (val>>8) & 0xff;
		y86bin->codes[4] = (val>>16) & 0xff;
		y86bin->codes[5] = (val>>24) & 0xff;
        goto cont;
      }
      
      case I_JMP:   /* 7:x dest - e.g., je End */
      case I_CALL: {/* 8:x dest - e.g., call Main */
		long val = 0;
		char *name;
		parse_t result = PARSE_ERR; 
		result = parse_data(&cur,&name,&val);
		line_t *line_buff = y86bin_listhead;
		if (result == PARSE_SYMBOL)
			add_reloc(name,y86bin);
		if (result == PARSE_DIGIT){
			while (line_buff->next != NULL)
				line_buff = line_buff->next;
			if (val > line_buff->y86bin.addr){
				line->type = TYPE_ERR;
				err_print("Invalid DEST");
			}
			y86bin->codes[1] = (val & 0xff);
			y86bin->codes[2] = (val>>8) & 0xff;
			y86bin->codes[3] = (val>>16) & 0xff;
			y86bin->codes[4] = (val>>24) & 0xff;
		}
		else if (result == PARSE_SYMBOL){
			y86bin->codes[1] = 0xff;
			y86bin->codes[2] = 0xff;
			y86bin->codes[3] = 0xff;
			y86bin->codes[4] = 0xff;
		}
        goto cont;
      }
      
      case I_DIRECTIVE: {
        /* further partition directive according to dtv_t */
        switch (LOW(inst->code)) {
          case D_DATA: {    /* .long data - e.g., .long 0xC0 */
			long val = 0;
			char *name;
			parse_t result = parse_data(&cur,&name,&val);
			if (result == PARSE_SYMBOL)
				add_reloc(name,y86bin);
			if (result == PARSE_DIGIT){
				y86bin->codes[0] = val & 0xff;
				y86bin->codes[1] = (val & 0xff00) >> 8;
				y86bin->codes[2] = (val & 0xff0000) >> 16;
				y86bin->codes[3] = (val & 0xff000000) >> 24;
			}
			else if (result == PARSE_SYMBOL){
				y86bin->codes[1] = 0xff;
				y86bin->codes[2] = 0xff;
				y86bin->codes[3] = 0xff;
				y86bin->codes[4] = 0xff;
			}
            goto cont;
          }
          
          case D_POS: {   /* .pos D - e.g., .pos 0x100 */
			long val = 0;
			if (parse_digit(&cur,&val) == PARSE_ERR)
				err_print("haha");
			vmaddr = val;
			y86bin->addr = vmaddr;
            goto cont;
          }
          
          case D_ALIGN: {   /* .align D - e.g., .align 4 */
			long val = 0;
			if (parse_digit(&cur,&val) == PARSE_ERR)
				err_print("haha");
			if (vmaddr % val != 0)
				vmaddr = vmaddr + val - (vmaddr % val);
			y86bin->addr = vmaddr;
			goto cont;
          }
          default:
            line->type = TYPE_ERR;
            err_print("Unknown directive");
            goto out;
        }
        break;
      }
      default:
        line->type = TYPE_ERR;
        err_print("Unknown instr");
        goto out;
    }

out:
    free(y86asm);
    return line->type;
}

/*
 * assemble: assemble an y86 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y86 assembly file)
 *
 * return
 *     0: success, assmble the y86 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y86asm;

    /* read y86 code line-by-line, and parse them to generate raw y86 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen  = strlen(asm_buf);
        if ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y86 assembly code */
        y86asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y86asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        /* set defualt */
        line->type = TYPE_COMM;
        line->y86asm = y86asm;
        line->next = NULL;

        /* add to y86 binary code list */
        y86bin_listtail->next = line;
        y86bin_listtail = line;
        y86asm_lineno ++;

        /* parse */
        if (parse_line(line) == TYPE_ERR)
            return -1;
    }

    /* skip line number information in err_print() */
    y86asm_lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y86 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void)
{
    reloc_t *rtmp = NULL;
    
    rtmp = reltab->next;
	
	symbol_t *rtsb = NULL;

	rtsb = symtab;

    while (rtmp) {
        /* find symbol */
		rtsb = find_symbol(rtmp->name);
		if (rtsb == NULL){
			err_print("Unknown symbol:'%s'",rtmp->name)
			return -1;
		}
		if (rtsb != NULL){
			rtmp->y86bin->codes[rtmp->y86bin->bytes-4] = (rtsb->addr) & 0xff;
			rtmp->y86bin->codes[rtmp->y86bin->bytes-3] = (rtsb->addr>>8) & 0xff;
			rtmp->y86bin->codes[rtmp->y86bin->bytes-2] = (rtsb->addr>>16) & 0xff;
			rtmp->y86bin->codes[rtmp->y86bin->bytes-1] = (rtsb->addr>>24) & 0xff;
		}
        /* relocate y86bin according itype */
		//if (rtmp->y86bin->codes[0] == 0xff)
		//	err_print("haha,%s",rtmp->y86bin->codes);
        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}

/*
 * binfile: generate the y86 binary file
 * args
 *     out: point to output file (an y86 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out)
{
	byte_t *image = (byte_t *)malloc(sizeof(byte_t));
	line_t *line_buff;
	line_buff = (line_t *)malloc(sizeof(line_t));
	memset(line_buff, '\0', sizeof(line_t));
	line_buff = y86bin_listhead->next;
	int vmaddr_buff = 0;
	byte_t* haha;
	int bytes_buff = 0;
	int vmaddr_buff_asm = 0;

    /* prepare image with y86 binary code */
	while (line_buff != NULL){
		if (vmaddr_buff <= line_buff->y86bin.addr){
			vmaddr_buff = line_buff->y86bin.addr;
			bytes_buff = line_buff->y86bin.bytes;
		}
		if (line_buff->next != NULL && vmaddr_buff_asm <= line_buff->next->y86bin.addr)
			vmaddr_buff_asm = line_buff->next->y86bin.addr;
		image = line_buff->y86bin.codes;
		fwrite(image,line_buff->y86bin.bytes,1,out);
		if (vmaddr_buff + bytes_buff < vmaddr_buff_asm){
			if (line_buff->next->next->next != 0)
				if ((line_buff->next->next->next->y86bin.bytes != 0  || line_buff->next->next->y86bin.bytes != 0)){
				haha = (byte_t *)malloc(sizeof(byte_t)*vmaddr_buff_asm - (vmaddr_buff + bytes_buff));
				memset(haha,0,sizeof(byte_t)*vmaddr_buff_asm - (vmaddr_buff + bytes_buff));
				fwrite(haha,sizeof(byte_t),vmaddr_buff_asm - (vmaddr_buff + bytes_buff),out);
				}
		}
		line_buff = line_buff->next;
	}
    /* binary write y86 code to output file (NOTE: see fwrite()) */
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = FALSE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[26];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y86bin = &line->y86bin;
        int i;
        
        strcpy(buf, "  0x000:              | ");
        
        hexstuff(buf+4, y86bin->addr, 3);
        if (y86bin->bytes > 0)
            for (i = 0; i < y86bin->bytes; i++)
                hexstuff(buf+9+2*i, y86bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                      | ");
    }

    printf("%s%s\n", buf, line->y86asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = y86bin_listhead->next;
    
    /* line by line */
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    y86bin_listhead = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(y86bin_listhead, 0, sizeof(line_t));
    y86bin_listtail = y86bin_listhead;
    y86asm_lineno = 0;
}

void finit(void)
{
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
    
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = y86bin_listhead->next;
        if (y86bin_listhead->y86asm) 
            free(y86bin_listhead->y86asm);
        free(y86bin_listhead);
        y86bin_listhead = ltmp;
    } while (y86bin_listhead);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }


    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y86 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);


    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 

    /* finit */
    finit();
    return 0;
}


