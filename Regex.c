#include "header.h"

INLINE(Regex * emptyRegex(unsigned int size)) {
    Regex * new = malloc(sizeof(Regex));
    if (new == NULL) {
        printf(MSG_MEMERR);
        new = malloc(sizeof(Regex));
    }
    size = size <= REGEX ? REGEX : size;
    new->size = size;
    new->strlen = 0;
    new->concrete = false;
    new->brkBrk4Alt = false;
    new->altDecomposable = 0;
    new->bracketed = true;
    new->regex = malloc(size);
    if (new->regex == NULL) {
        printf(MSG_MEMERR);
        new->regex = malloc(size);
    }
    new->regex[0] = '\0';
    return new;
}

INLINE(void freeRegex(Regex *RESTRICT r)) {
    if (r->regex) free(r->regex);
    free(r);
}

Regex * regexCpy(Regex *RESTRICT src) {
    if (src == NULL) return NULL;
    assert((src->size > src->strlen && src->strlen > 0) || src->strlen==0);
    Regex * ret = malloc(sizeof(Regex));
    memcpy(ret, src, sizeof(Regex));
    if (ret->size>0) {
        ret->regex = malloc(ret->size);
        memcpy(ret->regex, src->regex, ret->strlen);
        ret->regex[ret->strlen] = '\0';
    }
    else ret->regex = NULL;
    return ret;
}

void regexCompile(Regex * r, unsigned short fault) {
    if (fault>25) {
        sprintf(r->regex, "%hu", fault);
        r->strlen = 1+(unsigned int)ceilf(log10f((float)(fault+1)));;
    }
    else {r->regex[0] = 96 + fault;
        r->regex[1] = 0;
        r->strlen = 1;
    }
}

INLINE(bool endsWith(Regex * s1, Regex *s2)) {
    return s1->strlen > s2->strlen ?
          strcmp(s1->regex+s1->strlen-s2->strlen, s2->regex)==0 : false;
}

INLINE(bool beginsWith(Regex * s1, Regex *s2)) {
    return s1->strlen > s2->strlen ?
          memcmp(s1->regex, s2->regex, s2->strlen)==0 : false;
}

bool altAredyPresent(Regex * haystack, Regex * needle) { // checks if s1==a|b|..s2|d|..g
    if (needle->strlen>haystack->strlen || memchr(needle->regex, '|', needle->strlen)) return false;
    char * ref = needle->bracketed ? needle->regex :  malloc(needle->strlen+3);
    if (!needle->bracketed) sprintf(ref, "(%s)", needle->regex);
    
    char * pt = strstr(haystack->regex, ref); // 1st occurrence of needle
    while (pt != NULL) {
        if ((needle->bracketed && (pt[needle->strlen] == '|' || pt[needle->strlen] == '\0'))
        || (!needle->bracketed && (pt[needle->strlen+2] == '|' || pt[needle->strlen+2] == '\0'))) {
            if (!needle->bracketed) free(ref);
            debugif(DEBUG_REGEX, printlog("REGEX: Cutting alternative\n");)
            return true;
        }
        pt = strstr(pt+needle->strlen, ref);
    }

    if (!needle->bracketed) free(ref);
    return false;
}

doC11(mtx_t mutex;)
bool hasInit=false;

void regexMake(Regex* s1, Regex* s2, Regex* d, char op, Regex *s3) {
    doC11(
        if (!hasInit) {
            hasInit=true;
            mtx_init(&mutex, mtx_plain);
        }
    )
    static char *RESTRICT regexBuf = NULL;
    static size_t regexBufLen = 0;
    if (regexBuf==NULL) regexBuf = calloc(regexBufLen=REGEX, 1);

    unsigned int strl1 = s1->strlen, strl2 = s2->strlen, strl3 = 0; //int strl1 = strlen(s1->regex), strl2 = strlen(s2->regex), strl3 = 0;
    if (s3 != NULL) strl3 = s3->strlen; //strl3 = strlen(s3->regex);
    bool streq12 = strl1==strl2 ? strcmp(s1->regex, s2->regex)==0 : false;
    char * solution = NULL;
    unsigned int solLen = 0;

    doC11(mtx_lock(&mutex);)
    if (strl1+strl2+strl3+6 > regexBufLen) {
        do regexBufLen *= (REGEXLEVER > 1.5 ? REGEXLEVER : 1.5);
        while (strl1+strl2+strl3+6 > regexBufLen);
        debugif(DEBUG_REGEX, printlog("REALLOC regexBuf w %lu\n", regexBufLen))
        regexBuf = realloc(regexBuf, regexBufLen);
    }
    
    debugif(DEBUG_REGEX, {
        assert(strl1 < s1->size && strl2 < s2->size && (s3 == NULL || strl3 < s3->size));
        assert(strl1 == strlen(s1->regex) && strl2 == strlen(s2->regex));
        assert(s3 == NULL || strl3 == strlen(s3->regex));
    })
    
    if (op == 'c') {                                                        // Concatenation
        if (strl1>0 && strl2>0) { // concat
            solLen = strl1+strl2;
            solution = regexBuf;
            d->bracketed = false;
            d->brkBrk4Alt = false;
            d->concrete = s1->concrete || s2->concrete;
            unsigned int minStrl = strl1<strl2? strl1:strl2, maxStrl = strl1>strl2? strl1:strl2;
            Regex *longer = strl1>strl2? s1:s2, *shorter = strl1<strl2? s1:s2;
            if (minStrl+1==maxStrl && (longer->regex[minStrl]=='*' || longer->regex[minStrl]=='+') && memcmp(s1->regex, s2->regex, minStrl)==0) { // aa* -> a+
                if (d->regex == s2->regex || d->regex == s1->regex) {
                    if (d->size <= maxStrl) {
                        realloc(d->regex, maxStrl*REGEXLEVER);
                        d->size = maxStrl*REGEXLEVER;
                    }
                    d->strlen = maxStrl;
                    d->bracketed = true;
                    d->regex[minStrl] = '+';
                    doC11(mtx_unlock(&mutex);)
                    return;
                }
                else {
                    d->bracketed = true;
                    solLen = maxStrl;
                    sprintf(regexBuf, "%s+", shorter->regex);
                }
            }
            else if (streq12 && (s1->regex[strl1-1]=='*' || s1->regex[strl1-1]=='+')) { // a*a* -> a*   a+a+ -> a+
                solLen = strl1;
                solution = s1->regex;
                d->bracketed = s1->bracketed;
            }
            else if (d->regex == s1->regex && s1->regex != s2->regex && s1->size > solLen) {
                s1->strlen = solLen;
                s1->regex[solLen] = '\0';
                if (s1->altDecomposable || s2->altDecomposable) d->altDecomposable=2;
                doC11(mtx_unlock(&mutex);)  // Copy does not need mutex protection
                memcpy(s1->regex+strl1, s2->regex, strl2);
                doC99(assert(strlen(s1->regex) == solLen));
                return;
            }
            else {
                if (s1->altDecomposable || s2->altDecomposable) d->altDecomposable=2;
                sprintf(regexBuf, "%s%s", s1->regex, s2->regex);
            }
        }
        else if (strl1>0 && strl2==0) { // copy s1 in d
            solution = s1->regex;
            solLen = strl1;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
            d->altDecomposable = s1->altDecomposable;
        }
        else if (strl1==0 && strl2>0) {// copy s2 in d
            solution = s2->regex;
            solLen = strl2;
            d->bracketed = s2->bracketed;
            d->concrete = s2->concrete;
            d->altDecomposable = s2->altDecomposable;
        }
        else if (strl2==0 && strl2==0) { // empty
            doC11(mtx_unlock(&mutex);)
            if (d->regex == NULL) {
                d->regex = malloc(REGEX);
                d->size = REGEX;
            }
            d->regex[0] = '\0';
            d->strlen = 0;
            d->altDecomposable = 0;
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'a') {                                                   // Alternative
        if (strl1 > 0 && streq12) { // Copia s1 in d
            solution = s1->regex;
            solLen = strl1;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
            d->altDecomposable = s1->altDecomposable;
        }
        else if (strl1 > 0 && strl2 > 0) { // s1|s2
            solution = regexBuf;
            d->concrete = s1->concrete && s2->concrete;
            if (s1->altDecomposable<2 && s2->altDecomposable<2) {
                if (d->altDecomposable==0) d->altDecomposable=1;
                if (endsWith(s1, s2) || beginsWith(s1, s2) || altAredyPresent(s1, s2)) {
                    solution = s1->regex;
                    solLen = strl1;
                    goto END_REGEX_PROCESS;
                }
                if (endsWith(s2, s1) || beginsWith(s2, s1) || altAredyPresent(s2, s1)) {
                    solution = s2->regex;
                    solLen = strl2;
                    goto END_REGEX_PROCESS;
                }
            }
            if (strl1==strl2+1 && (s1->regex[strl2]=='?' || s1->regex[strl2]=='+' || s1->regex[strl2]=='*') && strncmp(s1->regex, s2->regex, strl2)==0) { // a?|a -> a?   a*|a -> a*   a+|a -> a+
                solution = s1->regex;
                solLen = strl1;
                d->bracketed = true;
                d->altDecomposable= s1->altDecomposable;
            }
            else if (strl2==strl1+1 && (s2->regex[strl1]=='?' || s2->regex[strl1]=='+' || s2->regex[strl1]=='*') && strncmp(s1->regex, s2->regex, strl1)==0) { // a|a? -> a?   a|a* -> a*   a|a+ -> a+
                solution = s2->regex;
                solLen = strl2;
                d->bracketed = true;
                d->altDecomposable= s2->altDecomposable;
            }
            else if ((s1->bracketed || s1->brkBrk4Alt) && (s2->bracketed || s2->brkBrk4Alt)) { // Both s1 and s2 don't need brackets -> s1|s2
                d->bracketed = false;
                d->brkBrk4Alt = true;
                if (d->altDecomposable==0) d->altDecomposable=1;
                solLen = strl1+strl2+1;
                if (d->regex==s1->regex && s1->size > solLen) {
                    solution = s1->regex;
                    solution[strl1] = '|';
                    memcpy(solution+strl1+1, s2->regex, strl2);
                    solution[solLen] = '\0';
                    assert(strlen(solution) == solLen);
                    d->strlen = solLen;
                }
                else if (d->regex==s2->regex && s2->size > solLen) {
                    solution = s2->regex;
                    solution[strl2] = '|';
                    memcpy(solution+strl2+1, s1->regex, strl1);
                    solution[solLen] = '\0';
                    assert(strlen(solution) == solLen);
                    d->strlen = solLen;
                }
                else sprintf(regexBuf, "%s|%s", s1->regex, s2->regex);
            }
            else if ((s1->bracketed || s1->brkBrk4Alt) && !s2->bracketed) { // s2 needs brackets -> s1|(s2)
                d->bracketed = false;
                d->brkBrk4Alt = true;
                if (d->altDecomposable==0) d->altDecomposable=1;
                solLen = strl1+strl2+3;
                if (d->regex==s1->regex && s1->size > solLen) {
                    solution = s1->regex;
                    sprintf(solution+strl1, "|(%s)", s2->regex);
                    assert(strlen(solution) == solLen);
                    s1->strlen = solLen;
                }
                else sprintf(regexBuf, "%s|(%s)", s1->regex, s2->regex);
            }
            else if (!s1->bracketed && (s2->bracketed || s2->brkBrk4Alt)) { // s1 needs brackets -> s2|(s1)
                d->bracketed = false;
                d->brkBrk4Alt = true;
                if (d->altDecomposable==0) d->altDecomposable=1;
                solLen = strl1+strl2+3;
                if (d->regex==s2->regex && s2->size > solLen) {
                    solution = s2->regex;
                    sprintf(solution+strl2, "|(%s)", s1->regex);
                    assert(strlen(solution) == solLen);
                    s2->strlen = solLen;
                }
                else sprintf(regexBuf, "(%s)|%s", s1->regex, s2->regex);
            } else {                                                        // Both need brackets -> (s1)|(s2)
                if (d->altDecomposable==0) d->altDecomposable=1;
                sprintf(regexBuf, "(%s)|(%s)", s1->regex, s2->regex);
                solLen = strl1+strl2+5;
                d->bracketed = false;
                d->brkBrk4Alt = true;
            }
        }
        else if (strl1 > 0 && strl2==0) { // s1|ε
            if (s1->altDecomposable) d->altDecomposable=2;
            solution = regexBuf;
            if (s1->concrete) {
                if (s1->bracketed) {
                    solLen = strl1+1;
                    if (d->regex==s1->regex && d->size>solLen) {
                        solution = s1->regex;
                        solution[strl1] = '?';
                        solution[solLen] = '\0';
                    }
                    else sprintf(regexBuf, "%s?", s1->regex);
                }
                else {
                    solLen = strl1+3;
                    sprintf(regexBuf, "(%s)?", s1->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solLen = strl1;
                if (d->regex == s1->regex) solution = s1->regex;
                else strcpy(regexBuf, s1->regex);
                d->bracketed = s1->bracketed;
            }
            d->concrete = false;
        } else if (strl2 > 0) { // ε|s2
            if (s2->altDecomposable) d->altDecomposable=2;
            solution = regexBuf;
            if (s2->concrete) {
                if (s2->bracketed) {
                    solLen = strl2+1;
                    if (d->regex==s2->regex && d->size > solLen) {
                        solution = s2->regex;
                        solution[strl2] = '?';
                        solution[solLen] = '\0';
                        assert(strlen(solution) == solLen);
                    }
                    else sprintf(regexBuf, "%s?", s2->regex);
                }
                else {
                    solLen = strl2+3;
                    sprintf(regexBuf, "(%s)?", s2->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solLen = strl2;
                if (d->regex==s2->regex) solution = s2->regex;
                else strcpy(regexBuf, s2->regex);
                d->bracketed = s2->bracketed;
            }
            d->concrete = false;
        }
        else { // ε|ε
            doC11(mtx_unlock(&mutex);)
            if (d->regex == NULL) {
                d->regex = calloc(REGEX, 1);
                d->size = REGEX;
            }
            d->regex[0] = '\0';
            d->strlen = 0;
            d->altDecomposable = 0;
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'r') {                                                   // Recursion
        if (strl3 == 0) {
            doC11(mtx_unlock(&mutex);)
            regexMake(s1, s2, d, 'c', NULL);
        }
        else {
            assert(d->regex!=s1->regex && d->regex!= s2->regex && d->regex!=s3->regex);
            d->size = strl1+strl2+strl3+4 < REGEX ? REGEX : strl1+strl2+strl3+4;
            d->regex = realloc(d->regex, d->size);
            d->regex[0] = '\0';
            d->brkBrk4Alt = false;
            if (s1->altDecomposable || s2->altDecomposable) d->altDecomposable=2;
            bool streq13 = strl1==strl3 ? strcmp(s1->regex, s3->regex)==0 : false;
            bool streq23 = strl2==strl3 ? strcmp(s2->regex, s3->regex)==0 : false;
            unsigned int minStrl13 = strl1<strl3? strl1:strl3, maxStrl13 = strl1>strl3? strl1:strl3;
            unsigned int minStrl23 = strl2<strl3? strl2:strl3, maxStrl23 = strl2>strl3? strl2:strl3;
            Regex *longer13 = strl1>strl3? s1:s3, *shorter13 = strl1<strl3? s1:s3;
            Regex *longer23 = strl2>strl3? s2:s3, *shorter23 = strl2<strl3? s2:s3;
            if (strl1 + strl2 >0) {
                if (streq13 && streq23) { // aa*a -> a+
                    if (s1->bracketed) {
                        sprintf(d->regex, "%s+", s1->regex);
                        d->strlen = strl1+1;
                    } else {
                        sprintf(d->regex, "(%s)+", s1->regex);
                        d->strlen = strl1+3;
                    }
                    d->bracketed = true;
                    d->concrete = s1->concrete;
                }
                else if (streq13 && s1->bracketed){ // aa*b -> a+b
                    sprintf(d->regex, "%s+%s", s1->regex, s2->regex);
                    d->strlen=strl1+strl2+1;
                    if (strl2>0) d->bracketed = false; else d->bracketed = true;
                    d->concrete = (s1->concrete) | (s2->concrete);
                }
                else if (minStrl13+1==maxStrl13 && longer13->regex[minStrl13]=='*' && memcmp(s1->regex, s3->regex, minStrl13)==0) { // aa**b -> a+b    a*a*b -> a+b
                    sprintf(d->regex, "%s+%s", shorter13->regex, s2->regex);
                    d->strlen = maxStrl13+strl2;
                    d->concrete = (shorter13->concrete) | (s2->concrete);
                    if (strl2>0) d->bracketed = false; else d->bracketed = true;
                }
                else if (minStrl23+1==maxStrl23 && longer23->regex[minStrl23]=='*' && memcmp(s2->regex, s3->regex, minStrl23)==0) { // ab**b -> ab+    ab*b* -> ab+
                    sprintf(d->regex, "%s%s+", s1->regex, shorter23->regex);
                    d->strlen = maxStrl23+strl1;
                    d->concrete = (shorter23->concrete) | (s1->concrete);
                    if (strl1>0) d->bracketed = false; else d->bracketed = true;
                }
                else if (s3->bracketed) {
                    if (s3->regex[strl3-1]=='?') {
                        d->bracketed = true;
                        sprintf(d->regex, "%s%s", s1->regex, s3->regex);
                        d->regex[strl1+strl3-1]='*';
                        strcpy(d->regex+strl1+strl3, s2->regex);
                        d->strlen = strl1+strl2+strl3;
                        d->concrete = false;
                    }
                    else {
                        sprintf(d->regex, "%s%s*%s", s1->regex, s3->regex, s2->regex);
                        d->strlen = strl1+strl2+strl3+1;
                        d->concrete = (s1->concrete) | (s2->concrete);
                        d->bracketed = false;
                    }
                    assert(strlen(d->regex) == d->strlen);
                } else {
                    sprintf(d->regex, "%s(%s)*%s", s1->regex, s3->regex, s2->regex);
                    d->strlen = strl1+strl2+strl3+3;
                    d->bracketed = false;
                    d->concrete = (s1->concrete) | (s2->concrete);
                    assert(strlen(d->regex) == d->strlen);
                }
            }
            else {
                if (s3->bracketed) {
                    if (s3->regex[strl3-1]=='?' || s3->regex[strl3-1]=='*' || s3->regex[strl3-1]=='+') {
                        memcpy(d->regex, s3->regex, strl3);
                        if (s3->regex[strl3-1]=='?') d->regex[strl3-1] = '*';
                        d->regex[strl3] = '\0';
                        d->strlen = strl3;
                        assert(strlen(d->regex) == d->strlen);
                    }
                    else {
                        sprintf(d->regex, "%s*", s3->regex);
                        d->strlen = strl3+1;
                        assert(strlen(d->regex) == d->strlen);
                    }
                }
                else {
                    sprintf(d->regex, "(%s)*", s3->regex);
                    d->strlen = strl3+3;
                    assert(strlen(d->regex) == d->strlen);
                }
                d->bracketed = true; // Se anche termina con *, non importa
                d->concrete = false;
            }
            doC11(mtx_unlock(&mutex);)
        }
        return;
    }
    END_REGEX_PROCESS:
    d->strlen = solLen;
    if (solution != d->regex) {
        if (d->size <= solLen) {
            if (d->size == 0) {
                d->regex = malloc(solLen*REGEXLEVER);
                d->regex[0] = '\0';
            }
            else d->regex = realloc(d->regex, solLen*REGEXLEVER);
            d->size = solLen*REGEXLEVER;
        }
        memcpy(d->regex, solution, solLen);
        d->regex[solLen] = '\0';
        debugif(DEBUG_REGEX, if (strlen(d->regex) != d->strlen) {printlog("Different strlen %ld vs %u - '%s' '", strlen(d->regex), d->strlen, d->regex); for(unsigned int i=0; i<d->strlen; i++)printf("%c",d->regex[i]);printf("'\n"); fflush(stdout);exit(0);});
    }
    doC11(mtx_unlock(&mutex);)
}