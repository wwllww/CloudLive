#pragma once

#include "BaseAfx.h"
#include "Instance.h"
#include <functional>
#include <vector>

struct BufferedDataPacket : DataPacket
{
    template <typename Func>
	BufferedDataPacket(Func &&func) : DataPacket(), func(std::forward<Func>(func)) {}
    std::function<void(DataPacket&)> func;

    void InitBuffer()
    {
        if (size || !func)
            return;

        func(*this);
        if (!size)
            return;

        buffer.assign(lpPacket, lpPacket + size);
        lpPacket = buffer.data();
    }

private:
    std::vector<BYTE> buffer;
};

extern CInstanceProcess *G_LiveInstance;

inline BufferedDataPacket GetBufferedSEIPacket()
{
    return [](DataPacket &p)
    {
		VideoEncoder *encoder = G_LiveInstance->GetVideoEncoder();
        assert(encoder);
        encoder->GetSEI(p);
    };
};

inline BufferedDataPacket GetBufferedSEIPacket_back()
{
	return [](DataPacket &p)
	{
		VideoEncoder *encoder = G_LiveInstance->GetVideoEncoder_back();
		assert(encoder);
		encoder->GetSEI(p);
	};
};

inline BufferedDataPacket GetBufferedAudioHeadersPacket()
{
    return [](DataPacket &p)
    {
		G_LiveInstance->GetAudioHeaders(p);
    };
};


inline BufferedDataPacket GetBufferedVideoHeadersPacket()
{
    return [](DataPacket &p)
    {
		G_LiveInstance->GetVideoHeaders(p);
    };
};

inline BufferedDataPacket GetBufferedVideoHeadersPacket_Back()
{
	return[](DataPacket &p)
	{
		G_LiveInstance->GetVideoHeaders_back(p);
	};
};

