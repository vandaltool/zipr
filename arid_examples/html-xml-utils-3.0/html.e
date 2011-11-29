 typedef void (*html_handle_error_fn)
  (void *clientdata, const string s, int lineno);
 typedef void* (*html_handle_start_fn)
  (void);
 typedef void (*html_handle_end_fn)
  (void *clientdata);
 typedef void (*html_handle_comment_fn)
  (void *clientdata, const string commenttext);
 typedef void (*html_handle_text_fn)
  (void *clientdata, const string text);
 typedef void (*html_handle_decl_fn)
  (void *clientdata, const string gi, const string fpi, const string url);
 typedef void (*html_handle_pi_fn)
  (void *clientdata, const string pi_text);
 typedef void (*html_handle_starttag_fn)
  (void *clientdata, const string name, pairlist attribs);
 typedef void (*html_handle_emptytag_fn)
  (void *clientdata, const string name, pairlist attribs);
 typedef void (*html_handle_endtag_fn)
  (void *clientdata, const string name);
 typedef void (*html_handle_endincl_fn)
  (void *clientdata);
 extern int yyparse(void);
 void set_error_handler(html_handle_error_fn f);
 void set_start_handler(html_handle_start_fn f);
 void set_end_handler(html_handle_end_fn f);
 void set_comment_handler(html_handle_comment_fn f);
 void set_text_handler(html_handle_text_fn f);
 void set_decl_handler(html_handle_decl_fn f);
 void set_pi_handler(html_handle_pi_fn f);
 void set_starttag_handler(html_handle_starttag_fn f);
 void set_emptytag_handler(html_handle_emptytag_fn f);
 void set_endtag_handler(html_handle_endtag_fn f);
 void set_endincl_handler(html_handle_endincl_fn f);
