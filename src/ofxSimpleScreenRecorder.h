#pragma once
#include "ofMain.h"

/*
 * ofxSimpleScreenRecorder.h
 *
 * a simple oF class which record screen to h.264 mpeg video without significant frame drops.
 *
 * ofxSimpleScreenRecorder would be useful when you document your sketch, Documentation is a frustrating process especially if the recorder causes damm frame drops. Those frame drops usually comes from saving process, recorder waits until last frame saved in order not to miss next frame of animation. ofxSimpleScreenRecorder saves frames to buffer and save imgages in buffer apart from main app's update so you won't feel any frame drops while you recording your sketch, what you see is what you will have.
 *
 *
 *
 * - REQUIREMENT
 *
 * ofxSimpleScreenRecorder depends on FFmpeg library to convert image sequence to video so FFmpeg should be installed. Follow the instruction on here https://trac.ffmpeg.org/wiki/CompilationGuide
 *
 *
 *
 * - USAGE,
 *
 * a. set the capture size and path with setup(width, height, path).
 *    you can specify the path where video will be saved, just make sure put "/" at the last
 *    ex. "local/somewhere/" not "local/somewhere"
 *    or simply leave it as a blank, it will use the bin folder in your oF project
 *
 * b. wrap your sketch with begin(), end() in your draw loop
 *    ex.
 *    begin();
 *          draw something, this will be saved
 *    end();
 *          draw something, this will be ignored
 *
 * c. call start() at when you want to start capturing.
 *
 * d. call stop() at when you want to stop capturing.
 *
 * e. wait until rendering process done.
 *    do not close the app during the process
 *
 * f. when process done, ofxSimpleScreenRecorder will open your terminal and run FFmpeg command
 *
 * g. folder where the video is will be popped-up
 *
 * h. please enjoy the last step of the process, click the red circle at left top on the terminal
 *
 * *. mainthread : store pixels of drawing to buffer array every bi-frame
 * *. ofthread   : save stored pixels png to given path (bin is default) on local drive
 * *. terminal : convert .png sequence to h.264 .mp4 with FFmpeg command and delete image sequence
 *
 *
 *
 * - TODOs
 * a. integrate FFmpeg library into code to convert buffer directly to video without process saving images to local storage and also without process openning terminal
 * b. Support different OS (Currently only supports OSX / tested on OSX El Capitan & oF_v0.9.7)
 *
 *
 *
 * created on Apr, 2017 at fakelove http://www.fakelove.tv/
 * by av http://avseoul.net http://kimsehyun.kr
 *
 * github https://github.com/avseoul/ofxSimpleScreenRecorder
 *
 */

class ofxSimpleScreenRecorder : public ofThread{
public:
    ofxSimpleScreenRecorder(){}
    ~ofxSimpleScreenRecorder(){
        waitForThread(true);
    }
    
    void setup(int _w, int _h, std::string _path = ""){
        w = _w, h = _h, bffrCount = 0, thrdBffrCount = 0, saveCount = 0;
        initBuffer();
        
        pixels.resize(numBffr);
        for(int i = 0; i < numBffr; i++)
            pixels[i] = ofPixels();
        
       // path = _path == "" ? ofDirectory::ofDirectory(_path).getAbsolutePath() + "/" : _path;
       path = _path == "" ? ofDirectory(_path).getAbsolutePath() + "/" : _path;
        std::cout << "ofxSSR::target path - " << path << endl;
        
        isRunning = false;
        isInit = true;
        stopTriggered = false;
    }
    
    void start(){
        if(!isRunning){
            reset();
            startThread();
            isRunning = true;
        } else
            std::cout << "ofxSSR::thread running" << endl;
    }
    
    void stop(){
        if(isRunning)
            stopTriggered = true;
        else
            std::cout << "ofxSSR::thread not running" << endl;
    }
    
    void begin(){
        fbo.begin();
        ofClear(0);
    }
    
    void end(){
        if(stopTriggered) showWarning();
        fbo.end();
        fbo.draw(0,0);
        
        bool _save = saveCount%2 == 0 ? true : false;
        if(_save && isRunning && !stopTriggered){
            if(!isInit)
                bffr_ping.unmap();
            
            isInit = false;
            
            fbo.getTexture().copyTo(bffr_ping);
            
            bffr_pong.bind(GL_PIXEL_UNPACK_BUFFER);
            unsigned char * p = bffr_pong.map<unsigned char>(GL_READ_ONLY);
            pixels[bffrCount].setFromPixels(p,w,h,OF_PIXELS_RGB);
            std::swap(bffr_ping, bffr_pong);
            
            bffrCount++;
            
            if(bffrCount >= numBffr) {
                std::cout<<"ofxSSR::buffer reaches limit - stopping rendering" << endl << "use setNumBffr(int _num) to increase limit. current value is " << numBffr << endl;
                stop();
            }
        }
        saveCount++;
        
        if(stopTriggered && bffrCount-1 <= thrdBffrCount){
            isRunning = false;
            runFFMpeg();
            reset();
            stopThread();
        }
    }
    
    void setNumBffr(int _num){
        numBffr = _num;
    }
    
private:
    int w, h, bffrCount, thrdBffrCount, saveCount, numBffr = 9999;
    std::string path;
    ofFbo fbo;
    ofBufferObject bffr_ping, bffr_pong;
    vector<ofPixels> pixels;
    bool isRunning, isInit, stopTriggered;
    
    void initBuffer(){
        fbo.allocate(w,h,GL_RGB);
        bffr_ping.allocate(w*h*3,GL_DYNAMIC_READ);
        bffr_pong.allocate(w*h*3,GL_DYNAMIC_READ);
    }
    
    void reset(){
        saveCount = 0;
        thrdBffrCount = 0;
        bffrCount = 0;
        stopTriggered = false;
    }
    
    void showWarning(){
        std::ostringstream warning;
        warning << "//////////////////////////" << endl << "// DO NOT CLOSE THE APP //" << endl << "//////////////////////////" << endl << endl << "PROCESS - " << getProgress() << "%";
        ofPushStyle();
        {
            ofSetColor(255,255,0);
            ofBeginShape();
            ofVertex(0, h/2.f-50.f, 0), ofVertex(0, h/2.f+50.f, 0), ofVertex(w, h/2.f+50.f, 0), ofVertex(w, h/2.f-50.f, 0);
            ofEndShape();
            ofSetColor(255,10,100);
            ofDrawBitmapString(warning.str(), w/2.f-90.f, h/2.f-50.f+27.f);
        }
        ofPopStyle();
        warning.str("");
    }
    
    std::string getPath(std::string _r, int _i){
        std::ostringstream _n;
        _n << setw(4) << setfill('0') << _i;
        std::string _p =  _r + "render_" + _n.str() + ".png";
        return _p;
    }
    
    std::string rmfiles(){
        std::string _s = "cd " + path + " && rm -rf ";
        for(int i = 0; i < thrdBffrCount+1; i++){
            std::ostringstream _n;
            _n << setw(4) << setfill('0') << i;
            std::string _p =  "render_" + _n.str() + ".png ";
            _s += _p;
        }
        return _s;
    }
    
    void runFFMpeg(){
        std::string _tstamp = ofGetTimestampString();
        std::string _rmfiles = rmfiles();
        
        /*
         * TODO - get diff command based on os
         *
         * if is linux
         *        std::string _cmd = "cd " + path + " && ffmpeg -start_number 1 -framerate 30 -i render_%04d.png -c:v libx264 -vf fps=30 -pix_fmt yuv420p video"+_tstamp+".mp4";
         * else if is mac open damm terminal
         * check this thread out
         * https://forum.openframeworks.cc/t/launching-and-configuring-terminal-window-from-of-application/18236/10
         */
        std::string _cmd = "osascript -e 'tell application \"Terminal\" to do script \"cd " + path + " && ffmpeg -start_number 1 -framerate 30 -i render_%04d.png -c:v libx264 -vf fps=30 -pix_fmt yuv420p -preset ultrafast video"+_tstamp+".mp4 && " + _rmfiles + " && open . && exit\"'";
        std::system(_cmd.c_str());
        std::cout << "ofxSSR::rendering completed" << endl;
    }
    
    void threadedFunction(){
        std::cout << "ofxSSR::thread initiated" << endl;
        ofSleepMillis(1000);
        std::cout << "ofxSSR::thread started" << endl;
        
        while(isRunning){
            std::cout<< "ofxSSR::saved buffers - "  << thrdBffrCount << " ea / stored buffers - " << bffrCount-1 << " ea" << endl;
            if(stopTriggered)
                std::cout<< "ofxSSR::rendering process - " << getProgress() << "%" << endl << endl << "//////////////////////////" << endl << "// DO NOT CLOSE THE APP //" << endl << "//////////////////////////" << endl << endl;
            
            std::string _p = getPath(path, thrdBffrCount);
            ofSaveImage(pixels[thrdBffrCount],_p);
            
            thrdBffrCount++;
        }
        std::cout << "ofxSSR::thread ended" << endl;
    }
    
    float getProgress(){
        return std::floor((float)thrdBffrCount/(float)(bffrCount-1.f)*10000.f)*.01f;
    }
};
