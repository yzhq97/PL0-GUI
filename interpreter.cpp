#include "stdafx.h"
#include "pl0.h"
#include "PL0-GUI.h"
#include "PL0-GUIDlg.h"

void print_stack_frame(int SP, int BP);
int execute_cycle(aop * instru, int * sp, int * bp, int * pc, FILE * ifp);
int OPR(int * sp, int * bp, int * pc, aop * instru);
int base(int * bp, aop * instru);

int stack[STACK_SIZE];

CString tmp;

void interpret(FILE * ifp) {
	aop * instru;

	int SP = 0;
	int BP = 1;
	int PC = 0;

	int chx = 0;
	int ret;

	while (BP != 0) {
		//输出当前指令
		instru = &code[PC];
		
		CString serial, l_cstr, a_cstr;
		l_cstr.Format(L"%d", instru->l);
		a_cstr.Format(L"%d", instru->a);
		serial.Format(L"%d", PC);
		listCodeHist->InsertItem(chx, serial);
		listCodeHist->SetItemText(chx, 1, CString(pcode_str[instru->op]));
		listCodeHist->SetItemText(chx, 2, l_cstr);
		listCodeHist->SetItemText(chx, 3, a_cstr);

		PC += 1;

		//执行指令
		ret = execute_cycle(instru, &SP, &BP, &PC, ifp);
		if (ret != 0) break;

		//输出栈帧
		tmp = L"";
		print_stack_frame(SP, BP);
		listCodeHist->SetItemText(chx, 4, tmp);

		chx += 1;
	}
}

void print_stack_frame(int SP, int BP) {
	CString str;
	//BP为0，程序结束
	if (BP == 0) return;
	//主栈帧
	else if (BP == 1) {
		for (int i = 1; i <= SP; i++) {
			str.Format(L"%d ", stack[i]);
			tmp += str;
		}
		return;
	}
	//存在调用
	else {
		print_stack_frame(BP - 1, stack[BP + 2]);

		//Covers one case, where CAL instruction is just called, meaning a new Stack Frame is created, but SP is still less than BP
		if (SP < BP) {
			tmp += L"| ";
			for (int i = 0; i < 4; i++) {
				str.Format(L"%d ", stack[BP+i]);
				tmp += str;
			}
		}
		//For SP being greater than BP, aka most cases
		else {
			tmp += L"| ";
			for (int i = BP; i <= SP; i++) {
				str.Format(L"%d ", stack[i]);
				tmp += str;
			}
		}
		return;
	}
}

int execute_cycle(aop * instru, int* sp, int* bp, int* pc, FILE * ifp) {
	CString outstr;
	switch (instru->op) {
		case lit_op: //LIT
			*sp = *sp + 1;
			if (*sp >= STACK_SIZE) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Ran out of stack!\r\n");
				return -1;
			}
			stack[*sp] = instru->a;
			break;
		case opr_op: //OPR function
			if (OPR(sp, bp, pc, instru) != 0)
				return -1;
			break;
		case lod_op: //LOD
			*sp = *sp + 1;
			if (*sp >= STACK_SIZE) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Ran out of stack!\r\n");
				return -1;
			}
			stack[*sp] = stack[base(bp, instru) + instru->a];
			break;
		case sto_op: //STO
			stack[base(bp, instru) + instru->a] = stack[*sp];
			*sp = *sp - 1;
			break;
		case cal_op: //CAL
			if (*sp+4 >= STACK_SIZE) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Ran out of stack!\r\n");
				return -1;
			}
			stack[*sp + 1] = 0; //RET
			stack[*sp + 2] = base(bp, instru); //SL
			stack[*sp + 3] = *bp; //DL
			stack[*sp + 4] = *pc; //RA
			*bp = *sp + 1;
			*pc = instru->a;
			break;
		case int_op: //INT
			*sp = *sp + instru->a;
			if (*sp >= STACK_SIZE) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Ran out of stack!\r\n");
				return -1;
			}
			break;
		case jmp_op: //JMP
			*pc = instru->a;
			break;
		case jpc_op: //JPC
			if (stack[*sp] == 0)
				*pc = instru->a;
			*sp = *sp - 1;
			break;
		case red_op: //RED
			*sp = *sp + 1;
			if (*sp >= STACK_SIZE) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Ran out of stack!\r\n");
				return -1;
			}
			if (fscanf_s(ifp, "%d", &stack[*sp]) == EOF) {
				AppendTextToEditCtrl(editOutput, L"ERROR: Unexpected EOF!\r\n");
				return -1;
			}
			break;
		case wrt_op: //WRT
			outstr.Format(L"PROGRAM OUTPUT: %d\r\n", stack[*sp]);
			AppendTextToEditCtrl(editOutput, outstr);
			*sp = *sp - 1;
			break;
		default:
			AppendTextToEditCtrl(editOutput, L"ERROR: Illegal OPR!\r\n");
			return -1;
			break;
	}
	return 0;
}

int OPR(int *sp, int* bp, int *pc, aop * instru) {

	switch (instru->a) {
	case ret_op:
		*sp = *bp - 1;
		*pc = stack[*sp + 4];
		*bp = stack[*sp + 3];
		break;
	case neg_op:
		stack[*sp] = -stack[*sp];
		break;
	case add_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] + stack[*sp + 1];
		break;
	case sub_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] - stack[*sp + 1];
		break;
	case mul_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] * stack[*sp + 1];
		break;
	case div_op:
		*sp = *sp - 1;
		if (stack[*sp + 1] == 0) {
			AppendTextToEditCtrl(editOutput, L"ERROR: Zero division error!\r\n");
			return -1;
		}
		stack[*sp] = stack[*sp] / stack[*sp + 1];
		break;
	case odd_op:
		stack[*sp] = stack[*sp] % 2;
		break;
	case mod_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] % stack[*sp + 1];
		break;
	case eql_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] == stack[*sp + 1];
		break;
	case neq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] != stack[*sp + 1];
		break;
	case lss_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] < stack[*sp + 1];
		break;
	case leq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] <= stack[*sp + 1];
		break;
	case gtr_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] > stack[*sp + 1];
		break;
	case geq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] >= stack[*sp + 1];
		break;
	}
	return 0;
}

int base(int * bp, aop * instru) {
	int l = instru->l;
	int b1;
	b1 = *bp;
	while (l>0) {
		b1 = stack[b1 + 1];
		l--;
	}
	return b1;
}
