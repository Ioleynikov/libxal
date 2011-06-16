/// @file
/// @author  Ivan Vucica
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Provides a source for M4A format. Actually supports any format supported by
/// the Apple's Audio Toolbox.

#if HAVE_M4A
#ifndef XAL_M4A_SOURCE_H
#define XAL_M4A_SOURCE_H

#include <AudioToolbox/AudioToolbox.h>

#include <hltypes/hstring.h>

#include "AudioManager.h"
#include "Source.h"
#include "xalExport.h"

namespace xal
{
	class xalExport M4A_Source : public Source
	{
	public:
		M4A_Source(chstr filename);
		~M4A_Source();

		bool open();
		void close();
		void rewind();
		bool load(unsigned char* output);
		int loadChunk(unsigned char* output, int size = STREAM_BUFFER_SIZE);

	protected:
		AudioFileID audioFileID;
		UInt32 chunkOffset;
		
		void _readFileProps();

	};

}

#endif
#endif