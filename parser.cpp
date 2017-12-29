#include "stdafx.h"
#include "pl0.h"

//�ݹ��½�����
//lev: ���
//dx: ������ջ�е�λ��
//tx: ���ű��±�
void program();
void block(int lev, int tx);
void const_declaration(int lev, int * ptx, int * pdx);
void var_declaration(int lev, int * ptx, int * pdx);
void statement(int lev, int * ptx);
void condition(int lev, int * ptx);
void expression(int lev, int * ptx);
void term(int lev, int * ptx);
void factor(int lev, int * ptx);
void identifier(int lev, int * ptx);

//���ߺ���
void emit(pcode_t op, int l, int a); //����ָ��
void enter(symtype_t t, int * ptx, int * pdx, int lev); //������ű�
void error(int errcase); //����

int nxtok(); //��ȡ��һ��token
int position(char * ident, int * ptx, int lev); //��λĳ����ʶ��

//��������
sym sym_table[SYMBOL_TABLE_SIZE];
aop code[CODE_SIZE];
int sym_length;
int code_length = 0;

int lx = 0, cx = 0, tx = 0,
token, num, type, errcnt = 0, diff, prev_diff = 0;
char ident[IDENTIFIER_LENGTH];

void parse() {
	errcnt = 0;
	lx = 0;
	cx = 0;
	tx = 0;
	prev_diff = 0;
	code_length = 0;

	program();
	if (errcnt > 0) return;

	sym_length = tx;
	code_length = cx;
}

//<����> ::= <�ֳ���>.
void program() {
	if (errcnt > 0) return;

	token = nxtok();
	block(0, 0);
	if (errcnt > 0) return;

	if (token != period_sym)
		error(9);
}

//<�ֳ���> ::= [<����˵������>][<����˵������>][<����˵������>]<���>
void block(int lev, int tx) {
	if (errcnt > 0) return;
	if (lev > MAX_LEXI_LEVELS) error(26);

	int dx, tx0, cx0;
	dx = 4; //��sp, bp, pc, retԤ���ռ�
	tx0 = tx;
	sym_table[tx].addr = cx; //���ڱ�����̵��õ�λ�ã���Ϊλ�û�û��ȷ������ֻ����ʱ��
	emit(jmp_op, 0, 0);


	//<����˵������> ::= const<��������>{,<��������>};
	if (token == const_sym) {
		token = nxtok();
		do {
			const_declaration(lev, &tx, &dx);
			if (errcnt > 0) return;
			while (token == comma_sym) {
				token = nxtok();
				const_declaration(lev, &tx, &dx);
				if (errcnt > 0) return;
			}
			if (token == semicolon_sym)
				token = nxtok();
			else error(5);
		} while (token == ident_sym);
	}

	//<����˵������>::= var<��ʶ��>{,<��ʶ��>};
	if (token == var_sym) {
		token = nxtok();
		do {
			var_declaration(lev, &tx, &dx);
			if (errcnt > 0) return;
			while (token == comma_sym) {
				token = nxtok();
				var_declaration(lev, &tx, &dx);
				if (errcnt > 0) return;
			}
			if (token == semicolon_sym)
				token = nxtok();
			else error(5);
		} while (token == ident_sym);
	}

	//<����˵������> ::= <�����ײ�><�ֳ���>;{<����˵������>}
	while (token == proc_sym) {
		token = nxtok();
		if (token == ident_sym) {
			enter(proc_type, &tx, &dx, lev);
			token = nxtok();
		}
		else error(4);

		if (token == semicolon_sym)
			token = nxtok();
		else error(5);

		block(lev + 1, tx);
		if (errcnt > 0) return;

		if (token == semicolon_sym)
			token = nxtok();
		else error(5);
	}

	//call��ַ����ȷ����
	code[sym_table[tx0].addr].a = cx;
	sym_table[tx0].addr = cx;

	cx0 = cx;
	emit(int_op, 0, dx);
	statement(lev, &tx);
	emit(opr_op, 0, ret_op);
}

//<��������> ::= <��ʶ��>=<�޷�������>
void const_declaration(int lev, int *ptx, int *pdx) {
	if (errcnt > 0) return;
	if (token == ident_sym) {
		token = nxtok();
		if ((token == eq_sym) || (token == becomes_sym)) {
			if (token == becomes_sym)
				error(1);
			token = nxtok();
			if (token == number_sym) {
				enter(const_type, ptx, pdx, lev);
				token = nxtok();
			}
		}
	}
}

//<��������>::= <��ʶ��>;
void var_declaration(int lev, int *ptx, int *pdx) {
	if (errcnt > 0) return;
	if (token == ident_sym) {
		enter(var_type, ptx, pdx, lev);
		token = nxtok();
	}
	else error(4);
}

//<���> ::= <��ֵ���>|<�������>|<����ѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<�ظ����>|<��>
void statement(int lev, int *ptx) {
	if (errcnt > 0) return;

	int i, cx1, cx2;

	//<��ֵ���> ::= <��ʶ��>:=<���ʽ>
	if (token == ident_sym) {
		i = position(ident, ptx, lev);
		if (i == 0) error(11);
		else if (sym_table[i].type != var_type) {
			error(12);
			i = 0;
		}

		token = nxtok();
		if (token == becomes_sym) token = nxtok();
		else error(13);

		expression(lev, ptx);
		if (errcnt > 0) return;
		if (i != 0) emit(sto_op, lev - sym_table[i].level, sym_table[i].addr);
	}

	//<���̵������> ::= call<��ʶ��>
	else if (token == call_sym) {
		if (errcnt > 0) return;
		token = nxtok();
		if (token != ident_sym) error(14);
		else {
			i = position(ident, ptx, lev);
			if (i == 0) error(11);
			else if (sym_table[i].type == proc_type) {
				emit(cal_op, lev - sym_table[i].level, sym_table[i].addr);
			}
			else error(15);
			token = nxtok();
		}
	}

	//<�������> ::= if<����>then<���>[else<���>]
	else if (token == if_sym) {
		if (errcnt > 0) return;
		token = nxtok();
		condition(lev, ptx);
		if (errcnt > 0) return;

		if (token == then_sym)
			token = nxtok();
		else error(16);

		cx1 = cx;
		emit(jpc_op, 0, 0); //�ȴ����� jpc ��λ��
		statement(lev, ptx);
		if (errcnt > 0) return;

		if (token == else_sym) {
			token = nxtok();
			code[cx1].a = cx + 1; //���� jpc λ��
			cx1 = cx;
			emit(jmp_op, 0, 0); //�ȴ����� jmp ��λ��
			statement(lev, ptx);
			if (errcnt > 0) return;
		}

		code[cx1].a = cx; //���� jpc �� jmp ��ַ
	}

	//<�������> ::= begin<���>{;<���>}end
	else if (token == begin_sym) {
		if (errcnt > 0) return;
		token = nxtok();
		statement(lev, ptx);
		if (errcnt > 0) return;

		while (token == semicolon_sym) {
			token = nxtok();
			statement(lev, ptx);
			if (errcnt > 0) return;
		}

		if (token == end_sym)
			token = nxtok();
		else error(17);
	}

	//<����ѭ�����> ::= while<����>do<���>
	else if (token == while_sym) {
		if (errcnt > 0) return;
		cx1 = cx;
		token = nxtok();
		condition(lev, ptx);
		if (errcnt > 0) return;
		cx2 = cx;
		emit(jpc_op, 0, 0); // �ȴ����� jpc λ��
		if (token == do_sym) token = nxtok(); else error(18);
		statement(lev, ptx);
		if (errcnt > 0) return;
		emit(jmp_op, 0, cx1);
		code[cx2].a = cx; // ���� jpc λ��
	}

	//<�ظ����> ::= repeat<���>{;<���>}until<����>
	else if (token == repeat_sym) {
		if (errcnt > 0) return;
		cx1 = cx;
		do {
			token = nxtok();
			statement(lev, ptx);
			if (errcnt > 0) return;
		} while (token == semicolon_sym);

		if (token == until_sym) {
			token = nxtok();
			condition(lev, ptx);
			if (errcnt > 0) return;
			emit(jpc_op, 0, cx1);
		}
		else error(27);
	}

	//<д���> ::= write'('<��ʶ��>{,<��ʶ��>}')'
	else if (token == write_sym) {
		if (errcnt > 0) return;
		token = nxtok();
		if (token == lparent_sym) {
			do {
				token = nxtok();
				identifier(lev, ptx);
				if (errcnt > 0) return;
				emit(wrt_op, 0, 1);
			} while (token == comma_sym);
			if (token != rparent_sym) error(22);
		}
		else error(28);
		token = nxtok();
	}

	//<�����> ::= read'('<��ʶ��>{,<��ʶ��>}')'
	else if (token == read_sym) {
		if (errcnt > 0) return;
		token = nxtok();
		if (token == lparent_sym) {
			do {
				token = nxtok();
				emit(red_op, 0, 0);
				i = position(ident, ptx, lev);
				if (i == 0) error(11);
				else if (sym_table[i].type != var_type) {
					error(12);
					i = 0;
				}
				if (i != 0)
					emit(sto_op, lev - sym_table[i].level, sym_table[i].addr);
				token = nxtok();
			} while (token == comma_sym);
			if (token != rparent_sym) error(22);
		}
		else error(28);
		token = nxtok();
	}

}

//<����> ::= <���ʽ><��ϵ�����><���ʽ>|odd<���ʽ>
void condition(int lev, int *ptx) {
	if (errcnt > 0) return;

	int relation;

	if (token == odd_sym) {
		token = nxtok();
		expression(lev, ptx);
		if (errcnt > 0) return;
		emit(opr_op, 0, odd_op);
	}
	else {
		expression(lev, ptx);
		if (errcnt > 0) return;
		if ((token != eq_sym) && (token != neq_sym) && (token != les_sym) && (token != leq_sym) && (token != gtr_sym) && (token != geq_sym)) {
			error(20);
		}
		else {
			relation = token;
			token = nxtok();
			expression(lev, ptx);
			if (errcnt > 0) return;
			switch (relation) {
			case eq_sym:
				emit(opr_op, 0, eql_op);
				break;
			case neq_sym:
				emit(opr_op, 0, neq_op);
				break;
			case les_sym:
				emit(opr_op, 0, lss_op);
				break;
			case leq_sym:
				emit(opr_op, 0, leq_op);
				break;
			case gtr_sym:
				emit(opr_op, 0, gtr_op);
				break;
			case geq_sym:
				emit(opr_op, 0, geq_op);
				break;
			}
		}
	}
}

//<���ʽ> ::= [+|-]<��>{<�ӷ������><��>}
void expression(int lev, int *ptx) {
	if (errcnt > 0) return;

	int addop;
	if (token == plus_sym || token == minus_sym) {
		addop = token;
		token = nxtok();
		term(lev, ptx);
		if (errcnt > 0) return;
		if (addop == minus_sym)
			emit(opr_op, 0, neg_op);
	} else {
		term(lev, ptx);
		if (errcnt > 0) return;
	}
	while (token == plus_sym || token == minus_sym) {
		addop = token;
		token = nxtok();
		term(lev, ptx);
		if (errcnt > 0) return;
		if (addop == plus_sym) {
			emit(opr_op, 0, add_op);
		} else {
			emit(opr_op, 0, sub_op);
		}
	}
}

//<��> ::= <����>{<�˷������><����>}
void term(int lev, int *ptx) {
	if (errcnt > 0) return;
	int mulop;
	factor(lev, ptx);
	if (errcnt > 0) return;
	while (token == mult_sym || token == slash_sym) {
		mulop = token;
		token = nxtok();
		factor(lev, ptx);
		if (errcnt > 0) return;
		if (mulop == mult_sym)
			emit(opr_op, 0, mul_op);
		else
			emit(opr_op, 0, div_op);
	}
}

//<����> ::= <��ʶ��>|<�޷�������>|'('<���ʽ>')'
void factor(int lev, int *ptx) {
	if (errcnt > 0) return;
	while ((token == ident_sym) || (token == number_sym) || (token == lparent_sym)) {
		//<��ʶ��>
		if (token == ident_sym) {
			identifier(lev, ptx);
			if (errcnt > 0) return;
		}

		//<�޷�������> ::= <����>{<����>}
		else if (token == number_sym) {
			if (num >= MAX_ADDR) {
				error(25);
				num = 0;
			}
			emit(lit_op, 0, num);
			token = nxtok();
		}

		//'('<���ʽ>')'
		else if (token == lparent_sym) {
			token = nxtok();
			expression(lev, ptx);
			if (errcnt > 0) return;
			if (token == rparent_sym)
				token = nxtok();
			else error(22);
		}
	}
}

//<��ʶ��> ::= <��ĸ>{<��ĸ>|<����>}
void identifier(int lev, int * ptx) {
	if (errcnt > 0) return;
	int i, type, level, adr, val;
	i = position(ident, ptx, lev);
	if (i == 0) error(11);
	else {
		type = sym_table[i].type;
		level = sym_table[i].level;
		adr = sym_table[i].addr;
		val = sym_table[i].val;
		if (type == const_type)
			emit(lit_op, 0, val);
		else if (type == var_type)
			emit(lod_op, lev - level, adr);
		else error(21);
	}
	token = nxtok();
}


//*****************************
//********* ���ߺ��� ***********
//*****************************

void emit(pcode_t op, int l, int a) {
	if (cx > CODE_SIZE)
		print_error("Program too long!");
	else {
		code[cx].op = op;
		code[cx].l = l;
		code[cx].a = a;
		cx++;
	}
}

//������ŵ����ű�
void enter(symtype_t t, int * ptx, int * pdx, int lev) {

	(*ptx)++;
	strcpy_s(sym_table[*ptx].name, ident);

	sym_table[*ptx].type = t;
	if (t == const_type) {
		sym_table[*ptx].val = num;
	}

	else if (t == var_type) {
		sym_table[*ptx].level = lev;
		sym_table[*ptx].addr = *pdx;
		(*pdx)++;
	}

	else {//procedure
		sym_table[*ptx].level = lev;
	}
}

//ȡ�� lex �����һ�� lex������Ǳ����������������� ident �У���������ֽ� ֵ���� num ��
int nxtok() {
	token = lex_list[lx].token;
	if (token == ident_sym)
		strcpy_s(ident, lex_list[lx].name);
	else if (token == number_sym)
		num = lex_list[lx].value;

	lx += 1;
	return token;
}

//����ĳ������ʱ���ҵ����ű��У���ǰ�������ģ�����Ŀ��õķ��Ŷ���
int position(char * ident, int * ptx, int lev) {
	int s = *ptx;
	int current_s = s;
	int diff_cnt = 0;

	while (s != 0) {
		if (strcmp(sym_table[s].name, ident) == 0) {
			if (sym_table[s].level <= lev) {
				if (diff_cnt != 0) prev_diff = diff;
				diff = lev - sym_table[s].level;
				if (diff_cnt == 0) current_s = s;
				if (diff < prev_diff) current_s = s;
				diff_cnt += 1;
			}
		}
		s -= 1;
	}
	return current_s;
}

//������
void error(int errcase) {
	errcnt++;
	switch (errcase) {
	case 1:
		print_error("Error 1: ");
		print_error("Use = instead of :=.");
		break;
	case 2:
		print_error("Error 2: ");
		print_error("= must be followed by a number.");
		break;
	case 3:
		print_error("Error 3: ");
		print_error("ident must be followed by =.");
		break;
	case 4:
		print_error("Error 4: ");
		print_error("const, var, procedure must be followed by identifier.");
		break;
	case 5:
		print_error("Error 5: ");
		print_error("Semicolon or comma missing.");
		break;
	case 6:
		print_error("Error 6: ");
		print_error("Incorrect symbol after procedure declaration.");
		break;
	case 7:
		print_error("Error 7: ");
		print_error("Statement expected");
		break;
	case 8:
		print_error("Error 8: ");
		print_error("Incorrect symbol after statement part in block.");
		break;
	case 9:
		print_error("Error 9: ");
		print_error("Period expected.");
		break;
	case 10:
		print_error("Error 10: ");
		print_error("Semicolon between statements missing.");
		break;
	case 11:
		print_error("Error 11: ");
		print_error("Undeclared identifier.");
		break;
	case 12:
		print_error("Error 12: ");
		print_error("Assignment to constant or procedure is not allowed.");
		break;
	case 13:
		print_error("Error 13: ");
		print_error("Assignment operator expected.");
		break;
	case 14:
		print_error("Error 14: ");
		print_error("call must be followed by an identifier");
		break;
	case 15:
		print_error("Error 15: ");
		print_error("Call of a constant or variable is meaningless.");
		break;
	case 16:
		print_error("Error 16: ");
		print_error("then expected");
		break;
	case 17:
		print_error("Error 17: ");
		print_error("Semicolon or } expected");
		break;
	case 18:
		print_error("Error 18: ");
		print_error("do expected");
		break;
	case 19:
		print_error("Error 19: ");
		print_error("Incorrect symbol following statement.");
		break;
	case 20:
		print_error("Error 20: ");
		print_error("Relational operator expected.");
		break;
	case 21:
		print_error("Error 21: ");
		print_error("Expression must not contain a procedure identifier.");
		break;
	case 22:
		print_error("Error 22: ");
		print_error("Right parenthesis missing.");
		break;
	case 23:
		print_error("Error 23: ");
		print_error("The preceding factor cannot begin with this symbol.");
		break;
	case 24:
		print_error("Error 24: ");
		print_error("An expression cannot begin with this symbol.");
		break;
	case 25:
		print_error("Error 25: ");
		print_error("This number is too large.");
		break;
	case 26:
		print_error("Error: 26 ");
		print_error("Level is larger than the maximum allowed lexicographical levels!");
		break;
	case 27:
		print_error("Error: 27 ");
		print_error("Expected until after repeat statements");
		break;
	case 28:
		print_error("Error: 28 ");
		print_error("Expected ( after read or write");
		break;
	default:
		print_error("Unknown Error");
		break;
	}
}