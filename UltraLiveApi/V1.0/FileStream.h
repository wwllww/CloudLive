#ifndef FILESTEAM_H
#define FILESTEAM_H

#include "RtmpPublish.h"

VideoFileStream* CreateFLVFileStream(CTSTR lpFile, bool bBack = false);
VideoFileStream* CreateMP4FileStream(CTSTR lpFile, bool bBack = false);
VideoFileStream* CreateFLVFileStreamNew(CTSTR lpFile, bool hasAudio, bool bBack, CInstanceProcess *IntanceProcess);

#endif