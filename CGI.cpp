#include<iostream>
#include<fstream>
#include<winsock2.h>
#include<vector>
#include<mysql.h>
#include<string>
#include<cstring>
#include<cstdio>
#include<cstdlib>
#include<unordered_map>
#include<unordered_set>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
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
		string ans(str);
		delete[] str;
		return ans;
	}

	string GbkToUtf(const char *gbk) {
		int len = MultiByteToWideChar(CP_ACP, 0, gbk, -1, NULL, 0);
		wchar_t *wstr = new wchar_t[len + 1];
		memset(wstr, 0, len + 1);
		MultiByteToWideChar(CP_ACP, 0, gbk, -1, wstr, len);
		len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* str = new char[len + 1];
		memset(str, 0, len + 1);
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
		if (wstr) delete[] wstr;
		string ans(str);
		delete[] str;
		return ans;
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
		inputFile.open(htmlPath);
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

string JsonToString(const rapidjson::Value &v){
    rapidjson::StringBuffer buff;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
    v.Accept(writer);
    string ans=buff.GetString();
    return ans;
}


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
	StringAlgorithm *sa = StringAlgorithm::getStringAlgorithm();
	if (mysql_real_connect(&myCont, "localhost", "root", "", "myTest", 3306, NULL, 0)) {
		ans = "connect ok\n";
		mysql_query(&myCont, "SET NAMES GBK");
		string qs = "select * from students";
		qs = qs + " where sex=\'" + requestData["fname"] + "\';";
		res = mysql_query(&myCont, qs.c_str());
		if (!res) {
			result = mysql_store_result(&myCont);
			if (result) {
				while (sql_row = mysql_fetch_row(result))
				{
					string id = sql_row[0];
					string sex = sql_row[2];
					ans = ans + "id:" + sql_row[0] + "\n";
					ans = ans + "sex:" + sa->GbkToUtf(sql_row[2]) + "\n";
					cookie.set_cookie("id", id);
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
	HttpResponseDecrorate::response(ans, "text/html", cookie);
}




void PostTest(Request r) {
	HttpResponseDecrorate::render("C:\\Users\\Administrator\\Desktop\\cgi\\template\\postTest.html");
}

void json(Request r) {
	HttpResponseDecrorate::response("{\"result\":\"ok\",\"name\":\"成龙\"}", "application/json");
}

void getTest(Request r) {
	unordered_map<string, string> requestCan = r.getData();
	if (requestCan.size() == 0) {
		return HttpResponseDecrorate::response("<b>no get things</b>");
	}
	else {
		string ans = "";
		for (auto iter = requestCan.begin(); iter != requestCan.end(); ++iter) {
			ans = ans + StringAlgorithm::getStringAlgorithm()->GbkToUtf(iter->first.c_str()) + "=" + StringAlgorithm::getStringAlgorithm()->GbkToUtf(iter->second.c_str()) + " ";
		}
		return HttpResponseDecrorate::response(ans);
	}
}

void getProvince(Request request){
	MYSQL mysql;    //mysql连接

      MYSQL_RES *res; //这个结构代表返回行的一个查询结果集

      MYSQL_ROW row; //一个行数据的类型安全(type-safe)的表示

      //char *query; //查询语句
      string query;

      int t;

      mysql_init(&mysql);

      rapidjson::Value ans(rapidjson::kObjectType);
      rapidjson::Value province_array(rapidjson::kArrayType);
      rapidjson::Document document;

     //主机名，账号，密码，数据库名，端口号
     mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
     string encoding="SET CHARACTER SET GBK";

      t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());

      if(t){
          return HttpResponseDecrorate::response("<b>error</b>");
      }

      //const char* q2 =" select * from goods";
      query = "select province_name,id from province";

      t=mysql_real_query(&mysql,query.c_str(),(unsigned int)strlen(query.c_str()));
      //c_str()函数是c++中将string转化为const char*的函数
      if(t){
          return HttpResponseDecrorate::response("<b>error</b>");
      }
      res=mysql_store_result(&mysql);

      while(row=mysql_fetch_row(res))
      {
          rapidjson::Value Every(rapidjson::kObjectType);

          string province_name=row[0];
          string id=row[1];

          Every.AddMember(rapidjson::Value().SetString(province_name.c_str(),document.GetAllocator()),
                          rapidjson::Value().SetString(id.c_str(),document.GetAllocator()),
                          document.GetAllocator());
          province_array.PushBack(Every,document.GetAllocator());

      }
      ans.AddMember("provinces",province_array,document.GetAllocator());
      rapidjson::StringBuffer buff;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
      ans.Accept(writer);

      string jsonString=buff.GetString();
      jsonString=StringAlgorithm::getStringAlgorithm()->GbkToUtf(jsonString.c_str());


      mysql_free_result(res);
      mysql_close(&mysql);

      return HttpResponseDecrorate::response(jsonString,"application/json");
}

void reg(Request r){
    if(r.getMethod()=="GET"){
        return HttpResponseDecrorate::render("template\\register.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=r.getData();
        string query="";
        query = query+  "insert alumnus values("+
                        "null,"+
                        "default,"+
                        "\""+data["username"]+"\","+
                        "\""+data["password"]+"\","+
                        "\""+data["name"]+"\","+
                        "\""+data["sex"]+"\","+
                        "\""+data["email"]+"\","+
                        "\""+data["major"]+"\","+
                        data["year"]+","+
                        "\""+data["qualification"]+"\","+
                        data["province"]+");";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json");
    }
}

void login(Request request){
    cookies c;
    if(request.getMethod()=="GET"){
        return HttpResponseDecrorate::render("template\\login.html");
    }else{
        string query="";
        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=request.getData();
        query=query+"select id from alumnus where username=\""+data["username"]+"\" and passwd=\""+data["password"]+"\";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        res=mysql_store_result(&mysql);
        string id;
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            id=row[0];
            getIn=true;
        }
        if(!getIn){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        c.set_cookie("UserId",id);
        mysql_free_result(res);
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json",c);
    }
}

void logout(Request request){
    cookies c;
    if(c.get_cookie("UserId").size()>0){
        c.del_cookie("UserId");
    }
    return HttpResponseDecrorate::render("template\\logout.html","text/html",c);
}

void getUserInfo(Request r){
    cookies c;
    string id;
    if((id=c.get_cookie("UserId"))!=""){
        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        query=query+"select alumnus.id,"+
                    "alumnus.pic,"+
                    "alumnus.username,"+
                    "alumnus.true_name,"+
                    "alumnus.sex,"+
                    "alumnus.email,"+
                    "alumnus.marjor,"+
                    "alumnus.graduate_year,"+
                    "alumnus.qualifications,"+
                    "province.province_name"+
                    " from alumnus,province where province.id=alumnus.province and alumnus.id="+id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        res=mysql_store_result(&mysql);
        string user_id,pic,user_name,true_name,sex,email,marjor,year,qual,province;
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            user_id=row[0];
            pic=row[1];
            user_name=row[2];
            true_name=row[3];
            sex=row[4];
            email=row[5];
            marjor=row[6];
            year=row[7];
            qual=row[8];
            province=row[9];
            getIn=true;
        }
        if(!getIn){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        mysql_free_result(res);
        mysql_close(&mysql);


        rapidjson::Value ans(rapidjson::kObjectType);
        rapidjson::Value userInfo(rapidjson::kObjectType);
        rapidjson::Document document;

        ans.AddMember("status",rapidjson::Value().SetString("ok",document.GetAllocator()),document.GetAllocator());

        userInfo.AddMember("id",rapidjson::Value().SetString(user_id.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("pic",rapidjson::Value().SetString(pic.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("user_name",rapidjson::Value().SetString(user_name.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("true_name",rapidjson::Value().SetString(true_name.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("sex",rapidjson::Value().SetString(sex.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("email",rapidjson::Value().SetString(email.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("marjor",rapidjson::Value().SetString(marjor.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("year",rapidjson::Value().SetString(year.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("qual",rapidjson::Value().SetString(qual.c_str(),document.GetAllocator()),document.GetAllocator());
        userInfo.AddMember("province",rapidjson::Value().SetString(province.c_str(),document.GetAllocator()),document.GetAllocator());

        ans.AddMember("userInfo",userInfo,document.GetAllocator());
        return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");
    }else{
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json",c);
    }
}

void message(Request request){
    cookies c;
    if(c.get_cookie("UserId").size()==0){
        return HttpResponseDecrorate::render("template\\LoginFirst.html");
    }
    if(request.getMethod()=="GET"){
        return HttpResponseDecrorate::render("template\\message.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=request.getData();
        string query="";
        query = query+  "insert message values("+
                        "null,"+
                        "\""+data["title"]+"\","+
                        "\""+data["content"]+"\","+
                        "default,"+
                        ""+data["userid"]+","+
                        "true,"+
                        "true);";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json");
    }
}

void getAllMessage(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }else{
        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        query=query+"select message.*,alumnus.true_name from message,alumnus where message.own=alumnus.id and admin_showed=1 and owner_showed=1 "+
        "union "+
        "select message.*,alumnus.true_name from message,alumnus where message.own=alumnus.id and admin_showed=1 and alumnus.id="+user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        res=mysql_store_result(&mysql);
        string message_id,title,content,timestamp,own_id,owner_showed,admin_showed,true_name,user_shown;

        rapidjson::Value ans(rapidjson::kObjectType);
        rapidjson::Value message_array(rapidjson::kArrayType);
        rapidjson::Document document;

        while(row=mysql_fetch_row(res)){
            message_id=row[0];
            title=row[1];
            content=row[2];
            timestamp=row[3];
            own_id=row[4];
            user_shown=row[5];
            true_name=row[7];
            if(own_id==user_id)
                own_id="true";
            else
                own_id="false";
            if(user_id=="1")
                own_id="true";
            rapidjson::Value one_row(rapidjson::kObjectType);

            one_row.AddMember("id",rapidjson::Value().SetString(message_id.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("title",rapidjson::Value().SetString(title.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("content",rapidjson::Value().SetString(content.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("timestamp",rapidjson::Value().SetString(timestamp.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("true_name",rapidjson::Value().SetString(true_name.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("isOwner",rapidjson::Value().SetString(own_id.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("user_shown",rapidjson::Value().SetString(user_shown.c_str(),document.GetAllocator()),document.GetAllocator());

            message_array.PushBack(one_row,document.GetAllocator());
        }
        mysql_free_result(res);
        mysql_close(&mysql);
        ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
        ans.AddMember("messages",message_array,document.GetAllocator());
        return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");
    }
}


void modifyMessage(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.empty())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        if(data.find("message_id")==data.end())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        string message_id=data["message_id"];
        if(message_id.size()==0)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        query=query+"select * from message where id="+message_id+" and own="+user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        res=mysql_store_result(&mysql);
        string admin_per;
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            admin_per=row[6];
            getIn=true;
        }
        if(admin_per=="0")
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        if(!getIn)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        mysql_free_result(res);
        mysql_close(&mysql);
        return HttpResponseDecrorate::render("template\\modifyMessage.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=request.getData();
        string query="";
        if(user_id=="1"){
            query = query+  "update message set "+
                        "title=\""+data["title"]+"\","
                        "content=\""+data["content"]+"\","
                        "admin_showed="+data["show"]+" where id="+data["message_id"]+";";
        }
        else{
            query = query+  "update message set "+
                        "title=\""+data["title"]+"\","
                        "content=\""+data["content"]+"\","
                        "owner_showed="+data["show"]+" where id="+data["message_id"]+";";
        }
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json");
    }
}

void Townsmen(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.find("id")==data.end()){
            return HttpResponseDecrorate::render("template\\townsmen.html");
        }else{
            return HttpResponseDecrorate::render("template\\townsmen_detail.html");
        }
    }else{
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
}

void getTownsmenDetail(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.find("id")==data.end()){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }else{
            rapidjson::Value ans(rapidjson::kObjectType);
            rapidjson::Document document;
            rapidjson::Value admin(rapidjson::kObjectType);

            MYSQL mysql;
            MYSQL_RES *res;
            MYSQL_ROW row;
            int t;
            mysql_init(&mysql);
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            string query="";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            query=query+
                "select t.pic,t.true_name,t.sex,t.email,t.marjor,t.graduate_year,t.qualifications,t.province_name,province.province_name from province,(select alumnus.*,province.province_name from alumnus,province where province.Admin=alumnus.id and province.id="+
                data["id"]+
                ") t where t.province=province.id;";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            res=mysql_store_result(&mysql);
            string pic,true_name,sex,email,marjor,graduate_year,qualifications,this_province,admin_province;
            bool getIn=false;
            while(row=mysql_fetch_row(res)){
                pic=row[0];
                true_name=row[1];
                sex=row[2];
                email=row[3];
                marjor=row[4];
                graduate_year=row[5];
                qualifications=row[6];
                this_province=row[7];
                admin_province=row[8];
                getIn=true;


                admin.AddMember("pic",rapidjson::Value().SetString(pic.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("true_name",rapidjson::Value().SetString(true_name.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("sex",rapidjson::Value().SetString(sex.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("email",rapidjson::Value().SetString(email.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("marjor",rapidjson::Value().SetString(marjor.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("graduate_year",rapidjson::Value().SetString(graduate_year.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("qualifications",rapidjson::Value().SetString(qualifications.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("this_province",rapidjson::Value().SetString(this_province.c_str(),document.GetAllocator()),document.GetAllocator());
                admin.AddMember("admin_province",rapidjson::Value().SetString(admin_province.c_str(),document.GetAllocator()),document.GetAllocator());
            }
            if(!getIn)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            mysql_free_result(res);
            mysql_close(&mysql);

             ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
            ans.AddMember("admin",admin,document.GetAllocator());
            return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");
        }
    }else{
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
}



void all_forum(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }else{
        if(request.getMethod()=="GET"){
            return HttpResponseDecrorate::render("template\\all_forum.html");
        }
        else{
            MYSQL mysql;
            mysql_init(&mysql);
            int t;
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            unordered_map<string,string> data=request.getData();
            string query="";
            query = query+  "insert forum values("+
                            "null,"+
                            "\""+data["title"]+"\","+
                            "\""+data["content"]+"\","+
                            user_id+","+
                            "true,"+
                            "true,"+
                            "default);";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            mysql_close(&mysql);
            return HttpResponseDecrorate::response("ok");
        }
    }
}

void getAllForum(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }else{
        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        query=query+"select forum.id,forum.title,forum.create_time,user_shown,content,alumnus.username,alumnus.id from forum,alumnus where forum.belong=alumnus.id and forum.admin_shown=1 and forum.user_shown=1 "+
        "union "+
        "select forum.id,forum.title,forum.create_time,user_shown,content,alumnus.username,alumnus.id from forum,alumnus where forum.belong=alumnus.id and forum.admin_shown=1 and forum.belong="+user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        res=mysql_store_result(&mysql);
        string forum_id,title,timestamp,user_shown,content,username,own_id;


        rapidjson::Value ans(rapidjson::kObjectType);
        rapidjson::Value message_array(rapidjson::kArrayType);
        rapidjson::Document document;

        while(row=mysql_fetch_row(res)){
            forum_id=row[0];
            title=row[1];
            timestamp=row[2];
            user_shown=row[3];
            content=row[4];
            username=row[5];
            own_id=row[6];
            if(own_id==user_id)
                own_id="true";
            else
                own_id="false";
            if(user_id=="1")
                own_id="true";
            rapidjson::Value one_row(rapidjson::kObjectType);

            one_row.AddMember("id",rapidjson::Value().SetString(forum_id.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("title",rapidjson::Value().SetString(title.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("content",rapidjson::Value().SetString(content.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("timestamp",rapidjson::Value().SetString(timestamp.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("isOwner",rapidjson::Value().SetString(own_id.c_str(),document.GetAllocator()),document.GetAllocator());
            one_row.AddMember("user_shown",rapidjson::Value().SetString(user_shown.c_str(),document.GetAllocator()),document.GetAllocator());

            message_array.PushBack(one_row,document.GetAllocator());
        }
        mysql_free_result(res);
        mysql_close(&mysql);
        ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
        ans.AddMember("forum",message_array,document.GetAllocator());
        return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");
    }
}


void forum(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }else{
        if(request.getMethod()=="GET"){
            unordered_map<string,string> data=request.getData();
            if(data.size()==0)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            if(data.find("id")==data.end())
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            string forum_id=data["id"];

            MYSQL mysql;
            MYSQL_RES *res;
            MYSQL_ROW row;
            int t;
            mysql_init(&mysql);
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            string query="";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            query=query+"select * from forum where forum.admin_shown=1 and forum.user_shown=1 and forum.id="+forum_id+";";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            res=mysql_store_result(&mysql);
            bool getIn=false;
            while(row=mysql_fetch_row(res)){
                getIn=true;
            }
            if(!getIn)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            mysql_free_result(res);
            mysql_close(&mysql);
            return HttpResponseDecrorate::render("template\\forum.html");
        }
        else{
            MYSQL mysql;
            mysql_init(&mysql);
            int t;
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            unordered_map<string,string> data=request.getData();
            string forum_id,comments_content;
            forum_id=data["forum_id"];
            comments_content=data["content"];
            string query="";
            query = query+"insert comments select null,"+
                        forum_id+",count(*)+1,null,"+
                        "\""+comments_content+"\","+
                        user_id+","+
                        "1,1,current_timestamp from comments where "
                        "forum_id="+
                        forum_id+";";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            mysql_close(&mysql);
            return HttpResponseDecrorate::response("ok");
        }
    }
}

void getForumData(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }else{
        if(request.getMethod()=="GET"){
            unordered_map<string,string> data=request.getData();
            if(data.size()==0)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            if(data.find("forum_id")==data.end())
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            string forum_id=data["forum_id"];

            rapidjson::Value ans(rapidjson::kObjectType);
            rapidjson::Document document;
            rapidjson::Value sources(rapidjson::kObjectType);
            rapidjson::Value comments(rapidjson::kArrayType);

            MYSQL mysql;
            MYSQL_RES *res;
            MYSQL_ROW row;
            int t;
            mysql_init(&mysql);
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            string query="";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            query=query+
                "select * from "+
                "(select forum.id id,forum.title,forum.content,forum.create_time,alumnus.username from forum,alumnus where forum.belong=alumnus.id and forum.user_shown=1 and forum.admin_shown=1 and "+
                "forum.id="+forum_id+
                ") forum_table left join"+
                "(select comments.forum_id id,comments.layer,comments.content,comments.create_time,ifnull(comments.repto,0),alumnus.username,comments.user_shown,comments.belong from forum,alumnus,comments where forum.id=comments.forum_id and comments.belong=alumnus.id and comments.admin_shown=1 and comments.user_shown=1 "+
                "union "+
                "select comments.forum_id id,comments.layer,comments.content,comments.create_time,ifnull(comments.repto,0),alumnus.username,comments.user_shown,comments.belong from forum,alumnus,comments where forum.id=comments.forum_id and comments.belong=alumnus.id and comments.admin_shown=1 and "+
                "comments.belong="+user_id+
                ") comments_table "
                "on forum_table.id=comments_table.id;";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t)
                return HttpResponseDecrorate::response(query);
            res=mysql_store_result(&mysql);
            string forum_title,forum_content,forum_time,forum_user,comments_layer,comments_content,comments_time,comments_repto,comments_user,comments_shown,comments_belong;
            bool getIn=false;
            while(row=mysql_fetch_row(res)){
                if(!getIn){
                    getIn=true;
                    forum_title=row[1];
                    forum_content=row[2];
                    forum_time=row[3];
                    forum_user=row[4];

                    sources.AddMember("title",rapidjson::Value().SetString(forum_title.c_str(),document.GetAllocator()),document.GetAllocator());
                    sources.AddMember("content",rapidjson::Value().SetString(forum_content.c_str(),document.GetAllocator()),document.GetAllocator());
                    sources.AddMember("time",rapidjson::Value().SetString(forum_time.c_str(),document.GetAllocator()),document.GetAllocator());
                    sources.AddMember("user",rapidjson::Value().SetString(forum_user.c_str(),document.GetAllocator()),document.GetAllocator());
                    ans.AddMember("forum",sources,document.GetAllocator());
                }

                if(row[5]){
                    rapidjson::Value comments_item(rapidjson::kObjectType);

                    comments_layer=row[6];
                    comments_content=row[7];
                    comments_time=row[8];
                    comments_repto=row[9];
                    comments_user=row[10];
                    comments_shown=row[11];
                    comments_belong=row[12];

                    if(comments_belong==user_id||user_id=="1")
                        comments_belong="true";
                    else
                        comments_belong="false";

                    comments_item.AddMember("layer",rapidjson::Value().SetString(comments_layer.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("content",rapidjson::Value().SetString(comments_content.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("time",rapidjson::Value().SetString(comments_time.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("repto",rapidjson::Value().SetString(comments_repto.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("user",rapidjson::Value().SetString(comments_user.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("isShown",rapidjson::Value().SetString(comments_shown.c_str(),document.GetAllocator()),document.GetAllocator());
                    comments_item.AddMember("isOwn",rapidjson::Value().SetString(comments_belong.c_str(),document.GetAllocator()),document.GetAllocator());

                    comments.PushBack(comments_item,document.GetAllocator());
                }


            }

            if(!getIn)
                return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
            mysql_free_result(res);
            mysql_close(&mysql);

            ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
            ans.AddMember("comments",comments,document.GetAllocator());
            return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");

        }
        else{
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
    }
}

void repto(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }else{
        if(request.getMethod()=="GET"){

            unordered_map<string,string> data=request.getData();
            if(data.size()==0)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            if(data.find("forum_id")==data.end())
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            if(data.find("layer_id")==data.end())
                return HttpResponseDecrorate::render("template\\NoPermission.html");

            string layer_id=data["layer_id"],forum_id=data["forum_id"];

            MYSQL mysql;
            MYSQL_RES *res;
            MYSQL_ROW row;
            mysql_init(&mysql);
            int t;
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t){
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            }
            string query="";
            query = query+"select * from comments where user_shown=1 and admin_shown=1 and "
                        "layer="+layer_id+
                        " and forum_id="+forum_id+";";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t){
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            }
            res=mysql_store_result(&mysql);
            bool getIn=false;
            while(row=mysql_fetch_row(res)){
                getIn=true;
            }
            if(!getIn)
                return HttpResponseDecrorate::render("template\\NoPermission.html");
            mysql_free_result(res);
            mysql_close(&mysql);
            return HttpResponseDecrorate::render("template\\repto.html");
        }
        else{
            MYSQL mysql;
            mysql_init(&mysql);
            int t;
            mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
            string encoding="SET CHARACTER SET GBK";
            t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            unordered_map<string,string> data=request.getData();
            string forum_id,comments_content,layer_id;
            forum_id=data["forum_id"];
            layer_id=data["layer_id"];
            comments_content=data["content"];
            string query="";
            query = query+"call repto_func("+
                            layer_id+","+
                            "\""+comments_content+"\","+
                            user_id+","+
                            forum_id+");";
            t=mysql_real_query(&mysql,query.c_str(),query.size());
            if(t){
                return HttpResponseDecrorate::response("error");
            }
            mysql_close(&mysql);
            return HttpResponseDecrorate::response("ok");
        }
    }
}


void modify_forum(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.empty())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        if(data.find("forum_id")==data.end())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        string forum_id=data["forum_id"];
        if(forum_id.size()==0)
            return HttpResponseDecrorate::render("template\\NoPermission.html");



        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        query=query+"select * from forum where admin_shown=1 and belong="+
                    user_id+" and id="+
                    forum_id+
                    ";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        res=mysql_store_result(&mysql);
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            getIn=true;
        }
        if(!getIn)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        mysql_free_result(res);
        mysql_close(&mysql);
        return HttpResponseDecrorate::render("template\\modify_forum.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=request.getData();
        string query="";
        if(user_id=="1"){
            query = query+"update forum set "+
                        "title=\""+data["title"]+"\","
                        "content=\""+data["content"]+"\","
                        "admin_shown="+data["show"]+" where id="+data["forum_id"]+";";
        }
        else{
            query = query+"update forum set "+
                        "title=\""+data["title"]+"\","
                        "content=\""+data["content"]+"\","
                        "user_shown="+data["show"]+" where id="+data["forum_id"]+";";
        }
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json");
    }
}


void modify_comments(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.empty())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        if(data.find("forum_id")==data.end())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        string forum_id=data["forum_id"];
        if(forum_id.size()==0)
            return HttpResponseDecrorate::render("template\\NoPermission.html");

        if(data.find("layer_id")==data.end())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        string layer_id=data["layer_id"];
        if(layer_id.size()==0)
            return HttpResponseDecrorate::render("template\\NoPermission.html");


        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        query=query+"select * from comments where admin_shown=1 and belong="+
                    user_id+" and forum_id="+
                    forum_id+
                    " and layer="+
                    layer_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        res=mysql_store_result(&mysql);
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            getIn=true;
        }
        if(!getIn)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        mysql_free_result(res);
        mysql_close(&mysql);
        return HttpResponseDecrorate::render("template\\modify_comments.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        }
        unordered_map<string,string> data=request.getData();
        string query="";
        if(user_id=="1"){
            query = query+"update comments set "+
                        "content=\""+data["content"]+"\","
                        "admin_shown="+data["show"]+" where forum_id="+data["forum_id"]+" and layer="+data["layer_id"]+";";
        }
        else{
            query = query+"update comments set "+
                        "content=\""+data["content"]+"\","
                        "user_shown="+data["show"]+" where forum_id="+data["forum_id"]+" and layer="+data["layer_id"]+";";
        }
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response(query);
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("{\"status\":\"ok\"}","application/json");
    }
}

void user_admin(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data["user_id"]!=user_id)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        return HttpResponseDecrorate::render("template\\user_admin.html");
    }else{
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
}


void townsmen_send(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::render("template\\NoPermission.html");
    }
    if(request.getMethod()=="GET"){
        unordered_map<string,string> data=request.getData();
        if(data.empty())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        if(data.find("province_id")==data.end())
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        string province_id=data["province_id"];
        if(province_id.size()==0)
            return HttpResponseDecrorate::render("template\\NoPermission.html");


        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        query=query+"select * from province where id="+
        province_id+
        " and admin="+
        user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        res=mysql_store_result(&mysql);
        bool getIn=false;
        while(row=mysql_fetch_row(res)){
            getIn=true;
        }
        if(!getIn)
            return HttpResponseDecrorate::render("template\\NoPermission.html");
        mysql_free_result(res);
        mysql_close(&mysql);
        return HttpResponseDecrorate::render("template\\townsmen_send.html");
    }else{
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        unordered_map<string,string> data=request.getData();
        string query="";
        query = query+  "insert townsmen_message select null,province.id,alumnus.id,"+
            "\""+data["content"]+"\""
            ",current_timestamp,false from alumnus,province where province.id=alumnus.province and province.id="+
            data["province_id"]+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("ok");
    }
}


void get_unread_townsmen_message(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
    if(request.getMethod()=="GET"){
        rapidjson::Value ans(rapidjson::kObjectType);
        rapidjson::Value townsmen_messages(rapidjson::kArrayType);
        rapidjson::Document document;


        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        query=query+"select * from townsmen_message where readed=0 and user_id="+user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        res=mysql_store_result(&mysql);
        bool getIn=false;
        string townsmen_send_id,message,create_time;
        while(row=mysql_fetch_row(res)){
            getIn=true;

            rapidjson::Value townsmen_message_item(rapidjson::kObjectType);

            townsmen_send_id=row[0];
            message=row[3];
            create_time=row[4];

            townsmen_message_item.AddMember("id",rapidjson::Value().SetString(townsmen_send_id.c_str(),document.GetAllocator()),document.GetAllocator());
            townsmen_message_item.AddMember("message",rapidjson::Value().SetString(message.c_str(),document.GetAllocator()),document.GetAllocator());
            townsmen_message_item.AddMember("create_time",rapidjson::Value().SetString(create_time.c_str(),document.GetAllocator()),document.GetAllocator());

            townsmen_messages.PushBack(townsmen_message_item,document.GetAllocator());

        }
        if(!getIn)
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        mysql_free_result(res);
        mysql_close(&mysql);


        ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
        ans.AddMember("townsmen_messages",townsmen_messages,document.GetAllocator());
        return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");

    }
    else{
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
}

void read_townsmen_message(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("error");
    }
    if(request.getMethod()=="GET"){
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        unordered_map<string,string> data=request.getData();
        if(data.find("townsmen_message_id")==data.end())
            return HttpResponseDecrorate::response("error");
        string query="";
        query = query+"update townsmen_message set readed=1 where id="+data["townsmen_message_id"]+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("ok");
    }
    else{
        return HttpResponseDecrorate::response("error");
    }
}

void get_user_unread_repto(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
    if(request.getMethod()=="GET"){
        rapidjson::Value ans(rapidjson::kObjectType);
        rapidjson::Value unread_repto(rapidjson::kArrayType);
        rapidjson::Document document;


        MYSQL mysql;
        MYSQL_RES *res;
        MYSQL_ROW row;
        int t;
        mysql_init(&mysql);
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        string query="";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t)
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        query=query+"select repto_message.id,repto_message.forum_id,repto_message.layer_id,alumnus.username from repto_message,alumnus where repto_message.repto_user_id=alumnus.id and readed=0 and belong="+
                user_id+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t)
            return HttpResponseDecrorate::response(query);
        res=mysql_store_result(&mysql);
        bool getIn=false;
        string unread_repto_id,forum_id,layer_id,repto_user;
        while(row=mysql_fetch_row(res)){
            getIn=true;

            rapidjson::Value unread_repto_item(rapidjson::kObjectType);

            unread_repto_id=row[0];
            forum_id=row[1];
            layer_id=row[2];
            repto_user=row[3];

            unread_repto_item.AddMember("unread_repto_id",rapidjson::Value().SetString(unread_repto_id.c_str(),document.GetAllocator()),document.GetAllocator());
            unread_repto_item.AddMember("forum_id",rapidjson::Value().SetString(forum_id.c_str(),document.GetAllocator()),document.GetAllocator());
            unread_repto_item.AddMember("layer_id",rapidjson::Value().SetString(layer_id.c_str(),document.GetAllocator()),document.GetAllocator());
            unread_repto_item.AddMember("repto_user",rapidjson::Value().SetString(repto_user.c_str(),document.GetAllocator()),document.GetAllocator());



            unread_repto.PushBack(unread_repto_item,document.GetAllocator());

        }
        if(!getIn)
            return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
        mysql_free_result(res);
        mysql_close(&mysql);


        ans.AddMember("status",rapidjson::Value().SetString("ok"),document.GetAllocator());
        ans.AddMember("unread_repto",unread_repto,document.GetAllocator());
        return HttpResponseDecrorate::response(StringAlgorithm::getStringAlgorithm()->GbkToUtf(JsonToString(ans).c_str()),"application/json");

    }
    else{
        return HttpResponseDecrorate::response("{\"status\":\"error\"}","application/json");
    }
}


void read_repto(Request request){
    cookies c;
    string user_id;
    if((user_id=c.get_cookie("UserId"))==""){
        return HttpResponseDecrorate::response("error");
    }
    if(request.getMethod()=="GET"){
        MYSQL mysql;
        mysql_init(&mysql);
        int t;
        mysql_real_connect(&mysql,"localhost", "root", "","mytest",3306,NULL,0);
        string encoding="SET CHARACTER SET GBK";
        t=mysql_real_query(&mysql,encoding.c_str(),encoding.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        unordered_map<string,string> data=request.getData();
        if(data.find("unread_repto_id")==data.end())
            return HttpResponseDecrorate::response("error");
        string query="";
        query = query+"update repto_message set readed=1 where id="+data["unread_repto_id"]+";";
        t=mysql_real_query(&mysql,query.c_str(),query.size());
        if(t){
            return HttpResponseDecrorate::response("error");
        }
        mysql_close(&mysql);
        return HttpResponseDecrorate::response("ok");
    }
    else{
        return HttpResponseDecrorate::response("error");
    }
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
		{ "/gtest",getTest },
		{"/register",reg},
		{"/login",login},
		{"/logout",logout},
		{"/message",message},
		{"/townsmen",Townsmen},
		{"/all_forum",all_forum},
		{"/forum",forum},
		{"/repto",repto},
		{"/townsmen_send",townsmen_send},
		{"/user_admin",user_admin},
		{"/read_townsmen_message",read_townsmen_message},
		{"/read_repto_comments",read_repto},


		//-------------------------modify--------------
		{"/modify/message",modifyMessage},
		{"/modify/forum",modify_forum},
		{"/modify/comments",modify_comments},


        //--------------------api-----------------------
		{"/api",json },
		{"/api/province",getProvince},
		{"/api/userinfo",getUserInfo},
		{"/api/allmessage",getAllMessage},
		{"/api/townsmen_detail",getTownsmenDetail},
		{"/api/all_forum",getAllForum},
		{"/api/forum",getForumData},
		{"/api/unread_repto",get_user_unread_repto},
		{"/api/townsmen_message",get_unread_townsmen_message}
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
