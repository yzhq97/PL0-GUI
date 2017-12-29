#include "stdafx.h"
#include "PL0-GUI.h"
#include "PL0-GUIDlg.h"
#include "pl0.h"

const char* reserver[16] = {
	"const", "var", "procedure", "call",
	"begin", "end", "if", "then",
	"else", "while", "do", "read",
	"write", "odd", "repeat", "until"
}; //16

const char special_symbols[13] = {
	'+', '-', '*', '/',
	'(', ')', '=', ',' ,
	'.', '<', '>', ';' ,
	':'
}; //13

const char * token_str[36] = {
	"illegal_sym",
	"nul_sym", "ident_sym", "number_sym", "plus_sym", "minus_sym",
	"mult_sym",  "slash_sym", "odd_sym", "eq_sym", "neq_sym", "les_sym", "leq_sym",
	"gtr_sym", "geq_sym", "lparent_sym", "rparent_sym", "comma_sym", "semicolon_sym",
	"period_sym", "becomes_sym", "begin_sym", "end_sym", "if_sym", "then_sym",
	"while_sym", "do_sym", "call_sym", "const_sym", "var_sym", "proc_sym", "write_sym",
	"read_sym" , "else_sym", "repeat_sym", "until_sym"
};

const char * pcode_str[11] = {
	"illegal",
	"LIT", "OPR", "LOD", "STO", "CAL",
	"INT", "JMP", "JPC", "RED", "WRT"
};

void AppendTextToEditCtrl(CEdit* edit, LPCTSTR pszText) {
	int nLength = edit->GetWindowTextLength();
	edit->SetSel(nLength, nLength);
	edit->ReplaceSel(pszText);
}