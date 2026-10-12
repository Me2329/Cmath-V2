#ifndef CMATH_V2_UI_HPP
#define CMATH_V2_UI_HPP
#include "cmath_ui/backend.hpp"
#include "cmath_ui/assets.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <cstring>
#include <cctype>
#include <cerrno>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#ifdef _MSC_VER
#pragma comment(lib,"Ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
extern char** environ;
#endif

namespace math {
enum class Platform { Auto, Linux, Windows, macOS };
enum class WindowMode { Browser, Native };
struct WindowOptions {
    unsigned short port = 0; // zero asks the OS for an unused loopback port
    bool open = true;       // false starts the engine and prints its URL only
    std::string python;    // optional interpreter for the native Qt host
    std::filesystem::path native_host; // optional explicit path to native.py
};
class UI {
public:
    struct Function {std::string label,code;std::function<double(double)> callback;double xmin,xmax;};
    std::vector<std::string> tools;
    ui_detail::Json cards=ui_detail::Json::array();
    std::vector<Function> registered_functions;
    UI& enable(const std::string& name){if(std::find(tools.begin(),tools.end(),name)==tools.end())tools.push_back(name);return *this;}
    UI& calculator(){return enable("calculator");} UI& algebra(){return enable("algebra");}
    UI& geometry(){return enable("geometry");} UI& statistics(){return enable("statistics");}
    UI& functions(){return enable("functions");}
    UI& all(){calculator();algebra();geometry();statistics();functions();return *this;}
    UI& value(std::string label,double result,std::string code="") {
        detail::require_finite(result,"UI value must be finite");
        cards.push(ui_detail::Json::object({{"label",std::move(label)},{"value",result},{"code",std::move(code)}}));return *this;
    }
    UI& text(std::string label,std::string content){cards.push(ui_detail::Json::object({{"label",std::move(label)},{"value",std::move(content)},{"code",""}}));return *this;}
    UI& expression(std::string label,std::string formula,double x=0){
        calculator();cards.push(ui_detail::Json::object({{"label",std::move(label)},{"value",math::evaluate(formula,x)},
            {"code","math::evaluate("+ui_detail::quote(formula,false)+", "+ui_detail::num(x)+")"},{"tool","calculator"},
            {"params",ui_detail::Json::object({{"expression",formula},{"x",ui_detail::num(x)}})}}));return *this;
    }
    UI& circle(std::string label,double radius){
        geometry();cards.push(ui_detail::Json::object({{"label",std::move(label)},{"value",math::circle_area(radius)},
            {"code","math::circle_area("+ui_detail::num(radius)+")"},{"tool","geometry"},
            {"params",ui_detail::Json::object({{"shape","circle"},{"a",ui_detail::num(radius)}})}}));return *this;
    }
    UI& quadratic(std::string label,double a,double b,double c){
        algebra();const auto roots=math::solve_quadratic(a,b,c);std::ostringstream summary;
        for(std::size_t i=0;i<roots.size();++i){if(i)summary<<", ";summary<<roots[i];}
        cards.push(ui_detail::Json::object({{"label",std::move(label)},{"value",summary.str()},{"tool","algebra"},
            {"code","math::solve_quadratic("+ui_detail::num(a)+", "+ui_detail::num(b)+", "+ui_detail::num(c)+")"},
            {"params",ui_detail::Json::object({{"mode","quadratic"},{"a",ui_detail::num(a)},{"b",ui_detail::num(b)},{"c",ui_detail::num(c)}})}}));return *this;
    }
    template<class F> UI& function(std::string label,F callback,double xmin=-10,double xmax=10,std::string code="") {
        if(!std::isfinite(xmin)||!std::isfinite(xmax)||xmin>=xmax)throw std::invalid_argument("Function bounds need min < max and finite values");
        if(registered_functions.size()>=32)throw std::invalid_argument("Register at most 32 functions per window");
        functions();registered_functions.push_back({std::move(label),std::move(code),std::function<double(double)>(std::move(callback)),xmin,xmax});return *this;
    }
    UI& plot(std::string label,std::string formula,double xmin=-10,double xmax=10){
        const std::string code="math::Expression f("+ui_detail::quote(formula,false)+");";
        return function(std::move(label),math::Expression(std::move(formula)),xmin,xmax,code);
    }
};
namespace ui_detail {
inline Platform host_platform(){
#if defined(_WIN32)
 return Platform::Windows;
#elif defined(__APPLE__)
 return Platform::macOS;
#else
 return Platform::Linux;
#endif
}
#if defined(_WIN32)
using Socket=SOCKET;inline constexpr Socket invalid_socket=INVALID_SOCKET;
inline void close_socket(Socket s){closesocket(s);}
struct Network {Network(){WSADATA d;if(WSAStartup(MAKEWORD(2,2),&d)!=0)throw std::runtime_error("Could not initialize Winsock");}~Network(){WSACleanup();}};
using Child=std::intptr_t;
inline Child launch(const std::vector<std::string>& args){std::vector<const char*> pointers;for(const auto&s:args)pointers.push_back(s.c_str());pointers.push_back(nullptr);const auto pid=_spawnvp(_P_NOWAIT,pointers[0],pointers.data());if(pid==-1)throw std::runtime_error("Could not start "+args[0]);return pid;}
inline bool child_exited(Child child){return WaitForSingleObject(reinterpret_cast<HANDLE>(child),0)==WAIT_OBJECT_0;}
#else
using Socket=int;inline constexpr Socket invalid_socket=-1;
inline void close_socket(Socket s){::close(s);}
struct Network {};
using Child=pid_t;
inline Child launch(const std::vector<std::string>& args){std::vector<char*> pointers;for(const auto&s:args)pointers.push_back(const_cast<char*>(s.c_str()));pointers.push_back(nullptr);pid_t child=0;const int error=posix_spawnp(&child,pointers[0],nullptr,nullptr,pointers.data(),environ);if(error)throw std::runtime_error("Could not start "+args[0]+": "+std::strerror(error));return child;}
inline bool child_exited(Child child){int status=0;return waitpid(child,&status,WNOHANG)==child;}
#endif
struct OwnedSocket {
    Socket socket=invalid_socket;
    explicit OwnedSocket(Socket s):socket(s){}
    ~OwnedSocket(){if(socket!=invalid_socket)close_socket(socket);}
    OwnedSocket(const OwnedSocket&)=delete;OwnedSocket& operator=(const OwnedSocket&)=delete;
};
inline std::string secret(){std::random_device rng;const char*hex="0123456789abcdef";std::string s;for(int i=0;i<32;++i)s+=hex[rng()&15u];return s;}
inline std::string decode(std::string_view value){
    const auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
    std::string out;
    for(std::size_t i=0;i<value.size();++i){char c=value[i];if(c=='+')out+=' ';else if(c=='%'){if(i+2>=value.size()||digit(value[i+1])<0||digit(value[i+2])<0)throw std::invalid_argument("Invalid form encoding");const int decoded=digit(value[i+1])*16+digit(value[i+2]);if(decoded==0)throw std::invalid_argument("NUL characters are not accepted");out+=static_cast<char>(decoded);i+=2;}else out+=c;}return out;
}
inline Params form(std::string_view text){Params p;std::size_t pos=0;while(pos<text.size()){auto end=text.find('&',pos);if(end==std::string_view::npos)end=text.size();auto part=text.substr(pos,end-pos);auto equals=part.find('=');if(equals==std::string_view::npos)throw std::invalid_argument("Invalid form field");const auto key=decode(part.substr(0,equals));if(p.contains(key))throw std::invalid_argument("Duplicate form field");p.emplace(key,decode(part.substr(equals+1)));if(p.size()>32)throw std::invalid_argument("Too many fields");pos=end+1;}return p;}
struct Request{std::string method,path,body;std::map<std::string,std::string> headers;};
inline Request read_request(Socket socket){
    std::string data;char buffer[4096];std::size_t end=std::string::npos;
    while((end=data.find("\r\n\r\n"))==std::string::npos){const auto n=recv(socket,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("Incomplete HTTP request");data.append(buffer,static_cast<std::size_t>(n));if(data.size()>16384)throw std::invalid_argument("Request headers too large");}
    Request r;std::istringstream lines(data.substr(0,end));std::string line,version;
    std::getline(lines,line);std::istringstream first(line);if(!(first>>r.method>>r.path>>version)||version.rfind("HTTP/1.",0)!=0)throw std::invalid_argument("Invalid HTTP request");
    while(std::getline(lines,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto colon=line.find(':');if(colon==std::string::npos)throw std::invalid_argument("Invalid HTTP header");auto key=line.substr(0,colon),value=line.substr(colon+1);for(char&c:key)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));while(!value.empty()&&value.front()==' ')value.erase(0,1);if(r.headers.contains(key))throw std::invalid_argument("Duplicate header");r.headers.emplace(std::move(key),std::move(value));}
    if(r.headers.contains("transfer-encoding"))throw std::invalid_argument("Chunked requests are not supported");
    std::size_t length=0;if(r.headers.contains("content-length")){const auto&s=r.headers.at("content-length");auto parsed=std::from_chars(s.data(),s.data()+s.size(),length);if(parsed.ec!=std::errc{}||parsed.ptr!=s.data()+s.size())throw std::invalid_argument("Invalid content length");}
    if(length>65536)throw std::invalid_argument("Request body exceeds 64 KB");
    r.body=data.substr(end+4);while(r.body.size()<length){const auto n=recv(socket,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("Incomplete request body");r.body.append(buffer,static_cast<std::size_t>(n));}r.body.resize(length);return r;
}
inline void send_response(Socket socket,int status,const std::string& body,const std::string& type="application/json"){
    std::string data="HTTP/1.1 "+std::to_string(status)+(status==200?" OK\r\n":" Error\r\n");
    data+="Content-Type: "+type+"; charset=utf-8\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'unsafe-inline'; img-src 'self' data: blob:; connect-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'none'\r\n\r\n"+body;
    std::size_t sent=0;while(sent<data.size()){
#ifdef MSG_NOSIGNAL
        constexpr int flags=MSG_NOSIGNAL;
#else
        constexpr int flags=0;
#endif
        const auto n=send(socket,data.data()+sent,static_cast<int>(std::min<std::size_t>(data.size()-sent,65536)),flags);if(n<=0)break;sent+=static_cast<std::size_t>(n);
    }
}
inline Json configuration(const std::string& title,const UI& ui){
    Json enabled=Json::array(),functions=Json::array();for(const auto&t:ui.tools)enabled.push(t);
    for(const auto&f:ui.registered_functions)functions.push(Json::object({{"label",f.label},{"code",f.code},{"xmin",f.xmin},{"xmax",f.xmax}}));
    return Json::object({{"title",title},{"version",math::version},{"tools",std::move(enabled)},{"cards",ui.cards},{"functions",std::move(functions)}});
}
inline Json custom_result(const Params& p,const UI& ui){
    const auto text=field(p,"custom");std::size_t index=0;auto parsed=std::from_chars(text.data(),text.data()+text.size(),index);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||index>=ui.registered_functions.size())throw std::invalid_argument("Unknown C++ function");
    const auto&f=ui.registered_functions[index];const auto evaluate=[&](double x){const double v=f.callback(x);detail::require_finite(v,"C++ function returned a nonfinite value");return v;};
    const double a=number(p,"xmin",num(f.xmin)),b=number(p,"xmax",num(f.xmax)),x=number(p,"x","1");
    auto out=response(f.label);out["plot"]=sample(evaluate,a,b);out["plot"]["expression"]=f.label;
    auto& metrics=out["metrics"];const auto op=field(p,"operation","plot");
    std::string code=f.code.empty()?"// Use the same callable you registered with ui.function().\n":f.code+"\n";
    if(op=="plot"){metrics.push(metric("f(x) at x = "+num(x),evaluate(x)));code+="// Evaluate your callable at x = "+num(x)+".";}
    else if(op=="derivative"){metrics.push(metric("Derivative",math::derivative(evaluate,x)));code+="auto slope = math::derivative(f, "+num(x)+");";}
    else if(op=="integral"){const auto r=math::integrate_adaptive(evaluate,a,b,1e-8,16);metrics.push(metric("Integral",r.value));metrics.push(metric("Estimated error",r.estimated_error));metrics.push(metric("Evaluations",r.evaluations));out["plot"]["fill"]=true;code+="auto area = math::integrate_adaptive(f, "+num(a)+", "+num(b)+");";}
    else if(op=="root"){const auto r=math::find_root(evaluate,a,b);metrics.push(metric("Root x",r));out["plot"]["root"]=r;code+="auto root = math::find_root(f, "+num(a)+", "+num(b)+");";}
    else throw std::invalid_argument("Unknown operation");
    out["cpp"]=code;out["steps"].push("The registered C++ callable runs in the C++ process. Its captures must remain valid while this window is open.");
    out["steps"].push("Graph sampling and calculus are numerical approximations; use smooth deterministic functions.");return out;
}
inline int run_window(std::string title,Platform target,WindowMode mode,UI ui,WindowOptions options){
    if(target!=Platform::Auto&&target!=host_platform())throw std::invalid_argument("Platform does not match this computer. The platform parameter does not cross-compile a program.");
    if(title.empty()||title.size()>160)throw std::invalid_argument("Window title must have 1 to 160 characters");
    if(ui.tools.empty())ui.all();
    [[maybe_unused]] Network network;
    OwnedSocket listener(::socket(AF_INET,SOCK_STREAM,0));if(listener.socket==invalid_socket)throw std::runtime_error("Could not create local socket");
#if !defined(_WIN32)
    fcntl(listener.socket,F_SETFD,FD_CLOEXEC);
#endif
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(options.port);
    if(bind(listener.socket,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||listen(listener.socket,8)!=0)throw std::runtime_error("Could not listen on the requested local port");
#if defined(_WIN32)
    int size=sizeof(address);
#else
    socklen_t size=sizeof(address);
#endif
    if(getsockname(listener.socket,reinterpret_cast<sockaddr*>(&address),&size)!=0)throw std::runtime_error("Could not query local port");
    const std::string host="127.0.0.1:"+std::to_string(ntohs(address.sin_port)),origin="http://"+host,token=secret(),path="/"+token,url=origin+path+"/";
    const std::string config=configuration(title,ui).dump();
    std::cout<<"\ncmath_v2 UI: "<<title<<"\n"<<url<<"\nUse Close in the window, or Ctrl+C here, to stop the engine.\n"<<std::flush;
    Child child=0;bool monitor=false;
    if(options.open){
        if(mode==WindowMode::Native){
            auto native=options.native_host.empty()?std::filesystem::path(__FILE__).parent_path()/"cmath_ui"/"native.py":options.native_host;
            if(!std::filesystem::is_regular_file(native))throw std::runtime_error("Native host not found. Keep cmath_ui/native.py beside the UI header or set WindowOptions.native_host.");
            auto python=options.python.empty()?(host_platform()==Platform::Windows?"python":"python3"):options.python;
            child=launch({python,std::filesystem::absolute(native).string(),url,title});monitor=true;
        }else{
#if defined(_WIN32)
            child=launch({"rundll32.exe","url.dll,FileProtocolHandler",url});
#elif defined(__APPLE__)
            child=launch({"open",url});
#else
            child=launch({"xdg-open",url});
#endif
        }
    }
    bool running=true,browser_reaped=false;
    while(running){
        if(monitor&&child_exited(child)){std::cout<<"Native window closed.\n";break;}
        if(!monitor&&child!=0&&!browser_reaped)browser_reaped=child_exited(child);
        fd_set ready;FD_ZERO(&ready);FD_SET(listener.socket,&ready);timeval wait{0,200000};
        const int selected=select(static_cast<int>(listener.socket+1),&ready,nullptr,nullptr,&wait);if(selected<=0)continue;
        OwnedSocket client(accept(listener.socket,nullptr,nullptr));if(client.socket==invalid_socket)continue;
#if defined(_WIN32)
        DWORD timeout=5000;setsockopt(client.socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));setsockopt(client.socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
#else
        fcntl(client.socket,F_SETFD,FD_CLOEXEC);timeval timeout{5,0};setsockopt(client.socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(client.socket,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
#ifdef SO_NOSIGPIPE
        int no_sigpipe=1;setsockopt(client.socket,SOL_SOCKET,SO_NOSIGPIPE,&no_sigpipe,sizeof(no_sigpipe));
#endif
#endif
        try{
            auto r=read_request(client.socket);
            if(field(r.headers,"host")!=host||(r.headers.contains("origin")&&r.headers.at("origin")!=origin)){send_response(client.socket,403,"{\"error\":\"Origin not allowed\"}");continue;}
            if(r.method=="GET"&&r.path==path+"/"){send_response(client.socket,200,html,"text/html");continue;}
            if(r.method=="GET"&&r.path==path+"/config"){send_response(client.socket,200,config);continue;}
            if(r.method!="POST"||field(r.headers,"x-math-token")!=token){send_response(client.socket,404,"{\"error\":\"Not found\"}");continue;}
            if(r.path==path+"/shutdown"){send_response(client.socket,200,"{\"ok\":true}");running=false;continue;}
            if(r.path!=path+"/api"){send_response(client.socket,404,"{\"error\":\"Not found\"}");continue;}
            if(field(r.headers,"content-type").rfind("application/x-www-form-urlencoded",0)!=0)throw std::invalid_argument("Expected form-encoded request");
            auto params=form(r.body);const auto tool=field(params,"tool");std::string module=tool;
            if(tool=="quadratic"||tool=="linear") module="algebra";
            if(tool=="custom") module="functions";
            if(std::find(ui.tools.begin(),ui.tools.end(),module)==ui.tools.end())throw std::invalid_argument("This tool is not enabled in this window");
            auto result=tool=="custom"?custom_result(params,ui):calculate(params);send_response(client.socket,200,result.dump());
        }catch(const std::exception& e){send_response(client.socket,400,Json::object({{"error",e.what()}}).dump());}
    }
    std::cout<<"Math workspace stopped.\n";return 0;
}
} // namespace ui_detail

template<class Setup> int ui_window(std::string title,Platform platform,WindowMode mode,Setup&& configure,WindowOptions options={}) {
    UI ui;std::invoke(std::forward<Setup>(configure),ui);return ui_detail::run_window(std::move(title),platform,mode,std::move(ui),std::move(options));
}
template<class Setup> int browser_window(std::string title,Platform platform,Setup&& configure,WindowOptions options={}) {
    return ui_window(std::move(title),platform,WindowMode::Browser,std::forward<Setup>(configure),std::move(options));
}
template<class Setup> int window(std::string title,Platform platform,Setup&& configure,WindowOptions options={}) {
    return ui_window(std::move(title),platform,WindowMode::Native,std::forward<Setup>(configure),std::move(options));
}
inline int browser_window(std::string title,Platform platform=Platform::Auto){return browser_window(std::move(title),platform,[](UI&ui){ui.all();});}
inline int window(std::string title,Platform platform=Platform::Auto){return window(std::move(title),platform,[](UI&ui){ui.all();});}
} // namespace math
#endif
