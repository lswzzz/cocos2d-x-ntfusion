//
//  errorReportThread.cpp
//  cocos2d_js_bindings
//
//  Created by gc_lsw on 5/19/17.
//
//

#include "ErrorReportThread.hpp"
#include "platform/CCFileUtils.h"
#include "deprecated/CCDictionary.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"
#include "ScriptingCore.h"

using namespace  rapidjson;

std::string parameter;
std::string url;
std::vector<std::string> expectList;
std::string header = "Content-type: application/x-www-form-urlencoded;charset=UTF-8";
size_t sentdelay = 100;
size_t loopdelay = 100;
std::string filename;
unsigned int line;
std::string message;

string&   replace_all(string&   str,const   string&   old_value,const   string&   new_value)
{
    while(true)   {
        string::size_type   pos(0);
        if(   (pos=str.find(old_value))!=string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }
    return   str;
}

ErrorReportThread::ErrorReportThread()
{
    Node::init();
    stop = false;
    _callback = nullptr;
    std::thread t1(&ErrorReportThread::update, this);
    _thread = &t1;
    _thread->detach();
}

void ErrorReportThread::registerJSFunction(JSContext * cx, JS::HandleObject global)
{
    JS_DefineFunction(cx, global, "setLogThreadParameter", ErrorReportThread::setParameter,1, JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineFunction(cx, global, "setLogThreadUrl", ErrorReportThread::setUrl,2, JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineFunction(cx, global, "setLogThreadExpectList", ErrorReportThread::setExpectList,2, JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineFunction(cx, global, "setLogThreadSentDelay", ErrorReportThread::setSentDelay,2, JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineFunction(cx, global, "setLogThreadLoopDelay", ErrorReportThread::setLoopDelay,2, JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineFunction(cx, global, "setLogThreadHeader", ErrorReportThread::setHeader,2, JSPROP_READONLY | JSPROP_PERMANENT);
}

bool ErrorReportThread::setParameter(JSContext* cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string parameter_;
        jsval_to_std_string(cx, args.get(0), &parameter_);
        parameter = parameter_.c_str();
    }
    args.rval().setUndefined();
    
    return true;
}

bool ErrorReportThread::setUrl(JSContext* cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string url_;
        jsval_to_std_string(cx, args.get(0), &url_);
        url = url_.c_str();
    }
    args.rval().setUndefined();
    
    return true;
}

bool ErrorReportThread::setExpectList(JSContext *cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string str;
        jsval_to_std_string(cx, args.get(0), &str);
        rapidjson::Document readdoc;
        readdoc.Parse<0>(str.c_str());
        rapidjson::Value &entry = readdoc["expectList"];
        for (int j = 0; j < entry.Size(); j++) {
            std::string instrument = entry[j].GetString();
            expectList.push_back(instrument);
        }
    }
    args.rval().setUndefined();
    
    return true;
}

bool ErrorReportThread::setSentDelay(JSContext *cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string str;
        jsval_to_std_string(cx, args.get(0), &str);
        sentdelay = atoi(str.c_str());
    }
    args.rval().setUndefined();
    
    return true;
}

bool ErrorReportThread::setLoopDelay(JSContext *cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string str;
        jsval_to_std_string(cx, args.get(0), &str);
        loopdelay = atoi(str.c_str());
    }
    args.rval().setUndefined();
    
    return true;
}

bool ErrorReportThread::setHeader(JSContext *cx, uint32_t argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc > 0) {
        std::string str;
        jsval_to_std_string(cx, args.get(0), &str);
        header = str.c_str();
    }
    args.rval().setUndefined();
    
    return true;
}

ErrorReportThread::~ErrorReportThread()
{
    stop = true;
}

void ErrorReportThread::addContext(JSErrorReport* report, const char* message)
{
    ReportContext* context = new ErrorReportThread::ReportContext(report, message);
    mut.lock();
    _queue.push(context);
    mut.unlock();
}

void ErrorReportThread::sendCallback(network::HttpClient *sender, network::HttpResponse *response)
{
}

void ErrorReportThread::update()
{
    while(!stop){
        if(!_queue.empty()){
            mut.lock();
            ReportContext* context = _queue.front();
            _queue.pop();
            mut.unlock();
            size_t size = expectList.size();
            bool contains = false;
            for(int i=0; i<size; i++){
                std::string d = expectList.at(i);
                if(context->_message.find(d) != std::string::npos){
                    contains = true;
                }
            }
            if(!contains && url.size() > 0){
                cocos2d::network::HttpRequest* request = new cocos2d::network::HttpRequest();
                request->setUrl(url.c_str());
                request->setRequestType(cocos2d::network::HttpRequest::Type::POST);
                std::vector<std::string> headers;
                headers.push_back(header);
                request->setHeaders(headers);
                std::string data;
                if(parameter.size() > 0){
                    data += parameter;
                    data += "&";
                }
                data += "filename=";
                data += context->_filename.c_str();
                data += "&";
                data += "lineno=";
                std::stringstream ls;
                ls << context->_lineno;
                std::string l;
                ls >> l;
                data += l;
                data += "&";
                data += "message=";
                data += context->_message;
                request->setRequestData(data.c_str(), data.length());
                request->setTag("POST TEST");
                request->setResponseCallback(this, httpresponse_selector(ErrorReportThread::sendCallback));
                cocos2d::network::HttpClient::getInstance()->send(request);
                request->release();
            }
            
            filename = context->_filename;
            line = context->_lineno;
            message = context->_message;
            delete context;
        }
        if(!_queue.empty()){
            std::chrono::milliseconds dura(sentdelay);
            std::this_thread::sleep_for(dura);
        }else{
            std::chrono::milliseconds dura(loopdelay);
            std::this_thread::sleep_for(dura);
        }
    }
}


