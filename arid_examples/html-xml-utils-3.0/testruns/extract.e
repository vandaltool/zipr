 void handle_error(void *unused, const string s, int lineno);
 void* start(void);
 void end(void *unused);
 void handle_comment(void *unused, const string commenttext);
 void handle_text(void *unused, const string text);
 void handle_decl(void *unused, const string gi,
   const string fpi, const string url);
 void handle_pi(void *unused, const string pi_text);
 void handle_starttag(void *unused, const string name, pairlist attribs);
 void handle_emptytag(void *unused, const string name,
       pairlist attribs);
 void handle_endtag(void *unused, const string name);
