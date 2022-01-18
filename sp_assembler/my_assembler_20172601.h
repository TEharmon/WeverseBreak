/*
* my_assembler 함수를 위한 변수 선언 및 매크로를 담고 있는 헤더 파일이다.
*/
#define MAX_INST 256
#define MAX_LINES 5000
#define MAX_OPERAND 3

/*
* instruction 목록 파일로 부터 정보를 받아와서 생성하는 구조체 변수이다.
* 라인 별로 하나의 instruction을 저장한다.
*/
struct inst_unit
{
	char str[10];
	unsigned char op;
	int format;
	int ops;
};
typedef struct inst_unit inst;
inst *inst_table[MAX_INST];
int inst_index;

/*
* 어셈블리 할 소스코드를 입력받는 테이블이다. 라인 단위로 관리할 수 있다.
*/
char *input_data[MAX_LINES];
static int line_num;

/*
* 어셈블리 할 소스코드를 토큰단위로 관리하기 위한 구조체 변수이다.
* operator는 renaming을 허용한다.
*/
struct token_unit
{
	char *label;
	char *operato;
	char operand[MAX_OPERAND][20];
	char comment[100];
	
	char nixbpe; //하위 6비트 사용 _ _ n i x b p e
				 //                  32 16 8 4 2 1 
};

typedef struct token_unit token;
token *token_table[MAX_LINES];
static int token_line;

/*
* 심볼을 관리하는 구조체이다.
* 심볼 테이블은 심볼 이름, 심볼의 위치로 구성된다.
* 추후 과제에 사용 예정
*/
struct symbol_unit
{
	char symbol[10];
	int addr;
};

/*
* 리터럴을 관리하는 구조체이다.
* 리터럴 테이블은 리터럴의 이름, 리터럴의 위치로 구성된다.
* 추후 프로젝트에서 사용된다.
*/
struct literal_unit
{
	char *literal;
	int addr;
};

typedef struct symbol_unit symbol;
symbol sym_table[MAX_LINES];
int symtab_index = 0; //새로추가

typedef struct literal_unit literal;
literal literal_table[MAX_LINES];
int littab_index = -1; //새로추가
static int locctr;
//--------------

static char *input_file;
static char *output_file;
int init_my_assembler(void);
int init_inst_file(char *inst_file);
int init_input_file(char *input_file);
int token_parsing(char *str);
int search_opcode(char *str);
static int assem_pass1(void);
void make_opcode_output(char *file_name);

void make_symtab_output(char *file_name);
void make_literaltab_output(char *file_name);
static int assem_pass2(void);
void make_objectcode_output(char *file_name);
int getSymtabAddr(char *str); //새로추가
int isStoredLit(char *str); //새로추가
int getLittabAddr(char *str); //새로 추가 

typedef struct _ModificationTable
{
	int addr; //수정해야 할 주소위치
	char operand[20]; //인자
	int condition; //0: copy  1:rdrec  2:wrrec
	int toFix; //고쳐야 할 바이트 4형식이면 5, word면 6
} Modify_table;
