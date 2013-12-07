#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20130304

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

#define YYPREFIX "yy"

#define YYPURE 0

#line 2 "code/tools/lcc/lburg/gram.y"
#include <stdio.h>
#include "lburg.h"
static char rcsid[] = "$Id: gram.y 145 2001-10-17 21:53:10Z timo $";
/*lint -e616 -e527 -e652 -esym(552,yynerrs) -esym(563,yynewstate,yyerrlab) */
static int yylineno = 0;
#line 8 "code/tools/lcc/lburg/gram.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	int n;
	char *string;
	Tree tree;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 38 "y.tab.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define TERMINAL 257
#define START 258
#define PPERCENT 259
#define ID 260
#define TEMPLATE 261
#define CODE 262
#define INT 263
#define YYERRCODE 256
static const short yylhs[] = {                           -1,
    0,    0,    4,    4,    6,    6,    6,    6,    7,    7,
    5,    5,    5,    5,    1,    3,    3,    3,    2,
};
static const short yylen[] = {                            2,
    3,    1,    0,    2,    3,    3,    1,    2,    0,    4,
    0,    7,    2,    3,    1,    1,    4,    6,    1,
};
static const short yydefred[] = {                         3,
    0,    0,    0,    9,    0,   11,    7,    4,    8,    0,
   15,    0,    0,    0,    5,    6,    0,   13,    0,    0,
   14,    0,   10,    0,    0,    0,    0,    0,   19,    0,
   17,    0,   12,    0,   18,
};
static const short yydgoto[] = {                          1,
   12,   30,   25,    2,   13,    8,   10,
};
static const short yysindex[] = {                         0,
    0,   -4,   -2,    0, -250,    0,    0,    0,    0,   -9,
    0,    1,  -10,  -49,    0,    0,    3,    0,  -44, -248,
    0, -244,    0,  -22, -242, -244, -245,  -37,    0,   10,
    0, -244,    0,  -20,    0,
};
static const short yyrindex[] = {                         0,
    0,   22,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   23,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -39,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
static const short yygindex[] = {                         0,
   11,    0,  -23,    0,    0,    0,    0,
};
#define YYTABLESIZE 255
static const short yytable[] = {                         18,
   15,   16,   28,   31,   16,    7,   32,    9,   34,   11,
   16,   20,   21,   22,   23,   24,   29,   26,   27,   33,
   35,    2,    1,   19,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   16,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   17,    0,    0,    0,   11,
   14,    3,    4,    5,    6,
};
static const short yycheck[] = {                         10,
   10,   41,   26,   41,   44,   10,   44,   10,   32,  260,
   10,   61,   10,   58,  263,  260,  262,   40,  261,   10,
   41,    0,    0,   13,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  261,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  256,   -1,   -1,   -1,  260,
  260,  256,  257,  258,  259,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 263
#if YYDEBUG
static const char *yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,"'('","')'",0,0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,
"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"TERMINAL","START","PPERCENT","ID","TEMPLATE","CODE","INT",
};
static const char *yyrule[] = {
"$accept : spec",
"spec : decls PPERCENT rules",
"spec : decls",
"decls :",
"decls : decls decl",
"decl : TERMINAL blist '\\n'",
"decl : START nonterm '\\n'",
"decl : '\\n'",
"decl : error '\\n'",
"blist :",
"blist : blist ID '=' INT",
"rules :",
"rules : rules nonterm ':' tree TEMPLATE cost '\\n'",
"rules : rules '\\n'",
"rules : rules error '\\n'",
"nonterm : ID",
"tree : ID",
"tree : ID '(' tree ')'",
"tree : ID '(' tree ',' tree ')'",
"cost : CODE",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  500
#endif
#endif

#define YYINITSTACKSIZE 500

typedef struct {
    unsigned stacksize;
    short    *s_base;
    short    *s_mark;
    short    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 60 "code/tools/lcc/lburg/gram.y"
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

int errcnt = 0;
FILE *infp = NULL;
FILE *outfp = NULL;
static char buf[BUFSIZ], *bp = buf;
static int ppercent = 0;
static int code = 0;

static int get(void) {
	if (*bp == 0) {
		bp = buf;
		*bp = 0;
		if (fgets(buf, sizeof buf, infp) == NULL)
			return EOF;
		yylineno++;
		while (buf[0] == '%' && buf[1] == '{' && buf[2] == '\n') {
			for (;;) {
				if (fgets(buf, sizeof buf, infp) == NULL) {
					yywarn("unterminated %{...%}\n");
					return EOF;
				}
				yylineno++;
				if (strcmp(buf, "%}\n") == 0)
					break;
				fputs(buf, outfp);
			}
			if (fgets(buf, sizeof buf, infp) == NULL)
				return EOF;
			yylineno++;
		}
	}
	return *bp++;
}

void yyerror(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	if (yylineno > 0)
		fprintf(stderr, "line %d: ", yylineno);
	vfprintf(stderr, fmt, ap);
	if (fmt[strlen(fmt)-1] != '\n')
		 fprintf(stderr, "\n");
	errcnt++;
	va_end(ap);
}

int yylex(void) {
	int c;

	if (code) {
		char *p;
		bp += strspn(bp, " \t\f");
		p = strchr(bp, '\n');
		if (p == NULL)
			p = strchr(bp, '\n');
		while (p > bp && isspace(p[-1]))
			p--;
		yylval.string = alloc(p - bp + 1);
		strncpy(yylval.string, bp, p - bp);
		yylval.string[p - bp] = 0;
		bp = p;
		code--;
		return CODE;
	}
	while ((c = get()) != EOF) {
		switch (c) {
		case ' ': case '\f': case '\t':
			continue;
		case '\n':
		case '(': case ')': case ',':
		case ':': case '=':
			return c;
		}
		if (c == '%' && *bp == '%') {
			bp++;
			return ppercent++ ? 0 : PPERCENT;
		} else if (c == '%' && strncmp(bp, "term", 4) == 0
		&& isspace(bp[4])) {
			bp += 4;
			return TERMINAL;
		} else if (c == '%' && strncmp(bp, "start", 5) == 0
		&& isspace(bp[5])) {
			bp += 5;
			return START;
		} else if (c == '"') {
			char *p = strchr(bp, '"');
			if (p == NULL) {
				yyerror("missing \" in assembler template\n");
				p = strchr(bp, '\n');
				if (p == NULL)
					p = strchr(bp, '\0');
			}
			assert(p);
			yylval.string = alloc(p - bp + 1);
			strncpy(yylval.string, bp, p - bp);
			yylval.string[p - bp] = 0;
			bp = *p == '"' ? p + 1 : p;
			code++;
			return TEMPLATE;
		} else if (isdigit(c)) {
			int n = 0;
			do {
				int d = c - '0';
				if (n > (INT_MAX - d)/10)
					yyerror("integer greater than %d\n", INT_MAX);
				else
					n = 10*n + d;
				c = get();
			} while (c != EOF && isdigit(c));
			bp--;
			yylval.n = n;
			return INT;
		} else if (isalpha(c)) {
			char *p = bp - 1;
			while (isalpha(*bp) || isdigit(*bp) || *bp == '_')
				bp++;
			yylval.string = alloc(bp - p + 1);
			strncpy(yylval.string, p, bp - p);
			yylval.string[bp - p] = 0;
			return ID;
		} else if (isprint(c))
			yyerror("invalid character `%c'\n", c);
		else
			yyerror("invalid character `\\%03o'\n", (unsigned char)c);
	}
	return 0;
}

void yywarn(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	if (yylineno > 0)
		fprintf(stderr, "line %d: ", yylineno);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, ap);
}
#line 387 "y.tab.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (short *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack)) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 1:
#line 22 "code/tools/lcc/lburg/gram.y"
	{ yylineno = 0; }
break;
case 2:
#line 23 "code/tools/lcc/lburg/gram.y"
	{ yylineno = 0; }
break;
case 6:
#line 31 "code/tools/lcc/lburg/gram.y"
	{
		if (nonterm(yystack.l_mark[-1].string)->number != 1)
			yyerror("redeclaration of the start symbol\n");
		}
break;
case 8:
#line 36 "code/tools/lcc/lburg/gram.y"
	{ yyerrok; }
break;
case 10:
#line 40 "code/tools/lcc/lburg/gram.y"
	{ term(yystack.l_mark[-2].string, yystack.l_mark[0].n); }
break;
case 12:
#line 44 "code/tools/lcc/lburg/gram.y"
	{ rule(yystack.l_mark[-5].string, yystack.l_mark[-3].tree, yystack.l_mark[-2].string, yystack.l_mark[-1].string); }
break;
case 14:
#line 46 "code/tools/lcc/lburg/gram.y"
	{ yyerrok; }
break;
case 15:
#line 49 "code/tools/lcc/lburg/gram.y"
	{ nonterm(yyval.string = yystack.l_mark[0].string); }
break;
case 16:
#line 52 "code/tools/lcc/lburg/gram.y"
	{ yyval.tree = tree(yystack.l_mark[0].string,  0,  0); }
break;
case 17:
#line 53 "code/tools/lcc/lburg/gram.y"
	{ yyval.tree = tree(yystack.l_mark[-3].string, yystack.l_mark[-1].tree,  0); }
break;
case 18:
#line 54 "code/tools/lcc/lburg/gram.y"
	{ yyval.tree = tree(yystack.l_mark[-5].string, yystack.l_mark[-3].tree, yystack.l_mark[-1].tree); }
break;
case 19:
#line 57 "code/tools/lcc/lburg/gram.y"
	{ if (*yystack.l_mark[0].string == 0) yyval.string = "0"; }
break;
#line 644 "y.tab.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (short) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
