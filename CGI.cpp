// CGI.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<fstream>
#include<iostream>
#include<cstdlib>
#include<string>
#include<cstring>
#include<mysql.h>
#include<unordered_map>
#include<map>
using namespace std;


void debugFunc(const string &s) {
	ofstream f("C:\\Users\\Administrator\\Desktop\\cgi\\debug.txt", ios::out | ios::app | ios::ate);
	f << s << endl;
	f.close();
}

class StringAlgorithm {
public:
	vector<string> split(string src, const string &tag) {
		string curr;
		vector<string> ans;
		if (src.size() > 0 && tag.size() > 0) {
			size_t pos = src.find(tag);
			while (pos < src.size()) {
				curr = src.substr(0, pos);
				if (curr.size() != 0)
					ans.push_back(curr);
				src = src.substr(pos + tag.size());
				pos = src.find(tag);
			}
		}
		if (src.size() != 0)
			ans.push_back(src);
		return ans;
	}
	string join(const vector<string> &list, const string &tag) {
		size_t n = list.size();
		if (n == 0)return string("");
		string ans = list[0];
		for (size_t i = 1; i < n; i++) {
			ans = ans + tag + list[i];
		}
		return ans;
	}
	int replace_all(string &s, const string &oldstr, const string &newstr) {
		if (oldstr.size() == 0)
			return 0;
		int count = 0;
		size_t pos = 0, curpos = 0;
		while ((pos = s.find(oldstr, curpos)) != s.npos) {
			s.replace(pos, oldstr.size(), newstr);
			curpos += newstr.size();
			count++;
		}
		return count;
	}
	string uri_encode(const string &source) {
		const char reserved[] = ";/?:@&=+\0";
		const char unsafe[] = " \"#%<>\0";
		const char other[] = "'`[],~!$^(){}|\\\r\n";

		char chr, element[4] = { 0 };
		string res;
		res.reserve(source.length() * 3);

		for (size_t i = 0; i<source.length(); ++i) {
			chr = source[i];
			if (strchr(reserved, chr) || strchr(unsafe, chr) || strchr(other, chr)) {
				sprintf(element, "%c%02X", '%', chr);
				res += element;
			}
			else {
				res += chr;
			}
		}

		return res;
	}
	string uri_decode(const string &source) {
		size_t pos = 0;
		string seq;
		string str = source;
		string rest = str;

		for (str = ""; ((pos = rest.find('%')) != rest.npos); ) {
			if ((pos + 2)<rest.length() && isalnum(rest[pos + 1]) && isalnum(rest[pos + 2])) {
				seq = rest.substr(pos + 1, 2);
				str += rest.substr(0, pos) + hex_to_asc(seq);
				rest = rest.erase(0, pos + 3);
			}
			else {
				str += rest.substr(0, pos + 1);
				rest = rest.erase(0, pos + 1);
			}
		}

		str += rest;
		return str;
	}
	string UtfToGbk(const char* utf8)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
		wchar_t* wstr = new wchar_t[len + 1];
		memset(wstr, 0, len + 1);
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
		len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* str = new char[len + 1];
		memset(str, 0, len + 1);
		WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
		if (wstr) delete[] wstr;
		return str;
	}
	void upper(string &s) {
		for (size_t i = 0; i<s.size(); i++)
			s[i] = toupper(s[i]);
	}
	void lower(string &s) {
		for (size_t i = 0; i<s.size(); i++)
			s[i] = tolower(s[i]);
	}
	static StringAlgorithm *getStringAlgorithm() {
		static StringAlgorithm sa;
		return &sa;
	}
	string get_env(const string &envname) {
		const char *env = envname.c_str();
		char *val = getenv(env);
		if (val != NULL)
			return string(val);
		else
			return string("");
	}
private:
	StringAlgorithm(){}
	char hex_to_asc(const string &src) {
		char digit;
		digit = (src[0] >= 'A' ? ((src[0] & 0xdf) - 'A') + 10 : (src[0] - '0'));
		digit *= 16;
		digit += (src[1] >= 'A' ? ((src[1] & 0xdf) - 'A') + 10 : (src[1] - '0'));

		return digit;
	}
};


class Request {
public:
	Request(const size_t formdata_maxsize=0) {
		StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
		this->_method = sa->get_env("REQUEST_METHOD");
		sa->upper(this->_method);
		string queryString;
		if (this->_method == "GET") {
			queryString = sa->get_env("QUERY_STRING");
			this->parse_urlencoded(queryString);
		}
		else if(this->_method == "POST") {
			string content_type = sa->get_env("CONTENT_TYPE");
			char c;
			string buf;
			if (formdata_maxsize > 0)
				buf.reserve(formdata_maxsize);
			else
				buf.reserve(256);
			if (content_type.find("application/x-www-form-urlencoded") != content_type.npos) {
				int content_length = atoi(getenv("CONTENT_LENGTH"));
				for (int i = 0; i<content_length; ++i) {
					cin >> c;
					buf += c;
				}
				this->parse_urlencoded(buf);
			}
		}
	}
	virtual ~Request() {};
	string getRequestData(const string &name) {
		if (this->data.find(name) != this->data.end()) {
			return data[name];
		}
		else {
			return string("");
		}
	}
	inline string getMethod() const{
		return this->_method;
	}
	inline unordered_map<string, string> getData() const{
		return this->data;
	}
private:
	void addToData(const string &name, const string &value) {
		if (data[name] == "")
			data[name] = value;
		else
			data[name] += (" " + value);
	}
	void parse_urlencoded(const string &buf) {
		StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
		string buffer = buf;
		vector<string> pairslist = sa->split(buffer, "&");
		for (size_t i = 0; i<pairslist.size(); ++i) {
			string name = pairslist[i].substr(0, pairslist[i].find("="));
			string value = pairslist[i].substr(pairslist[i].find("=") + 1);
			sa->replace_all(name, "+", " ");
			name=sa->uri_decode(name);
			name = sa->UtfToGbk(name.c_str());
			sa->replace_all(value, "+", " ");
			value = sa->uri_decode(value);
			value= sa->UtfToGbk(value.c_str());
			addToData(name, value);
		}
	}
	unordered_map<string, string> data;
	string _method;
};

struct cookie_struct {
	string name;
	string value;
	string expires;
	string path;
	string domain;
};

class cookies {
public:
public:
	cookies() {
		StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
		string cookie_string = sa->get_env("HTTP_COOKIE");
		this->parse_cookie(cookie_string);
	}
	virtual ~cookies() {};
	string get_cookie(const string &name) {
		if (name.size()>0 && _cookies.find(name) != _cookies.end()) {
			return _cookies[name];
		}
		else {
			return string("");
		}
	}
	unordered_map<string, cookie_struct> getChange() {
		return this->change;
	}
	void set_cookie(const string &name, const string &value,
		const string &expires = "", const string &path = "/",
		const string &domain = "") {
		cookie_struct cs;
		cs.name = name;
		cs.value = value;
		cs.expires = expires;
		cs.path = path;
		cs.domain = domain;
		change[name] = cs;
	}
	inline void del_cookie(const string &name) {
		this->set_cookie(name, "", "Thursday,01-January-1970 08:00:01 GMT");
	}
private:
	void parse_cookie(const string &buf) {
		StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
		string buffer = buf;
		vector<string> pairslist = sa->split(buffer, ";");

		for (size_t i = 0; i<pairslist.size(); ++i) {
			string name = pairslist[i].substr(0, pairslist[i].find("="));
			string value = pairslist[i].substr(pairslist[i].find("=") + 1);

			sa->replace_all(name, "+", " ");
			name = sa->uri_decode(name);
			sa->replace_all(value, "+", " ");
			value = sa->uri_decode(value);

			_cookies[name] = value;
		}
	}
	unordered_map<string, string> _cookies;
	unordered_map<string, cookie_struct> change;
};

class HttpResponseDecrorate {
public:
	void static render(const string &htmlPath, const string &content_type = "text/html", cookies Cookie = cookies()) {
		string res = getHeader(content_type, Cookie);
		string HtmlFile = "", HtmlLine = "";
		ifstream inputFile;
		inputFile.open(htmlPath, ios::skipws);
		while (getline(inputFile, HtmlLine)) {
			HtmlFile = HtmlFile + HtmlLine + "\n";
		}
		cout << res << HtmlFile;
	}
	void static response(const string &content, const string &content_type = "text/html" , cookies Cookie=cookies()) {
		string res = getHeader(content_type, Cookie);
		cout << res << content;
	}
private:
	string static getHeader(const string &content_type, cookies Cookie) {
		string cookie_s = "";
		unordered_map<string, cookie_struct> ChangeCookieMap = Cookie.getChange();
		for (auto cookie = ChangeCookieMap.begin(); cookie != ChangeCookieMap.end();++cookie) {
			string expires_setting, domain_setting;
			if (cookie->second.expires == "")
				expires_setting = "";
			else
				expires_setting = "expires=" + cookie->second.expires + "; ";
			cookie_s = cookie_s
				+ "Set-Cookie: " + cookie->second.name + "=" + cookie->second.value + "; "
				+ expires_setting
				+ "path=" + cookie->second.path + "; "
				+ "domain=" + cookie->second.domain + "; " + "\n";
		}
		string res = "";
		res = res + cookie_s + "Content-type: " + content_type + ";charset=utf-8\n\n";
		return res;
	}
};


void add(Request r) {
	HttpResponseDecrorate::response("<b>hello world:/</b><br/><img src='/static/1.jpg'/>");
}

void test(Request r) {
	HttpResponseDecrorate::render("C:\\Users\\Administrator\\Desktop\\cgi\\template\\test.html");
}

void queryTest(Request r) {
	cookies cookie;
	MYSQL myCont;
	MYSQL_RES *result = NULL;
	MYSQL_ROW sql_row;
	int res;
	string ans = "";
	mysql_init(&myCont);
	unordered_map<string, string> requestData = r.getData();
	if (mysql_real_connect(&myCont, "localhost", "root", "huangweiwei", "myTest", 3306, NULL, 0)) {
		ans = "connect ok\n";
		mysql_query(&myCont, "SET NAMES GBK");
		string qs = "select * from students";
		qs = qs + " where age=\'" + requestData["fname"] + "\';";
		res = mysql_query(&myCont, qs.c_str());
		if (!res) {
			result = mysql_store_result(&myCont);
			if (result) {
				while (sql_row = mysql_fetch_row(result))
				{
					string id = sql_row[0];
					string sex = sql_row[2];
					ans = ans + "id:" + sql_row[0] + "\n";
					ans = ans + "sex:" + sql_row[2] + "\n";
					cookie.set_cookie("id", id);
					cookie.set_cookie("sex", sex);
				}
			}
			else {
				ans = "query error\n";
			}
		}
	}
	else {
		ans = "error\n";
	}
	if (result != NULL)
		mysql_free_result(result);
	mysql_close(&myCont);
	/*cout << "Content-type: text/html\n\n";
	cout << ans << endl;*/
	HttpResponseDecrorate::response(ans, "text/html", cookie);
}

void PostTest(Request r) {
	/*string HtmlFile = "", HtmlLine = "";
	ifstream inputFile;
	inputFile.open("C:\\Users\\Administrator\\Desktop\\cgi\\template\\postTest.html", ios::skipws);
	while (getline(inputFile, HtmlLine)) {
		HtmlFile = HtmlFile + HtmlLine + "\r\n";
	}
	cout << "Content-type: text/html\n\n" << HtmlFile << endl;*/
	HttpResponseDecrorate::render("C:\\Users\\Administrator\\Desktop\\cgi\\template\\postTest.html");
}

void json(Request r) {
	HttpResponseDecrorate::response("{\"result\":\"ok\",\"name\":\"成龙\"}", "application/json");
}

int main()
{
	StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
	string s = sa->get_env("SCRIPT_NAME");
	Request r;
	unordered_map<string, void(*)(Request)> urlMapping{
		{ "/",add },
		{ "/test",test },
		{ "/query",queryTest },
		{"/ptest",PostTest },
		{"/api",json },
	};
	if (urlMapping.find(s) != urlMapping.end()) {
		urlMapping[s](r);
	}
	else {
		cout << "Content-type: text/html\n\n";
		string error_message = "<b>404 can't find any function in the CGI.Using path is " + s + ".</b>";
		cout << error_message << endl;
	}
    return 0;
}

