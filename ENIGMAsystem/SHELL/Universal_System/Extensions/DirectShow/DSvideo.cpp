/** Copyright (C) 2013 Robert B. Colton
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY {
} without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "VideoStruct.h"
#include "DSvideo.h"
#include <windows.h>
#include <dshow.h>

#include "Platforms/Win32/WINDOWSmain.h"

namespace enigma {
	vector<VideoStruct*> videoStructs;
}

namespace enigma_user {

void action_show_video(string fname, bool windowed, bool loop) {    // Initialize the COM library.
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
	   MessageBox(NULL, "Failed to initialize COM library.", "ERROR", MB_ICONERROR | MB_OK);
        return;
    }
	
	int vid = video_add(fname);
	video_set_scale(vid, true);
	video_set_fullscreen(vid, windowed);
	video_start(vid, loop);

    CoUninitialize();
}

int video_add(string fname) {
	enigma::VideoStruct* videoStruct = new enigma::VideoStruct();
    IGraphBuilder *pGraph = NULL;

    // Initialize the COM library.
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
	   MessageBox(NULL, "Failed to initialize COM library.", "ERROR", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Create the filter graph manager and query for interfaces.
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IGraphBuilder, (void **)&pGraph);
    if (FAILED(hr))
    {
		MessageBox(NULL, "Failed to create the Filter Graph Manager.", "ERROR", MB_ICONERROR | MB_OK);
        return -1;
    }
	
	
	// Build the graph.
    hr = pGraph->RenderFile(std::wstring(fname.begin(), fname.end()).c_str(), NULL);
	
	IVideoWindow *pVidWin = NULL;
	pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVidWin);
	
	pVidWin->put_Owner((OAHWND)enigma::hWnd);
	
	pVidWin->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	
	videoStruct->pGraph = pGraph;
	enigma::videoStructs.push_back(videoStruct);
	return enigma::videoStructs.size() - 1;
}

void video_delete(int id) {
	get_video(videoStruct, id);
	delete enigma::videoStructs[id];
}

bool video_exists(int id) {
	return (enigma::videoStructs[id] != NULL);
}

void video_start(int id, bool loop) {
	get_video(videoStruct, id);
	IMediaControl *pControl = NULL;
    IMediaEvent   *pEvent = NULL;
    HRESULT hr = videoStruct->pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
    hr = videoStruct->pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);
	
	// Run the graph.
    hr = pControl->Run();
    while (SUCCEEDED(hr)) {
        if (SUCCEEDED(hr)) {
            // Wait for completion.
            long evCode;
            pEvent->WaitForCompletion(INFINITE, &evCode);
        } else {
			MessageBox(NULL, "Failed to run the COM control.", "ERROR", MB_ICONERROR | MB_OK);
			// Don't return so pointers can be released.
		}
		
		if (loop) {
			video_set_seek(id, 0);
		} else {
			break;
		}
    } 
	if (FAILED(hr)) {
		MessageBox(NULL, "Failed to render the Filter Graph Manager.", "ERROR", MB_ICONERROR | MB_OK);
		// Don't return so pointers can be released.
    }

    pControl->Release();
	pEvent->Release();
}

void video_pause(int id) {

}

void video_stop(int id) {
	video_pause(id);
	video_set_seek(id, 0);
}

void video_set_seek(int id, long position) {
	get_video(videoStruct, id);
	IMediaSeeking* pSeek;
	HRESULT hr = videoStruct->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
	DWORD dwCap = 0;
	hr = pSeek->GetCapabilities(&dwCap);
		
	if (AM_SEEKING_CanSeekAbsolute & dwCap)
	{
		// Graph can seek to absolute positions.
		REFERENCE_TIME rtNow = position;
		hr = pSeek->SetPositions(
			&rtNow, AM_SEEKING_AbsolutePositioning, 
			NULL, AM_SEEKING_NoPositioning
			);
	}

	pSeek->Release();
}

void video_set_fullscreen(int id, bool fullscreen) {
	get_video(videoStruct, id);
	IVideoWindow *pVidWin = NULL;
	videoStruct->pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVidWin);
	
	pVidWin->put_FullScreenMode(-fullscreen);
}

void video_set_window_region(int id, long x, long y, long w, long h) {
	get_video(videoStruct, id);
	IVideoWindow *pVidWin = NULL;
	videoStruct->pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVidWin);
	
	pVidWin->SetWindowPosition(x, y, w, h);
}

void video_set_destination_region(int id, long x, long y, long w, long h) {
	get_video(videoStruct, id);
	IBasicVideo *pBasicVideo;
	videoStruct->pGraph->QueryInterface(IID_IBasicVideo,
	 (LPVOID *)&pBasicVideo);
	
	pBasicVideo->SetDestinationPosition(x, y, w, h);

	// release
	pBasicVideo->Release();
}

void video_set_source_region(int id, long x, long y, long w, long h) {
	get_video(videoStruct, id);
	IBasicVideo *pBasicVideo;
	videoStruct->pGraph->QueryInterface(IID_IBasicVideo,
	 (LPVOID *)&pBasicVideo);
	
	pBasicVideo->SetSourcePosition(x, y, w, h);

	// release
	pBasicVideo->Release();
}

void video_set_scale(int id, bool scale) {
	get_video(videoStruct, id);
	IVideoWindow *pVidWin = NULL;
	videoStruct->pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVidWin);
	RECT rc;
	if (scale) {
		GetClientRect(enigma::hWnd, &rc);
	} else {
	
	}
	pVidWin->SetWindowPosition(0, 0, rc.right, rc.bottom);
}

long video_get_seek(int id) {
	get_videor(videoStruct, id, -1);
	IMediaSeeking* pSeek;
	HRESULT hr = videoStruct->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
	DWORD dwCap = 0;
	hr = pSeek->GetCapabilities(&dwCap);
	
	long long position = 0;
	if (AM_SEEKING_CanSeekAbsolute & dwCap)
	{
		hr = pSeek->GetCurrentPosition(&position);
	}

	pSeek->Release();
	
	return position;
}

long video_get_duration(int id) {
	get_videor(videoStruct, id, -1);
	IMediaSeeking* pSeek;
	HRESULT hr = videoStruct->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
	DWORD dwCap = 0;
	hr = pSeek->GetCapabilities(&dwCap);
	
	long long duration = 0;
	if (AM_SEEKING_CanSeekAbsolute & dwCap)
	{
		hr = pSeek->GetDuration(&duration);
	}

	pSeek->Release();
	
	return duration;
}

long video_get_width(int id) {
	get_videor(videoStruct, id, -1);
	IBasicVideo *pBasicVideo;
	videoStruct->pGraph->QueryInterface(IID_IBasicVideo,
	 (LPVOID *)&pBasicVideo);

	// obtain width and height
	long width;
	pBasicVideo->get_VideoWidth(&width);

	// release
	pBasicVideo->Release();
	
	return width;
}

long video_get_height(int id) {
	get_videor(videoStruct, id, -1);
	IBasicVideo *pBasicVideo;
	videoStruct->pGraph->QueryInterface(IID_IBasicVideo,
	 (LPVOID *)&pBasicVideo);

	// obtain width and height
	long height;
	pBasicVideo->get_VideoHeight(&height);

	// release
	pBasicVideo->Release();
	
	return height;
}

}