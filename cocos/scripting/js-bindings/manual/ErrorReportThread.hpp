//
//  errorReportThread.hpp
//  cocos2d_js_bindings
//
//  Created by gc_lsw on 5/19/17.
//
//

#ifndef errorReportThread_hpp
#define errorReportThread_hpp

#include <stdio.h>
#include <queue>
#include <thread>
#include <mutex>
#include "jsapi.h"
#include "jsfriendapi.h"
#include "cocos2d.h"
#include "js_bindings_config.h"
#include "js_bindings_core.h"
#include "spidermonkey_specifics.h"
#include "js_manual_conversions.h"
#include "mozilla/Maybe.h"
#include "js-BindingsExport.h"
#include "extensions/cocos-ext.h"
#include "network/HttpClient.h"

using namespace std;

using namespace cocos2d;
using namespace cocos2d::network;

class ErrorReportThread : public cocos2d::Node
{
    
public:
    class ReportContext{
    public:
        ReportContext(JSErrorReport *report, const char* message){
            _lineno = (unsigned int)report->lineno;
            if(report->filename == NULL){
                _filename = "";
            }else{
                _filename = report->filename;
            }
            if(message == NULL){
                _message = "";
            }else{
                _message = message;
            }
        }
        unsigned int _lineno;
        std::string _filename;
        std::string _message;
    };
    
private:
    std::queue<ReportContext*> _queue;
    std::thread* _thread;
    std::mutex mut;
    bool stop;
public:
    ErrorReportThread();
    
    ~ErrorReportThread();
    
    static void registerJSFunction(JSContext * cx, JS::HandleObject global);
    
    static bool setParameter(JSContext* cx, uint32_t argc, jsval* vp);
    static bool setUrl(JSContext* cx, uint32_t argc, jsval* vp);
    static bool setExpectList(JSContext *cx, uint32_t argc, jsval* vp);
    static bool setSentDelay(JSContext *cx, uint32_t argc, jsval* vp);
    static bool setLoopDelay(JSContext *cx, uint32_t argc, jsval* vp);
    static bool setHeader(JSContext *cx, uint32_t argc, jsval* vp);
    void setCallback(const std::function<void(std::string filename, unsigned int line, std::string message)>& callback) {_callback = callback;};
    
    void addContext(JSErrorReport* report, const char* message);
    void sendCallback(HttpClient *sender, HttpResponse *response);

    std::function<void(std::string filename, unsigned int line, std::string message)> _callback;

private:
    void update();
    
};
#endif /* errorReportThread_hpp */
