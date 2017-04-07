# ofxSimpleScreenRecorder
a simple oF class which record screen to h.264 mpeg video without significant frame drops

## Requirement 
This depends on FFmpeg library to convert image sequence to video so FFmpeg should be installed. Follow the instruction on here https://trac.ffmpeg.org/wiki/CompilationGuide

## How it works
1. Set the capture size and path with setup(width, height, path). You can specify the path where video will be saved, just make sure put "/" at the last (ex. "local/somewhere/" not "local/somewhere") or simply leave it as a blank, it will use the bin folder in your oF project

2. Wrap your sketch with begin(), end() in your draw loop

3. Call start() at when you want to start capturing

4. Call stop() at when you want to stop capturing

5. Wait until rendering process done
   do not close the app during the process 
   
6. When process done, this will open your terminal and run FFmpeg command

7. Folder where the video is will be popped-up

8. Please enjoy the last step of the process, click the red circle at left top on the terminal
   
mainthread : store pixels of drawing to buffer array every bi-frame
ofthread   : save stored pixels .png to given path on local drive (bin is default)
terminal   : convert .png sequence to h.264 .mp4 with FFmpeg command and delete image sequence


```c++
ofxSimpleScreenRecorder mRenderer; 

void ofApp::setup(){
    mRenderer.setup(ofGetWidth(), ofGetHeight());
}

void ofApp::draw(){
    mRenderer.begin();
        draw something, this will be saved
    mRenderer.end();
        draw semething, this will be ignored
}

void ofApp::keyReleased(int key){
    if(key == '1')
        mRenderer.start();
    else if(key == '2')
        mRenderer.stop();
}
```

