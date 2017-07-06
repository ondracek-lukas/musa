// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

#include "commandParser.h"

#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "util.h"
#include "messages.h"
// #include "safe.h"
// #include "script.h"

#define isDelim(c) (((c)<'0')||(((c)>'9')&&((c)<'A'))||(((c)>'Z')&&((c)<'a'))||((c)>'z'))

// -- trie manipulation --

#define IMPORTANCE_CLASSES 2
enum importance {
	builtinCmd=1,
	builtinAliasCmd=0
};

struct trie {
	char c;
	struct trie *sibling;
	struct trie *child;
	void *data;
	enum importance importance;
	bool leaf;  // no children allowed
};


static inline struct trie **trieGetChildPtr(struct trie *trie, char c) {
	struct trie **pTrie;
	pTrie=&trie->child;
	while (*pTrie && ((*pTrie)->c != c))
		pTrie=&(*pTrie)->sibling;
	return pTrie;
}

static inline struct trie *trieGetChild(struct trie *trie, char c) {
	trie=trie->child;
	while (trie && (trie->c != c))
		trie=trie->sibling;
	return trie;
}

static inline struct trie *trieGetChildCreate(struct trie *trie, char c) {
	struct trie **pTrie=&trie->child;
	while (*pTrie && ((*pTrie)->c != c))
		pTrie=&(*pTrie)->sibling;
	if (!*pTrie) {
		*pTrie=calloc(1, sizeof(struct trie));
		(*pTrie)->c=c;
		(*pTrie)->leaf = false;
		(*pTrie)->data = NULL;
	}
	return *pTrie;
}

/* destroy commented
static void trieDestroy(struct trie **pTrie, enum importance imp) {
	for (struct trie **pt=&(*pTrie)->child, *t2=*pt; *pt; (*pt!=t2) || (pt=&(*pt)->sibling), (t2=*pt))
		trieDestroy(pt, imp);

	if (!(*pTrie)->scriptExpr || ((*pTrie)->importance == imp)) {
		if ((*pTrie)->child) {
			(*pTrie)->scriptExpr=0;
		} else {
			struct trie *t=*pTrie;
			(*pTrie)=t->sibling;
			free(t->scriptExpr);
			free(t->paramsFlags);
			free(t);
		}
	}
}

static void trieDestroyBranch(struct trie **pTrie, char *prefix, enum importance imp) {
	if (*prefix) {
		if (!*pTrie)
			return;
		trieDestroyBranch(trieGetChildPtr(*pTrie, *prefix), prefix+1, imp);
		if (!(*pTrie)->child && !(*pTrie)->scriptExpr)
			trieDestroy(pTrie, imp);
	} else {
		trieDestroy(pTrie, imp);
	}
}
*/

static bool trieAdd(struct trie *trie, char *prefix, void *data, bool leaf, enum importance importance) {
	struct trie *trie2=trie;
	for (char *p=prefix; *p; p++) {
		trie=trieGetChildCreate(trie, *p);
		if (trie->leaf) {       // Ambiguous
			return false;
		}
	}

	if (trie->child && leaf){ // Ambiguous
		return false;
	}
	if (trie->data){          // Already exists
		return false;
	}

	trie=trie2;
	for (char *p=prefix; *p; p++) {
		trie=trieGetChild(trie, *p);
		if ((isDelim(*(p+1))) && (trie->importance<importance))
			trie->importance=importance;
	}
	trie->importance=importance;
	trie->data = data;
	trie->leaf = leaf;

	return true;
}

static bool parseParam(char **dst, char **src, bool allRemaining, bool escaped) {
	while (*src && (**src==' '))
		(*src)++;
	bool quoted= (**src=='"') && !allRemaining;
	(*src)+=quoted;
	for (; **src; (*src)++) {
		if (!allRemaining && (**src==(quoted?'"':' '))) {
			(*src)++;
			**dst='\0';
			return false; // not opened
		}
		if (escaped && ((**src=='\\') || (**src=='"')))
			*(*dst)++ = '\\';
		*(*dst)++ = **src;
	}
	**dst='\0';
	return true; // opened
}

static void *trieParse(struct trie *trie, char *str, char **strEnd) {

	for (; *str; str++) {
		struct trie *trie2=trie;
		trie=trieGetChild(trie, *str);
		if (!trie) {
			trie=trie2;
			break;
		}
	}

	if (*str && !trie->leaf) return NULL;
	if (strEnd) *strEnd = str;
	return trie->data;

/*
	if (!trie->data)
		return 0;

	if ((!trie->params) && *cmd)
		return 0;

	utilStrRealloc(&scriptExpr, 0, strlen(trie->scriptExpr) + strlen(cmd) + 2);
	char *str=scriptExpr;
	char *format=trie->scriptExpr;
	char *flags=trie->paramsFlags;
	int params=trie->params;
	bool varArgs=(params<0);

	for (; *format; format++) {
		if (*format=='%') {
			if (*format+1=='%') {
				*str++='%';
				format++;
			} else {
				if (params>0)
					params--;
				do {
					if (params<0)
						params++;
					if (*flags!='-')
						*str++ = '"';
					parseParam(&str, &cmd, !params, (*flags!='-'));
					if (*flags!='-')
						*str++ = '"';
					if (varArgs && *cmd)
						*str++ = ',';
					if(!*++flags)
						flags--;
				} while (varArgs && *cmd);
			}
		} else {
			*str++ = *format;
		}
	}
	*str++='\0';

	return scriptExpr;
	*/
}

static void trieComplete(struct trie *trie, struct utilStrList **pLists, char **prefix, char **prefixEnd) {
	utilStrRealloc(prefix, prefixEnd, 1);

	bool retCurrent=trie->data;
	if (!retCurrent)
		for (struct trie *t=trie->child; t; t=t->sibling)
			if (isDelim(t->c)) {
				retCurrent=true;
				break;
			}

	if (retCurrent) {
		**prefixEnd='\0';
		utilStrListAddAfter(&(pLists[trie->importance]));
		utilStrRealloc(&(pLists[trie->importance])->str, 0, strlen(*prefix)+1);
		strcpy((pLists[trie->importance])->str, *prefix);
	}

	trie=trie->child;
	(*prefixEnd)++;
	for (; trie; trie=trie->sibling) {
		if (isDelim(trie->c))
			continue;
		*(*prefixEnd-1)=trie->c;
		trieComplete(trie, pLists, prefix, prefixEnd);
	}
	(*prefixEnd)--;
}

static struct utilStrList *trieGetSuffixes(struct trie *trie) {
	static char *completions=0;
	static char *completionsEnd=0;
	struct utilStrList *lists[IMPORTANCE_CLASSES];
	for (int i=0; i<IMPORTANCE_CLASSES; i++) lists[i]=NULL;
	trieComplete(trie, lists, &completions, &completionsEnd);

	struct utilStrList *list=0;
	for (int imp=IMPORTANCE_CLASSES-1; imp>=0; imp--) {
		if (lists[imp])
			while (lists[imp]->prev)
				lists[imp]=lists[imp]->prev;
		utilStrListMoveAfter(&list, lists[imp]);
	}

	if (list)
		while (list->prev)
			list=list->prev;
	return list;
}


// -- commands translation and completion --

static struct trie commands;   // using struct msgAlias as data


static __attribute__((constructor)) void init() {
	commands.leaf = false;
	commands.data = false;
	struct msgAlias *alias = msgAliases;
	while (alias->name) {
		if (!trieAdd(
				&commands,
				alias->name,
				alias,
				*alias->args,
				(alias->preferred ? builtinCmd : builtinAliasCmd))) {
			utilExitErr("Conflicting alias '%s'", alias->name);
		}
		alias++;
	}
}
/* commented rmBranch
void consoleTranslRmBranch(char *prefix) {
	if (*prefix) {
		struct trie **pTrie=trieGetChildPtr(&commands, *prefix);
		if (!*pTrie)
			return;
		trieDestroyBranch(pTrie, prefix+1, builtinCmd);
		trieDestroyBranch(pTrie, prefix+1, builtinAliasCmd);
	} else {
		for (struct trie **pt=&commands.child, *t2=*pt; *pt; (*pt!=t2) || (pt=&(*pt)->sibling), (t2=*pt)) {
			trieDestroy(pt, builtinCmd);
			trieDestroy(pt, builtinAliasCmd);
		}
	}
}
*/

bool cpExecute(char *cmd) {
	struct msgAlias *msg = trieParse(&commands, cmd, &cmd);
	if (!msg) return false;
	int cnt=0;
	for (enum msgDataType *type = msg->args; *type; type++) cnt++;
	void **args = utilCalloc(cnt,sizeof(void*));
	void **arg = args;
	bool err=false;
	for (enum msgDataType *type = msg->args; !err && *type; type++, arg++) {
		while (*cmd==' ') cmd++;
		if (!*cmd) {
			err=true;
			break;
		}
		int len=0;
		switch (*type) {
			case MSG_INT:
			case MSG_BOOL:
				*arg = utilMalloc(sizeof(int));
				if (sscanf(cmd, "%d%n", *arg, &len) != 1) err=true;
				cmd += len;
				break;
			case MSG_FLOAT:
				*arg = utilMalloc(sizeof(float));
				if (sscanf(cmd, "%f%n", *arg, &len) != 1) err=true;
				cmd += len;
				break;
			case MSG_DOUBLE:
				*arg = utilMalloc(sizeof(double));
				if (sscanf(cmd, "%lf%n", *arg, &len) != 1) err=true;
				cmd += len;
				break;
			case MSG_PATH:
			case MSG_STRING: {
				char delim;
				bool quoted=false;
				switch (*cmd) {
					case '"':
					case '\'':
						delim = *cmd;
						cmd++;
						quoted=true;
						break;
					default:
						delim = (*(type+1) ? ' ' : '\0');
						break;
				}
				for (char *c=cmd; *c && (*c != delim); c++) len++;
				*arg = utilMalloc(sizeof(char*));
				*(void**)*arg = utilMalloc(sizeof(char)*(len+1));
				memcpy(*(char**)*arg, cmd, len); (*(char**)*arg)[len] = '\0';
				cmd += len;
				if (quoted && (*(cmd++) != delim)) err=true;
				break; }
			default:
				utilExitErr("Unknown data type in parser.");
		}
	}
	if (!err && !*cmd) {
		msgSend(msg->msg, args);
	}
	int i=0;
	for (enum msgDataType *type = msg->args; *type; type++, i++) {
		switch (*type) {
			case MSG_STRING:
				free(*(void**)args[i]);
			default:
				free(args[i]);
		}
	}
	free(args);
	return !err && !*cmd;
}

struct utilStrList *cpComplete(char *prefix) {
	struct trie *trie=&commands;
	struct trie *trie2=trie;
	while (trie && *prefix) {
		trie2=trie;
		trie=trieGetChild(trie, *prefix);
		if (trie)
			prefix++;
	}
	if (trie && !trie->leaf) {
		return trieGetSuffixes(trie);
	} else {
		if (!trie)
			trie=trie2;
		struct msgAlias *msg = trie->data;
		if (!msg)
			return NULL;
		char *argBegin = prefix;
		enum msgDataType *type;
		char delim = '\0';
		bool quoted=false;
		for (type = msg->args; *type; type++) {
			while (*prefix==' ') prefix++;
			switch (*type) {
				case MSG_INT:
				case MSG_BOOL:
				case MSG_FLOAT:
				case MSG_DOUBLE:
					delim = ' ';
					quoted = false;
					break;
				case MSG_PATH:
				case MSG_STRING:
					switch (*prefix) {
						case '"':
						case '\'':
							delim = *prefix;
							prefix++;
							quoted = true;
							break;
						default:
							delim = (*(type+1) ? ' ' : '\0');
							quoted = false;
							break;
					}
					break;
				default:
					utilExitErr("Unknown data type in parser (complete).");
			}
			argBegin = prefix;
			while (*prefix && (*prefix!=delim)) prefix++;
			if (delim && (*prefix == delim)) *prefix++;
			else if (!*prefix) break;
		}
		if (*prefix) return NULL;
		struct utilStrList *ret=NULL;
		switch (*type) {
			case MSG_PATH:
				ret = cpPathComplete(argBegin);
			case MSG_STRING:
				if (quoted) {
					if (ret) {
						for (struct utilStrList *r = ret; r; r = r->next) {
							int len = strlen(r->str);
							utilStrRealloc(&r->str, NULL, len+2);
							r->str[len++] = delim;
							r->str[len]   = '\0';
						}
					} else {
						utilStrListAddAfter(&ret);
						utilStrRealloc(&ret->str, NULL, 2);
						ret->str[0] = delim;
						ret->str[1] = '\0';
					}
				}
				break;
//				case 'c':
//					return consoleTranslColorComplete(param, false);
//				case 'C':
//					return consoleTranslColorComplete(param, true);
		}
		return ret;
	}
	return NULL;
}

/* to be removed
void consoleTranslUserAdd2(char *prefix, char *scriptExpr) {
	consoleTranslUserAdd(prefix, scriptExpr, 0, NULL);
}
void consoleTranslUserAdd3(char *prefix, char *scriptExpr, int params) {
	consoleTranslUserAdd(prefix, scriptExpr, params, NULL);
}
void consoleTranslUserAdd(char *prefix, char *scriptExpr, int params, char *paramsFlags) {
	if (!trieAdd(
		&commands,
		prefix,
		scriptExpr,
		params,
		paramsFlags,
		userCmd
	)) scriptThrowException("The command would be ambiguous");
}
void consoleTranslUserRmBranch(char *prefix) {
	if (*prefix) {
		struct trie **pTrie=trieGetChildPtr(&commands, *prefix);
		if (!*pTrie)
			return;
		trieDestroyBranch(pTrie, prefix+1, userCmd);
	} else {
		for (struct trie **pt=&commands.child, *t2=*pt; *pt; (*pt!=t2) || (pt=&(*pt)->sibling), (t2=*pt))
			trieDestroy(pt, userCmd);
	}
}
void consoleTranslUserRmAll() {
	consoleTranslUserRmBranch("");
}
*/

// -- path completion --

struct utilStrList *cpPathComplete(char *prefix) {
	char *path=utilExpandPath(prefix);
	static char *tmp=0;
	utilStrRealloc(&tmp, 0, strlen(path)+1);
	prefix=tmp;
	if (prefix!=path) {
		strcpy(prefix, path);
		path=prefix;
	}
	prefix=utilFileNameFromPath(path);
	char c='\0';
	if (prefix==path) {
		path=".";
	} else {
		c=*prefix;
		*prefix='\0';
	}
	DIR *dir=opendir(path);
	if (!dir)
		return NULL;
	if (c) *prefix=c;

	int prefixLen=strlen(prefix);


	struct dirent *file;
	struct utilStrList *list=0;
	while ((file=readdir(dir))) {
		if ((strncmp(prefix, file->d_name, prefixLen)==0) && (*prefix || (*file->d_name!='.'))) {
			utilStrListAddAfter(&list);
			utilStrRealloc(&list->str, 0, strlen(file->d_name)-prefixLen+1);
			strcpy(list->str, file->d_name+prefixLen);
		}
	}
	closedir(dir);

	if (list)
		while (list->prev)
			list=list->prev;

	return list;
}


/* colors commented
// -- color translation and completion --

struct trie colors;
struct trie colorsWithAlpha;

struct utilStrList *consoleTranslColorComplete(char *prefix, bool withAlphaChannel) {
	struct trie *trie=(withAlphaChannel? &colorsWithAlpha:&colors);
	struct utilStrList *list=0;
	int codeLen=0;
	while (*prefix) {
		if (((codeLen==0) &&   (*prefix=='#')) ||
		    ((codeLen> 0) && (((*prefix>='0') && (*prefix<='9')) ||
		                      ((*prefix>='a') && (*prefix<='f')) ||
		                      ((*prefix>='A') && (*prefix<='F'))))) {
			codeLen++;
		} else {
			codeLen=-1;
		}
		if (trie)
			trie=trieGetChild(trie, *prefix);
		else if (codeLen<0)
			break;
		prefix++;
	}
	if (codeLen>=0) {
		if (withAlphaChannel && (codeLen<=9)) {
			utilStrListAddAfter(&list);
			utilStrRealloc(&list->str, 0, 10);
			strcpy(list->str, "#AARRGGBB"+codeLen);
		} else if (codeLen<=7) {
			utilStrListAddAfter(&list);
			utilStrRealloc(&list->str, 0, 8);
			list->str[0]='\0';
			strcpy(list->str, "#RRGGBB"+codeLen);
		}
	}
	if (trie)
		utilStrListMoveAfter(&list, trieGetSuffixes(trie));
	while (list && list->prev)
		list=list->prev;
	return list;
}

#define normalizeColorChar(dst, src) \
	if (((src>='0') && (src<='9')) || \
	    ((src>='A') && (src<='F'))) \
		dst=src; \
	else if ((src>='a') && (src<='f')) \
		dst=src-'a'+'A'; \
	else { \
		ret=0; \
		break; \
	}

char *consoleTranslColorNormalize(char *color) {
	char *ret=0;
	static char lastColor[8];
	if (*color=='#') {
		lastColor[0]='#';
		ret=lastColor;
		for (int i=1; i<7; i++)
			normalizeColorChar(lastColor[i], color[i]);
		lastColor[7]='\0';
		if (color[7])
			ret=0;
	} else {
		ret=trieTranslate(&colors, color);
		if (ret) {
			strcpy(lastColor, ret);
			ret=lastColor;
		}
	}
	if (!ret)
		scriptThrowException("Wrong color");
	return ret;
}

char *consoleTranslColorANormalize(char *color) {
	char *ret=0;
	static char lastColor[10];
	if (*color=='#') {
		lastColor[0]='#';
		if (strlen(color)==7) {
			ret=lastColor;
			lastColor[1]='F';
			lastColor[2]='F';
			for (int i=1; i<7; i++)
				normalizeColorChar(lastColor[i+2], color[i]);
		} else if (strlen(color)==9) {
			ret=lastColor;
			for (int i=1; i<9; i++)
				normalizeColorChar(lastColor[i], color[i]);
		}
		lastColor[10]='\0';
	} else {
		ret=trieTranslate(&colorsWithAlpha, color);
		if (ret) {
			strcpy(lastColor, ret);
			ret=lastColor;
		}
	}
	if (!ret)
		scriptThrowException("Wrong color");
	return ret;
}

#undef normalizeColorChar

void consoleTranslColorAdd(char *alias, char *color) {
	char *normalizedAlpha=consoleTranslColorANormalize(color);
	if (!normalizedAlpha)
		return;
	char *normalized=     consoleTranslColorNormalize(color);
	scriptCatchException();

	if (trieAdd(&colorsWithAlpha, alias, normalizedAlpha, 0, 0, userCmd)) {
		if (normalized) {
			if (trieAdd(&colors, alias, normalized, 0, 0, userCmd))
				return;
		} else {
			return;
		}
	}

	scriptThrowException("Color alias already exists");
}

void consoleTranslColorRemoveAll() {
	for (struct trie **pt=&colors.child, *t2=*pt; *pt; (*pt!=t2) || (pt=&(*pt)->sibling), (t2=*pt))
		trieDestroy(pt, userCmd);
	for (struct trie **pt=&colorsWithAlpha.child, *t2=*pt; *pt; (*pt!=t2) || (pt=&(*pt)->sibling), (t2=*pt))
		trieDestroy(pt, userCmd);
}
*/
