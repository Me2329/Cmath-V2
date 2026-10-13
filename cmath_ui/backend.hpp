#ifndef CMATH_UI_BACKEND_HPP
#define CMATH_UI_BACKEND_HPP
#include "../cmath_v2_expression.hpp"
#include <map>
#include <variant>
#include <sstream>
#include <iomanip>
#include <locale>

namespace math::ui_detail {
inline std::string quote(const std::string& text, bool html_safe = true) {
    std::string out="\"";
    const char* hex="0123456789abcdef";
    for(unsigned char c:text) {
        if(c=='"' || c=='\\') { out+='\\'; out+=static_cast<char>(c); }
        else if(c=='\n') out+="\\n";
        else if(c=='\r') out+="\\r";
        else if(c=='\t') out+="\\t";
        else if(c<32 || (html_safe && (c=='<' || c=='>' || c=='&'))) {
            out+="\\u00";out+=hex[c>>4];out+=hex[c&15];
        } else out+=static_cast<char>(c);
    }
    return out+'"';
}
inline std::string num(double x) {
    if(!std::isfinite(x)) throw std::domain_error("Result is outside finite numeric range.");
    std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(16)<<x;return out.str();
}
struct Json {
    using Array=std::vector<Json>;using Object=std::map<std::string,Json>;
    std::variant<std::nullptr_t,bool,double,std::string,Array,Object> value=nullptr;
    Json()=default;Json(std::nullptr_t):value(nullptr){} Json(bool v):value(v){} Json(double v):value(v){}
    Json(int v):value(static_cast<double>(v)){} Json(std::size_t v):value(static_cast<double>(v)){}
    Json(const char* v):value(std::string(v)){} Json(std::string v):value(std::move(v)){}
    static Json array(Array a={}) {Json j;j.value=std::move(a);return j;}
    static Json object(Object o={}) {Json j;j.value=std::move(o);return j;}
    Json& operator[](const std::string& key){return std::get<Object>(value)[key];}
    void push(Json j){std::get<Array>(value).push_back(std::move(j));}
    std::string dump() const {
        return std::visit([](const auto& v)->std::string{
            using T=std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T,std::nullptr_t>) return "null";
            else if constexpr(std::is_same_v<T,bool>) return v?"true":"false";
            else if constexpr(std::is_same_v<T,double>) return num(v);
            else if constexpr(std::is_same_v<T,std::string>) return quote(v);
            else if constexpr(std::is_same_v<T,Array>) {std::string s="[";for(const auto& x:v){if(s.size()>1)s+=',';s+=x.dump();}return s+"]";}
            else {std::string s="{";for(const auto& [k,x]:v){if(s.size()>1)s+=',';s+=quote(k)+":"+x.dump();}return s+"}";}
        },value);
    }
};
using Params=std::map<std::string,std::string>;
inline std::string field(const Params& p,const std::string& key,const std::string& fallback="") {
    const auto it=p.find(key);return it==p.end()?fallback:it->second;
}
inline double number(const Params& p,const std::string& key,const std::string& fallback="0") {
    try{return math::evaluate(field(p,key,fallback));}
    catch(const std::exception& e){throw std::invalid_argument(key+": "+e.what());}
}
inline Json metric(std::string label,Json value,std::string unit="") {
    return Json::object({{"label",std::move(label)},{"value",std::move(value)},{"unit",std::move(unit)}});
}
inline Json response(std::string title) {
    return Json::object({{"title",std::move(title)},{"metrics",Json::array()},{"steps",Json::array()},{"cpp",""}});
}
template<class F>
inline Json sample(const F& f,double a,double b,std::size_t count=401) {
    if(!std::isfinite(a)||!std::isfinite(b)||a>=b) throw std::domain_error("Graph bounds must be finite with minimum < maximum.");
    Json points=Json::array();
    for(std::size_t i=0;i<count;++i) {
        const double x=std::lerp(a,b,static_cast<double>(i)/static_cast<double>(count-1));
        try {points.push(Json::array({x,f(x)}));} catch(const std::domain_error&){points.push(Json::array({x,nullptr}));}
    }
    std::string expression="C++ function";
    if constexpr(requires { f.source(); }) expression=f.source();
    return Json::object({{"points",std::move(points)},{"xmin",a},{"xmax",b},{"expression",expression}});
}
inline std::vector<double> dataset(std::string input) {
    for(char& c:input) if(c==','||c==';')c=' ';
    std::istringstream in(input);in.imbue(std::locale::classic());
    std::vector<double> data;double x=0;
    while(in>>x) {if(!std::isfinite(x))throw std::domain_error("Data must be finite.");data.push_back(x);if(data.size()>1000)throw std::domain_error("Use at most 1000 data values.");}
    if(!in.eof())throw std::invalid_argument("Use numbers separated by commas, spaces, semicolons, or newlines.");
    if(data.empty())throw std::invalid_argument("Enter at least one data value.");
    return data;
}
inline Json calculate(const Params& p) {
    const std::string tool=field(p,"tool","calculator");
    Json out=response("Result");auto& metrics=out["metrics"];auto& steps=out["steps"];
    if(tool=="calculator") {
        const auto expression=field(p,"expression","sin_deg(30) + sqrt(25)");
        const double x=number(p,"x");math::Expression f(expression);
        out["title"]="Expression evaluated";metrics.push(metric("Result",f(x)));
        steps.push("Parse parentheses, functions, and operator precedence.");
        steps.push("Substitute x = "+num(x)+"; constants pi, tau, e and phi are available.");
        steps.push("Evaluate with the real C++ math engine. Trigonometric functions use radians unless their name ends in _deg.");
        out["cpp"]="#include \"cmath_v2_expression.hpp\"\n\nauto result = math::evaluate("+quote(expression,false)+", "+num(x)+");";
    } else if(tool=="quadratic") {
        const double a=number(p,"a","1"),b=number(p,"b","-5"),c=number(p,"c","6");
        const auto roots=math::solve_quadratic(a,b,c);out["title"]="Quadratic equation";
        std::size_t i=0;for(const auto& r:roots) {
            std::string text=num(r.real());if(r.imag()!=0)text+=(r.imag()<0?" - ":" + ")+num(std::abs(r.imag()))+"i";
            metrics.push(metric("Root "+std::to_string(++i),text));
        }
        if(roots.empty())metrics.push(metric("Solution","No solutions"));
        steps.push(num(a)+"x² + ("+num(b)+")x + ("+num(c)+") = 0");
        steps.push(a==0?"The quadratic term is zero; solve the remaining linear or constant equation.":"Use discriminant b² − 4ac to distinguish real and complex roots.");
        steps.push("The C++ solver scales by powers of two and uses a stable formula to reduce cancellation.");
        out["cpp"]="auto roots = math::solve_quadratic("+num(a)+", "+num(b)+", "+num(c)+");\nfor (auto root : roots) std::cout << root << '\\n';";
        math::Expression f(num(a)+"*x^2+("+num(b)+")*x+("+num(c)+")");out["plot"]=sample(f,-10,10);
    } else if(tool=="linear") {
        const double a=number(p,"a","2"),b=number(p,"b","1"),c=number(p,"c","7"),d=number(p,"d","1"),e=number(p,"e","-1"),f=number(p,"f","2");
        const auto r=math::solve_linear2(a,b,c,d,e,f);out["title"]="Two simultaneous equations";
        metrics.push(metric("x",r.x));metrics.push(metric("y",r.y));
        steps.push(num(a)+"x + ("+num(b)+")y = "+num(c));steps.push(num(d)+"x + ("+num(e)+")y = "+num(f));
        steps.push("Solve the 2×2 coefficient system using scaled determinants. A zero determinant has no unique solution.");
        out["cpp"]="auto solution = math::solve_linear2("+num(a)+", "+num(b)+", "+num(c)+", "+num(d)+", "+num(e)+", "+num(f)+");\nstd::cout << solution.x << ' ' << solution.y;";
    } else if(tool=="geometry") {
        const std::string shape=field(p,"shape","circle");
        const double a=number(p,"a","3"),b=number(p,"b","4"),c=number(p,"c","5");
        if(a<=0 || (shape!="circle"&&shape!="sphere"&&b<=0))throw std::domain_error("Shape dimensions must be positive.");
        out["title"]="Geometry · "+shape;out["shape"]=Json::object({{"kind",shape},{"a",a},{"b",b},{"c",c}});
        if(shape=="circle") {
            metrics.push(metric("Area",math::circle_area(a),"units²"));metrics.push(metric("Circumference",math::circle_circumference(a),"units"));
            steps.push("A = πr² = π × "+num(a)+"²");steps.push("C = 2πr");out["cpp"]="auto area = math::circle_area("+num(a)+");\nauto circumference = math::circle_circumference("+num(a)+");";
        } else if(shape=="rectangle") {
            metrics.push(metric("Area",math::rectangle_area(a,b),"units²"));metrics.push(metric("Perimeter",math::detail::finite_result(2.0L*(static_cast<long double>(a)+b),"Perimeter overflow"),"units"));
            steps.push("A = width × height");steps.push("P = 2 × (width + height)");out["cpp"]="auto area = math::rectangle_area("+num(a)+", "+num(b)+");";
        } else if(shape=="triangle") {
            metrics.push(metric("Area",math::triangle_area(a,b,c),"units²"));metrics.push(metric("Perimeter",math::detail::finite_result(static_cast<long double>(a)+b+c,"Perimeter overflow"),"units"));
            steps.push("Check the triangle inequality: any two sides must exceed the third.");steps.push("Use a rearranged Heron formula to reduce cancellation for thin triangles.");out["cpp"]="auto area = math::triangle_area("+num(a)+", "+num(b)+", "+num(c)+");";
        } else if(shape=="sphere") {
            metrics.push(metric("Volume",math::sphere_volume(a),"units³"));metrics.push(metric("Surface area",math::sphere_surface_area(a),"units²"));
            steps.push("V = 4πr³ / 3");steps.push("A = 4πr²");out["cpp"]="auto volume = math::sphere_volume("+num(a)+");\nauto surface = math::sphere_surface_area("+num(a)+");";
        } else if(shape=="cylinder") {
            metrics.push(metric("Volume",math::cylinder_volume(a,b),"units³"));
            metrics.push(metric("Surface area",math::detail::finite_result(2*std::numbers::pi_v<long double>*a*(static_cast<long double>(a)+b),"Surface overflow"),"units²"));
            steps.push("V = πr²h");steps.push("A = 2πr(r + h)");out["cpp"]="auto volume = math::cylinder_volume("+num(a)+", "+num(b)+");";
        } else throw std::invalid_argument("Unknown shape.");
    } else if(tool=="statistics") {
        const auto data=dataset(field(p,"data"));const bool sample_stats=field(p,"sample")=="true";
        const auto s=math::describe(data,sample_stats);out["title"]="Dataset summary";
        metrics.push(metric("Count",s.count));metrics.push(metric("Mean",s.mean));metrics.push(metric("Median",s.median));metrics.push(metric("Standard deviation",s.stddev));
        metrics.push(metric("Minimum",s.min));metrics.push(metric("Maximum",s.max));metrics.push(metric("Variance",s.variance));metrics.push(metric("Sum",s.sum));
        metrics.push(metric("25th percentile",math::percentile(data,25)));metrics.push(metric("75th percentile",math::percentile(data,75)));
        steps.push(sample_stats?"Sample variance uses n − 1 in the denominator.":"Population variance uses n in the denominator.");
        steps.push("Compensated sums and centered variance updates reduce floating-point precision loss.");steps.push("Percentiles linearly interpolate sorted positions q × (n − 1).");
        Json values=Json::array();std::string list;for(double v:data){values.push(v);if(!list.empty())list+=", ";list+=num(v);}out["data"]=std::move(values);
        out["cpp"]="std::vector<double> data{"+list+"};\nauto stats = math::describe(data, "+(sample_stats?"true":"false")+");\nstd::cout << stats.mean << '\\n';";
    } else if(tool=="functions") {
        const std::string expression=field(p,"expression","sin(x)");math::Expression f(expression);
        const double a=number(p,"xmin","-6.283185307179586"),b=number(p,"xmax","6.283185307179586"),x=number(p,"x","1");
        const std::string operation=field(p,"operation","plot");out["title"]="Function explorer";
        out["plot"]=sample(f,a,b);out["cpp"]="math::Expression f("+quote(expression,false)+");\n";
        auto& code=std::get<std::string>(out["cpp"].value);
        if(operation=="plot") {metrics.push(metric("f(x) at x = "+num(x),f(x)));code+="std::cout << f("+num(x)+");";steps.push("The graph samples 401 points. Gaps mark points outside the real domain; finite sampling can miss narrow features or discontinuities.");}
        else if(operation=="derivative") {metrics.push(metric("Derivative at x = "+num(x),math::derivative(f,x)));code+="auto slope = math::derivative(f, "+num(x)+");";steps.push("Approximate the slope using central differences with an automatic step.");}
        else if(operation=="integral") {
            const auto r=math::integrate_adaptive(f,a,b,1e-8,16);metrics.push(metric("Definite integral",r.value));metrics.push(metric("Estimated error",r.estimated_error));metrics.push(metric("Function evaluations",r.evaluations));out["plot"]["fill"]=true;
            code+="auto area = math::integrate_adaptive(f, "+num(a)+", "+num(b)+");\nstd::cout << area.value;";steps.push("Adaptively subdivide the interval using Simpson estimates. The error estimate is not a rigorous bound; use a smooth function.");
        } else if(operation=="root") {
            const double r=math::find_root(f,a,b);metrics.push(metric("Root x",r));metrics.push(metric("f(root)",f(r)));out["plot"]["root"]=r;
            code+="auto root = math::find_root(f, "+num(a)+", "+num(b)+");";steps.push("Bisect a bracket with opposite endpoint signs. The function must be continuous across the interval.");
        } else throw std::invalid_argument("Unknown function operation.");
    } else throw std::invalid_argument("Unknown tool.");
    return out;
}
} // namespace math::ui_detail
#endif
