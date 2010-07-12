/************************************************************************************\
This source file is part of the KS(X) audio library                                  *
For latest info, see http://libatres.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Kresimir Spes (kreso@cateia.com)                                  *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#include <stdio.h>
#include <xal/AudioManager.h>
#include <hltypes/harray.h>
#include <hltypes/hstring.h>

//#define _TEST_SOUND
//#define _TEST_FADE_IN
#define _TEST_FADE_OUT

#if defined _TEST_FADE_IN || defined _TEST_FADE_OUT
#include <windows.h>
#endif

int main(int argc, char **argv)
{
	xal::init();
	xal::Sound* s;
	harray<hstr> files = audiomgr->loadPathCategory("../media", "cat");
	//harray<hstr> files = audiomgr->loadPathCategory("../media", "cat", "test.");
	//harray<hstr> files = audiomgr->loadPath("..");
	//harray<hstr> files = audiomgr->loadPath("..", "test");
#ifdef _TEST_SOUND
	s = audiomgr->getSound("bark");
	//s = audiomgr->getSound("test.bark");
	//s = audiomgr->getSound("bark");
	//s = audiomgr->getSound("testbark");
	s->play();
	while (s->isPlaying()) { }
	audiomgr->setCategoryGain("cat", 0.33f);
	s->play();
	while (s->isPlaying()) { }
	audiomgr->setCategoryGain("cat", 1.0f);
	s->play();
	while (s->isPlaying()) { }
	
	//s->play(1.0f);
	//while (s->isPlaying()) { Sleep(0.01f); audiomgr->update(0.01f); }
#endif
#ifdef _TEST_FADE_IN
	s = audiomgr->getSound("wind");
	s->play(1.0f);
	for (int i = 0; i < 20; i++)
	{
		Sleep(100.0f);
		printf("T:%d P:%s F:%s\n", i, hstr(s->isPlaying()).c_str(), hstr(s->isFading()).c_str());
		audiomgr->update(0.1f);
	}
	s->stop();
#endif
#ifdef _TEST_FADE_OUT
	s = audiomgr->getSound("wind");
	s->play();
	s->stop(1.0f);
	for (int i = 0; i < 20; i++)
	{
		Sleep(100.0f);
		printf("T:%d P:%s F:%s\n", i, hstr(s->isPlaying()).c_str(), hstr(s->isFading()).c_str());
		audiomgr->update(0.1f);
	}
#endif
	
	xal::destroy();
	return 0;
}