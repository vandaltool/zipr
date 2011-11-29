 void handle_error(void *clientdata, const string s, int lineno);
 void* start(void);
 void end(void *clientdata);
 void handle_comment(void *clientdata, const string commenttext);
 void handle_text(void *clientdata, const string text);
 void handle_decl(void *clientdata, const string gi,
   const string fpi, const string url);
 void handle_pi(void *clientdata, const string pi_text);
 void handle_starttag(void *clientdata, const string name,
       pairlist attribs);
 void handle_emptytag(void *clientdata, const string name,
       pairlist attribs);
 void handle_endtag(void *clientdata, const string name);
