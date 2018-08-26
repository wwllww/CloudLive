#ifndef METADATA_H
#define METADATA_H
#include "rtmp.h"

//big endian conversion functions
#define QWORD_BE(val) (((val>>56)&0xFF) | (((val>>48)&0xFF)<<8) | (((val>>40)&0xFF)<<16) | (((val>>32)&0xFF)<<24) | \
	(((val>>24)&0xFF)<<32) | (((val>>16)&0xFF)<<40) | (((val>>8)&0xFF)<<48) | ((val&0xFF)<<56))
#define DWORD_BE(val) (((val>>24)&0xFF) | (((val>>16)&0xFF)<<8) | (((val>>8)&0xFF)<<16) | ((val&0xFF)<<24))
#define WORD_BE(val)  (((val>>8)&0xFF) | ((val&0xFF)<<8))

#define SAVC(x)    static const AVal av_##x = AVC(#x)
#define STR2AVAL(av,str) av.av_val = str; av.av_len = (int)strlen(av.av_val)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(FCSubscribe);
SAVC(onFCSubscribe);
SAVC(createStream);
SAVC(deleteStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(secureToken);

SAVC(send);

SAVC(onMetaData);
SAVC(duration);
SAVC(width);
SAVC(height);
SAVC(videocodecid);
SAVC(videodatarate);
SAVC(framerate);
SAVC(audiocodecid);
SAVC(audiodatarate);
SAVC(audiosamplerate);
SAVC(audiosamplesize);
SAVC(audiochannels);
SAVC(stereo);
SAVC(encoder);
SAVC(fileSize);
SAVC(avc1);
SAVC(mp4a);

#endif