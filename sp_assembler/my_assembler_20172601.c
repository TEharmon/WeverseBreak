/*
* 파일명 : my_assembler_20172601.c
* 설  명 : 이 프로그램은 SIC/XE 머신을 위한 간단한 Assembler 프로그램의 메인루틴으로,
* 입력된 파일의 코드 중, 명령어에 해당하는 OPCODE를 찾아 출력한다.
* 파일 내에서 사용되는 문자열 "00000000"에는 자신의 학번을 기입한다.
*/

/*
*
* 프로그램의 헤더를 정의한다.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "my_assembler_20172601.h"
#define _CRT_SECURE_NO_WARNINGS
int totalline = 0;
int pass1save = 0;//pass1에서 문장을 토큰으로 쪼개 저장할때 반복인자
int instruction_count;
int copy_size, rdrecsize, wrrecsize;
int outputline; 
int condition_flag = 0; //새로추가 0이면 copy 1이면 RDREC 2이면 WRREC를 읽는 중
const int mask3 = 4095; //(2^12-1) 3형식 출력용 마스크
const int mask4 = 1048575;//(2^20-1) 4형식 출력용 마스크
char object_line[300];

//출력할 라인 수
				/* ----------------------------------------------------------------------------------
				* 설명 : 사용자로 부터 어셈블리 파일을 받아서 명령어의 OPCODE를 찾아 출력한다.
				* 매계 : 실행 파일, 어셈블리 파일
				* 반환 : 성공 = 0, 실패 = < 0
				* 주의 : 현재 어셈블리 프로그램의 리스트 파일을 생성하는 루틴은 만들지 않았다.
				*		   또한 중간파일을 생성하지 않는다.
				* ----------------------------------------------------------------------------------
				*/
void freeALL();

int main(int args, char *arg[])
{

	if (init_my_assembler() < 0)
	{
		printf("init_my_assembler: 프로그램 초기화에 실패 했습니다.\n");
		return -1;
	}

	if (assem_pass1() < 0)
	{
		printf("assem_pass1: 패스1 과정에서 실패하였습니다.  \n");
		return -1;
	}
	//make_opcode_output("output_20172601.txt");
	make_symtab_output("symtab_20172601.txt");
	make_literaltab_output("literaltab_20172601.txt");
	
	if (assem_pass2() < 0)
	{
		printf(" assem_pass2: 패스2 과정에서 실패하였습니다.  \n");
		return -1;
	}

	make_objectcode_output("output2_20172601.txt");
	
	freeALL();
	return 0;
}

/* ----------------------------------------------------------------------------------
* 설명 : 프로그램 초기화를 위한 자료구조 생성 및 파일을 읽는 함수이다.
* 매계 : 없음
* 반환 : 정상종료 = 0 , 에러 발생 = -1
* 주의 : 각각의 명령어 테이블을 내부에 선언하지 않고 관리를 용이하게 하기
*		   위해서 파일 단위로 관리하여 프로그램 초기화를 통해 정보를 읽어 올 수 있도록
*		   구현하였다.
* ----------------------------------------------------------------------------------
*/
int init_my_assembler(void)
{
	int result;
	if ((result = init_inst_file("Appendix.txt")) < 0) //inst.data
		return -1;
	if ((result = init_input_file("input.txt")) < 0)
		return -1;
	return result;
}

/* ----------------------------------------------------------------------------------
* 설명 : 머신을 위한 기계 코드목록 파일을 읽어 기계어 목록 테이블(inst_table)을
*        생성하는 함수이다.
* 매계 : 기계어 목록 파일
* 반환 : 정상종료 = 0 , 에러 < 0
* 주의 : 기계어 목록파일 형식은 자유롭게 구현한다. 예시는 다음과 같다.
*
*	===============================================================================
*		   | 이름 | 형식 | 기계어 코드 | 오퍼랜드의 갯수 | NULL|
*	===============================================================================
*
* ----------------------------------------------------------------------------------
*/
int init_inst_file(char *inst_file)
{
	FILE *file;
	int errno;
	int i = 0;
	inst in;

	file = fopen(inst_file, "rt");
	if (file != NULL)
	{
		fscanf(file, "%s %d %hu %d", in.str, &(in.format), &(in.op), &(in.ops));
		inst_table[i] = (inst*)malloc(sizeof(inst));
		strcpy(inst_table[i]->str, in.str);
		inst_table[i]->op = in.op;
		inst_table[i]->format = in.format;
		inst_table[i]->ops = in.ops;
		//printf("%s %d %X %d\n", inst_table[i]->str, inst_table[i]->format, inst_table[i]->op, inst_table[i]->ops);
		while (!feof(file))
		{
			++i;
			fscanf(file, "%s %d %hu %d", in.str, &(in.format), &(in.op), &(in.ops));
			inst_table[i] = (inst*)malloc(sizeof(inst));
			strcpy(inst_table[i]->str, in.str);
			inst_table[i]->op = in.op;
			inst_table[i]->format = in.format;
			inst_table[i]->ops = in.ops;
			//printf("%s %d %X %d\n", inst_table[i]->str, inst_table[i]->format, inst_table[i]->op, inst_table[i]->ops);
		}
		fclose(file);
		instruction_count = i;
		errno = 0;
	}
	else
	{
		errno = -1;
	}

	return errno;
}

/* ----------------------------------------------------------------------------------
* 설명 : 어셈블리 할 소스코드를 읽어 소스코드 테이블(input_data)를 생성하는 함수이다.
* 매계 : 어셈블리할 소스파일명
* 반환 : 정상종료 = 0 , 에러 < 0
* 주의 : 라인단위로 저장한다.
*
* ----------------------------------------------------------------------------------
*/
int init_input_file(char *input_file)
{
	FILE *file;
	int errno;
	int i = 0;
	char linebuffer[100];
	file = fopen("input.txt", "rt");
	if (file != NULL)
	{
		fgets(linebuffer, 100, file);
		input_data[i] = (char*)calloc(strlen(linebuffer), sizeof(char));
		strcpy(input_data[i], linebuffer);
		++totalline;
		while (!feof(file))
		{
			++i;
			fgets(linebuffer, 100, file);
			input_data[i] = (char*)calloc(strlen(linebuffer), sizeof(char));
			strcpy(input_data[i], linebuffer);
			++totalline;
		}
		fclose(file);
		errno = 0;
	}
	else {
		errno = -1;
	}
	return errno;
}

/* ----------------------------------------------------------------------------------
* 설명 : 소스 코드를 읽어와 토큰단위로 분석하고 토큰 테이블을 작성하는 함수이다.
*        패스 1로 부터 호출된다.
* 매계 : 파싱을 원하는 문자열
* 반환 : 정상종료 = 0 , 에러 < 0
* 주의 : my_assembler 프로그램에서는 라인단위로 토큰 및 오브젝트 관리를 하고 있다.
* ----------------------------------------------------------------------------------
*/
int token_parsing(char *str)
{
	char *tok = strtok(str, "\t\n");
	char *tok2 = NULL;
	char tok_s[30];
	int searchindex;
	int searchformat;

	if (pass1save == 0) //첫번째 start라인인 경우만 실행
	{
		token_table[pass1save] = (token*)malloc(sizeof(token));
		for (int i = 0; i < 4; ++i)
		{
			if (i == 0)
			{
				token_table[pass1save]->label = tok;
			}
			else if (i == 1)
			{
				token_table[pass1save]->operato=tok;
			}
			else if (i == 2)
			{
				strcpy(token_table[pass1save]->operand, tok);

			}
			else
			{
				strcpy(token_table[pass1save]->comment, tok);
			}

			tok = strtok(NULL, "\t\n");
		}
		token_table[pass1save]->nixbpe = 0;

	}
	else
	{
		token_table[pass1save] = (token*)malloc(sizeof(token));
		for (int i = 0; i < 4; ++i)
		{
			if (i == 0) //label
			{
				if (str[0] != '\t')
				{
					token_table[pass1save]->label = tok;
					tok = strtok(NULL, "\t\n");
				}
				else
				{
					token_table[pass1save]->label = "\0";

				}

			}
			else if (i == 1) //operator
			{
				if (tok != NULL)
				{
					token_table[pass1save]->operato=tok;
					tok = strtok(NULL, "\t\n");
				}
				else
				{
					token_table[pass1save]->operato="";
				}
			}
			else if (i == 2)
			{
				//operand 개수 파악하기
				if (strcmp(token_table[pass1save]->operato,"RSUB") == 0)
				{
					strcpy(token_table[pass1save]->operand[0], "");
					strcpy(token_table[pass1save]->operand[1], "");
					strcpy(token_table[pass1save]->operand[2], "");
					strcpy(token_table[pass1save]->comment, "");
					//strtok(NULL, "\t\n");
					//continue;
					break;
				}
				int j = 0;
				if (tok != NULL)
				{
					strcpy(tok_s, tok);
					tok2 = strtok(tok_s, ",");
					if (tok2 != NULL)
					{
						strcpy(token_table[pass1save]->operand[0], tok2);
						strcpy(token_table[pass1save]->operand[1], "");
						strcpy(token_table[pass1save]->operand[2], "");
						tok2 = strtok(NULL, ",");

						if (tok2 != NULL)
						{
							strcpy(token_table[pass1save]->operand[1], tok2);
							strcpy(token_table[pass1save]->operand[2], "");
							tok2 = strtok(NULL, ",");
							if (tok2 != NULL)
							{
								strcpy(token_table[pass1save]->operand[2], tok2);
							}
						}
						tok = strtok(NULL, "\t\n");
					}
					else
					{
						strcpy(token_table[pass1save]->operand[0], tok);
						strcpy(token_table[pass1save]->operand[1], "");
						strcpy(token_table[pass1save]->operand[2], "");
						tok = strtok(NULL, "\t\n");
					}
				}
				else
				{
					strcpy(token_table[pass1save]->operand[0], "");
					strcpy(token_table[pass1save]->operand[1], "");
					strcpy(token_table[pass1save]->operand[2], "");
				}

			}
			else //i=4일경우, comment를 읽는 부분
			{
				if (tok == NULL)
				{
					strcpy(token_table[pass1save]->comment, "\0");
				}
				else
				{
					strcpy(token_table[pass1save]->comment, tok);
				}

				tok = strtok(NULL, "\t\n");
			}

		}
		//nixbpe를 정하는 알고리즘
		if (token_table[pass1save]->label != ".")
		{
			if (strchr(token_table[pass1save]->operato,'+')!=NULL) //4형식을 사용하는 경우
			{
				token_table[pass1save]->nixbpe = 49;
				//operand에 X가 있으면 nixbpe에서 x에 해당하는 값인 8 더해주기
				if (strcmp(token_table[pass1save]->operand[1], "X") == 0)
				{
					token_table[pass1save]->nixbpe=token_table[pass1save]->nixbpe+8;
				}
			}
			else //4형식이 아닌경우
			{
				searchindex = search_opcode(token_table[pass1save]->operato);
				if (searchindex!=-1) //opcode table을 찾아서 존재하면
				{
					searchformat = inst_table[searchindex]->format;
					if (strchr(token_table[pass1save]->operand[0], '@') != NULL) //인자를 읽어 @이면
					{
						token_table[pass1save]->nixbpe = 34;
					}
					else if (strchr(token_table[pass1save]->operand[0], '#') != NULL)
					{
						token_table[pass1save]->nixbpe = 16;
					}
					else
					{
						if (searchformat == 3 && strcmp(token_table[pass1save]->operato, "RSUB")==0)
							token_table[pass1save]->nixbpe = 48;
						else if (searchformat == 3)
							token_table[pass1save]->nixbpe = 50;
						else
							token_table[pass1save]->nixbpe = 0;
					}
				}
				else //존재하지 않으면
				{
					token_table[pass1save]->nixbpe = 0;
				}
			}
		}
		else 
		{
			token_table[pass1save]->nixbpe = 0;
		}
	}
	++pass1save;
	return 0;
}

/* ----------------------------------------------------------------------------------
* 설명 : 입력 문자열이 기계어 코드인지를 검사하는 함수이다.
* 매계 : 토큰 단위로 구분된 문자열
* 반환 : 정상종료 = 기계어 테이블 인덱스, 에러 < 0
* 주의 :
*
* ----------------------------------------------------------------------------------
*/
int search_opcode(char *str)
{
	//앞에 4바이트 추가인 +가 있으면 무시하고 검색
	int loc = -1;
	if (str[0] == '+')
	{
		char news[10];
		memset(news, '\0', 10);
		strcpy(news, str);
		char * newstring;
		newstring = strtok(news, "+");
		
		for (int i = 0; i <= instruction_count; ++i)
		{
			if (strcmp(newstring, inst_table[i]->str) == 0)
			{
				loc = i;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i <= instruction_count; ++i)
		{
			if (strcmp(str, inst_table[i]->str) == 0)
			{
				loc = i;
				break;
			}
		}
	}
	return loc;
}

/*
새로 추가 getSymtabAddr
입력: 토큰테이블의 인자
출력: 입력에 맞는 심볼테이블에 저장된 주소, 저장된 것이 없으면 -1
기능: 심볼테이블에서 심볼을 입력받아 그것에 해당하는 addr값을 리턴한다.
*/
int getSymtabAddr(char *str)
{
	int i;
	if (condition_flag == 0)
		i = 0;
	else if (condition_flag == 1)
		i = 9;
	else
		i = 14;
	int re;
	char inputstr[20];
	char *temp;
	memset(inputstr, '\0', 20);
	strcpy(inputstr, str);
	//@ 가 있으면 떼어냄
	if (strchr(inputstr, '@') != NULL)
	{
		temp = strtok(inputstr, "@");
		while (i<MAX_LINES)
		{
			if (strcmp(sym_table[i].symbol, temp) == 0)
			{
				re = sym_table[i].addr;
				break;
			}
			++i;
		}
		if (i == MAX_LINES)
		{
			re = -1;
		}
	}
	else
	{
		while (i<MAX_LINES)
		{
			if (strcmp(sym_table[i].symbol, inputstr) == 0 || strstr(sym_table[i].symbol,inputstr)!=NULL)
			{
				re = sym_table[i].addr;
				break;
			}
			++i;
		}
		if (i == MAX_LINES)
		{
			re = -1;
		}
	}
	
	return re;
}

/* ----------------------------------------------------------------------------------
* 설명 : 어셈블리 코드를 위한 패스1과정을 수행하는 함수이다.
*		   패스1에서는..
*		   1. 프로그램 소스를 스캔하여 해당하는 토큰단위로 분리하여 프로그램 라인별 토큰
*		   테이블을 생성한다.
*
* 매계 : 없음
* 반환 : 정상 종료 = 0 , 에러 = < 0
* 주의 : 현재 초기 버전에서는 에러에 대한 검사를 하지 않고 넘어간 상태이다.
*	  따라서 에러에 대한 검사 루틴을 추가해야 한다.
*
* -----------------------------------------------------------------------------------
*/
static int assem_pass1(void)
{
	/* input_data의 문자열을 한줄씩 입력 받아서
	* token_parsing()을 호출하여 token_unit에 저장
	*/
	int i = 0;
	int check;
	int tmploc; //임시변수로 RESB일경우 문자열로된 operand을 숫자로 바꿔 저장한다. 
	char *oper1 = NULL; // - 연산시 사용 앞 인자
	char *oper2 = NULL; // 뒤의 인자
	char oper[20];
	char littoken[10];
	char *littoken2;
	int litlocctr[3] = { -1, -1, -1 };
	int litlocctrcount = 0;
	while (i<totalline)
	{
		check = token_parsing(input_data[i]);
		if (check != 0)
		{
			printf("파싱이 실패한듯..\n");
			break;
		}
		//symboltable & littable 구성
		if (i == 0)
		{
			strcpy(sym_table[symtab_index].symbol, token_table[i]->label);
			sym_table[symtab_index].addr = locctr;
			++symtab_index;
		}
		if (i >= 3)
		{
			if (strcmp(token_table[i]->label, ".")!=0) //이 아니면 시행
			{
				if (token_table[i]->label != "\0") //label을 심볼테이블에 저장하는 경우
				{
					if (strcmp(token_table[i]->label, token_table[2]->operand[0]) == 0) //RDREC를 읽을 경우
					{
						//copy_size = locctr;
						locctr = 0;
						strcpy(sym_table[symtab_index].symbol, "RDREC");
						sym_table[symtab_index].addr = locctr;
						++symtab_index;
					}
					else if (strcmp(token_table[i]->label, token_table[2]->operand[1]) == 0) //WRREC를 읽을경우
					{
						rdrecsize = locctr;
						locctr = 0;
						strcpy(sym_table[symtab_index].symbol, "WRREC");
						sym_table[symtab_index].addr = locctr;
						++symtab_index;
					}
					else
					{
						strcpy(sym_table[symtab_index].symbol, token_table[i]->label);
						sym_table[symtab_index].addr = locctr;
						++symtab_index;
						//locctr 증가시키기
						if (token_table[i]->nixbpe >= 0 && token_table[i]->nixbpe % 2 == 1)
						{
							locctr = locctr + 4;
						}
						else if (strcmp(token_table[i]->operato, "RESW") == 0)
						{
							tmploc = atoi(token_table[i]->operand[0]);
							locctr += 3 * tmploc;
						}
						else if (strcmp(token_table[i]->operato, "RESB") == 0)
						{
							tmploc = atoi(token_table[i]->operand[0]);
							locctr += tmploc;
						}
						else if (strcmp(token_table[i]->operato, "WORD") == 0)
						{
							locctr += 3;
						}
						else if (strcmp(token_table[i]->operato, "BYTE") == 0)
						{
							tmploc = (strlen(token_table[i]->operand[0]) - 3) / 2;
							locctr += tmploc;
						}
						else if (search_opcode(token_table[i]->operato) != -1)
						{
							locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						}
						else
						{
							//기타 다른 문자 LTORG, EQU, EXTREF를 읽은 경우 주소계산
							//operato==EQU, operand=="*"일 경우
							if ((strcmp(token_table[i]->operato, "EQU") == 0) && (strcmp(token_table[i]->operand[0], "*") == 0))
							{
								copy_size = locctr;
							}
							else if ((strcmp(token_table[i]->operato, "EQU") == 0) && (strchr(token_table[i]->operand[0], '-') != NULL))
							{
								--symtab_index;
								memset(oper, '\0', 20);
								strcpy(oper, token_table[i]->operand[0]);
								oper1 = strtok(oper, "-");
								oper2 = strtok(NULL, "-");
								locctr = getSymtabAddr(oper1) - getSymtabAddr(oper2);
								sym_table[symtab_index].addr = locctr;
								++symtab_index;
							}
							else 
							{

							}
						}
					}

				}
				else //라벨이 없어 심볼테이블에 추가하지 않는 경우 locctr만 계산함
				{
					//locctr 증가시키기
					if (token_table[i]->nixbpe >= 0 && token_table[i]->nixbpe % 2 == 1)
					{
						locctr = locctr + 4;
					}
					else if (strcmp(token_table[i]->operato, "RESW") == 0)
					{
						tmploc = atoi(token_table[i]->operand[0]);
						locctr += 3 * tmploc;
					}
					else if (strcmp(token_table[i]->operato, "RESB") == 0)
					{
						tmploc = atoi(token_table[i]->operand[0]);
						locctr += tmploc;
					}
					else if (strcmp(token_table[i]->operato, "WORD") == 0)
					{
						locctr += 3;
					}
					else if (strcmp(token_table[i]->operato, "BYTE") == 0)
					{
						tmploc = (strlen(token_table[i]->operand[0]) - 3) / 2;
						locctr += tmploc;
					}
					else if (search_opcode(token_table[i]->operato) != -1)
					{
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
					}
					else
					{
						if (strcmp(token_table[i]->operato, "LTORG") == 0)
						{
							int k = 0;
							while (literal_table[k].literal != NULL)
							{
								literal_table[k].addr = locctr;
								locctr += 3;
								++k;
							}
							//literal_table[littab_index].addr = locctr;
							//locctr += strlen(literal_table[littab_index].literal);
						}
						//기타 다른 문자 LTORG, EQU, EXTREF를 읽은 경우 주소계산
						else if (strcmp(token_table[i]->operato, "END") == 0)
						{
							literal_table[littab_index].addr = locctr;
							locctr += strlen(literal_table[littab_index].literal)/2;
							wrrecsize = locctr;
							break;
						}
					}
				}
				//operand를 읽어 리터럴인지 아닌지 확인하는 과정
				if (strchr(token_table[i]->operand[0], '=') != NULL)
				{
					//literal table에 검색해본후, 없으면 저장 있으면 건너뛰기
					memset(littoken, '\0', 10);
					strcpy(littoken, token_table[i]->operand[0]);
					if (token_table[i]->operand[0][2]=='\0')
					{
						littoken2 = strtok(littoken, "=");
						if (littab_index == -1)
						{
							//처음이니까 저장
							++littab_index;
							literal_table[littab_index].literal = (char*)malloc(sizeof(char)*strlen(littoken2) + 1);
							strcpy(literal_table[littab_index].literal, littoken2);
							//litlocctr[litlocctrcount] = locctr;
							//++litlocctrcount;
							//literal_table[littab_index].addr = locctr;
						}
						else
						{
							if (isStoredLit(littoken2) == -1)
							{
								++littab_index;
								//insertLIT(littoken2);
								literal_table[littab_index].literal = (char*)malloc(sizeof(char)*strlen(littoken2) + 1);
								strcpy(literal_table[littab_index].literal, littoken2);
								//litlocctr[litlocctrcount] = locctr;
								//++litlocctrcount;
							}
						}
					}
					else
					{
						strtok(littoken, "\'");
						littoken2 = strtok(NULL, "\'");
						if (littab_index == -1)
						{
							//처음이니까 저장
							++littab_index;
							literal_table[littab_index].literal = (char*)malloc(sizeof(char)*strlen(littoken2) + 1);
							strcpy(literal_table[littab_index].literal, littoken2);
							//litlocctr[litlocctrcount] = locctr;
							//++litlocctrcount;
						}
						else
						{
							if (isStoredLit(littoken2) == -1)
							{
								++littab_index;
								//insertLIT(littoken2);
								literal_table[littab_index].literal = (char*)malloc(sizeof(char)*strlen(littoken2) + 1);
								strcpy(literal_table[littab_index].literal, littoken2);
								//litlocctr[litlocctrcount] = locctr;
								//++litlocctrcount;
							}
						}
					}	

				}
			}
			else {} //.이면 그냥 무시
		}
		++i;
	}
	outputline = i;
	return 0;
}


/*리터럴테이블에 저장되어있는지 확인하는 함수
입력: 문자열
출력: 리터럴의 위치 없으면 -1*/
int isStoredLit(char *str)
{
	int i = 0;
	int re;
	while (i <= littab_index)
	{
		if (strcmp(literal_table[i].literal, str)==0)
		{
			re = i;
			break;
		}
		++i;
	}
	if (i > littab_index)
		re=-1;
	return re;
}

/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 명령어 옆에 OPCODE가 기록된 표(과제 3번) 이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 만약 인자로 NULL값이 들어온다면 프로그램의 결과를 표준출력으로 보내어
*        화면에 출력해준다.
*        또한 과제 3번에서만 쓰이는 함수이므로 이후의 프로젝트에서는 사용되지 않는다.
* -----------------------------------------------------------------------------------
*/
void make_opcode_output(char *file_name)
{
	//label	operator	operand	opcode 순으로 파일에 작성
	FILE * file;
	file = fopen(file_name, "wt");
	int i = 0;
	int check; // -1이면 optable에 없는 것

	if (file != NULL)
	{
		fprintf(file, "%s\t%s\t\t%d\n", token_table[i]->label, token_table[i]->operato, i);
		++i;
		while (i <= outputline)
		{
			if (strcmp(token_table[i]->label, ".") == 0)
			{
				fprintf(file, "\n");
			}
			else
			{
				check = search_opcode(token_table[i]->operato);
				if (check == -1) //optable 에 없는 경우
				{
					if (strcmp(token_table[i]->operand[0], "") == 0)//operand 0
					{
						fprintf(file, "%s\t%s\n", token_table[i]->label, token_table[i]->operato);
					}
					else if ((strcmp(token_table[i]->operand[0], "") != 0) && (strcmp(token_table[i]->operand[1], "") == 0))
					{
						fprintf(file, "%s\t%s\t%s\t\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0]);
					}
					else if ((strcmp(token_table[i]->operand[1], "") != 0) && (strcmp(token_table[i]->operand[2], "") == 0))//operand 2
					{
						fprintf(file, "%s\t%s\t%s"",""%s\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0], token_table[i]->operand[1]);
					}
					else
					{
						fprintf(file, "%s\t%s\t%s"",""%s"",""%s\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0], token_table[i]->operand[1], token_table[i]->operand[2]);
					}

				}
				else //있는경우
				{
					//operand의 개수를 세서 개수별로 구분해 출력
					if (inst_table[check]->ops == 1) //인자가 1개
					{
						fprintf(file, "%s\t%s\t%s\t%x\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0], inst_table[check]->op);
					}
					else if (inst_table[check]->ops == 2)
					{
						fprintf(file, "%s\t%s\t%s"",""%s\t%x\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0], token_table[i]->operand[1], inst_table[check]->op);
					}
					else if (inst_table[check]->ops == 3)
					{
						fprintf(file, "%s\t%s\t%s"",""%s"",""%s\t%x\n", token_table[i]->label, token_table[i]->operato,token_table[i]->operand[0], token_table[i]->operand[1], token_table[i]->operand[2], inst_table[check]->op);
					}
					else
					{
						if (strcmp(token_table[i]->operato,"RSUB") == 0)
						{
							fprintf(file, "\t%s\t\t%x\n", token_table[i]->operato, inst_table[check]->op);
						}
						else
						{
							fprintf(file, "%s\t%s\t\t%x\n", token_table[i]->label, token_table[i]->operato,inst_table[check]->op);
						}

					}
				}
			}

			++i;
		}
		fclose(file);
	}
	printf("완료\n");
}

void freeALL() //메모리에 할당 받은 모든 테이블 할당해제
{
	int i;
	for (i = 0; i <= instruction_count; i++)
	{
		free(inst_table[i]);
	}
	for (i = 0; i < totalline; i++)
	{
		input_data[i] = NULL;
	}
	for (i = 0; i <= MAX_LINES; i++)
	{
		free(token_table[i]);
	}
	for (i = 0; i <= littab_index; i++)
	{
		free(literal_table[i].literal);
	}
}

/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 SYMBOL별 주소값이 저장된 TABLE이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 만약 인자로 NULL값이 들어온다면 프로그램의 결과를 표준출력으로 보내어
*        화면에 출력해준다.
*
* -----------------------------------------------------------------------------------*/
void make_symtab_output(char *file_name)
{
	int i = 0;
	if (file_name != NULL)
	{
		FILE *file;
		file = fopen(file_name, "wt");
		printf("\n=====SymbolTable=====\n");
		for (i = 0; i < symtab_index; i++)
		{
			printf("%s\t%4X\n", sym_table[i].symbol, sym_table[i].addr);
			fprintf(file, "%s\t%4X\n", sym_table[i].symbol, sym_table[i].addr);
		}
		fclose(file);
	}
	else
	{
		printf("\n=====SymbolTable=====\n");
		for (i = 0; i < symtab_index; i++)
		{
			printf("%s\t%4X\n", sym_table[i].symbol, sym_table[i].addr);
		}
	}
	
}



/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 LITERAL별 주소값이 저장된 TABLE이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 만약 인자로 NULL값이 들어온다면 프로그램의 결과를 표준출력으로 보내어
*        화면에 출력해준다.
*
* -----------------------------------------------------------------------------------*/
void make_literaltab_output(char *file_name)
{
	int i = 0;
	if (file_name != NULL)
	{
		FILE *file;
		file = fopen(file_name, "wt");
		printf("\n=====Literal Table=====\n");
		for (i = 0; i <= littab_index; i++)
		{
			printf("%s\t%4X\n", literal_table[i].literal, literal_table[i].addr);
			fprintf(file, "%s\t%4X\n", literal_table[i].literal, literal_table[i].addr);
		}
		fclose(file);
	}
	else
	{
		printf("\n=====Literal Table=====\n");
		for (i = 0; i <= littab_index; i++)
		{
			printf("%s\t%4X\n", literal_table[i].literal, literal_table[i].addr);
		}
	}
	
}

/*
새로추가 getLittabAddr
입력: 토큰 테이블의 operand
출력: 입력에 맞는 Iiteral의 주소, 없으면 -1*/
int getLittabAddr(char *str)
{
	int i = 0;
	int re;
	char inputstr[20];
	strcpy(inputstr, str);
	while (i <= littab_index)
	{
		if (strstr(inputstr, literal_table[i].literal) != NULL)
		{
			re = literal_table[i].addr;
			break;
		}
		++i;
	}
	if (i > littab_index)
		re = -1;
	return re;
}

/* ----------------------------------------------------------------------------------
* 설명 : 어셈블리 코드를 기계어 코드로 바꾸기 위한 패스2 과정을 수행하는 함수이다.
*		   패스 2에서는 프로그램을 기계어로 바꾸는 작업은 라인 단위로 수행된다.
*		   다음과 같은 작업이 수행되어 진다.
*		   1. 실제로 해당 어셈블리 명령어를 기계어로 바꾸는 작업을 수행한다.
* 매계 : 없음
* 반환 : 정상종료 = 0, 에러발생 = < 0
* 주의 :
* -----------------------------------------------------------------------------------*/
static int assem_pass2(void) //operator 와 operand를 읽어 인스트럭션 코드를 완성한다.
{
	int i = 0;
	unsigned char opcode;
	short xbpe=0;
	int operand_address;
	char instbuffer[10];
	int r1;
	int	r2; //2형식 레지스터 출력용
	memset(object_line, '\0', 300);
	int line_acumal = 0; //오브젝트 코드 라인당 들어가는 바이트 수를 저장

	Modify_table m[20];
	int modifyindex = 0;

	while (i <= outputline)
	{
		if (i == 0)
			strcpy(object_line," ");
		operand_address = 0;
		memset(instbuffer, '\0', 10);
		r1 = r2 = 0;
		if (strcmp(token_table[i]->label, ".") != 0) //label이 . 이 아닌 경우
		{
			if (strcmp(token_table[i]->operato, "START") == 0)
			{
				locctr = 0;
				printf("\nH%s%06X%06X\n",token_table[i]->label,locctr,copy_size);
			}
			else if (strcmp(token_table[i]->operato, "END") == 0)
			{
				//printf("%c%c\n", literal_table[1].literal[0], literal_table[1].literal[1]);
				sprintf(instbuffer,"%c%c\n", literal_table[1].literal[0], literal_table[1].literal[1]);
				strcat(object_line, instbuffer);
				locctr =locctr+ strlen(literal_table[littab_index].literal)-1;
				line_acumal=line_acumal+ strlen(literal_table[littab_index].literal) - 1;
				printf("%02X%s\n", line_acumal, object_line);
				memset(object_line, '\0', 300);
				strcpy(object_line, " ");
				//printf("\nM%06X%02X%s\n");
				for (int j = 0; j < modifyindex; j++)
				{
					if (condition_flag == m[j].condition && m[j].toFix == 5) {
						printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
					}
					else if (condition_flag == m[j].condition && m[j].toFix == 6)
					{
						printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
						printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
					}
					else {}
				}
				printf("E\n");
				break;
			}
			else if (strcmp(token_table[i]->operato, "EXTDEF") == 0)
			{
				printf("D%s%06X%s%06X%s%06X\n",token_table[i]->operand[0],getSymtabAddr(token_table[i]->operand[0])
				, token_table[i]->operand[1], getSymtabAddr(token_table[i]->operand[1])
				, token_table[i]->operand[2], getSymtabAddr(token_table[i]->operand[2]));
			}
			else if (strcmp(token_table[i]->operato, "EXTREF") == 0)
			{
				printf("R%s", token_table[i]->operand[0]);
				if (strcmp(token_table[i]->operand[1], "") != 0)
					printf("%s", token_table[i]->operand[1]);
				if (strcmp(token_table[i]->operand[2], "") != 0)
					printf("%s", token_table[i]->operand[2]);
				printf("\nT%06X",locctr);
			}
			else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "RDREC") == 0)
			{
				
				line_acumal = 0;
				locctr = 0;
				//printf("\nM%06X%02X%s\n");
				for (int j = 0; j < modifyindex; j++)
				{
					if (condition_flag == m[j].condition && m[j].toFix==5) {
						printf("M%06X%02X+%s\n",m[j].addr,m[j].toFix,m[j].operand);
					}
					else if (condition_flag == m[j].condition && m[j].toFix == 6)
					{
						printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand,"-"));
						printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
					}
					else {}
				}
				printf("E%06X\n", locctr);
				printf("\nH%s%06X%06X\n",token_table[i]->label,locctr,rdrecsize);
				++condition_flag;
			}
			else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "WRREC") == 0)
			{
				printf("%02X%s\n", line_acumal, object_line);
				memset(object_line, '\0', 300);
				strcpy(object_line, " ");
				locctr = 0;
				line_acumal = 0;
				//printf("\nM%06X%02X%s\n");
				for (int j = 0; j < modifyindex; j++)
				{
					if (condition_flag == m[j].condition && m[j].toFix == 5) {
						printf("\nM%06X%02X+%s", m[j].addr, m[j].toFix, m[j].operand);
					}
					else if (condition_flag == m[j].condition && m[j].toFix == 6)
					{
						printf("\nM%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
						printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
					}
					else {}
				}
				printf("E\n");
				printf("\nH%s%06X%06X\n", token_table[i]->label, locctr, wrrecsize);
				++condition_flag;
			}
			else if (strcmp(token_table[i]->operato, "RESW") == 0)
			{
				locctr += 3 * atoi(token_table[i]->operand[0]);
			}
			else if (strcmp(token_table[i]->operato, "RESB") == 0)
			{
				locctr += atoi(token_table[i]->operand[0]);
			}
			else if (strcmp(token_table[i]->operato, "WORD") == 0)
			{
				if (line_acumal >= 29)
				{
					printf("%02X%s\n", line_acumal, object_line);
					line_acumal = 0;
					printf("T%06X", locctr);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
				}
				m[modifyindex].addr = locctr;
				m[modifyindex].condition = condition_flag;
				m[modifyindex].toFix = 6;
				strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
				++modifyindex;
				locctr += 3;
				line_acumal += 3;
				//printf("%06d",operand_address);
				sprintf(instbuffer, "%06d", operand_address);
				strcat(object_line, instbuffer);
			}
			else if (strcmp(token_table[i]->operato, "BYTE") == 0)
			{
				if (line_acumal >= 29)
				{
					printf("%02X%s\n", line_acumal, object_line);
					line_acumal = 0;
					printf("T%06X", locctr);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
				}
				//printf("%c%c",token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
				sprintf(instbuffer, "%c%c", token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
				strcat(object_line,instbuffer);
				locctr += (strlen(token_table[i]->operand[0]) - 3) / 2;
				line_acumal+= (strlen(token_table[i]->operand[0]) - 3) / 2;
			}
			else if (strcmp(token_table[i]->operato, "LTORG") == 0)
			{
				printf("%02X%s\n", line_acumal, object_line);
				line_acumal = 0;
				memset(object_line, '\0', 300);
				strcpy(object_line, " ");
				printf("\nT%06X",locctr);
				printf("%02X",strlen(literal_table[0].literal));
				printf("%X""%X""%X\n",literal_table[0].literal[0], literal_table[0].literal[1], literal_table[0].literal[2]);
				
				locctr += strlen(literal_table[0].literal);
				
			}
			else
			{
				if (token_table[i]->nixbpe % 2 == 1 && token_table[i]->nixbpe >= 48) //4형식
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n",line_acumal,object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					m[modifyindex].addr = locctr + 1;
					m[modifyindex].toFix = 5;
					m[modifyindex].condition = condition_flag;
					strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
					++modifyindex;
					locctr += 4;
					line_acumal += 4;
					opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
					xbpe = token_table[i]->nixbpe - 48;
					operand_address = 0;
					
					//printf("%02X""%hhd""%05X", opcode, xbpe, (operand_address)&mask4);
					sprintf(instbuffer, "%X""%hhd""05X", opcode, xbpe, (operand_address)&mask4);
					strcat(object_line, instbuffer);
				}
				else if (token_table[i]->nixbpe >= 48 && token_table[i]->nixbpe % 2 == 0) //sicxe 단순 어드레싱
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					locctr += 3;
					line_acumal += 3;
					opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
					xbpe = token_table[i]->nixbpe - 48;
					if (xbpe==2)
					{
						operand_address = getSymtabAddr(token_table[i]->operand[0]);
						if (operand_address == -1)
						{
							operand_address = getLittabAddr(token_table[i]->operand[0]);
							if (operand_address == -1)
							{
								operand_address = 0;
								//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								strcat(object_line, instbuffer);
							}
							else
							{
								operand_address = operand_address - locctr;
								
								//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								strcat(object_line, instbuffer);
							}					
						}
						else
						{
							operand_address = operand_address - locctr;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
						
					}
					else
					{
						operand_address = 0;
						//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						sprintf(instbuffer,"%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						strcat(object_line,instbuffer);
					}
				}
				else if (token_table[i]->nixbpe < 48 && token_table[i]->nixbpe >= 32) //indirect 어드레싱 @operand
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
					line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

					opcode = inst_table[search_opcode(token_table[i]->operato)]->op+2;
					xbpe = token_table[i]->nixbpe - 32;
					//operand_address구하기
					if (xbpe == 2)
					{
						operand_address = getSymtabAddr(token_table[i]->operand[0]);
						operand_address = operand_address - locctr;
						//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						sprintf(instbuffer,"%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						strcat(object_line, instbuffer);
					}
					else
					{
						operand_address = 0;
						//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						strcat(object_line, instbuffer);
					}
				}
				else if (token_table[i]->nixbpe < 32 && token_table[i]->nixbpe >= 16) //immediate 어드레싱 #operand
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
					line_acumal+= inst_table[search_opcode(token_table[i]->operato)]->format;

					opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 1;
					xbpe= token_table[i]->nixbpe - 16;
					operand_address =token_table[i]->operand[0][1]-48;
					//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
					sprintf(instbuffer,"%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
					strcat(object_line, instbuffer);
				}
				else if (search_opcode(token_table[i]->operato) != -1 && inst_table[search_opcode(token_table[i]->operato)]->format < 3)//2형식
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					locctr+= inst_table[search_opcode(token_table[i]->operato)]->format;
					line_acumal+= inst_table[search_opcode(token_table[i]->operato)]->format;

					opcode = inst_table[search_opcode(token_table[i]->operato)]->op;
					//A:0, S:4, T:5, X:1
					if (inst_table[search_opcode(token_table[i]->operato)]->ops == 1)//register 1개
					{
						if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
						else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
						else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
						else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
						else {}
						//printf("%02X%X%X",opcode,r1,r2);
						sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
						strcat(object_line, instbuffer);
					}
					else //register 2개
					{
						if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
						else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
						else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
						else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
						else {}
						if (strcmp(token_table[i]->operand[1], "A") == 0) { r2 = 0; }
						else if (strcmp(token_table[i]->operand[1], "S") == 0) { r2 = 4; }
						else if (strcmp(token_table[i]->operand[1], "T") == 0) { r2 = 5; }
						else if (strcmp(token_table[i]->operand[1], "X") == 0) { r2 = 1; }
						else {}
						//printf("%X%X%X", opcode, r1, r2);
						sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
						strcat(object_line, instbuffer);
					}
				}
			}
		}
		else // .인 경우는 무시
		{

		}
		++i;
	}
	condition_flag = 0;
}

/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 object code (프로젝트 1번) 이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 만약 인자로 NULL값이 들어온다면 프로그램의 결과를 표준출력으로 보내어
*        화면에 출력해준다.
*
* -----------------------------------------------------------------------------------*/
void make_objectcode_output(char *file_name)
{
	if (file_name != NULL)
	{
		FILE *file;
		file = fopen(file_name, "wt");
		/* add your code here */
		int i = 0;
		unsigned char opcode;
		short xbpe = 0;
		int operand_address;
		char instbuffer[10];
		int r1;
		int	r2; //2형식 레지스터 출력용
		memset(object_line, '\0', 300);
		int line_acumal = 0; //오브젝트 코드 라인당 들어가는 바이트 수를 저장

		Modify_table m[20];
		int modifyindex = 0;

		while (i <= outputline)
		{
			if (i == 0)
				strcpy(object_line, " ");
			operand_address = 0;
			memset(instbuffer, '\0', 10);
			r1 = r2 = 0;
			if (strcmp(token_table[i]->label, ".") != 0) //label이 . 이 아닌 경우
			{
				if (strcmp(token_table[i]->operato, "START") == 0)
				{
					locctr = 0;
					fprintf(file,"H%s%06X%06X\n", token_table[i]->label, locctr, copy_size);
				}
				else if (strcmp(token_table[i]->operato, "END") == 0)
				{
					//printf("%c%c\n", literal_table[1].literal[0], literal_table[1].literal[1]);
					sprintf(instbuffer, "%c%c", literal_table[1].literal[0], literal_table[1].literal[1]);
					strcat(object_line, instbuffer);
					locctr = locctr + strlen(literal_table[littab_index].literal) - 1;
					line_acumal = line_acumal + strlen(literal_table[littab_index].literal) - 1;
					fprintf(file,"%02X%s\n", line_acumal, object_line);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							fprintf(file,"M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					fprintf(file,"E\n");
					break;
				}
				else if (strcmp(token_table[i]->operato, "EXTDEF") == 0)
				{
					fprintf(file,"D%s%06X%s%06X%s%06X\n", token_table[i]->operand[0], getSymtabAddr(token_table[i]->operand[0])
						, token_table[i]->operand[1], getSymtabAddr(token_table[i]->operand[1])
						, token_table[i]->operand[2], getSymtabAddr(token_table[i]->operand[2]));
				}
				else if (strcmp(token_table[i]->operato, "EXTREF") == 0)
				{
					fprintf(file,"R%s", token_table[i]->operand[0]);
					if (strcmp(token_table[i]->operand[1], "") != 0)
						fprintf(file,"%s", token_table[i]->operand[1]);
					if (strcmp(token_table[i]->operand[2], "") != 0)
						fprintf(file,"%s", token_table[i]->operand[2]);
					fprintf(file,"\nT%06X", locctr);
				}
				else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "RDREC") == 0)
				{

					line_acumal = 0;
					locctr = 0;
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							fprintf(file,"M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					fprintf(file,"E%06X\n", locctr);
					fprintf(file,"H%s%06X%06X\n", token_table[i]->label, locctr, rdrecsize);
					++condition_flag;
				}
				else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "WRREC") == 0)
				{
					fprintf(file,"%02X%s\n", line_acumal, object_line);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					locctr = 0;
					line_acumal = 0;
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							fprintf(file,"M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							fprintf(file,"M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					fprintf(file,"E\n");
					fprintf(file,"H%s%06X%06X\n", token_table[i]->label, locctr, wrrecsize);
					++condition_flag;
				}
				else if (strcmp(token_table[i]->operato, "RESW") == 0)
				{
					locctr += 3 * atoi(token_table[i]->operand[0]);
				}
				else if (strcmp(token_table[i]->operato, "RESB") == 0)
				{
					locctr += atoi(token_table[i]->operand[0]);
				}
				else if (strcmp(token_table[i]->operato, "WORD") == 0)
				{
					if (line_acumal >= 29)
					{
						fprintf(file,"%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						fprintf(file,"T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					m[modifyindex].addr = locctr;
					m[modifyindex].condition = condition_flag;
					m[modifyindex].toFix = 6;
					strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
					++modifyindex;
					locctr += 3;
					line_acumal += 3;
					//printf("%06d",operand_address);
					sprintf(instbuffer, "%06d", operand_address);
					strcat(object_line, instbuffer);
				}
				else if (strcmp(token_table[i]->operato, "BYTE") == 0)
				{
					if (line_acumal >= 29)
					{
						fprintf(file,"%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						fprintf(file,"T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					//printf("%c%c",token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
					sprintf(instbuffer, "%c%c", token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
					strcat(object_line, instbuffer);
					locctr += (strlen(token_table[i]->operand[0]) - 3) / 2;
					line_acumal += (strlen(token_table[i]->operand[0]) - 3) / 2;
				}
				else if (strcmp(token_table[i]->operato, "LTORG") == 0)
				{
					fprintf(file,"%02X%s\n", line_acumal, object_line);
					line_acumal = 0;
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					fprintf(file,"T%06X", locctr);
					fprintf(file,"%02X", strlen(literal_table[0].literal)+2+ strlen(literal_table[1].literal)+2+ strlen(literal_table[2].literal));
					fprintf(file,"%06X",atoi(literal_table[0].literal));
					fprintf(file, "%X""%X""%X", literal_table[1].literal[0], literal_table[1].literal[1], literal_table[1].literal[2]);
					fprintf(file, "%06X\n", atoi(literal_table[2].literal));
					
					locctr += 9;
					//locctr += strlen(literal_table[0].literal);

				}
				else
				{
					if (token_table[i]->nixbpe % 2 == 1 && token_table[i]->nixbpe >= 48) //4형식
					{
						if (line_acumal >= 29)
						{
							fprintf(file,"%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							fprintf(file,"T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						m[modifyindex].addr = locctr + 1;
						m[modifyindex].toFix = 5;
						m[modifyindex].condition = condition_flag;
						strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
						++modifyindex;
						locctr += 4;
						line_acumal += 4;
						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
						xbpe = token_table[i]->nixbpe - 48;
						operand_address = 0;

						//printf("%02X""%hhd""%05X", opcode, xbpe, (operand_address)&mask4);
						sprintf(instbuffer, "%X""%hhd""%05X", opcode, xbpe, (operand_address)&mask4);
						strcat(object_line, instbuffer);
					}
					else if (token_table[i]->nixbpe >= 48 && token_table[i]->nixbpe % 2 == 0) //sicxe 단순 어드레싱
					{
						if (line_acumal >= 29)
						{
							fprintf(file,"%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							fprintf(file,"T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += 3;
						line_acumal += 3;
						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
						xbpe = token_table[i]->nixbpe - 48;
						if (xbpe == 2)
						{
							operand_address = getSymtabAddr(token_table[i]->operand[0]);
							if (operand_address == -1)
							{
								operand_address = getLittabAddr(token_table[i]->operand[0]);
								if (operand_address == -1)
								{
									operand_address = 0;
									//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									strcat(object_line, instbuffer);
								}
								else
								{
									operand_address = operand_address - locctr;

									//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									strcat(object_line, instbuffer);
								}
							}
							else
							{
								operand_address = operand_address - locctr;
								//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								strcat(object_line, instbuffer);
							}

						}
						else
						{
							operand_address = 0;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
					}
					else if (token_table[i]->nixbpe < 48 && token_table[i]->nixbpe >= 32) //indirect 어드레싱 @operand
					{
						if (line_acumal >= 29)
						{
							fprintf(file,"%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							fprintf(file,"T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 2;
						xbpe = token_table[i]->nixbpe - 32;
						//operand_address구하기
						if (xbpe == 2)
						{
							operand_address = getSymtabAddr(token_table[i]->operand[0]);
							operand_address = operand_address - locctr;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
						else
						{
							operand_address = 0;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
					}
					else if (token_table[i]->nixbpe < 32 && token_table[i]->nixbpe >= 16) //immediate 어드레싱 #operand
					{
						if (line_acumal >= 29)
						{
							fprintf(file,"%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							fprintf(file,"T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 1;
						xbpe = token_table[i]->nixbpe - 16;
						operand_address = token_table[i]->operand[0][1] - 48;
						//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						strcat(object_line, instbuffer);
					}
					else if (search_opcode(token_table[i]->operato) != -1 && inst_table[search_opcode(token_table[i]->operato)]->format < 3)//2형식
					{
						if (line_acumal >= 29)
						{
							fprintf(file,"%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							fprintf(file,"T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op;
						//A:0, S:4, T:5, X:1
						if (inst_table[search_opcode(token_table[i]->operato)]->ops == 1)//register 1개
						{
							if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
							else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
							else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
							else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
							else {}
							//printf("%02X%X%X",opcode,r1,r2);
							sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
							strcat(object_line, instbuffer);
						}
						else //register 2개
						{
							if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
							else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
							else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
							else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
							else {}
							if (strcmp(token_table[i]->operand[1], "A") == 0) { r2 = 0; }
							else if (strcmp(token_table[i]->operand[1], "S") == 0) { r2 = 4; }
							else if (strcmp(token_table[i]->operand[1], "T") == 0) { r2 = 5; }
							else if (strcmp(token_table[i]->operand[1], "X") == 0) { r2 = 1; }
							else {}
							//printf("%X%X%X", opcode, r1, r2);
							sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
							strcat(object_line, instbuffer);
						}
					}
				}
			}
			else // .인 경우는 무시
			{

			}
			++i;
		}
		condition_flag = 0;
		fclose(file);
	}
	else
	{
		int i = 0;
		unsigned char opcode;
		short xbpe = 0;
		int operand_address;
		char instbuffer[10];
		int r1;
		int	r2; //2형식 레지스터 출력용
		memset(object_line, '\0', 300);
		int line_acumal = 0; //오브젝트 코드 라인당 들어가는 바이트 수를 저장

		Modify_table m[20];
		int modifyindex = 0;

		while (i <= outputline)
		{
			if (i == 0)
				strcpy(object_line, " ");
			operand_address = 0;
			memset(instbuffer, '\0', 10);
			r1 = r2 = 0;
			if (strcmp(token_table[i]->label, ".") != 0) //label이 . 이 아닌 경우
			{
				if (strcmp(token_table[i]->operato, "START") == 0)
				{
					locctr = 0;
					printf("\nH%s%06X%06X\n", token_table[i]->label, locctr, copy_size);
				}
				else if (strcmp(token_table[i]->operato, "END") == 0)
				{
					//printf("%c%c\n", literal_table[1].literal[0], literal_table[1].literal[1]);
					sprintf(instbuffer, "%c%c\n", literal_table[1].literal[0], literal_table[1].literal[1]);
					strcat(object_line, instbuffer);
					locctr = locctr + strlen(literal_table[littab_index].literal) - 1;
					line_acumal = line_acumal + strlen(literal_table[littab_index].literal) - 1;
					printf("%02X%s\n", line_acumal, object_line);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					printf("E\n");
					break;
				}
				else if (strcmp(token_table[i]->operato, "EXTDEF") == 0)
				{
					printf("D%s%06X%s%06X%s%06X\n", token_table[i]->operand[0], getSymtabAddr(token_table[i]->operand[0])
						, token_table[i]->operand[1], getSymtabAddr(token_table[i]->operand[1])
						, token_table[i]->operand[2], getSymtabAddr(token_table[i]->operand[2]));
				}
				else if (strcmp(token_table[i]->operato, "EXTREF") == 0)
				{
					printf("R%s", token_table[i]->operand[0]);
					if (strcmp(token_table[i]->operand[1], "") != 0)
						printf("%s", token_table[i]->operand[1]);
					if (strcmp(token_table[i]->operand[2], "") != 0)
						printf("%s", token_table[i]->operand[2]);
					printf("\nT%06X", locctr);
				}
				else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "RDREC") == 0)
				{

					line_acumal = 0;
					locctr = 0;
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							printf("M%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					printf("E%06X\n", locctr);
					printf("\nH%s%06X%06X\n", token_table[i]->label, locctr, rdrecsize);
					++condition_flag;
				}
				else if (strcmp(token_table[i]->operato, "CSECT") == 0 && strcmp(token_table[i]->label, "WRREC") == 0)
				{
					printf("%02X%s\n", line_acumal, object_line);
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					locctr = 0;
					line_acumal = 0;
					//printf("\nM%06X%02X%s\n");
					for (int j = 0; j < modifyindex; j++)
					{
						if (condition_flag == m[j].condition && m[j].toFix == 5) {
							printf("\nM%06X%02X+%s", m[j].addr, m[j].toFix, m[j].operand);
						}
						else if (condition_flag == m[j].condition && m[j].toFix == 6)
						{
							printf("\nM%06X%02X+%s\n", m[j].addr, m[j].toFix, strtok(m[j].operand, "-"));
							printf("M%06X%02X-%s\n", m[j].addr, m[j].toFix, strtok(NULL, "-"));
						}
						else {}
					}
					printf("E\n");
					printf("\nH%s%06X%06X\n", token_table[i]->label, locctr, wrrecsize);
					++condition_flag;
				}
				else if (strcmp(token_table[i]->operato, "RESW") == 0)
				{
					locctr += 3 * atoi(token_table[i]->operand[0]);
				}
				else if (strcmp(token_table[i]->operato, "RESB") == 0)
				{
					locctr += atoi(token_table[i]->operand[0]);
				}
				else if (strcmp(token_table[i]->operato, "WORD") == 0)
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					m[modifyindex].addr = locctr;
					m[modifyindex].condition = condition_flag;
					m[modifyindex].toFix = 6;
					strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
					++modifyindex;
					locctr += 3;
					line_acumal += 3;
					//printf("%06d",operand_address);
					sprintf(instbuffer, "%06d", operand_address);
					strcat(object_line, instbuffer);
				}
				else if (strcmp(token_table[i]->operato, "BYTE") == 0)
				{
					if (line_acumal >= 29)
					{
						printf("%02X%s\n", line_acumal, object_line);
						line_acumal = 0;
						printf("T%06X", locctr);
						memset(object_line, '\0', 300);
						strcpy(object_line, " ");
					}
					//printf("%c%c",token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
					sprintf(instbuffer, "%c%c", token_table[i]->operand[0][2], token_table[i]->operand[0][3]);
					strcat(object_line, instbuffer);
					locctr += (strlen(token_table[i]->operand[0]) - 3) / 2;
					line_acumal += (strlen(token_table[i]->operand[0]) - 3) / 2;
				}
				else if (strcmp(token_table[i]->operato, "LTORG") == 0)
				{
					printf("%02X%s\n", line_acumal, object_line);
					line_acumal = 0;
					memset(object_line, '\0', 300);
					strcpy(object_line, " ");
					printf("\nT%06X", locctr);
					printf("%02X", strlen(literal_table[0].literal));
					printf("%X""%X""%X\n", literal_table[0].literal[0], literal_table[0].literal[1], literal_table[0].literal[2]);

					locctr += 3;

				}
				else
				{
					if (token_table[i]->nixbpe % 2 == 1 && token_table[i]->nixbpe >= 48) //4형식
					{
						if (line_acumal >= 29)
						{
							printf("%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							printf("T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						m[modifyindex].addr = locctr + 1;
						m[modifyindex].toFix = 5;
						m[modifyindex].condition = condition_flag;
						strcpy(m[modifyindex].operand, token_table[i]->operand[0]);
						++modifyindex;
						locctr += 4;
						line_acumal += 4;
						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
						xbpe = token_table[i]->nixbpe - 48;
						operand_address = 0;

						//printf("%02X""%hhd""%05X", opcode, xbpe, (operand_address)&mask4);
						sprintf(instbuffer, "%X""%hhd""%X", opcode, xbpe, (operand_address)&mask4);
						strcat(object_line, instbuffer);
					}
					else if (token_table[i]->nixbpe >= 48 && token_table[i]->nixbpe % 2 == 0) //sicxe 단순 어드레싱
					{
						if (line_acumal >= 29)
						{
							printf("%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							printf("T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += 3;
						line_acumal += 3;
						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 3;
						xbpe = token_table[i]->nixbpe - 48;
						if (xbpe == 2)
						{
							operand_address = getSymtabAddr(token_table[i]->operand[0]);
							if (operand_address == -1)
							{
								operand_address = getLittabAddr(token_table[i]->operand[0]);
								if (operand_address == -1)
								{
									operand_address = 0;
									//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									strcat(object_line, instbuffer);
								}
								else
								{
									operand_address = operand_address - locctr;

									//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
									strcat(object_line, instbuffer);
								}
							}
							else
							{
								operand_address = operand_address - locctr;
								//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
								strcat(object_line, instbuffer);
							}

						}
						else
						{
							operand_address = 0;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
					}
					else if (token_table[i]->nixbpe < 48 && token_table[i]->nixbpe >= 32) //indirect 어드레싱 @operand
					{
						if (line_acumal >= 29)
						{
							printf("%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							printf("T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 2;
						xbpe = token_table[i]->nixbpe - 32;
						//operand_address구하기
						if (xbpe == 2)
						{
							operand_address = getSymtabAddr(token_table[i]->operand[0]);
							operand_address = operand_address - locctr;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
						else
						{
							operand_address = 0;
							//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
							strcat(object_line, instbuffer);
						}
					}
					else if (token_table[i]->nixbpe < 32 && token_table[i]->nixbpe >= 16) //immediate 어드레싱 #operand
					{
						if (line_acumal >= 29)
						{
							printf("%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							printf("T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op + 1;
						xbpe = token_table[i]->nixbpe - 16;
						operand_address = token_table[i]->operand[0][1] - 48;
						//printf("%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						sprintf(instbuffer, "%02X""%hhd""%03X", opcode, xbpe, (operand_address)&mask3);
						strcat(object_line, instbuffer);
					}
					else if (search_opcode(token_table[i]->operato) != -1 && inst_table[search_opcode(token_table[i]->operato)]->format < 3)//2형식
					{
						if (line_acumal >= 29)
						{
							printf("%02X%s\n", line_acumal, object_line);
							line_acumal = 0;
							printf("T%06X", locctr);
							memset(object_line, '\0', 300);
							strcpy(object_line, " ");
						}
						locctr += inst_table[search_opcode(token_table[i]->operato)]->format;
						line_acumal += inst_table[search_opcode(token_table[i]->operato)]->format;

						opcode = inst_table[search_opcode(token_table[i]->operato)]->op;
						//A:0, S:4, T:5, X:1
						if (inst_table[search_opcode(token_table[i]->operato)]->ops == 1)//register 1개
						{
							if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
							else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
							else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
							else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
							else {}
							//printf("%02X%X%X",opcode,r1,r2);
							sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
							strcat(object_line, instbuffer);
						}
						else //register 2개
						{
							if (strcmp(token_table[i]->operand[0], "A") == 0) { r1 = 0; }
							else if (strcmp(token_table[i]->operand[0], "S") == 0) { r1 = 4; }
							else if (strcmp(token_table[i]->operand[0], "T") == 0) { r1 = 5; }
							else if (strcmp(token_table[i]->operand[0], "X") == 0) { r1 = 1; }
							else {}
							if (strcmp(token_table[i]->operand[1], "A") == 0) { r2 = 0; }
							else if (strcmp(token_table[i]->operand[1], "S") == 0) { r2 = 4; }
							else if (strcmp(token_table[i]->operand[1], "T") == 0) { r2 = 5; }
							else if (strcmp(token_table[i]->operand[1], "X") == 0) { r2 = 1; }
							else {}
							//printf("%X%X%X", opcode, r1, r2);
							sprintf(instbuffer, "%02X%X%X", opcode, r1, r2);
							strcat(object_line, instbuffer);
						}
					}
				}
			}
			else // .인 경우는 무시
			{

			}
			++i;
		}
		condition_flag = 0;
	}
}
