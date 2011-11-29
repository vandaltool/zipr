 void handle_error(void *clientdata, const string s, int lineno);
 void* start(void);
 void end(void *clientdata);
 void handle_comment(void *clientdata, string commenttext);
 void handle_text(void *clientdata, string text);
 void handle_decl(void *clientdata, string gi,
   string fpi, string url);
 void handle_pi(void *clientdata, string pi_text);
 void handle_starttag(void *clientdata, string name, pairlist attribs);
 void handle_emptytag(void *clientdata, string name, pairlist attribs);
 void handle_endtag(void *clientdata, string name);
