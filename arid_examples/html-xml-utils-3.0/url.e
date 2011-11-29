 typedef struct {
  string full;
  string proto;
  string user;
  string password;
  string machine;
  string port;
  string path;
  string fragment;
} *URL;
 void URL_dispose(URL url);
 URL URL_new(const string url);
 URL URL_absolutize(const URL base, const URL url);
 string URL_s_absolutize(const string base, const string url);
