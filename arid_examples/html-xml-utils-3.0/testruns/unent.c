/* C code produced by gperf version 2.7 */
/* Command-line: gperf -a -c -C -o -t -p -k 1,2,$ -D -N lookup_entity unent.hash  */						/* -*-indented-text-*- */

/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 2 Dec 1998
 * Version: $Id: unent.hash,v 1.8 2004/04/28 08:57:55 bbos Exp $
 *
 * Input file for gperf, to generate a perfect hash function
 * of all HTML named character entities. This list translates
 * names to Unicode numbers.
 */
#include <config.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include <stdio.h>
#include <ctype.h>

struct _Entity {char *name; unsigned int code;}
;

#define TOTAL_KEYWORDS 252
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 9
#define MAX_HASH_VALUE 729
/* maximum key range = 721, duplicates = 4 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned short asso_values[] =
    {
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730,  20,
       25,  15,   0, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 244,   5,   5,   0, 190,
      730,   5,   5, 129, 730,  10,   5,   5,   0,  97,
        0, 730,   0,  45,   5,  95, 730, 730,   5,  60,
        0, 730, 730, 730, 730, 730, 730,   5, 189,  35,
       30,   0, 240, 225,  85,  30,   0,  80,  15, 248,
      230,  40, 220,  30, 145, 245, 251, 100,   0,  20,
       10, 215,  15, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730, 730, 730, 730, 730,
      730, 730, 730, 730, 730, 730
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]] + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
const struct _Entity *
lookup_entity (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct _Entity wordlist[] =
    {
      {"Zeta", 918},
      {"Delta", 916},
      {"eacute", 233},
      {"Beta", 914},
      {"THORN", 222},
      {"aacute", 225},
      {"le", 8804},
      {"Gamma", 915},
      {"Lambda", 923},
      {"zeta", 950},
      {"Kappa", 922},
      {"alpha", 945},
      {"lambda", 955},
      {"equiv", 8801},
      {"zwj", 8205},
      {"zwnj", 8204},
      {"delta", 948},
      {"iacute", 237},
      {"acute", 180},
      {"iexcl", 161},
      {"oacute", 243},
      {"cedil", 184},
      {"oline", 8254},
      {"Ccedil", 199},
      {"Pi", 928},
      {"laquo", 171},
      {"divide", 247},
      {"Xi", 926},
      {"lceil", 8968},
      {"Yacute", 221},
      {"xi", 958},
      {"loz", 9674},
      {"ecirc", 234},
      {"iota", 953},
      {"acirc", 226},
      {"Sigma", 931},
      {"ldquo", 8220},
      {"ccedil", 231},
      {"kappa", 954},
      {"Theta", 920},
      {"circ", 710},
      {"icirc", 238},
      {"Uacute", 218},
      {"Oacute", 211},
      {"uacute", 250},
      {"Tau", 932},
      {"ocirc", 244},
      {"Phi", 934},
      {"euml", 235},
      {"Chi", 935},
      {"auml", 228},
      {"Rho", 929},
      {"Iacute", 205},
      {"euro", 8364},
      {"iuml", 239},
      {"Prime", 8243},
      {"chi", 967},
      {"Dagger", 8225},
      {"ouml", 246},
      {"real", 8476},
      {"larr", 8592},
      {"Ucirc", 219},
      {"Ocirc", 212},
      {"ucirc", 251},
      {"Iota", 921},
      {"Yuml", 376},
      {"darr", 8595},
      {"dagger", 8224},
      {"radic", 8730},
      {"raquo", 187},
      {"beta", 946},
      {"rceil", 8969},
      {"Eacute", 201},
      {"Nu", 925},
      {"ETH", 208},
      {"Icirc", 206},
      {"Mu", 924},
      {"Uuml", 220},
      {"Ouml", 214},
      {"uuml", 252},
      {"rdquo", 8221},
      {"yacute", 253},
      {"ge", 8805},
      {"egrave", 232},
      {"ne", 8800},
      {"para", 182},
      {"aelig", 230},
      {"agrave", 224},
      {"harr", 8596},
      {"gamma", 947},
      {"permil", 8240},
      {"nabla", 8711},
      {"weierp", 8472},
      {"Iuml", 207},
      {"lang", 9001},
      {"piv", 982},
      {"uarr", 8593},
      {"Aacute", 193},
      {"Ntilde", 209},
      {"deg", 176},
      {"eta", 951},
      {"igrave", 236},
      {"atilde", 227},
      {"cap", 8745},
      {"bdquo", 8222},
      {"Ecirc", 202},
      {"exist", 8707},
      {"and", 8743},
      {"Alpha", 913},
      {"oelig", 339},
      {"ograve", 242},
      {"rho", 961},
      {"alefsym", 8501},
      {"Psi", 936},
      {"pi", 960},
      {"image", 8465},
      {"sigma", 963},
      {"tilde", 732},
      {"cent", 162},
      {"ni", 8715},
      {"copy", 169},
      {"pound", 163},
      {"otilde", 245},
      {"omega", 969},
      {"rarr", 8594},
      {"clubs", 9827},
      {"forall", 8704},
      {"cong", 8773},
      {"lsquo", 8216},
      {"lsaquo", 8249},
      {"bull", 8226},
      {"Euml", 203},
      {"diams", 9830},
      {"hellip", 8230},
      {"lowast", 8727},
      {"Scaron", 352},
      {"iquest", 191},
      {"Acirc", 194},
      {"micro", 181},
      {"Ugrave", 217},
      {"Ograve", 210},
      {"crarr", 8629},
      {"ugrave", 249},
      {"or", 8744},
      {"yuml", 255},
      {"hearts", 9829},
      {"phi", 966},
      {"eth", 240},
      {"there4", 8756},
      {"theta", 952},
      {"sube", 8838},
      {"supe", 8839},
      {"ndash", 8211},
      {"Otilde", 213},
      {"Omega", 937},
      {"cup", 8746},
      {"tau", 964},
      {"Igrave", 204},
      {"Auml", 196},
      {"sup3", 179},
      {"uml", 168},
      {"mdash", 8212},
      {"sup1", 185},
      {"prime", 8242},
      {"curren", 164},
      {"reg", 174},
      {"sup2", 178},
      {"oslash", 248},
      {"rang", 9002},
      {"aring", 229},
      {"quot", 34},
      {"frac14", 188},
      {"frac34", 190},
      {"prod", 8719},
      {"trade", 8482},
      {"macr", 175},
      {"frasl", 8260},
      {"lfloor", 8970},
      {"lArr", 8656},
      {"upsih", 978},
      {"lrm", 8206},
      {"rlm", 8207},
      {"frac12", 189},
      {"Egrave", 200},
      {"dArr", 8659},
      {"ordf", 170},
      {"nu", 957},
      {"Oslash", 216},
      {"rsquo", 8217},
      {"rsaquo", 8250},
      {"ordm", 186},
      {"perp", 8869},
      {"yen", 165},
      {"Eta", 919},
      {"mu", 956},
      {"ensp", 8194},
      {"epsilon", 949},
      {"ang", 8736},
      {"empty", 8709},
      {"plusmn", 177},
      {"emsp", 8195},
      {"asymp", 8776},
      {"Agrave", 192},
      {"amp", 38},
      {"hArr", 8660},
      {"sbquo", 8218},
      {"part", 8706},
      {"brvbar", 166},
      {"ntilde", 241},
      {"szlig", 223},
      {"uArr", 8657},
      {"infin", 8734},
      {"psi", 968},
      {"sect", 167},
      {"Atilde", 195},
      {"notin", 8713},
      {"isin", 8712},
      {"oplus", 8853},
      {"int", 8747},
      {"scaron", 353},
      {"OElig", 338},
      {"lt", 60},
      {"sigmaf", 962},
      {"not", 172},
      {"omicron", 959},
      {"sim", 8764},
      {"minus", 8722},
      {"sdot", 8901},
      {"times", 215},
      {"middot", 183},
      {"rfloor", 8971},
      {"sub", 8834},
      {"rArr", 8658},
      {"otimes", 8855},
      {"shy", 173},
      {"Upsilon", 933},
      {"upsilon", 965},
      {"thinsp", 8201},
      {"sup", 8835},
      {"thorn", 254},
      {"Omicron", 927},
      {"prop", 8733},
      {"thetasym", 977},
      {"sum", 8721},
      {"Aring", 197},
      {"nbsp", 160},
      {"Epsilon", 917},
      {"AElig", 198},
      {"nsub", 8836},
      {"fnof", 402},
      {"spades", 9824},
      {"gt", 62}
    };

  static const short lookup[] =
    {
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,    0,    1,    2,   -1,   -1,    3,    4,
         5,    6,   -1,   -1,    7,    8,   -1,   -1,
         9,   10,   -1,   -1,   -1,   -1,   11,   12,
        -1,   -1,   -1,   13,   -1,   -1,   14,   15,
        16,   17,   -1,   -1,   -1,   18,   -1,   -1,
        -1,   -1,   19,   20,   -1,   -1,   -1,   21,
        -1,   -1,   -1,   -1,   22,   23,   24,   -1,
        -1,   25,   26,   27,   -1,   -1,   28,   29,
        30,   31,   -1,   32,   -1,   -1,   -1,   33,
        34,   -1,   -1,   -1,   -1,   35,   -1,   -1,
        -1,   -1,   36,   37,   -1,   -1,   -1,   38,
        -1,   -1,   -1,   -1,   39,   -1,   -1,   -1,
        40,   41,   42,   -1,   43,   -1,   -1,   44,
        -1,   45,   -1,   46,   -1,   -1,   47,   48,
        -1,   -1,   -1,   49,   50,   -1,   -1,   -1,
        51,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   52,   -1,   -1,   -1,
        53,   -1,   -1,   -1,   -1,   54,   55,   -1,
        -1,   56,   -1,   -1,   57,   -1,   -1,   58,
        -1,   -1,   -1,   -1,   59,   -1,   -1,   -1,
        -1,   60,   61,   -1,   62,   -1,   -1,   63,
        -1,   -1,   64,   65,   -1,   -1,   -1,   -1,
        66,   -1,   67,   -1,   -1,   -1,   68,   -1,
        -1,   -1,   -1,   69,   -1,   -1,   70,   -1,
        71,   72,   73,   74,   75,   -1,   -1,   76,
        -1,   -1,   -1,   -1,   -1,   -1,   77,   -1,
        78,   -1,   -1,   79,   80,   -1,   -1,   -1,
        -1,   -1,   81,   82,   -1,   -1,   -1,   83,
        84,   -1,   85,   86,   87,   -1,   -1,   88,
        89,   90,   -1,   -1,   -1,   91,   92,   -1,
        93,   94,   -1,   -1,   -1,   95,   96,   97,
        -1,   98,   99,  100,   -1,  101,  102,  103,
       104,  105,  106,   -1,  107,  108,  109,  110,
        -1,  111,   -1,  112,   -1,   -1,  113,   -1,
        -1,   -1,  114,  115,   -1,  116,  117,   -1,
        -1,   -1,  118,   -1,  119,   -1,  120,  121,
        -1,  122,  123,  124,  125,  126,   -1,   -1,
       127,  128,  129,   -1,  130,  131,  132,  133,
       134,   -1,   -1,   -1,  135,  136,   -1,  137,
        -1,   -1,   -1,  138,   -1,   -1,  139,   -1,
       140,   -1,  141,  142,  143,   -1,  144,   -1,
       145,   -1,  146,  147,   -1,   -1,  148,   -1,
        -1,   -1,  149,   -1,   -1, -604,  152, -102,
        -2,   -1,  153,  154,   -1,   -1,  155,  156,
       157,   -1,   -1,  158,  159,   -1,  160,   -1,
       161,  162,  163,  164,   -1,  165,  166,   -1,
       167,   -1,   -1,  168,  169,   -1,   -1,   -1,
        -1,  170,   -1,   -1,   -1,   -1,   -1, -645,
       -81,   -2,   -1,   -1,   -1,   -1,   -1,  173,
        -1,  174,  175,   -1,   -1,  176,  177,   -1,
       178,   -1,  179, -665,  -72,   -2,   -1,   -1,
       182,   -1,   -1,   -1,   -1,  183,   -1,  184,
        -1,   -1,   -1,   -1,   -1,  185,   -1,   -1,
       186,  187,   -1,  188,  189,  190,   -1,   -1,
        -1,   -1,   -1,   -1,  191,   -1,   -1,   -1,
       192,  193,  194,   -1,   -1,   -1,  195,   -1,
        -1,  196,   -1,   -1,   -1,   -1,   -1,  197,
        -1,   -1,   -1,   -1,  198,   -1,   -1,  199,
       200,   -1,   -1, -734,  203,   -1,  204,  205,
       206,  -51,   -2,   -1,   -1,  207,   -1,  208,
        -1,   -1,  209,   -1,   -1,  210,   -1,  211,
        -1,   -1,  212,   -1,  213,  214,   -1,   -1,
        -1,  215,   -1,   -1,   -1,  216,  217,   -1,
        -1,   -1,  218,   -1,  219,  220,   -1,  221,
        -1,  222,   -1,   -1,  223,  224,  225,   -1,
       226,   -1,  227,  228,   -1,   -1,   -1,  229,
       230,  231,  232,   -1,   -1,   -1,  233,   -1,
        -1,   -1,   -1,   -1,  234,   -1,   -1,   -1,
       235,   -1,   -1,   -1,   -1,  236,   -1,   -1,
        -1,   -1,  237,   -1,   -1,   -1,   -1,   -1,
       238,   -1,   -1,  239,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  240,   -1,
        -1,   -1,   -1,   -1,   -1,  241,   -1,   -1,
       242,   -1,   -1,   -1,  243,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  244,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  245,   -1,   -1,   -1,  246,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       247,   -1,   -1,   -1,  248,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  249,   -1,  250,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  251
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                return &wordlist[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct _Entity *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register const struct _Entity *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}

static int leave_builtin = 0;	/* Leave standard entities untouched */

/* append_utf8 -- append the UTF-8 sequence for code n */
static void append_utf8(const unsigned int n)
{
  if (n <= 0x7F) {
    putchar((unsigned char)(n));
  } else if (n <= 0x7FF) {
    putchar((unsigned char)(0xC0 | (n >> 6)));
    putchar((unsigned char)(0x80 | (n & 0x3F)));
  } else if (n <= 0xFFFF) {
    putchar((unsigned char)(0xE0 | (n >> 12)));
    putchar((unsigned char)(0x80 | ((n >> 6) & 0x3F)));
    putchar((unsigned char)(0x80 | (n & 0x3F)));
  } else if (n <= 0x1FFFFF) {
    putchar((unsigned char)(0xF0 | (n >> 18)));
    putchar((unsigned char)(0x80 | ((n >> 12) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 6) & 0x3F)));
    putchar((unsigned char)(0x80 | (n & 0x3F)));
  } else if (n <= 0x3FFFFFF) {
    putchar((unsigned char)(0xF0 | (n >> 24)));
    putchar((unsigned char)(0x80 | ((n >> 18) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 12) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 6) & 0x3F)));
    putchar((unsigned char)(0x80 | (n & 0x3F)));
  } else {
    putchar((unsigned char)(0xF0 | (n >> 30)));
    putchar((unsigned char)(0x80 | ((n >> 24) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 18) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 12) & 0x3F)));
    putchar((unsigned char)(0x80 | ((n >> 6) & 0x3F)));
    putchar((unsigned char)(0x80 | (n & 0x3F)));
  }
}

/* append_named -- append the UTF-8 sequence of a named entity */
static void append_named(const unsigned char *name, unsigned int len)
{
  const struct _Entity *e = lookup_entity(name, len);
  int i;

  if (!e || (leave_builtin && (e->code == 38 || e->code == 60
      || e->code == 62 || e->code == 34))) {	/* Keep it */
    putchar('&');
    for (i = 0; i < len; i++) putchar(name[i]);
    putchar(';');
  } else {					/* Convert to Unicode */
    append_utf8(e->code);
  }
}

/* expand -- print string, expanding entities to UTF-8 sequences */
static void expand(const unsigned char *s)
{
  unsigned int i, n;

  for (i = 0; s[i];) {
    if (s[i] != '&') {				/* Literal character */
      putchar(s[i++]);
    } else if (isalnum(s[i+1])) {		/* Named entity, eg. &eacute */
      for (i++, n = 1; isalnum(s[i+n]); n++) ;
      append_named(s + i, n);
      i += n;
      if (s[i] == ';') i++;
    } else if (s[i+1] != '#') {			/* SGML-style "&" on its own */
      append_named("amp", 3);
      i++;
    } else if (s[i+2] != 'x') {			/* Decimal entity, eg. &#70 */
      for (n = 0, i += 2; isdigit(s[i]); i++) n = 10 * n + s[i] - '0';
      if (leave_builtin && (n == 38 || n == 60 || n == 62 || n == 34))
        printf("&#%d;", n);
      else
        append_utf8(n);
      if (s[i] == ';') i++;
    } else {					/* Hex entity, eg. &#x5F */
      for (n = 0, i += 3; isxdigit(s[i]); i++)
	if (isdigit(s[i])) n = 16 * n + s[i] - '0';
	else n = 16 * n + toupper(s[i]) - 'A' + 10;
      if (leave_builtin && (n == 38 || n == 60 || n == 62 || n == 34))
        printf("&#x%x;", n);
      else
        append_utf8(n);
      if (s[i] == ';') i++;
    }
  }
}

/* main -- read input, expand entities, write out again */
int main(int argc, char *argv[])
{
  unsigned char buf[4096];
  FILE *infile;
  int i = 1;

  if (i < argc && strcmp(argv[i], "-b") == 0) {
    leave_builtin = 1;
    i++;
  }
  if (i == argc) {
    infile = stdin;
  } else if (argc != i + 1) {
    fprintf(stderr, "Version %s\nUsage: %s [-b] [file]\n\
(input is UTF-8 with &-entities, output is UTF-8 without &-entities)\n",
	    VERSION, argv[0]);
    exit(2);
  } else if (!(infile = fopen(argv[1], "r"))) {
    perror(argv[1]);
    exit(1);
  }

  while (fgets(buf, sizeof(buf), infile)) expand(buf);

  fclose(infile);
  return 0;
}
